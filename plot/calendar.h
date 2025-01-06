#ifndef __calendar_h__
#define __calendar_h__

#include "return_code.h"

#define BASE_YEAR 1970
#define MAX_YEAR  2100

typedef struct {
  int year;     // year, BASE_YEAR thru MAX_YEAR
  int month;    // month 1-12
  int mday;     // day of month 1-31
  int hour;     // hour 0-23
  int minute;   // minute 0-59
  int second;   // second 0-59
  int yday;     // day of the year 1-366
} CALENDAR_t;

// seconds since 00:00:00 Jan-01 BASE_YEAR
typedef long CALENDAR_TIME_t;


DTW_RETURN_CODE_t CALENDAR_to_time_convert 
( CALENDAR_t *calendar,
  CALENDAR_TIME_t *time );

DTW_RETURN_CODE_t CALENDAR_from_time_convert 
( CALENDAR_t *calendar,
  CALENDAR_TIME_t time );

DTW_RETURN_CODE_t CALENDAR_advanceSeconds
( CALENDAR_t *calendar,
  int seconds );

DTW_RETURN_CODE_t CALENDAR_advanceMinutes 
( CALENDAR_t *calendar,
  int minutes );

DTW_RETURN_CODE_t CALENDAR_advanceHours 
( CALENDAR_t *calendar,
  int hours );

DTW_RETURN_CODE_t CALENDAR_advanceDays 
( CALENDAR_t *calendar,
  int days );

#endif
