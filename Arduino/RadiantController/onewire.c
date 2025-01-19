#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"

#include <util/delay.h>

#include "onewire.h"
#include "led.h"

//#define DEBUG_ONEWIRE

#define TRUE 1
#define FALSE 0

//
// Signal                          Arduino Nano Pin         ATmega328 Port
// ------------------------------------------------------------------------
// 1 wire data bus 1
//    tri-state in                       D5                     D5
//    tri-state out                      D6                     D6
//    tri-state oe_n                     D2                     D2
//
// 1/4 of 74125 quad transceiver used to drive onewire
//

#define DIRECT_READ         ((PIND & _BV(PD5)) ? 1 : 0)
#if D2_USED_BY_ONEWIRE
#define DIRECT_MODE_INPUT   (PORTD |= _BV(PD2))
#define DIRECT_MODE_OUTPUT  (PORTD &= ~_BV(PD2))
#else
#error Need pin to onewire tristate oe
#endif
#define DIRECT_WRITE_LOW    (PORTD &= ~_BV(PD6))
#define DIRECT_WRITE_HIGH   (PORTD |= _BV(PD6))

void onewire_init(void)
{
  DDRD &= ~_BV(PD5); // in
  DDRD |= _BV(PD6);  // out
  DDRD |= _BV(PD2);  // oe
  onewire_resetSearch();
}

//
// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//

int onewire_reset(void)
{
  int r = 0;
  int retries = 125;

  // wait until the wire is high 
  DIRECT_MODE_INPUT;
  do {
    if (--retries == 0) {
#ifdef DEBUG_ONEWIRE
      debug_puts("d: onewire_reset() bus not high\n");
#endif
      return 0;
    }
    _delay_us(2);
  } while ( !DIRECT_READ );  
  
  // pull wire low for 500 uSec
  DIRECT_WRITE_LOW;
  DIRECT_MODE_OUTPUT;
  _delay_us(500);
#ifdef DEBUG_ONEWIRE
  if (DIRECT_READ)
    debug_puts("d: onewire_reset() bus not low\n");
#endif
  
  // float wire and then read wire
  DIRECT_MODE_INPUT;
  _delay_us(80);
  r = !DIRECT_READ;
  _delay_us(420);
#ifdef DEBUG_ONEWIRE
  if (!r) debug_puts("d: onewire_reset() no devices present\n");
#endif
  
  return r;
}

//
// Write a bit.
//
static void onewire_writeBit(int v)
{
  cli(); // disable interrupts
  if (v & 1) {
    DIRECT_WRITE_LOW;
    DIRECT_MODE_OUTPUT;
    _delay_us(10);
    DIRECT_WRITE_HIGH;
    _delay_us(55);
  } else {
    DIRECT_WRITE_LOW;
    DIRECT_MODE_OUTPUT;
    _delay_us(65);
    DIRECT_WRITE_HIGH;
    _delay_us(5);
  }
  sei(); // enable interrupts
}

//
// Read a bit.
//
int onewire_readBit(void)
{
  int r = 0;
  cli(); // disable interrupts
  DIRECT_MODE_OUTPUT;
  DIRECT_WRITE_LOW;
  _delay_us(3);
  DIRECT_MODE_INPUT;
  //_delay_us(9);
  _delay_us(9+5); // slow down read to give more time for heavy loaded net to settle
  r = DIRECT_READ;
  _delay_us(53-5);
  sei(); // enable interrupts
  return r;
}

//
// Write a byte.
//
void onewire_write(int v)
{
  int bitMask;

  for (bitMask = 0x01; bitMask & 0xff; bitMask <<= 1) {
    onewire_writeBit( (bitMask & v) ? 1 : 0 );
  }
}

//
// Read a byte
//
int onewire_read() 
{
  int bitMask;
  int r = 0;

  for (bitMask = 0x01; bitMask & 0xff; bitMask <<= 1) {
    if ( onewire_readBit() ) r |= bitMask;
  }

  return r;
}

//
// Do a ROM select
//
void onewire_select(unsigned char *rom)
{
  int i;
  onewire_write(0x55); // choose ROM
  for( i = 0; i < 8; i++) onewire_write(rom[i]);
}

//
// Do a ROM skip
//
void onewire_skip()
{
  onewire_write(0xCC); // Skip ROM
}

int onewire_powerUp(void)
{
#if ONEWIRE_PARASITIC_POWER
  // drive data line high
  DIRECT_MODE_OUTPUT;
  DIRECT_WRITE_HIGH;
  if (!DIRECT_READ) {
#ifdef DEBUG_ONEWIRE
    debug_puts("d: onewire_powerUp() not high\n");
#endif
    return 0;
  }
  return 1;
#endif
}

