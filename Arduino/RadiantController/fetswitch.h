#ifndef __fetswitch_h__
#define __fetswitch_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  FETSWITCH_0 = 0,
  FETSWITCH_1 = 1,
  FETSWITCH_2 = 2
} FETSWITCH_t;

#define CIRCULATOR_Radiant FETSWITCH_2
#define CIRCULATOR_HotTub  FETSWITCH_1

void fetSwitchOn(FETSWITCH_t n);
void fetSwitchOff(FETSWITCH_t n);
void fetSwitchInit();

#ifdef __cplusplus
}
#endif

#endif
