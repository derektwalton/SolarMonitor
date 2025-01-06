#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "uart.h"
#include "control.h"
#include "time.h"
#include "thermal.h"
#include "temperature.h"
#include "switch.h"

static volatile int txdone = 1;

static void txcb(void)
{
  txdone = 1;
}

#define HEADER_INTERVAL 20 // header every 20 lines

static char header0[] = "// Uptime    |    Thermal Controller Outputs    |                          Radiant Controller Inputs                            |     Switches      |     Controller Outputs      | Extra |";
static char header1[] = "// d:h:m:s    CollT  StorT  Aux1T  Aux2T Pmp UpL  Vac  Vpv T1st  T2nd  Tbas  Tout  TXchI TXchM TXchO THotO TRadR THTbR H1st H2nd Away HTub Sw3  Sw4  Radt HTub AuxF FDmp B1st B2nd  Troot  ";

void transmit(void)
{
  static char s[sizeof(header0)+3+5]; // +3 for cr, lf, and 0 termination, +5 for safety
  static long t0=0;
  static int count=0;
  static int state=0;
  int i;
  double t;
  char *ss;

  if (txdone && time_elapsed_mSec(t0)>2000) {
    switch(state) {

    case 0:
      txdone = 0;
      sprintf(s,"%s%c%c",header0,0xd,0xa);
      uart_puts(s, txcb);
      state = 1;
      break;

    case 1:
      txdone = 0;
      sprintf(s,"%s%c%c",header1,0xd,0xa);
      uart_puts(s, txcb);
      state = 2;
      break;

    case 2:

      sprintf(s,"%3.3d:%2.2d:%2.2d:%2.2d ",
	      upTime / 60 / 60 / 24, // days
	      upTime / 60 / 60 % 24, // hours
	      upTime / 60 % 60, // minutes
	      upTime % 60); // seconds
      for(i=0;i<4;i++) {
	switch(i) {
	case 0: t = thermal.collT; break;
	case 1: t = thermal.storT; break;
	case 2: t = thermal.aux1T; break;
	case 3: t = thermal.aux2T; break;
	}
#if 0
	// synthesize interesting thermal system data stream
	switch(rand()&7) {
	case 0: t = 0; break;
	case 1: t = 128.8; break;
	case 2: t = -40.1; break;
	case 3: t = 0.5; break;
	case 4: t = THERMAL_OPEN; break;
	case 5: t = THERMAL_SHORT; break;
	case 6: t = -0.1; break;
	}
	if (rand() & 1) 
	  thermal.flags |= THERMAL_FLAGS_commLinkDown;
	else
	  thermal.flags &= ~THERMAL_FLAGS_commLinkDown;
#endif
	ss = s+strlen(s);
	if (thermal.flags & THERMAL_FLAGS_commLinkDown)
	  sprintf(ss,"   ?   ");
	else if (t == THERMAL_OPEN) 
	  sprintf(ss," OPEN  ");
	else if (t == THERMAL_SHORT) 
	  sprintf(ss," SHORT ");
	else
	  sprintf(ss,"% 6.1f ",t);
      }
      sprintf(s+strlen(s),"%s ",thermal_isUnknown(thermal.pump) ? " ? " : thermal.pump ? "ON " : " . ");
      sprintf(s+strlen(s),"%s ",thermal_isUnknown(thermal.uplim) ? " ? " : thermal.uplim ? "ON " : " . ");
      
      sprintf(s+strlen(s),"%4.1f ",Vmain);
      sprintf(s+strlen(s),"%4.1f ",Vpv);
      for(i=0;i<10;i++) {
	switch(i) {
	case 0: t = gTemperature[TEMPERATURE_1stFloor]; break;
	case 1: t = gTemperature[TEMPERATURE_2ndFloor]; break;
	case 2: t = gTemperature[TEMPERATURE_Basement]; break;
	case 3: t = gTemperature[TEMPERATURE_Outside]; break;
	case 4: t = gTemperature[TEMPERATURE_GlycolExchIn]; break;
	case 5: t = gTemperature[TEMPERATURE_GlycolExchMid]; break;
	case 6: t = gTemperature[TEMPERATURE_GlycolExchOut]; break;
	case 7: t = gTemperature[TEMPERATURE_TankHotOut]; break;
	case 8: t = gTemperature[TEMPERATURE_RadiantReturn]; break;
	case 9: t = gTemperature[TEMPERATURE_HotTubReturn]; break;
	}
#if 0
	// synthesize interesting temp data stream
	switch(rand()&7) {
	case 0: t = 0; break;
	case 1: t = 99.9; break;
	case 2: t = -40.1; break;
	case 3: t = 0.5; break;
	case 4: t = TEMPERATURE_UNKNOWN; break;
	case 5: t = 75.6; break;
	case 6: t = -0.1; break;
	}
#endif
	ss = s+strlen(s);
	if (t == TEMPERATURE_UNKNOWN) 
	  sprintf(ss,"   ?  ");
	else
	  sprintf(ss,"%5.1f ",t);
      }
      
      sprintf(s+strlen(s),"%s ",heat1stFloor ? " ON " : " .  ");
      sprintf(s+strlen(s),"%s ",heat2ndFloor ? " ON " : " .  ");
      
      sprintf(s+strlen(s),"%s ",(switches & SWITCH_away) ? " ON " : " .  ");
      sprintf(s+strlen(s),"%s ",(switches & SWITCH_heatHotTub) ? " ON " : " .  ");
      sprintf(s+strlen(s),"%s ",(switches & SWITCH_3) ? " ON " : " .  ");
      sprintf(s+strlen(s),"%s ",(switches & SWITCH_4) ? " ON " : " .  ");
      
      sprintf(s+strlen(s),"%s ",circulatorRadiant ? " ON " : " .  ");
      sprintf(s+strlen(s),"%s ",circulatorHotTub ? " ON " : " .  ");
      sprintf(s+strlen(s),"%s ",0 ? " ON " : " .  ");
      sprintf(s+strlen(s),"%s ",finnedTubeDump ? " ON " : " .  ");
      sprintf(s+strlen(s),"%s ",heat1stFloorBoost ? " ON " : " .  ");
      sprintf(s+strlen(s),"%s ",heat2ndFloorBoost ? " ON " : " .  ");
      
      t = gTemperature[TEMPERATURE_RootCellar];
      ss = s+strlen(s);
      if (t == TEMPERATURE_UNKNOWN) 
	sprintf(ss,"   ?  ");
      else
	sprintf(ss,"%5.1f ",t);
      
      sprintf(s+strlen(s),"%c%c",0xd,0xa);
      
      txdone = 0;
      t0 = time_get();
      uart_puts(s, txcb);
      
      if (++count == HEADER_INTERVAL) {
	count = 0;
	state = 0;
      }
      break;

    }
  }
}
