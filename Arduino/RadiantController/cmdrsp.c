#include <inttypes.h>

#include <avr/interrupt.h>

#include <stdio.h>

#include "defines.h"
#include "i2c.h"
#include "cmdrsp.h"


int heatBoost1stFloorTargetTemp=0;
int heatBoost1stFloorMinutes=0;
int heatBoost2ndFloorTargetTemp=0;
int heatBoost2ndFloorMinutes=0;


#if NO_I2C 

void cmdrsp_poll(void)
{}

#else

/*

This module implements a command / response channel with a controlling agent.  Each command
and response is 8 bytes, with the last byte being a checksum.

Commands:

ESC 'H' {1,2} <Temp> <Minutes> 0 0 CHKSUM     
                                                  
    Boosts heat 'H' for 1st or 2nd floor to temperature specified for specified number 
    of minutes.

    Responses:

       ESC 'E' 0 0 0 0 0 CHKSUM     - success
       ESC 'E' x 0 0 0 0 CHKSUM     - error x (see error codes below)



*/

#define ESC 0x1b


#define ERROR_CODE_ok                      0
#define ERROR_CODE_badCheckSum             1
#define ERROR_CODE_badTargetTemperature    2
#define ERROR_CODE_badHeatZone             3
#define ERROR_CODE_unknownCommand          4
#define ERROR_CODE_noESC                   5
#define ERROR_CODE_badCommandLength        6

static volatile int i2cgetdone;
static volatile int i2cputdone;

static uint8_t in_data[8], out_data[8];


void i2cgetcb(uint8_t *s, int n, int error)
{
  i2cgetdone = n;
}

void i2cputcb(uint8_t *s, int n, int error)
{
  i2cputdone = n;
}

uint8_t checksum(uint8_t *data)
{
  uint8_t checksum = data[0] + data[1] + data[2] + data[3] + data[4] + data[5] + data[6];
  return checksum;
}

void constructResponse(int errorCode)
{
  out_data[0] = ESC;
  out_data[1] = 'E';
  out_data[2] = errorCode;
#if 0
  out_data[3] = 0;
  out_data[4] = 0;
  out_data[5] = 0;
  out_data[6] = 0;
#endif
  out_data[7] = checksum(out_data);
}


void cmdrsp_poll(void)
{
  static int needsInit = 1;
  static int get;

  if (needsInit) {
    i2cgetdone = 0;
    i2cputdone = 0;
    i2c_get(in_data,8,i2cgetcb);
    get = 1;
    needsInit = 0;
  }

  if (get) {
    if (i2cgetdone) {

      out_data[3] = in_data[0];
      out_data[4] = in_data[1];
      out_data[5] = in_data[2];
      out_data[6] = in_data[3];

      if (i2cgetdone==8) {

	// parse command / construct response
	if (checksum(in_data)==in_data[7]) {
	  if (in_data[0]==ESC) {
	    switch(in_data[1]) {
	      
	    case 'H':
	      switch(in_data[2]) {
	      case 1:
		if ((in_data[3] < 75) && (in_data[3] > 49)) {
		  heatBoost1stFloorTargetTemp = in_data[3];
		  heatBoost1stFloorMinutes = in_data[4];
		  constructResponse(ERROR_CODE_ok);
		} else {
		  // bad target temperature
		  heatBoost1stFloorMinutes = 0;
		  constructResponse(ERROR_CODE_badTargetTemperature);
		}
		break;
	      case 2:
		if ((in_data[3] < 75) && (in_data[3] > 49)) {
		  heatBoost2ndFloorTargetTemp = in_data[3];
		  heatBoost2ndFloorMinutes = in_data[4];
		  constructResponse(ERROR_CODE_ok);
		} else {
		  // bad target temperature
		  heatBoost2ndFloorMinutes = 0;
		  constructResponse(ERROR_CODE_badTargetTemperature);
		}
		break;
	      default:
		// bad heat zone
		heatBoost1stFloorMinutes = 0;
		heatBoost2ndFloorMinutes = 0;
		constructResponse(ERROR_CODE_badHeatZone);
		break;
	      }
	      break;
	      
	      
	    default:
	      // unknown command
	      constructResponse(ERROR_CODE_unknownCommand);
	      break;
	    }
	  } else {
	    // no ESC in byte 0
	    constructResponse(ERROR_CODE_noESC);
	  }	  
	} else {
	  // bad checksum
	  constructResponse(ERROR_CODE_badCheckSum);
	}
      } else {
	// command length not 8 bytes
	constructResponse(ERROR_CODE_badCommandLength);
      }
	
      i2c_put(out_data,8,i2cputcb);
      i2cgetdone = 0;
      get = 0;
    }
  } else {
    if (i2cputdone) {
      i2c_get(in_data,8,i2cgetcb);
      i2cputdone = 0;
      get = 1;
    }
  }

}

#endif

