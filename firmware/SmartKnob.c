#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <pico/bootrom.h>
#include <math.h>


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
            //sprintf(buf,"%7ld %7ld %7ld %7d\n", (Xdma<<8)>>18, (Ydma<<8)>>18, (Pdma<<8)>>18, num_ovfl);
            sprintf(buf,"%7ld %7ld %7ld\n", Xtilt, Ytilt, Press);
            //sprintf(buf,"%7d %7d %7d\n", dbgint(), 0, 0);
            tud_cdc_n_write(0, buf, strlen(buf));
            tud_cdc_write_flush();
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
#define MOTOR_FRIC 3
#define MOTOR_UNIFORM 4
#define MOTOR_CUSTOM 5
#define FLAG_START 0x01
#define FLAG_STOP  0x02
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
            Motor_set_mode_detents(0);
        }else if(buffer[2] == MOTOR_FRIC){
            Motor_set_mode_friction(buffer[3]);
        }else if(buffer[2] == MOTOR_UNIFORM){
            float offset = *((float*)(&buffer[4]));
            Motor_set_mode_detents_offset(buffer[3], offset);
            float emin=*((float*)(&buffer[8])), emax=*((float*)(&buffer[12]));
            if(!isnan(emin) && !isnan(emax) && !isinf(emin) && !isinf(emax))
                Motor_add_endstops_to_mode(emin, emax);
        }else if(buffer[2] == MOTOR_CUSTOM){
            float offset = *((float*)(&buffer[4]));
            //max 14 floats remain; array begins at buffer[8]
            float *detents_array = (float*)(&buffer[8]);
            if(buffer[3]>14) buffer[3] = 14;
            Motor_set_mode_custom_detents_offset(buffer[3], detents_array, offset);
        }
        return;
    }
    if(buffer[0] == MESSAGE_LCD){
        static uint8_t in_transfer = 0;
        static int param_index = -1;
        static uint8_t params_present;
        
        //char buf[500];
        //buf[0]='\0';
        uint8_t flags = buffer[2];
        uint8_t elem_count = buffer[1];
        if(elem_count > 15)elem_count = 15;
        if(flags & FLAG_START){
            if(in_transfer){
                //an error occurred; force finish prevvious transfer
                while(param_index<8){
                    param_index++;
                    multicore_fifo_push_blocking(0);
                }
                multicore_fifo_push_blocking(SUBMIT_LIST);
            }
            multicore_fifo_push_blocking(START_EDIT);
            in_transfer = 1;
            param_index = -1;
        }
        
        for(int i=0; i<elem_count && in_transfer; i++){
            uint32_t data = *((uint32_t*)&(buffer[(i+1)*4]));
            if(param_index == -1){
                params_present = data>>24;
                multicore_fifo_push_blocking(data&0x00FFFFFF);
                param_index++;
            }else{
                multicore_fifo_push_blocking(data);
                param_index++;
            }
            while((param_index>=0 && param_index<8) && ((params_present & (1<<param_index)) == 0)){
                //if parameter is not present sent a 0
                multicore_fifo_push_blocking(0);
                param_index++;
            }
            if(param_index == 8){
                param_index = -1;
            }
        }
        if((flags & FLAG_STOP) && in_transfer == 1){
            multicore_fifo_push_blocking(SUBMIT_LIST);
        }
        /*sprintf(buf+strlen(buf),"%d\n", param_index);
        tud_cdc_n_write(0, buf, strlen(buf));
        tud_cdc_write_flush();*/
        
                    /*#define psh multicore_fifo_push_blocking
        if(buffer[1]==15){
            psh(START_EDIT);
            psh(FILL_SCREEN);
            psh(knob_angle);psh(0);psh(0);psh(0);psh(0);psh(0);psh(0);psh(0);
            psh(DRAW_CIRCLE);
            psh(120);psh(120);psh(knob_angle<<8);psh(0xF0F0);
            
        }else{
            psh(0);psh(0);psh(0);psh(0);
            psh(SUBMIT_LIST);
        }*/
       
        while(multicore_fifo_rvalid())
            multicore_fifo_pop_blocking();
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