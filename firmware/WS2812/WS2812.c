#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "WS2812.pio.h"
#include "WS2812.h"

uint32_t LEDS[16];
uint WS2812_sm;

uint WS2812_dma;

/*uint32_t ring[16]={0xffffff,0xff0000,0x00ff00,0x0000ff,0x000000,0xffff00,0xff00ff,0x00ffff,
0xffffff,0xff0000,0x00ff00,0x0000ff,0x000000,0xffff00,0xff00ff,0x00ffff,};*/
uint32_t ring[16]={0x0f0f0f,0,0,0,0x0f0000,0,0,0, 0x000f00,0,0,0,0x00000f};
uint32_t WS2812_data[16];


void WS2812_refresh(uint r){
    for (int i = 0; i < NUM_PIXELS; ++i) {
        WS2812_data[i] = (ring[ (i+r*16/(1024*16)) % NUM_PIXELS])<<8;
    }
    //start DMA
    dma_channel_transfer_from_buffer_now(WS2812_dma, WS2812_data, NUM_PIXELS);
}

void WS2812_init() {

    WS2812_sm = pio_claim_unused_sm(WS2812_PIO, true);
    uint offset = pio_add_program(WS2812_PIO, &ws2812_program);
        
	ws2812_program_init(WS2812_PIO, WS2812_sm, offset, WS2812_PIN, 800000);

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