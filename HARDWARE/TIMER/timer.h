#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"

//�����������
#define DIR PAout(5) //PA5
#define XiFen 2000
#define PERIOD 999
#define Fenpin 71
#define Meatime 20
//�����������
#define IN1_H PEout(0)=1
#define IN1_L PEout(0)=0
#define IN2_H PEout(1)=1
#define IN2_L PEout(1)=0
#define RPERIOD 2999
#define RFenpin 71
#define XianShu 16
#define Redu_ratio 19

void StepMotor_Init(void);    //���������ʼ��
void Motor_Stepset(float steer_order);   //�趨ת��
void Motor_Reset(void);       //�������״̬����

void ReMotor_PWM_Init(void);  //���������ʼ��
void TIM1_Encoder_Init(u16 arr,u16 psc);   //TIM1������ģʽ
void TIM3_Encoder_Init(u16 arr,u16 psc);   //TIM3������ģʽ
void Encoder_Read_Speed(float *rf_speed,float *rr_speed);
void ReMotor_dircontrol(u8 dir);    //��������������
#endif
