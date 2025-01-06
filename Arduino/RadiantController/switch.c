#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"
#include "adc.h"
#include "switch.h"


//
//
//              ADC4
//               /------- 1K ----- SW1 ----- GND
// +5 ---- 1K ---|
//               \------- 2K ----- SW2 ----- GND
//
//              ADC7
//               /------- 1K ----- SW3 ----- GND
// +5 ---- 1K ---|
//               \------- 2K ----- SW4 ----- GND
//
//
//
//       SW1 open    SW1 open      SW1 closed    SW1 closed
//       SW2 open    SW2 closed    SW2 open      SW2 closed
//
// Rsw   infinite     2K           1K         1/R=1/1K + 1/2K=3/2K, R=2/3
//        1K/1K       2K/3K        1K/2K      2/3 / 5/3 = 2/5
// V       255        255*2/3      255*1/2    255*2/5
//
//                ^            ^           ^
//                |            |           |


#define average(a0, a1)  ((a0 + a1)/2)

int switches;

int switch_sample(void)
{
  int result = 0;
  int analog;
  
  analog = adcSample(ADC_CHANNEL_Vswitch0);
  if (analog > average(255,255*2/3)) {
    // SW1 open, SW2 open
  } else if (analog > average(255*2/3,255*1/2)) {
    // SW1 open, SW2 closed
    result |= SWITCH_2;
  } else if (analog > average(255*1/2,255*2/5)) {
    // SW1 closed, SW2 open
    result |= SWITCH_1;
  } else {
    // SW1 closed, SW2 closed
    result |= SWITCH_1;
    result |= SWITCH_2;
  }

  analog = adcSample(ADC_CHANNEL_Vswitch1);
  if (analog > average(255,255*2/3)) {
    // SW3 open, SW4 open
  } else if (analog > average(255*2/3,255*1/2)) {
    // SW3 open, SW4 closed
    result |= SWITCH_4;
  } else if (analog > average(255*1/2,255*2/5)) {
    // SW3 closed, SW4 open
    result |= SWITCH_3;
  } else {
    // SW3 closed, SW4 closed
    result |= SWITCH_3;
    result |= SWITCH_4;
  }

  switches = result;

  return(result);
}
