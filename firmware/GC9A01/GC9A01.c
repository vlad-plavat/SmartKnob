#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "GC9A01.pio.h"
#include "GC9A01.h"
#include "GC9A01_helpers.h"
#include <math.h>

static uint GC9A01_sm, GC9A01_offset;
static uint GC9A01_dma_dat;


uint16_t frame1[240][240],frame2[240][240]={
	#include "img.vec"
};


void GC9A01_update(void (*prt)(const char*, ...), void (*usb)(void), uint32_t knob_angle){
        /*uint32_t bri=(sin(10.f*time_us_32()/1000000)+1)/2*1023;
		pwm_set_gpio_level(BLCTRL,(bri*bri/1024)/2+128);*/

        //static uint32_t col = 0xaaaaaaaa;
        //LCD_SetPos(120-50,120-50,120+50,120+50);

        //#define blue 0b1010111011011100u
        #define blue 0b0000000000011111u
        #define dist(x1,y1,x2,y2) (sqrt(((float)x1-x2)*((float)x1-x2) + ((float)y1-y2)*((float)y1-y2)))
        // Initialize the program using the helper function in our .pio file
            //starts and stalls
        //int32_t ballx=120+50*sin(10.0f*time_us_32()/1000000);
        //int32_t bally=120+50*cos(10.0f*time_us_32()/1000000),radius=12;

        /*for(int i=0;i<240;i++){
            for(int j=0;j<240;j++){
                if(i > bally+radius || i<bally-radius || j>ballx+radius || j<ballx-radius)
                    frame1[i][j]=blue;
                else
                    frame1[i][j]=~blue;
                
            }
        }*/
		
		//#define ARG time_us_32()/4
		//register int sinth = sin(10.0f*ARG/1000000)*8192,costh = cos(10.0f*ARG/1000000)*8192;
		register int sinth = sin(knob_angle*360.0/1024/16 *3.14/180)*8192,costh = cos(knob_angle*360.0/1024/16 *3.14/180)*8192;
		#define CLAMP(X) ((X>239)?239:(X<0)?0:X)
        
static int frameready=0;

if(!frameready)
    for(register int y=50;y<190;y++){
        for(register int x=50;x<190;x++){
            frame1[y][x]=
            frame2	[ CLAMP(((x-120)*sinth+(y-120)*costh)/8192+120) ]
                    [ CLAMP(((x-120)*costh-(y-120)*sinth)/8192+120)];
            
                    
            
        }
        usb();
    }
frameready=1;

/*for(int i=bally-radius;i<bally+radius;i++){
            for(int j=ballx-radius;j<ballx+radius;j++){
                frame1[i][j]=blue;
                
            }
        }*/

        static int first=1;

        if(!first && dma_channel_is_busy(GC9A01_dma_dat)){
            return;
        }
        first=0;
        frameready=0;
        

        gpio_set_function(GC9A01_CLK, GPIO_FUNC_SIO);
        gpio_set_function(GC9A01_DAT, GPIO_FUNC_SIO);

        gpio_put(GC9A01_CSN,1);
        LCD_SetPos(0,0,239,239);
        gpio_put(GC9A01_D_C,1);
        gpio_put(GC9A01_CSN,0); D
        gpio_set_dir(GC9A01_DAT,GPIO_OUT);

/*for(int i=0;i<240;i++){
    for(int j=0;j<240;j++){
        
        for(uint16_t m=0x8000;m;m=m>>1){
            gpio_put(GC9A01_CLK,0); gpio_put(GC9A01_DAT,frame2[i][j]&m);
            gpio_put(GC9A01_CLK,1);
        }
        gpio_put(GC9A01_CLK,0);
    }
    usb();
}*/


        GC9A01_program_init(GC9A01_PIO, GC9A01_sm, GC9A01_offset, GC9A01_CLK, GC9A01_DAT);

        /*for(int i=0;i<240;i++){
                for(int j=0;j<240;j++){
                    pio_sm_put_blocking(GC9A01_PIO, GC9A01_sm, frame1[i][j]<<16);
                }
            }*/


        dma_channel_transfer_from_buffer_now(GC9A01_dma_dat, frame1, 240*240);
        
        //col *= 123456+5615613;


        static int cycles = 0;
        static uint64_t time;
        if(cycles%100==0){
			//printf("adr0:%p adr1:%p\n",&frame1[0][0],&frame1[0][1]);
            char buf[100];
            sprintf(buf, "\n%ffps\n\n",1000000*100.0/(time_us_64()-time));
            prt(buf);
            time = time_us_64();
        }
        cycles++;

        //printf("0x%06lx %08lx\n", readFromCmd(RDDID,0x800000), readFromCmd(RDDST,0x80000000));
        
        //sleep_ms(200);
}

void GC9A01_init(){
    
    GC9A01_Initial();

    
    PIO pio = GC9A01_PIO;
    GC9A01_sm = pio_claim_unused_sm(pio, true);
    GC9A01_offset = pio_add_program(pio, &GC9A01_program);

    GC9A01_dma_dat = dma_claim_unused_channel(true);

    
    dma_channel_config c = dma_channel_get_default_config(GC9A01_dma_dat);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_dreq(&c, pio_get_dreq(GC9A01_PIO, GC9A01_sm, true));
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    dma_channel_configure(GC9A01_dma_dat, &c,
                        &GC9A01_PIO->txf[GC9A01_sm], // write address
                        frame1, // read address
                        240*240, // element count (each element is of size transfer_data_size)
                        false); // start

    /*gpio_set_function(BLCTRL, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BLCTRL);

	pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.f);
	pwm_config_set_wrap(&config, 1024);
    pwm_init(slice_num, &config, true);*/
}