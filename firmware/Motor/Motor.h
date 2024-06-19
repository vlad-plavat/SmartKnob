#ifndef _Motor_H
#define _Motor_H

#define Motor_A 0
#define Motor_B 2
#define Motor_C 4

#define PWM_RESOLUTION 1000
#define MOTOR_MAX_POWER_LIMIT 349

extern int ang_speed_as_int;
extern float ph, angle_full_rot, angle_full_rot_offset, ang_speed, power;
extern int32_t num_ovfl;
extern int32_t motor_power_max;

void Motor_init(uint32_t *anglevar);
void Motor_task();
void Motor_set_mode_detents(int detents);
void Motor_set_mode_detents_offset(int detents, int32_t offset);
void Motor_add_endstops_to_mode(float emin, float emax);
void Motor_set_mode_constant_velocity();
void Motor_set_mode_friction(int8_t friction);
void Motor_vibrate();

#endif