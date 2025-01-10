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

    sprintf(command,"./honeywell.py -s");
    fp = popen(command,"r");
      
    if (fp) {
      double temperature_1st=HONEYWELL_UNKNOWN;
      double setpoint_1st=HONEYWELL_UNKNOWN;
      double fanIsRunning_1st=HONEYWELL_UNKNOWN;
      double temperature_2nd=HONEYWELL_UNKNOWN;
      double setpoint_2nd=HONEYWELL_UNKNOWN;
      double fanIsRunning_2nd=HONEYWELL_UNKNOWN;
      while(fgets(data,512,fp)) {
	char c;
#ifdef DEBUG_HONEYWELL
	fprintf(stdout,"honeywell %s",data);
#endif
	sscanf(data,"1st Indoor Temperature: %lf",&temperature_1st);
	sscanf(data,"1st Heat Setpoint: %lf",&setpoint_1st);
	if (sscanf(data,"1st fanIsRunning: %c",&c)==1)
	  fanIsRunning_1st=(c=='T') ? 1 : 0;
	sscanf(data,"2nd Indoor Temperature: %lf",&temperature_2nd);
	sscanf(data,"2nd Heat Setpoint: %lf",&setpoint_2nd);
	if (sscanf(data,"2nd fanIsRunning: %c",&c)==1)
	  fanIsRunning_2nd=(c=='T') ? 1 : 0;
      }
      fclose(fp);

      if ( temperature_1st != HONEYWELL_UNKNOWN &&
	   setpoint_1st != HONEYWELL_UNKNOWN &&
	   fanIsRunning_1st != HONEYWELL_UNKNOWN &&
	   temperature_2nd != HONEYWELL_UNKNOWN &&
	   setpoint_2nd != HONEYWELL_UNKNOWN &&
	   fanIsRunning_2nd != HONEYWELL_UNKNOWN ) {
	pthread_mutex_lock(&mutex);
	locations[0].temperature = temperature_1st;
	locations[0].setpoint = setpoint_1st;
	locations[0].fanIsRunning = fanIsRunning_1st;
	time(&locations[0].time);
	locations[1].temperature = temperature_2nd;
	locations[1].setpoint = setpoint_2nd;
	locations[1].fanIsRunning = fanIsRunning_2nd;
	time(&locations[1].time);
	pthread_mutex_unlock(&mutex);
#ifdef DEBUG_HONEYWELL
	fprintf(stdout,"honeywell %s temp=%lf setpoint=%lf fan=%lf\n",
		locations[0].name,
		locations[0].temperature,
		locations[0].setpoint,
		locations[0].fanIsRunning);
	fprintf(stdout,"honeywell %s temp=%lf setpoint=%lf fan=%lf\n",
		locations[1].name,
		locations[1].temperature,
		locations[1].setpoint,
		locations[1].fanIsRunning);
#endif
      } else {
#ifdef DEBUG_HONEYWELL
	fprintf(stdout,"honeywell UNKNOWN\n");
#endif

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
