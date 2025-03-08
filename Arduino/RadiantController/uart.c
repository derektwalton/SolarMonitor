#include <avr/io.h>
#include <avr/interrupt.h>

#include "defines.h"
#include "uart.h"

#define UART_BAUD  57600

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

  // do this last as rxnum is indicator to ISR 
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


/* *********************************************************** */

//
// Software 2400 baud UART RX
//
// Originally I used the single Arduino UART to both send data to
// UDOO board (TX) and receive data from thermal controller (RX).
// While this worked, it made it cumbersome to re-program this
// radiant controller as I needed to switch a jumper on the board
// before and after programming.  Instead, let's use the existing
// 100 uSec sub-tick to sample the thermal UART signal.  With a
// baud rate of 2400 a bit time is 416 uSec or just over 4 sub-
// ticks.  Hopefully this is enough oversampling to get reliable
// data recovery.
//
// Time(mS)   Sample
// ------------------------------------------------
//  0                   begin of start bit
//  0.208      2.1      middle of start bit
//  0.625      6.3      middle of bit 0
//  1.042     10.4      middle of bit 1
//  1.458     14.6      middle of bit 2
//  1.875     18.8      middle of bit 3
//  2.292     22.9      middle of bit 4
//  2.708     27.1      middle of bit 5
//  3.125     31.3      middle of bit 6
//  3.542     35.4      middle of bit 7
//  3.958     39.6      middle of stop bit
//
//

static char *rxptr_2400;
static int rxi_2400, rxnum_2400;
static void (*rxcallback_2400)(char *s, int n, int error);

int uart2400_gets
( char *s, 
  int len, 
  void(*callback)(char *s, int n, int error) )
{

  // return error if receive already queued
  if (rxnum_2400) 
    return 1;

  // prep receive buffer and callback
  rxptr_2400 = s;
  rxcallback_2400 = callback;
  rxi_2400 = 0;

  // do this last as rxnum is indicator to ISR 
  // that receive buffer is present
  rxnum_2400 = len;

  return 0;
}

static char UDR_2400;

static void uart2400_rx_complete()
{
  static char clast;
  char c;

  // read the character
  c = UDR_2400;

  // add to incoming string if there is still space
  if ( (rxi_2400+1) < rxnum_2400 ) {
    if ((clast==0x0a) || (rxi_2400!=0)) {
      rxptr_2400[rxi_2400++] = c;
      rxptr_2400[rxi_2400] = 0; // 0 terminate the string
    }
  }

  // if carriage return return string to caller
  if (rxnum_2400 && (c==0x0a)) {
    rxnum_2400 = 0;
    rxcallback_2400(rxptr_2400, rxi_2400, 0);
  }

  clast = c;
}

void uart2400_sample(void)
{
  static char sample=0xff;
  char rxwire = PINC & _BV(PC5);
  if (sample==0xff) {
    if (!rxwire) sample=0;
  } else {
    sample++;
    if (sample==6  ||
	sample==10 ||
	sample==15 ||
	sample==19 ||
	sample==23 ||
	sample==27 ||
	sample==31 ||
	sample==35) {
      UDR_2400 = (UDR_2400>>1) | (rxwire ? 0x80 : 0x00);
    }
    if (sample==40) {
      uart2400_rx_complete();
      sample==0xff;
    }
  }
}


