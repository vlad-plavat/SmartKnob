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
//SCRATCH X is free since core1 stack is in bank0

uint8_t __attribute__((section (".frameAddress"))) image8K[8192];
uint8_t __attribute__((section (".scratch_x"))) imagae4K[4096];
uint8_t image32K[32768];
uint8_t __attribute__((section (".frameAddress"))) filler[728];

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
#include "images/joystick_img.h"
#include "images/mouse_img.h"
#include "images/cursor_image.h"
#include "images/cursor32.h"
#include "images/cool_s.h"
#include "images/crosshair.h"
#include "images/ledring.h"
#include "images/brightness_img.h"
#include "images/start_app_img.h"
#include "images/power_img.h"
#include "images/font16.h"
#include "images/vinyl_img.h"
#include "images/kick_img.h"
#include "images/snare_img.h"
#include "images/hihat_img.h"
#include "images/clap_img.h"

predefined_image predefined_image_array[NUM_PREDEFINED_IMAGES]={
{.image=smartknob_image_data, .h=smartknob_image_height, .w=smartknob_image_width},
{.image=crosshair_data      , .h=crosshair_height,       .w=crosshair_width},
{.image=ledring_data        , .h=ledring_height,         .w=ledring_width},
{.image=brightness_img_data , .h=brightness_img_height,  .w=brightness_img_width},
{.image=start_app_img_data  , .h=start_app_img_height,   .w=start_app_img_width},
{.image=power_img_data      , .h=power_img_height,       .w=power_img_width},
{.image=mouse_img_data      , .h=mouse_img_height,       .w=mouse_img_width},
{.image=joystick_img_data   , .h=joystick_img_height,    .w=joystick_img_width},
{.image=vinyl_img_data     , .h=vinyl_img_height,       .w=vinyl_img_width},
{.image=kick_img_data      , .h=kick_img_height,        .w=kick_img_width},
{.image=snare_img_data     , .h=snare_img_height,       .w=snare_img_width},
{.image=hihat_img_data     , .h=hihat_img_height,       .w=hihat_img_width},
{.image=clap_img_data      , .h=clap_img_height,        .w=clap_img_width},
{.image=cursor_image_data  , .h=cursor_image_height,    .w=cursor_image_width},
};

#include "draw_functions.h"

int32_t LCD_brightness=1024, LCD_max_brightness=0, no_frame_yet=1;

#define NR_MAX_INSTRUCTIONS 20
typedef struct instruction_t{
    uint8_t instr;
    union{
        struct{
            uint32_t x,y;
            uint32_t arg3,arg4,arg5,arg6,arg7,arg8;
        };
        uint32_t params[8];
    };
    
}instruction_t;

#define NR_STRINGS 8
#define MAX_STRING_LENGTH 256

typedef struct render_data_t{
    int nr_instr;
    instruction_t instr_list[NR_MAX_INSTRUCTIONS];
    char strings[NR_STRINGS][MAX_STRING_LENGTH];
}render_data_t;

render_data_t render_data_array[3]={{.nr_instr=0},{.nr_instr=0},{.nr_instr=0}};

volatile int render_data_read_idx = 0;//where we render from
volatile int render_data_write_idx = 0;//we write at this + 1


