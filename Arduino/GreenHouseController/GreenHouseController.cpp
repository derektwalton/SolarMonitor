#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <MCP23017.h>
#include <lcd.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>


// In preferences add the following as additional board manager URL
// https://arduino.esp8266.com/stable/package_esp8266com_index.json
//
// In board manager, search for and install esp8266
//
// Select Arduino->tools->Board
//
// NodeMCU 1.0 (ESP-12E Module)
//   80 Mhz
//   115200
//   4M (3M SPIFFS)
//


// Pin use
//
// D1 SPI
// D2 SPI
// D3 DHT
// D4 One wire temperature
//

char ssid[] = "WaltonStudio";  //  your network SSID (name)
char pass[] = "deadbeef00";       // your network password

// options for debug away from actual devices
#define ENABLE_LCD 1
#define ENABLE_DHT 1
#define ENABLE_ONEWIRE 1
#define ENABLE_NET 1

// LCD strings
char topLine[32] = {0};
char botLine[32] = {0};

#if ENABLE_DHT
#define DHTPIN            D3        // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);
#endif

#if ENABLE_LCD
MCP23017 mcp23017;
LCD lcd(&mcp23017,1);
#endif

#if ENABLE_ONEWIRE
#define ONEWIRE_MAX 8
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(D4); // pin -OR- pin_I, pin_O, pin_OEn

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address and converted temperatures
unsigned nOneWire;
struct {
  DeviceAddress address;
  unsigned hash;
  float temperature;
} oneWireDevices[ONEWIRE_MAX];

// map oneWire hashes to location
//Device0address: 284EF6EB020000D0 hash: A9
//Device1address: 28B134EC020000A1 hash: E2
//Device2address: 2843C379970403EE hash: AF
//Device3address: 283B0FEC02000070 hash: 82
//Device4address: 2284792400000043 hash: B8
struct {
  unsigned hash;
  char name[8];
} oneWireMap[] =
  {
    { 0xa9, "gnd13" }, // 13" deep in ground under middle of the pool
    { 0xe2, "gnd3" }, // 3" deep in ground under middle of the pool
    { 0xaf, "pool" }, // pool water
    { 0xb8, "panel" }, // solar panel air space temperature
    { 0x82, "inner" }, // inner greenhouse / cold frame
    { 0x00, "out" }, // outside ambient temperature
    { 0x00, "spare1" },
    { 0x00, "spare2" },
    { 0x00, "" }
  };

float ow_getTemp(char *name)
{
  unsigned i,j;
  for(j=0;oneWireMap[j].name[0];j++) {
    if (!strcmp(name,oneWireMap[j].name)) {
      for(i=0;i<nOneWire;i++) {
	if (oneWireMap[j].hash == oneWireDevices[i].hash)
	  return oneWireDevices[i].temperature;
      }
    }
  }
  return NAN;
}

#endif

float temperatureAmbient;
float humidityAmbient;
unsigned logCount = 0;

#define DBGPRINT_ONEWIRE 0
#define DBGPRINT_DHT 0
  
#define BOOT_WAIT_SEC 0 //(5*60)

void setup_oneWire()
{
  unsigned j;

  // start out all termperatures at NaN
  for(j=0;j<ONEWIRE_MAX;j++) {
    oneWireDevices[j].temperature=NAN;
  }

#if ENABLE_ONEWIRE
  // locate devices on the bus
  delay(1000);
  Serial.print("Locating devices...");
  sensors.begin();
  nOneWire = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(nOneWire, DEC);
  Serial.println(" devices.");
  if (nOneWire > ONEWIRE_MAX)
    nOneWire = ONEWIRE_MAX;

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode())
    Serial.println("ON");
  else
    Serial.println("OFF");
  
  for(j=0;j<nOneWire;j++) {
    if (!sensors.getAddress(oneWireDevices[j].address, j)) {
      Serial.print("Unable to find address for device "); 
      Serial.print(j);
    }
  }
  
  // show the addresses we found on the bus
  for(j=0;j<nOneWire;j++) {
    oneWireDevices[j].hash = 0x00;
    Serial.print("Device");
    Serial.print(j);
    Serial.print("address: ");
    for (uint8_t i = 0; i < 8; i++)
      {
	if (oneWireDevices[j].address[i] < 16) Serial.print("0");
	Serial.print(oneWireDevices[j].address[i], HEX);
	oneWireDevices[j].hash ^= oneWireDevices[j].address[i];
      }
    Serial.print(" hash: ");
    if (oneWireDevices[j].hash < 16) Serial.print("0");
    Serial.print(oneWireDevices[j].hash, HEX);
    Serial.println();
  }

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different
  // resolutions)
  sensors.setResolution(oneWireDevices[0].address, 9);
  
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(oneWireDevices[0].address), DEC); 
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

