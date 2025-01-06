#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "calendar.h"

#define DTW_SUCCESS 0
#define DTW_ERROR   1
#define DTW_EOF     2
#define DTW_BAD_SAMPLE_DATASTREAM 3
#define DTW_BAD_SAMPLE_DATETIME   4
#define DTW_BAD_SAMPLE_VALUE 5
#define DTW_NODATA 6


#define ISODATETIME_SIZE 64
#define SAMPLENAME_SIZE 32

#define min(a,b) ((a)<(b) ? (a) : (b))
#define max(a,b) ((a)>(b) ? (a) : (b))

//
// Define the sample names as represented in the input merged files.
//

typedef enum {
  DATATYPE_double,
  DATATYPE_int,
  DATATYPE_boolean
} DATATYPE_t;

typedef struct {
  char name[SAMPLENAME_SIZE];   // Lake house sample names
  DATATYPE_t type;
} MERGE_t;

MERGE_t merge[] =
  {
    { "T1st", DATATYPE_double },
    { "T2nd", DATATYPE_double },
    { "Tbas", DATATYPE_double },
    { "Tout", DATATYPE_double },
    { "Tcoll", DATATYPE_double },
    { "Tstor", DATATYPE_double },
    { "TXchI", DATATYPE_double },
    { "TXchM", DATATYPE_double },
    { "TXchO", DATATYPE_double },
    { "THotO", DATATYPE_double },
    { "TRadR", DATATYPE_double },
    { "THtbR", DATATYPE_double },
    { "circSolar", DATATYPE_boolean },
    { "circRadiant", DATATYPE_boolean },
    { "circXchg", DATATYPE_boolean },
    { "finnedDump", DATATYPE_boolean },
    { "PVacPower", DATATYPE_double },
    { "PVacEnergy", DATATYPE_double },
    { "Thottub", DATATYPE_double },
    { "Troot", DATATYPE_double },
    { "Tstat_1st", DATATYPE_double },
    { "Tstat_lake_1st_Tsetpoint", DATATYPE_double },
    { "H1st", DATATYPE_double },
    { "Tstat_2nd", DATATYPE_double },
    { "Tstat_lake_2nd_Tsetpoint", DATATYPE_double },
    { "H2nd", DATATYPE_double },
    { "Tcondo", DATATYPE_double },
    { "Tstat_condo_Tsetpoint", DATATYPE_double },
    { "Hcondo", DATATYPE_double },
    // added 4/27/2018
    { "Tsoil", DATATYPE_double },
    { "Tambient", DATATYPE_double },
    { "Hambient", DATATYPE_double },
    { "soilHeat", DATATYPE_boolean },
    { "roomHeat", DATATYPE_boolean },
    { "light", DATATYPE_boolean },
    { "fan", DATATYPE_boolean },
    // added 5/16/2018
    { "GH_Tambient", DATATYPE_double },
    { "GH_Hambient", DATATYPE_double },
     // added 4/11/2019
    { "GH_gnd13_T", DATATYPE_double },
    { "GH_gnd3_T", DATATYPE_double },
    { "GH_pool_T", DATATYPE_double },
    { "GH_panel_T", DATATYPE_double },
    { "GH_inner_T", DATATYPE_double },
    { "GH_out_T", DATATYPE_double },
    { "GH_spare1_T", DATATYPE_double },
    { "GH_spare2_T", DATATYPE_double },
    { "GH_pump", DATATYPE_int },
    { "GH_fan", DATATYPE_boolean },
    { "GH_dehumidifier", DATATYPE_boolean },
    { "GH_control3", DATATYPE_boolean },
    // added 3/6/2021
    { "Tambient2", DATATYPE_double },
    { "Hambient2", DATATYPE_double },
    { "smoke", DATATYPE_boolean },
};

#define MERGEINDEX_TOUT        (3)
#define MERGEINDEX_H1ST        (22)
#define MERGEINDEX_H2ND        (25)
#define MERGEINDEX_H1STorH2ND  (10000)

#define N_MERGE (sizeof(merge)/sizeof(MERGE_t))


//
// Define the sample names to be written to the output file.
//

typedef struct {
  char name[SAMPLENAME_SIZE];   // Lake house sample names
  DATATYPE_t type;
  char printFormat[16];
  int mergeIndex;
  int valuePresent;
  double value;  
} OUTPUT_t;

