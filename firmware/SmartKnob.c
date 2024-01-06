#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <pico/bootrom.h>


#include "HX711.h"
#include "MT6701.h"
#include "usb.h"
#include "utils.h"
#include "WS2812.h"
#include "Motor.h"
#include "GC9A01.h"
#include "boot_button.h"

#include "Settings.h"
#include "modes.h"

#define CORE1_STACK_SIZE 512

uint8_t stack_core1[CORE1_STACK_SIZE];

int main(){
    set_sys_clock_khz(125000,false);
    init_boot_button();
    read_settings();
    WS2812_init();
    MT6701_init(&knob_angle);
    HX711_init();
    Motor_init(&knob_angle);
    GC9A01_init(&knob_angle);
    multicore_launch_core1_with_stack(GC9A01_run, (uint32_t*)stack_core1, CORE1_STACK_SIZE);
    bool enter_settings = check_button_enter_settings();
    if(enter_settings){
        settings_menu();
        //reset_usb_boot(0,0);
    }
    if(settings.mode == JOYSTICK_MODE) init_as_joystick();
    if(settings.mode == MOUSE_MODE) init_as_mouse();
    if(settings.mode == SMART_MODE) init_as_smartknob();
    tusb_init();
    
    int millis=0;
    while(1){
        WS2812_refresh(knob_angle);
        service_usb();
        
        check_boot_button();
        Motor_task();
        HX711_update();
        if(millis==1000){
            char buf[100];
            sprintf(buf,"%7ld %7ld %7ld\n", Xtilt, Ytilt, Press );
            tud_cdc_n_write(0, buf, strlen(buf));
            tud_cdc_write_flush();
            millis=0;
        }
        sleep_ms(1);
        millis++;
    }
    return 0;
}