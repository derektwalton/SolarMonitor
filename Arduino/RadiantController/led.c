#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"
#include "led.h"

//
// Pin mapping:
//
// LED9  - PD2
// LED10 - PD3
// LED11 - PD4
// LED12 - PB5
//
// DDRx  - controls data direction (0 = input, no drive, 1 = output, drive)
// PORTx - controls value out output (0 = lo, 1 = hi)
// PINx  - shows value on port (0 = lo, 1 = hi)
//

int ledOnPortD2;

void ledOn(LED_t n)
{
  switch(n)
    {
    case LED_9:
#if D2_USED_BY_ONEWIRE
#else
      DDRD |= _BV(PD2);
#endif
      break;
    case LED_10:
#if D3_USED_BY_F007TH
#else
      DDRD |= _BV(PD3);
#endif
      break;
    case LED_11:
      DDRD |= _BV(PD4);
      break;
    case LED_12:
      DDRB |= _BV(PB5);
      break;
    }
}

void ledOff(LED_t n)
{
  switch(n)
    {
    case LED_9:
#if D2_USED_BY_ONEWIRE
#else
      DDRD &= ~_BV(PD2);
#endif
      break;
    case LED_10:
#if D3_USED_BY_F007TH
#else
      DDRD &= ~_BV(PD3);
#endif
      break;
    case LED_11:
      DDRD &= ~_BV(PD4);
      break;
    case LED_12:
      DDRB &= ~_BV(PB5);
      break;
    }
}


void ledInit()
{
  // Output high (for LED on)
#if D2_USED_BY_ONEWIRE
#else
  PORTD |= _BV(PD2);
#endif
#if D3_USED_BY_F007TH
#else
  PORTD |= _BV(PD3);
#endif
  PORTD |= _BV(PD4);
  PORTB |= _BV(PB5);
}