OUTPUT_t output_summary4up[] =
  {
    { "T1st", DATATYPE_double, "%.1lf" },
    { "T2nd", DATATYPE_double, "%.1lf" },
    { "Tbas", DATATYPE_double, "%.1lf" },
    { "Tout", DATATYPE_double, "%.1lf" },
    { "Tcoll", DATATYPE_double, "%.1lf" },
    { "Tstor", DATATYPE_double, "%.1lf" },
    { "TXchI", DATATYPE_double, "%.1lf" },
    { "TXchM", DATATYPE_double, "%.1lf" },
    { "TXchO", DATATYPE_double, "%.1lf" },
    { "THotO", DATATYPE_double, "%.1lf" },
    { "TRadR", DATATYPE_double, "%.1lf" },
    { "THtbR", DATATYPE_double, "%.1lf" },
    { "H1st", DATATYPE_boolean, "%d" },
    { "H2nd", DATATYPE_boolean, "%d" },
    { "circSolar", DATATYPE_boolean, "%d" },
    { "circRadiant", DATATYPE_boolean, "%d" },
    { "circXchg", DATATYPE_boolean, "%d" },
    { "finnedDump", DATATYPE_boolean, "%d" },
    { "PVacPower", DATATYPE_double, "%.0lf" },
    { "PVacEnergy", DATATYPE_double, "%.1lf" },
    { "HeatFraction", DATATYPE_double, "%.2lf" },
    { "Troot", DATATYPE_double, "%.1lf" },
    { "Tcondo", DATATYPE_double, "%.1lf" },
    { "Hcondo", DATATYPE_boolean, "%d" },
  };

#define OUTPUT_SUMMARY4UP_PVacEnergy        (19)
#define OUTPUT_SUMMARY4UP_HeatFraction      (20)

OUTPUT_t output_climate[] =
  {
    { "Tavg", DATATYPE_double, "%.1lf" },
    { "Tmin", DATATYPE_double, "%.1lf" },
    { "Tmax", DATATYPE_double, "%.1lf" },
  };

#define OUTPUT_CLIMATE_Tavg (0)
#define OUTPUT_CLIMATE_Tmin (1)
#define OUTPUT_CLIMATE_Tmax (2)

OUTPUT_t output_growroom[] =
  {
    { "Tsoil", DATATYPE_double, "%.1lf" },
    { "Tambient", DATATYPE_double, "%.1lf" },
    { "Hambient", DATATYPE_double, "%.1lf" },
    { "soilHeat", DATATYPE_boolean, "%d" },
    { "roomHeat", DATATYPE_boolean, "%d" },
    { "light", DATATYPE_boolean, "%d" },
    { "fan", DATATYPE_boolean, "%d" },
    { "Tambient2", DATATYPE_double, "%.1lf" },
    { "Hambient2", DATATYPE_double, "%.1lf" },
    { "smoke", DATATYPE_boolean, "%d" },
  };

OUTPUT_t output_greenhouse[] =
  {
    { "GH_Tambient", DATATYPE_double, "%.1lf" },
    { "GH_Hambient", DATATYPE_double, "%.1lf" },
    { "GH_gnd13_T", DATATYPE_double, "%.1lf" },
    { "GH_gnd3_T", DATATYPE_double, "%.1lf" },
    { "GH_pool_T", DATATYPE_double, "%.1lf" },
    { "GH_panel_T", DATATYPE_double, "%.1lf" },
    { "GH_inner_T", DATATYPE_double, "%.1lf" },
    { "Tout", DATATYPE_double, "%.1lf" },
    { "GH_pump",DATATYPE_int,"%d" },
    { "GH_fan",DATATYPE_int,"%d" },
  };

OUTPUT_t output_rawdata[] =
  {
    { "T1st", DATATYPE_double, "%.1lf" },
    { "T2nd", DATATYPE_double, "%.1lf" },
    { "Tbas", DATATYPE_double, "%.1lf" },
    { "Tout", DATATYPE_double, "%.1lf" },
    { "Tcoll", DATATYPE_double, "%.1lf" },
    { "Tstor", DATATYPE_double, "%.1lf" },
    { "TXchI", DATATYPE_double, "%.1lf" },
    { "TXchM", DATATYPE_double, "%.1lf" },
    { "TXchO", DATATYPE_double, "%.1lf" },
    { "THotO", DATATYPE_double, "%.1lf" },
    { "TRadR", DATATYPE_double, "%.1lf" },
    { "THtbR", DATATYPE_double, "%.1lf" },
    { "H1st", DATATYPE_boolean, "%d" },
    { "H2nd", DATATYPE_boolean, "%d" },
    { "circSolar", DATATYPE_boolean, "%d" },
    { "circRadiant", DATATYPE_boolean, "%d" },
    { "circXchg", DATATYPE_boolean, "%d" },
    { "finnedDump", DATATYPE_boolean, "%d" },
    { "PVacPower", DATATYPE_double, "%.0lf" },
    { "PVacEnergy", DATATYPE_double, "%.1lf" },
    { "HeatFraction", DATATYPE_double, "%.2lf" },
    { "Troot", DATATYPE_double, "%.1lf" },
    { "Tcondo", DATATYPE_double, "%.1lf" },
    { "Hcondo", DATATYPE_boolean, "%d" },
  };

