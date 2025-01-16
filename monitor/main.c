#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "radiant.h"
#include "honeywell.h"
#include "pv.h"
#include "thingspeak.h"
#include "growroom.h"
#include "greenhouse.h"

#define ENABLE_PV 1
#define ENABLE_RADIANT 1
#define ENABLE_GROWROOM 0
#define ENABLE_GROWROOM_EXT 0
#define ENABLE_GREENHOUSE 0
#define ENABLE_HONEYWELL 1
#define ENABLE_THINGSPEAK 0


/*

This is the program that I am running on the UDOO board to log PV and thermal solar
data at the Lakehouse.  To get it run run automatically after boot add it to 
/etc/rc.local as shown below.


/etc/rc.local:

#!/bin/sh -e
#
# rc.local
#
# This script is executed at the end of each multiuser runlevel.
# Make sure that the script will "exit 0" on success or any other
# value on error.
#
# In order to enable or disable this script just change the execution
# bits.
#
# By default this script does nothing.

/home/udooer/solarMonitor/monitor

exit 0

*/

typedef enum {
  DATATYPE_double,
  DATATYPE_int,
  DATATYPE_boolean
} DATATYPE_t;

struct {
  char name[32];
  DATATYPE_t type;
  char printFormat[16];
  int valuePresent;
  double value;
} samples[] =
  {
    { "T1st", DATATYPE_double, "%.1lf", 0 },
    { "T2nd", DATATYPE_double, "%.1lf", 0 },
    { "Tbas", DATATYPE_double, "%.1lf", 0 },
    { "Tout", DATATYPE_double, "%.1lf", 0 },
    { "Tcoll", DATATYPE_double, "%.1lf", 0 },
    { "Tstor", DATATYPE_double, "%.1lf" },
    { "TXchI", DATATYPE_double, "%.1lf", 0 },
    { "TXchM", DATATYPE_double, "%.1lf", 0 },
    { "TXchO", DATATYPE_double, "%.1lf", 0 },
    { "THotO", DATATYPE_double, "%.1lf", 0 },
    { "TRadR", DATATYPE_double, "%.1lf", 0 },
    { "THTbR", DATATYPE_double, "%.1lf", 0 },
    { "circSolar", DATATYPE_boolean, "%d", 0 },
    { "circRadiant", DATATYPE_boolean, "%d", 0 },
    { "circHTub", DATATYPE_boolean, "%d", 0 },
    { "finnedDump", DATATYPE_boolean, "%d", 0 },
    { "PVacPower", DATATYPE_double, "%.0lf", 0 },
    { "PVacEnergy", DATATYPE_double, "%.1lf", 0 },
    { "Thottub", DATATYPE_double, "%.1lf", 0 },
    { "Troot", DATATYPE_double, "%.1lf", 0 },
    { "Tstat_lake_1st_T", DATATYPE_double, "%.1lf", 0 },
    { "Tstat_lake_1st_Tsetpoint", DATATYPE_double, "%.1lf", 0 },
    { "H1st", DATATYPE_double, "%.1lf", 0 },
    { "Tstat_lake_2nd_T", DATATYPE_double, "%.1lf", 0 },
    { "Tstat_lake_2nd_Tsetpoint", DATATYPE_double, "%.1lf", 0 },
    { "H2nd", DATATYPE_double, "%.1lf", 0 },
#if ENABLE_GROWROOM
    // seedling grow room
    { "grow_soil_T", DATATYPE_double, "%.1lf", 0 },
    { "grow_ambient_T", DATATYPE_double, "%.1lf", 0 },
    { "grow_ambient_humidity", DATATYPE_double, "%.1lf", 0 },
    { "grow_soil_heat", DATATYPE_boolean, "%d", 0 },
    { "grow_room_heat", DATATYPE_boolean, "%d", 0 },
    { "grow_light", DATATYPE_boolean, "%d", 0 },
    { "grow_fan", DATATYPE_boolean, "%d", 0 },
#endif
#if ENABLE_GREENHOUSE
    // greenhouse
    { "greenhouse_ambient_T", DATATYPE_double, "%.1lf", 0 },
    { "greenhouse_ambient_humidity", DATATYPE_double, "%.1lf", 0 },
    { "greenhouse_gnd13_T", DATATYPE_double, "%.1lf", 0 },
    { "greenhouse_gnd3_T", DATATYPE_double, "%.1lf", 0 },
    { "greenhouse_pool_T", DATATYPE_double, "%.1lf", 0 },
    { "greenhouse_panel_T", DATATYPE_double, "%.1lf", 0 },
    { "greenhouse_inner_T", DATATYPE_double, "%.1lf", 0 },
    { "greenhouse_out_T", DATATYPE_double, "%.1lf", 0 },
    { "greenhouse_spare1_T", DATATYPE_double, "%.1lf", 0 },
    { "greenhouse_spare2_T", DATATYPE_double, "%.1lf", 0 },
    { "greenhouse_pump", DATATYPE_int, "%d", 0 },
    { "greenhouse_fan", DATATYPE_boolean, "%d", 0 },
    { "greenhouse_dehumidifer", DATATYPE_boolean, "%d", 0 },
    { "greenhouse_control3", DATATYPE_boolean, "%d", 0 },
#endif
#if ENABLE_GROWROOM_EXT
    // seedling grow room extended
    { "grow_ambient_T2", DATATYPE_double, "%.1lf", 0 },
    { "grow_ambient_humidity2", DATATYPE_double, "%.1lf", 0 },
    { "grow_smoke", DATATYPE_boolean, "%d", 0 },
#endif
  };


