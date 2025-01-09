#ifndef __onewire_h__
#define __onewire_h__

void onewire_init(void);
int onewire_reset(void);
int onewire_readBit(void);
void onewire_write(int v);
int onewire_read();
void onewire_select(unsigned char *rom);
void onewire_skip();
void onewire_powerDown();
int onewire_powerUp();
void onewire_resetSearch();
int onewire_search(unsigned char *addr);
unsigned char onewire_crc8(unsigned char *d, int len);

#define ONEWIRE_PARASITIC_POWER 1
#define ONEWIRE_SEARCH_ENABLE   0

#endif
