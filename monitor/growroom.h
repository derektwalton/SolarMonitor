#ifndef __growroom_h__
#define __growroom_h__

typedef struct {

  double soil_T;
  double ambient_T;
  double ambient_humidity;

  int soil_heat;
  int room_heat;
  int light;
  int fan;

  double ambient_T2;
  double ambient_humidity2;
  int smoke;

} GROWROOM_t;

#define GROWROOM_UNKNOWN (-95)

#define growroom_isUnknown(a) ((a<=GROWROOM_UNKNOWN) ? 1 : 0)

int growroom_start(void);
int growroom_sample(GROWROOM_t *h);


#endif
