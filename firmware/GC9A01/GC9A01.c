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

uint16_t lineBuffer[240];

static __force_inline void drawRectGradientH(int16_t x, int16_t y, uint8_t h, uint8_t w, uint16_t color1, uint16_t color2){
    uint8_t red1 = color1>>11, red2 = color2>>11;
    uint8_t grn1 = (color1>>5)&0x3f, grn2 = (color2>>5)&0x3f;
    uint8_t blu1 = color1&0x1f, blu2 = color2&0x1f;
    for(int i=0; i<w; i++){
        uint8_t r = red1 + (red2-red1)*(i)/w;
        uint8_t g = grn1 + (grn2-grn1)*(i)/w;
        uint8_t b = blu1 + (blu2-blu1)*(i)/w;
        lineBuffer[i]= (r<<11) | (g<<5) | b;
    }
    h--;w--;//function calculates inclusive limits
    int16_t top = y<0?0:y;
    int16_t left = x<0?0:x;
    int16_t right = (x+w)>=WIDTH?WIDTH-1:(x+w);
    int16_t bottom = (y+h)>=HEIGHT?HEIGHT-1:(y+h);
    if(top>bottom) return;
    if(left>right) return;
    for(register int line = top; line<=bottom; line++){
        uint8_t maxcol = (WIDTH-1)-cuts[line];
        register uint16_t* lineptr = frame[line]-cuts[line];
        register int col = left<cuts[line]?cuts[line]:left;
        register int rgt = right>maxcol?maxcol:right;
        for(; col<=rgt; col++){
            lineptr[col] = lineBuffer[col-left];
        }
    }
}

static __force_inline void drawRectGradientV(int16_t x, int16_t y, uint8_t h, uint8_t w, uint16_t color1, uint16_t color2){
    uint8_t red1 = color1>>11, red2 = color2>>11;
    uint8_t grn1 = (color1>>5)&0x3f, grn2 = (color2>>5)&0x3f;
    uint8_t blu1 = color1&0x1f, blu2 = color2&0x1f;
    for(int i=0; i<w; i++){
        uint8_t r = red1 + (red2-red1)*(i)/w;
        uint8_t g = grn1 + (grn2-grn1)*(i)/w;
        uint8_t b = blu1 + (blu2-blu1)*(i)/w;
        lineBuffer[i]= (r<<11) | (g<<5) | b;
    }
    h--;w--;//function calculates inclusive limits
    int16_t top = y<0?0:y;
    int16_t left = x<0?0:x;
    int16_t right = (x+w)>=WIDTH?WIDTH-1:(x+w);
    int16_t bottom = (y+h)>=HEIGHT?HEIGHT-1:(y+h);
    if(top>bottom) return;
    if(left>right) return;
    for(register int line = top; line<=bottom; line++){
        uint8_t maxcol = (WIDTH-1)-cuts[line];
        register uint16_t* lineptr = frame[line]-cuts[line];
        register int col = left<cuts[line]?cuts[line]:left;
        register int rgt = right>maxcol?maxcol:right;
        for(; col<=rgt; col++){
            lineptr[col] = lineBuffer[line-top];
        }
    }
}