typedef enum {
  OUTPUT_TYPE_summary4up,
  OUTPUT_TYPE_climate,
  OUTPUT_TYPE_growroom,
  OUTPUT_TYPE_greenhouse,
  OUTPUT_TYPE_rawdata,
} OUTPUT_TYPE_t;

typedef struct {
  OUTPUT_TYPE_t type;
  int nOutput;
  OUTPUT_t *output;  
  CALENDAR_TIME_t time;
} OUTPUT_SELECT_t;

int iso8601_datetime_set(char *s, CALENDAR_t *calendar)
{
  *s = 0;
  sprintf(s,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ",
	  calendar->year,
	  calendar->month,
	  calendar->mday,
	  calendar->hour,
	  calendar->minute,
	  calendar->second);
  return DTW_SUCCESS;
}

int iso8601_datetime_get(char *s, CALENDAR_t *calendar)
{
  if (sscanf(s,"%d-%d-%dT%d:%d:%dZ",
	     &calendar->year,
	     &calendar->month,
	     &calendar->mday,
	     &calendar->hour,
	     &calendar->minute,
	     &calendar->second)!=6)
    return DTW_ERROR;

  return DTW_SUCCESS;
}

#define RESULT_CHK(a) if (a!=DTW_SUCCESS) return a

int init(OUTPUT_SELECT_t *output_select)
{
  int i,j;

  // init output to record mapping
  for(i=0;i<output_select->nOutput;i++) {
    output_select->output[i].mergeIndex = -1;
    for(j=0;j<N_MERGE;j++) {
      if (!strcmp(output_select->output[i].name,merge[j].name)) {
	output_select->output[i].mergeIndex = j;
	break;
      }
    }
  }

  return DTW_SUCCESS;
}

typedef struct {
  FILE *fp;
  CALENDAR_t calendarNow;
  CALENDAR_t calendarThru;
  char prefix[128], suffix[16];
} FILESET_t;

int fileset_fopen(FILESET_t *fs, char *prefix, char *suffix, CALENDAR_TIME_t start, CALENDAR_TIME_t thru)
{
  if (!fs)
    return DTW_ERROR;

  CALENDAR_from_time_convert(&fs->calendarNow,start);
  CALENDAR_from_time_convert(&fs->calendarThru,thru);

  strcpy(fs->prefix,prefix);
  strcpy(fs->suffix,suffix);

  fs->fp=NULL;

  return DTW_SUCCESS;
}

int fileset_fgets(char *s,int max,FILESET_t *fs)
{
  char fn[140];
  CALENDAR_TIME_t now,thru;

  if (!fs)
    return DTW_ERROR;

  while(!fs->fp || fgets(s,max,fs->fp)==NULL) {

    // at end of current file ... close it
    if (fs->fp)
      fclose(fs->fp);

    // check to see if we've passed our end datetime ... if so, then EOF
    CALENDAR_to_time_convert(&fs->calendarNow,&now);
    CALENDAR_to_time_convert(&fs->calendarThru,&thru);
    if (now > thru)
      return DTW_EOF;

    // construct next file name and attempt to open file
    sprintf(fn,"%s%.4d-%.2d-%.2d%s",
	    fs->prefix,
	    fs->calendarNow.year,
	    fs->calendarNow.month,
	    fs->calendarNow.mday,
	    fs->suffix);
    fprintf(stderr,"opening file %s\n",fn);
    fs->fp = fopen(fn,"r");

    // advance time to next file
    fs->calendarNow.second = 0;
    fs->calendarNow.minute = 0;
    fs->calendarNow.hour = 0;
    CALENDAR_advanceDays(&fs->calendarNow,1);

  }

  return DTW_SUCCESS;
}

