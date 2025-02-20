#include <MCP23017.h>
#include "lcd.h"

//
// Mapping of GPIOs to character LCD signals
//
// DB[7:0]  D[7:0]    GPA[7:0]
// R/W      A0        GPB[0]
// RS       A1        GPB[1]
//          A2        GPB[2]
//          A3        GPB[3]
// E        CS0       GPB[4]
//          CS1       GPB[5]
//          CS2       GPB[6]
//          (NC)      GPB[7]
//

#define PORTB 1
#define PORTA 0

#define PORTB_RW   (1<<0)
#define PORTB_RS   (1<<1)
#define PORTB_LED0 (1<<2)
#define PORTB_LED1 (1<<3)
#define PORTB_E    (1<<4)
#define PORTB_LED2 (1<<5)
#define PORTB_LED3 (1<<6)

LCD::LCD(MCP23017* _mcp23017, bool _ledLoForOn)
{
  mcp23017 = _mcp23017;
  ledLoForOn = _ledLoForOn;
}

void LCD::begin(void)
{
  // always drive address and chip selects
  mcp23017->gpio_drive(PORTB, 0xff);  

  // function set 8-bit bus, 2-lines, 5x8 chars
  write(0, 0x38);
  waitBusy();

  // display on
  write(0, 0x0e);
  waitBusy();
  
  // entry mode set, increase address by one
  write(0, 0x06);
  waitBusy();

  // clear display
  write(0, 0x01);
  waitBusy();
}

void LCD::write(int rs, uint8_t d)
{
  mcp23017->gpio_output_bits(PORTB, (rs ? PORTB_RS : 0), PORTB_RS | PORTB_RW); // present address and R/W = 0
  mcp23017->gpio_drive(PORTA, 0xff);          // drive data bus
  mcp23017->gpio_output(PORTA, d);            // output data onto bus
  mcp23017->gpio_output_setbits(PORTB, PORTB_E); // E to high
  mcp23017->gpio_output_clrbits(PORTB, PORTB_E); // E to low
}

void LCD::read(int rs, uint8_t *d)
{
  mcp23017->gpio_output_bits(PORTB, (rs ? PORTB_RS : 0) | PORTB_RW, PORTB_RS | PORTB_RW); // present address and R/W = 1
  mcp23017->gpio_drive(PORTA, 0x00);          // tri-state data bus drivers
  mcp23017->gpio_output_setbits(PORTB, PORTB_E); // E to high
  mcp23017->gpio_input(PORTA, d);             // sample data bus
  mcp23017->gpio_output_clrbits(PORTB, PORTB_E); // E to low
}

void LCD::waitBusy()
{
  uint8_t d;
  do {
    read(0,&d);
  } while (d & 0x80);
}

void LCD::update(char *topLine, char *botLine)
{
  int i;

  // clear display
  write(0, 0x01);
  waitBusy();
  
  for(i=0;i<16;i++) {
    if (!topLine[i])
      break;
    // entry mode set, increase address by one
    write(1, topLine[i]);
    waitBusy();
  }
  
  // move to beginning of bottom line
  write(0, 0x80 + 64);
  waitBusy();
  
  for(i=0;i<16;i++) {
    if (!botLine[i])
      break;
    // entry mode set, increase address by one
    write(1, botLine[i]);
    waitBusy();
  }
  
}

void LCD::updateLED(uint8_t led, uint8_t on)
{
  uint8_t mask;

  switch(led) {
  case 0: mask = PORTB_LED0; break;
  case 1: mask = PORTB_LED1; break;
  case 2: mask = PORTB_LED2; break;
  case 3: mask = PORTB_LED3; break;
  default: mask = 0; break;
  } 

  if (mask) {
    if (on && !ledLoForOn || !on && ledLoForOn) {
      mcp23017->gpio_output_setbits(PORTB, mask);
    } else {
      mcp23017->gpio_output_clrbits(PORTB, mask);
    }
  }

}
