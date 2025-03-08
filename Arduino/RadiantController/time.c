#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"
#include "time.h"

#include "f007th.h"
#include "uart.h"
#include "led.h"

static long tick = 0;
static uint8_t subtick = 0;

// Timer 2 overflow interrupt
ISR(TIMER2_OVF_vect)
{
  unsigned char rf_io = RF_GPIO;
  unsigned char uart2400_io = UART2400_GPIO;
  uart2400_sample(uart2400_io);
  f007th_sample(rf_io);
  subtick++;
  if (subtick==10) {
    subtick=0;
    tick++;
  }
}

long time_get(void)
{
  return tick;
}

int time_elapsed_mSec(long t0)
{
  long mSec = tick - t0;
  return (int) mSec;
}

int time_elapsed_sec(long t0)
{
  long mSec = tick - t0;
  return (int) (mSec / 1000);
}

long time_secondsToTicks(int seconds)
{
  return ((long) seconds) * ((long) 1000);
}

void time_init(void)
{
  // Create a near 1 mSec system timer tick and 100 usec subtick
  //
  // Use fast PWM mode with TOP = OCR2A in order to create a TOV2 interrupt
  // every 100 uSec.  This is mode WGM0[2:0] = 7.  Set clock select = 3 for 
  // CLKio / 64.

#if 0
  OCR2A = (F_CPU / 10000) / 64;  // 25 for 16MHz system clock
#else
  // 1/17/2025
  // For some reason I needed to change this for the Chinese knock-off Nanos
  // in order to get the correct sampling rate.
  OCR2A = (F_CPU /  5000) / 64;
#endif
  TIMSK2 = _BV(TOIE0);  // set overflow interrupt enable
  TCCR2A = 
    (3 << WGM00); // WGM0[1:0] = b11
  TCCR2B = 
    (1 << WGM02) |
    (3 << CS00);
}


