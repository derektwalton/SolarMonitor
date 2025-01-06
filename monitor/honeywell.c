#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "honeywell.h"

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
    { "condo", HONEYWELL_UNKNOWN, HONEYWELL_UNKNOWN, HONEYWELL_UNKNOWN },
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
	double temperature, setpoint, fanIsRunning;
	char c;
	fgets(data,512,fp);
	//fputs(data,stdout);
	if (sscanf(data,"Indoor Temperature: %lf",&temperature)!=1)
	  temperature=HONEYWELL_UNKNOWN;
	fgets(data,512,fp);
	//fputs(data,stdout);
	if (sscanf(data,"Heat Setpoint: %lf",&setpoint)!=1)
	  setpoint=HONEYWELL_UNKNOWN;
	fgets(data,512,fp);
	//fputs(data,stdout);
	if (sscanf(data,"fanIsRunning: %c",&c)!=1)
	  fanIsRunning=HONEYWELL_UNKNOWN;
	else
	  fanIsRunning=(c=='T') ? 1 : 0;

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
#if 0
	  fprintf(stdout,"honeywell %s %lf %lf %lf\n",
		  locations[i].name,
		  locations[i].temperature,
		  locations[i].setpoint,
		  locations[i].fanIsRunning);
#endif
	}
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

  h->condo_T = HONEYWELL_UNKNOWN;
  h->condo_Tsetpoint = HONEYWELL_UNKNOWN;
  h->condo_fan = HONEYWELL_UNKNOWN;

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
  if (difftime(now,locations[2].time)<2*60) {
    h->condo_T = locations[2].temperature;
    h->condo_Tsetpoint = locations[2].setpoint;
    h->condo_fan = locations[2].fanIsRunning;
  }

  pthread_mutex_unlock(&mutex);
}
