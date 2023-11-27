#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "Motor.h"


uint ph_a, ph_b, ph_c;

void Motor_task(){
    uint32_t ph = (time_us_32()/50000)%6;
    pwm_set_both_levels(ph_a,(ph+1)%6<3?500:0,0);
    pwm_set_both_levels(ph_b,(ph+3)%6<3?500:0,0);
    pwm_set_both_levels(ph_c,(ph+5)%6<3?500:0,0);
    
}

void Motor_init() {

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