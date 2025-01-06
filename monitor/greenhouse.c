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

#include "greenhouse.h"

#define BUFLEN 512  //Max length of buffer
#define PORT 8889   //The port on which to listen for incoming data
 
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
static double gnd13_T;
static double gnd3_T;
static double pool_T;
static double panel_T;
static double inner_T;
static double out_T;
static double spare1_T;
static double spare2_T;
static int pump;
static int fan;
static int dehumidifier;
static int control3;
static int logCount;

static int parseInt(char **s, int *i)
{
  int v;
  char *ss;

  if (**s == 0) {
    return GREENHOUSE_UNKNOWN;
  }

  if (**s == ',' || **s == '\n') {
    *i = *i + 1;
    *s = *s + 1;
    return GREENHOUSE_UNKNOWN;
  }

  if (**s == 'x') {
    *i = *i + 1;
    while(**s != ',' && **s != '\n' && **s != 0)
      *s = *s + 1;
    if (**s != 0)
      *s = *s + 1;
    return GREENHOUSE_UNKNOWN;
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
    return GREENHOUSE_UNKNOWN;
  }
  
}

static double parseFloat(char **s, int *i)
{
  double v;
  char *ss;

  if (**s == 0) {
    return GREENHOUSE_UNKNOWN;
  }

  if (**s == ',' || **s == '\n') {
    *i = *i + 1;
    *s = *s + 1;
    return GREENHOUSE_UNKNOWN;
  }

  if (**s == 'x') {
    *i = *i + 1;
    while(**s != ',' && **s != '\n' && **s != 0)
      *s = *s + 1;
    if (**s != 0)
      *s = *s + 1;
    return GREENHOUSE_UNKNOWN;
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
    return GREENHOUSE_UNKNOWN;
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
	int sumpPump, circPump;
	char *s = buf;
	int i = 0;

	year = parseInt(&s,&i);
	month = parseInt(&s,&i);
	day = parseInt(&s,&i);
	hour = parseInt(&s,&i);
	minute = parseInt(&s,&i);
	ambient_T = parseFloat(&s,&i);
	ambient_humidity = parseFloat(&s,&i);
	gnd13_T = parseFloat(&s,&i);
	gnd3_T = parseFloat(&s,&i);
	pool_T = parseFloat(&s,&i);
	panel_T = parseFloat(&s,&i);
	inner_T = parseFloat(&s,&i);
	out_T = parseFloat(&s,&i);
	spare1_T = parseFloat(&s,&i);
	spare2_T = parseFloat(&s,&i);
	sumpPump = parseInt(&s,&i);
	circPump = parseInt(&s,&i);
	fan = parseInt(&s,&i);
	dehumidifier = parseInt(&s,&i);
	logCount = parseInt(&s,&i);

	if (sumpPump == GREENHOUSE_UNKNOWN || circPump == GREENHOUSE_UNKNOWN)
	  pump = GREENHOUSE_UNKNOWN;
	else
	  pump = sumpPump*2 + circPump;
	control3 = 0;
	
	if (i==20) {
#if 0
	  fprintf(stdout,"parse ok .. %d %d %d %d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %d %d %d %d %d\n",
		  year, month, day, hour, minute,
		  ambient_T,ambient_humidity,
		  gnd13_T,
		  gnd3_T,
		  pool_T,
		  panel_T,
		  inner_T,
		  out_T,
		  spare1_T,
		  spare2_T,
		  pump,
		  fan,
		  dehumidifier,
		  logCount);
#endif
	  time(&sampleTime);
	}
      }

    }
  
  close(s);
  return 0;
}

int greenhouse_start(void)
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

int greenhouse_sample(GREENHOUSE_t *h)
{
  time_t now;

  h->ambient_T = GREENHOUSE_UNKNOWN;
  h->ambient_humidity = GREENHOUSE_UNKNOWN;
  h->gnd13_T = GREENHOUSE_UNKNOWN;
  h->gnd3_T = GREENHOUSE_UNKNOWN;
  h->pool_T = GREENHOUSE_UNKNOWN;
  h->panel_T = GREENHOUSE_UNKNOWN;
  h->inner_T = GREENHOUSE_UNKNOWN;
  h->out_T = GREENHOUSE_UNKNOWN;
  h->spare1_T = GREENHOUSE_UNKNOWN;
  h->spare2_T = GREENHOUSE_UNKNOWN;
  h->pump = GREENHOUSE_UNKNOWN;
  h->fan = GREENHOUSE_UNKNOWN;
  h->dehumidifier = GREENHOUSE_UNKNOWN;
  h->control3 = GREENHOUSE_UNKNOWN;
  
  time(&now);
  
  pthread_mutex_lock(&mutex);

  if (difftime(now,sampleTime)<2*60) {
    h->ambient_T = ambient_T;
    h->ambient_humidity = ambient_humidity;
    h->gnd13_T = gnd13_T;
    h->gnd3_T = gnd3_T;
    h->pool_T = pool_T;
    h->panel_T = panel_T;
    h->inner_T = inner_T;
    h->out_T = out_T;
    h->spare1_T = spare1_T;
    h->spare2_T = spare2_T;
    h->pump = pump;
    h->fan = fan;
    h->dehumidifier = dehumidifier;
    h->control3 = control3;
  }

  pthread_mutex_unlock(&mutex);
}