int fileset_fclose(FILESET_t *fs)
{
  if (!fs)
    return DTW_ERROR;

  if (fs->fp)
    fclose(fs->fp);

  fs->fp=NULL;
  
  return DTW_SUCCESS;
}


typedef struct {
  CALENDAR_TIME_t time;
  int valuePresent[N_MERGE];
  double value[N_MERGE];
} RECORD_t;


int get_record(FILESET_t *fs, RECORD_t *record)
{
  int result, i, j;
  char s[256], *p;
  CALENDAR_t calendar;

  while (1) {

    result = fileset_fgets(s,sizeof(s)-1,fs);
    RESULT_CHK(result);

    p = strtok(s,",");
    if (!p || iso8601_datetime_get(s,&calendar)!=DTW_SUCCESS)
      return DTW_BAD_SAMPLE_DATASTREAM;
    CALENDAR_to_time_convert(&calendar,&record->time);

    for(i=0;i<N_MERGE;i++) {

      p = strtok(NULL, ",");
      if (!p) { 
	record->valuePresent[i] = 0;
      } else if (*p == 'x') {
	record->valuePresent[i] = 0;
      } else {
	record->valuePresent[i] = 1;
	switch(merge[i].type) {
	case DATATYPE_double:
	  if (sscanf(p,"%lf",&record->value[i])!=1)
	    return DTW_BAD_SAMPLE_DATASTREAM;
	  break;
	case DATATYPE_int:
	case DATATYPE_boolean:
	  if (sscanf(p,"%d",&j)!=1)
	    return DTW_BAD_SAMPLE_DATASTREAM;
	  record->value[i] = (double) j;
	  break;	  
	}
      }
      
    }

    break;

  }

  return DTW_SUCCESS;
}

typedef struct {
  FILESET_t fs;
  CALENDAR_TIME_t startTime, thruTime, centerTime;
  int incSeconds;
  RECORD_t *records;
  int nRecords;
  int fillPtr,drainPtr,centerPtr;
  int fsEof;
} RING_t;


int ring_init(RING_t *ring, CALENDAR_TIME_t startTime, CALENDAR_TIME_t thruTime, int incSeconds)
{
  int result;

  ring->startTime = startTime;
  ring->thruTime = thruTime;

  ring->centerTime = startTime;
  ring->incSeconds = incSeconds;

  result = fileset_fopen
    (&ring->fs,
#ifdef _WIN32
     "C:\\Users\\derek\\Documents\\projects\\udooLakeHouse\\solarLogs\\data_",    // prefix
#else
     "../solarLogs/data_",               // prefix
#endif     
     ".txt",                             // suffix
     startTime - ring->incSeconds,       // start datetime
     thruTime  + ring->incSeconds  );    // thru datetime
  if (result!=DTW_SUCCESS) {
    fprintf(stderr,"Couldn't open output file.\n");
    return DTW_ERROR;
  }

  // assume worst case 1 record every 50 seconds, plus some for overhead
  ring->nRecords = (2*ring->incSeconds) / 50 + 3;
    
  ring->records = (RECORD_t *) malloc(ring->nRecords * sizeof(RECORD_t));
  if (!ring->records) {
    return DTW_ERROR;
  }

  ring->fillPtr = ring->drainPtr = ring->centerPtr = 0;
  ring->fsEof = 0;

  return DTW_SUCCESS;
}

int ring_fill(RING_t *ring)
{
  if (ring->fsEof) 
    return DTW_SUCCESS;

  while (ring->fillPtr > ring->drainPtr &&
	 ring->records[ring->drainPtr % ring->nRecords].time < (ring->centerTime - ring->incSeconds)) {
    ring->drainPtr++;
    if (ring->centerPtr < ring->drainPtr) 
      ring->centerPtr = ring->drainPtr;
  }

  while ( (ring->fillPtr - ring->drainPtr) < ring->nRecords ) {
    if (get_record(&ring->fs,&ring->records[ring->fillPtr % ring->nRecords])==DTW_SUCCESS ) {
      if (ring->records[ring->fillPtr % ring->nRecords].time <= (ring->thruTime + ring->incSeconds)) {
	ring->fillPtr++;
      } else {
	ring->fsEof = 1;
	return DTW_SUCCESS;
      }
    } else {
      ring->fsEof = 1;
      return DTW_SUCCESS;
    }    
  }

  return DTW_SUCCESS;
}

