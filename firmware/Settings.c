#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "hardware/flash.h"
#include "Settings.h"
#include "MT6701.h"
#include "Motor.h"
#include "HX711.h"
#include "WS2812.h"
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
    
    if(settings.LCD_max_brightness == 0) settings.LCD_max_brightness = 1;
    LCD_max_brightness = settings.LCD_max_brightness;
    if(settings.LED_max_brightness == 0) settings.LED_max_brightness = 1;
    LED_max_brightness = settings.LED_max_brightness;
    if(settings.motor_power_max == 0) settings.motor_power_max = 100;
    motor_power_max = settings.motor_power_max;
    flash_get_unique_id(flash_id);
}

void save_settings(){
    multicore_reset_core1();
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

void push_string(uint8_t index, char *s){
    multicore_fifo_push_blocking(WRITE_STRING);
    multicore_fifo_push_blocking(index);
    while(*s){
        multicore_fifo_push_blocking(*s);
        s++;
    }
    multicore_fifo_push_blocking('\0');
}

int in_menu = 0, prev_buf = 0, prev_menu = 0;

void settings_menu(){
    /*
    if(settings.mode == MOUSE_MODE)settings.mode = JOYSTICK_MODE;
    else if(settings.mode == JOYSTICK_MODE)settings.mode = SMART_MODE;
    else settings.mode = MOUSE_MODE;*/
    
    enum Images images[6]={JOYSTICK_IMG,CROSSHAIR_CALIB,BRIGHTNESS_IMG,LEDRING,START_APP_IMG,POWER_IMG};
    enum Images mode_images[3]={JOYSTICK_IMG,MOUSE_IMG,SMART_KNOB_IMG};

    float knob_offset = knob_angle;
    Motor_set_mode_detents(6);
    knob_offset = 360.0-knob_offset*360.0/1024/16;
    uint8_t pressed=0, prev_pressed=0;
    uint32_t press_debounce_time = 0, prev_press_debounce_time = 0;

    while(1){
        uint8_t cr_pressed = (Press > PressLimit2);
        static uint8_t prev_measured_press = 0;
        if(cr_pressed != prev_measured_press)
            press_debounce_time = 0;
        prev_measured_press = cr_pressed;
        if(press_debounce_time <= 20000)
            press_debounce_time += time_us_32()-prev_press_debounce_time;
        prev_press_debounce_time = time_us_32();
        prev_pressed = pressed;
        if(press_debounce_time >= 20000)
            pressed=cr_pressed;
        if(in_menu != prev_menu){
            knob_offset = 360.0-knob_angle*360.0/1024/16;
        }
        float anglef = 360-(knob_angle*360/16/1024);
        anglef -= knob_offset;
        while(anglef<0)anglef+=360;

        if(settings.mode == MOUSE_MODE)images[0]=MOUSE_IMG;
        if(settings.mode == JOYSTICK_MODE)images[0]=JOYSTICK_IMG;
        if(settings.mode == SMART_MODE)images[0]=SMART_KNOB_IMG;

        if(in_menu == 0){
            if(in_menu != prev_menu){
                knob_offset = knob_angle;
                Motor_set_mode_detents(6);
                knob_offset = 360.0-knob_offset*360.0/1024/16;
            }
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
            if(on_setting==MENU_MODE) s = "Mode";
            if(on_setting==MENU_POWER) s = "Motor power";
            if(on_setting==MENU_START) s = "Start";
            if(on_setting==MENU_LED) s = "LED ring";
            if(on_setting==MENU_LCD) s = "LCD screen";
            if(on_setting==MENU_CALIB) s = "Calibrate";

            if(pressed && !prev_pressed && on_setting!=7){
                Motor_vibrate();
                if(on_setting != MENU_CALIB)
                    in_menu = on_setting;
                if(on_setting==MENU_START){
                    break;
                }
            }

            push_string(0,s);
            push_string(5,"Settings");

            multicore_fifo_push_blocking(PRINT_LINE_BIG);
            multicore_fifo_push_blocking(120-lengthOf(s));
            multicore_fifo_push_blocking(178);
            multicore_fifo_push_blocking(10000);multicore_fifo_push_blocking(0);
            multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0xe2c2);
            multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);

            multicore_fifo_push_blocking(PRINT_LINE_BIG);
            multicore_fifo_push_blocking(120-lengthOf("Settings"));
            multicore_fifo_push_blocking(20);
            multicore_fifo_push_blocking(10000);multicore_fifo_push_blocking(0);
            multicore_fifo_push_blocking(5);multicore_fifo_push_blocking(0xe2c2);
            multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);

            multicore_fifo_push_blocking(SUBMIT_LIST);
            
        }else if(in_menu == MENU_MODE){
            if(in_menu != prev_menu){
                knob_offset = knob_angle;
                Motor_set_mode_detents(3);
                knob_offset = 360.0-knob_offset*360.0/1024/16;
            }

            multicore_fifo_push_blocking(START_EDIT);

            multicore_fifo_push_blocking(FILL_SCREEN);
            multicore_fifo_push_blocking(0x1D5E);fill(7);

            for(int i=0; i<3; i++){
                int32_t sinof = sin((anglef+90+i*120)*DEG2RAD)*(64.0*1024);
                int32_t cosof = cos((anglef+90+i*120)*DEG2RAD)*(64.0*1024);
                multicore_fifo_push_blocking(DRAW_IMAGE);
                multicore_fifo_push_blocking(120+FIX_TO_PX(cosof*120));
                multicore_fifo_push_blocking(FIX_TO_PX(sinof*120));
                multicore_fifo_push_blocking(mode_images[i]);multicore_fifo_push_blocking(15<<16);
                multicore_fifo_push_blocking(1<<16);multicore_fifo_push_blocking(1<<15);
                multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(1);
            }
            
            int on_setting;
            float angle_add = anglef + 60;
            if(angle_add>360)angle_add-=360;
            for(on_setting = 1; on_setting<4; on_setting++){
                if(120*on_setting-90 < angle_add && angle_add < 120*on_setting-30){
                    break;
                }
            }
            char *s="";
            if(on_setting==1) s = "Joystick";
            if(on_setting==2) s = "Smartknob";
            if(on_setting==3) s = "Mouse";


            multicore_fifo_push_blocking(PRINT_LINE_BIG);
            multicore_fifo_push_blocking(120-lengthOf(s));
            multicore_fifo_push_blocking(178);
            multicore_fifo_push_blocking(10000);multicore_fifo_push_blocking(0);
            multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0xe2c2);
            multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);

            push_string(0,s);
            push_string(5,"Mode");
            multicore_fifo_push_blocking(PRINT_LINE_BIG);
            multicore_fifo_push_blocking(120-lengthOf("Mode"));
            multicore_fifo_push_blocking(20);
            multicore_fifo_push_blocking(10000);multicore_fifo_push_blocking(0);
            multicore_fifo_push_blocking(5);multicore_fifo_push_blocking(0xe2c2);
            multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);

            multicore_fifo_push_blocking(SUBMIT_LIST);

            if(pressed && !prev_pressed && on_setting != 4){
                Motor_vibrate();
                if(on_setting == 1)settings.mode = JOYSTICK_MODE;
                if(on_setting == 2)settings.mode = SMART_MODE;
                if(on_setting == 3)settings.mode = MOUSE_MODE;
                in_menu = 0;
            }
        }else if(in_menu == MENU_LCD || in_menu == MENU_LED || in_menu == MENU_POWER){
            static float init_value;
            int32_t *value_to_edit = &LCD_max_brightness;
            int32_t max_value=1024;
            uint16_t color=0xffff;
            char *s="";
            if(in_menu == MENU_LCD){value_to_edit = &LCD_max_brightness;max_value=GC9A01_MAX_BRIGHTNESS;
                color=0xfdc0;s="Brightness";}
            if(in_menu == MENU_LED){value_to_edit = &LED_max_brightness;max_value=WS2812_MAX_LED_BRIGHTNESS;
                color=0xffff;s="LED strength";}
            if(in_menu == MENU_POWER){value_to_edit = &motor_power_max;max_value=MOTOR_MAX_POWER_LIMIT;
                color=0xdbc1;s="Motor power";}

            if(in_menu != prev_menu){
                init_value = *value_to_edit;
                knob_offset = knob_angle;
                Motor_set_mode_detents(20);
                Motor_add_endstops_to_mode(-init_value, max_value-init_value);
                knob_offset = 360.0-knob_offset*360.0/1024/16;
            }

            int32_t temp_value = init_value + angle_full_rot_offset;
            if(temp_value > max_value)temp_value=max_value;
            if(temp_value < 5)temp_value=5;
            *value_to_edit = temp_value;
            settings.LCD_max_brightness = LCD_max_brightness;
            settings.LED_max_brightness = LED_max_brightness;
            settings.motor_power_max = motor_power_max;

            multicore_fifo_push_blocking(START_EDIT);

            multicore_fifo_push_blocking(FILL_SCREEN);
            multicore_fifo_push_blocking(0x1D5E);fill(7);

            multicore_fifo_push_blocking(DRAW_IMAGE);
            psh(120);psh(120);psh(images[7-in_menu]);psh(15<<16);psh(1<<16);psh(1<<15);psh(0);psh(1);

            multicore_fifo_push_blocking(DRAW_CIRCLE);
            psh(120);psh(119);psh(20<<16);psh(0x1D5E);
            psh(0);psh(0);psh(0);psh(0);
            multicore_fifo_push_blocking(DRAW_CIRCLE_PART);
            psh(120);psh(119);psh(20<<16);psh(color);
            psh(((*value_to_edit)<<16)/max_value-1);psh(0);psh(0);psh(0);

            push_string(5,s);
            multicore_fifo_push_blocking(PRINT_LINE_BIG);
            psh(120-lengthOf(s));psh(25);psh(10000);psh(0);psh(5);psh(0xe2c2);psh(0);psh(0);

            multicore_fifo_push_blocking(SUBMIT_LIST);

            if(pressed && !prev_pressed){
                Motor_vibrate();
                in_menu = 0;
            }
        }

        
        prev_menu = prev_buf;
        prev_buf = in_menu;
        while(!multicore_fifo_rvalid()){
            Motor_task();
            HX711_update();
            WS2812_refresh(knob_angle);
        }
        multicore_fifo_pop_blocking();
        
    }

    save_settings();
    sleep_ms(1);
    watchdog_enable(1, 1); while(1);
}