typedef struct {
  FILE *fp;
  int year, mon, mday;
  char prefix[128];
  char suffix[128];
} LOG_FILESET_t;


int iso8601_datetime_set(char *s, struct tm *time)
{
  *s = 0;
  sprintf(s,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ",
	  time->tm_year+1900,
	  time->tm_mon+1,
	  time->tm_mday,
	  time->tm_hour,
	  time->tm_min,
	  time->tm_sec);
  return 0;
}

int log_fileset_fputs(LOG_FILESET_t *fs, time_t *rawtime, char *ss)
{
  char s[256];
  struct tm *tm_time;
  tm_time = gmtime(rawtime);

  if (!fs->fp ||
      tm_time->tm_year != fs->year ||
      tm_time->tm_mon != fs->mon ||
      tm_time->tm_mday != fs->mday) {

    if (fs->fp)
      fclose(fs->fp);

    fs->year = tm_time->tm_year;
    fs->mon = tm_time->tm_mon;
    fs->mday = tm_time->tm_mday;

    sprintf(s,"%s%.4d-%.2d-%.2d%s",fs->prefix,fs->year+1900,fs->mon+1,fs->mday,fs->suffix);
    fs->fp = fopen(s,"a");
  }

  iso8601_datetime_set(s, tm_time);
  fprintf(fs->fp,"%s%s",s,ss);
  fflush(fs->fp);

  printf("l: %s%s",s,ss);
}

int log_fileset_fopen(LOG_FILESET_t *fs, char *prefix, char *suffix)
{
  if (!fs)
    return 1;

  strcpy(fs->prefix,prefix);
  strcpy(fs->suffix,suffix);

  fs->fp=NULL;

  return 0;
}

int put_record(LOG_FILESET_t *fs)
{
  char s[512];
  time_t rawtime;
  struct tm *tm_time;
  int i,j;

#if 0
  static int count=0;
  if(count==0) {
    fprintf(fp,"//datetime");    
    for(i=0;i<sizeof(samples)/sizeof(samples[0]);i++) {
      fprintf(fp,",");
      fprintf(fp,"%s",samples[i].name);
    }    
    fprintf(fp,"\n");
    count = 100;
  } else {
    count--;
  }
#endif

  s[0]=0;
  for(i=0;i<sizeof(samples)/sizeof(samples[0]);i++) {
    sprintf(s+strlen(s),",");
    if (samples[i].valuePresent) {
      switch(samples[i].type) {
      case DATATYPE_double:
	sprintf(s+strlen(s),samples[i].printFormat,samples[i].value);
	break;
      case DATATYPE_int:
      case DATATYPE_boolean:
	j = (int) samples[i].value;
	sprintf(s+strlen(s),samples[i].printFormat,j);
	break;
      }
    } else {
      sprintf(s+strlen(s),"x");
    }
  } 
  sprintf(s+strlen(s),"\n");

  time(&rawtime);
  log_fileset_fputs(fs,&rawtime,s);

  return 0;
}

