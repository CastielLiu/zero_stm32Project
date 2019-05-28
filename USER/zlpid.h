#ifndef _ZLPID_H
#define _ZLPID_H
#include "stdio.h"

#define OutputMax 100.0
#define OutputMin 0.0

typedef struct{
	float output;
	float error;
	float error_last;
	float error_previous;
	float kp,ki,kd;
}zlpid;

void pid_init(zlpid *pid);
void pid_realise(zlpid *pid, float set_speed, float actual_speed);

#endif
