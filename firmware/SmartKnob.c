#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <pico/bootrom.h>
#include "Settings.h"


#include "HX711.h"
#include "usb.h"
#include "utils.h"
#include "MT6701.h"
#include "WS2812.h"
#include "Motor.h"
#include "GC9A01.h"
#include "boot_button.h"

#define CORE1_STACK_SIZE 512

uint32_t knob_angle;
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
    bool enter_settings = check_button_enter_settings();
    if(enter_settings){
        //settings_menu();
        reset_usb_boot(0,0);
    }
    tusb_init();
    
    int millis=0;
    multicore_launch_core1_with_stack(GC9A01_run, (uint32_t*)stack_core1, CORE1_STACK_SIZE);
    while(1){
        WS2812_refresh(knob_angle);
        service_usb();
        char buf[100];
        //sprintf(buf,"%p %p %08x \n",dbgptr(), dbgptr2(), dbgint());
        //sprintf(buf,"%f \n",dbgfloat());
        sprintf(buf,"%u \n",dbgint());
        tud_cdc_n_write(0, buf, strlen(buf));
        
        check_boot_button();
        //Motor_task();
        if(millis==100){
            millis=0;
            HX711_update();

            /*for(int i=0; i<240; i++){
                sprintf(buf,"%p ",((uint16_t **)dbgptr())[i]);
                tud_cdc_n_write(0, buf, strlen(buf));
                service_usb();
                tud_cdc_write_flush();
            }
            tud_cdc_n_write(0, "\n\n\n", 3);*/
        }
        sleep_ms(1);
        millis++;
    }
    return 0;
}