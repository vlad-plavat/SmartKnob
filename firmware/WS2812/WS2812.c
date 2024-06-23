#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "WS2812.pio.h"
#include "WS2812.h"

int32_t LED_max_brightness;

uint32_t LEDS[16];
uint WS2812_sm;

uint WS2812_dma;

/*uint32_t ring[16]={0xffffff,0xff0000,0x00ff00,0x0000ff,0x000000,0xffff00,0xff00ff,0x00ffff,
0xffffff,0xff0000,0x00ff00,0x0000ff,0x000000,0xffff00,0xff00ff,0x00ffff,};*/
//GRB
uint32_t ring[16]={0xffffff,0,0,0,0xff0000,0,0,0, 0x00ff00,0,0,0,0x0000ff};
uint32_t WS2812_data[16];
bool LED_rotating = true;


void WS2812_set_ring_array(void *arr, bool rotating){
    LED_rotating = rotating;
    uint8_t *array = arr;
    for (int i = 0; i < NUM_PIXELS; ++i) {
        ring[i] = array[i*3+2]|(array[i*3+1]<<8)|(array[i*3]<<16);
    }
}

void WS2812_refresh(uint r){
    static uint32_t prev_call_time = 0;
    uint32_t cr_call_time = time_us_32();
    if(cr_call_time - prev_call_time < 1000) return;
    if(dma_channel_is_busy(WS2812_dma)) return;
    prev_call_time = cr_call_time;
    if(!LED_rotating) r = 0;

    for (int i = 0; i < NUM_PIXELS; ++i) {


        uint32_t col = (ring[ (i+r*16/(1024*16)) % NUM_PIXELS]);
        uint8_t b = col&0xFF, r=(col>>16)&0xFF, g=(col>>8)&0xFF;
        b = (int32_t)b*LED_max_brightness/WS2812_MAX_LED_BRIGHTNESS;
        r = (int32_t)r*LED_max_brightness/WS2812_MAX_LED_BRIGHTNESS;
        g = (int32_t)g*LED_max_brightness/WS2812_MAX_LED_BRIGHTNESS;
        col = b | r<<8 | g<<16;

        WS2812_data[i] = col<<8;
    }
    //start DMA
    dma_channel_transfer_from_buffer_now(WS2812_dma, WS2812_data, NUM_PIXELS);
}

void WS2812_init() {

    WS2812_sm = pio_claim_unused_sm(WS2812_PIO, true);
    uint offset = pio_add_program(WS2812_PIO, &ws2812_program);
        
	ws2812_program_init(WS2812_PIO, WS2812_sm, offset, WS2812_PIN, WS2812_FREQ);

    //configure ping-pong DMA
    WS2812_dma = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(WS2812_dma);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_dreq(&c, pio_get_dreq(WS2812_PIO, WS2812_sm, true));
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    dma_channel_configure(WS2812_dma, &c,
                          &WS2812_PIO->txf[WS2812_sm], // write address
                          WS2812_data, // read address
                          NUM_PIXELS,
                          false); // don't start yet

}