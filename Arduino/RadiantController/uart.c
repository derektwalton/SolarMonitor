#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"
#include "uart.h"

// Need 2400 to match thermal controller output
#define UART_BAUD  2400


static char *txptr;
static void (*txcallback)(void);

// USART Data Register Empty
ISR(USART_UDRE_vect)
{
  // send next character if there is one
  if (*txptr) {
    UDR0 = *(txptr++);
  } 

  // otherwise, disable interrupt and
  // execute the callback
  else {
    UCSR0B &= ~_BV(UDRIE0);
    txptr = NULL;
    txcallback();
  }
}

int uart_puts(char *s, void (*callback)(void))
{
  // return error if transmit already queued
  if (txptr)
    return 1;

  // stash away pointer to string and callback function
  txptr = s;
  txcallback = callback;

  // enable data register empty interrupt
  UCSR0B |= _BV(UDRIE0);  

  return 0;
}



static char *rxptr;
static int rxi, rxnum;
static void (*rxcallback)(char *s, int n, int error);

// USART Rx Complete
ISR(USART_RX_vect)
{
  static char clast;
  char c;
  int error = 0;

  if ( UCSR0A & _BV(FE0)  ||    // frame error?
       UCSR0A & _BV(DOR0) ||    // data overrun error?
       UCSR0A & _BV(UPE0) )     // parity error?
    error = 1;

  // read the character
  c = UDR0;

  // add to incoming string if there is still space
  if ( (rxi+1) < rxnum ) {
    if ((clast==0x0a) || (rxi!=0)) {
      rxptr[rxi++] = c;
      rxptr[rxi] = 0; // 0 terminate the string
    }
  }

  if (error) {
    // begin looking for a line again if error
    rxi = 0;
    c = 0;
  } else {
    // if carriage return return string to caller
    if (rxnum && (c==0x0a)) {
      rxnum = 0;
      rxcallback(rxptr, rxi, error);
    }
  }

  clast = c;
}

int uart_gets
( char *s, 
  int len, 
  void(*callback)(char *s, int n, int error) )
{

  // return error if receive already queued
  if (rxnum) 
    return 1;

  // prep receive buffer and callback
  rxptr = s;
  rxcallback = callback;
  rxi = 0;

  // do this last to as rxnum is indicator to ISR 
  // that receive buffer is present
  rxnum = len;

  return 0;
}



#define UART_BAUD_REG ((F_CPU / 16 / UART_BAUD) - 1)
void uart_init(void)
{
  rxptr = NULL;
  rxcallback = NULL;
  rxnum = 0;
  txptr = NULL;
  txcallback = NULL;


  UBRR0L = (unsigned char ) UART_BAUD_REG;
  UBRR0H = (unsigned char ) (UART_BAUD_REG >> 8);

  // set tx enable, rx enable, and enable rx complete interrupt
  UCSR0B = _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0);
}
