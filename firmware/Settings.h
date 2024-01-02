#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "GC9A01.h"
#define SETTINGS_OFFSET (2*1024*1024 - 4*1024)

struct Settings{//should use 256 bytes (1 writable FLASH page)
    union{
        struct{
        enum {MOUSE_MODE, JOYSTICK_MODE, SMART_MODE}mode;
        };
        uint8_t filler[256];
    };
};

extern uint8_t flash_id[8];
extern struct Settings settings;

void read_settings();
void save_settings();
void settings_menu();

#endif