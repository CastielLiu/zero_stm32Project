#include "data_deal.h"

//将浮点数f转化为4个字节数据存放在byte[4]中
void Float_to_Byte(float f,u8 byte[])
{
	FloatLongType fl;
	fl.fdata=f;
	byte[0]=(unsigned char)fl.ldata;
	byte[1]=(unsigned char)(fl.ldata>>8);
	byte[2]=(unsigned char)(fl.ldata>>16);
	byte[3]=(unsigned char)(fl.ldata>>24);
}

//将4个字节数据byte[4]转化为浮点数存放于f中
void Byte_to_Float(float *f,u8 byte[])
{
	FloatLongType fl;
	fl.ldata=0;
	fl.ldata=byte[3];
	fl.ldata=(fl.ldata<<8)|byte[2];
	fl.ldata=(fl.ldata<<8)|byte[1];
	fl.ldata=(fl.ldata<<8)|byte[0];
	*f=fl.fdata;
}

//发送数据处理函数，将数据转化为字节存储到发送缓存区
//sendbuff:发送数据缓存区
//data:发送数组
//num:数组长度
void sendfloat(u8 sendbuff[],float data[],int num)
{
	int i,j;
	u8 byte[4];
	//数据开头
	sendbuff[0]=0xff;
	sendbuff[1]=0xff;
	for(i=0;i<num;i++)
	{
		Float_to_Byte(data[i],byte);   //拆解数据
		for(j=0;j<4;j++)       //拆解之后的字节存入发送缓存区
		{
			sendbuff[i*4+j+2] = byte[j];
		}	
	}
	sendbuff[4*num+2]='\r';
	sendbuff[4*num+3]='\n';	
}

//接收数据处理函数，将数据缓存区里的字符转化为所需数组
//recvbuff:接收数据缓存区
//data:接收数组
//num:数组长度
void recvfloat(u8 recvbuff[],float data[],int num)
{
	int i,j;
	float f=0;
	u8 byte[4];
	if(recvbuff[0]==0xff && recvbuff[1]==0xff)  //检查数据头
	{
		for(i=0;i<num;i++)     //n个数据
		{
			for(j=0;j<4;j++)     //取出数据的各个字节
			{
				byte[j] = recvbuff[i*4+j+2];
			}
			Byte_to_Float(&f,byte);    //转化为数据
			data[i] = f;     //存入数组
		}
	}
}
 
//将双精度浮点数d转化为8个字节数据存放在byte[8]中
void Double_to_Byte(double d,u8 byte[])
{
	int i;
	DoubleLongType du;
	du.ddata=d;
	for(i=0;i<8;i++)
	{
		byte[i]=(u8)(du.ldata>>i*8);
	}
}

//将8个字节数据byte[8]转化为双精度浮点数存放于d中
void Byte_to_Double(double *d,u8 byte[])
{
	int i;
	DoubleLongType du;
	du.ldata=0;
	du.ldata=byte[7];
	for(i=6;i>=0;i--)
	{
		du.ldata=(du.ldata<<8)|byte[i];
	}
	*d=du.ddata;
}

//接收数据处理函数，将数据缓存区里的字符转化为所需数组
//recvbuff:接收数据缓存区
//data:接收数组
//num:数组长度
void recvdouble(u8 recvbuff[],double data[],int num)
{
	int i,j;
	double d=0;
	u8 byte[8];
	if(recvbuff[0]==0xff && recvbuff[1]==0xff)  //检查数据头
	{
		for(i=0;i<num;i++)     //n个数据
		{
			for(j=0;j<8;j++)     //取出数据的各个字节
			{
				byte[j] = recvbuff[i*8+j+2];
			}
			Byte_to_Double(&d,byte);    //转化为数据
			data[i] = d;     //存入数组
		}
	}
}

//发送数据处理函数，将数据转化为字节存储到发送缓存区
//sendbuff:发送数据缓存区
//data:发送数组
//num:数组长度
void senddouble(u8 sendbuff[],double data[],int num)
{
	int i,j;
	u8 byte[8];
	//数据开头
	sendbuff[0]=0xff;
	sendbuff[1]=0xff;
	for(i=0;i<num;i++)
	{
		Double_to_Byte(data[i],byte);   //拆解数据
		for(j=0;j<8;j++)       //拆解之后的字节存入发送缓存区
		{
			sendbuff[i*8+j+2] = byte[j];
		}	
	}	
	sendbuff[8*num+2]='\r';
	sendbuff[8*num+3]='\n';
}

/*
--接收数据处理函数，stm32专用--
返回值为拆分之后数组的长度
recvbuff--接收到的原始字符串
def--分割字符串，一般为','
data--拆分后获得的最终数组
*/
int split(char *recvbuff, char *def, float data[])
{
	int i = 0;
	char *temp = strtok(recvbuff,def);
	while(temp)
	{
		data[i] = atof(temp);
		temp = strtok(NULL,def);
		i++;
	}
	return i;
}

/*
--发送数据处理函数，stm32专用--
sendbuff--待发送的字符串
def--分割字符串，一般为','
data--需要发送的数组
*/
void combine(char *sendbuff, char *def, float data[], int num)
{
	int i;
	char str[10];   //拆分字符缓存区
	sprintf(str, "%.2f", data[0]);   //将浮点转化为字符串
	strcpy(sendbuff,str);       //将其拷贝到发送数据缓存区
	for(i=1;i<num;i++)
	{
		strcat(sendbuff,def);  //拼接字符串
		sprintf(str, "%.2f", data[i]);   //将浮点转化为字符串
		strcat(sendbuff,str);       //将其拷贝到发送数据缓存区
	}
	strcat(sendbuff,"\r\n");
}
