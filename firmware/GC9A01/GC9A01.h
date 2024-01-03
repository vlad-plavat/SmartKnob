#ifndef _GC9A01_H
#define _GC9A01_H

#include "commands.h"

#define GC9A01_RST 22
#define GC9A01_D_C 26
#define GC9A01_BLCTRL 27
#define GC9A01_CLK 18//SPI0
#define GC9A01_CSN 21
#define GC9A01_DAT 19//&20

#define WIDTH 240
#define HEIGHT 240

#define GC9A01_PIO pio1

void __not_in_flash_func(GC9A01_run)();
uint16_t __not_in_flash_func(lengthOf)(const char *s);
void GC9A01_init();
void *dbgptr();
void *dbgptr2();
int dbgint();
float dbgfloat();

#endif