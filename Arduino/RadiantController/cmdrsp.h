#ifndef __cmdresp_h__
#define __cmdresp_h__

#ifdef __cplusplus
extern "C" {
#endif

void cmdrsp_poll(void);

extern int heatBoost1stFloorTargetTemp;
extern int heatBoost1stFloorMinutes;
extern int heatBoost2ndFloorTargetTemp;
extern int heatBoost2ndFloorMinutes;

#ifdef __cplusplus
}
#endif

#endif