static __force_inline void drawRectangle(int16_t x, int16_t y, uint8_t h, uint8_t w, uint16_t color){
    if(h==0 || w==0) return;
    h--;w--;//function calculates inclusive limits
    int16_t top = y<0?0:y;
    int16_t left = x<0?0:x;
    int16_t right = (x+w)>=WIDTH?WIDTH-1:(x+w);
    int16_t bottom = (y+h)>=HEIGHT?HEIGHT-1:(y+h);
    if(top>bottom) return;
    if(left>right) return;
    for(register int line = top; line<=bottom; line++){
        uint8_t maxcol = (WIDTH-1)-cuts[line];
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

        register uint8_t maxcol = (WIDTH-1)-cuts[line];
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

//16.16 fixed point math
static __force_inline void drawRotatedImageOrLine(int32_t x, int32_t y, int32_t h, int32_t w, int32_t angle, void *image, uint16_t color){
    double anglef = angle/(64.0*1024);
    int32_t horig = h, worig = w;
    uint16_t *img = image;
    int32_t sinof = sin(anglef*DEG2RAD)*(64.0*1024);
    int32_t cosof = cos(anglef*DEG2RAD)*(64.0*1024);
    w--; h--;
    x=x<<16; y=y<<16; w=w<<16; h=h<<16;
    int32_t x1=x, y1=y;
    //drawRectangle(FIX_TO_PX(x1),FIX_TO_PX(y1),3,3,0xffff);
    int32_t x2=((int64_t)w*cosof)>>16; x2+=x;
    int32_t y2=((int64_t)w*sinof)>>16; y2+=y;
    //drawRectangle(FIX_TO_PX(x2),FIX_TO_PX(y2),3,3,0xffff);
    int32_t x3=(((int64_t)w*cosof)>>16) - (((int64_t)h*sinof)>>16); x3+=x;
    int32_t y3=(((int64_t)w*sinof)>>16) + (((int64_t)h*cosof)>>16); y3+=y;
    //drawRectangle(FIX_TO_PX(x3),FIX_TO_PX(y3),3,3,0xffff);
    int32_t x4= - (((int64_t)h*sinof)>>16); x4+=x;
    int32_t y4=   (((int64_t)h*cosof)>>16); y4+=y;
    //drawRectangle(FIX_TO_PX(x4),FIX_TO_PX(y4),3,3,0xffff);

    int32_t xmin = MIN(MIN(x1,x2),MIN(x3,x4)), ymin = MIN(MIN(y1,y2),MIN(y3,y4));
    int32_t xmax = MAX(MAX(x1,x2),MAX(x3,x4)), ymax = MAX(MAX(y1,y2),MAX(y3,y4));
    
    //drawRectangle(FIX_TO_PX(xmin),FIX_TO_PX(ymin),3,3,0xff00);
    //drawRectangle(FIX_TO_PX(xmax),FIX_TO_PX(ymax),3,3,0xff00);

    int32_t lmax = FIX_TO_PX(ymax), colmax = FIX_TO_PX(xmax);
    int32_t lmin = FIX_TO_PX(ymin), colmin = FIX_TO_PX(xmin);

    int16_t top = lmin<0?0:lmin;
    int16_t left = colmin<0?0:colmin;
    int16_t right = colmax>WIDTH?WIDTH-1:colmax;
    int16_t bottom = lmax>HEIGHT?HEIGHT-1:lmax;
    if(top>=bottom) return;
    if(left>=right) return;

    for(int32_t lin = top; lin<=bottom; lin++){
        register uint16_t* lineptr = frame[lin]-cuts[lin];
        int32_t trimmed_left = left<cuts[lin]?cuts[lin]:left;
        int32_t trimmed_right = right>((WIDTH-1)-cuts[lin])?(WIDTH-1)-cuts[lin]:right;
        
        int32_t colorig =  ((((int64_t)(trimmed_left<<16)-x)*cosof)>>16) + ((((int64_t)((lin)<<16)-y)*sinof)>>16);
        int32_t linorig =  ((((int64_t)(trimmed_left<<16)-x)*sinof)>>16) - ((((int64_t)((lin)<<16)-y)*cosof)>>16) + (horig<<16);
        if(image != NULL){
            for(int32_t col = trimmed_left; col<=trimmed_right; col++){
                uint32_t linorig2 = linorig>>16, colorig2 = colorig>>16;
                linorig += sinof;
                colorig += cosof;

                if(linorig2 >= (uint32_t)(horig)) continue;
                if(colorig2 >= (uint32_t)(worig)) continue;
                register uint16_t color = *(img+worig*linorig2+colorig2);
                if(color)
                    lineptr[col] = color;
            }
        }else{
            for(int32_t col = trimmed_left; col<=trimmed_right; col++){
                uint32_t linorig2 = linorig>>16, colorig2 = colorig>>16;
                linorig += sinof;
                colorig += cosof;

                if(linorig2 >= (uint32_t)(horig)) continue;
                if(colorig2 >= (uint32_t)(worig)) continue;
                lineptr[col] = color;
            }
        }
    }
}

static __force_inline void drawRotatedImage(int32_t x, int32_t y, int32_t h, int32_t w, int32_t angle, void *image){
    drawRotatedImageOrLine(x, y, h, w, angle, image, 0);
}

static __force_inline void drawRotatedLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t thickness, uint16_t color){
    float hypot = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    int32_t angle = (acos(1.0*(y2-y1)/hypot)*RAD2DEG)*(64.0*1024);
    int32_t sinof = (1.0*(x2-x1)/hypot)*(64.0*1024);
    int32_t cosof = (1.0*(y2-y1)/hypot)*(64.0*1024);
    if(sinof<0) angle = -angle;

    int32_t deltax = ((int64_t)(thickness<<16)/2*cosof)>>16;
    int32_t deltay = ((int64_t)(thickness<<16)/2*sinof)>>16;

    drawRotatedImageOrLine(x1-FIX_TO_PX(deltax), y1+FIX_TO_PX(deltay), hypot, thickness, -angle, NULL, color);
}


static __force_inline void drawPartialCircleFrac(int32_t x, int32_t y, int32_t radius, uint16_t color, uint16_t fraction){
    int32_t ymin=y-FIX_TO_PX(radius), ymax=y+FIX_TO_PX(radius);
    int32_t y_from_center = -FIX_TO_PX(radius);
    fraction = 0xffff-fraction;
    int32_t frac_limit = FIX_TO_PX(((int64_t)radius*2*fraction)>>16);
    
    for(int32_t line = ymin; line<=ymax; line++, y_from_center++){
        if(line-ymin < frac_limit) continue;
        if(line<0 || line>=HEIGHT) continue;
        float width_squared = (((int64_t)radius*radius)>>(16+16)) - (y_from_center-0.5)*(y_from_center-0.5);
        if(width_squared<=0) continue;
        int32_t line_width = sqrt(width_squared)+0.75;
        
        drawRectangle(x-line_width,line,1,line_width*2, color);//efficient enough; has clipping
    }
}

#define drawCircleFrac(x,y,r,c) {drawPartialCircleFrac((x),(y),(r),(c),0xffff);}

static __force_inline void drawRotatedLineRoundEdges(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t thickness, uint16_t color){
    drawRotatedLine(x1,y1,x2,y2,thickness,color);
    drawCircleFrac(x1,y1, (thickness<<16)/2, color);
    drawCircleFrac(x2,y2, (thickness<<16)/2, color);
}

static __force_inline void fillScreen(uint16_t color){
    memset(frame[0],color,pixcount*2);
}

static __force_inline void printLine(int16_t x, int16_t y, uint16_t maxW, uint16_t scrolled, char *s, uint16_t color, uint8_t italic){
    register const char *p;
    int16_t top = y<0?0:y;
    register uint8_t charLine=top-y;
    int16_t bottom = (y+16)>=HEIGHT?HEIGHT-1:(y+16);
    for(register int line = top; line<bottom; line++){
        register uint16_t* lineptr = frame[line]-cuts[line];
        p=s;
        register int16_t frameCol = !italic?x:x+(top-line)/2-(top-y)/2;
        register int16_t right = x+maxW;
        register int16_t scrolleft = scrolled;
        int16_t limL = cuts[line];
        int16_t limR = (WIDTH-1)-cuts[line];
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
                            if(frameCol >= limL && frameCol <= limR)
                                lineptr[frameCol] = color;
                        }
                        frameCol++;
                        mask = mask>>1;
                    }
                }else{
                    register uint16_t charData = ((uint16_t*)(chrtbl_f16[charIndex]))[charLine];
                    charData = __builtin_bswap16(charData);
                    register uint16_t mask = 0x8000>>charCol;
                    for(; charCol<charWidth; charCol++){
                        if(frameCol >= right) break;
                        if(charData & mask){
                            lineptr[frameCol] = color;
                        }
                        frameCol++;
                        mask = mask>>1;
                    }
                }
            }
            p++;
        }
        charLine++;
    }
}