void setup()
{
  // start serial port
  Serial.begin(115200);
  Serial.println("Greenhouse Controller");

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
}

void loop_oneWire()
{
#if ENABLE_ONEWIRE
  unsigned j;

  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  if (DBGPRINT_ONEWIRE) Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  if (DBGPRINT_ONEWIRE) Serial.println("DONE");
  
  // loop thru all devices reading and qualifying temperatures
  for(j=0;j<nOneWire;j++) {
    oneWireDevices[j].temperature = DallasTemperature::toFahrenheit
      (sensors.getTempC(oneWireDevices[j].address));
    if (DBGPRINT_ONEWIRE) Serial.print("Temp F: ");
    if (DBGPRINT_ONEWIRE) Serial.println(oneWireDevices[j].temperature);
    if (oneWireDevices[j].temperature < -25.0)
      oneWireDevices[j].temperature = NAN;
    if (oneWireDevices[j].temperature > 180.0)
      oneWireDevices[j].temperature = NAN;
    // some errors result in 0x000 for temperature in Celsius, send to NAN
    if (oneWireDevices[j].temperature == 32.0)
      oneWireDevices[j].temperature = NAN;
  }
#else
#endif  
}

void loop_dht()
{
#if ENABLE_DHT
  static int t0;
  float temperature, humidity;
  int tElapsed;

  tElapsed = minute() - t0;
  if (tElapsed < 0 ) tElapsed += 60;
  
  // Get temperature event and print its value.
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  temperature = event.temperature*9.0/5.0 + 32;
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
  humidity = event.relative_humidity;
  if (isnan(event.relative_humidity)) {
    if (DBGPRINT_DHT) Serial.println("Error reading humidity!");
  }
  else {
    if (DBGPRINT_DHT) Serial.print("Humidity: ");
    if (DBGPRINT_DHT) Serial.print(event.relative_humidity);
    if (DBGPRINT_DHT) Serial.println("%");
  }

  if (!isnan(temperature) && !isnan(humidity)) {
    temperatureAmbient = temperature;
    humidityAmbient = humidity;
    t0 = minute();
  } else if (tElapsed > 5) {
    temperatureAmbient = temperature;
    humidityAmbient = humidity;
  }
#else
  temperatureAmbient = NAN;
  humidityAmbient = NAN;
#endif
}

unsigned net_state = 0;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xCE }; 
IPAddress timeServer(192, 168, 100, 101);
const int timeZone = -5;  // Eastern Standard Time (USA)
WiFiUDP UdpNTP;
unsigned int localPortNTP = 8888;  // local port to listen for NTP UDP packets
time_t getNtpTime();
IPAddress logServer(192, 168, 100, 101);
WiFiUDP UdpLOG;
unsigned int localPortLOG = 8889;  // local port to listen for log UDP packets
char logPacket[256];

void loop_net()
{
#if ENABLE_NET
  switch(net_state) {
  case 0:
    // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    net_state++;
    break;
  case 1:
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
    } else {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      UdpNTP.begin(localPortNTP);
      setSyncProvider(getNtpTime);
      UdpLOG.begin(localPortLOG);
      net_state++;
    }
    break;
  case 2:
    break;
  }
#endif
}

typedef enum {
  PUMP_STATE_off,
  PUMP_STATE_primeSump,
  PUMP_STATE_primeBoth,
  PUMP_STATE_on,
  PUMP_STATE_drainBack
} PUMP_STATE_t;

unsigned pump_state = PUMP_STATE_off;

typedef enum {
  CLIMATE_STATE_idle,
  CLIMATE_STATE_hot,
  CLIMATE_STATE_cold
} CLIMATE_STATE_t;

