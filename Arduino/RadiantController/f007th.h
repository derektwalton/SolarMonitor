#ifndef __f007th_h__
#define __f007th_h__

#include <stdint.h>

#define RF_GPIO (PINC & _BV(PC2))

// call at least every 5 mSec
void f007th_poll(void);

// call every 100 usec (10 kHz) with value of GPIO pin connected to 433MHz receiver
void f007th_sample(unsigned sample);

// callback to receive decoded temperature, etc.
void f007th_callback(uint8_t channel, int16_t tempx10, uint8_t humidity, uint8_t low_battery);

#endif
