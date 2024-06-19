#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "Motor.h"
#include <math.h>


uint ph_a, ph_b, ph_c;
int32_t *rotor_angle;
int32_t num_ovfl=0;
float angle_full_rot, angle_full_rot_offset;
float angle;

float ph, ang_speed;
int ang_speed_as_int;

#define CLAMP_TO(X,L) ((X)>L?L:((X)<-L?-L:(X)))
#define CLAMP_FRICTION(X) (CLAMP_TO(X,6))
#define CLAMP_FORCE(X) (CLAMP_TO(X,18))
#define sgn(X) ((X)>=0?1:-1)
#define absf(X) ((X)>0?(X):-(X))
#define DEG2RAD (3.14159/180)

#define ROTOR_CALIB_VALUE (3.35)


#define NUM_MAX_DETENTS 20
int32_t motor_power_max = 299;
int num_detents = 1;
enum Motor_general_modes{MOTOR_NORMAL, MOTOR_VELOCITY, MOTOR_FRICTION}motor_general_mode;
int8_t friction_value = 0;
enum Motor_endstop_modes{MOTOR_ENDSTOPS, MOTOR_NO_ENDSTOPS}motor_endstop_mode;
float endstop_min=-22.5*13, endstop_max=22.5*21;
enum Motor_detent_modes{MOTOR_CUSTOM_DETENTS, MOTOR_UNIFORM_DETENTS}motor_detent_mode; 
float detent_positions[NUM_MAX_DETENTS];
float angle_offset = 0, constant_velocity_force = 0;
float power = 0;
int vibration_time_left_us=0;

float prev_angle = 0;
void calculate_angles(){
    //compute current angle 0-360
    float tmp_angle = 360.0-(*rotor_angle)*360.0/1024/16;
    if(tmp_angle<0)tmp_angle+=360;
    tmp_angle = tmp_angle + ROTOR_CALIB_VALUE;
    if(absf(tmp_angle-angle)>0.1){
        angle = tmp_angle;
    }

    //compute full rotation angle
    if(prev_angle>270 && angle<90) num_ovfl++;
    if(prev_angle<90 && angle>270) num_ovfl--;
    angle_full_rot = angle + 360*num_ovfl;
    angle_full_rot_offset = angle_full_rot - angle_offset;

}

void Motor_task(){
    static uint32_t prev_call_time = 0;
    uint32_t cr_call_time = time_us_32();
    if(cr_call_time - prev_call_time < 500) return;
    
    calculate_angles();
    //compute angular speed
    float deltaAngle = angle - prev_angle;
    float deltaTime = cr_call_time - prev_call_time;
    if(deltaAngle > 180) deltaAngle-=360;
    if(deltaAngle <-180) deltaAngle+=360;
    ang_speed = 1000.0*(deltaAngle)/(deltaTime);
    ang_speed_as_int = ang_speed*1000000;
    prev_angle = angle;
    prev_call_time = cr_call_time;

    //compute force that needs to be applied
    ph=0;
    int no_more_action = 0;
    if(motor_general_mode == MOTOR_FRICTION){
        no_more_action = 1;
        power = motor_power_max;
        if(friction_value > 3) friction_value=3;
        if(friction_value <-50) friction_value=-50;
        
        if(fabs(ang_speed) < 0.1)
            power = motor_power_max*fabs(ang_speed);
        if(friction_value > 0){
            power = motor_power_max*(5.0-fabs(ang_speed))/5;
            if(power<0) power=0;
        }
        ph = ang_speed * friction_value;
    }else if(motor_general_mode == MOTOR_VELOCITY){
        no_more_action = 1;
        power = motor_power_max;
        /*if(ang_speed < 2*180.0/1000){
            constant_velocity_force++;
        }else{
            constant_velocity_force--;
        }
        if(constant_velocity_force > 18)constant_velocity_force = 18;
        if(constant_velocity_force <-18)constant_velocity_force = -18;
        ph = constant_velocity_force;*/
        float target = 3*180.0/1000;
        static float prev_ang_speed;
        float deri = (ang_speed - prev_ang_speed)/(deltaTime);
        prev_ang_speed = ang_speed;
        //float in = (ang_speed - prev_ang_speed)/(cr_call_time - prev_call_time);

        ph = -100*(ang_speed-target) + 200*deri;

    }else if(motor_endstop_mode == MOTOR_ENDSTOPS){
        if(angle_full_rot_offset < endstop_min){
            power = motor_power_max;
            no_more_action = 1;
            ph += CLAMP_TO((endstop_min-angle_full_rot_offset)/5, 12);
            float frict_f = CLAMP_FRICTION(sgn(ang_speed)*(sqrt(absf(ang_speed))/(ang_speed*ang_speed-absf(ang_speed)+1))*20);
            frict_f /= 5;
            //float frict_f = CLAMP_FRICTION(ang_speed*50);
            ph -= frict_f;
        }else if(angle_full_rot_offset > endstop_max){
            power = motor_power_max;
            no_more_action = 1;
            ph -= CLAMP_TO((angle_full_rot_offset-endstop_max)/5, 12);
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

    static uint32_t prev_vibr = 0;
    if(vibration_time_left_us>0){
        ph +=  9*sin((time_us_32()%10000)*1.0/10000*2*3.14159);
        vibration_time_left_us -= time_us_32() - prev_vibr;
    }
    prev_vibr = time_us_32();

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
}

void Motor_set_mode_detents_offset(int detents, int32_t offset){
    num_detents = detents;
    motor_detent_mode = MOTOR_UNIFORM_DETENTS;
    motor_endstop_mode = MOTOR_NO_ENDSTOPS;
    num_ovfl = 0;
    
    angle_offset = 360.0-offset*360.0/1024/16 + ROTOR_CALIB_VALUE;
    calculate_angles();
    motor_general_mode = MOTOR_NORMAL;
}

void Motor_set_mode_detents(int detents){
    Motor_set_mode_detents_offset(detents, *rotor_angle);
}

void Motor_add_endstops_to_mode(float emin, float emax){
    endstop_min = emin;
    endstop_max = emax;
    motor_endstop_mode = MOTOR_ENDSTOPS;
}

void Motor_set_mode_constant_velocity(){
    motor_general_mode = MOTOR_VELOCITY;
}

void Motor_set_mode_friction(int8_t friction){
    motor_general_mode = MOTOR_FRICTION;
    friction_value = friction;
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

    //Motor_set_mode_detents(8,0);
    Motor_set_mode_detents_offset(0,0);

}

void Motor_vibrate(){
    vibration_time_left_us = 50000;
}