int ring_center_update(RING_t *ring)
{
  ring_fill(ring);
  while (ring->fillPtr > (ring->centerPtr+1) &&
	 abs(ring->records[(ring->centerPtr+1) % ring->nRecords].time - ring->centerTime) <
	 abs(ring->records[(ring->centerPtr+0) % ring->nRecords].time - ring->centerTime) ) {
    ring->centerPtr++;
    ring_fill(ring);
  }

  return DTW_SUCCESS;
}

typedef struct {
  double min, avg, max;
  double knownFraction;
} TIMEPERIOD_INFO_t;


#define SAMPLE_VALID_SECS 120

int ring_get_timeperiod_info
( RING_t *ring, 
  int mergeIndex, 
  TIMEPERIOD_INFO_t *info)
{
  RECORD_t *previous, *current, *next;
  int index = ring->drainPtr;
  double leftTime, rightTime;
  int minMaxUnknown=1;
  double currentValue;
  int currentValuePresent;

  CALENDAR_TIME_t startTime = ring->centerTime - ring->incSeconds/2;
  CALENDAR_TIME_t thruTime = ring->centerTime + (ring->incSeconds - ring->incSeconds/2);

  info->min = 0.0;
  info->max = 0.0;
  info->avg = 0.0;
  info->knownFraction = 0.0;

  previous = NULL;
  if (index < ring->fillPtr) {
    current = &ring->records[index % ring->nRecords];
  } else {
    current = NULL;
  }

  for (; current ; previous = current, current = next, index++) {

    if ((index + 1) < ring->fillPtr) {
      next = &ring->records[(index + 1) % ring->nRecords];
      if (next->time < startTime)
	continue;
    } else {
      next = NULL;
    }

    if (previous && previous->time >= thruTime)
      break;

    if (mergeIndex != MERGEINDEX_H1STorH2ND) {
      currentValue = current->value[mergeIndex];
      currentValuePresent = current->valuePresent[mergeIndex];
    } else {
      currentValue = 0.0;
      currentValuePresent = 0;
      if (current->valuePresent[MERGEINDEX_H1ST]) {
	if (current->value[MERGEINDEX_H1ST]==1.0) currentValue = 1.0;
	currentValuePresent = 1;
      }
      if (current->valuePresent[MERGEINDEX_H2ND]) {
	if (current->value[MERGEINDEX_H2ND]==1.0) currentValue = 1.0;
	currentValuePresent = 1;
      }
    }

    if (!currentValuePresent)
      continue;

    leftTime = (double)current->time - (double)(previous ? min((current->time - previous->time),SAMPLE_VALID_SECS) : SAMPLE_VALID_SECS)/2.0;
    leftTime = max((double)startTime,leftTime);
    leftTime = (double)current->time - leftTime;
    leftTime = max(0.0,leftTime);

    rightTime = (double)current->time + (double)(next ? min((next->time - current->time),SAMPLE_VALID_SECS) : SAMPLE_VALID_SECS)/2.0;
    rightTime = min((double)thruTime,rightTime);
    rightTime = rightTime - (double)current->time;
    rightTime = max(0.0,rightTime);
    
    if (leftTime!=0.0 || rightTime!=0.0) {
      info->knownFraction += leftTime + rightTime;
      info->avg += currentValue * (leftTime + rightTime);
      info->min = minMaxUnknown ? currentValue : min(info->min,currentValue);
      info->max = minMaxUnknown ? currentValue : max(info->max,currentValue);
      minMaxUnknown = 0;
    }

  }

  if (info->knownFraction==0)
    info->avg = 0;
  else
    info->avg /= info->knownFraction;
  info->knownFraction /= (double) (thruTime - startTime);

  return DTW_SUCCESS;
}



