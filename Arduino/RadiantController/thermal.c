#include <string.h>
#include <stdio.h>

#include "uart.h"
#include "time.h"
#include "debug.h"
#include "led.h"
#include "thermal.h"

//#define DISABLE_THERMAL
#define DEBUG_THERMAL

volatile int rxdone = 0;

void rxcb(char *s, int n, int error)
{
  rxdone = 1;
}

#define MAXLEN (80+1)        // 80 characters plus termination 0
#define TIMEOUT 20           // 20 seconds
#define MAX_TIMEOUT_COUNT 5  // 5 consecutive timeouts then set thermal values to unknown

#define MAX_FAIL_COUNT 10

THERMAL_t thermal;

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

static int parseLine(char *s)
{
  int i, hours, minutes;
  char *field;
  double t, tmp;
  static int failCount = 0;


  // debug hook
#if 0
  if (s[0]==':') {
    debug(&s[1]);
	return;
  }
#endif

  // Parse the fields ... start all flags clear
  thermal.flags = 0;
        
  // RUNTIME
  field = getNextField(s);
  if (!field)
    return; // skip blank lines
  else if (strstr(field,"RUNTIME")) 
    return; // skip header lines
  else if (sscanf(field,"%d:%d",&hours,&minutes)!=2)
    thermal.flags |= THERMAL_FLAGS_notUnderstood;
  else
    thermal.runtime = hours*60 + minutes;
  
  // COLL-T, STOR-T, DIFF-T, HILI-T, AUX-1, AUX-2
  for(i=0;i<6;i++) {	
    field = getNextField(NULL);
    if (!field) {
      thermal.flags |= THERMAL_FLAGS_missingField;
      if (++failCount > MAX_FAIL_COUNT) 
	t = THERMAL_MISSING;
    } else if (!strncmp(field,"OPEN",4)) {
      thermal.flags |= (i==4 || i==5) ? 0 : THERMAL_FLAGS_thermistorOpen;
      t = THERMAL_OPEN;
    } else if (!strncmp(field,"SHRT",4)) {
      thermal.flags |= THERMAL_FLAGS_thermistorShort;
      t = THERMAL_SHORT;
    } else if (sscanf(field,"%lf",&tmp)!=1) {
      thermal.flags |= THERMAL_FLAGS_notUnderstood;
      if (++failCount > MAX_FAIL_COUNT) 
	t = THERMAL_NOTUNDERSTOOD;
    } else {
      t = tmp;
    }
    switch(i) {
    case 0: thermal.collT = t; break;
    case 1: thermal.storT = t; break;
    case 2: thermal.diffT = t; break;
    case 3: thermal.hiliT = t; break;
    case 4: thermal.aux1T = t; break;
    case 5: thermal.aux2T = t; break;
    }
  }
  
  // PUMP
  field = getNextField(NULL);
  if (!field) 
    thermal.flags |= THERMAL_FLAGS_missingField;
  else if (!strncmp(field,"ON",2)) 
    thermal.pump = 1;
  else if (!strncmp(field,"OFF",3)) 
    thermal.pump = 0;
  else 
    thermal.flags |= THERMAL_FLAGS_notUnderstood;      
  
  // UPLim
  field = getNextField(NULL);
  if (!field) 
    thermal.flags |= THERMAL_FLAGS_missingField;
  else if (!strncmp(field,"ON",2)) 
    thermal.uplim = 1;
  else if (!strncmp(field,"OFF",3)) 
    thermal.uplim = 0;
  else 
    thermal.flags |= THERMAL_FLAGS_notUnderstood;            

  // reset fail count if it appears we have parsed OK
  if ( !(thermal.flags &
	 (THERMAL_FLAGS_commLinkDown |
	  THERMAL_FLAGS_missingField |
	  THERMAL_FLAGS_notUnderstood)) )
    failCount = 0; 

#ifdef DEBUG_THERMAL
#if 0
  debug_puts("d: thermal summary:\n");
  if (thermal.flags & THERMAL_FLAGS_commLinkDown) debug_puts("d: FLAGS_commLinkDown\n");
  if (thermal.flags & THERMAL_FLAGS_missingField) debug_puts("d: FLAGS_missingField\n");
  if (thermal.flags & THERMAL_FLAGS_notUnderstood) debug_puts("d: FLAGS_notUnderstood\n");
  for(i=0;i<6;i++) {	
    switch(i) {
    case 0: t = thermal.collT; debug_puts("d: collT "); break;
    case 1: t = thermal.storT; debug_puts("d: storT "); break;
    case 2: t = thermal.diffT; debug_puts("d: diffT "); break;
    case 3: t = thermal.hiliT; debug_puts("d: hiliT "); break;
    case 4: t = thermal.aux1T; debug_puts("d: aux1T "); break;
    case 5: t = thermal.aux2T; debug_puts("d: aux2T "); break;
    }
    if (t==THERMAL_MISSING) debug_puts("MISSING\n");
    else if (t==THERMAL_OPEN) debug_puts("OPEN\n");
    else if (t==THERMAL_SHORT) debug_puts("SHRT\n");
    else if (t==THERMAL_NOTUNDERSTOOD) debug_puts("NOTUNDERSTOOD\n");
    else {
      char s[16];
      sprintf(s,"%lf\n",t);
      debug_puts(s);
    }
  }
#endif
#endif
  
  return(failCount);
}

void thermal_poll(void)
{
  static char s[MAXLEN];
  static int needsInit = 1;
  static long t0;
  static int timeoutCount = 0;

#ifndef DISABLE_THERMAL
  if (needsInit || rxdone) {
    if (!needsInit) {
#ifdef DEBUG_THERMAL
      debug_puts("d: thermal recv ");
      debug_puts(s);
#endif
      if (!parseLine(s)) timeoutCount=0;
    }
    needsInit = 0;
    rxdone = 0;
    t0 = time_get();
    uart2400_gets(s, sizeof(s), rxcb);
  }

  else if (time_elapsed_sec(t0) > TIMEOUT) {
    t0 = time_get();
    if (timeoutCount < MAX_TIMEOUT_COUNT) {
      timeoutCount++;
      debug_puts("d: thermal timeout\n");
      if (timeoutCount == MAX_TIMEOUT_COUNT) {
	debug_puts("d: thermal max fail count\n");
	thermal.flags = THERMAL_FLAGS_commLinkDown;
	thermal.collT = THERMAL_UNKNOWN;
	thermal.storT = THERMAL_UNKNOWN;
	thermal.diffT = THERMAL_UNKNOWN;
	thermal.hiliT = THERMAL_UNKNOWN;
	thermal.aux1T = THERMAL_UNKNOWN;
	thermal.aux2T = THERMAL_UNKNOWN;
	thermal.pump = THERMAL_UNKNOWN;
	thermal.uplim = THERMAL_UNKNOWN;
      }
    }
  }

  if (thermal.flags & 
      ( THERMAL_FLAGS_commLinkDown |
	THERMAL_FLAGS_missingField |
	THERMAL_FLAGS_notUnderstood )) {
    ledOff(LED_THERMAL_STATUS_OK);
  } else {
    ledOn(LED_THERMAL_STATUS_OK);
  }
#endif
  
}
