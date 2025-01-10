#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "honeywell.h"

#define DEBUG_HONEYWELL

static pthread_t tid;
static pthread_mutex_t mutex;

struct {
  char name[16];
  double temperature;
  double setpoint;
  double fanIsRunning;
  time_t time;
} locations[] =
  {
    { "1st", HONEYWELL_UNKNOWN, HONEYWELL_UNKNOWN, HONEYWELL_UNKNOWN },
    { "2nd", HONEYWELL_UNKNOWN, HONEYWELL_UNKNOWN, HONEYWELL_UNKNOWN },
  };



static void* task(void *arg)
{
  FILE *fp;
  char command[32];
  char data[512];
  int i;
  
  while (1) {

    for(i=0;i<sizeof(locations)/sizeof(locations[0]);i++) {
    
      sprintf(command,"./honeywell_%s.py -s",locations[i].name);
      fp = popen(command,"r");
      
      if (fp) {
	double temperature=HONEYWELL_UNKNOWN;
	double setpoint=HONEYWELL_UNKNOWN;
	double fanIsRunning=HONEYWELL_UNKNOWN;
	while(fgets(data,512,fp)) {
	  char c;
#ifdef DEBUG_HONEYWELL
	  fprintf(stdout,"honeywell %s %s",locations[i].name,data);
#endif
	  sscanf(data,"Indoor Temperature: %lf",&temperature);
	  sscanf(data,"Heat Setpoint: %lf",&setpoint);
	  if (sscanf(data,"fanIsRunning: %c",&c)==1)
	    fanIsRunning=(c=='T') ? 1 : 0;
	}
	fclose(fp);

	if ( temperature != HONEYWELL_UNKNOWN &&
	     setpoint != HONEYWELL_UNKNOWN &&
	     fanIsRunning != HONEYWELL_UNKNOWN ) {
	  pthread_mutex_lock(&mutex);
	  locations[i].temperature = temperature;
	  locations[i].setpoint = setpoint;
	  locations[i].fanIsRunning = fanIsRunning;
	  time(&locations[i].time);
	  pthread_mutex_unlock(&mutex);
#ifdef DEBUG_HONEYWELL
	  fprintf(stdout,"honeywell %s temp=%lf setpoint=%lf fan=%lf\n",
		  locations[i].name,
		  locations[i].temperature,
		  locations[i].setpoint,
		  locations[i].fanIsRunning);
#endif
	} else {
#ifdef DEBUG_HONEYWELL
	  fprintf(stdout,"honeywell %s UNKNOWN\n",locations[i].name);
#endif
	}

	// honeywell service is not happy if polled too quickly 
	// so wait 5 minutes between polls
	sleep(5*60);
      }
    }
  }
}

int honeywell_start(void)
{
  int err;

  err = pthread_mutex_init(&mutex,NULL);
  if (err!=0)
    return err;
  
  err = pthread_create(&tid,NULL,task,NULL);
  if (err!=0)
    return err;

  return 0;
}

int honeywell_sample(HONEYWELL_t *h)
{
  time_t now;

  h->lake_1st_T = HONEYWELL_UNKNOWN;
  h->lake_1st_Tsetpoint = HONEYWELL_UNKNOWN;
  h->lake_1st_fan = HONEYWELL_UNKNOWN;

  h->lake_2nd_T = HONEYWELL_UNKNOWN;
  h->lake_2nd_Tsetpoint = HONEYWELL_UNKNOWN;
  h->lake_2nd_fan = HONEYWELL_UNKNOWN;

  time(&now);
  
  pthread_mutex_lock(&mutex);

  if (difftime(now,locations[0].time)<2*60) {
    h->lake_1st_T = locations[0].temperature;
    h->lake_1st_Tsetpoint = locations[0].setpoint;
    h->lake_1st_fan = locations[0].fanIsRunning;
  }
  if (difftime(now,locations[1].time)<2*60) {
    h->lake_2nd_T = locations[1].temperature;
    h->lake_2nd_Tsetpoint = locations[1].setpoint;
    h->lake_2nd_fan = locations[1].fanIsRunning;
  }

  pthread_mutex_unlock(&mutex);
}
