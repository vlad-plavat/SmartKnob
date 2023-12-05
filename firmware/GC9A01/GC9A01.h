#ifndef _GC9A01_H
#define _GC9A01_H

#define GC9A01_RST 22
#define GC9A01_D_C 26
#define GC9A01_BLCTRL 27
#define GC9A01_CLK 18//SPI0
#define GC9A01_CSN 21
#define GC9A01_DAT 19//&20

#define GC9A01_PIO pio1

void __not_in_flash_func(GC9A01_run)();
void GC9A01_init();

#endif