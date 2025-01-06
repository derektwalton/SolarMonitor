#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"
#include "fetswitch.h"

//
// Pin mapping:
//
// FETSWITCH0 - PD7
// FETSWTICH1 - PB0
// FETSWITCH2 - PB1
//
// DDRx  - controls data direction (0 = input, no drive, 1 = output, drive)
// PORTx - controls value out output (0 = lo, 1 = hi)
// PINx  - shows value on port (0 = lo, 1 = hi)
//

void fetSwitchOn(FETSWITCH_t n)
{
  switch(n)
    {
    case FETSWITCH_0:
      PORTD |= _BV(PD7); // PD7 -> 1
      break;
    case FETSWITCH_1:
      PORTB |= _BV(PB0);
      break;
    case FETSWITCH_2:
      PORTB |= _BV(PB1);
      break;
    }
}

void fetSwitchOff(FETSWITCH_t n)
{
  switch(n)
    {
    case FETSWITCH_0:
      PORTD &= ~_BV(PD7); // PD7 -> 0
      break;
    case FETSWITCH_1:
      PORTB &= ~_BV(PB0);
      break;
    case FETSWITCH_2:
      PORTB &= ~_BV(PB1);
      break;
    }
}


void fetSwitchInit()
{
  // Output low (FETs off)
  PORTD &= ~_BV(PD7); // bitwise AND with b01111111 (ie. PD7 -> 0)
  PORTB &= ~_BV(PB0);
  PORTB &= ~_BV(PB1);

  // Set data direction to output (ie. drive the pins)
  DDRD |= _BV(PD7); // bitwise OR with b10000000
  DDRB |= _BV(PB0); // bitwise OR with b00000001
  DDRB |= _BV(PB1); // bitwise OR with b00000010
}
