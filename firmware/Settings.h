#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "GC9A01.h"
#define SETTINGS_OFFSET (2*1024*1024 - 4*1024)

struct Settings{//should use 256 bytes (1 writable FLASH page)
    union{
        struct{
        enum {MOUSE_MODE, JOYSTICK_MODE, SMART_MODE}mode;
        int32_t LCD_max_brightness;
        int32_t LED_max_brightness;
        int32_t motor_power_max;
        };
        uint8_t filler[256];
    };
};

extern uint8_t flash_id[8];
extern struct Settings settings;

#define MENU_MODE 1
#define MENU_POWER 2
#define MENU_START 3
#define MENU_LED 4
#define MENU_LCD 5
#define MENU_CALIB 6

void read_settings();
void save_settings();
void settings_menu();

#endif