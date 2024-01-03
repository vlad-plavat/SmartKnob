#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "Motor.h"
#include <math.h>


uint ph_a, ph_b, ph_c;
uint32_t *rotor_angle;

#define DEG2RAD (3.14159/180)

void Motor_task(){
    float ph = 360.0-(*rotor_angle)*360.0/1024/16-14.7;

    pwm_set_both_levels(ph_a,(sin(ph      *DEG2RAD*5)+1.01)*50,0);
    pwm_set_both_levels(ph_b,(sin((ph+120/5)*DEG2RAD*5)+1.01)*50,0);
    pwm_set_both_levels(ph_c,(sin((ph+240/5)*DEG2RAD*5)+1.01)*50,0);
    
}

void Motor_init(uint32_t *anglevar) {

    rotor_angle = anglevar;

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