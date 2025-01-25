#ifndef __switch_h__
#define __switch_h__

typedef enum {
  SWITCH_1 = 1,
  SWITCH_2 = 2,
  SWITCH_3 = 4,
  SWITCH_4 = 8,
} SWITCH_t;

#define SWITCH_noDHW            SWITCH_1
#define SWITCH_heatHotTub       SWITCH_2
#define SWITCH_noHeat           SWITCH_3
#define SWITCH_thermostatHotTub SWITCH_4

extern int switches;

int switch_sample(void);

#endif
