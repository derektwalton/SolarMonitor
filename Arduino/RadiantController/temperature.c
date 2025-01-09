#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"

#include <util/delay.h>

#include "temperature.h"
#include "onewire.h"
#include "led.h"
#include "time.h"
#include "f007th.h"

//#define DEBUG_TEMPERATURE_ONEWIRE
//#define DEBUG_TEMPERATURE_F007TH

double gTemperature[N_TEMPS];

// ----------------------------------------------------------------------------------------------------------
// OneWire temperature sensors
// ----------------------------------------------------------------------------------------------------------

static int initDone = 0;

typedef struct {
  TEMPERATURE_t location;
  unsigned char oneWireAddr[8];
  char foundIt;
  unsigned char failCount;
} TEMP_SENSOR_t;

#define MAX_FAIL_COUNT 10

TEMP_SENSOR_t sensors[] =
{
#if (TEMPERATURE_NO_1ST_2ND_ONEWIRE==0)
  { TEMPERATURE_1stFloor,      { 0x28, 0xbc, 0x03, 0xec, 0x02, 0x00, 0x00, 0x3e }, 1, MAX_FAIL_COUNT },
  { TEMPERATURE_2ndFloor,      { 0x28, 0xd4, 0x1f, 0xec, 0x02, 0x00, 0x00, 0x01 }, 1, MAX_FAIL_COUNT },
#endif
  { TEMPERATURE_Basement,      { 0x28, 0xf9, 0x26, 0xec, 0x02, 0x00, 0x00, 0x8a }, 1, MAX_FAIL_COUNT },
  { TEMPERATURE_Outside,       { 0x28, 0x92, 0x2b, 0xec, 0x02, 0x00, 0x00, 0x5d }, 1, MAX_FAIL_COUNT },
  { TEMPERATURE_GlycolExchIn,  { 0x28, 0xc2, 0x37, 0xec, 0x02, 0x00, 0x00, 0x2e }, 1, MAX_FAIL_COUNT },
  { TEMPERATURE_GlycolExchMid, { 0x28, 0xe8, 0x1c, 0xec, 0x02, 0x00, 0x00, 0xdf }, 1, MAX_FAIL_COUNT },
  { TEMPERATURE_GlycolExchOut, { 0x22, 0xb7, 0x8c, 0x24, 0x00, 0x00, 0x00, 0x83 }, 1, MAX_FAIL_COUNT },
  { TEMPERATURE_TankHotOut,    { 0x28, 0x2e, 0x38, 0xec, 0x02, 0x00, 0x00, 0x15 }, 1, MAX_FAIL_COUNT },
  { TEMPERATURE_RadiantReturn, { 0x28, 0x40, 0x04, 0xec, 0x02, 0x00, 0x00, 0x60 }, 1, MAX_FAIL_COUNT },
  { TEMPERATURE_HotTubReturn,  { 0x28, 0xc5, 0x3f, 0xec, 0x02, 0x00, 0x00, 0x95 }, 1, MAX_FAIL_COUNT },
};

#define NSENSORS (sizeof(sensors)/sizeof(TEMP_SENSOR_t))

static void setUnknown_onewire(void)
{
  int i;
  for(i=0;i<NSENSORS;i++) {
    sensors[i].failCount++;
    if (sensors[i].failCount > MAX_FAIL_COUNT) {
      gTemperature[sensors[i].location] = TEMPERATURE_UNKNOWN;
    }
  }
}

static int temperature_onewire_Init(void)
{
  setUnknown_onewire();

#if ONEWIRE_SEARCH_ENABLE
  {
    int i;
	for(i=0;i<NSENSORS;i++)
      sensors[i].foundIt = 0;
  }
#endif

  onewire_init();

  if ( !onewire_powerUp() ) {
    return FAIL;
  }
  _delay_ms(10);
  
  if ( !onewire_reset() ) {
    return FAIL;
  }
    
#if ONEWIRE_SEARCH_ENABLE
    
  {
    unsigned char addr[8];
    int j,match;
    
    for(i=0;i<NSENSORS;i++)
      sensors[i].foundIt = 0;
    
    //dc_log_printf("finding temp sensors...");
    while(onewire_search(addr)) {
      for(i=0;i<NSENSORS;i++) {
	if (sensors[i].foundIt) continue;
	match = 1;
	for(j=0;j<8;j++) {
	  if (addr[j]!=sensors[i].oneWireAddr[j]) {
	    match = 0;
	    break;
	  }
	}
	if (!match) continue;
	sensors[i].foundIt = 1;
	//dc_log_printf("found sensor %s",sensors[i].name);
	break;
      }
#if 0
      if (!match) {
	dc_log_printf("unknown sensor:");
	dc_log_printf(" %0.2x%0.2x%0.2x%0.2x %0.2x%0.2x%0.2x%0.2x",
		      addr[0],
		      addr[1],
		      addr[2],
		      addr[3],
		      addr[4],
		      addr[5],
		      addr[6],
		      addr[7]);
      }
#endif
    }
  }
#endif

  return SUCCESS;
}


#define CONVERT_T        0x44
#define READ_SCRATCHPAD  0xbe


