#ifndef __control_h__
#define __control_h__

#ifdef __cplusplus
extern "C" {
#endif

extern double Vmain,Vpv;
extern unsigned upTime;

extern int circulatorRadiant;
extern int circulatorHotTub;
extern int finnedTubeDump;

extern int heat1stFloor;
extern int heat2ndFloor;

void control_poll(void);

#ifdef __cplusplus
}
#endif

#endif
