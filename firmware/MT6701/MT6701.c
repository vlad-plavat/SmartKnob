#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "spi_read.pio.h"
#include "MT6701.h"

void MT6701_init(uint32_t* angle) {

    
    //configure state machine
    PIO pio = MT6701_PIO;
    uint sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &spi_read_program);

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

    //start state machine
	spi_read_init(pio, sm, offset, MT6701_CLKDIV, MT6701_NBITS, MT6701_SCK, MT6701_CSN, MT6701_DIN);

}