void onewire_powerDown(void)
{
#if ONEWIRE_PARASITIC_POWER
  // drive data line low
  DIRECT_MODE_OUTPUT;
  DIRECT_WRITE_LOW;
#endif
}

//
// Compute 8 bit CRC directly.
//
unsigned char onewire_crc8( unsigned char *d, int len )
{
  unsigned char crc = 0;
  
  while (len--) {
    unsigned char inbyte = *d++;
    for (int i = 8; i; i--) {
      unsigned char mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}

typedef struct {
  int LastDiscrepancy;
  int LastDeviceFlag;
  unsigned char rom[8];
} ONEWIRE_t;

static ONEWIRE_t ow;

void onewire_resetSearch()
{
  int i;

  // reset the search state
  ow.LastDiscrepancy = 0;
  ow.LastDeviceFlag = FALSE;
  for(i=0;i<8;i++) {
    ow.rom[i] = 0;
  }
}

//
// Perform a search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use onewire_resetSearch() to
// start over.
//
int onewire_search(unsigned char *addr)
{
  int id_bit_number;
  int last_zero, rom_byte_number, search_result;
  int id_bit, cmp_id_bit;
  int rom_byte_mask, search_direction;

  // initialize for search
  id_bit_number = 1;
  last_zero = 0;
  rom_byte_number = 0;
  rom_byte_mask = 1;
  search_result = 0;

  // if the last call was not the last one
  if (!ow.LastDeviceFlag) {

    // 1-Wire reset
    if (!onewire_reset()) {
      // reset the search
      ow.LastDiscrepancy = 0;
      ow.LastDeviceFlag = FALSE;
      return FALSE;
    }

    // issue the search command
    onewire_write(0xF0);

    // loop to do the search
    do {

      // read a bit and its complement
      id_bit = onewire_readBit();
      cmp_id_bit = onewire_readBit();

      // check for no devices on 1-wire
      if ((id_bit == 1) && (cmp_id_bit == 1)) {
	//dc_log_printf("no devices on one wire!?");
	break;
      }
      else {

	// all devices coupled have 0 or 1
	if (id_bit != cmp_id_bit)
	  search_direction = id_bit;  // bit write value for search

	else {

	  // if this discrepancy if before the Last Discrepancy
	  // on a previous next then pick the same as last time
	  if (id_bit_number < ow.LastDiscrepancy)
	    search_direction = ((ow.rom[rom_byte_number] & rom_byte_mask) > 0);
	  else
	    // if equal to last pick 1, if not then pick 0
	    search_direction = (id_bit_number == ow.LastDiscrepancy);

	  // if 0 was picked then record its position in LastZero
	  if (search_direction == 0) {
	    last_zero = id_bit_number;
	  }
	}

	// set or clear the bit in the ROM byte rom_byte_number
	// with mask rom_byte_mask
	if (search_direction == 1) {
	  //dc_log_printf(" bit = 1");
	  ow.rom[rom_byte_number] |= rom_byte_mask;
	}
	else {
	  //dc_log_printf(" bit = 0");
	  ow.rom[rom_byte_number] &= ~rom_byte_mask;
	}

	// serial number search direction write bit
	onewire_writeBit(search_direction);

	// increment the byte counter id_bit_number
	// and shift the mask rom_byte_mask
	id_bit_number++;
	rom_byte_mask <<= 1;

	// if the mask is 0x100 then go to new SerialNum byte rom_byte_number and reset mask
	if (! (rom_byte_mask & 0xff) )
	  {
	    //dc_log_printf(" byte = %0.2x",ow.rom[rom_byte_number]);
	    rom_byte_number++;
	    rom_byte_mask = 1;
	  }
      }
    }
    while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

    // if the search was successful then
    if (!(id_bit_number < 65)) {

      // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
      ow.LastDiscrepancy = last_zero;

      // check for last device
      if (ow.LastDiscrepancy == 0)
	ow.LastDeviceFlag = TRUE;

      search_result = TRUE;
    }
  }

  // if no device found then reset counters so next 'search' will be like a first
  if (!search_result || !ow.rom[0]) {
    ow.LastDiscrepancy = 0;
    ow.LastDeviceFlag = FALSE;
    search_result = FALSE;
  }

  for (int i = 0; i < 8; i++) 
    addr[i] = ow.rom[i];

  return search_result;
}
