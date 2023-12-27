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

#include "../HX711/HX711.h"
#include "images/smartknob_image.h"
#include "font.h"

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

uint16_t lineBuffer[240];

static __force_inline void drawRectangle(int16_t x, int16_t y, uint8_t h, uint8_t w, uint16_t color){
    h--;w--;//function calculates inclusive limits
    int16_t top = y<0?0:y;
    int16_t left = x<0?0:x;
    int16_t right = (x+w)>=WIDTH?WIDTH-1:(x+w);
    int16_t bottom = (y+h)>=HEIGHT?HEIGHT-1:(y+h);
    if(top>=bottom) return;
    if(left>=right) return;
    for(register int line = top; line<=bottom; line++){
        uint8_t maxcol = 239-cuts[line];
        register uint16_t* lineptr = frame[line]-cuts[line];
        register int col = left<cuts[line]?cuts[line]:left;
        register int rgt = right>maxcol?maxcol:right;
        for(; col<=rgt; col++){
            lineptr[col] = color;
        }
    }
}

static __force_inline void drawImage(int16_t x, int16_t y, uint8_t h, uint8_t w, void *image){
    h--;w--;//function calculates inclusive limits
    int16_t top = y<0?0:y;
    int16_t left = x<0?0:x;
    int16_t right = (x+w)>=WIDTH?WIDTH-1:(x+w);
    int16_t bottom = (y+h)>=HEIGHT?HEIGHT-1:(y+h);
    if(top>=bottom) return;
    if(left>=right) return;
    register uint16_t* imgLineptr = image;
    imgLineptr += (w+1)*h-(top-y)*(w+1);
    for(register int line = top; line<=bottom; line++){
        dma_channel_set_read_addr(GC9A01_dma_buf, imgLineptr, false);
        dma_channel_set_write_addr(GC9A01_dma_buf, lineBuffer, false);
        dma_channel_set_trans_count(GC9A01_dma_buf, (w+1+1)/2, true);

        register uint8_t maxcol = 239-cuts[line];
        register uint16_t* lineptr = frame[line]-cuts[line];
        register int col = left<cuts[line]?cuts[line]:left;
        register int rgt = right>maxcol?maxcol:right;
        register uint8_t imgCol=(col-x);
        while(dma_channel_is_busy(GC9A01_dma_buf));
        for(; col<=rgt; col++){
            if(lineBuffer[imgCol] != 0)
                lineptr[col] = lineBuffer[imgCol];
            imgCol++;
        }
        imgLineptr -= (w+1);
    }
}

static __force_inline void fillScreen(uint16_t color){
    memset(frame[0],color,pixcount*2);
}

static __force_inline void printLine(int16_t x, int16_t y, uint16_t maxW, uint16_t scrolled, char *s, uint16_t color){
    register const char *p;
    register uint8_t charLine=0;
    for(register int line = y; line<y+16*2; line++){//double size
        register uint16_t* lineptr = frame[line]-cuts[line];
        p=s;
        register int16_t frameCol = x;
        register int16_t right = x+maxW;
        register int16_t scrolleft = scrolled;
        while(*p){
            register uint8_t charIndex = (*p) - ' ';//first char
            if(charIndex <= 127){
                register unsigned char charWidth = widtbl_f16[charIndex];
                if(scrolleft > charWidth){ scrolleft -= charWidth; p++; continue;}
                register uint8_t charCol=scrolleft;
                scrolleft=0;
                if(charWidth <= 8){
                    register uint8_t charData = chrtbl_f16[charIndex][charLine];
                    register uint8_t mask = 0x80>>charCol;
                    for(; charCol<charWidth; charCol++){
                        if(frameCol >= right) break;
                        if(charData & mask){
                            lineptr[frameCol] = color;//double size
                            lineptr[frameCol+1] = color;
                        }
                        frameCol+=2;
                        mask = mask>>1;
                    }
                }else{
                    register uint16_t charData = ((uint16_t*)(chrtbl_f16[charIndex]))[charLine];
                    charData = __builtin_bswap16(charData);
                    register uint16_t mask = 0x8000>>charCol;
                    for(; charCol<charWidth; charCol++){
                        if(frameCol >= right) break;
                        if(charData & mask){
                            lineptr[frameCol] = color;//double size
                            lineptr[frameCol+1] = color;
                        }
                        frameCol+=2;
                        mask = mask>>1;
                    }
                }
            }
            p++;
        }
        if(line%2==1)//double size
            charLine++;
    }
}

static __force_inline uint16_t lengthOf(const char *s){
    const char *p = s;
    register uint16_t width = 0;
    while(*p){
        register uint8_t charIndex = (*p) - ' ';//first char
        if(charIndex <= 127){
            width += widtbl_f16[charIndex];
        }
        p++;
    }
    return width;
}

void __not_in_flash_func(GC9A01_run)(){
    while(1){
        
        uint32_t bri=(sin(10.f*time_us_32()/1000000)+1)/2*1023;
		pwm_set_gpio_level(GC9A01_BLCTRL,(bri*bri/1024)/2+128);

       /* #define dist(x1,y1,x2,y2) (sqrt(((float)x1-x2)*((float)x1-x2) + ((float)y1-y2)*((float)y1-y2)))
        
		#define ARG time_us_32()/4
		#define CLAMP(X) ((X>239)?239:(X<0)?0:X)*/

        //render
        unsigned long render_start_time = time_us_32();
        
        /*for(register int i=120-32;i<120+32;i++){
            register uint16_t* lineptr = frame[i]-cuts[i];
            for(register int j=120-32;j<120+32;j++){
                lineptr[j]=cnt;
            }
        }*/
        static int x=120-64,y=120-64;
        fillScreen((*knob_angle)>>6);

        int16_t Yval = (Ytilt>0?-Ytilt/32:-Ytilt/16);
        int16_t Xval = -Xtilt/20+25;
        if(!(abs(-Xtilt/24)>64 || abs(Yval)>64)){
            Xval = Yval = 0;
        }
        if ( Xval > 127 ) Xval=127;
        if ( Yval > 127 ) Yval=127;
        if ( Xval < -128) Xval=-128;
        if ( Yval < -128) Yval=-128;

        x += Xval/32;
        y += Yval/32;

        //drawRectangle(x,y,64,64, cnt);
        //drawImage(x,y,128,128, smartknob_image_data);
        printLine(32,120, 150, 150*(*knob_angle)/16/1024, "Denisuc MicWtz",0xff00);
        
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