#ifndef _GC9A01_HELPERS_H
#define _GC9A01_HELPERS_H

#define D

static inline void __not_in_flash_func(write)(uint8_t data){
    for(uint8_t m=0x80;m;m=m>>1){
            gpio_put(GC9A01_CLK,0); gpio_put(GC9A01_DAT,data&m); D
            gpio_put(GC9A01_CLK,1); D
        }
        gpio_put(GC9A01_CLK,0);
}

static inline void __not_in_flash_func(Write_Cmd)(uint8_t cmd){
    gpio_put(GC9A01_D_C,0);
    gpio_put(GC9A01_CSN,0); D
    gpio_set_dir(GC9A01_DAT,GPIO_OUT);
    write(cmd);
    gpio_put(GC9A01_CSN,1);
}

static inline void __not_in_flash_func(Write_Cmd_Data)(uint8_t data){
    gpio_put(GC9A01_D_C,1);
    gpio_put(GC9A01_CSN,0); D
    gpio_set_dir(GC9A01_DAT,GPIO_OUT);
    write(data);
    gpio_put(GC9A01_CSN,1);
}


static inline void  __not_in_flash_func(Write_Data_U16)(uint16_t y)
{
	unsigned char m,n;
	m=y>>8;
	n=y;
	Write_Cmd_Data(m);
	Write_Cmd_Data(n);
}
static inline void __not_in_flash_func(LCD_SetPos)(unsigned int Xstart,unsigned int Ystart,unsigned int Xend,unsigned int Yend)
{
	Write_Cmd(0x2a);   
	Write_Cmd_Data(Xstart>>8);
	Write_Cmd_Data(Xstart);
 	Write_Cmd_Data(Xend>>8);
	Write_Cmd_Data(Xend);

	Write_Cmd(0x2b);   
	Write_Cmd_Data(Ystart>>8);
	Write_Cmd_Data(Ystart);
	Write_Cmd_Data(Yend>>8);
	Write_Cmd_Data(Yend);
	
  	Write_Cmd(0x2c);//LCD_WriteCMD(GRAMWR);
}

static inline void __not_in_flash_func(LCD_DrawPoint)(unsigned int x,unsigned int y,unsigned int color)
{
	LCD_SetPos(x,y,x,y);
	Write_Data_U16(color);
} 

static inline uint32_t __not_in_flash_func(readFromCmd)(uint8_t cmd, uint32_t startmask){
    gpio_put(GC9A01_CSN,0); D
    gpio_set_dir(GC9A01_DAT,GPIO_OUT);
    gpio_put(GC9A01_D_C,0);
    for(uint8_t m=0x80;m;m=m>>1){
        gpio_put(GC9A01_CLK,0); gpio_put(GC9A01_DAT,cmd&m); D
        gpio_put(GC9A01_CLK,1); D
    }
    gpio_put(GC9A01_CLK,0); D
    gpio_set_dir(GC9A01_DAT,GPIO_IN);
    gpio_put(GC9A01_CLK,1); D

    uint32_t rsp=0;
    for(uint32_t m=startmask;m;m=m>>1){
        gpio_put(GC9A01_CLK,0); sleep_us(1);
        if(gpio_get(GC9A01_DAT))rsp|=m;
        gpio_put(GC9A01_CLK,1); sleep_us(1);
    }
    gpio_put(GC9A01_CLK,0);
    gpio_put(GC9A01_D_C,1); D
    gpio_put(GC9A01_CSN,1);
    return rsp;
}

