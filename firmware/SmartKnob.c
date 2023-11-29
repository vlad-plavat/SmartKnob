#include <stdio.h>
#include "pico/stdlib.h"
#include <pico/bootrom.h>
#include "utils.h"
#include "MT6701.h"
#include "WS2812.h"
#include "HX711.h"
#include "Motor.h"
#include "usb.h"

uint32_t knob_angle;

int main(){
    CHECKBUTTON

    MT6701_init(&knob_angle);
    WS2812_init();
    HX711_init();
    Motor_init(&knob_angle);
    tusb_init();

    while(1){
        CHECK_SERIAL_QUIT
        WS2812_refresh(knob_angle);
        sleep_ms(10);
        HX711_update();
        //printf("%ld %ld %ld\n", Xtilt, Ytilt, Press);
        Motor_task();
        //printf("angle: %f\n", knob_angle*360.0/(16*1024));
        //sleep_ms(50);
        service_usb();
    }
    return 0;
}