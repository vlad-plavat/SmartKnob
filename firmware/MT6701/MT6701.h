#ifndef _MT6701_H
#define _MT6701_H

#define MT6701_SCK 10
#define MT6701_CSN 9
#define MT6701_DIN 8

//divider for state machine
#define MT6701_CLKDIV 6
#define MT6701_NBITS 14
#define MT6701_PIO pio0

//2 DMA channels are used in ping-pong configuration
//therefore, reload value doesn't matter
#define DMA_RELOAD_VAL (0xffffffff-1)

void MT6701_init(uint32_t* angle);

#endif