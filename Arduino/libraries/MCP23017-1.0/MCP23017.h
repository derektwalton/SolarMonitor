#ifndef __MCP23017_h__
#define __MCP23017_h__

#include <stdint.h>

class MCP23017 {

 public: 

  void begin(uint8_t addr);
  void begin(void);

  void gpio_drive(int selB, uint8_t d);
  void gpio_drive_bits(int selB, uint8_t d, uint8_t mask);
  void gpio_drive_setbits(int selB, uint8_t d);
  void gpio_drive_clrbits(int selB, uint8_t d);
  void gpio_output(int selB, uint8_t d);
  void gpio_output_bits(int selB, uint8_t d, uint8_t mask);
  void gpio_output_setbits(int selB, uint8_t d);
  void gpio_output_clrbits(int selB, uint8_t d);
  void gpio_input(int selB, uint8_t *d);
  
 private:

  uint8_t i2caddr;

  struct {
    uint8_t iodira;
    uint8_t iodirb;
    uint8_t olata;
    uint8_t olatb;
  } shadow;
  
  void writeReg(uint8_t a, uint8_t d);
  void readReg(uint8_t a, uint8_t *d);

};

#endif
