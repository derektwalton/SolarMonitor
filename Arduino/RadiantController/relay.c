#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"
#include "relay.h"

//
// Pin mapping:
//
// RELAY0 - PB2
// RELAY1 - PB3
// RELAY2 - PB4
//
// DDRx  - controls data direction (0 = input, no drive, 1 = output, drive)
// PORTx - controls value out output (0 = lo, 1 = hi)
// PINx  - shows value on port (0 = lo, 1 = hi)
//

void relayOn(RELAY_t n)
{
  switch(n)
    {
    case RELAY_0:
      PORTB |= _BV(PB2); // PB2 -> 1
      break;
    case RELAY_1:
      PORTB |= _BV(PB3);
      break;
    case RELAY_2:
      PORTB |= _BV(PB4);
      break;
    }
}

void relayOff(RELAY_t n)
{
  switch(n)
    {
    case RELAY_0:
      PORTB &= ~_BV(PB2); // PB2 -> 0
      break;
    case RELAY_1:
      PORTB &= ~_BV(PB3);
      break;
    case RELAY_2:
      PORTB &= ~_BV(PB4);
      break;
    }
}


void relayInit(void)
{
  // Output low (FETs off)
  PORTB &= ~_BV(PB2); // bitwise AND with b01111011 (ie. PB2 -> 0)
  PORTB &= ~_BV(PB3);
  PORTB &= ~_BV(PB4);

  // Set data direction to output (ie. drive the pins)
  DDRB |= _BV(PB2); // bitwise OR with b00000100
  DDRB |= _BV(PB3); // bitwise OR with b00001000
  DDRB |= _BV(PB4); // bitwise OR with b00010000
}
