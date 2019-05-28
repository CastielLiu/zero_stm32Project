#include "zlpid.h"

void pid_init(zlpid *pid)
{
	pid->output = 0.0;
	pid->error = 0.0;
	pid->error_last = 0.0;
	pid->error_previous = 0.0;
	pid->kp = 0.1;
	pid->ki = 0.01;
	pid->kd = 0.01;
}

void pid_realise(zlpid *pid, float set_speed, float actual_speed)
{ 
	float increment_output; 
	pid->error = set_speed - actual_speed;
	//计算控制输出的增量
	increment_output = pid->kp*(pid->error - pid->error_last) + pid->ki*pid->error + pid->kd*(pid->error - 2*pid->error_last + pid->error_previous);
	pid->output += increment_output;
	pid->error_previous = pid->error_last;
	pid->error_last = pid->error;
	if(pid->output <= OutputMin)
		pid->output = OutputMin;
	if(pid->output >= OutputMax)
		pid->output = OutputMax;
}
