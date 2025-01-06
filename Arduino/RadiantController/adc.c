#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"
#include "adc.h"

int adcSample(ADC_CHANNEL_t n)
{
  ADMUX = 
    (1 << REFS0) |   // AVcc with external cap at AREF pin
    (1 << ADLAR) |   // left adjust conversion results
    (n << MUX0);     // channel
  
  ADCSRA =
    (1 << ADEN) |    // ADC enable = 1
    (1 << ADSC) |    // ADC start conversion = 0
    (0 << ADATE) |   // ADC auto trigger enable = 0
    (0 << ADIE) |    // ADC interrupt enable = 0
    (7 << ADPS0);    // 16 MHz / 128 = 125 KHz

  while (bit_is_set(ADCSRA, ADSC));

  return ADCH;
}

void adcInit()
{
  ADMUX = 
    (1 << REFS0) |   // AVcc with external cap at AREF pin
    (1 << ADLAR) |   // left adjust conversion results
    (0 << MUX0);     // dummy channel

  ADCSRA =
    (1 << ADEN) |    // ADC enable = 1
    (0 << ADSC) |    // ADC start conversion = 0
    (0 << ADATE) |   // ADC auto trigger enable = 0
    (0 << ADIE) |    // ADC interrupt enable = 0
    (7 << ADPS0);    // 16 MHz / 128 = 125 KHz
}
