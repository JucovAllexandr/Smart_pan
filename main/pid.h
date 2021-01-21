#ifndef PID_H
#define PID_H

void pid_init(float kp, float ki, float kd, float min, float max);
void pid_reset();

void pid_set_coefficients(float kp, float ki, float kd);
void pid_set_out_limits(float min, float max);

float pid_compute(float actual_value, float desired_value, float iteration_time);

#endif