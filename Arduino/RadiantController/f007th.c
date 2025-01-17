#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"

#include "f007th.h"

//#define DEBUG_F007TH

#define SAMPLE_BYTES 128
static uint8_t sample_buffer[SAMPLE_BYTES];
static uint8_t sample_wrPtr, sample_wrPtr_bit;
static uint8_t sample_rdPtr;

// protos for functions from weathermon code below
static void init(void);
static void processStream(uint8_t d);

// call at least every 5 mSec
static uint8_t initComplete = 0;
void f007th_poll(void)
{
  if (!initComplete) {
    sample_wrPtr = 0;
    sample_wrPtr_bit = 0;
    sample_rdPtr = 0;
    init();
    initComplete = 1;
  } else {
    uint8_t n = sample_wrPtr - sample_rdPtr;
#ifdef DEBUG_F007TH
    if (n>128) debug_puts("f: overflow\n");
#endif
    while (n) {
      processStream(sample_buffer[ sample_rdPtr & (SAMPLE_BYTES-1) ]);
      sample_rdPtr ++;
      n--;
    }
  }
}

// call every 100 usec (10 kHz) with value of GPIO pin connected to 433MHz receiver
void f007th_sample(unsigned sample)
{
  if (initComplete) {
    if (sample)
      sample_buffer[ sample_wrPtr & (SAMPLE_BYTES-1) ] |= (1<<sample_wrPtr_bit);
    else
      sample_buffer[ sample_wrPtr & (SAMPLE_BYTES-1) ] &= ~(1<<sample_wrPtr_bit);
    sample_wrPtr_bit++;
    if (sample_wrPtr_bit==8) {
      sample_wrPtr_bit=0;
      sample_wrPtr++;
    }
  }
}

//
// ==========================================================================================================
// ==========================================================================================================
//

// Derived from https://github.com/adilosa/weathermon

// F007-TH Data Encoding Format

// •RF communication at 434MHz
// •Uses Manchester Encoding ( '1' represented by high --> low transition)
// •bit time = 1ms
// •195 bit frame, 65 bit packet repeated 3 times

// •frames sent every ~1 min (varies by channel)
// ◦Map of channel id to trans interval: {1: 53s, 2: 57s, 3: 59s, 4: 61s, 5: 67s, 6: ??s, 7: 71s, 8: 73s}
// ◦Source: github.com/gr-ambient. Ch 6 interval missing there too.

// •Packet Format
// ◦header: all 1s, with final 01 before data starts
// ◦8 bit sensor ID: F007th = Ox45
// ◦8 bit random ID: resets each time the battery is changed
// ◦1 bit low battery indicator: 1 ==> low battery
// ◦3 bit channel number: channel 1-8 as binary 0-7, selectable on F007th unit using dip switches on back
// ◦12 bit temperature data:
//     To Farenheit: subtract 400 and divide by 10 e.g. 010001001011 => 1099, (1099-400) / 10 = 69.9F
//     To Celsius: subtract 720 and multiply by 0.0556 e.g. (1099 - 720) * 0.0556 = 21.1C
// ◦8 bit humidity data: relative humidity as integer
// ◦8 bit checksum: cracked by Ron (BaronVonSchnowzer on Arduino forum)
// ◦padding: some 0s to fill out packet
//
//
// Sample Data:
// 0        1        2        3        4        5        6        7
// FD       45       4F       04       4B       0B       52       0
// 0   1    2   3    4   5    6   7    8   9    A   B    C   D    E
// 11111101 01000101 01001111 00000100 01001011 00001011 01010010 0000
// header   sensorid randomid BCh#temp_perature humidity checksum xxxx
//                            ^ battery, channel number
//
//

#define PACKET_BYTES 6
static uint8_t packet[PACKET_BYTES];
static int8_t packetBits;

