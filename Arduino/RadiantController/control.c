#include <inttypes.h>

#include "switch.h"
#include "power.h"
#include "time.h"
#include "thermal.h"
#include "temperature.h"
#include "fetswitch.h"
#include "relay.h"
#include "adc.h"
#include "led.h"
#include "cmdrsp.h"

#include "control.h"

/*

Solar Thermal Heat Dump Control

The solar panels are expected to generate more heat than will be used to pre-heat for domestic 
hot water.  In order to prevent the storage tank or the collectors to get too hot (tank is rated
to 175 F, gycol in collector loop will gum up if continually overheated), excess heat must be 
dumped.  We will have two dump zones: radiant floor heat on 1st floor of the house and a 
finned tube annex to the glycol loop in the basement.

In addition, we need to try to maintain a mimumum storage tank temperature in order to provide
a high enough temperature to kill Legionella bacteria, and thus maintain sanitary water.  
Legionella populations breed and increase at around body temp (96 F).  Above 115-120, Legionella
populations decrease with time.  The higher the temp, the faster the Legionalla die.

Inputs:

thermal.collT - thermal collector temperature
thermal.storT - thermal storage tank temperature
gTemperature[TEMPERATURE_1stFloor] - inside temperature at 1st floor of house
switches & SWITCH_away - switch indicating away (no DHW) rather than home (tank for DHW)
switches & SWITCH_heatHotTub - switch indicating user wants heat to hot tub
switches & SWITCH_noHeat - switch indicating that don't want any heat into radiant floor
switches & SWITCH_thermostatHotTub - perform thermostat control for hot tub

Outputs:

circulatorRadiant - radiant floor loop circulator
circulatorHotTub - circulator two flat plate heat exchangers
finnedTubeDump - zone valve to divert glycol thru finned tube dump path

*/

#define THERMAL_DUMP_LIMIT_COLLECTOR    (195.0)   // collector temp F above which to dump
#define THERMAL_2XDUMP_LIMIT_COLLECTOR  (205.0)   // collector temp F above which to do 2x dump
#define THERMAL_DUMP_HI_LIMIT_TANK      (165.0)   // storage temp F above which to dump
#define THERMAL_2XDUMP_HI_LIMIT_TANK    (170.0)   // storage temp F above which to do 2x dump
#define THERMAL_DUMP_LO_LIMIT_TANK      (125.0)   // minimum storage temp F for heat extraction
#define THERMAL_DUMP_RADIANT_THRESHOLD  ( 72.0)   // 1st floor temp below which to dump to radiant
#define THERMAL_DUMP_HOTTUB_LO_LIMIT    ( 85.0)   // temperature which indicates hot tub jets on
#define THERMAL_DUMP_HOTTUB_HI_LIMIT    ( 98.0)   // hot tub temp above which to stop dumping

int circulatorRadiant = 0;
int circulatorHotTub = 0;
int finnedTubeDump = 0;

