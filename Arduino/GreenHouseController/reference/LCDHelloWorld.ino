#include <Wire.h>
#include "MCP23017.h"
#include "lcd.h"

char topLine[] = "HelloWorldLine0";
char botLine[] = "HelloWorldLine1";

MCP23017 mcp23017;
LCD lcd(&mcp23017,1);

void setup()
{
  delay(5*1000);
  
  // start serial port
  Serial.begin(115200);
  Serial.println("LCD Hello World");

  Wire.begin(D2,D1); // join i2c bus (address optional for master)
  mcp23017.begin(0);
  lcd.begin();
}

void loop()
{
  static uint8_t count=0;

  lcd.update(topLine,botLine);
  
  lcd.updateLED(0,count&1);
  lcd.updateLED(1,count&2);
  lcd.updateLED(2,count&4);
  lcd.updateLED(3,count&8);

  delay(1000);
  count++;
}
