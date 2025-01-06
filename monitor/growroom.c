#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "growroom.h"

#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to listen for incoming data
 
static pthread_t tid;
static pthread_mutex_t mutex;

static time_t sampleTime;

static int year;
static int month;
static int day;
static int hour;
static int minute;
static double ambient_T;
static double ambient_humidity;
static double soil_T;
static int soil_heat;
static int room_heat;
static int light;
static int fan;
static int logCount;
static double ambient_T2;
static double ambient_humidity2;
static int smoke;

static int parseInt(char **s, int *i)
{
  int v;
  char *ss;

  if (**s == 0) {
    return GROWROOM_UNKNOWN;
  }

  if (**s == ',' || **s == '\n') {
    *i = *i + 1;
    *s = *s + 1;
    return GROWROOM_UNKNOWN;
  }

  if (**s == 'x') {
    *i = *i + 1;
    while(**s != ',' && **s != '\n' && **s != 0)
      *s = *s + 1;
    if (**s != 0)
      *s = *s + 1;
    return GROWROOM_UNKNOWN;
  }
    
  ss = *s;
  while(**s != ',' && **s != '\n' && **s != 0)
    *s = *s + 1;
  if (**s != 0)
    *s = *s + 1;
  if (sscanf(ss,"%d",&v)==1) {
    *i = *i + 1;
    return v;
  } else {
    return GROWROOM_UNKNOWN;
  }
  
}

static double parseFloat(char **s, int *i)
{
  double v;
  char *ss;

  if (**s == 0) {
    return GROWROOM_UNKNOWN;
  }

  if (**s == ',' || **s == '\n') {
    *i = *i + 1;
    *s = *s + 1;
    return GROWROOM_UNKNOWN;
  }

  if (**s == 'x') {
    *i = *i + 1;
    while(**s != ',' && **s != '\n' && **s != 0)
      *s = *s + 1;
    if (**s != 0)
      *s = *s + 1;
    return GROWROOM_UNKNOWN;
  }
    
  ss = *s;
  while(**s != ',' && **s != '\n' && **s != 0)
    *s = *s + 1;
  if (**s != 0)
    *s = *s + 1;
  if (sscanf(ss,"%lf",&v)==1) {
    *i = *i + 1;
    return v;
  } else {
    return GROWROOM_UNKNOWN;
  }
  
}

static void* task(void *arg)
{
  struct sockaddr_in si_me, si_other;
     
  int s, i, slen = sizeof(si_other) , recv_len;
  char buf[BUFLEN];
     
  //create a UDP socket
  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
      fprintf(stdout,"socket() fail\n");
      return;
    }
     
  // zero out the structure
  memset((char *) &si_me, 0, sizeof(si_me));
     
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(PORT);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
  //bind socket to port
  if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
      fprintf(stdout,"bind() fail\n");
      return;
    }
     
  //keep listening for data
  while(1)
    {
      //try to receive some data, this is a blocking call
      if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
	  fprintf(stdout,"recvfrom() fail\n");
	  return;
        }
      buf[recv_len]=0;

      fprintf(stdout,"%s",buf);
      {
	char *s = buf;
	int i = 0;

	year = parseInt(&s,&i);
	month = parseInt(&s,&i);
	day = parseInt(&s,&i);
	hour = parseInt(&s,&i);
	minute = parseInt(&s,&i);
	ambient_T = parseFloat(&s,&i);
	ambient_humidity = parseFloat(&s,&i);
	soil_T = parseFloat(&s,&i);
	soil_heat = parseInt(&s,&i);
	room_heat = parseInt(&s,&i);
	light = parseInt(&s,&i);
	fan = parseInt(&s,&i);
	logCount = parseInt(&s,&i);
	ambient_T2 = parseFloat(&s,&i);
	ambient_humidity2 = parseFloat(&s,&i);
	smoke = parseInt(&s,&i);
	
	if (i==16) {
#if 0
	  fprintf(stdout,"parse ok .. %d %d %d %d %d %lf %lf %lf %d %d %d %d %d\n",
		  year, month, day, hour, minute,
		  ambient_T,ambient_humidity,soil_T,
		  soil_heat, room_heat, light, fan, logCount);
#endif
	  time(&sampleTime);
	}
      }

    }
  
  close(s);
  return 0;
}

int growroom_start(void)
{
  int err;

  printf("%s\n",__func__);

  err = pthread_mutex_init(&mutex,NULL);
  if (err!=0)
    return err;
  
  err = pthread_create(&tid,NULL,task,NULL);
  if (err!=0)
    return err;

  return 0;
}

int growroom_sample(GROWROOM_t *h)
{
  time_t now;

  h->soil_T = GROWROOM_UNKNOWN;
  h->ambient_T = GROWROOM_UNKNOWN;
  h->ambient_humidity = GROWROOM_UNKNOWN;
  h->soil_heat = GROWROOM_UNKNOWN;
  h->room_heat = GROWROOM_UNKNOWN;
  h->light = GROWROOM_UNKNOWN;
  h->fan = GROWROOM_UNKNOWN;
  h->ambient_T2 = GROWROOM_UNKNOWN;
  h->ambient_humidity2 = GROWROOM_UNKNOWN;
  h->smoke = GROWROOM_UNKNOWN;

  time(&now);
  
  pthread_mutex_lock(&mutex);

  if (difftime(now,sampleTime)<2*60) {
    h->soil_T = soil_T;
    h->ambient_T = ambient_T;
    h->ambient_humidity = ambient_humidity;
    h->soil_heat = soil_heat;
    h->room_heat = room_heat;
    h->light = light;
    h->fan = fan;
    h->ambient_T2 = ambient_T2;
    h->ambient_humidity2 = ambient_humidity2;
    h->smoke = smoke;
  }

  pthread_mutex_unlock(&mutex);
}