static void control_thermalDump(void)
{
  uint8_t needDump, need2xDump, needHeat, needHeatNoDHW, needHotTub, needXferBoost;
  uint8_t doRadiantDump, doFinnedTubeDump, doXferBoost;
  static long t0 = 0;

  if (time_elapsed_sec(t0) > 20) {


    // Full control (but no hot tub)

    if (thermal_isUnknown(thermal.collT) || thermal_isUnknown(thermal.storT))
      needDump = 0;
    else
      needDump = 
	(thermal.collT > THERMAL_DUMP_LIMIT_COLLECTOR) ||
	(thermal.storT > THERMAL_DUMP_HI_LIMIT_TANK);
    
    if (thermal_isUnknown(thermal.collT) || thermal_isUnknown(thermal.storT))
      need2xDump = 0;
    else
      need2xDump =
	(thermal.collT > THERMAL_2XDUMP_LIMIT_COLLECTOR) ||
	(thermal.storT > THERMAL_2XDUMP_HI_LIMIT_TANK);
    
    if (thermal_isUnknown(thermal.collT) || thermal_isUnknown(thermal.storT))
      needXferBoost = 0;
    else
      needXferBoost = 
	(thermal.collT > (thermal.storT + 10));

    // determine if need heat, and tank is hot enough to shed heat from the perspective
    // of use as domestic hot water, and considering minimum temperature for Legionella 
    // sanitation
    if ( gTemperature[TEMPERATURE_1stFloor] == TEMPERATURE_UNKNOWN ||
	 thermal_isUnknown(thermal.storT) ||
	 (switches & SWITCH_noHeat) )
      needHeat = 0;
    else
      needHeat =
	(gTemperature[TEMPERATURE_1stFloor] < THERMAL_DUMP_RADIANT_THRESHOLD) &&
	(thermal.storT > THERMAL_DUMP_LO_LIMIT_TANK);

    // determine if need heat, assume that tank is NOT used for domestic hot water.  in
    // this case, no attempt is made to keep tank temp high enough to sanitize against 
    // Legionella
    if ( thermal_isUnknown(thermal.storT) ||
	 (switches & SWITCH_noHeat) )
      needHeatNoDHW = 0;
    else if ( gTemperature[TEMPERATURE_1stFloor] == TEMPERATURE_UNKNOWN )
      needHeatNoDHW = (thermal.storT > 70.0);
    else
      needHeatNoDHW =
	(gTemperature[TEMPERATURE_1stFloor] < THERMAL_DUMP_RADIANT_THRESHOLD) &&
	(thermal.storT > (gTemperature[TEMPERATURE_1stFloor] + 2));
    

    if (switches & SWITCH_heatHotTub) 
      needHotTub = 1;
    else if ( thermal_isUnknown(thermal.hotTubT) ||
	      thermal_isUnknown(thermal.storT) ||
	      !(switches & SWITCH_thermostatHotTub) )
      needHotTub = 0;
    else
      needHotTub = 
	(thermal.hotTubT > THERMAL_DUMP_HOTTUB_LO_LIMIT) &&
	(thermal.hotTubT < THERMAL_DUMP_HOTTUB_HI_LIMIT) &&
	(thermal.storT > THERMAL_DUMP_LO_LIMIT_TANK);

    doRadiantDump = 
      ((switches & SWITCH_away) ? needHeatNoDHW : needHeat) ||
      (needDump && needHeat) ||
      need2xDump;
    
    doFinnedTubeDump =
      (needDump && !doRadiantDump) ||
      need2xDump;
    
    doXferBoost = (!doRadiantDump && needXferBoost) || needHotTub;
    
    circulatorRadiant = doRadiantDump;
    circulatorHotTub = doXferBoost;
    finnedTubeDump = doFinnedTubeDump;

    if (circulatorRadiant) {
      fetSwitchOn(CIRCULATOR_Radiant);
    } else {
      fetSwitchOff(CIRCULATOR_Radiant);
    }
    if (circulatorHotTub) {
      fetSwitchOn(CIRCULATOR_HotTub);
    } else {
      fetSwitchOff(CIRCULATOR_HotTub);
    }
    if (finnedTubeDump) {
      relayOn(RELAY_finnedTubeDump);
    } else {
      relayOff(RELAY_finnedTubeDump);
    }

    t0 = time_get();
  }

  
}


/*

House heat call monitoring

This radiant controller also monitors the internal house temperatures, and the state of the forced
hot air heating system (ie. whether the 1st floor or 2nd floor thermostats are calling for heat).

*/

//#define DEBUG_TSTAT

int heat1stFloor = 0;
int heat2ndFloor = 0;

int tstat0,tstat1;

static void control_houseHeat(void)
{
  static long t0 = 0;
  static long t0HeatBoost = 0;

  if (time_elapsed_sec(t0) > 20) {


    tstat0 = adcSample(ADC_CHANNEL_Tstat0);
    tstat1 = adcSample(ADC_CHANNEL_Tstat1);

#ifdef DEBUG_TSTAT
    {
      char s[48];
      sprintf(s,"d: Tstat{0,1} = %d, %d\n",tstat0,tstat1);
      debug_puts(s);
    }
#endif
    
    // don't know why these are not balanced ..
    heat1stFloor = tstat0 < 128;
    heat2ndFloor = tstat1 < 224;

    t0 = time_get();

  }

}


double Vmain,Vpv;
unsigned upTime;

void control_poll(void)
{

  Vmain = getPowerVoltage(POWER_Vmain);
  Vpv = getPowerVoltage(POWER_Vpv);

#if (D2_USED_BY_ONEWIRE==0)
  if (Vmain < 8.0)
    ledOff(LED_MAIN_POWER_OK);
  else
    ledOn(LED_MAIN_POWER_OK);
#endif

#if (D3_USED_BY_F007TH==0)
  if (Vpv < 8.0)
    ledOff(LED_PV_POWER_OK);
  else
    ledOn(LED_PV_POWER_OK);
#endif
  
  upTime = time_elapsed_sec((long) 0 );
  temperature_UpdateTemperatures();

  switch_sample();

  control_thermalDump();
  control_houseHeat();
}
