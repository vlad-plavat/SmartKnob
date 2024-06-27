#ifndef _MODES_INCLUDED
#define _MODES_INCLUDED
//initializations for specific device types

void init_as_mouse(){
    multicore_fifo_push_blocking(START_EDIT);
    multicore_fifo_push_blocking(FILL_SCREEN);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(DRAW_IMAGE);
    multicore_fifo_push_blocking(120);multicore_fifo_push_blocking(120);
    multicore_fifo_push_blocking(MOUSE_IMG);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(1);
    multicore_fifo_push_blocking(SUBMIT_LIST);
    
    usb_mode = USB_MOUSE;

    Motor_set_mode_detents_offset(20, 0);
}

void init_as_joystick(){
    multicore_fifo_push_blocking(START_EDIT);
    multicore_fifo_push_blocking(FILL_SCREEN);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(DRAW_IMAGE);
    multicore_fifo_push_blocking(120);multicore_fifo_push_blocking(120);
    multicore_fifo_push_blocking(JOYSTICK_IMG);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(1);
    multicore_fifo_push_blocking(SUBMIT_LIST);

    usb_mode = USB_JOYSTICK;
    Motor_set_mode_detents(0);
}


void init_as_smartknob(){
    multicore_fifo_push_blocking(START_EDIT);
    multicore_fifo_push_blocking(FILL_SCREEN);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(DRAW_IMAGE);
    multicore_fifo_push_blocking(120);multicore_fifo_push_blocking(120);
    multicore_fifo_push_blocking(SMART_KNOB_IMG);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(0);
    multicore_fifo_push_blocking(0);multicore_fifo_push_blocking(1);
    multicore_fifo_push_blocking(SUBMIT_LIST);
    
    usb_mode = USB_SMART;
    Motor_set_mode_detents(0);
}

#endif