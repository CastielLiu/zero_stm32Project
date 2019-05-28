#include "sys.h"
#include "usart.h"	  
#include "can.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��ucos,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos ʹ��	  
#endif
 

//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
_sys_exit(int x) 
{ 
	x = x; 
} 
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//ѭ������,ֱ���������   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 

/*ʹ��microLib�ķ���*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t) ch);

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/
 
#if EN_USART1_RX   //���ʹ���˽���
	  
  
void uart_init(u32 bound){
  //GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��USART1��GPIOAʱ��

	//USART1_TX   GPIOA.9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA.9

	//USART1_RX	  GPIOA.10��ʼ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//PA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA.10  

	//Usart1 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���

	//USART ��ʼ������

	USART_InitStructure.USART_BaudRate = bound;//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

	USART_Init(USART1, &USART_InitStructure); //��ʼ������1
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//�������ڽ����ж�

	USART_Cmd(USART1, ENABLE);                    //ʹ�ܴ���1 
}

uint8_t generateCheckNum(const uint8_t* ptr,int len)
{
    uint8_t sum=0;
    int i=2;

    for( ;i<len-1 ; i++)
        sum += ptr[i];
    return sum;
}

void USART1_write(u8 *buf,u8 len)
{
	int i=0;
	for(; i<len; i++)
	{
		while((USART1->SR&0X40)==0);
		USART1->DR = buf[i]; 
	}
}


#define MaxBufSize 20
u8 pkg_buf[MaxBufSize];
extern float g_speed;
extern float g_steeringAngle;

u8 headerFound = 0;
u8 seq = 0;
u8 pkgLen=0;
//u8 sendBuf[10]={0x66,0xcc,0x07,0x00}; //debug

void USART1_IRQHandler(void)                	//����1�жϷ������
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
		pkg_buf[seq++] =USART_ReceiveData(USART1);	//��ȡ���յ�������
		//printf("seq:%d\t %x\r\n",seq,pkg_buf[seq-1]);
		if(headerFound==0 && seq>1 && pkg_buf[seq-1]==0xcc && pkg_buf[seq-2]==0x66)
		{
			headerFound = 1;
			seq = 2;
			//printf("found\r\n");
		}
		else if(seq==3 && headerFound)
		{
			pkgLen = pkg_buf[2]+3;
			//printf("len:%d\r\n",pkgLen);
		}
		else if(seq==pkgLen && headerFound)
		{
			if(pkg_buf[pkgLen-1]==generateCheckNum(pkg_buf,pkgLen))
			{
				g_speed = 0.01 * (pkg_buf[4] + pkg_buf[5]*256 - 65535/2);
				g_steeringAngle = 0.01 * (pkg_buf[6] + pkg_buf[7]*256 - 65535/2);
				//printf("%f\t%f\r\n",g_speed,g_steeringAngle);
				//printf("%x\t%x\t%x\t%x\r\n",pkg_buf[4],pkg_buf[5],pkg_buf[6],pkg_buf[7]);
				CanSendSystemStatus(1);
			}
			else
				CanSendSystemStatus(0);
				//printf("%x\r\n",generateCheckNum(pkg_buf,pkgLen));
			headerFound = 0;
		}
		else if(seq == MaxBufSize)
		{
			seq = 0;
			headerFound = 0;
		}
	}
} 
#endif	

