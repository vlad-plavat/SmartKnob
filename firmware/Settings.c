#include <stdio.h>
#include "Settings.h"
#include "hardware/flash.h"



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
    


    save_settings();
}