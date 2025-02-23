#ifndef __power_h__
#define __power_h__

typedef enum {
  POWER_Vpv
} POWER_t;

double getPowerVoltage(POWER_t n);

#endif
