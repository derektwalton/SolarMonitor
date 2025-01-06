#include <inttypes.h>

#include <avr/interrupt.h>

#include <stdio.h>

#include "defines.h"
#include "uart.h"
#include "fetswitch.h"
#include "adc.h"
#include "switch.h"
#include "led.h"
#include "power.h"
#include "time.h"
#include "temperature.h"
#include "debug.h"

static volatile int txdone;

static void txcb(void)
{
  txdone = 1;
}

void debug(char *s)
{

  switch(s[0]) {
    
  case 'F':
    // s[0]: F = FETSWITCH
    switch(s[1]) {
      // s[1]: {0,1,2} FETSWITCH number
    case '0':
      switch(s[2]) {
      case '0':
	fetSwitchOff(FETSWITCH_0);
	break;
      case '1':
	fetSwitchOn(FETSWITCH_0);
	break;
      }
      break;
    case '1':
      switch(s[2]) {
      case '0':
	fetSwitchOff(FETSWITCH_1);
	break;
      case '1':
	fetSwitchOn(FETSWITCH_1);
	break;
      }
      break;
    case '2':
      switch(s[2]) {
      case '0':
	fetSwitchOff(FETSWITCH_2);
	break;
      case '1':
	fetSwitchOn(FETSWITCH_2);
	break;
      }
      break;
    }
    break;
    
  case 'L':
    // s[0]: L = LED
    switch(s[1]) {
      // s[1]: {0,1,2} LED number
    case '0':
      switch(s[2]) {
      case '0':
	ledOff(LED_9);
	break;
      case '1':
	ledOn(LED_9);
	break;
      }
      break;
    case '1':
      switch(s[2]) {
      case '0':
	ledOff(LED_10);
	break;
      case '1':
	ledOn(LED_10);
	break;
      }
      break;
    case '2':
      switch(s[2]) {
      case '0':
	ledOff(LED_11);
	break;
      case '1':
	ledOn(LED_11);
	break;
      }
      break;
    case '3':
      switch(s[2]) {
      case '0':
	ledOff(LED_12);
	break;
      case '1':
	ledOn(LED_12);
	break;
      }
      break;
    }
    break;
    
  case 'A':
    // s[0]: A = ADC
    {
      ADC_CHANNEL_t channel;
      int result;
      channel = s[1] - '0';
      result = adcSample(channel);
      sprintf(s,"channel %d = %d\n",channel,result);
    }
    break;
    
  case 'S':
    {
      int result;
      result = switch_sample();
      sprintf(s,"sw1=%d, sw2=%d, sw3=%d, sw4=%d\n",
	      (result & SWITCH_1) ? 1 : 0,
	      (result & SWITCH_2) ? 1 : 0,
	      (result & SWITCH_3) ? 1 : 0,
	      (result & SWITCH_4) ? 1 : 0);
    }
    break;
    
  case 'V':
    // s[0]: V = power supply voltages
    {
      double Vmain,Vpv;
      Vmain = getPowerVoltage(POWER_Vmain);
      Vpv = getPowerVoltage(POWER_Vpv);
      sprintf(s,"Vmain = %lf, Vpv = %lf\n",Vmain,Vpv);
    }
    break;
    
  case 'U':
    // s[0]: U = up time
    {
      int upTime;
      upTime = time_elapsed_mSec((long) 0 );
      sprintf(s,"uptime = %d mSec\n",upTime);
    }
    break;
    
  case 'T':
    // s[0]: T = one wire temperatures
    if (temperature_UpdateTemperatures()==SUCCESS) {
      sprintf(s,"temps = %lf, %lf, %lf, %lf\n",
	      gTemperature[TEMPERATURE_1stFloor],
	      gTemperature[TEMPERATURE_2ndFloor],
	      gTemperature[TEMPERATURE_Basement],
	      gTemperature[TEMPERATURE_Outside] );
    } else {
      sprintf(s,"temp update failed\n");
    }
    break;
    
  }

  txdone = 0;
  uart_puts(s, txcb);
  while (!txdone);
}