int ring_get_output_record(RING_t *ring, OUTPUT_SELECT_t *output_select)
{
  int result = DTW_SUCCESS;
  RECORD_t *record;
  int i,j;
  TIMEPERIOD_INFO_t info;
  double T0, T1;

  if (ring->centerTime > ring->thruTime)
    return DTW_EOF;

  ring_center_update(ring);

  if (ring->fillPtr <= ring->centerPtr) {
    result = DTW_NODATA;
  }

  record = &ring->records[ring->centerPtr % ring->nRecords];

  if (abs(record->time - ring->centerTime) > 2*ring->incSeconds) {
    result = DTW_NODATA;
  }

  if (result != DTW_NODATA) {

    output_select->time = ring->centerTime;

#if 1
    // simply copy from merge record
    for(i=0;i<output_select->nOutput;i++) {
      j = output_select->output[i].mergeIndex;
      if (j!=-1) {
	output_select->output[i].valuePresent = record->valuePresent[j];
	output_select->output[i].value = record->value[j];
      }
    }
#else
    // average and copy from merge record
    for(i=0;i<output_select->nOutput;i++) {
      j = output_select->output[i].mergeIndex;
      if (j!=-1) {
	ring_get_timeperiod_info
	  ( ring,
	    j,                                                           // mergeIndex
	    &info);
	if (info.knownFraction > 0.1) {
	  output_select->output[i].valuePresent = 1;
	}
	output_select->output[i].value = info.avg;
      }
    }
#endif

    switch(output_select->type) {

    case OUTPUT_TYPE_summary4up:
      ring_get_timeperiod_info
	( ring,
	  MERGEINDEX_H1STorH2ND,                                       // mergeIndex
	  &info);
      if (info.knownFraction > 0.1) {
	output_select->output[OUTPUT_SUMMARY4UP_HeatFraction].valuePresent = 1;
      }
      output_select->output[OUTPUT_SUMMARY4UP_HeatFraction].value = info.avg;

      break;

    case OUTPUT_TYPE_climate:
      // temperature info
      ring_get_timeperiod_info
	( ring,
	  MERGEINDEX_TOUT,
	  &info);
      if (info.knownFraction > 0.1) {
	output_select->output[OUTPUT_CLIMATE_Tavg].valuePresent = 1;
	output_select->output[OUTPUT_CLIMATE_Tmin].valuePresent = 1;
	output_select->output[OUTPUT_CLIMATE_Tmax].valuePresent = 1;      
      }
      output_select->output[OUTPUT_CLIMATE_Tavg].value = info.avg;
      output_select->output[OUTPUT_CLIMATE_Tmin].value = info.min;
      output_select->output[OUTPUT_CLIMATE_Tmax].value = info.max;      
      break;

    default:
      break;

    }    

  }

  ring->centerTime += ring->incSeconds;

  return result;
}

int ring_close(RING_t *ring)
{
  free(ring->records);
  return fileset_fclose(&ring->fs);
}


int output_record_gnuplot(FILE *fp, OUTPUT_SELECT_t *output_select)
{
  char s[256];
  CALENDAR_t calendar;
  static int count=0;
  int i,k;
  CALENDAR_TIME_t easternTime;

  if(count==0) {
    fprintf(fp,"datetime");    
    for(i=0;i<output_select->nOutput;i++) {
      fprintf(fp," ");
      fprintf(fp,"%s",output_select->output[i].name);
    }    
    fprintf(fp,"\n");
    count = 9999999;
  } else {
    count--;
  }

  easternTime = output_select->time - 5*60*60;
  CALENDAR_from_time_convert(&calendar,easternTime);
  iso8601_datetime_set(s,&calendar);
  fprintf(fp,"%s",s);
  for(i=0;i<output_select->nOutput;i++) {
    fprintf(fp," ");
    if (output_select->output[i].valuePresent) {
      switch(output_select->output[i].type) {
      case DATATYPE_double:
	fprintf(fp,output_select->output[i].printFormat,output_select->output[i].value);
	break;
      case DATATYPE_int:
      case DATATYPE_boolean:
	k = (int) output_select->output[i].value;
	fprintf(fp,output_select->output[i].printFormat,k);
	break;
      }
    } else {
      fprintf(fp,"NaN");
    }
  }
  fprintf(fp,"\n");

  return DTW_SUCCESS;
}


int pvEnergy1stValid = 0;
double pvEnergy1st = 0.0, pvEnergyLast = 0.0;
CALENDAR_t startCalendar, endCalendar;
int interval_days = 2;
double propaneGallons = 0.0;