static inline void GC9A01_Initial(void)
{ 


    gpio_set_function(GC9A01_RST, GPIO_FUNC_SIO);
	if(gpio_get_function(GC9A01_BLCTRL)!=GPIO_FUNC_PWM)
    	gpio_set_function(GC9A01_BLCTRL, GPIO_FUNC_SIO);
    gpio_set_function(GC9A01_D_C, GPIO_FUNC_SIO);
    gpio_set_function(GC9A01_CSN, GPIO_FUNC_SIO);
    gpio_set_function(GC9A01_CLK, GPIO_FUNC_SIO);
    gpio_set_function(GC9A01_DAT, GPIO_FUNC_SIO);

    gpio_set_dir(GC9A01_RST,GPIO_OUT);
    gpio_set_dir(GC9A01_BLCTRL,GPIO_OUT);
    gpio_set_dir(GC9A01_D_C,GPIO_OUT);
    gpio_set_dir(GC9A01_CSN,GPIO_OUT);
    gpio_set_dir(GC9A01_CLK,GPIO_OUT);
    gpio_set_dir(GC9A01_DAT,GPIO_OUT);

    gpio_put(GC9A01_RST,0);
    sleep_ms(1);
    gpio_put(GC9A01_RST,1);
    gpio_put(GC9A01_BLCTRL,1);
    gpio_put(GC9A01_D_C,0);
    gpio_put(GC9A01_CSN,1);
    gpio_put(GC9A01_CLK,0);
    gpio_put(GC9A01_DAT,1);

  	gpio_put(GC9A01_CSN,1);
	sleep_ms(5);
	gpio_put(GC9A01_RST,0);
	sleep_ms(10);
	gpio_put(GC9A01_RST,1);
	sleep_ms(120);  


 //************* Start Initial Sequence **********// 
	Write_Cmd(0xEF);
 
	Write_Cmd(0xEB);
	Write_Cmd_Data(0x14); 
	
    Write_Cmd(0xFE);			 
	Write_Cmd(0xEF); 

	Write_Cmd(0xEB);	
	Write_Cmd_Data(0x14); 

	Write_Cmd(0x84);			
	Write_Cmd_Data(0x40); 

	Write_Cmd(0x85);			
	Write_Cmd_Data(0xFF); 

	Write_Cmd(0x86);			
	Write_Cmd_Data(0xFF); 

	Write_Cmd(0x87);			
	Write_Cmd_Data(0xFF);

	Write_Cmd(0x88);			
	Write_Cmd_Data(0x0A);

	Write_Cmd(0x89);			
	Write_Cmd_Data(0x21); 

	Write_Cmd(0x8A);			
	Write_Cmd_Data(0x00); 

	Write_Cmd(0x8B);			
	Write_Cmd_Data(0x80); 

	Write_Cmd(0x8C);			
	Write_Cmd_Data(0x01); 

	Write_Cmd(0x8D);			
	Write_Cmd_Data(0x01); 

	Write_Cmd(0x8E);			
	Write_Cmd_Data(0xFF); 

	Write_Cmd(0x8F);			
	Write_Cmd_Data(0xFF); 


	Write_Cmd(0xB6);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x00);

	Write_Cmd(0x36);////////////////////ORDER
	Write_Cmd_Data(0x48);//0x10 0x04
  


	Write_Cmd(0x3A);			
	Write_Cmd_Data(0x05); 


	Write_Cmd(0x90);			
	Write_Cmd_Data(0x08);
	Write_Cmd_Data(0x08);
	Write_Cmd_Data(0x08);
	Write_Cmd_Data(0x08); 

	Write_Cmd(0xBD);			
	Write_Cmd_Data(0x06);
	
	Write_Cmd(0xBC);			
	Write_Cmd_Data(0x00);	

	Write_Cmd(0xFF);			
	Write_Cmd_Data(0x60);
	Write_Cmd_Data(0x01);
	Write_Cmd_Data(0x04);

	Write_Cmd(0xC3);			
	Write_Cmd_Data(0x13);
	Write_Cmd(0xC4);			
	Write_Cmd_Data(0x13);

	Write_Cmd(0xC9);			
	Write_Cmd_Data(0x22);

	Write_Cmd(0xBE);			
	Write_Cmd_Data(0x11); 

	Write_Cmd(0xE1);			
	Write_Cmd_Data(0x10);
	Write_Cmd_Data(0x0E);

	Write_Cmd(0xDF);			
	Write_Cmd_Data(0x21);
	Write_Cmd_Data(0x0c);
	Write_Cmd_Data(0x02);

	Write_Cmd(0xF0);   
    Write_Cmd_Data(0x45);
    Write_Cmd_Data(0x09);
 	Write_Cmd_Data(0x08);
  	Write_Cmd_Data(0x08);
 	Write_Cmd_Data(0x26);
 	Write_Cmd_Data(0x2A);

 	Write_Cmd(0xF1);    
 	Write_Cmd_Data(0x43);
 	Write_Cmd_Data(0x70);
 	Write_Cmd_Data(0x72);
 	Write_Cmd_Data(0x36);
 	Write_Cmd_Data(0x37);  
 	Write_Cmd_Data(0x6F);


 	Write_Cmd(0xF2);   
 	Write_Cmd_Data(0x45);
 	Write_Cmd_Data(0x09);
 	Write_Cmd_Data(0x08);
 	Write_Cmd_Data(0x08);
 	Write_Cmd_Data(0x26);
 	Write_Cmd_Data(0x2A);

 	Write_Cmd(0xF3);   
 	Write_Cmd_Data(0x43);
 	Write_Cmd_Data(0x70);
 	Write_Cmd_Data(0x72);
 	Write_Cmd_Data(0x36);
 	Write_Cmd_Data(0x37); 
 	Write_Cmd_Data(0x6F);

	Write_Cmd(0xED);	
	Write_Cmd_Data(0x1B); 
	Write_Cmd_Data(0x0B); 

	Write_Cmd(0xAE);			
	Write_Cmd_Data(0x77);
	
	Write_Cmd(0xCD);			
	Write_Cmd_Data(0x63);		


	Write_Cmd(0x70);			
	Write_Cmd_Data(0x07);
	Write_Cmd_Data(0x07);
	Write_Cmd_Data(0x04);
	Write_Cmd_Data(0x0E); 
	Write_Cmd_Data(0x0F); 
	Write_Cmd_Data(0x09);
	Write_Cmd_Data(0x07);
	Write_Cmd_Data(0x08);
	Write_Cmd_Data(0x03);

	Write_Cmd(0xE8);			
	Write_Cmd_Data(0x34);

	Write_Cmd(0x62);			
	Write_Cmd_Data(0x18);
	Write_Cmd_Data(0x0D);
	Write_Cmd_Data(0x71);
	Write_Cmd_Data(0xED);
	Write_Cmd_Data(0x70); 
	Write_Cmd_Data(0x70);
	Write_Cmd_Data(0x18);
	Write_Cmd_Data(0x0F);
	Write_Cmd_Data(0x71);
	Write_Cmd_Data(0xEF);
	Write_Cmd_Data(0x70); 
	Write_Cmd_Data(0x70);

	Write_Cmd(0x63);			
	Write_Cmd_Data(0x18);
	Write_Cmd_Data(0x11);
	Write_Cmd_Data(0x71);
	Write_Cmd_Data(0xF1);
	Write_Cmd_Data(0x70); 
	Write_Cmd_Data(0x70);
	Write_Cmd_Data(0x18);
	Write_Cmd_Data(0x13);
	Write_Cmd_Data(0x71);
	Write_Cmd_Data(0xF3);
	Write_Cmd_Data(0x70); 
	Write_Cmd_Data(0x70);

	Write_Cmd(0x64);			
	Write_Cmd_Data(0x28);
	Write_Cmd_Data(0x29);
	Write_Cmd_Data(0xF1);
	Write_Cmd_Data(0x01);
	Write_Cmd_Data(0xF1);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x07);

	Write_Cmd(0x66);			
	Write_Cmd_Data(0x3C);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0xCD);
	Write_Cmd_Data(0x67);
	Write_Cmd_Data(0x45);
	Write_Cmd_Data(0x45);
	Write_Cmd_Data(0x10);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x00);

	Write_Cmd(0x67);			
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x3C);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x01);
	Write_Cmd_Data(0x54);
	Write_Cmd_Data(0x10);
	Write_Cmd_Data(0x32);
	Write_Cmd_Data(0x98);

	Write_Cmd(0x74);			
	Write_Cmd_Data(0x10);	
	Write_Cmd_Data(0x85);	
	Write_Cmd_Data(0x80);
	Write_Cmd_Data(0x00); 
	Write_Cmd_Data(0x00); 
	Write_Cmd_Data(0x4E);
	Write_Cmd_Data(0x00);					
	
    Write_Cmd(0x98);			
	Write_Cmd_Data(0x3e);
	Write_Cmd_Data(0x07);

	Write_Cmd(0x35);	
	Write_Cmd(0x21);

	Write_Cmd(0x11);
	sleep_ms(120);
	Write_Cmd(0x29);
	sleep_ms(20);
}

#endif