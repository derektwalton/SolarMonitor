#ifndef __time_h__
#define __time_h__

#ifdef __cplusplus
extern "C" {
#endif

long time_get(void);
long time_secondsToTicks(int seconds);
int time_elapsed_mSec(long t0);
int time_elapsed_sec(long t0);
void time_init(void);

#ifdef __cplusplus
}
#endif

#endif
