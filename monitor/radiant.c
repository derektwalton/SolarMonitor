#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "radiant.h"

/* baudrate settings are defined in <asm/termbits.h>, which is
included by <termios.h> */
#define BAUDRATE B2400            
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define MAXLEN (200+1)    // 200 characters plus termination 0

static char device[32];

static pthread_t tid;
static pthread_mutex_t mutex;

RADIANT_t radiant, radiant_unknown = 
  {
    1, // commLinkDown,
    0,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    0,
    0,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    RADIANT_UNKNOWN,
    0,0, // heat call
    0,0,0,0, // switches
    0, // circulatorRadiant
    0, // circulatorHTub
    0, // auxFET
    0, // finnedDump
    0,0, // heat boost relays
    RADIANT_UNKNOWN, // Troot
  };

static char *getNextField(char *string)
{
  static char *cPtr;
  if (string) 
    cPtr = string;
  else {
    // walk past field value to space
    while(*cPtr && *cPtr != ' ') cPtr++;
    if (!*cPtr) return NULL;
  }
  // walk to next field value value (ie. non-space)
  while(*cPtr && *cPtr == ' ') cPtr++;
  if (!*cPtr) return NULL;
  return cPtr;
}

#if 1
#define LOGF(a,b) printf("  %s %lf\n",a,b)
#define LOGI(a,b) printf("  %s %d\n",a,b)
#else
#define LOGF(a,b)
#define LOGI(a,b)
#endif

