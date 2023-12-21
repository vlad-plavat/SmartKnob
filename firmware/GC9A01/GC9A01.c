#include <stdio.h>
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

static uint GC9A01_sm, GC9A01_offset;
static uint GC9A01_dma_dat, GC9A01_dma_ctrl;
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
unsigned long render_time;
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

void __not_in_flash_func(GC9A01_run)(){
    while(1){
        
        uint32_t bri=(sin(10.f*time_us_32()/1000000)+1)/2*1023;
		pwm_set_gpio_level(GC9A01_BLCTRL,(bri*bri/1024)/2+128);

       /**/ #define dist(x1,y1,x2,y2) (sqrt(((float)x1-x2)*((float)x1-x2) + ((float)y1-y2)*((float)y1-y2)))
        
		#define ARG time_us_32()/4
		#define CLAMP(X) ((X>239)?239:(X<0)?0:X)*/

        //render
        unsigned long render_start_time = time_us_32();
        memset(frame[0],/*0x80+0x7f*(cnt/10%2)*/(*knob_angle)>>6,pixcount*2);
        for(register int i=120-32;i<120+32;i++){
            register uint16_t* lineptr = frame[i]-cuts[i];
            for(register int j=120-32;j<120+32;j++){
                lineptr[j]=cnt;
            }
        }
        
        /*asm volatile(".syntax unified\n"
                    "movs r0, 120-32\n"
                    "0:cmp r0, 120+32\n"
                    "beq 1f\n"

                        "lsls r3, r0, #2 \n"
                        "ldr r2 , [%[frame], r3]\n"
                        "ldrb r5, [%[cuts], r0]\n"
                        "lsls r5, r5, #1 \n"
                        "subs r2, r2, r5\n"

                        "movs r1, 120-32\n"
                        "2:cmp r1, 120+32\n"
                        "beq 3f\n"
                            "lsls r3, r1, #1 \n"
                            "strh %[cnt], [r2, r3]\n"

                        "adds r1, r1, #1\n"
                        "b 2b\n"
                        "3:\n"



                    "adds r0, r0, #1\n"
                    "b 0b\n"
                    "1:\n"
                    ".syntax divided\n"
            :
            : [frame] "l" (frame), [cnt] "l" (cnt), [cuts] "l" (cuts)
            : "r0" , "r1" , "r2" , "r3" , "r5" , "memory", "cc"
        );*/

        render_time = time_us_32() - render_start_time;

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
        LCD_SetPos(0,0,239,239);
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

    gpio_set_function(GC9A01_BLCTRL, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(GC9A01_BLCTRL);
	pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.f);
	pwm_config_set_wrap(&config, 1024);
    pwm_init(slice_num, &config, true);
}