#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "HX711.pio.h"
#include "HX711.h"

int32_t Xtilt, Ytilt, Press;
int32_t Xdma, Ydma, Pdma;
uint HX711_X_sm, HX711_Y_sm, HX711_P_sm;
int32_t PressLimit1=3500, PressLimit2=4500;

void HX711_update(){

    int32_t Xm=Xdma, Ym=Ydma, Pm=Pdma;
    if(Xm&0x00800000) Xm |= 0xff000000;
    if(Ym&0x00800000) Ym |= 0xff000000;
    if(Pm&0x00800000) Pm |= 0xff000000;

    Xm/=1024;Ym/=1024;Pm/=1024;
    Xm -= 884;
    Ym -= -1449;
    Pm -= -690;
     
    Xtilt = Xm*0.741 +Ym*0.806 +Pm*0.691;
    Ytilt = Xm*-0.874+Ym*-3.878+Pm*-2.052;
    Press = Xm*-1.212+Ym*4.526 +Pm*6.427;
    Press *= 2;
}

void initOneHX711(uint dat, uint SCK){
    gpio_set_function(dat, GPIO_FUNC_SIO);
	gpio_set_function(SCK, GPIO_FUNC_SIO);

	gpio_set_dir(SCK, GPIO_OUT);
	gpio_set_dir(dat, GPIO_IN);

	gpio_put(SCK, 1);
	sleep_ms(1);
	gpio_put(SCK, 0);
}

void start_DMA_pair_HX711(PIO pio, uint sm, int32_t *dest){

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
                          dest, // write address
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
                          dest, // write address
                          &pio->rxf[sm], // read address
                          DMA_RELOAD_VAL,
                          false); // don't start yet

    //start DMA
    dma_start_channel_mask(1u << dma_rx);
}

void HX711_init(){

    HX711_X_sm = pio_claim_unused_sm(HX711_PIO, true);
    HX711_Y_sm = pio_claim_unused_sm(HX711_PIO, true);
    HX711_P_sm = pio_claim_unused_sm(HX711_PIO, true);
    uint offset = pio_add_program(HX711_PIO, &hx711_program);
	hx711_program_init(HX711_PIO, HX711_X_sm, offset, HX711_X_SCK, HX711_X_DAT);
	hx711_program_init(HX711_PIO, HX711_Y_sm, offset, HX711_Y_SCK, HX711_Y_DAT);
	hx711_program_init(HX711_PIO, HX711_P_sm, offset, HX711_P_SCK, HX711_P_DAT);

   start_DMA_pair_HX711(HX711_PIO, HX711_X_sm, &Xdma);
   start_DMA_pair_HX711(HX711_PIO, HX711_Y_sm, &Ydma);
   start_DMA_pair_HX711(HX711_PIO, HX711_P_sm, &Pdma);
}