#include "timer.h"
#include "led.h"
#include "usart.h"
#include "delay.h"
#include "zlpid.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK 精英 STM32开发板
//PWM  驱动代码			   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2010/12/03
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 正点原子 2009-2019
//All rights reserved
////////////////////////////////////////////////////////////////////////////////// 	  

//定时器2脉冲输出-步进电机控制
//TIM2_CH3:PB10,完全重映射
u16 TIM2_PULSE_CNT = 0;     //输出脉冲数
u16 NEED_PULSE_NUM = 0;     //所需脉冲数
float pre_steer = 0;        //步进电机转角值

void StepMotor_Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //使能定时器2时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);   //使能GPIO和AFIO复用功能时钟
	
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);   //重映射
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;     //PB10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 		
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推免输出
	GPIO_Init(GPIOB ,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;		//PA5端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);					 //根据设定参数初始化
	GPIO_ResetBits(GPIOA, GPIO_Pin_5);   //输出设置低电平
	 
	TIM_DeInit(TIM2);
	TIM_TimeBaseStructure.TIM_Period = PERIOD; 
	TIM_TimeBaseStructure.TIM_Prescaler = Fenpin;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;   //时钟分频	
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	  
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;   //PWM1位正常占空比模式，pwm2为反极性模式	
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  //使能输出
	TIM_OCInitStructure.TIM_Pulse = 0;     //占空时间
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性:TIM输出比较极性高
	
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);        //通道3
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);  
	
	TIM_ClearFlag(TIM2,TIM_FLAG_Update);         
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  //TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
	 
	TIM_ARRPreloadConfig(TIM2, ENABLE);
	TIM_Cmd(TIM2, ENABLE);   //启动定时器 
}

//定时器2中断函数
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2 ,TIM_IT_Update)!= RESET)
	{
		TIM_ClearITPendingBit(TIM2 ,TIM_IT_Update);
		if(TIM2_PULSE_CNT < NEED_PULSE_NUM)
		{
			TIM2_PULSE_CNT++;    //定时器溢出时脉冲数+1
			if(DIR)    //turn right
				pre_steer += (360.0/XiFen);
			else
				pre_steer -= (360.0/XiFen);
		}	
		else        //已输出指定数量的脉冲
			TIM_SetCompare3(TIM2,0);  //拉低通道3,停止脉冲输出
	}
}

//设定脉冲数
void Motor_Stepset(float steer_order)
{
	float det_steer;
	det_steer = steer_order - pre_steer;
	if((TIM2_PULSE_CNT >= NEED_PULSE_NUM) && (det_steer!=0))  //上个转角指令已执行完毕
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
		TIM_SetCompare3(TIM2,0.5*PERIOD);   //开启输出
	}
}

//步进电机重置
void Motor_Reset(void)
{
	TIM2_PULSE_CNT = 0;
	NEED_PULSE_NUM = 0;
	pre_steer = 0;
}

