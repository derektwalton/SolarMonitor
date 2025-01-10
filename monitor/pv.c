#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>

#include "pv.h"

//#define DEBUG_PV

/* baudrate settings are defined in <asm/termbits.h>, which is
included by <termios.h> */
#define BAUDRATE B19200            
#define _POSIX_SOURCE 1 /* POSIX compliant source */

static char device[32];

PV_t pv;

#define MAX_FAIL_COUNT 10

static pthread_t tid;
static pthread_mutex_t mutex;

static uint8_t cmd_getDCVoltage[] = {0x0a, 0x09, 0x03, 0x00, 0xba, 0x00, 0x01, 0xa4, 0xa7, 0x0d};
static uint8_t cmd_getACPower[]   = {0x0a, 0x09, 0x03, 0x00, 0xc1, 0x00, 0x01, 0xd4, 0xbe, 0x0d};
static uint8_t cmd_getACCurrent[] = {0x0a, 0x09, 0x03, 0x00, 0xc2, 0x00, 0x01, 0x24, 0xbe, 0x0d};
static uint8_t cmd_getACEnergy[]  = {0x0a, 0x09, 0x03, 0x00, 0xc4, 0x00, 0x02, 0x84, 0xbe, 0x0d};
static uint8_t cmd_getACVoltage[] = {0x0a, 0x09, 0x03, 0x00, 0xc0, 0x00, 0x01, 0x85, 0x7e, 0x0d};

static int response_get(int fd, uint8_t *rsp, int n)
{
  int i,j,result;
  for(i=0;i<n;) {
    result = read(fd,&rsp[i],n-i);
    i+=result;
    if(result==0)
      break;
  }
#ifdef DEBUG_PV
  fprintf(stdout,"%s exp %d got %d:",__func__,n,i);
  for(j=0;j<i;j++)
    fprintf(stdout," 0x%.2x",rsp[j]);
  fprintf(stdout,"\n");
#endif
  return i;
}

static void* task(void *arg)
{
  uint8_t rsp[16];
  int flags;
  int failCount=0;
  int fd, result, j;
  struct termios oldtio,newtio;

  /* 
     Open modem device for reading and writing and not as controlling tty
     because we don't want to get killed if linenoise sends CTRL-C.
  */
  fd = open(device, O_RDWR | O_NOCTTY ); 
  if (fd <0) {perror(device); exit(1); }
  
  tcgetattr(fd,&oldtio); /* save current serial port settings */
  bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */
  
  /* 
     BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
     CRTSCTS : output hardware flow control (only used if the cable has
     all necessary lines. See sect. 7 of Serial-HOWTO)
     CS8     : 8n1 (8bit,no parity,1 stopbit)
     CLOCAL  : local connection, no modem contol
     CREAD   : enable receiving characters
  */
  newtio.c_cflag = BAUDRATE /* | CRTSCTS*/ | CS8 | CLOCAL | CREAD;
  
  /*
    IGNPAR  : ignore bytes with parity errors
    ICRNL   : map CR to NL (otherwise a CR input on the other computer
    will not terminate input)
    otherwise make device raw (no other input processing)
  */
  newtio.c_iflag = IGNPAR /* | ICRNL */;
  
  /*
    Raw output.
  */
  newtio.c_oflag = 0;
  
  /*
    ICANON  : enable canonical input
    disable all echo functionality, and don't send signals to calling program
  */
  newtio.c_lflag = 0;
  
  /* 
     initialize all control characters 
     default values can be found in /usr/include/termios.h, and are given
     in the comments, but we don't need them here
  */
  newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
  newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
  newtio.c_cc[VERASE]   = 0;     /* del */
  newtio.c_cc[VKILL]    = 0;     /* @ */
  newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
  newtio.c_cc[VTIME]    = 10;    /* 10*.1 = 1 sec timeout */
  newtio.c_cc[VMIN]     = 0;     /* return after 1 char or timeout */
  newtio.c_cc[VSWTC]    = 0;     /* '\0' */
  newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
  newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
  newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
  newtio.c_cc[VEOL]     = 0;     /* '\0' */
  newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
  newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
  newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
  newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
  newtio.c_cc[VEOL2]    = 0;     /* '\0' */
  
  /* 
     now clean the modem line and activate the settings for the port
  */
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);
  
  /*
    terminal settings done ... run our communications loop
  */

  pv.dcVoltage = PV_UNKNOWN;
  pv.acPower = PV_UNKNOWN;
  pv.acCurrent = PV_UNKNOWN;
  pv.acEnergy = PV_UNKNOWN;
  pv.acVoltage = PV_UNKNOWN;

