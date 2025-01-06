#ifndef __lcd_h__
#define __lcd_h__

#include <stdint.h>
#include <MCP23017.h>
 
class LCD {

 public: 

  LCD(MCP23017*, bool _ledLoForOn);
  void begin(void);
  void update(char *topLine, char *botLine);
  void updateLED(uint8_t led, uint8_t on);

 private:

  MCP23017* mcp23017;
  bool ledLoForOn;

  void write(int rs, uint8_t d);
  void read(int rs, uint8_t *d);
  void waitBusy(void);

};

#endif
