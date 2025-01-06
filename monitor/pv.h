#ifndef __pv_h__
#define __pv_h__

typedef struct {
  double dcVoltage;
  double acPower;
  double acCurrent;
  double acEnergy;
  double acVoltage;
} PV_t;

#define PV_FLAGS_commLinkDown     (1<<0)
#define PV_UNKNOWN                (-1.0)

#define pv_isUnknown(a) ((a<=PV_UNKNOWN) ? 1 : 0)

int pv_start(char *device);
int pv_sample(PV_t *pv);

#endif