unsigned climate_state = CLIMATE_STATE_idle;

unsigned pumpSumpOn = 0;
unsigned pumpCircOn = 0;
unsigned fanOn = 0;
unsigned dehumidifierOn = 0;

void loop_control()
{
  static int pump_t0;
  int tElapsed;

  tElapsed = minute() - pump_t0;
  if (tElapsed < 0 ) tElapsed += 60;
  
  switch(pump_state) {

  case PUMP_STATE_off:
    if (timeStatus()!=timeNotSet &&
	( !isnan(ow_getTemp("panel")) &&
	  !isnan(ow_getTemp("pool")) &&
	  (ow_getTemp("panel") > (ow_getTemp("pool") + 30.0))
	  ||
	  !isnan(ow_getTemp("panel")) &&
	  (ow_getTemp("panel") > 100.0) ) ) {
      pump_t0 = minute();
      pump_state = PUMP_STATE_primeSump;
    }
    break;

  case PUMP_STATE_primeSump:
    if (timeStatus()==timeNotSet) {
      pump_state = PUMP_STATE_off;
    } else if (tElapsed > 3) {
      pump_t0 = minute();
      pump_state = PUMP_STATE_primeBoth;
    }
    break;
    
  case PUMP_STATE_primeBoth:
    if (timeStatus()==timeNotSet) {
      pump_state = PUMP_STATE_off;
    } else if (tElapsed > 2) {
      pump_t0 = minute();
      pump_state = PUMP_STATE_on;
    }
    break;
    
  case PUMP_STATE_on:
    if ( !( !isnan(ow_getTemp("panel")) &&
	    !isnan(ow_getTemp("pool")) &&
	    (ow_getTemp("panel") > (ow_getTemp("pool") + 10.0)) ) ) {
      pump_t0 = minute();
      pump_state = PUMP_STATE_drainBack;
    }
    break;
    
  case PUMP_STATE_drainBack:
    if (timeStatus()==timeNotSet) {
      pump_state = PUMP_STATE_off;
    } else if (tElapsed > 5) {
      pump_state = PUMP_STATE_off;
    }
    break;

  }


  switch(climate_state) {

  case CLIMATE_STATE_idle:
    if (!isnan(temperatureAmbient) &&
	(temperatureAmbient > 76.0)) {
      climate_state = CLIMATE_STATE_hot;
    } else if (!isnan(temperatureAmbient) &&
	       (temperatureAmbient < 38.0)) {
      climate_state = CLIMATE_STATE_cold;
    }
    break;
  
  case CLIMATE_STATE_hot:
    if (isnan(temperatureAmbient) || (temperatureAmbient < 74.0)) {
      climate_state = CLIMATE_STATE_idle;
    }
    break;
    
  case CLIMATE_STATE_cold:
    if (isnan(temperatureAmbient) || (temperatureAmbient > 40.0)) {
      climate_state = CLIMATE_STATE_idle;
    }
    break;
    
  }

  pumpSumpOn = (pump_state == PUMP_STATE_primeSump) || (pump_state == PUMP_STATE_primeBoth);
  pumpCircOn = (pump_state == PUMP_STATE_primeBoth) || (pump_state == PUMP_STATE_on);
  fanOn = (climate_state == CLIMATE_STATE_hot);
  dehumidifierOn = (climate_state == CLIMATE_STATE_cold);

}

