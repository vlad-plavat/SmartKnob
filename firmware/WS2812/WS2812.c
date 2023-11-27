#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "WS2812.pio.h"
#include "WS2812.h"

uint32_t LEDS[16];
uint WS2812_sm;


/*uint32_t ring[16]={0xffffff,0xff0000,0x00ff00,0x0000ff,0x000000,0xffff00,0xff00ff,0x00ffff,
0xffffff,0xff0000,0x00ff00,0x0000ff,0x000000,0xffff00,0xff00ff,0x00ffff,};*/
uint32_t ring[16]={0xffffff,0,0,0,0xff0000,0,0,0, 0x00ff00,0,0,0,0x0000ff};

#define NUM_PIXELS 16

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(WS2812_PIO, WS2812_sm, pixel_grb << 8u);
}

void WS2812_refresh(uint r){
    for (int i = 0; i < NUM_PIXELS; ++i) {
        //put_pixel(ring[(i+time_us_32()/300000)%NUM_PIXELS]);
        put_pixel(ring[ (i+r*16/(1024*16)) % NUM_PIXELS]);
    }
}

void WS2812_init() {

    WS2812_sm = pio_claim_unused_sm(WS2812_PIO, true);
    uint offset = pio_add_program(WS2812_PIO, &ws2812_program);
        
	ws2812_program_init(WS2812_PIO, WS2812_sm, offset, WS2812_PIN, 800000);

   /*

    //configure ping-pong DMA
    const uint dma_rx = dma_claim_unused_channel(true);
    const uint dma_rx2 = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(dma_rx);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, false);
	channel_config_set_chain_to(&c, dma_rx2);
    dma_channel_configure(dma_rx, &c,
                          angle, // write address
                          &pio->rxf[sm], // read address
                          DMA_RELOAD_VAL,
                          false); // don't start yet


    c = dma_channel_get_default_config(dma_rx2);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, false);
	channel_config_set_chain_to(&c, dma_rx);
    dma_channel_configure(dma_rx2, &c,
                          angle, // write address
                          &pio->rxf[sm], // read address
                          DMA_RELOAD_VAL,
                          false); // don't start yet

    //start DMA
    dma_start_channel_mask(1u << dma_rx);
*/
   
}