#ifndef __greenhouse_h__
#define __greenhouse_h__

typedef struct {

  double ambient_T;
  double ambient_humidity;
  double gnd13_T;
  double gnd3_T;
  double pool_T;
  double panel_T;
  double inner_T;
  double out_T;
  double spare1_T;
  double spare2_T;
  int pump;
  int fan;
  int dehumidifier;
  int control3;

} GREENHOUSE_t;

#define GREENHOUSE_UNKNOWN (-95)

#define greenhouse_isUnknown(a) ((a<=GREENHOUSE_UNKNOWN) ? 1 : 0)

int greenhouse_start(void);
int greenhouse_sample(GREENHOUSE_t *h);


#endif
