#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "Motor.h"
#include <math.h>


uint ph_a, ph_b, ph_c;
int32_t *rotor_angle;
int32_t num_ovfl=0;
float angle_full_rot;

int motor_dbg_a, motor_dbg_b, motor_dbg_c;
float ph, ang_speed;

#define CLAMP_TO(X,L) ((X)>L?L:((X)<-L?-L:(X)))
#define CLAMP_FRICTION(X) (CLAMP_TO(X,6))
#define CLAMP_FORCE(X) (CLAMP_TO(X,18))
#define sgn(X) ((X)>=0?1:-1)
#define absf(X) ((X)>0?(X):-(X))
#define DEG2RAD (3.14159/180)

#define ROTOR_CALIB_VALUE (3.35)


#define NUM_MAX_DETENTS 20
int motor_power_max = 299;
int num_detents = 1;
enum Motor_endstop_modes{MOTOR_ENDSTOPS, MOTOR_NO_ENDSTOPS}motor_endstop_mode;
float endstop_min=-22.5*13, endstop_max=22.5*21;
enum Motor_detent_modes{MOTOR_CUSTOM_DETENTS, MOTOR_UNIFORM_DETENTS}motor_detent_mode; 
float detent_positions[NUM_MAX_DETENTS];
float angle_offset = 0;
float power = 0;

void Motor_task(){
    static float prev_angle = 0;
    static uint32_t prev_call_time = 0;
    uint32_t cr_call_time = time_us_32();
    if(cr_call_time - prev_call_time < 500) return;
    //compute current angle 0-360
    float angle = 360.0-(*rotor_angle)*360.0/1024/16;
    if(angle<0)angle+=360;
    angle = angle + ROTOR_CALIB_VALUE;

    //compute full rotation angle
    if(prev_angle>270 && angle<90) num_ovfl++;
    if(prev_angle<90 && angle>270) num_ovfl--;
    angle_full_rot = angle + 360*num_ovfl;

    //compute angular speed
    ang_speed = 1000*(angle - prev_angle)/(cr_call_time - prev_call_time);
    prev_angle = angle;
    prev_call_time = cr_call_time;

    //compute force that needs to be applied
    ph=0;
    int no_more_action = 0;
    
    if(motor_endstop_mode == MOTOR_ENDSTOPS){
        if(angle_full_rot < endstop_min){
            power = motor_power_max;
            no_more_action = 1;
            ph += CLAMP_TO((endstop_min-angle_full_rot)/5, 12);
            float frict_f = CLAMP_FRICTION(sgn(ang_speed)*(sqrt(absf(ang_speed))/(ang_speed*ang_speed-absf(ang_speed)+1))*20);
            frict_f /= 5;
            //float frict_f = CLAMP_FRICTION(ang_speed*50);
            ph -= frict_f;
        }else if(angle_full_rot > endstop_max){
            power = motor_power_max;
            no_more_action = 1;
            ph -= CLAMP_TO((angle_full_rot-endstop_max)/5, 12);
            float frict_f = CLAMP_FRICTION(sgn(ang_speed)*(sqrt(absf(ang_speed))/(ang_speed*ang_speed-absf(ang_speed)+1))*20);
            //float frict_f = CLAMP_FRICTION(ang_speed*50);
            frict_f /= 5;
            ph -= frict_f;
        }
    }
    if(no_more_action == 0){
        if(motor_detent_mode == MOTOR_UNIFORM_DETENTS && num_detents != 0){
            power = motor_power_max;
            float anglim = 360.0/num_detents/2;
            for(int i=0; i<num_detents+1; i++){
                float relative_angle = (angle-angle_offset);
                if(relative_angle<0)relative_angle+=360;

                if(relative_angle < anglim){
                    float dif = relative_angle - (anglim-360.0/num_detents/2);
                    float detent_f = CLAMP_TO(dif*100/(360.0/2/num_detents),12);
                    ph -= detent_f;
                    float frict_f = CLAMP_FRICTION(sgn(ang_speed)*(sqrt(absf(ang_speed))/(ang_speed*ang_speed-absf(ang_speed)+1))*20);
                    //float frict_f = CLAMP_FRICTION(ang_speed*50);
                    if(absf(ang_speed)<0.015*num_detents) frict_f/=(40.0/num_detents);
                    ph -= frict_f;
                    //ph -= CLAMP_FRICTION(ang_speed*10);
                    
                    break;
                }
                anglim += 360.0/num_detents;
            }
        }else if(motor_detent_mode == MOTOR_CUSTOM_DETENTS){

        }
    }

    // ph = angle + 18*sin((time_us_32()%50000)*1.0/50000*2*3.14159);
    // pwm_set_both_levels(ph_a,(sin(ph        *DEG2RAD*5)+1.001)*199,0);
    // pwm_set_both_levels(ph_b,(sin((ph+120/5)*DEG2RAD*5)+1.001)*199,0);
    // pwm_set_both_levels(ph_c,(sin((ph+240/5)*DEG2RAD*5)+1.001)*199,0);
    
    /*for(int i=0; i<num_detents; i++){
        anglim += 360/num_detents;

        if(angle < anglim){
            float dif = angle - (anglim-(360/2/num_detents));
            float detent_f = CLAMP_TO(dif*200/(360/2/num_detents),12);
            ph = angle - detent_f;
            float frict_f = CLAMP_FRICTION(sgn(ang_speed)*(sqrt(absf(ang_speed))/(ang_speed*ang_speed-absf(ang_speed)+1))*20);
            //float frict_f = CLAMP_FRICTION(ang_speed*50);
            ph -= frict_f;
            //ph -= CLAMP_FRICTION(ang_speed*10);
            
            break;
        }
    }*/
    if(ph<0){
        if(ph<-18)ph=-18;
        power *= (ph/(-18.01));
        ph=-18;
    }else if(ph>0){
        if(ph>18)ph=18;
        power *= (ph/18.01);
        ph=18;
    }
    ph += angle;
    pwm_set_both_levels(ph_a,(sin(ph        *DEG2RAD*5)+1.001)*power,0);
    pwm_set_both_levels(ph_b,(sin((ph+120/5)*DEG2RAD*5)+1.001)*power,0);
    pwm_set_both_levels(ph_c,(sin((ph+240/5)*DEG2RAD*5)+1.001)*power,0);
    ph -= angle;
    //ph = angle - CLAMP_FORCE(ang_speed*20);
    // motor_dbg_a = (sin(ph        *DEG2RAD*5)+1.001)*199;
    // motor_dbg_b = (sin((ph+120/5)*DEG2RAD*5)+1.001)*199;
    // motor_dbg_c = (sin((ph+240/5)*DEG2RAD*5)+1.001)*199;

    // pwm_set_both_levels(ph_a,(sin(ph        *DEG2RAD*5)+1.001)*199,0);
    // pwm_set_both_levels(ph_b,(sin((ph+120/5)*DEG2RAD*5)+1.001)*199,0);
    // pwm_set_both_levels(ph_c,(sin((ph+240/5)*DEG2RAD*5)+1.001)*199,0);
    
    // pwm_set_both_levels(ph_a,750,0);
    // pwm_set_both_levels(ph_b,0,0);
    // pwm_set_both_levels(ph_c,750,0);
}

