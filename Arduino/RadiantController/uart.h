#ifndef __uart_h__
#define __uart_h__

#ifdef __cplusplus
extern "C" {
#endif

//#define UART_BAUD  57600
#define UART_BAUD  2400

  int uart_puts
  ( char *s, 
    void (*callback)(void) );
  
  int uart_gets
  ( char *s, 
    int len, 
    void(*callback)(char *s, int n, int error) );
  
  int uart2400_gets
  ( char *s, 
    int len, 
    void(*callback)(char *s, int n, int error) );
  
#define UART2400_GPIO (PINC & _BV(PC5))

  void uart2400_sample(unsigned char rxwire);
  
  void uart_init(void);
  
#ifdef __cplusplus
}
#endif

#endif
