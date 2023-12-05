#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <pico/bootrom.h>
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
    CHECKBUTTON

    MT6701_init(&knob_angle);
    WS2812_init();
    HX711_init();
    Motor_init(&knob_angle);
    GC9A01_init();
    tusb_init();
    init_boot_button();

    int millis=0;
    multicore_launch_core1_with_stack(GC9A01_run, (uint32_t*)stack_core1, CORE1_STACK_SIZE);
    while(1){
        WS2812_refresh(knob_angle);
        service_usb();
        check_boot_button();
        if(millis==10){
            millis=0;
            HX711_update();
            //Motor_task();
        }
        sleep_ms(10);
        millis++;
    }
    return 0;
}