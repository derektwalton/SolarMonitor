#include <inttypes.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"

#include <util/twi.h>

#define I2C_OUR_ADDRESS 0x01

#if NO_I2C

void i2c_init(void)
{}

#else

static uint8_t *data;
static int n,i;
static int get;
static void (*cback)(uint8_t *s, int n, int error);

// I2C (two wire interface) interrupt
ISR(TWI_vect)
{
  switch (TW_STATUS) {

  case TW_SR_SLA_ACK:
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
    break;

  case TW_SR_DATA_ACK:
    if (i<n) data[i] = TWDR;
    i++;
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
    break;
    
  case TW_SR_STOP:
    TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA);
    cback(data, i, 0);
    break;
    
  case TW_ST_SLA_ACK:
    TWDR = data[i++];
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | ((i<n) ? _BV(TWEA) : 0);
    break;

  case TW_ST_DATA_ACK:
    TWDR = data[i++];
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | ((i<n) ? _BV(TWEA) : 0);
    break;
    
  case TW_ST_LAST_DATA:
  case TW_ST_DATA_NACK:
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
    cback(data, i, 0);
    break;

  default:
    break;

  }

}

void i2c_init(void)
{
  TWAR = I2C_OUR_ADDRESS << 1;
  TWCR = _BV(TWEN) | _BV(TWEA);
}

int i2c_get
( uint8_t *s,
  int len,
  void (*callback)(uint8_t *s, int n, int error))
{
  data = s;
  n = len;
  get = 1;
  cback = callback;
  i = 0;
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
  return SUCCESS;
}

int i2c_put
( uint8_t *s,
  int len,
  void (*callback)(uint8_t *s, int n, int error))
{
  data = s;
  n = len;
  get = 0;
  cback = callback;
  i = 0;
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
  return SUCCESS;
}

#endif
