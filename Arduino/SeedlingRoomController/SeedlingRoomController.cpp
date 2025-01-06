#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <MCP23017.h>
#include <lcd.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <TimeLib.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>

// Select Arduino->tools->Board
//
// Arduino/Genuino Mega or Mega 2560


// Mode
#define MODE_seed       0
#define MODE_veg_flower 1
#define MODE_veg_dry    2
#define MODE MODE_seed

// options for debug away from actual devices
#define ENABLE_LCD 1
#define ENABLE_DHT 1
#define ENABLE_DHT2 1
#define ENABLE_ONEWIRE 1
#define ENABLE_NET 1
#define ENABLE_SMOKE 1

// LCD strings
char topLine[17] = {0};
char botLine[17] = {0};

#if ENABLE_DHT
#define DHTPIN            8         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);
#endif

#if ENABLE_DHT2
#define DHT2PIN           7         // Pin which is connected to the DHT sensor.
#define DHT2TYPE          DHT22     // DHT 22 (AM2302)
DHT_Unified dht2(DHT2PIN, DHT2TYPE);
#endif

#if ENABLE_LCD
MCP23017 mcp23017;
LCD lcd(&mcp23017,1);
#endif

#if ENABLE_ONEWIRE
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(9); // pin -OR- pin_I, pin_O, pin_OEn

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress soilThermometer;
#endif

// define pins used to control outlet relays
#define PIN_OUTLET1 35
#define PIN_OUTLET2 33
#define PIN_OUTLET3 31
#define PIN_OUTLET4 29

#define LIGHT_HOURS_V 18
#define LIGHT_HOURS_F 12
#define LIGHT_HOURS_R 12
float temperatureSoil;
float temperatureAmbient;
float humidityAmbient;
float temperatureAmbient2;
float humidityAmbient2;
unsigned heatSoil;
unsigned heatRoom;
unsigned growLightShelves;
unsigned growLightCOB;
unsigned logCount = 0;

// define pin used for analog detection of smoke alarm, and # of detections to trigger shutdown
#define PIN_SMOKE 2
#define SMOKE_TRIGGER 5

#define DBGPRINT_ONEWIRE 1
#define DBGPRINT_DHT 0
#define DBGPRINT_DHT2 0
#define DBGPRINT_NET 0
#define DBGPRINT_SMOKE 0

#define BOOT_WAIT_SEC 10 //(5*60)

void setup_oneWire()
{
#if ENABLE_ONEWIRE
  // locate devices on the bus
  unsigned n;
  delay(1000);
  Serial.print("Locating devices...");
  sensors.begin();
  n = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(n, DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  if (!sensors.getAddress(soilThermometer, 0)) 
    Serial.println("Unable to find address for Device 0"); 
  
  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    if (soilThermometer[i] < 16) Serial.print("0");
    Serial.print(soilThermometer[i], HEX);
  }
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(soilThermometer, 9);
 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(soilThermometer), DEC); 
  Serial.println();
#else
  Serial.println("OneWire disabled");
#endif
}

void setup_dht()
{
#if ENABLE_DHT
  // Initialize device.
  dht.begin();
  Serial.println("DHTxx Unified Sensor Example");
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");  
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");  
  Serial.println("------------------------------------");
#else
  Serial.println("DHT disabled");
#endif
}

void setup_dht2()
{
#if ENABLE_DHT2
  // Initialize device.
  dht2.begin();
  Serial.println("DHTxx Unified Sensor Example");
  // Print temperature sensor details.
  sensor_t sensor;
  dht2.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");  
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht2.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");  
  Serial.println("------------------------------------");
#else
  Serial.println("DHT2 disabled");
#endif
}