int plt_script_create(OUTPUT_SELECT_t *output_select)
{
  FILE *fpPltIn,*fpPltOut;
  char s[256],*ss,stime[32];

  switch(output_select->type) {
  case OUTPUT_TYPE_summary4up:
    fpPltIn = fopen("summary4up.plt","r");
    break;
  case OUTPUT_TYPE_climate:
    fpPltIn = fopen("climate.plt","r");
    break;
  case OUTPUT_TYPE_growroom:
    fpPltIn = fopen("growroom.plt","r");
    break;
  case OUTPUT_TYPE_greenhouse:
    fpPltIn = fopen("greenhouse.plt","r");
    break;
  default:
    fpPltIn = NULL;
    break;
  }    
  fpPltOut = fopen("out.plt","w");
  if (!fpPltOut) {
    fprintf(stderr,"Couldn't open output file out.plt.\n");
    exit(-1);
  }

  while((ss=fgets(s,sizeof(s)-1,fpPltIn))!=NULL) {
    while(*ss) {
      if (!strncmp("#datetime_start",ss,strlen("#datetime_start"))) {
	iso8601_datetime_set(stime,&startCalendar);
	fprintf(fpPltOut,"%s",stime);
	ss+=strlen("#datetime_start");		    
      } else if (!strncmp("#datetime_end",ss,strlen("#datetime_end"))) {
	iso8601_datetime_set(stime,&endCalendar);
	fprintf(fpPltOut,"%s",stime);
	ss+=strlen("#datetime_end");		    
      } else if (!strncmp("#date",ss,strlen("#date"))) {
	fprintf(fpPltOut,"%d/%d/%d",startCalendar.month,startCalendar.mday,startCalendar.year);
	ss+=strlen("#date");		    
      } else if (!strncmp("#pvEnergySummary",ss,strlen("#pvEnergySummary"))) {
	fprintf(fpPltOut,"Total %4.1lf KWhour (avg %4.1lf per day)",
		pvEnergyLast-pvEnergy1st,(pvEnergyLast-pvEnergy1st)/interval_days);
	ss+=strlen("#pvEnergySummary");		    
      } else if (!strncmp("#pvEnergy",ss,strlen("#pvEnergy"))) {
	fprintf(fpPltOut,"%4.1lf",pvEnergyLast-pvEnergy1st);
	ss+=strlen("#pvEnergy");		    
      } else if (!strncmp("#propaneGallons",ss,strlen("#propaneGallons"))) {
	fprintf(fpPltOut,"%4.1lf",propaneGallons);
	ss+=strlen("#propaneGallons");		    
      } else if (!strncmp("#propaneSummary",ss,strlen("#propaneSummary"))) {
	fprintf(fpPltOut,"Total propane gallons %4.1lf (avg %4.1f per day)",
		propaneGallons,propaneGallons/interval_days);
	ss+=strlen("#propaneSummary");		    
      } else {
	fputc(*ss++,fpPltOut);
      }
    }
  }      
  

  fclose(fpPltIn);
  fclose(fpPltOut);

  return DTW_SUCCESS;
}

void usage(char *s)
{
  fprintf(stderr,"%s [MM/DD/YYYY] [-dN]\n",s);
}