// checksum code from BaronVonSchnowzer at
// http://forum.arduino.cc/index.php?topic=214436.15
static uint8_t checksum(uint8_t length, uint8_t *buff)
{
  uint8_t mask = 0x7C;
  uint8_t checksum = 0x64;
  uint8_t data;
  uint8_t byteCnt;
  
  for (byteCnt=0; byteCnt < length; byteCnt++) {
    uint8_t bitCnt;
    data = buff[byteCnt];
    for (bitCnt=0; bitCnt<8; bitCnt++) {
      uint8_t bit;
      // Rotate mask right
      bit = mask & 1;
      mask = (mask >> 1) | (mask << 7);
      if (bit) {
	mask ^= 0x18;
      }
      
      // XOR mask into checksum if data bit is 1
      if (data & 0x80) {
	checksum ^= mask;
      }
      data <<= 1;
    }
  }
  return checksum;
}

static void record_sensor_data()
{
  uint8_t ch = ((packet[2] & 0x70) / 16) + 1;
  uint8_t data_type = packet[0];
  int16_t tempx10 = ((packet[2] & 0x7) * 256 + packet[3]) - 400;
  uint8_t hum = packet[4]; 
  uint8_t low_bat = (packet[2]>>7) & 1;
  uint8_t check_byte = packet[PACKET_BYTES-1];
  uint8_t check = checksum(PACKET_BYTES-1, &packet[0]);
  if (check != check_byte ||
      data_type != 0x45 ||
      ch < 1 ||
      ch > 8 ||
      hum > 100 ||
      tempx10 < -200 ||
      tempx10 > 1200 ) {
    return;
  }
  f007th_callback(ch, tempx10, hum, low_bat);
}


#ifdef EXTRACTION_SIMPLE

#define HEADER_BITS 10
static uint8_t nHeaderBits;

//
// In order to not lock out other tasks while doing F007TH packet search and processing, we use an approach
// which samples the 433MHz signal in interrupt routine, collects samples, and does batch stream processing
// to search for and process packets.
//

//
// The F007TH uses Manchester encoding at a rate of 1 bit per 1 mSec.  We need to oversample the F007TH
// signal in order to reliably decode packets.
//
//                   |     bit time      |
//
//      1 1 0 0 0 0 0 1 1 1 1 1 0 0 0 0 0 1 1 1 1 1 0 0 0 0 0 1 1 1 1 1 0 0
//                             *                 x x x
//                          detected         search window
//                         transition           
//
// We use 10 samples per bit.  Once we detect a transition representing a Manchester encoded bit, we
// define a search window in which to look for the transition associated with the next Manchester
// encoded bit.  Ideally this would occur exactly 10 samples after the current transition.  However,
// we expect there to be some variation so allow for an uncertainty of 1 sample, so we have a search
// window of 3 potential transition locations.  In order to check for transitions in these 3
// locations, we need a window of 4 bits into the sample stream.
//

#define SAMPLES_PER_BIT        (10)
#define NEXT_BIT_UNCERTAINTY   (1)
#define WINDOW_SIZE            (1 + 2*NEXT_BIT_UNCERTAINTY)
#define WINDOW_BITS            (WINDOW_SIZE + 1)

// We maintain a buffer of up to 32 samples of F007TH stream
static uint32_t sbuf_d;
static int8_t sbuf_bits;
#define SBUF_ADVANCE(x)	sbuf_d >>= (x);	sbuf_bits -= (x)

// This function is used to look for transitions at the head of the sample stream.  The function
// inspects WINDOW_BITS samples of the stream.

static uint8_t findTransition(void)
{
  uint32_t window = sbuf_d;
  uint32_t pattern;
  uint8_t n;

  // adjust stream so we only need to look for 1->0 transition
  if ((window & 1)==0)
    window = ~window;
  
  // limit stream to WINDOW_BITS bits
  n = WINDOW_BITS;
  pattern = (1 << WINDOW_BITS) - 1;
  window &= pattern;

  // look for the 1->0 transition
  while(n) {
    pattern >>= 1;
    n--;
    if (window==pattern)
      break;
  }

  // return the position of the transition
  //    0 - no transition found
  //    1 - transition at bit0->bit1
  //    2 - transition at bit1->bit2
  //   ...
  
  return n;
}

// We advance through the F007TH sample stream using 3 different types of moves.  If we find no
// transition in the window, we simply advance the window.  If we find a transition which does not
// match our target direction, we advance past the transition.  Finally, if we find a valid
// Manchester bit, we advance the window to look for the next Manchester bit.  Define macros here
// for these three types of moves.

