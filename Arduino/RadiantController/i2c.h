#ifndef __i2c_h__
#define __i2c_h__

#ifdef __cplusplus
extern "C" {
#endif

void i2c_init(void);
int i2c_get
( uint8_t *s,
  int len,
  void (*callback)(uint8_t *s, int n, int error));
int i2c_put
( uint8_t *s,
  int len,
  void (*callback)(uint8_t *s, int n, int error));

#ifdef __cplusplus
}
#endif

#endif