void loop()
{
  static uint8_t count=0;
  char fstring0[16],fstring1[16],fstring2[16];

  delay(5000);

  loop_oneWire();
  loop_dht();  
  loop_control();
  loop_net();
  

  //
  // Update LCD display
  //
  // ................
  // A xx.x xx% xx:xx
  // OW # xx.x
  //

  if (isnan(temperatureAmbient))
    strcpy(fstring0,"----");
  else
    dtostrf(temperatureAmbient,4,1,fstring0);
  
  if (isnan(humidityAmbient))
    strcpy(fstring1,"--");
  else
    dtostrf(humidityAmbient,2,0,fstring1);

  if (timeStatus()==timeNotSet) {
    strcpy(fstring2,"--:--");
  } else {
    snprintf(fstring2,sizeof(fstring2),"%02d:%02d",hour(),minute());
  }

  snprintf(topLine,sizeof(topLine),"A %s %s%% %s",
	   fstring0,
	   fstring1,
	   fstring2);

  if (nOneWire) {
    unsigned i = count % nOneWire,j;
  
    if (isnan(oneWireDevices[i].temperature))
      strcpy(fstring0,"----");
    else
      dtostrf(oneWireDevices[i].temperature,4,1,fstring0);

    for(j=0;oneWireMap[j].name[0];j++) {
      if (oneWireMap[j].hash == oneWireDevices[i].hash)
	break;
    }
    if (oneWireMap[j].name[0]) {
      snprintf(botLine,sizeof(botLine),"OW %s %s",
	       oneWireMap[j].name,
	       fstring0);
    } else {
      snprintf(botLine,sizeof(botLine),"OW %d %x %s",
	       i,
	       oneWireDevices[i].hash,
	       fstring0);
    }

  } else {
    snprintf(botLine,sizeof(botLine),"OW (null)");
  }

#if ENABLE_LCD
  lcd.update(topLine,botLine);
  lcd.updateLED(0,pumpSumpOn ? 0 : 1);
  lcd.updateLED(1,pumpCircOn ? 0 : 1);
  lcd.updateLED(2,fanOn ? 0 : 1);
  lcd.updateLED(3,dehumidifierOn ? 0 : 1);
#endif

  Serial.println(topLine);
  Serial.println(botLine);


  //
  // Send log info to log server
  //
  
#if ENABLE_NET
  snprintf(logPacket,sizeof(logPacket),
	   "%d,%d,%d,%d,%d,",year(),month(),day(),hour(),minute());

  if (isnan(temperatureAmbient))
    strcpy(fstring0,"x");
  else
    dtostrf(temperatureAmbient,4,1,fstring0);
  snprintf(logPacket+strnlen(logPacket,sizeof(logPacket)),
	   sizeof(logPacket)-strnlen(logPacket,sizeof(logPacket)),
	   "%s,",fstring0);

  if (isnan(humidityAmbient))
    strcpy(fstring1,"x");
  else
    dtostrf(humidityAmbient,2,0,fstring1);
  snprintf(logPacket+strnlen(logPacket,sizeof(logPacket)),
	   sizeof(logPacket)-strnlen(logPacket,sizeof(logPacket)),
	   "%s,",fstring1);

  {
    unsigned i,j;
    for(j=0;oneWireMap[j].name[0];j++) {
      for(i=0;i<nOneWire;i++) {
	if (oneWireMap[j].hash == oneWireDevices[i].hash)
	  break;
      }
      if (i==nOneWire || isnan(oneWireDevices[i].temperature))
	strcpy(fstring0,"x");
      else
	dtostrf(oneWireDevices[i].temperature,4,1,fstring0);
      snprintf(logPacket+strnlen(logPacket,sizeof(logPacket)),
	       sizeof(logPacket)-strnlen(logPacket,sizeof(logPacket)),
	       "%s,",fstring0);
    }
  }

  // control outputs
  snprintf(logPacket+strnlen(logPacket,sizeof(logPacket)),
	   sizeof(logPacket)-strnlen(logPacket,sizeof(logPacket)),
	   "%d,",pumpSumpOn ? 1 : 0);
  snprintf(logPacket+strnlen(logPacket,sizeof(logPacket)),
	   sizeof(logPacket)-strnlen(logPacket,sizeof(logPacket)),
	   "%d,",pumpCircOn ? 1 : 0);
  snprintf(logPacket+strnlen(logPacket,sizeof(logPacket)),
	   sizeof(logPacket)-strnlen(logPacket,sizeof(logPacket)),
	   "%d,",fanOn ? 1 : 0);
  snprintf(logPacket+strnlen(logPacket,sizeof(logPacket)),
	   sizeof(logPacket)-strnlen(logPacket,sizeof(logPacket)),
	   "%d,",dehumidifierOn ? 1 : 0);

  snprintf(logPacket+strnlen(logPacket,sizeof(logPacket)),
	   sizeof(logPacket)-strnlen(logPacket,sizeof(logPacket)),
	   "%d\n",logCount);

  UdpLOG.beginPacket(logServer, 8889);
  UdpLOG.write((byte *)logPacket, strnlen(logPacket,sizeof(logPacket)));
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
