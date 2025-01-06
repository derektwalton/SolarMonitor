#ifndef __uart_h__
#define __uart_h__

#ifdef __cplusplus
extern "C" {
#endif

int uart_puts
( char *s, 
 void (*callback)(void) );

int uart_gets
( char *s, 
  int len, 
  void(*callback)(char *s, int n, int error) );

void uart_init(void);

#ifdef __cplusplus
}
#endif

#endif
