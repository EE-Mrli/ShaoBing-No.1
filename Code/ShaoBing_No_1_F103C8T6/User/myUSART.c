#include "myUSART.h"
#include <stdio.h>
#include <string.h>
#include "DS18B20.h"
unsigned char httpNeedPostFlag = 0;
unsigned char wifiIsConnected = 1;
#define USART1_REC_LEN 50
u8 Uart1_TempBuf_Tail = 0;//���ջ�����βָ��
u8 Uart1_TempBuf[250];//���ջ���������

u16 tnum = 0;
u8 Uart1_RevBuf_Tail = 0;//���ջ�����βָ��
u8 Uart1_RevBuf[USART1_REC_LEN];//���ջ���������
u8 receiveSucceedFlag = 0;
u8 startReceiveFlag = 0;
char TxBuffer[20];

void UART1_init()
{
  //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);
  USART_DeInit(USART1);
 

  //USART1�˿�����
  //UASART_TX   PA9
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;    //�����������
  GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��PA9
  //USART1_RX      PA10
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
  GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��PA10

  //USART1 ��ʼ������
  USART_InitStructure.USART_BaudRate = 115200;//����������
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
  USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;    //�շ�ģʽ     
  USART_Init(USART1, &USART_InitStructure); //��ʼ������1

    //Usart1 NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//����1�ж�ͨ��
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//��ռ���ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;        //�����ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            //IRQͨ��ʹ��
  NVIC_Init(&NVIC_InitStructure);    //����ָ���Ĳ�����ʼ��VIC�Ĵ���
    
  USART_Init(USART1, &USART_InitStructure);
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//��������ж�
  USART_Cmd(USART1, ENABLE);  //ʹ�ܴ���1

}

//����1�жϷ������
void USART1_IRQHandler(void)                    
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //�����ж�
    {    
        char temp;
        //Uart1_RevBuf[Uart1_RevBuf_Tail] = USART_ReceiveData(USART1);//��ȡ���յ������ݣ���β�����
        //USART_SendData(USART1,Uart1_RevBuf[Uart1_RevBuf_Tail]);//���ͽ��յ�������
        //while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
        //{}
        //Uart1_RevBuf_Tail++;
        //if(Uart1_RevBuf_Tail>USART1_REC_LEN-1)
        //{
        //    Uart1_RevBuf_Tail = 0;    
        // }
		temp = USART_ReceiveData(USART1);
		
		//tnum++;
		if(temp == '[')
		{
			startReceiveFlag = 1;
			Uart1_RevBuf_Tail = 0;
		}
		else if(temp == ']')
		{
			startReceiveFlag = 0;
			receiveSucceedFlag = 1;
			Uart1_RevBuf[Uart1_RevBuf_Tail] = '\0';
			if(strcmp((const char *)Uart1_RevBuf,"Connected") == 0)
			{
				wifiIsConnected = 1;
			}
		}
		else if(startReceiveFlag)
		{
			Uart1_RevBuf[Uart1_RevBuf_Tail++] = temp;
		}
		else
		{
			Uart1_TempBuf[Uart1_TempBuf_Tail++] = temp;
			if(Uart1_TempBuf_Tail == 250)Uart1_TempBuf_Tail=0;
		}
		
    } 
}



void myUSART_SentTempData(void)
{
	char temp[10],i = 0;
	
	//httpNeedPostFlag = 0;
	sprintf(temp,"%.1f",DS18B20_Temperature);
	TxBuffer[0] = '[';
	while(temp[i] != '\0')
	{
		TxBuffer[i + 1] = temp [i]; 
		i++;
	}
	TxBuffer[i + 1] = ']';
	myUSART_SendString(USART1,(char *)TxBuffer);
	
}


void myUSART_SendString(USART_TypeDef * myUSART,char * str_p)
{
	while(*str_p != '\0')
	{
		USART_SendData(myUSART, *str_p++);
		while(!USART_GetFlagStatus(myUSART,USART_FLAG_TC));
	}
	
}

//"[LinkStart]" :��ʼ����WIFI    ���ӳɹ��򷵻� "[Connected]"
//"[BreakLink]" :�Ͽ�WIFI		�Ͽ��򷵻� "[UnConnected]"
//"[18.8]" :	�¶�����
void myUSART_ConnectWifi(void)
{
	myUSART_SendString(USART1,(char *)"[LinkStart]");
}
void myUSART_BreakWifi(void)
{
	myUSART_SendString(USART1,(char *)"[BreakLink]");
}