void recordValue(char *s, double v, int valuePresent)
{
  int i;
  for(i=0;i<sizeof(samples)/sizeof(samples[0]);i++) {
    if (!strcmp(s,samples[i].name)) {
      samples[i].valuePresent = valuePresent;
      samples[i].value = v;
      return;
    }
  }
  printf("no match for %s\n",s);
}

void fetchValue(char *s, double *v, int *valuePresent)
{
  int i;
  *v = 0;
  *valuePresent = 0;
  for(i=0;i<sizeof(samples)/sizeof(samples[0]);i++) {
    if (!strcmp(s,samples[i].name)) {
      *valuePresent = samples[i].valuePresent;
      *v = samples[i].value;
      return;
    }
  }
  printf("no match for %s\n",s);
}

#if ENABLE_RADIANT
static void get_radiant(void)
{
  RADIANT_t radiant;
  radiant_sample(&radiant);

  recordValue("T1st", radiant.T1st, !radiant_isUnknown(radiant.T1st));
  recordValue("T2nd", radiant.T2nd, !radiant_isUnknown(radiant.T2nd));
  recordValue("Tbas", radiant.Tbas, !radiant_isUnknown(radiant.Tbas));
  recordValue("Tout", radiant.Tout, !radiant_isUnknown(radiant.Tout));
  recordValue("Tcoll", radiant.thermal_collT, !radiant_isUnknown(radiant.thermal_collT));
  recordValue("Tstor", radiant.thermal_storT, !radiant_isUnknown(radiant.thermal_storT));
  recordValue("Thottub", radiant.thermal_aux1T, !radiant_isUnknown(radiant.thermal_aux1T));
  recordValue("TXchI", radiant.TXchI, !radiant_isUnknown(radiant.TXchI));
  recordValue("TXchM", radiant.TXchM, !radiant_isUnknown(radiant.TXchM));
  recordValue("TXchO", radiant.TXchO, !radiant_isUnknown(radiant.TXchO));
  recordValue("THotO", radiant.THotO, !radiant_isUnknown(radiant.THotO));
  recordValue("TRadR", radiant.TRadR, !radiant_isUnknown(radiant.TRadR));
  recordValue("THTbR", radiant.THTbR, !radiant_isUnknown(radiant.THTbR));
  recordValue("H1st", radiant.H1st, !radiant_isUnknown(radiant.H1st));
  recordValue("H2nd", radiant.H2nd, !radiant_isUnknown(radiant.H2nd));
  recordValue("circSolar", radiant.thermal_pump, !radiant_isUnknown(radiant.thermal_pump));
  recordValue("circRadiant", radiant.circulatorRadiant, !radiant_isUnknown(radiant.circulatorRadiant));
  recordValue("circHTub", radiant.circulatorHTub, !radiant_isUnknown(radiant.circulatorHTub));
  recordValue("finnedDump", radiant.finnedDump, !radiant_isUnknown(radiant.finnedDump));
  recordValue("Troot", radiant.Troot, !radiant_isUnknown(radiant.Troot));
}
#endif

#if ENABLE_HONEYWELL
static void get_honeywell(void)
{
  HONEYWELL_t honeywell;
  honeywell_sample(&honeywell);

  recordValue("Tstat_lake_1st_T", honeywell.lake_1st_T, !honeywell_isUnknown(honeywell.lake_1st_T));
  recordValue("Tstat_lake_1st_Tsetpoint", honeywell.lake_1st_Tsetpoint, !honeywell_isUnknown(honeywell.lake_1st_Tsetpoint));
  //recordValue("Tstat_lake_1st_fan", honeywell.lake_1st_fan, !honeywell_isUnknown(honeywell.lake_1st_fan));

  recordValue("Tstat_lake_2nd_T", honeywell.lake_2nd_T, !honeywell_isUnknown(honeywell.lake_2nd_T));
  recordValue("Tstat_lake_2nd_Tsetpoint", honeywell.lake_2nd_Tsetpoint, !honeywell_isUnknown(honeywell.lake_2nd_Tsetpoint));
  //recordValue("Tstat_lake_2nd_fan", honeywell.lake_2nd_fan, !honeywell_isUnknown(honeywell.lake_2nd_fan));
}
#endif

