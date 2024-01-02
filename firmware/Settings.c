#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/flash.h"
#include "Settings.h"



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

void settings_menu(){
    /*
    if(settings.mode == MOUSE_MODE)settings.mode = JOYSTICK_MODE;
    else if(settings.mode == JOYSTICK_MODE)settings.mode = SMART_MODE;
    else settings.mode = MOUSE_MODE;*/
    
    while(1){
        multicore_fifo_push_blocking(START_EDIT);

        multicore_fifo_push_blocking(FILL_SCREEN);
        multicore_fifo_push_blocking(0x003);multicore_fifo_push_blocking(0);
        multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
        multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
        multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);

        multicore_fifo_push_blocking(ROTATED_SCALED_IMAGE);
        multicore_fifo_push_blocking(64);multicore_fifo_push_blocking(64);
        multicore_fifo_push_blocking(SMART_KNOB);multicore_fifo_push_blocking(15<<16);
        multicore_fifo_push_blocking(1<<16);multicore_fifo_push_blocking(1<<15);
        multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
        
        multicore_fifo_push_blocking(SUBMIT_LIST);
        
        sleep_ms(100);
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