#define SBUF_ADVANCE_noTransition    SBUF_ADVANCE(WINDOW_SIZE)
#define SBUF_ADVANCE_pastTransition  SBUF_ADVANCE(n)
#define SBUF_ADVANCE_nextBit         SBUF_ADVANCE( (SAMPLES_PER_BIT - NEXT_BIT_UNCERTAINTY) + (n - 1) )

// In the Manchester encoding scheme a 1->0 transition encodes a "1" and a 0->1 transition encodes
// a "0".  If we find a transition at the head of the stream, the decoded value of the Manchester bit
// associeated with the transition will equal the value head bit of the stream.
#define MANCHESTER_BIT (sbuf_d&1)


typedef enum {
  STATE_search,
  STATE_header,
  STATE_packet
} STATE_t;

static STATE_t state;

static void processStream(uint8_t d)
{
  uint8_t n;
  
  // add incoming bits to our sample buffer ... take care to handle the case of a negative remainder
  if (sbuf_bits<0) {
    sbuf_d = d >> (-sbuf_bits);
  } else {
    sbuf_d |= d << sbuf_bits;
  }
  sbuf_bits += 8;

  //
  // keep on processing the stream while we have enough sample bits to perform transition detection
  //
  
  while (sbuf_bits >= WINDOW_BITS) {

    n = findTransition();
    
    switch(state) {

    case STATE_search:
      if (n) {
	// found transition
	if (MANCHESTER_BIT==1) {
	  // found "1", perhaps beginning of header
	  nHeaderBits = 1;
	  SBUF_ADVANCE_nextBit;
	  state = STATE_header;
	} else {
	  // found "0" ... advance past transition and try again
	  SBUF_ADVANCE_pastTransition;
	}
      } else {
	// no transition found ... advance window
	SBUF_ADVANCE_noTransition;
      }
      break;

    case STATE_header:
      if (n) {
	// found transition
	if (MANCHESTER_BIT==1) {
	  // found "1", increment header bit count
	  nHeaderBits++;
	  SBUF_ADVANCE_nextBit;
	} else {
	  // found "0"
	  if (nHeaderBits > HEADER_BITS) {
	    // we've gotten enough header bits, start packet reception
	    packetBits = -1;
	    SBUF_ADVANCE_nextBit;
	    state = STATE_packet;
	  } else {
	    // not enough header bits ... advance past transition and go back to search
	    SBUF_ADVANCE_pastTransition;
	    state = STATE_search;
	  }
	}
      } else {
	// no transition found ... advance window and go back to search
	SBUF_ADVANCE_noTransition;
	state = STATE_search;
      }
      break;

    case STATE_packet:
      if (n) {
	// found transition
	if (packetBits>=0)
	  packet[packetBits/8] = (packet[packetBits/8]<<1) | MANCHESTER_BIT;
	packetBits++;
	if (packetBits==PACKET_BYTES*8) {
	  record_sensor_data(); 
	  // advance past transition and begin search for next packet
	  SBUF_ADVANCE_pastTransition;
	  state = STATE_search;
	} else {
	  SBUF_ADVANCE_nextBit;
	}
      } else {
	// no transition found ... advance window and go back to search
	SBUF_ADVANCE_noTransition;
	state = STATE_search;
      }
      break;

    }

  }
}

static void init(void)
{
  sbuf_bits = 0;
  state = STATE_search;
}

#else

#define HEADER_BITS 10
static uint8_t nHeaderBits;

//
// In order to not lock out other tasks while doing F007TH packet search and processing, we use an approach
// which samples the 433MHz signal in interrupt routine, collects samples, and does batch stream processing
// to search for and process packets.
//