#if ENABLE_PV
static void get_pv(void)
{
  PV_t pv;
  pv_sample(&pv);

  recordValue("PVacPower", pv.acPower, !pv_isUnknown(pv.acPower));
  recordValue("PVacEnergy", pv.acEnergy, !pv_isUnknown(pv.acEnergy));
}
#endif

#if ENABLE_GROWROOM
static void get_growroom(void)
{
  GROWROOM_t growroom;
  growroom_sample(&growroom);

  recordValue("grow_soil_T", growroom.soil_T, !growroom_isUnknown(growroom.soil_T));
  recordValue("grow_ambient_T", growroom.ambient_T, !growroom_isUnknown(growroom.ambient_T));
  recordValue("grow_ambient_humidity", growroom.ambient_humidity, !growroom_isUnknown(growroom.ambient_humidity));
  recordValue("grow_soil_heat", growroom.soil_heat, !growroom_isUnknown(growroom.soil_heat));
  recordValue("grow_room_heat", growroom.room_heat, !growroom_isUnknown(growroom.room_heat));
  recordValue("grow_light", growroom.light, !growroom_isUnknown(growroom.light));
  recordValue("grow_fan", growroom.fan, !growroom_isUnknown(growroom.fan));
#if ENABLE_GROWROOM_EXT
  recordValue("grow_ambient_T2", growroom.ambient_T2, !growroom_isUnknown(growroom.ambient_T2));
  recordValue("grow_ambient_humidity2", growroom.ambient_humidity2, !growroom_isUnknown(growroom.ambient_humidity2));
  recordValue("grow_smoke", growroom.smoke, !growroom_isUnknown(growroom.smoke));
#endif
}
#endif

#if ENABLE_GREENHOUSE
static void get_greenhouse(void)
{
  GREENHOUSE_t greenhouse;
  greenhouse_sample(&greenhouse);

  recordValue("greenhouse_ambient_T", greenhouse.ambient_T, !greenhouse_isUnknown(greenhouse.ambient_T));
  recordValue("greenhouse_ambient_humidity", greenhouse.ambient_humidity, !greenhouse_isUnknown(greenhouse.ambient_humidity));
  recordValue("greenhouse_gnd13_T", greenhouse.gnd13_T, !greenhouse_isUnknown(greenhouse.gnd13_T));
  recordValue("greenhouse_gnd3_T", greenhouse.gnd3_T, !greenhouse_isUnknown(greenhouse.gnd3_T));
  recordValue("greenhouse_pool_T", greenhouse.pool_T, !greenhouse_isUnknown(greenhouse.pool_T));
  recordValue("greenhouse_panel_T", greenhouse.panel_T, !greenhouse_isUnknown(greenhouse.panel_T));
  recordValue("greenhouse_inner_T", greenhouse.inner_T, !greenhouse_isUnknown(greenhouse.inner_T));
  recordValue("greenhouse_out_T", greenhouse.out_T, !greenhouse_isUnknown(greenhouse.out_T));
  recordValue("greenhouse_spare1_T", greenhouse.spare1_T, !greenhouse_isUnknown(greenhouse.spare1_T));
  recordValue("greenhouse_spare2_T", greenhouse.spare2_T, !greenhouse_isUnknown(greenhouse.spare2_T));
  recordValue("greenhouse_pump", greenhouse.pump, !greenhouse_isUnknown(greenhouse.pump));
  recordValue("greenhouse_fan", greenhouse.fan, !greenhouse_isUnknown(greenhouse.fan));
  recordValue("greenhouse_dehumidifier", greenhouse.dehumidifier, !greenhouse_isUnknown(greenhouse.dehumidifier));
  recordValue("greenhouse_control3", greenhouse.control3, !greenhouse_isUnknown(greenhouse.control3));
}
#endif

