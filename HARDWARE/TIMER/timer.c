#include "timer.h"
#include "led.h"
#include "usart.h"
#include "delay.h"
#include "zlpid.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK ��Ӣ STM32������
//PWM  ��������			   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2010/12/03
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ����ԭ�� 2009-2019
//All rights reserved
////////////////////////////////////////////////////////////////////////////////// 	  

//��ʱ��2�������-�����������
//TIM2_CH3:PB10,��ȫ��ӳ��
u16 TIM2_PULSE_CNT = 0;     //���������
u16 NEED_PULSE_NUM = 0;     //����������
float pre_steer = 0;        //�������ת��ֵ

void StepMotor_Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //ʹ�ܶ�ʱ��2ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);   //ʹ��GPIO��AFIO���ù���ʱ��
	
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);   //��ӳ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;     //PB10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 		
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //�����������
	GPIO_Init(GPIOB ,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;		//PA5�˿�����
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);					 //�����趨������ʼ��
	GPIO_ResetBits(GPIOA, GPIO_Pin_5);   //������õ͵�ƽ
	 
	TIM_DeInit(TIM2);
	TIM_TimeBaseStructure.TIM_Period = PERIOD; 
	TIM_TimeBaseStructure.TIM_Prescaler = Fenpin;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;   //ʱ�ӷ�Ƶ	
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	  
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;   //PWM1λ����ռ�ձ�ģʽ��pwm2Ϊ������ģʽ	
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  //ʹ�����
	TIM_OCInitStructure.TIM_Pulse = 0;     //ռ��ʱ��
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //�������:TIM����Ƚϼ��Ը�
	
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);        //ͨ��3
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);  
	
	TIM_ClearFlag(TIM2,TIM_FLAG_Update);         
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  //TIM2�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //��ռ���ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //�����ȼ�3��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);  //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���
	 
	TIM_ARRPreloadConfig(TIM2, ENABLE);
	TIM_Cmd(TIM2, ENABLE);   //������ʱ�� 
}

//��ʱ��2�жϺ���
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2 ,TIM_IT_Update)!= RESET)
	{
		TIM_ClearITPendingBit(TIM2 ,TIM_IT_Update);
		if(TIM2_PULSE_CNT < NEED_PULSE_NUM)
		{
			TIM2_PULSE_CNT++;    //��ʱ�����ʱ������+1
			if(DIR)    //turn right
				pre_steer += (360.0/XiFen);
			else
				pre_steer -= (360.0/XiFen);
		}	
		else        //�����ָ������������
			TIM_SetCompare3(TIM2,0);  //����ͨ��3,ֹͣ�������
	}
}

//�趨������
void Motor_Stepset(float steer_order)
{
	float det_steer;
	det_steer = steer_order - pre_steer;
	if((TIM2_PULSE_CNT >= NEED_PULSE_NUM) && (det_steer!=0))  //�ϸ�ת��ָ����ִ�����
	{
		if(det_steer >= 0)   //turn right
		{
			DIR = 1;
		}
		else     //turn left
		{
			DIR = 0;
			det_steer = -det_steer;
		}
		NEED_PULSE_NUM = (u16)(det_steer*XiFen/360);
		TIM2_PULSE_CNT = 0;
		TIM_SetCompare3(TIM2,0.5*PERIOD);   //�������
	}
}

//�����������
void Motor_Reset(void)
{
	TIM2_PULSE_CNT = 0;
	NEED_PULSE_NUM = 0;
	pre_steer = 0;
}

//��ʱ��5PWM���-�������
//TIM5_CH3:PA2, TIM5_CH4:PA3
void ReMotor_PWM_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);	//ʹ�ܶ�ʱ��5ʱ��
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOE, ENABLE);  //ʹ��GPIO�����AFIO���ù���ģ��ʱ��
	
   //���ø�����Ϊ�����������
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIO
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;  //PD12,13,�������
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);//��ʼ��GPIO
	GPIO_SetBits(GPIOE,GPIO_Pin_0 | GPIO_Pin_1); 
	
   //��ʼ��TIM5
	TIM_TimeBaseStructure.TIM_Period = RPERIOD; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ
	TIM_TimeBaseStructure.TIM_Prescaler = RFenpin; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure); //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
	
	//��ʼ��TIM5 Channe3,4 PWMģʽ	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //ѡ��ʱ��ģʽ:TIM�����ȵ���ģʽ2
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //�Ƚ����ʹ��
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //�������:TIM����Ƚϼ��Ը�
	
	TIM_OC3Init(TIM5, &TIM_OCInitStructure);  //����Tָ���Ĳ�����ʼ������TIM3 OC2
	TIM_OC3PreloadConfig(TIM5, TIM_OCPreload_Enable);  //ʹ��TIM3��CCR2�ϵ�Ԥװ�ؼĴ���
	
	TIM_OC4Init(TIM5, &TIM_OCInitStructure);  //����Tָ���Ĳ�����ʼ������TIM3 OC2
	TIM_OC4PreloadConfig(TIM5, TIM_OCPreload_Enable);  //ʹ��TIM3��CCR2�ϵ�Ԥװ�ؼĴ���
	
	//��ʱ���ж����ú���,��һ�������Ƕ�ʱ�����ڶ�������ѡ���ж�Դ��������ʹ�ܲ���
	TIM_ClearFlag(TIM5, TIM_FLAG_Update);
	TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
	
	TIM_ARRPreloadConfig(TIM5, ENABLE); //ʹ��TIMx��ARR�ϵ�Ԥװ�ؼĴ���
	TIM_Cmd(TIM5, ENABLE);  //ʹ��TIM5
}


