#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/flash.h"
#include "Settings.h"
#include "MT6701.h"
#include "Motor.h"
#include <math.h>

# define M_PI 3.14159265358979323846
#define DEG2RAD ((M_PI * 2) / 360)
#define RAD2DEG (360 / (M_PI * 2))
#define FIX_TO_PX(x) (((x)&0x0000ffff)<0x00008000?(x)>>16:((x)>>16)+1)

struct Settings settings;
uint8_t flash_id[8];
const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + SETTINGS_OFFSET);

void read_settings(){
    for(int i=0; i<256; i++){
        settings.filler[i] = flash_target_contents[i];
    }
    flash_get_unique_id(flash_id);
    settings.mode = MOUSE_MODE;
}

void save_settings(){
    flash_range_erase(SETTINGS_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_OFFSET, (uint8_t*)&settings, FLASH_PAGE_SIZE);
}

void fill(uint8_t cnt){
    while(cnt){
        multicore_fifo_push_blocking(0);
        cnt--;
    }
}

#define psh multicore_fifo_push_blocking

void push_string(char *s){
    while(*s){
        multicore_fifo_push_blocking(*s);
        s++;
    }
    multicore_fifo_push_blocking('\0');
}

void settings_menu(){
    /*
    if(settings.mode == MOUSE_MODE)settings.mode = JOYSTICK_MODE;
    else if(settings.mode == JOYSTICK_MODE)settings.mode = SMART_MODE;
    else settings.mode = MOUSE_MODE;*/
    
    enum Images images[6]={JOYSTICK_IMG,CROSSHAIR_CALIB,BRIGHTNESS_IMG,LEDRING,START_APP_IMG,POWER_IMG};

    float knob_offset = knob_angle;
    Motor_set_mode_detents(6, knob_offset);
    knob_offset = 360.0-knob_offset*360.0/1024/16;
    while(1){
        float anglef = 360-(knob_angle*360/16/1024);
        anglef -= knob_offset;
        while(anglef<0)anglef+=360;

        multicore_fifo_push_blocking(START_EDIT);

        multicore_fifo_push_blocking(FILL_SCREEN);
        multicore_fifo_push_blocking(0x1D5E);fill(7);

        for(int i=0; i<6; i++){
            int32_t sinof = sin((anglef+90+i*60)*DEG2RAD)*(64.0*1024);
            int32_t cosof = cos((anglef+90+i*60)*DEG2RAD)*(64.0*1024);
            multicore_fifo_push_blocking(DRAW_IMAGE);
            multicore_fifo_push_blocking(120+FIX_TO_PX(cosof*150));
            multicore_fifo_push_blocking(-30+FIX_TO_PX(sinof*150));
            multicore_fifo_push_blocking(images[i]);multicore_fifo_push_blocking(15<<16);
            multicore_fifo_push_blocking(1<<16);multicore_fifo_push_blocking(1<<15);
            multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(1);
        }
        
        int on_setting;
        float angle_add = anglef + 30;
        if(angle_add>360)angle_add-=360;
        for(on_setting = 1; on_setting<7; on_setting++){
            if(60*on_setting-45 < angle_add && angle_add < 60*on_setting-15){
                break;
            }
        }
        char *s="";
        if(on_setting==1) s = "Mode";
        if(on_setting==2) s = "2";
        if(on_setting==3) s = "3";
        if(on_setting==4) s = "4";
        if(on_setting==5) s = "5";
        if(on_setting==6) s = "6";

        multicore_fifo_push_blocking(WRITE_STRING);
        push_string(s);

        multicore_fifo_push_blocking(PRINT_LINE_BIG);
        multicore_fifo_push_blocking(120-lengthOf(s));
        multicore_fifo_push_blocking(178);
        multicore_fifo_push_blocking(10000);multicore_fifo_push_blocking(0);
        multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0xe2c2);
        multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);

        
        multicore_fifo_push_blocking(SUBMIT_LIST);
        
        while(!multicore_fifo_rvalid()){
            Motor_task();
        }
        multicore_fifo_pop_blocking();
        /*multicore_fifo_push_blocking(START_EDIT);

        multicore_fifo_push_blocking(DRAW_RECTANGLE);
        multicore_fifo_push_blocking(64);multicore_fifo_push_blocking(64);
        multicore_fifo_push_blocking(128);multicore_fifo_push_blocking(128);
        multicore_fifo_push_blocking(0xff00);multicore_fifo_push_blocking(0);
        multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
        
        multicore_fifo_push_blocking(SUBMIT_LIST);
        sleep_ms(100);*/
    }

    save_settings();
}