void Motor_set_mode_detents(int detents, int32_t offset){
    num_detents = detents;
    motor_detent_mode = MOTOR_UNIFORM_DETENTS;
    motor_endstop_mode = MOTOR_NO_ENDSTOPS;
    
    angle_offset = 360.0-offset*360.0/1024/16 + ROTOR_CALIB_VALUE;
}

void Motor_init(uint32_t *anglevar) {

    rotor_angle = (int32_t*)anglevar;

    gpio_set_function(Motor_A, GPIO_FUNC_PWM);
	gpio_set_function(Motor_B, GPIO_FUNC_PWM);
	gpio_set_function(Motor_C, GPIO_FUNC_PWM);

    ph_a = pwm_gpio_to_slice_num(Motor_A);
    ph_b = pwm_gpio_to_slice_num(Motor_B);
    ph_c = pwm_gpio_to_slice_num(Motor_C);
    
    pwm_config config = pwm_get_default_config();
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 4);//31.25kHz
    pwm_config_set_wrap(&config, PWM_RESOLUTION);
    // Load the configuration into our PWM slice, and set it running.
    pwm_init(ph_a, &config, true);
    pwm_init(ph_b, &config, true);
    pwm_init(ph_c, &config, true);

    pwm_set_chan_level(ph_a, 0, 0);
    pwm_set_chan_level(ph_b, 0, 0);
    pwm_set_chan_level(ph_b, 0, 0);

    Motor_set_mode_detents(8,0);
}