void setup()
{
  // turn outlets off by default
  pinMode(PIN_OUTLET1, OUTPUT);
  pinMode(PIN_OUTLET2, OUTPUT);
  pinMode(PIN_OUTLET3, OUTPUT);
  pinMode(PIN_OUTLET4, OUTPUT);
  digitalWrite(PIN_OUTLET1, 0);
  digitalWrite(PIN_OUTLET2, 0);
  digitalWrite(PIN_OUTLET3, 0);
  digitalWrite(PIN_OUTLET4, 0);

  delay(1*1000);
  
  // start serial port
  Serial.begin(9600);
  Serial.println("Seedling Room Controller");

#if ENABLE_LCD
  Wire.begin();
  mcp23017.begin(0);
  lcd.begin();
#else
  Serial.println("LCD disabled");
#endif


  // In case we are restarting because power has just returned to the house
  // wait here for a while to allow the main lake house controller to boot
  // and be in a position to serve NTP time to us.  I have seen Arduino code
  // lock up enventually if unable to get NTP.
#if ENABLE_NET
  {
    unsigned currentTime = millis()/1000, previousTime=0;
    char s[16];
    while(currentTime < BOOT_WAIT_SEC) {
      if (currentTime != previousTime) {
	sprintf(s,"boot wait %d",BOOT_WAIT_SEC-currentTime);
#if ENABLE_LCD
	lcd.update("Seeding Control",s);
#endif
	Serial.println(s);
	previousTime = currentTime;
      }
      currentTime = millis()/1000;
    }
  }
#endif
  
  setup_oneWire();
  setup_dht();
  setup_dht2();
}

void loop_oneWire()
{
#if ENABLE_ONEWIRE
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  if (DBGPRINT_ONEWIRE) Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  if (DBGPRINT_ONEWIRE) Serial.println("DONE");
  
  temperatureSoil = DallasTemperature::toFahrenheit(sensors.getTempC(soilThermometer));
  if (DBGPRINT_ONEWIRE) Serial.print("Temp F: ");
  if (DBGPRINT_ONEWIRE) Serial.println(temperatureSoil);

  if (temperatureSoil < 40.0)
    temperatureSoil = NAN;
  if (temperatureSoil > 100.0)
    temperatureSoil = NAN;
#else
    temperatureSoil = NAN;
#endif  
}

