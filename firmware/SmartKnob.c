#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <pico/bootrom.h>


#include "HX711.h"
#include "MT6701.h"
#include "usb.h"
#include "utils.h"
#include "WS2812.h"
#include "Motor.h"
#include "GC9A01.h"
#include "boot_button.h"

#include "Settings.h"
#include "modes.h"

#define CORE1_STACK_SIZE 512

uint8_t stack_core1[CORE1_STACK_SIZE];

void handleUsbPacket(uint8_t *buffer, uint8_t buflen);
void fillDescriptor(hid_smartknob_report_t *rep);

int main(){
    set_sys_clock_khz(125000,false);
    init_boot_button();
    read_settings();
    WS2812_init();
    MT6701_init(&knob_angle);
    HX711_init();
    Motor_init(&knob_angle);
    GC9A01_init(&knob_angle);
    multicore_launch_core1_with_stack(GC9A01_run, (uint32_t*)stack_core1, CORE1_STACK_SIZE);
    bool enter_settings = check_button_enter_settings();
    if(enter_settings){
        settings_menu();
        //reset_usb_boot(0,0);
    }
    if(settings.mode == JOYSTICK_MODE) init_as_joystick();
    if(settings.mode == MOUSE_MODE) init_as_mouse();
    if(settings.mode == SMART_MODE) init_as_smartknob();
    usb_packetCallback = handleUsbPacket;
    usb_fill_descriptor_callback = fillDescriptor;
    tusb_init();
    
    int millis=0;
    while(1){
        WS2812_refresh(knob_angle);
        service_usb();
        
        check_boot_button();
        Motor_task();
        HX711_update();
        if(millis==10){
            char buf[100];
            sprintf(buf,"%7ld %7ld %7ld %7f\n", Xtilt, Ytilt, Press, angle_full_rot_offset);
            //tud_cdc_n_write(0, buf, strlen(buf));
            //tud_cdc_write_flush();
            millis=0;
        }
        sleep_ms(1);
        millis++;
    }
    return 0;
}

#define MESSAGE_NOP 0
#define MESSAGE_LED 1
#define MESSAGE_LCD 2
#define MESSAGE_MOT 3
#define MOTOR_OFF 2
#define MOTOR_VEL 1
void handleUsbPacket(uint8_t *buffer, uint8_t buflen){
    if(buflen != 64) return;
    if(buffer[0] == MESSAGE_NOP) return;
    if(buffer[0] == MESSAGE_LED){
        WS2812_set_ring_array(buffer+1, buffer[1+48]);
        return;
    }
    if(buffer[0] == MESSAGE_MOT){
        if(buffer[1])
            Motor_vibrate();
        if(buffer[2] == MOTOR_VEL){
            Motor_set_mode_constant_velocity();
        }else if(buffer[2] == MOTOR_OFF){
            Motor_set_mode_detents(0, 0);
        }
        return;
    }
    if(buffer[0] == MESSAGE_LCD){
        
        return;
    }
}

void fillDescriptor(hid_smartknob_report_t *rep){
    rep->Xtilt = Xtilt;
    rep->Ytilt = Ytilt;
    rep->Press = Press;
    rep->angle = knob_angle;
    rep->speed = ang_speed_as_int;
}