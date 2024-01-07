#ifndef _WS2812_H
#define _WS2812_H

#define WS2812_PIN 6
#define WS2812_FREQ 800000
#define WS2812_PIO pio0
#define NUM_PIXELS 16

#define WS2812_MAX_LED_BRIGHTNESS 1024
extern int32_t LED_max_brightness;

void WS2812_init();
void WS2812_refresh(uint r);
void WS2812_set_ring_array(void *arr, bool rotating);

#endif