#ifdef DEBUG_PV
#define LOGF(a,b) printf("  %s %lf\n",a,b)
#define LOGI(a,b) printf("  %s %d\n",a,b)
#else
#define LOGF(a,b)
#define LOGI(a,b)
#endif

  while (1) {

#ifdef DEBUG_PV
    printf("pv_task poll\n");
#endif
    
    flags = 0;

    write(fd,cmd_getDCVoltage,sizeof(cmd_getDCVoltage));
    result = response_get(fd,rsp,9);
    pthread_mutex_lock(&mutex);
    if (result == 9) {
      j = (rsp[4]<<8) | (rsp[5]);
      pv.dcVoltage = ((double)j) / 10.0;
      LOGF("dcVoltage",pv.dcVoltage);
      failCount = 0;
    } else {
      if (++failCount > MAX_FAIL_COUNT) {
	pv.dcVoltage = PV_UNKNOWN;
	flags |= PV_FLAGS_commLinkDown;
      }
    }
    pthread_mutex_unlock(&mutex);

    write(fd,cmd_getACPower,sizeof(cmd_getACPower));
    result = response_get(fd,rsp,9);
    pthread_mutex_lock(&mutex);
    if (result == 9) {
      j = (rsp[4]<<8) | (rsp[5]);
      pv.acPower = ((double)j);	  
      LOGF("acPower",pv.acPower);
      failCount = 0;
    } else {
      if (++failCount > MAX_FAIL_COUNT) {
	pv.acPower = PV_UNKNOWN;
	flags |= PV_FLAGS_commLinkDown;
      }
    }
    pthread_mutex_unlock(&mutex);

    write(fd,cmd_getACCurrent,sizeof(cmd_getACCurrent));
    result = response_get(fd,rsp,9);
    pthread_mutex_lock(&mutex);
    if (result == 9) {
      j = (rsp[4]<<8) | (rsp[5]);
      pv.acCurrent = ((double)j) / 10.0;	  
      LOGF("acCurrent",pv.acCurrent);
      failCount = 0;
    } else {
      if (++failCount > MAX_FAIL_COUNT) {
	pv.acCurrent = PV_UNKNOWN;
	flags |= PV_FLAGS_commLinkDown;
      }
    }
    pthread_mutex_unlock(&mutex);
    
    write(fd,cmd_getACEnergy,sizeof(cmd_getACEnergy));
    result = response_get(fd,rsp,11);
    pthread_mutex_lock(&mutex);
    if (result == 11) {
      j = (rsp[6]<<8) | (rsp[7]);
      pv.acEnergy = ((double)j) / 10.0;
      j = (rsp[4]<<8) | (rsp[5]);
      pv.acEnergy += ((double)j) * 1000.0;
      LOGF("acEnergy",pv.acEnergy);
      failCount = 0;
    } else {
      if (++failCount > MAX_FAIL_COUNT) {
	pv.acEnergy = PV_UNKNOWN;
	flags |= PV_FLAGS_commLinkDown;
      }
    }
    pthread_mutex_unlock(&mutex);

    write(fd,cmd_getACVoltage,sizeof(cmd_getACVoltage));
    result = response_get(fd,rsp,9);
    pthread_mutex_lock(&mutex);
    if (result == 9) {
      j = (rsp[4]<<8) | (rsp[5]);
      pv.acVoltage = ((double)j) / 10.0;	  
      LOGF("acVoltage",pv.acVoltage);
      failCount = 0;
    } else {
      if (++failCount > MAX_FAIL_COUNT) {
	pv.acVoltage = PV_UNKNOWN;
	flags |= PV_FLAGS_commLinkDown;
      }
    }
    pthread_mutex_unlock(&mutex);

    sleep(10);
  }

#undef LOGF
#undef LOGI  

  /* restore the old port settings */
  tcsetattr(fd,TCSANOW,&oldtio);
}



int pv_start(char *d)
{
  int err;

  if (!d)
    return -1;

  strcpy(device,d);
  
  printf("%s using %s\n",__func__,device);
  
  err = pthread_mutex_init(&mutex,NULL);
  if (err!=0)
    return err;
  
  err = pthread_create(&tid,NULL,task,NULL);
  if (err!=0)
    return err;

  return 0;
}

int pv_sample(PV_t *p)
{
  pthread_mutex_lock(&mutex);
  *p = pv;
  pthread_mutex_unlock(&mutex);
}
