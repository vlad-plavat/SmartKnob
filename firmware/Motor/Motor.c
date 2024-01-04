#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "Motor.h"
#include <math.h>


uint ph_a, ph_b, ph_c;
int32_t *rotor_angle;

int motor_dbg_a, motor_dbg_b, motor_dbg_c;
float ph, ang_speed;

#define CLAMP_TO(X,L) ((X)>L?L:((X)<-L?-L:(X)))
#define CLAMP_FRICTION(X) (CLAMP_TO(X,6))
#define CLAMP_FORCE(X) (CLAMP_TO(X,18))
#define sgn(X) ((X)>=0?1:-1)
#define absf(X) ((X)>0?(X):-(X))
#define DEG2RAD (3.14159/180)


int num_detents = 17;
float angle_offset = 0;

void Motor_task(){
    static float prev_angle = 0;
    static uint32_t prev_call_time = 0;
    float angle = 360.0-(*rotor_angle)*360.0/1024/16;//-14.7
    if(angle<0)angle+=360;
    //ph = ph - 38.833;
    angle = angle + 3.35;

    uint32_t cr_call_time = time_us_32();
    ang_speed = 1000*(angle - prev_angle)/(cr_call_time - prev_call_time);
    prev_angle = angle;
    prev_call_time = cr_call_time;
//ph=0;
    float anglim = 0;


    // ph = angle + 18*sin((time_us_32()%50000)*1.0/50000*2*3.14159);
    // pwm_set_both_levels(ph_a,(sin(ph        *DEG2RAD*5)+1.001)*199,0);
    // pwm_set_both_levels(ph_b,(sin((ph+120/5)*DEG2RAD*5)+1.001)*199,0);
    // pwm_set_both_levels(ph_c,(sin((ph+240/5)*DEG2RAD*5)+1.001)*199,0);
    
    for(int i=0; i<num_detents; i++){
        anglim += 360/num_detents;

        if(angle < anglim){
            float dif = angle - (anglim-(360/2/num_detents));
            float detent_f = CLAMP_TO(dif*100/(360/2/num_detents),12);
            ph = angle - detent_f;
            float frict_f = CLAMP_FRICTION(sgn(ang_speed)*(sqrt(absf(ang_speed))/(ang_speed*ang_speed-absf(ang_speed)+1))*20);
            
            ph -= frict_f;
            //ph -= CLAMP_FRICTION(ang_speed*10);
            pwm_set_both_levels(ph_a,(sin(ph        *DEG2RAD*5)+1.001)*199,0);
            pwm_set_both_levels(ph_b,(sin((ph+120/5)*DEG2RAD*5)+1.001)*199,0);
            pwm_set_both_levels(ph_c,(sin((ph+240/5)*DEG2RAD*5)+1.001)*199,0);
            break;
        }
    }
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
    angle_offset = offset*360.0/1024/16;
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
}