//��������������
void ReMotor_dircontrol(u8 dir)
{
	if(dir)   //ǰ��
	{
		IN1_H;
		IN2_L;
	}
	else    //����
	{
		IN1_L;
		IN2_H;
	}
}

//������ٴ���
//��ʱ��3������ģʽ��˫���ؼ���
//TIM3_CH1:PA6, TIM3_CH2:PA7
void TIM3_Encoder_Init(u16 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;     //GPIO�ṹ��
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;   //ʱ������  
	TIM_ICInitTypeDef  TIM3_ICInitStructure;    //��ʱ��ʹ��ģʽ�ṹ��

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //Tim3ʱ��ʹ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  //ʹ��GPIO�����AFIO���ù���ģ��ʱ��ʹ��
	
	//��ʼ��GPIOC-4��ͨ����IO��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7; //TIM_CH1,2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  //��������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);     //��ʼ��GPIOA
	
	//��ʼ��TIM3����
	TIM_DeInit(TIM3);
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ
	TIM_TimeBaseStructure.TIM_Prescaler = psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ  
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
	
	//Tim3,������ģʽ3,˫���ش���
	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_BothEdge ,TIM_ICPolarity_BothEdge);
  TIM_ICStructInit(&TIM3_ICInitStructure);//���ṹ�������ȱʡ����
  TIM3_ICInitStructure.TIM_ICFilter = 6;  //���������˲���
  TIM_ICInit(TIM3, &TIM3_ICInitStructure);//
	
	//��ʱ���ж����ú���,��һ�������Ƕ�ʱ�����ڶ�������ѡ���ж�Դ��������ʹ�ܲ���
	//TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	//TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM3->CNT = 0;   //���ü�����
	TIM_Cmd(TIM3, ENABLE);  //ʹ��TIMx����,������ʱ��3						 
}

//��ʱ��1������ģʽ��˫���ؼ���
//TIM1_CH1:PE9, TIM1_CH2:PE11
void TIM1_Encoder_Init(u16 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;     //GPIO�ṹ��
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;   //ʱ������  
	TIM_ICInitTypeDef  TIM_ICInitStructure;    //��ʱ��ʹ��ģʽ�ṹ��

	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); //Tim3ʱ��ʹ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE | RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO, ENABLE);  //ʹ��GPIO����ʱ��
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, ENABLE);   //��ӳ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9|GPIO_Pin_11; //TIM_CH1,2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  //��������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);     //��ʼ��GPIOE
	
	//��ʼ��TIM1����
	TIM_DeInit(TIM1);
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ
	TIM_TimeBaseStructure.TIM_Prescaler = psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ  
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
	
	//Tim5,������ģʽ3,˫���ش���
	TIM_EncoderInterfaceConfig(TIM1, TIM_EncoderMode_TI12, TIM_ICPolarity_BothEdge ,TIM_ICPolarity_BothEdge);
  TIM_ICStructInit(&TIM_ICInitStructure);//���ṹ�������ȱʡ����
  TIM_ICInitStructure.TIM_ICFilter = 6;  //���������˲���
  TIM_ICInit(TIM1, &TIM_ICInitStructure);//
	
	//��ʱ���ж����ú���,��һ�������Ƕ�ʱ�����ڶ�������ѡ���ж�Դ��������ʹ�ܲ���
	//TIM_ClearFlag(TIM5, TIM_FLAG_Update);
	//TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
	TIM1->CNT = 0;   //���ü�����
	TIM_Cmd(TIM1, ENABLE);  //ʹ��TIMx����,������ʱ��1				 
}

//��ȡ������ʱ���ļ���ֵ
void Encoder_Read_Speed(float *rf_speed,float *rr_speed)
{
	int T1_CNT,T3_CNT;
	TIM1->CNT = 0;
	TIM3->CNT = 0;
	delay_ms(Meatime);          //����ʱ��20ms
	T1_CNT = (int)((s16)(TIM1->CNT));
	T3_CNT = (int)((s16)(TIM3->CNT));
	*rf_speed = 15000.0*T1_CNT/(Meatime*XianShu*Redu_ratio);  
	*rr_speed = 15000.0*T3_CNT/(Meatime*XianShu*Redu_ratio);
}