static __force_inline void render(){
        unsigned long render_start_time = time_us_32();

        //cnt2 = render_data_array[render_data_read_idx].nr_instr;

        irq_set_enabled(SIO_IRQ_PROC1, false);
        if(render_data_write_idx != render_data_read_idx){
            no_frame_yet = 0;
            //in case both buffers are done
            render_data_read_idx++;
            if(render_data_read_idx >= 3) render_data_read_idx -= 3;
            multicore_fifo_push_blocking(FRAME_SWAPPED);
        }
        irq_set_enabled(SIO_IRQ_PROC1, true);

        //fillScreen(0x2);
        /*if(instruction_list == instr1){
            drawRectangle(64,64,128,128, 0x0a0a);
        }else{
            drawRectangle(64,64,128,128, 0xa0a0);
        }*/
        render_data_t *render_data_current = &render_data_array[render_data_read_idx];

        for(int i=0; i<render_data_current->nr_instr; i++){
            instruction_t ins = render_data_current->instr_list[i];
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
                drawImage(ins.x - ins.arg8*predefined_image_array[ins.arg3].w/2,
                          ins.y - ins.arg8*predefined_image_array[ins.arg3].h/2,
                predefined_image_array[ins.arg3].h, predefined_image_array[ins.arg3].w,
                predefined_image_array[ins.arg3].image);
            }else if(ins.instr == ROTATED_SCALED_IMAGE){
                drawRotatedScaledImage(ins.x - ins.arg8*predefined_image_array[ins.arg3].w/2,
                                        ins.y - ins.arg8*predefined_image_array[ins.arg3].h/2,
                predefined_image_array[ins.arg3].h, predefined_image_array[ins.arg3].w,
                ins.arg4,predefined_image_array[ins.arg3].image, ins.arg5, ins.arg6);
            }else if(ins.instr == ROTATED_IMAGE){
                drawRotatedImage(ins.x - /*ins.arg8*/0*predefined_image_array[ins.arg3].w/2,
                                ins.y - /*ins.arg8*/0*predefined_image_array[ins.arg3].h/2,
                predefined_image_array[ins.arg3].h, predefined_image_array[ins.arg3].w,
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
                uint8_t which = ins.arg5;
                if(which > NR_STRINGS) which = NR_STRINGS - 1;
                printLine(ins.x, ins.y, ins.arg3, ins.arg4, render_data_current->strings[which], ins.arg6, ins.arg7);
            }else if(ins.instr == PRINT_LINE_BIG){
                uint8_t which = ins.arg5;
                if(which > NR_STRINGS) which = NR_STRINGS - 1;
                printLine32(ins.x, ins.y, ins.arg3, ins.arg4, render_data_current->strings[which], ins.arg6, ins.arg7);
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





void __not_in_flash_func(fifo_rcv_interrupt)() {
    static int cr_param_index = 0;//-1 means function value, -2,-3 is for string
    static int in_transfer = 0;
    static char *stringptr = NULL;
    static int chars_written;
    // Just record the latest entry
    while (multicore_fifo_rvalid()){
        uint32_t data = multicore_fifo_pop_blocking();
        int write_loc = render_data_write_idx + 1;
        if(write_loc>=3)write_loc-=3;

        if(in_transfer == 0){
            if(data == START_EDIT){cnt2=1;
                //if the fifo is full we overwrite the last element
                if(write_loc == render_data_read_idx){
                    render_data_write_idx--;
                    write_loc--;
                }
                in_transfer = 1;
                cr_param_index = -1;
                render_data_array[write_loc].nr_instr = 0;
            }
            continue;
        }

        int *nr_instr_local = &render_data_array[write_loc].nr_instr;
        instruction_t *instruction_list_local = render_data_array[write_loc].instr_list;
        char (*strings_local)[MAX_STRING_LENGTH] = render_data_array[write_loc].strings;

        //in transfer
        if(cr_param_index == -1){cnt2=2;
            //new command
            if(data == SUBMIT_LIST){
                if(in_transfer){
                    //should never be equal, since we decrement write index if they
                    //were equal at start
                    if(write_loc != render_data_read_idx){
                        cnt2=0;
            //cnt2++;
                        render_data_write_idx++;
                        if(render_data_write_idx >= 3) render_data_write_idx -= 3;
                    }
                }
                in_transfer = 0;
            }else if(data == WRITE_STRING){
                cr_param_index = -2;
            }else{
                if(*nr_instr_local < NR_MAX_INSTRUCTIONS){
                    instruction_list_local[*nr_instr_local].instr = data;
                }
                cr_param_index++;
            }
            continue;
        }
        
        //param !=-1 and in transfer
        if(cr_param_index >= 0){cnt2=3;
            if(*nr_instr_local < NR_MAX_INSTRUCTIONS){
                instruction_list_local[*nr_instr_local].params[cr_param_index] = data;
            }
            cr_param_index++;
            if(cr_param_index == 8){cnt2=4;
                //next instrucion
                cr_param_index = -1;
                (*nr_instr_local)++;
            }
        }else{
            if(cr_param_index == -2){
                //read string index
                uint8_t which = data;
                if(which > NR_STRINGS) which = NR_STRINGS - 1;
                stringptr = strings_local[which];
                chars_written = 0;
                cr_param_index = -3;
            }else if(cr_param_index == -3){
                stringptr[chars_written++] = data;
                if(chars_written == MAX_STRING_LENGTH){
                    chars_written--;
                    stringptr[chars_written]='\0';
                }
                if(data == '\0'){
                    cr_param_index = -1;
                }
            }
        }

        
    }

    multicore_fifo_clear_irq();
}

void __not_in_flash_func(GC9A01_run)(){
    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, fifo_rcv_interrupt);
    irq_set_enabled(SIO_IRQ_PROC1, true);
    while(1){
        if(multicore_fifo_rvalid()) fifo_rcv_interrupt();//prevent locks
        render();

        //wait
        static int first=1, first_since_frame=2;
        #define DMA_DONE (dma_hw->intr & (1u << GC9A01_dma_dat))
        while(!first && !DMA_DONE){
            //continue;
        }
        if(!first){
            pwm_set_gpio_level(GC9A01_BLCTRL,first_since_frame?0:(LCD_brightness*LCD_brightness/1024)*LCD_max_brightness/1024);
        }
        
        first=0;
        if(!no_frame_yet)
            first_since_frame --;
        if(first_since_frame<0)first_since_frame=0;
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
    no_frame_yet = 1;
    
    uint slice_num = pwm_gpio_to_slice_num(GC9A01_BLCTRL);
	pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.f);
	pwm_config_set_wrap(&config, GC9A01_MAX_BRIGHTNESS);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(GC9A01_BLCTRL,0);
    gpio_set_function(GC9A01_BLCTRL, GPIO_FUNC_PWM);

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
}