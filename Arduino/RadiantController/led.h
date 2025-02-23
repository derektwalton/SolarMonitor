#ifndef __led_h__
#define __led_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  LED_9,
  LED_10,
  LED_11,
  LED_12
} LED_t;


#define D2_USED_BY_ONEWIRE 1

#if (D2_USED_BY_ONEWIRE==0)
#define LED_MAIN_POWER_OK       LED_9
#endif

#define LED_PV_POWER_OK         LED_10
#define LED_THERMAL_STATUS_OK   LED_11
#define LED_ONEWIRE_STATUS_OK   LED_12

void ledOn(LED_t n);
void ledOff(LED_t n);
void ledInit();

#ifdef __cplusplus
}
#endif

#endif