static void* task(void *arg)
{
  int fd, c, result;
  struct termios oldtio,newtio;
  char s[MAXLEN];
  char *field;
  int i, j, days, hours, minutes, seconds;
  double t;

  /* 
     Open modem device for reading and writing and not as controlling tty
     because we don't want to get killed if linenoise sends CTRL-C.
  */
  fd = open(device, O_RDWR | O_NOCTTY ); 
  if (fd <0) {perror(device); exit(-1); }
  
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
  newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
  
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
  newtio.c_lflag = ICANON;
  
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
  newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
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
    terminal settings done, now handle input
  */
  while (1) {
    
    result = read(fd,s,MAXLEN-1); 
    s[result]=0;             /* set end of string, so we can printf */
    
    if (result==0) {
      //radiant.commLinkDown = 1;
    }
    
    else {

      pthread_mutex_lock(&mutex);
      
      radiant.commLinkDown = 0;
      
      // RUNTIME
      field = getNextField(s);
      if (!field) 
	radiant.uptime = RADIANT_MISSING;
      else if (strstr(field,"//")) {
	pthread_mutex_unlock(&mutex);
	continue; // skip header lines
      } else if (sscanf(field,"%d:%d:%d:%d",&days,&hours,&minutes,&seconds)!=4) 
	radiant.uptime = RADIANT_NOTUNDERSTOOD;
      else
	radiant.uptime = days*24*60*60 + hours*60*60 + minutes*60 + seconds;
      
      // COLL-T, STOR-T, AUX-1, AUX-2
      {
	double t[4];
	for(i=0;i<4;i++) {	
	  field = getNextField(NULL);
	  if (!field)
	    t[i] = RADIANT_MISSING;
	  else if (!strncmp(field,"OPEN",4)) 
	    t[i] = RADIANT_OPEN;
	  else if (!strncmp(field,"SHORT",5)) 
	    t[i] = RADIANT_SHORT;
	  else if (!strncmp(field,"?",1)) 
	    t[i] = RADIANT_UNKNOWN;
	  else if (sscanf(field,"%lf",&t[i])!=1)
	    t[i] = RADIANT_NOTUNDERSTOOD;
	}
	// work around issue in which periodically thermal values are all
	// zero presumably because we occaisonally get contention on the arduino
	// RX line between thermal controller and the serial output of the USB
	// to serial converter we use to connect to udoo
	if (t[0]!=0.0 || t[1]!=0.0 || t[2]!=0.0 || t[3]!=0.0) {
	  for(i=0;i<4;i++) {	
	    if (!radiant_isUnknown(t[i])) {
	      switch(i) {
	      case 0: radiant.thermal_collT = t[i]; LOGF("collT",t[i]); break;
	      case 1: radiant.thermal_storT = t[i]; LOGF("storT",t[i]); break;
	      case 2: radiant.thermal_aux1T = t[i]; LOGF("aux1T",t[i]); break;
	      case 3: radiant.thermal_aux2T = t[i]; LOGF("aux2T",t[i]); break;
	      }
	    }
	  }
	}
      }
      
      // PUMP UPLim
      for(i=0;i<2;i++) {	
	field = getNextField(NULL);
       if (!field) 
	 j = RADIANT_MISSING;
       else if (!strncmp(field,"ON",2)) 
	 j = 1;
       else if (!strncmp(field,".",1)) 
	 j = 0;
       else if (!strncmp(field,"?",1)) 
	 j = RADIANT_UNKNOWN;
       else 
	 j = RADIANT_NOTUNDERSTOOD;      
       if (!radiant_isUnknown(t)) {
	 switch(i) {
	 case 0: radiant.thermal_pump = j; LOGI("pump",j); break;
	 case 1: radiant.thermal_uplim = j; LOGI("uplim",j); break;
	 }
       }
      }
      
      // Vav Vpv T1st T2nd Tbas Tout TXchI TXchO THotO TRadR THTbR
      for(i=0;i<12;i++) {	
	field = getNextField(NULL);
	if (!field)
	  t = RADIANT_MISSING;
	else if (!strncmp(field,"?",1)) 
	  t = RADIANT_UNKNOWN;
	else if (sscanf(field,"%lf",&t)!=1)
	  t = RADIANT_NOTUNDERSTOOD;
	if (!radiant_isUnknown(t)) {
	  switch(i) {
	  case 0: radiant.Vac = t; LOGF("Vac",t); break;
	  case 1: radiant.Vpv = t; LOGF("Vpv",t); break;
	  case 2: radiant.T1st = t; LOGF("T1st",t); break;
	  case 3: radiant.T2nd = t; LOGF("T2nd",t); break;
	  case 4: radiant.Tbas = t; LOGF("Tbas",t); break;
	  case 5: radiant.Tout = t; LOGF("Tout",t); break;
	  case 6: radiant.TXchI = t; LOGF("TXchI",t); break;
	  case 7: radiant.TXchM = t; LOGF("TXchM",t); break;
	  case 8: radiant.TXchO = t; LOGF("TXchO",t); break;
	  case 9: radiant.THotO = t; LOGF("THotO",t); break;
	  case 10: radiant.TRadR = t; LOGF("TRadR",t); break;
	  case 11: radiant.THTbR = t; LOGF("THTbR",t); break;
	  }
	}
      }
      
      // Away HTub Sw3  Sw4  Radt HTub AuxF FDmp B1st B2nd
      for(i=0;i<12;i++) {	
	field = getNextField(NULL);
	if (!field)
	  j = RADIANT_MISSING;
	else if (!strncmp(field,"ON",2)) 
	  j = 1;
	else if (!strncmp(field,".",1)) 
	  j = 0;
	else 
	  j = RADIANT_UNKNOWN;
	if (!radiant_isUnknown(t)) {
	  switch(i) {
	  case 0: radiant.H1st = j; LOGI("H1st",j); break;
	  case 1: radiant.H2nd = j; LOGI("H2nd",j); break;
	  case 2: radiant.switchAway = j; LOGI("switchAway",j); break;
	  case 3: radiant.switchHTub = j; LOGI("switchHTub",j); break;
	  case 4: radiant.switchSw3 = j; LOGI("sw3",j); break;
	  case 5: radiant.switchSw4 = j; LOGI("sw4",j); break;
	  case 6: radiant.circulatorRadiant = j; LOGI("circRadiant",j); break;
	  case 7: radiant.circulatorHTub = j; LOGI("circHTub",j); break;
	  case 8: radiant.auxFET = j; LOGI("auxFET",j); break;
	  case 9: radiant.finnedDump = j; LOGI("finnedDump",j); break;
	  case 10: radiant.relay1st = j; LOGI("relay1st",j); break;
	  case 11: radiant.relay2nd = j; LOGI("relay2nd",j); break;
	  }
	}
      }

      // Troot
      for(i=0;i<1;i++) {	
	field = getNextField(NULL);
	if (!field)
	  t = RADIANT_MISSING;
	else if (!strncmp(field,"?",1)) 
	  t = RADIANT_UNKNOWN;
	else if (sscanf(field,"%lf",&t)!=1)
	  t = RADIANT_NOTUNDERSTOOD;
	if (!radiant_isUnknown(t)) {
	  switch(i) {
	  case 0: radiant.Troot = t; LOGF("Troot",t); break;
	  }
	}
      }
      
      pthread_mutex_unlock(&mutex);
    }    

  }

  /* restore the old port settings */
  tcsetattr(fd,TCSANOW,&oldtio);
}

int radiant_start(char *d)
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

int radiant_sample(RADIANT_t *r)
{
  pthread_mutex_lock(&mutex);
  *r = radiant;
  radiant = radiant_unknown;
  pthread_mutex_unlock(&mutex);
}
