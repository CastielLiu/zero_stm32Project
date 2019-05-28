#include "led.h"
#include "can.h"
#include "delay.h"
#include "sys.h"
#include "usart.h" 
#include "timer.h"
#include "stdlib.h"
#include "math.h"
#include "bk.h"
#include "24l01.h"
#include "wireless_data.h"


extern float pre_steer;
float g_speed = 0.0;
float g_steeringAngle =0.0;
//遥控器数据接收缓存-LC
u8 wirelessBuf[32];

int main(void)
{	
	u16 steer_pwm,speed_pwm;
	float resteer;      //手动模式转角控制指令
	//float rf_speed,rr_speed;   //测速值
	float dianji;
	u8 manual = 1;   //默认为手动模式
	u16 LX,RY;
	
	NRF24L01_Init();
	
	delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	
	uart_init(115200);	 //串口初始化为115200
	CAN_Mode_Init(CAN_SJW_1tq,CAN_BS2_8tq,CAN_BS1_9tq,4,CAN_Mode_Normal);
	
	LED_Init();			     //LED端口初始化
	BK_Init();             //制动端口初始化

	StepMotor_Init();                 //步进电机配置初始化
	ReMotor_PWM_Init();      //
	TIM1_Encoder_Init(0xffff,0);      //TIM5编码器模式初始化
	TIM3_Encoder_Init(0xffff,0);      //TIM3编码器模式初始化	
	TIM_SetCompare3(TIM5,0);   //TIM5_CH3
	TIM_SetCompare4(TIM5,0);   //TIM5_CH4

	while(NRF24L01_Check())
	{
 		delay_ms(350);
		LED1 =!LED1;
		LED0=!LED0;
	}
	NRF24L01_RX_Mode();

	

	while(1)
	{
		if(NRF24L01_RxPacket(wirelessBuf)==0)
		{
			manual = wirelessBuf[8];
			LX = (wirelessBuf[6]<<8)+wirelessBuf[7];
			RY = (wirelessBuf[2]<<8)+wirelessBuf[3];
//			printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t \r\n",wirelessBuf[0],wirelessBuf[1],wirelessBuf[2],wirelessBuf[3],wirelessBuf[4],wirelessBuf[5],wirelessBuf[6],wirelessBuf[7],wirelessBuf[8]);
		}

		if(manual)			
		{
			if(LX<2250&&LX>1948)
			{
				resteer=2048;
				//steer_pwm=25000;
			}
			else
				resteer=LX;
			steer_pwm=(u16)((1.80-((2048-(resteer-30))*0.25)/2048)*1000);
			//steer_pwm=(u16)(1880);
			//steer_pwm=25000;
			if(RY<1948&&RY>0)
			{
				dianji=3276;
				GPIO_ResetBits(GPIOB,GPIO_Pin_7);
			}
			else if(RY>=1948&&RY<2148)
			{
				dianji=2048;
				GPIO_SetBits(GPIOB,GPIO_Pin_7);
			 }
		
			else
			{
				GPIO_SetBits(GPIOB,GPIO_Pin_7);
				dianji=RY;
			}
				
			speed_pwm=(u16)((0.1-((2048.0-dianji)*0.20)/2048.0)*(RPERIOD+1));
			//speed_pwm=(u16)(0.3*RPERIOD);
			TIM_SetCompare3(TIM5,steer_pwm);   //TIM5_CH3
			TIM_SetCompare4(TIM5,speed_pwm);   //TIM5_CH4
				
			//printf("LX:%d, RY:%d\r\n",LX,RY);
			//printf("mode:%d, resteer:%.1f,dianji:%.1f, steer_pwm:%d, speed_pwm:%d\r\n",manual,resteer,dianji,steer_pwm,speed_pwm);
		}
	
		else     //自动模式
		{
			steer_pwm=(u16)((1.80-g_steeringAngle*0.35/25.0)*1000); //steer
			//steer_pwm=(u16)(2000);
			TIM_SetCompare3(TIM5,steer_pwm);
			speed_pwm=(u16)((0.2+g_speed*3.6*0.8/40.0)*RPERIOD);//motor
			TIM_SetCompare4(TIM5,speed_pwm);
		}
		delay_ms(29);
		
	}	
}
