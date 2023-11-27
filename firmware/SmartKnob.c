#include <stdio.h>
#include "pico/stdlib.h"
#include <pico/bootrom.h>
#include "MT6701.h"
#include "WS2812.h"
#include "utils.h"

uint32_t knob_angle;

int main() {
    CHECKBUTTON
    stdio_init_all();
    while(!stdio_usb_connected());

    MT6701_init(&knob_angle);
    WS2812_init();


    while(1){
        CHECK_SERIAL_QUIT;
        WS2812_refresh(knob_angle);
        sleep_ms(1);
        //printf("angle: %f\n", knob_angle*360.0/(16*1024));
        //sleep_ms(50);
    }
    return 0;
}