int main(int argc, char *argv[])
{
  FILE *fpo;
  RING_t ring;
  OUTPUT_SELECT_t output_select;
  int result,i;
  CALENDAR_TIME_t startTime, thruTime;
  CALENDAR_t calendar;
  int incSeconds;

  {
    time_t ctime = time (NULL);
    struct tm tm_time;
    tm_time = *localtime(&ctime);
    calendar.year = tm_time.tm_year + 1900;
    calendar.month = tm_time.tm_mon + 1;
    calendar.mday = tm_time.tm_mday;
    calendar.hour = tm_time.tm_hour;
    calendar.minute = tm_time.tm_min;
    calendar.second = tm_time.tm_sec;
    fprintf(stdout,"current datetime %d/%d/%d %d:%d\n",
	    calendar.month,
	    calendar.mday,
	    calendar.year,
	    calendar.hour,
	    calendar.minute);
  }

  output_select.type = OUTPUT_TYPE_summary4up;
  output_select.nOutput = sizeof(output_summary4up)/sizeof(OUTPUT_t);
  output_select.output = output_summary4up;

#ifndef _WIN32
  // udoo machine runs on UTC
  CALENDAR_advanceHours(&calendar,-5);
#endif
  calendar.hour = 0;
  calendar.minute = 0;
  calendar.second = 0;
  CALENDAR_advanceDays(&calendar,-1);
  
  for(i=1;i<argc;i++) {
    if (!strncmp(argv[i],"-d",2)) {
      sscanf(argv[i],"-d%d",&interval_days);
    }
    if (!strcmp(argv[i],"-summary")) {
      output_select.type = OUTPUT_TYPE_summary4up;
      output_select.nOutput = sizeof(output_summary4up)/sizeof(OUTPUT_t);
      output_select.output = output_summary4up;
    }
    if (!strcmp(argv[i],"-climate")) {
      output_select.type = OUTPUT_TYPE_climate;
      output_select.nOutput = sizeof(output_climate)/sizeof(OUTPUT_t);
      output_select.output = output_climate;
    }
    if (!strcmp(argv[i],"-growroom")) {
      output_select.type = OUTPUT_TYPE_growroom;
      output_select.nOutput = sizeof(output_growroom)/sizeof(OUTPUT_t);
      output_select.output = output_growroom;
    }
    if (!strcmp(argv[i],"-greenhouse")) {
      output_select.type = OUTPUT_TYPE_greenhouse;
      output_select.nOutput = sizeof(output_greenhouse)/sizeof(OUTPUT_t);
      output_select.output = output_greenhouse;
    }
    if (!strcmp(argv[i],"-rawdata")) {
      output_select.type = OUTPUT_TYPE_rawdata;
      output_select.nOutput = sizeof(output_rawdata)/sizeof(OUTPUT_t);
      output_select.output = output_rawdata;
    }
    if (*argv[i]!='-') {
      if (sscanf(argv[1],"%d/%d/%d",&calendar.month,&calendar.mday,&calendar.year)!=3) {
	usage(argv[0]);
	exit(-1);
      }
    }
  }

  fprintf(stdout,"start date %d/%d/%d\n",
	  calendar.month,
	  calendar.mday,
	  calendar.year);
  
  startCalendar = calendar;
  endCalendar = calendar;
  CALENDAR_advanceDays(&endCalendar,interval_days);

  CALENDAR_to_time_convert(&startCalendar,&startTime);
  CALENDAR_to_time_convert(&endCalendar,&thruTime);
  startTime += 5*60*60; // +5HRs to get to EST
  thruTime += 5*60*60; // +5HRs to get to EST

  result = init(&output_select);
  if (result!=DTW_SUCCESS) {
    fprintf(stderr,"failed to init\n");
    exit(-1);
  }

  switch(output_select.type) {
  case OUTPUT_TYPE_summary4up:
  case OUTPUT_TYPE_growroom:
  case OUTPUT_TYPE_greenhouse:
    incSeconds = interval_days * 24 * 60 * 60 / 1000;
    break;
  case OUTPUT_TYPE_climate:
    incSeconds = 24 * 60 * 60;
    break;
  case OUTPUT_TYPE_rawdata:
  default:
    // don't subsample / oversample data, just echo data values and times
    incSeconds = SAMPLE_VALID_SECS;
    break;
  }    
  result = ring_init
    ( &ring,
      startTime,                           // start datetime
      thruTime,                            // thru datetime
      incSeconds );                        // incSeconds
  if (result!=DTW_SUCCESS) {
    fprintf(stderr,"Couldn't init ring buffer.\n");
    exit(-1);
  }
      
  fpo = fopen("out.txt","w");
  if (!fpo) {
    fprintf(stderr,"Couldn't open output file.\n");
    exit(-1);
  }
  
  while(1) {

    result=ring_get_output_record(&ring,&output_select);
    if (result==DTW_NODATA)
      continue;
    if (result!=DTW_SUCCESS)
      break;
    output_record_gnuplot(fpo,&output_select);    
    
    switch(output_select.type) {
    case OUTPUT_TYPE_summary4up:
      if (output_select.output[OUTPUT_SUMMARY4UP_PVacEnergy].valuePresent) {
	if (!pvEnergy1stValid) {
	  pvEnergy1st = output_select.output[OUTPUT_SUMMARY4UP_PVacEnergy].value;
	  pvEnergy1stValid = 1;
	}
	pvEnergyLast = output_select.output[OUTPUT_SUMMARY4UP_PVacEnergy].value;	  
      }
      if (output_select.output[OUTPUT_SUMMARY4UP_HeatFraction].valuePresent) {
	propaneGallons += 
	  output_select.output[OUTPUT_SUMMARY4UP_HeatFraction].value /* onFraction */
	  * ((double)ring.incSeconds) / 60.0 / 60.0 /* hours per slice */
	  * 120000.0 /* 120kBTU / hour */
	  / 91700.0; /* 91.7kBTU / gallon */
      }
      break;
    case OUTPUT_TYPE_climate:
      break;
    case OUTPUT_TYPE_growroom:
      break;
    case OUTPUT_TYPE_greenhouse:
      break;
    default:
      break;
    }    

  }


  if (result!=DTW_EOF) {
    fprintf(stderr,"extract failed with error %d\n",result);
    exit(-1);
  }

  switch(output_select.type) {
  case OUTPUT_TYPE_rawdata:
    break;
  default:
    plt_script_create(&output_select);
    break;
  }

  ring_close(&ring);
  fclose(fpo);
}


