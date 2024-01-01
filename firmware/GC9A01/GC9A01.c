#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "GC9A01.pio.h"
#include "GC9A01.h"
#include "GC9A01_helpers.h"
#include "frametype.h"
#include <math.h>
#include <string.h>

#include "../HX711/HX711.h"
#include "images/smartknob_image.h"
#include "images/cursor_image.h"
#include "images/cursor32.h"
#include "images/cool_s.h"
#include "images/font16.h"

# define M_PI 3.14159265358979323846
#define DEG2RAD ((M_PI * 2) / 360)
#define RAD2DEG (360 / (M_PI * 2))
#define FIX_TO_PX(x) (((x)&0x0000ffff)<0x00008000?(x)>>16:((x)>>16)+1)
#ifndef MAX
    #define MIN(x,y) ((x)<(y)?(x):(y))
    #define MAX(x,y) ((x)>(y)?(x):(y))
#endif

static uint GC9A01_sm, GC9A01_offset;
static uint GC9A01_dma_dat, GC9A01_dma_ctrl, GC9A01_dma_buf;
static uint32_t *knob_angle;

uint16_t pixcount=0;

//uint16_t __attribute__((section (".frameAddress"))) frame1[200][200],frame2[240][240];

uint8_t __attribute__((section (".frameAddress"))) filler[8920];

uint16_t * __attribute__((section (".frameAddress"))) control_blocks1[241];
uint16_t * __attribute__((section (".frameAddress"))) control_blocks2[241];
uint16_t **control_blocks;

uint16_t **frame;

//union 
//uint16_t frame1[1][1],frame2[1][1];
int cnt=0;
float fps=0;
unsigned long render_time, cnt2=0;
void *dbgptr(){
    return NULL;
}
void *dbgptr2(){
    return NULL;
}
int dbgint(){
    return render_time;
}
float dbgfloat(){
    return fps;
}

#include "draw_functions.h"

static __force_inline void render(){
        unsigned long render_start_time = time_us_32();
        
        static int x=120,y=120;
        fillScreen(/*(*knob_angle)>>(6+4)*/0x0);

        int16_t Yval = (Ytilt>0?-Ytilt/32:-Ytilt/16);
        int16_t Xval = -Xtilt/20+25;
        if(!(abs(-Xtilt/24)>64 || abs(Yval)>64)){
            Xval = Yval = 0;
        }
        if ( Xval > 127 ) Xval=127;
        if ( Yval > 127 ) Yval=127;
        if ( Xval < -128) Xval=-128;
        if ( Yval < -128) Yval=-128;

        x += Xval/64;
        y += Yval/64;

        drawRectangle(x,y,64,64, cnt);
        drawRectangle(64,64,128,128, 0x0a0a);
        drawRectGradientH(x,y,64,64, 0xff00,0x00ff);
        drawRectGradientV(x-80,y,64,64, 0xff00,0x00ff);
        drawRectangle(120,120,1,1, 0xffff);
        drawImage(120,120,cool_s_height, cool_s_width, cool_s_data);
        drawRotatedImage(x,y,cool_s_height, cool_s_width, -((*knob_angle)*360)<<(16-14), cool_s_data);
        drawRotatedScaledImage(x,y,cool_s_height, cool_s_width, -((*knob_angle)*360)<<(16-14), cool_s_data, 1.5*(64.0*1024), 0.5*(64.0*1024));
        drawRotatedLineRoundEdges(x,y,x+100*sin((*knob_angle*360.0)/16/1024*DEG2RAD), y+100*cos((*knob_angle*360.0)/16/1024*DEG2RAD), 20, 0x00ff);
        drawPartialCircleFrac(x,y,20<<16,0x00ff, (*knob_angle)*4);
        printLine(x-128,y, 150, 50*(*knob_angle)/16/1024, "Ceva text",0xff00,1);
        printLine32(x-128,y+32, 150, 50*(*knob_angle)/16/1024, "Text mare",0xff00,1);
        
        render_time = time_us_32() - render_start_time;
}




