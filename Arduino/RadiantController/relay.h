#ifndef __relay_h__
#define __relay_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  RELAY_0 = 0,
  RELAY_1 = 1,
  RELAY_2 = 2
} RELAY_t;


#define RELAY_heat1stFloorBoost RELAY_0
#define RELAY_heat2ndFloorBoost RELAY_1
#define RELAY_finnedTubeDump RELAY_2

void relayOn(RELAY_t n);
void relayOff(RELAY_t n);
void relayInit(void);

#ifdef __cplusplus
}
#endif

#endif
