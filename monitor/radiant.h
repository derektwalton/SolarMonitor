#ifndef __radiant_h__
#define __radiant_h__

typedef struct {
  int commLinkDown;
  int uptime;

  // thermal 
  double thermal_collT;
  double thermal_storT;
  double thermal_aux1T; // currently hot tub input to heat exchanger
  double thermal_aux2T;
  int thermal_pump;
  int thermal_uplim;

  // radiant controller inputs
  double Vac;
  double Vpv;
  double T1st; // T 1st floor
  double T2nd; // T 2nd floor
  double Tbas; // T basement
  double Tout; // T outside
  double TXchI; // T glycol exchange in
  double TXchM; // T glycol exchange middle
  double TXchO; // T glycol exchange out
  double THotO; // T tank hot out
  double TRadR; // T radiant return
  double THTbR; // T hot tub return
  int H1st; // heat call 1st floor
  int H2nd; // heat call 2nd floor

  // radiant controller switches
  int switchAway; // away selected (home vs away)
  int switchHTub; // hot tub in service
  int switchSw3;
  int switchSw4;

  // radiant controller output
  int circulatorRadiant;
  int circulatorHTub;
  int auxFET;
  int finnedDump; // finned tube thermal dump
  int relay1st; // 1st floor heat boost
  int relay2nd; // 2nd floor heat boost  

  // extra temperatures
  double Troot; // root cellar temperature
  
} RADIANT_t;

#define RADIANT_OPEN (-99.0)
#define RADIANT_SHORT (-98.0)
#define RADIANT_NOTUNDERSTOOD (-97.0)
#define RADIANT_MISSING (-96.0)
#define RADIANT_UNKNOWN (-95)

#define radiant_isUnknown(a) ((a<=RADIANT_UNKNOWN) ? 1 : 0)

int radiant_start(char *device);
int radiant_sample(RADIANT_t *r);

#endif
