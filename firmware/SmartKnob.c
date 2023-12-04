#include <stdio.h>
#include "pico/stdlib.h"
#include <pico/bootrom.h>
#include "HX711.h"
#include "usb.h"
#include "utils.h"
#include "MT6701.h"
#include "WS2812.h"
#include "Motor.h"
#include "GC9A01.h"
#include "boot_button.h"

uint32_t knob_angle;

int main(){
    CHECKBUTTON

    MT6701_init(&knob_angle);
    WS2812_init();
    HX711_init();
    Motor_init(&knob_angle);
    GC9A01_init();
    tusb_init();
    init_boot_button();

    while(1){
        WS2812_refresh(knob_angle);
        sleep_ms(10);
        HX711_update();
        //Motor_task();
        GC9A01_update(dbgprintf, service_usb, knob_angle);
        service_usb();
        check_boot_button();
    }
    return 0;
}