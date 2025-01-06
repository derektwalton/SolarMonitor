#include <HardwareSerial.h>
#include <Wire.h>
#include "MCP23017.h"

#define MCP23017_ADDRESS 0x20

// Microchip IO expander MCP23017

// addresses for IOCON.BANK = 0 (power up default)
#define IODIRA   0x00
#define IODIRB   0x01
#define IPOLA    0x02
#define IPOLB    0x03
#define GPINTENA 0x04
#define GPINTENB 0x05
#define DEFVALA  0x06
#define DEFVALB  0x07
#define INTCONA  0x08
#define INTCONB  0x09
#define IOCONA   0x0a
#define IOCONB   0x0b
#define GPPUA    0x0c
#define GPPUB    0x0d
#define INTFA    0x0e
#define INTFB    0x0f
#define INTCAPA  0x10
#define INTCAPB  0x11
#define GPIOA    0x12
#define GPIOB    0x13
#define OLATA    0x14
#define OLATB    0x15

void MCP23017::begin(uint8_t addr)
{
  uint8_t d;

  Serial.println("MCP23017::begin");

  if (addr > 7) {
    addr = 7;
  }
  i2caddr = addr;
  
  // check to see if communication ok and set port A and B
  // to all all inputs
  writeReg(IODIRA,0x55);
  readReg(IODIRA, &d);
  if (d!=0x55) {
    Serial.println("  MCP23017 communication no good");
  }
  writeReg(IODIRA,0xff);
  readReg(IODIRA, &d);
  if (d!=0xff) {
    Serial.println("  MCP23017 communication no good");
  }
  writeReg(IODIRB,0xff);

  shadow.iodira = 0xff;
  shadow.iodirb = 0xff;
  shadow.olata = 0x00;
  shadow.olatb = 0x00;
}

void MCP23017::begin(void)
{
  begin(0);
}

void MCP23017::writeReg(uint8_t a, uint8_t d)
{
  //Serial.println("MCP23017::writeReg");
  Wire.beginTransmission(MCP23017_ADDRESS | i2caddr);
  Wire.write(a);
  Wire.write(d);
  Wire.endTransmission();
}


void MCP23017::readReg(uint8_t a, uint8_t *d)
{
  //Serial.println("MCP23017::readReg");
  Wire.beginTransmission(MCP23017_ADDRESS | i2caddr);
  Wire.write(a);
  Wire.endTransmission();
  Wire.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  *d = Wire.read();
}

void MCP23017::gpio_drive(int selB, uint8_t d)
{
  if (selB) {
    shadow.iodirb = ~d;
    writeReg(IODIRB, shadow.iodirb);
  } else {
    shadow.iodira = ~d;
    writeReg(IODIRA, shadow.iodira);
  }
}

void MCP23017::gpio_drive_bits(int selB, uint8_t d, uint8_t mask)
{
  if (selB) {
    shadow.iodirb = shadow.iodirb & ~mask | d & mask;
    writeReg(IODIRB, shadow.iodirb);
  } else {
    shadow.iodira = shadow.iodira & ~mask | d & mask;
    writeReg(IODIRA, shadow.iodira);
  }
}

void MCP23017::gpio_drive_setbits(int selB, uint8_t d)
{
  if (selB) {
    shadow.iodirb &= ~d;
    writeReg(IODIRB, shadow.iodirb);
  } else {
    shadow.iodira &= ~d;
    writeReg(IODIRA, shadow.iodira);
  }
}

void MCP23017::gpio_drive_clrbits(int selB, uint8_t d)
{
  if (selB) {
    shadow.iodirb |= d;
    writeReg(IODIRB, shadow.iodirb);
  } else {
    shadow.iodira |= d;
    writeReg(IODIRA, shadow.iodira);
  }
}

void MCP23017::gpio_output(int selB, uint8_t d)
{
  if (selB) {
    shadow.olatb = d;
    writeReg(OLATB, shadow.olatb);
  } else {
    shadow.olata = d;
    writeReg(OLATA, shadow.olata);
  }
}

void MCP23017::gpio_output_bits(int selB, uint8_t d, uint8_t mask)
{
  if (selB) {
    shadow.olatb = shadow.olatb & ~mask | d & mask;
    writeReg(OLATB, shadow.olatb);
  } else {
    shadow.olata = shadow.olata & ~mask | d & mask;
    writeReg(OLATA, shadow.olata);
  }
}

void MCP23017::gpio_output_setbits(int selB, uint8_t d)
{
  if (selB) {
    shadow.olatb |= d;
    writeReg(OLATB, shadow.olatb);
  } else {
    shadow.olata |= d;
    writeReg(OLATA, shadow.olata);
  }
}

void MCP23017::gpio_output_clrbits(int selB, uint8_t d)
{
  if (selB) {
    shadow.olatb &= ~d;
    writeReg(OLATB, shadow.olatb);
  } else {
    shadow.olata &= ~d;
    writeReg(OLATA, shadow.olata);
  }
}

void MCP23017::gpio_input(int selB, uint8_t *d)
{
  if (selB) {
    readReg(GPIOB, d);
  } else {
    readReg(GPIOA, d);
  }
}
