#include <stdio.h>
#include <stdlib.h>

#include "calendar.h"

#define N_YEARS         (MAX_YEAR - BASE_YEAR + 1)

#define SECS_PER_MINUTE (60)
#define SECS_PER_HOUR   (60 * 60)
#define SECS_PER_DAY    (60 * 60 * 24)

static int isLeapYear(int year)
{
  if ((year % 400)==0)
    return 1;
  else if ((year%100)==0)
    return 0;
  else if ((year%4)==0)
    return 1;
  else 
    return 0;
}

static int leapYear[N_YEARS];
static int accumulatedLeapDays[N_YEARS];

static int daysNonLeapYear[12] = 
  { 31, // Jan
    28, // Feb
    31, // Mar
    30, // Apr
    31, // May
    30, // Jun
    31, // Jul
    31, // Aug
    30, // Sep
    31, // Oct
    30, // Nov
    31  // Dec
  };

static int daysLeapYear[12] = 
  { 31, // Jan
    29, // Feb
    31, // Mar
    30, // Apr
    31, // May
    30, // Jun
    31, // Jul
    31, // Aug
    30, // Sep
    31, // Oct
    30, // Nov
    31  // Dec
  };

static int accumulatedDaysNonLeapYear[12];
static int accumulatedDaysLeapYear[12];

static int calendarInitialized=0;

DTW_RETURN_CODE_t CALENDAR_init()
{
  int i;

  for(i=0;i<N_YEARS;i++) {
    leapYear[i]=isLeapYear(BASE_YEAR+i);
    if (i==0) {
      accumulatedLeapDays[i] = 0;
    } else {
      accumulatedLeapDays[i] = accumulatedLeapDays[i-1] + leapYear[i-1];
    }
  }
  
  for(i=0;i<12;i++) {
    if (i==0) {
      accumulatedDaysNonLeapYear[i] = 0;
      accumulatedDaysLeapYear[i] = 0;
    } else {
      accumulatedDaysNonLeapYear[i] = accumulatedDaysNonLeapYear[i-1] + daysNonLeapYear[i-1];
      accumulatedDaysLeapYear[i] = accumulatedDaysLeapYear[i-1] + daysLeapYear[i-1];
    }
  }

  calendarInitialized = 1;

  return DTW_SUCCESS;
}


DTW_RETURN_CODE_t CALENDAR_to_time_convert 
( CALENDAR_t *calendar,
  CALENDAR_TIME_t *time )
{
  if ( !calendar || !time )
    return DTW_BAD_ARGUMENT;

  if (!calendarInitialized)
    CALENDAR_init();

  // year
  if (calendar->year < BASE_YEAR || calendar->year > MAX_YEAR)
    return DTW_BAD_ARGUMENT;
  *time = ((calendar->year - BASE_YEAR) * 365 + accumulatedLeapDays[calendar->year - BASE_YEAR]) * SECS_PER_DAY;

  // month
  if (calendar->month < 1 || calendar->month > 12)
    return DTW_BAD_ARGUMENT;
  if (leapYear[calendar->year - BASE_YEAR]) 
    *time += accumulatedDaysLeapYear[calendar->month-1] * SECS_PER_DAY;
  else
    *time += accumulatedDaysNonLeapYear[calendar->month-1] * SECS_PER_DAY;

  // day
  if (calendar->mday < 1 ||
      leapYear[calendar->year - BASE_YEAR] && (calendar->mday > daysLeapYear[calendar->month-1]) ||
      !leapYear[calendar->year - BASE_YEAR] && (calendar->mday > daysNonLeapYear[calendar->month-1]) )
    return DTW_BAD_ARGUMENT;
  *time += (calendar->mday - 1) * SECS_PER_DAY;

  // hour
  if (calendar->hour < 0 || calendar->hour >=24)
    return DTW_BAD_ARGUMENT;
  *time += calendar->hour * 60 * 60;

  // minute
  if (calendar->minute < 0 || calendar->minute >=60)
    return DTW_BAD_ARGUMENT;
  *time += calendar->minute * 60;

  // second
  if (calendar->second < 0 || calendar->second >=60)
    return DTW_BAD_ARGUMENT;
  *time += calendar->second;

  return DTW_SUCCESS;
}