void __not_in_flash_func(GC9A01_run)(){
    while(1){
        
        uint32_t bri=(sin(10.f*time_us_32()/1000000)+1)/2*1023;
		pwm_set_gpio_level(GC9A01_BLCTRL,(bri*bri/1024)/2+128);

        render();

        //wait
        static int first=1;
        #define DMA_DONE (dma_hw->intr & (1u << GC9A01_dma_dat))
        while(!first && !DMA_DONE){
            //continue;
        }
        first=0;
        cnt++;
        dma_hw->ints0 = 1u << GC9A01_dma_dat;//reset interrupt
        
        //transfer frame
        gpio_set_function(GC9A01_CLK, GPIO_FUNC_SIO);
        gpio_set_function(GC9A01_DAT, GPIO_FUNC_SIO);

        gpio_put(GC9A01_CSN,1);
        LCD_SetPos(0,0,(WIDTH-1),(HEIGHT-1));
        gpio_put(GC9A01_D_C,1);
        gpio_put(GC9A01_CSN,0); D
        gpio_set_dir(GC9A01_DAT,GPIO_OUT);

        GC9A01_program_init(GC9A01_PIO, GC9A01_sm, GC9A01_offset, GC9A01_CLK, GC9A01_DAT);

        dma_channel_transfer_from_buffer_now(GC9A01_dma_ctrl, control_blocks, 1);

        //swap buffers
        if(frame == frame1){
            frame = frame2;
            control_blocks = control_blocks2;
        }else{
            frame = frame1;
            control_blocks = control_blocks1;
        }

    }
}




void GC9A01_init(uint32_t *k_angle){
    init_frame_buffers();
    for(int i=0; i<240; i++){
        pixcount += 240-2*cuts[i];
        control_blocks1[i] = frame1[i]-cuts[i];
        control_blocks2[i] = frame2[i]-cuts[i];
    }
    control_blocks1[240]=0;
    control_blocks2[240]=0;

    control_blocks = control_blocks1;
    frame = frame1;

    GC9A01_Initial();
    knob_angle = k_angle;
    
    PIO pio = GC9A01_PIO;
    GC9A01_sm = pio_claim_unused_sm(pio, true);
    GC9A01_offset = pio_add_program(pio, &GC9A01_program);

    GC9A01_dma_dat = dma_claim_unused_channel(true);
    GC9A01_dma_ctrl = dma_claim_unused_channel(true);
    GC9A01_dma_buf = dma_claim_unused_channel(true);
    
    dma_channel_config c = dma_channel_get_default_config(GC9A01_dma_dat);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_dreq(&c, pio_get_dreq(GC9A01_PIO, GC9A01_sm, true));
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_chain_to(&c, GC9A01_dma_ctrl);
    channel_config_set_irq_quiet(&c, true);
    dma_channel_configure(GC9A01_dma_dat, &c,
                        &GC9A01_PIO->txf[GC9A01_sm], // write address
                        frame2, // read address
                        240, // element count (each element is of size transfer_data_size)
                        false); // start

    c = dma_channel_get_default_config(GC9A01_dma_ctrl);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);
    channel_config_set_ring(&c, true, 2); // 1 << 2 (=4) byte boundary on write ptr
    dma_channel_configure(GC9A01_dma_ctrl, &c,
                        &dma_hw->ch[GC9A01_dma_dat].al3_read_addr_trig, // write address
                        &control_blocks2[0], // read address
                        1, // element count (each element is of size transfer_data_size)
                        false); // start

    c = dma_channel_get_default_config(GC9A01_dma_buf);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);
    dma_channel_configure(GC9A01_dma_buf, &c,
                        0, // write address
                        0, // read address
                        0, // element count (each element is of size transfer_data_size)
                        false); // start

    gpio_set_function(GC9A01_BLCTRL, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(GC9A01_BLCTRL);
	pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.f);
	pwm_config_set_wrap(&config, 1024);
    pwm_init(slice_num, &config, true);
}