//
// The F007TH uses Manchester encoding at a rate of 1 bit per 1 mSec.  We need to oversample the F007TH
// signal in order to reliably decode packets.
//
//                   |     bit time      |
//
//      1 1 0 0 0 0 0 1 1 1 1 1 0 0 0 0 0 1 1 1 1 1 0 0 0 0 0 1 1 1 1 1 0 0
//                             *                 x x x
//                          detected         search window
//                         transition           
//
// We use 10 samples per bit.  Once we detect a transition representing a Manchester encoded bit, we
// define a search window in which to look for the transition associated with the next Manchester
// encoded bit.  Ideally this would occur exactly 10 samples after the current transition.  However,
// we expect there to be some variation so allow for an uncertainty of 1 sample, so we have a search
// window of 3 potential transition locations.  In order to check for transitions in these 3
// locations, we need a window of 4 bits into the sample stream.
//

#define SAMPLES_PER_BIT        (10)
#define NEXT_BIT_UNCERTAINTY   (1)
#define WINDOW_SIZE            (1 + 2*NEXT_BIT_UNCERTAINTY)
#define WINDOW_BITS            (WINDOW_SIZE + 1)

// We maintain a buffer of up to 32 samples of F007TH stream
static uint32_t sbuf_d;
static int8_t sbuf_bits;
#define SBUF_ADVANCE(x)	sbuf_d >>= (x);	sbuf_bits -= (x)

// ideal Manchester "1" will return +64
// ideal Manchester "0" will return -64
#define ENERGY_THRESHOLD 40
static int8_t findEdgeEnergy(uint8_t i)
{
  int8_t energy = 0;
  uint32_t window = sbuf_d >> i;
  if (window & 0x01) energy += 8; else energy -= 8;
  if (window & 0x02) energy += 8; else energy -= 8;
  if (window & 0x04) energy += 8; else energy -= 8;
  if (window & 0x08) energy += 8; else energy -= 8;
  if (window & 0x10) energy -= 8; else energy += 8;
  if (window & 0x20) energy -= 8; else energy += 8;
  if (window & 0x40) energy -= 8; else energy += 8;
  if (window & 0x80) energy -= 8; else energy += 8;
  return energy;
}

static uint8_t findSimpleManchesterOne(uint8_t i)
{
  uint32_t window = sbuf_d >> i;
  return ((window & 0x7e) == (0x0e));
}

typedef enum {
  STATE_search,
  STATE_header,
  STATE_packet
} STATE_t;

static STATE_t state;

#ifdef DEBUG_F007TH
static char dbg_s[64+5], *dbg_ss; // +5 for "r: \n\0"
#endif

