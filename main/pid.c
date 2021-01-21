#include "pid.h"

static float Kp = 0, Ki = 0, Kd = 0; 
static float Min = 0, Max = 0;
static float error_prior = 0;
static float integral_prior = 0;

void pid_init(float kp, float ki, float kd, float min, float max)
{
    Kp = kp;
    Ki = ki;
    Kd = kd;
    Min = min;
    Max = max;
}

void pid_reset()
{
    error_prior = 0;
    integral_prior = 0;
}

void pid_set_coefficients(float kp, float ki, float kd)
{
    Kp = kp;
    Ki = ki;
    Kd = kd;
}

void pid_set_out_limits(float min, float max)
{
    Min = min;
    Max = max;
}

float pid_compute(float actual_value, float desired_value, float iteration_time)
{
    float error = desired_value - actual_value;
    float integral = integral_prior + error * iteration_time;
    float derivative = (error - error_prior) / iteration_time;
    float output = Kp * error + Ki * integral + Kd * derivative;
    error_prior = error;
    integral_prior = integral;

    if(output > Max) output = Max;
    if(output < Min) output = Min;

    return output;
}