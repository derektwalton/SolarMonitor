#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"
#include "adc.h"
#include "power.h"

//
// PV power supply ------ 47K  ----- Vpv -----  10K ------ GND
//

double getPowerVoltage(POWER_t n)
{
  double voltage;
  int analog=0;
  
  switch(n) {
  case POWER_Vmain:
    analog = adcSample(ADC_CHANNEL_Vmain);
    break;
  case POWER_Vpv:
    analog = adcSample(ADC_CHANNEL_Vpv);
    break;    
  }

  // voltage at analog pin
  voltage = ((double) analog) / 255.0 * 5.0;

  // reverse out resistor divider
  voltage *= 47.0 / 10.0;

  return (voltage);
}