static __force_inline void printLine32(int16_t x, int16_t y, uint16_t maxW, uint16_t scrolled, char *s, uint16_t color, uint8_t italic){
    register const char *p;
    int16_t top = y<0?0:y;
    register uint8_t charLine=(top-y)/2;
    int16_t bottom = (y+16*2)>=HEIGHT?HEIGHT-1:(y+16*2);
    for(register int line = top; line<bottom; line++){//double size
        register uint16_t* lineptr = frame[line]-cuts[line];
        p=s;
        register int16_t frameCol = !italic?x:x+(top-line)/2-(top-y)/2;
        register int16_t right = x+maxW;
        register int16_t scrolleft = scrolled;
        int16_t limL = cuts[line];
        int16_t limR = (WIDTH-1)-cuts[line];
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
                            if(frameCol >= limL && frameCol <= limR)
                                lineptr[frameCol] = color;//double size
                            if(frameCol+1 >= limL && frameCol+1 <= limR)
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

        //render
        unsigned long render_start_time = time_us_32();
        
        /*for(register int i=120-32;i<120+32;i++){
            register uint16_t* lineptr = frame[i]-cuts[i];
            for(register int j=120-32;j<120+32;j++){
                lineptr[j]=cnt;
            }
        }*/
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

        //drawRectangle(x,y,64,64, cnt);
        //drawRectangle(64,64,128,128, 0xff00);
        //drawRectGradientH(x,y,64,64, 0xff00,0x00ff);
        //drawRectGradientV(x+64,y+64,64,64, 0xff00,0x00ff);
        //drawRectangle(120,120,1,1, 0xffff);
        //drawImage(120,120,cool_s_height, cool_s_width, cool_s_data);
        //drawRotatedImage(x,y,cool_s_height, cool_s_width, -((*knob_angle)*360)<<(16-14), cool_s_data);
        //drawRotatedLine(x,y,cool_s_height, cool_s_width, ((*knob_angle)*360)<<(16-14), 0x00ff);
        //drawRotatedLineRoundEdges(x,y,x+100*sin((*knob_angle*360.0)/16/1024*DEG2RAD), y+100*cos((*knob_angle*360.0)/16/1024*DEG2RAD), 20, 0x00ff);
        drawPartialCircleFrac(x,y,/*100.0*(*knob_angle)*4*/20<<16,0x00ff, (*knob_angle)*4);
        //drawRectangle(120,120,1,1, 0xffff);
        //printLine(x-128,y, 150, 50*(*knob_angle)/16/1024, "Roxilina",0xff00,1);
        //printLine32(x-128,y+32, 150, 50*(*knob_angle)/16/1024, "Roxipod",0xff00,1);
        
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