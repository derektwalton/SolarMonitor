#ifndef __adc_h__
#define __adc_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ADC_CHANNEL_adc0 = 0,
  ADC_CHANNEL_adc1 = 1,
  ADC_CHANNEL_adc2 = 2,
  ADC_CHANNEL_adc3 = 3,
  ADC_CHANNEL_adc4 = 4,
  ADC_CHANNEL_adc5 = 5,
  ADC_CHANNEL_adc6 = 6,
  ADC_CHANNEL_adc7 = 7,
  ADC_CHANNEL_adc8 = 8,
  ADC_CHANNEL_Vbg = 14,
  ADC_CHANNEL_gnd = 15,
} ADC_CHANNEL_t;

#define ADC_CHANNEL_Vswitch0  ADC_CHANNEL_adc3
#define ADC_CHANNEL_Vpv       ADC_CHANNEL_adc6
#define ADC_CHANNEL_Vswitch1  ADC_CHANNEL_adc7
#define ADC_CHANNEL_Tstat0    ADC_CHANNEL_adc1
#define ADC_CHANNEL_Tstat1    ADC_CHANNEL_adc0

int adcSample(ADC_CHANNEL_t n);
void adcInit();

#ifdef __cplusplus
}
#endif

#endif