static void get_ttyUSB ( char *inverter,
			 char *arduino )
{
  FILE *fp;
  char s[128],*ss;

  *inverter = 0;
  *arduino = 0;
  
  fp = popen("./ttyUSBx","r");
  if (fp) {
    while(fgets(s,sizeof(s),fp)) {
      if (strstr(s," - Prolific")) {
	*strstr(s," - Prolific") = 0;
	strcpy(inverter,s);
      }
      if (strstr(s," - FTDI_USB_Serial_Converter_FTE1JHMJ")) {
	*strstr(s," - FTDI_USB_Serial_Converter_FTE1JHMJ") = 0;
	strcpy(inverter,s);
      }
      if (strstr(s," - Gravitech_ARDUINO")) {
	*strstr(s," - Gravitech_ARDUINO") = 0;
	strcpy(arduino,s);
      }
      if (strstr(s," - 1a86_USB2.0-Ser_")) {
	*strstr(s," - 1a86_USB2.0-Ser_") = 0;
	strcpy(arduino,s);
      }
    }
  }

  fclose(fp);
}

// logging interval in minutes
#define INTERVAL 2

main()
{
  int i,err;
  time_t t0,t1;
  struct tm *tm_time;
  LOG_FILESET_t fs;
  char inverter[32];
  char arduino[32];
  
#if 0
  pid_t process_id = 0;
  pid_t sid = 0;

  // Create child process
  process_id = fork();
  // Indication of fork() failure
  if (process_id < 0)
    {
      printf("fork failed!\n");
      // Return failure in exit status
      exit(1);
    }

  // PARENT PROCESS. Need to kill it.
  if (process_id > 0)
    {
      printf("process_id of child process %d \n", process_id);
      // return success in exit status
      exit(0);
    }

  //unmask the file mode
  umask(0);

  //set new session
  sid = setsid();
  if(sid < 0)
    {
      // Return failure
      exit(1);
    }

  // Close stdin. stdout and stderr
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
#endif
  
  // Change the current working directory to root.
  chdir("/home/udooer/SolarMonitor/monitor");

  for(i=0;i<sizeof(samples)/sizeof(samples[0]);i++) {
    samples[i].valuePresent = 0;
  }    

  get_ttyUSB (inverter,arduino);
  
#if ENABLE_RADIANT
  err = radiant_start(arduino);
  if (err) {
    printf("radiant_start() fail\n");
  }
#endif
  
#if ENABLE_HONEYWELL
  err = honeywell_start();
  if (err) {
    printf("honeywell_start() fail\n");
  }
#endif
  
#if ENABLE_PV
  err = pv_start(inverter);
  if (err) {
    printf("pv_start() fail\n");
  }
#endif

#if ENABLE_GROWROOM
  err = growroom_start();
  if (err) {
    printf("growroom_start() fail\n");
  }
#endif
  
#if ENABLE_GREENHOUSE
  err = greenhouse_start();
  if (err) {
    printf("greenhouse_start() fail\n");
  }
#endif
  
  log_fileset_fopen(&fs,"/home/udooer/SolarMonitor/solarLogs/data_",".txt");
  
  while(1) {

    while(1) {
      time(&t1);
      tm_time = gmtime(&t1);
      if (tm_time->tm_sec==0 && (tm_time->tm_min % INTERVAL)==0 && t1!=t0) {
	t0=t1;
	break;
      }
      usleep(10*1000);
    }
    

#if ENABLE_RADIANT
    get_radiant();
#endif
#if ENABLE_HONEYWELL
    get_honeywell();
#endif
#if ENABLE_PV
    get_pv();
#endif
#if ENABLE_GROWROOM
    get_growroom();
#endif
#if ENABLE_GREENHOUSE
    get_greenhouse();
#endif
    
    put_record(&fs);

#if ENABLE_THINKSPEAK
    thingspeak();
#endif
    
  }

  return 0;
}