//定时器5PWM输出-电机控制
//TIM5_CH3:PA2, TIM5_CH4:PA3
void ReMotor_PWM_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);	//使能定时器5时钟
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOE, ENABLE);  //使能GPIO外设和AFIO复用功能模块时钟
	
   //设置该引脚为复用输出功能
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIO
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;  //PD12,13,方向控制
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);//初始化GPIO
	GPIO_SetBits(GPIOE,GPIO_Pin_0 | GPIO_Pin_1); 
	
   //初始化TIM5
	TIM_TimeBaseStructure.TIM_Period = RPERIOD; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler = RFenpin; //设置用来作为TIMx时钟频率除数的预分频值 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
	
	//初始化TIM5 Channe3,4 PWM模式	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //选择定时器模式:TIM脉冲宽度调制模式2
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性:TIM输出比较极性高
	
	TIM_OC3Init(TIM5, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM3 OC2
	TIM_OC3PreloadConfig(TIM5, TIM_OCPreload_Enable);  //使能TIM3在CCR2上的预装载寄存器
	
	TIM_OC4Init(TIM5, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM3 OC2
	TIM_OC4PreloadConfig(TIM5, TIM_OCPreload_Enable);  //使能TIM3在CCR2上的预装载寄存器
	
	//定时器中断配置函数,第一个参数是定时器，第二个参数选择中断源，第三个使能参数
	TIM_ClearFlag(TIM5, TIM_FLAG_Update);
	TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
	
	TIM_ARRPreloadConfig(TIM5, ENABLE); //使能TIMx在ARR上的预装载寄存器
	TIM_Cmd(TIM5, ENABLE);  //使能TIM5
}


//驱动电机方向控制
void ReMotor_dircontrol(u8 dir)
{
	if(dir)   //前进
	{
		IN1_H;
		IN2_L;
	}
	else    //后退
	{
		IN1_L;
		IN2_H;
	}
}

//电机测速代码
//定时器3编码器模式，双边沿计数
//TIM3_CH1:PA6, TIM3_CH2:PA7
void TIM3_Encoder_Init(u16 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;     //GPIO结构体
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;   //时基参数  
	TIM_ICInitTypeDef  TIM3_ICInitStructure;    //定时器使用模式结构体

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //Tim3时钟使能
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  //使能GPIO外设和AFIO复用功能模块时钟使能
	
	//初始化GPIOC-4个通道的IO口
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7; //TIM_CH1,2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  //浮空输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);     //初始化GPIOA
	
	//初始化TIM3参数
	TIM_DeInit(TIM3);
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler = psc; //设置用来作为TIMx时钟频率除数的预分频值  
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
	
	//Tim3,编码器模式3,双边沿触发
	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_BothEdge ,TIM_ICPolarity_BothEdge);
  TIM_ICStructInit(&TIM3_ICInitStructure);//将结构体的内容缺省输入
  TIM3_ICInitStructure.TIM_ICFilter = 6;  //配置输入滤波器
  TIM_ICInit(TIM3, &TIM3_ICInitStructure);//
	
	//定时器中断配置函数,第一个参数是定时器，第二个参数选择中断源，第三个使能参数
	//TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	//TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM3->CNT = 0;   //重置计数器
	TIM_Cmd(TIM3, ENABLE);  //使能TIMx外设,启动定时器3						 
}

//定时器1编码器模式，双边沿计数
//TIM1_CH1:PE9, TIM1_CH2:PE11
void TIM1_Encoder_Init(u16 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;     //GPIO结构体
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;   //时基参数  
	TIM_ICInitTypeDef  TIM_ICInitStructure;    //定时器使用模式结构体

	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); //Tim3时钟使能
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE | RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO, ENABLE);  //使能GPIO外设时钟
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, ENABLE);   //重映射
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9|GPIO_Pin_11; //TIM_CH1,2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  //浮空输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);     //初始化GPIOE
	
	//初始化TIM1参数
	TIM_DeInit(TIM1);
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler = psc; //设置用来作为TIMx时钟频率除数的预分频值  
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
	
	//Tim5,编码器模式3,双边沿触发
	TIM_EncoderInterfaceConfig(TIM1, TIM_EncoderMode_TI12, TIM_ICPolarity_BothEdge ,TIM_ICPolarity_BothEdge);
  TIM_ICStructInit(&TIM_ICInitStructure);//将结构体的内容缺省输入
  TIM_ICInitStructure.TIM_ICFilter = 6;  //配置输入滤波器
  TIM_ICInit(TIM1, &TIM_ICInitStructure);//
	
	//定时器中断配置函数,第一个参数是定时器，第二个参数选择中断源，第三个使能参数
	//TIM_ClearFlag(TIM5, TIM_FLAG_Update);
	//TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
	TIM1->CNT = 0;   //重置计数器
	TIM_Cmd(TIM1, ENABLE);  //使能TIMx外设,启动定时器1				 
}

//读取两个定时器的计数值
void Encoder_Read_Speed(float *rf_speed,float *rr_speed)
{
	int T1_CNT,T3_CNT;
	TIM1->CNT = 0;
	TIM3->CNT = 0;
	delay_ms(Meatime);          //测量时长20ms
	T1_CNT = (int)((s16)(TIM1->CNT));
	T3_CNT = (int)((s16)(TIM3->CNT));
	*rf_speed = 15000.0*T1_CNT/(Meatime*XianShu*Redu_ratio);  
	*rr_speed = 15000.0*T3_CNT/(Meatime*XianShu*Redu_ratio);
}

