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

#define RELAY_finnedTubeDump RELAY_0

void relayOn(RELAY_t n);
void relayOff(RELAY_t n);
void relayInit(void);

#ifdef __cplusplus
}
#endif

#endif
