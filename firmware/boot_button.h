#ifndef _BOOT_BUTTON_H_
#define _BOOT_BUTTON_H_

#include <stdint.h>
#include "pico/stdlib.h"

#define BOOT_BUTTON_PIN 11
#define BOOT_DELAY_MS 500

bool prev_rst_state = true;
uint32_t prev_rst_check_time = 0;
uint32_t stable_time_rst = 0;

static inline void init_boot_button(){
    gpio_set_function(BOOT_BUTTON_PIN,GPIO_FUNC_SIO);
    gpio_set_dir(BOOT_BUTTON_PIN, GPIO_IN);
    gpio_set_pulls(BOOT_BUTTON_PIN,true,false);
    sleep_ms(1);
    
    stable_time_rst = 0;
    prev_rst_check_time = time_us_32();
    prev_rst_state = true;
}

static inline void check_boot_button(){
    uint32_t cr_time = time_us_32();
    bool cr_state = gpio_get(BOOT_BUTTON_PIN);
    if(cr_state != prev_rst_state){
        if(stable_time_rst < BOOT_DELAY_MS*1000){
            if(cr_state == 1){
                reset_usb_boot(0,0);
            }else{
                //
            }
        }
        stable_time_rst = 0;
    }else{
        stable_time_rst += cr_time-prev_rst_check_time;
        if(stable_time_rst > 10*BOOT_DELAY_MS*1000){
            stable_time_rst = 10*BOOT_DELAY_MS*1000;
        }
    }

    prev_rst_check_time = cr_time;
    prev_rst_state = cr_state;
}

#endif /* _BOOT_BUTTON_H_ */