static void processStream(uint8_t d)
{
  uint8_t max_position;
  int8_t em, e, ep;
  
  // add incoming bits to our sample buffer ... take care to handle the case of a negative remainder
  if (sbuf_bits<0) {
    sbuf_d = d >> (-sbuf_bits);
  } else {
    sbuf_d |= ((uint32_t) d) << sbuf_bits;
  }
  sbuf_bits += 8;

#if 0

  // This section of code can be enabled to capture raw samples from the 433MHz receiver and
  // forward them out the Nano UART, thru the USB to serial converter, to the host.  However,
  // in order to do so a few system changes are required to provided adequate bandwidth.
  // These changes are:
  //
  // (1) Change UART BAUD_RATE in uart.c to 57600.  Since this single UART is used to
  //     both RX from the thermal controller and TX to the host, changing the rate away
  //     from the 2400 used by the thermal controller will bring this link down.
  //
  // (2) In thermal.c, define DISABLE_THERMAL to stop thermal communication.
  //
  // (3) In transmit.c, define DISABLE_TRANSMIT to stop data link to host.  Now we will just
  //     have raw 433MHz data to the host.
  //
  // (4) If needed, disable debug printing by removing and defines of the form DEBUG_*.
  //
  
  while (sbuf_bits >= 10) {
    uint8_t c;
    c = sbuf_d & 0x1f;
    c += (c<16) ? 'a' : ('A' - 16);
    *dbg_ss++ = c;
    SBUF_ADVANCE(5);
    c = sbuf_d & 0x1f;
    c += (c<16) ? 'a' : ('A' - 16);
    *dbg_ss++ = c;
    SBUF_ADVANCE(5);
    *dbg_ss = 0;
    if (strlen(dbg_s)==64+3) {
      *dbg_ss++='\n';
      *dbg_ss=0;
      debug_puts(dbg_s);
      dbg_s[3] = 0;
      dbg_ss = &dbg_s[3];
    }
  }

#else
  
  //
  // keep on processing the stream while we have enough sample bits to perform transition detection
  //
  
  while (sbuf_bits >= 2*SAMPLES_PER_BIT) {

    switch(state) {

    case STATE_search:
      if (findSimpleManchesterOne(1)) {
	// found "1", perhaps beginning of header
	nHeaderBits = 1;
	SBUF_ADVANCE(SAMPLES_PER_BIT);
	state = STATE_header;
      } else {
	SBUF_ADVANCE(1);
      }
      break;

    case STATE_header:
    case STATE_packet:
      em = findEdgeEnergy(0);
      e  = findEdgeEnergy(1);
      ep = findEdgeEnergy(2);
      if (abs(e) >= abs(em)) {
	if (abs(e) >= abs(ep)) {
	  // e is max
	  max_position = 1;
	} else {
	  // ep is max
	  max_position = 2;
	  e = ep;
	}
      } else {
	if (abs(em) >= abs(ep)) {
	  // em is max
	  max_position = 0;
	  e = em;
	} else {
	  // ep is max
	  max_position = 2;
	  e = ep;
	}
      }

      switch(state) {

      case STATE_header:
	if (e>ENERGY_THRESHOLD) {
	  // found "1", increment header bit count
	  nHeaderBits++;
#ifdef DEBUG_F007TH
	  *dbg_ss++='h';
	  *dbg_ss=0;
#endif
	  SBUF_ADVANCE(SAMPLES_PER_BIT-1+max_position);
	} else if (e<-ENERGY_THRESHOLD && nHeaderBits>HEADER_BITS) {
	  // found "0" after a header
	  packetBits = -1;
	  SBUF_ADVANCE(SAMPLES_PER_BIT-1+max_position);
	  state = STATE_packet;
#ifdef DEBUG_F007TH
	  *dbg_ss++='p';
	  *dbg_ss=0;
#endif
	} else {
	  // no transition or not enough header bits ... go back to search
	  state = STATE_search;
#ifdef DEBUG_F007TH
	  if (strlen(dbg_s)>4) {
	    *dbg_ss++='\n';
	    *dbg_ss=0;
	    debug_puts(dbg_s);
	  }
	  dbg_s[3] = 0;
	  dbg_ss = &dbg_s[3];
#endif
	}
	break;

      case STATE_packet:
	if (e>ENERGY_THRESHOLD || e<-ENERGY_THRESHOLD) {
	  if (packetBits>=0) {
	    packet[packetBits/8] = (packet[packetBits/8]<<1) | (e>0 ? 1 : 0);
#ifdef DEBUG_F007TH
	    *dbg_ss++=e>0 ? '1' : '0';
	    *dbg_ss=0;
#endif
	  }
	  packetBits++;
	  SBUF_ADVANCE(SAMPLES_PER_BIT-1+max_position);
	  if (packetBits==PACKET_BYTES*8) {
#ifdef DEBUG_F007TH
	    *dbg_ss++='E';
	    *dbg_ss++='\n';
	    *dbg_ss=0;
	    debug_puts(dbg_s);
	    dbg_s[3] = 0;
	    dbg_ss = &dbg_s[3];;
#endif
	    record_sensor_data(); 
	    state = STATE_search;
	  }
	} else {
	  // no transition or not enough header bits ... go back to search
	  state = STATE_search;
#ifdef DEBUG_F007TH
	  if (strlen(dbg_s)>4) {
	    *dbg_ss++='\n';
	    *dbg_ss=0;
	    debug_puts(dbg_s);
	  }
	  dbg_s[3] = 0;
	  dbg_ss = &dbg_s[3];
#endif
	}
	break;

	default:
	break;

      }
      break;
    }
  }
#endif
}

static void init(void)
{
  sbuf_bits = 0;
  state = STATE_search;
#ifdef DEBUG_F007TH
  dbg_s[0] = 'f';
  dbg_s[1] = ':';
  dbg_s[2] = ' ';
  dbg_s[3] = 0;
  dbg_ss = &dbg_s[3];
#endif
}

#endif


