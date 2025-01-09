#define NO_I2C 1
#define CONTROL_NO_TSTAT 1

#include "defines.h"
#include "uart.h"
#include "fetswitch.h"
#include "relay.h"
#include "adc.h"
#include "switch.h"
#include "led.h"
#include "power.h"
#include "time.h"
#include "temperature.h"
#include "thermal.h"
#include "transmit.h"
#include "control.h"
#include "i2c.h"
#include "cmdrsp.h"
#include "debug.c"

void setup() {

  uart_init();
  time_init();
  fetSwitchInit();
  relayInit();
  adcInit();
  ledInit();
  i2c_init();
  temperature_Init();

  debug_puts("d: INIT DONE\n");
}

void loop() {
  thermal_poll();
  cmdrsp_poll();
    
  control_poll();

  transmit();
}
