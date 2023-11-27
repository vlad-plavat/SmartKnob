#ifndef _WS2812_H
#define _WS2812_H

#define WS2812_PIN 6
#define WS2812_FREQ 800000
#define WS2812_PIO pio0

void WS2812_init();
void WS2812_refresh();

#endif