static int temperature_onewire_UpdateTemperatures(void)
{
  int rCode;
  int i,j;
  unsigned char scratchPad[9];
  int itempC;
  double ftempC,ftempF;

#ifdef DEBUG_TEMPERATURE_ONEWIRE
  debug_puts("d: temperature_onewire_UpdateTemperatures\n");
#endif
  
  if (!initDone) {
    rCode = temperature_onewire_Init();
    if (rCode != SUCCESS) {
#ifdef DEBUG_TEMPERATURE_ONEWIRE
      debug_puts("d:   call to temperature_onewire_Init fail\n");
#endif
      onewire_powerDown();
      return(rCode);
    }
    initDone = 1;
  } else {
    if ( !onewire_powerUp() ) {
      return FAIL;
    }
    _delay_ms(10);
  }

  // tell all temperature chips to convert temperature
  if (!onewire_reset()) {
#ifdef DEBUG_TEMPERATURE_ONEWIRE
    debug_puts("d:   call to onewire_reset fail\n");
#endif
    onewire_powerDown();
    setUnknown_onewire();
    return(FAIL);
  }
  onewire_skip();
  onewire_write(CONVERT_T);

  // wait for temperature chips to complete converting temperature
#if ONEWIRE_PARASITIC_POWER
  _delay_ms(1000);
#else
  do {
    int r = onewire_readBit();
  } while(r==0);
#endif  

  // poll all temperatures
#ifdef DEBUG_TEMPERATURE_ONEWIRE
  debug_puts("d:   temperatures:\n");
#endif
  for(i=0;i<NSENSORS;i++) {
    if (!sensors[i].foundIt) 
      continue;
    if (!onewire_reset()) {
#ifdef DEBUG_TEMPERATURE_ONEWIRE
      debug_puts("d:   call to onewire_reset fail\n");
#endif
      onewire_powerDown();
      setUnknown_onewire();
      return(FAIL);
    }
    onewire_select(sensors[i].oneWireAddr);
    onewire_write(READ_SCRATCHPAD);
    for(j=0;j<9;j++) {
      scratchPad[j] = onewire_read();
    }
    if (scratchPad[8] != onewire_crc8(scratchPad,8)) {
      if (sensors[i].failCount >= MAX_FAIL_COUNT) {
	gTemperature[sensors[i].location] = TEMPERATURE_UNKNOWN;
      } else {
	sensors[i].failCount++;
      }
#ifdef DEBUG_TEMPERATURE_ONEWIRE
      {
	char s[64];
	sprintf(s,"d:   onewire crc error location %d\n",sensors[i].location);
	debug_puts(s);
      }
#endif
    } else {
      itempC = scratchPad[0] + (scratchPad[1]<<8);
      ftempC = ((double) itempC) * 0.0625;
      ftempF = ftempC * 9.0 / 5.0 + 32.0;
#ifdef DEBUG_TEMPERATURE_ONEWIRE
      {
	char s[64];
	sprintf(s,"d:   onewire %5.11f location %d\n",ftempF,sensors[i].location);
	debug_puts(s);
      }
#endif
      if (gTemperature[sensors[i].location] != TEMPERATURE_UNKNOWN) {
	if (ftempF > (gTemperature[sensors[i].location] + 5.0))
	  ftempF += 0.5;
	if (ftempF < (gTemperature[sensors[i].location] - 5.0))
	  ftempF -= 0.5;
      }
      gTemperature[sensors[i].location] = ftempF;
      sensors[i].failCount = 0;
    }
  }

  onewire_powerDown();
  return(SUCCESS);
}


// ----------------------------------------------------------------------------------------------------------
// F007TH temperature sensors
// ----------------------------------------------------------------------------------------------------------

typedef struct {
  TEMPERATURE_t location;
  uint8_t channel;
  long tUpdate;
} F007TH_SENSOR_t;

F007TH_SENSOR_t f007th_sensors[] =
{
  { TEMPERATURE_1stFloor, 1, 0},
  { TEMPERATURE_2ndFloor, 2, 0},
  { TEMPERATURE_RootCellar, 3, 0},
};

#define F007TH_NSENSORS (sizeof(f007th_sensors)/sizeof(F007TH_SENSOR_t))

void f007th_callback(uint8_t channel, int16_t tempx10, uint8_t humidity, uint8_t low_battery)
{
  int i;
  for(i=0;i<F007TH_NSENSORS;i++) {
    if (f007th_sensors[i].channel==channel) {
      gTemperature[f007th_sensors[i].location] = ((double)tempx10) / 10.0;
      f007th_sensors[i].tUpdate = time_get();
      return;
    }
  }
}

int temperature_f007th_init()
{
  int i;
  for(i=0;i<F007TH_NSENSORS;i++) {
    gTemperature[f007th_sensors[i].location] = TEMPERATURE_UNKNOWN;
  }
  return(SUCCESS);
}

int temperature_f007th_update()
{
  int i;

#ifdef DEBUG_TEMPERATURE_F007TH
  debug_puts("d: f007th update temperatures\n");
#endif
  
  // process incoming bitstream looking for temperature update packets
  f007th_poll();

  // if no update in 10 mins then mark temperature as unknown
  for(i=0;i<F007TH_NSENSORS;i++) {
    if ( time_elapsed_sec(f007th_sensors[i].tUpdate) > 10*60 ) {
      gTemperature[f007th_sensors[i].location] = TEMPERATURE_UNKNOWN;
    }
  }
  
  return(SUCCESS);
}


// ----------------------------------------------------------------------------------------------------------
// Overall control
// ----------------------------------------------------------------------------------------------------------

int temperature_Init(void)
{
  temperature_onewire_Init();
  temperature_f007th_init();
  return(SUCCESS);
}

int temperature_UpdateTemperatures(void)
{
  uint8_t i;
  
  for(i=0;i<TEMPERATURE__last;i++)
    if (gTemperature[i] == TEMPERATURE_UNKNOWN)
      break;
  if (i<TEMPERATURE__last) {
    ledOff(LED_ONEWIRE_STATUS_OK);
  } else {
    ledOn(LED_ONEWIRE_STATUS_OK);
  }

  // do 1-wire temps for 2 mins, then f007th temps for 2 mins
  if (time_elapsed_sec(0) / (2*60) % 2)
    temperature_f007th_update();
  else
    temperature_onewire_UpdateTemperatures();
  return(SUCCESS);
}