void loop_dht()
{
#if ENABLE_DHT
  // Get temperature event and print its value.
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  temperatureAmbient = DallasTemperature::toFahrenheit(event.temperature);
  if (isnan(event.temperature)) {
    if (DBGPRINT_DHT) Serial.println("Error reading temperature!");
  }
  else {
    if (DBGPRINT_DHT) Serial.print("Temperature: ");
    if (DBGPRINT_DHT) Serial.print(event.temperature);
    if (DBGPRINT_DHT) Serial.println(" *C");
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  humidityAmbient = event.relative_humidity;
  if (isnan(event.relative_humidity)) {
    if (DBGPRINT_DHT) Serial.println("Error reading humidity!");
  }
  else {
    if (DBGPRINT_DHT) Serial.print("Humidity: ");
    if (DBGPRINT_DHT) Serial.print(event.relative_humidity);
    if (DBGPRINT_DHT) Serial.println("%");
  }
#else
  temperatureAmbient = NAN;
  humidityAmbient = NAN;
#endif
}

void loop_dht2()
{
#if ENABLE_DHT2
  // Get temperature event and print its value.
  sensors_event_t event;  
  dht2.temperature().getEvent(&event);
  temperatureAmbient2 = DallasTemperature::toFahrenheit(event.temperature);
  if (isnan(event.temperature)) {
    if (DBGPRINT_DHT2) Serial.println("Error reading temperature!");
  }
  else {
    if (DBGPRINT_DHT2) Serial.print("Temperature: ");
    if (DBGPRINT_DHT2) Serial.print(event.temperature);
    if (DBGPRINT_DHT2) Serial.println(" *C");
  }
  // Get humidity event and print its value.
  dht2.humidity().getEvent(&event);
  humidityAmbient2 = event.relative_humidity;
  if (isnan(event.relative_humidity)) {
    if (DBGPRINT_DHT2) Serial.println("Error reading humidity!");
  }
  else {
    if (DBGPRINT_DHT2) Serial.print("Humidity: ");
    if (DBGPRINT_DHT2) Serial.print(event.relative_humidity);
    if (DBGPRINT_DHT2) Serial.println("%");
  }
#else
  temperatureAmbient2 = NAN;
  humidityAmbient2 = NAN;
#endif
}

unsigned net_up = 0;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
IPAddress timeServer(192, 168, 100, 101);
const int timeZone = -5;  // Eastern Standard Time (USA)
EthernetUDP UdpNTP;
unsigned int localPortNTP = 8888;  // local port to listen for NTP UDP packets
time_t getNtpTime();
IPAddress logServer(192, 168, 100, 101);
EthernetUDP UdpLOG;
unsigned int localPortLOG = 8889;  // local port to listen for log UDP packets
byte logPacket[128];

void loop_net()
{
#if ENABLE_NET
  if (!net_up) {
    if (Ethernet.begin(mac) == 0) {
    } else {
      net_up = 1;
      Serial.print("IP number assigned by DHCP is ");
      Serial.println(Ethernet.localIP());
      UdpNTP.begin(localPortNTP);
      setSyncProvider(getNtpTime);
      UdpLOG.begin(localPortLOG);
    }
  }
#endif
}

void loop()
{
  static uint8_t count=0;
  static uint8_t smoke=0;
  unsigned wait;
  char fstring0[16],fstring1[16],fstring2[16];

  // while waiting for next update cycle check for smoke detector alarm signal
  for(wait=0;wait<5000;) {
#if ENABLE_SMOKE
    if (analogRead(PIN_SMOKE)>512) {
      smoke += (smoke<SMOKE_TRIGGER) ? 1 : 0;
#if DBGPRINT_SMOKE
      Serial.print("smoke++\n");
#endif
    }
#endif
    delay(100);
    wait+=100;
  }

  loop_oneWire();
  loop_dht();  
  loop_dht2();  
  loop_net();
  

  //
  // Update controller outputs
  //

#if (MODE == MODE_seed)

  //
  // Both shelves and COB area for seedlings
  //
  
  if (isnan(temperatureSoil) ||
      timeStatus()==timeNotSet)
    heatSoil = 0;
  else if (!heatSoil && temperatureSoil<77.0)
    heatSoil = 1;
  else if (heatSoil && temperatureSoil>79.0)
    heatSoil = 0;
  
  if (isnan(temperatureAmbient) ||
      timeStatus()==timeNotSet)
    heatRoom = 0;
  else if (!heatRoom && temperatureAmbient<74.0)
    heatRoom = 1;
  else if (heatRoom && temperatureAmbient>76.0)
    heatRoom = 0;
  
  if (timeStatus()==timeNotSet)
    growLightShelves = 0;
  else if (hour() >= (12 - LIGHT_HOURS_V/2 - LIGHT_HOURS_V%2) &&
	   hour() < (12 + LIGHT_HOURS_V/2))
    growLightShelves = 1;
  else
    growLightShelves = 0;

  growLightCOB = growLightShelves;
  
#elif (MODE == MODE_veg_flower)

  //
  // Shelves for vegetative growth, COB area for flowering
  //

  int light_hours;
  float tAmbientTarget;

#define VTOF_TIME (14*SECS_PER_DAY)
  
  tmElements_t vtof_begin =
    { 0, // Second
      0, // Minute
      0, // Hour
      0, // Wday, Sunday is day 1
      7, // Day
      5, // Month
      2021-1970 }; // Year-1970

  {
    time_t t = now(), start=makeTime(vtof_begin);
    Serial.print("now()=");
    Serial.println((unsigned)t,DEC);
    Serial.print("makeTime=");
    Serial.println((unsigned)start,DEC);
    if (t < start) {
      light_hours = LIGHT_HOURS_V;
    } else if (t > (start + VTOF_TIME)) {
      light_hours = LIGHT_HOURS_F;
    } else {
      t -= start;
      light_hours = LIGHT_HOURS_V - (LIGHT_HOURS_V - LIGHT_HOURS_F) * t / VTOF_TIME;
    }
    Serial.print("light_hours=");
    Serial.println(light_hours,DEC);
  }

  
  heatSoil = 0;
  
  if (timeStatus()==timeNotSet)
    growLightShelves = 0;
  else if (hour() >= (12 - LIGHT_HOURS_V/2 - LIGHT_HOURS_V%2) &&
	   hour() < (12 + LIGHT_HOURS_V/2))
    growLightShelves = 1;
  else
    growLightShelves = 0;
  
  if (timeStatus()==timeNotSet) {
    growLightCOB = 0;
    tAmbientTarget = 60.0;
  } else if (hour() >= (12 - light_hours/2 - light_hours%2) &&
	     hour() < (12 + light_hours/2)) {
    growLightCOB = 1;
    tAmbientTarget = 70.0;
  } else {
    growLightCOB = 0;
    tAmbientTarget = 60.0;
  }
  
  if (isnan(temperatureAmbient) ||
      timeStatus()==timeNotSet)
    heatRoom = 0;
  else if (!heatRoom && temperatureAmbient<(tAmbientTarget-1.0))
    heatRoom = 1;
  else if (heatRoom && temperatureAmbient>(tAmbientTarget+1.0))
    heatRoom = 0;
  
#elif (MODE == MODE_veg_dry)
  
  //
  // Shelves for vegetative growth, COB area for drying
  //
  
  heatSoil = 0;
  
  if (timeStatus()==timeNotSet)
    growLightShelves = 0;
  else if (hour() >= (12 - LIGHT_HOURS_V/2 - LIGHT_HOURS_V%2) &&
	   hour() < (12 + LIGHT_HOURS_V/2))
    growLightShelves = 1;
  else
    growLightShelves = 0;
  
  if (isnan(temperatureAmbient) ||
      isnan(humidityAmbient) ||
      timeStatus()==timeNotSet)
    heatRoom = 0;
  else if (!heatRoom &&
	   ( humidityAmbient > 70.0 &&
	     temperatureAmbient < 69.0 ))
    heatRoom = 1;
  else if (heatRoom &&
	   ( humidityAmbient < 60.0 ||
	     temperatureAmbient > 70.0 )) 
    heatRoom = 0;
  
  growLightCOB = 0;

#else

  heatSoil = 0;
  heatRoom = 0;
  growLightShelves = 0;
  growLightCOB = 0;
  
#endif
    
  if (smoke==SMOKE_TRIGGER) {
    heatSoil = 0;
    heatRoom = 0;
    growLightShelves = 0;
  }
  
  digitalWrite(PIN_OUTLET1, heatSoil);
  delay(500);
  digitalWrite(PIN_OUTLET2, heatRoom);
  delay(500);
  digitalWrite(PIN_OUTLET3, growLightShelves);
  delay(500);
  digitalWrite(PIN_OUTLET4, growLightCOB);
  delay(500);

  //
  // Update LCD display
  //
  // ................
  // Y xx.x xx% xx:xx
  // S xx.x SHLF MODE
  //

  if (isnan((count%2) ? temperatureAmbient2 : temperatureAmbient))
    strcpy(fstring0,"----");
  else
    dtostrf((count%2) ? temperatureAmbient2 : temperatureAmbient,4,1,fstring0);
  
  if (isnan((count%2) ? humidityAmbient2 : humidityAmbient))
    strcpy(fstring1,"--");
  else
    dtostrf((count%2) ? humidityAmbient2 : humidityAmbient,2,0,fstring1);

  if (timeStatus()==timeNotSet) {
    strcpy(fstring2,"--:--");
  } else {
    sprintf(fstring2,"%02d:%02d",hour(),minute());
  }

  sprintf(topLine,"%d %s %s%% %s",
	  count%2,
	  fstring0,
	  fstring1,
	  fstring2);

  if (isnan(temperatureSoil))
    strcpy(fstring0,"----");
  else
    dtostrf(temperatureSoil,4,1,fstring0);
  
  if (smoke<SMOKE_TRIGGER) {
#if (MODE == MODE_seed)
    sprintf(botLine,"S %s %c%c%c%c SEED",
#elif (MODE == MODE_veg_flower)
    sprintf(botLine,"S %s %c%c%c%c VF",
#elif (MODE == MODE_veg_dry)
    sprintf(botLine,"S %s %c%c%c%c VDRY",
#else
    sprintf(botLine,"S %s %c%c%c%c OFF",
#endif
	    fstring0,
	    heatSoil ? 'S' : '.',
	    heatRoom ? 'H' : '.',
	    growLightShelves ? 'L' : '.',
	    growLightCOB ? 'C' : '.' );
  } else {
    sprintf(botLine,"S %s !!SMOKE!!",fstring0);
  }

#if ENABLE_LCD
  lcd.update(topLine,botLine);
  lcd.updateLED(0,count&1);
  lcd.updateLED(1,count&2);
  lcd.updateLED(2,count&4);
  lcd.updateLED(3,count&8);
#endif

  Serial.println(topLine);
  Serial.println(botLine);


  //
  // Send log info to log server
  //
  
#if ENABLE_NET
  sprintf(logPacket,"%d,%d,%d,%d,%d,",year(),month(),day(),hour(),minute());

  if (isnan(temperatureAmbient))
    strcpy(fstring0,"x");
  else
    dtostrf(temperatureAmbient,4,1,fstring0);
  sprintf(logPacket+strlen(logPacket),"%s,",fstring0);

  if (isnan(humidityAmbient))
    strcpy(fstring1,"x");
  else
    dtostrf(humidityAmbient,2,0,fstring1);
  sprintf(logPacket+strlen(logPacket),"%s,",fstring1);

  if (isnan(temperatureSoil))
    strcpy(fstring0,"x");
  else
    dtostrf(temperatureSoil,4,1,fstring0);
  sprintf(logPacket+strlen(logPacket),"%s,",fstring0);

  sprintf(logPacket+strlen(logPacket),"%d,",heatSoil?1:0);
  sprintf(logPacket+strlen(logPacket),"%d,",heatRoom?1:0);
  sprintf(logPacket+strlen(logPacket),"%d,",growLightShelves?1:0);
  sprintf(logPacket+strlen(logPacket),"%d,",growLightCOB?1:0);

  sprintf(logPacket+strlen(logPacket),"%d,",logCount);

  if (isnan(temperatureAmbient2))
    strcpy(fstring0,"x");
  else
    dtostrf(temperatureAmbient2,4,1,fstring0);
  sprintf(logPacket+strlen(logPacket),"%s,",fstring0);

  if (isnan(humidityAmbient2))
    strcpy(fstring1,"x");
  else
    dtostrf(humidityAmbient2,2,0,fstring1);
  sprintf(logPacket+strlen(logPacket),"%s,",fstring1);

  sprintf(logPacket+strlen(logPacket),"%d\n",(smoke<SMOKE_TRIGGER) ? 0 : 1);  

  if (DBGPRINT_NET) Serial.print((char *)logPacket);
  
  UdpLOG.beginPacket(logServer, 8888);
  UdpLOG.write(logPacket, strlen(logPacket));
  UdpLOG.endPacket();
  logCount++;
#endif
  
  count++;
}


/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBufferNTP[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBufferNTP, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBufferNTP[0] = 0b11100011;   // LI, Version, Mode
  packetBufferNTP[1] = 0;     // Stratum, or type of clock
  packetBufferNTP[2] = 6;     // Polling Interval
  packetBufferNTP[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBufferNTP[12]  = 49;
  packetBufferNTP[13]  = 0x4E;
  packetBufferNTP[14]  = 49;
  packetBufferNTP[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  UdpNTP.beginPacket(address, 123); //NTP requests are to port 123
  UdpNTP.write(packetBufferNTP, NTP_PACKET_SIZE);
  UdpNTP.endPacket();
}

time_t getNtpTime()
{
  while (UdpNTP.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = UdpNTP.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      UdpNTP.read(packetBufferNTP, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBufferNTP[40] << 24;
      secsSince1900 |= (unsigned long)packetBufferNTP[41] << 16;
      secsSince1900 |= (unsigned long)packetBufferNTP[42] << 8;
      secsSince1900 |= (unsigned long)packetBufferNTP[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}
