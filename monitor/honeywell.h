#ifndef __honeywell_h__
#define __honeywell_h__

typedef struct {

  double lake_1st_T;
  double lake_1st_Tsetpoint;
  double lake_1st_fan;

  double lake_2nd_T;
  double lake_2nd_Tsetpoint;
  double lake_2nd_fan;

} HONEYWELL_t;

#define HONEYWELL_UNKNOWN (-95)

#define honeywell_isUnknown(a) ((a<=RADIANT_UNKNOWN) ? 1 : 0)

int honeywell_start(void);
int honeywell_sample(HONEYWELL_t *h);


#endif
