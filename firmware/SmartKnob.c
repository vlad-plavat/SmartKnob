#include <stdio.h>
#include "pico/stdlib.h"
#include <pico/bootrom.h>
#include "MT6701.h"
#include "utils.h"

uint32_t knob_angle;

int main() {
    CHECKBUTTON
    stdio_init_all();
    while(!stdio_usb_connected());

    MT6701_init(&knob_angle);

    while(1){
        CHECKOUT;
        printf("angle: %f\n", knob_angle*360.0/(16*1024));
        sleep_ms(50);
    }
    return 0;
}