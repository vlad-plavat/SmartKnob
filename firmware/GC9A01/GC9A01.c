#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "GC9A01.pio.h"
#include "GC9A01.h"
#include "GC9A01_helpers.h"
#include "frametype.h"
#include <math.h>
#include <string.h>


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
    return cnt2;
}
float dbgfloat(){
    return fps;
}


#include "images/smartknob_image.h"
#include "images/cursor_image.h"
#include "images/cursor32.h"
#include "images/cool_s.h"
#include "images/font16.h"

predefined_image predefined_image_array[NUM_PREDEFINED_IMAGES]={
{.image=smartknob_image_data, .h=smartknob_image_height, .w=smartknob_image_width},
};

#include "draw_functions.h"

#define NR_MAX_INSTRUCTIONS 20
typedef struct instruction{
    uint8_t instr;
    uint16_t x,y;
    uint32_t arg3,arg4,arg5,arg6,arg7,arg8;
}instruction;
int nr_instr1 = 0, nr_instr2 = 0;
int *nr_instr = &nr_instr1;
instruction instr1[NR_MAX_INSTRUCTIONS],instr2[NR_MAX_INSTRUCTIONS];
instruction *instruction_list = instr1;
volatile uint8_t swap_lists = 0;

static __force_inline void render(){
        unsigned long render_start_time = time_us_32();
        cnt2 = *nr_instr;

        irq_set_enabled(SIO_IRQ_PROC1, false);   
        if(swap_lists){
            swap_lists = 0;
            if(instruction_list == instr1){
                instruction_list = instr2;
                nr_instr = &nr_instr2;
            }else{
                instruction_list = instr1;
                nr_instr = &nr_instr1;
            }
        }
        irq_set_enabled(SIO_IRQ_PROC1, true);

        //fillScreen(0x2);
        /*if(instruction_list == instr1){
            drawRectangle(64,64,128,128, 0x0a0a);
        }else{
            drawRectangle(64,64,128,128, 0xa0a0);
        }*/
        for(int i=0; i<(*nr_instr); i++){
            instruction ins = instruction_list[i];
            if(ins.instr == DRAW_RECTANGLE){
                drawRectangle(ins.x,ins.y,
                                ins.arg3,ins.arg4,
                                ins.arg5);
            }else if(ins.instr == GRADIENT_H){
                drawRectGradientH(ins.x, ins.y, ins.arg3, ins.arg4,
                ins.arg5, ins.arg6);
            }else if(ins.instr == GRADIENT_V){
                drawRectGradientV(ins.x, ins.y, ins.arg3, ins.arg4,
                ins.arg5, ins.arg6);
            }else if(ins.instr == DRAW_IMAGE){
                drawImage(ins.x, ins.y, predefined_image_array[ins.arg3].h, predefined_image_array[ins.arg3].w,
                predefined_image_array[ins.arg3].image);
            }else if(ins.instr == ROTATED_SCALED_IMAGE){
                drawRotatedScaledImage(ins.x, ins.y, predefined_image_array[ins.arg3].h, predefined_image_array[ins.arg3].w,
                ins.arg4,predefined_image_array[ins.arg3].image, ins.arg5, ins.arg6);
            }else if(ins.instr == ROTATED_IMAGE){
                drawRotatedImage(ins.x, ins.y, predefined_image_array[ins.arg3].h, predefined_image_array[ins.arg3].w,
                ins.arg4,predefined_image_array[ins.arg3].image);
            }else if(ins.instr == DRAW_LINE){
                drawRotatedLine(ins.x, ins.y, ins.arg3, ins.arg4,
                ins.arg5, ins.arg6);
            }else if(ins.instr == DRAW_LINE_ROUNDED){
                drawRotatedLineRoundEdges(ins.x, ins.y, ins.arg3, ins.arg4,
                ins.arg5, ins.arg6);
            }else if(ins.instr == DRAW_CIRCLE){
                drawCircleFrac(ins.x, ins.y, ins.arg3, ins.arg4);
            }else if(ins.instr == DRAW_CIRCLE_PART){
                drawPartialCircleFrac(ins.x, ins.y, ins.arg3, ins.arg4, ins.arg5);
            }else if(ins.instr == FILL_SCREEN){
                fillScreen(ins.x);
            }else if(ins.instr == PRINT_LINE){
                printLine(ins.x, ins.y, ins.arg3, ins.arg4, "HELLO", ins.arg6, ins.arg7);
            }else if(ins.instr == PRINT_LINE_BIG){
                printLine32(ins.x, ins.y, ins.arg3, ins.arg4, "HELLO", ins.arg6, ins.arg7);
            }
        }
        /*static int x=120,y=120;
        fillScreen(0x2);

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
        printLine32(x-128,y+32, 150, 50*(*knob_angle)/16/1024, "Text mare",0xff00,1);*/

        
        
        render_time = time_us_32() - render_start_time;
}





void fifo_rcv_interrupt() {
    // Just record the latest entry
    while (multicore_fifo_rvalid()){
        uint32_t data = multicore_fifo_pop_blocking();
        int *nr_instr_local = nr_instr==&nr_instr1?&nr_instr2:&nr_instr1;
        instruction *instruction_list_local = instruction_list==instr1?instr2:instr1;
        if(data == 0x01){
            *nr_instr_local = 0;
        }else if(data == 0x02){
            swap_lists = 1;
        }else{
            if((*nr_instr_local) == NR_MAX_INSTRUCTIONS){
                for(int i=0; i<8; i++)multicore_fifo_pop_blocking();
                return;
            }
            instruction_list_local[*nr_instr_local].instr = data;
            instruction_list_local[*nr_instr_local].x = multicore_fifo_pop_blocking();
            instruction_list_local[*nr_instr_local].y = multicore_fifo_pop_blocking();
            instruction_list_local[*nr_instr_local].arg3 = multicore_fifo_pop_blocking();
            instruction_list_local[*nr_instr_local].arg4 = multicore_fifo_pop_blocking();
            instruction_list_local[*nr_instr_local].arg5 = multicore_fifo_pop_blocking();
            instruction_list_local[*nr_instr_local].arg6 = multicore_fifo_pop_blocking();
            instruction_list_local[*nr_instr_local].arg7 = multicore_fifo_pop_blocking();
            instruction_list_local[*nr_instr_local].arg8 = multicore_fifo_pop_blocking();
            (*nr_instr_local)++;
        }
    }

    multicore_fifo_clear_irq();
}

void __not_in_flash_func(GC9A01_run)(){
    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, fifo_rcv_interrupt);
    irq_set_enabled(SIO_IRQ_PROC1, true);
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