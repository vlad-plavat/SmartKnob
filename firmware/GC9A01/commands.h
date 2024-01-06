#ifndef _COMMANDS_H_INCLUDED
#define _COMMANDS_H_INCLUDED

#define START_EDIT  0x01
#define SUBMIT_LIST 0x02

#define DRAW_RECTANGLE 0x03
#define GRADIENT_H 0x04
#define GRADIENT_V 0x04
#define DRAW_IMAGE 0x05
#define ROTATED_SCALED_IMAGE 0x06
#define ROTATED_IMAGE 0x07
#define DRAW_LINE 0x08
#define DRAW_CIRCLE 0x09
#define DRAW_CIRCLE_PART 0x0A
#define DRAW_LINE_ROUNDED 0x0B
#define FILL_SCREEN 0x0C
#define PRINT_LINE 0x0D
#define PRINT_LINE_BIG 0x0E

#define WRITE_STRING 0x10

#define FRAME_SWAPPED 0x10

enum Images{SMART_KNOB_IMG=0, CROSSHAIR_CALIB, LEDRING, BRIGHTNESS_IMG, 
            START_APP_IMG, POWER_IMG, MOUSE_IMG, JOYSTICK_IMG, NUM_PREDEFINED_IMAGES};
typedef struct predefined_image{
    void *image;
    uint8_t w,h;
}predefined_image;

extern predefined_image predefined_image_array[NUM_PREDEFINED_IMAGES];

#endif