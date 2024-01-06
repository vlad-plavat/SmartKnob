#ifndef _Motor_H
#define _Motor_H

#define Motor_A 0
#define Motor_B 2
#define Motor_C 4

#define PWM_RESOLUTION 1000
#define MOTOR_MAX_POWER_LIMIT 349

extern int motor_dbg_a, motor_dbg_b, motor_dbg_c;
extern float ph, angle_full_rot, angle_full_rot_offset, ang_speed, power;
extern int32_t num_ovfl;
extern int32_t motor_power_max;

void Motor_init();
void Motor_task();
void Motor_set_mode_detents(int detents, int32_t offset);
void Motor_vibrate();

#endif