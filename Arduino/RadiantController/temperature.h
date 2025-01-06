#ifndef __temperature_h__
#define __temperature_h__

#ifdef __cplusplus
extern "C" {
#endif

// we no longer have 1-wire temp sensors at thermostat locations
#define TEMPERATURE_NO_1ST_2ND_ONEWIRE 1

typedef enum { 
  TEMPERATURE_1stFloor,
  TEMPERATURE_2ndFloor,
  TEMPERATURE_Basement,
  TEMPERATURE_Outside,
  TEMPERATURE_GlycolExchIn,
  TEMPERATURE_GlycolExchMid,
  TEMPERATURE_GlycolExchOut,
  TEMPERATURE_TankHotOut,
  TEMPERATURE_RadiantReturn,
  TEMPERATURE_HotTubReturn,
  TEMPERATURE_RootCellar,
  TEMPERATURE__last
} TEMPERATURE_t;

#define N_TEMPS TEMPERATURE__last

extern double gTemperature[N_TEMPS];

#define TEMPERATURE_UNKNOWN (-99.0)

int temperature_Init(void);
int temperature_UpdateTemperatures(void);

#ifdef __cplusplus
}
#endif

#endif
