#ifndef __control_h__
#define __control_h__

#ifdef __cplusplus
extern "C" {
#endif

// we no longer have connection to 1st/2nd floor thermostats
#define CONTROL_NO_TSTAT 1

extern double Vmain,Vpv;
extern unsigned upTime;

extern int circulatorRadiant;
extern int circulatorHotTub;
extern int finnedTubeDump;

#if CONTROL_NO_TSTAT
#define heat1stFloor 0
#define heat2ndFloor 0
#define heat1stFloorBoost 0
#define heat2ndFloorBoost 0
#else
extern int heat1stFloor;
extern int heat2ndFloor;
extern int heat1stFloorBoost;
extern int heat2ndFloorBoost;
#endif

void control_poll(void);

#ifdef __cplusplus
}
#endif

#endif