DTW_RETURN_CODE_t CALENDAR_from_time_convert 
( CALENDAR_t *calendar,
  CALENDAR_TIME_t time )
{
  if ( !calendar || !time )
    return DTW_BAD_ARGUMENT;

  if (!calendarInitialized)
    CALENDAR_init();

  // year
  calendar->year = time / 365 / SECS_PER_DAY + BASE_YEAR;
  if ( ((calendar->year - BASE_YEAR) * 365 + accumulatedLeapDays[calendar->year - BASE_YEAR]) * SECS_PER_DAY 
       > time )
    calendar->year--;
  time -= ((calendar->year - BASE_YEAR) * 365 + accumulatedLeapDays[calendar->year - BASE_YEAR]) * SECS_PER_DAY;

  // month
  for (calendar->month=1;calendar->month<=12;calendar->month++) {
    if (leapYear[calendar->year - BASE_YEAR]) {
      if (calendar->month==12 || time < (accumulatedDaysLeapYear[calendar->month] * SECS_PER_DAY)) {
	calendar->yday = accumulatedDaysLeapYear[calendar->month-1];
	time -= (accumulatedDaysLeapYear[calendar->month-1] * SECS_PER_DAY);
	break;
      }
    } else {
      if (calendar->month==12 || time < (accumulatedDaysNonLeapYear[calendar->month] * SECS_PER_DAY)) {
	calendar->yday = accumulatedDaysNonLeapYear[calendar->month-1];
	time -= (accumulatedDaysNonLeapYear[calendar->month-1] * SECS_PER_DAY);
	break;
      }
    }
  }
  
  // day
  calendar->mday = (time / SECS_PER_DAY) + 1;
  calendar->yday += calendar->mday;
  time -= (calendar->mday - 1) * SECS_PER_DAY;

  // hour
  calendar->hour = time / SECS_PER_HOUR;
  time -= calendar->hour * SECS_PER_HOUR;

  // minute
  calendar->minute = time / SECS_PER_MINUTE;
  time -= calendar->minute * SECS_PER_MINUTE;

  // second
  calendar->second = time;

  return DTW_SUCCESS;
}


DTW_RETURN_CODE_t CALENDAR_advanceSeconds
( CALENDAR_t *calendar,
  int seconds )
{
	CALENDAR_TIME_t time;
	CALENDAR_to_time_convert(calendar,&time);
	time += seconds;
	CALENDAR_from_time_convert(calendar,time);
	return DTW_SUCCESS;
}

DTW_RETURN_CODE_t CALENDAR_advanceMinutes 
( CALENDAR_t *calendar,
  int minutes )
{
  CALENDAR_advanceSeconds(calendar, minutes*60);
	return DTW_SUCCESS;
}

DTW_RETURN_CODE_t CALENDAR_advanceHours 
( CALENDAR_t *calendar,
  int hours )
{
  CALENDAR_advanceSeconds(calendar, hours*60*60);
	return DTW_SUCCESS;
}

DTW_RETURN_CODE_t CALENDAR_advanceDays 
( CALENDAR_t *calendar,
  int days )
{
  CALENDAR_advanceSeconds(calendar, days*24*60*60);
	return DTW_SUCCESS;
}




#if 0

#include <time.h>

/*
** Test out the calendar implementation.  Compare with ctime.h.
*/

int main(int argc, char *argv[])
{
  struct tm std_tm;
  time_t std_time;

  CALENDAR_t calendar;
  CALENDAR_TIME_t calendar_time, calendar_time0;

  int seconds;
  int i;


  std_time = time(&std_time);
  std_tm = *gmtime(&std_time);

  calendar.year = std_tm.tm_year + 1900;
  calendar.month = std_tm.tm_mon + 1;
  calendar.mday = std_tm.tm_mday;
  calendar.hour = std_tm.tm_hour;
  calendar.minute = std_tm.tm_min;
  calendar.second = std_tm.tm_sec;

  CALENDAR_to_time_convert(&calendar, &calendar_time);

  seconds = 23;

  for(i=0;i < 5*365*SECS_PER_DAY;i+=seconds) {

  calendar_time += seconds;
  std_time += seconds;

#if 0
	fprintf(stdout,"%d/%d/%d %.2d:%.2d:%.2d + %d => ",
	    std_tm.tm_year+1900,
	    std_tm.tm_mon+1,
	    std_tm.tm_mday,
	    std_tm.tm_hour,
	    std_tm.tm_min,
	    std_tm.tm_sec,
		seconds);
#endif

    std_tm = *gmtime(&std_time);

#if 0
	fprintf(stdout,"%d/%d/%d %.2d:%.2d:%.2d\n",
	    std_tm.tm_year+1900,
	    std_tm.tm_mon+1,
	    std_tm.tm_mday,
	    std_tm.tm_hour,
	    std_tm.tm_min,
	    std_tm.tm_sec);
#endif

    CALENDAR_from_time_convert(&calendar, calendar_time);
    CALENDAR_to_time_convert(&calendar, &calendar_time);
    CALENDAR_from_time_convert(&calendar, calendar_time);

    if ( calendar.year != (std_tm.tm_year+1900) ||
	 calendar.month != (std_tm.tm_mon + 1) ||
	 calendar.mday != std_tm.tm_mday ||
	 calendar.hour != std_tm.tm_hour ||
	 calendar.minute != std_tm.tm_min ||
	 calendar.second != std_tm.tm_sec )
      {
	fprintf(stderr,"year %d %d\n", calendar.year , (std_tm.tm_year+1900));
	fprintf(stderr,"month %d %d\n", calendar.month , (std_tm.tm_mon + 1));
	fprintf(stderr,"day %d %d\n", calendar.mday , std_tm.tm_mday);
	fprintf(stderr,"hour %d %d\n", calendar.hour , std_tm.tm_hour);
	fprintf(stderr,"minute %d %d\n", calendar.minute , std_tm.tm_min);
	fprintf(stderr,"second %d %d\n", calendar.second , std_tm.tm_sec);
      }

    }

}

#endif
