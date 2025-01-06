#ifndef __thermal_h__
#define __thermal_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int flags;
  int runtime;
  double collT;
  double storT;
  double diffT;
  double hiliT;
  double aux1T;
  double aux2T;
  int pump;
  int uplim;
} THERMAL_t;

#define hotTubT aux1T

extern THERMAL_t thermal;

#define THERMAL_FLAGS_commLinkDown     (1<<0)
#define THERMAL_FLAGS_missingField     0 // FIXME(1<<1)
#define THERMAL_FLAGS_thermistorOpen   (1<<2)
#define THERMAL_FLAGS_thermistorShort  (1<<3)
#define THERMAL_FLAGS_notUnderstood    (1<<4)

#define THERMAL_OPEN (-99.0)
#define THERMAL_SHORT (-98.0)
#define THERMAL_NOTUNDERSTOOD (-97.0)
#define THERMAL_MISSING (-96.0)
#define THERMAL_UNKNOWN (-95)

#define thermal_isUnknown(a) ((a<=THERMAL_UNKNOWN) ? 1 : 0)

void thermal_poll(void);

#ifdef __cplusplus
}
#endif

#endif
