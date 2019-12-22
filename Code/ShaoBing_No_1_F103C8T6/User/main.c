/************************************************************************************
*  Copyright (c), 2019, LXG.
*
* FileName		:main.c
* Author		:firestaradmin
* Version		:1.0
* Date			:2019.12.8
* Description	:�������
* History		:
*
*
*************************************************************************************/
#include <stm32f10x.h>  
#include "stm32f10x_it.h"
#include "OLED_I2C_Buffer.h"
#include "delay.h"
#include "DS18B20.h"
#include "myUSART.h"
#include "myShaoBingApp.h"
#include "myKey.h"
#include "AT24C04.h"


unsigned short TIM4_Timer5MsCounter = 0;	//5MS������
unsigned char TIM4_Timer1SCounter = 0;		//1S������
unsigned char OLED_CurveNeedDrawFlag = 0;	//OLED������Ҫ���Ʊ�־
unsigned char DS18B20_NeedUpdataFlag = 0;	//DS18B20������Ҫ���±�־
unsigned char alarmNeedProcessFlag = 0;		//���������账���־
unsigned char alarmStatusFlag = 0;			//����״̬��־
unsigned char alarmExceededFlag = 0;		//�������������ޱ�־
unsigned char samplingPeriod5ms = 100;		//��������
unsigned char voiceLightAlarmFlag = 0;		//����ʹ�ܱ�־
unsigned char wifiFlag = 0;					//WIFIʹ�ܱ�־


void TIM4_Int_Init(u16 arr,u16 psc);//TIM4��ʼ��
void SYS_Configuration(void);//��ʼ��
void alarm_Init(void);//������ʼ��
void alarmStop(void);//ֹͣ���ⱨ��
void alarmGOGOGO(void);//�������ⱨ��
void readValue(void);//��ȡAT24C04��ֵ

int main()
{
	SYS_Configuration(); //��ʼ��
	OLED_STARTUP_VIDEO();//��������
	DS18B20_UpdataTempx5Average();	//���һ��DS18B20�¶�����
	readValue();//��ȡAT24C04��ֵ
	while(1)
	{	
		//while(1)��ѭ���� ˳��ִ�����½��̣����̻������谭��ʹ��ȫ�ֱ�����Ϣ���ݻ��ƣ��ɱ�֤ʵʱ�ԡ�
		myKey_GetKeyValue();//��ȡ��ֵ
		app_Handle_KeyState();//��ֵ����
		app_Updata_Interface();//�������
		app_Dynamic_Display();//��̬��ʾ
		
		//
		if(DS18B20_NeedUpdataFlag)//DS18B20��ȡ�¶�����
		{
			if(DS18B20_State == DS18B20_CONNECT_SUCCESS)
				DS18B20_UpdataTempx5Average();	
		}
		
		if(DS18B20_Temperature > alarmTEMPHIGH || DS18B20_Temperature < alarmTEMPLOW)//�жϱ����¶��������Ƿ񳬳�
		{
			alarmExceededFlag = 1;
		}
		else
		{
			alarmExceededFlag = 0;
		}
			
		if(httpNeedPostFlag && wifiIsConnected && wifiFlag)//�ж��Ƿ���ҪWIFI-POST
		{
			myUSART_SentTempData();
			httpNeedPostFlag = 0;
		}
		
		if(voiceLightAlarmFlag && alarmNeedProcessFlag && alarmExceededFlag)//�ж��Ƿ�Ҫ�򿪾���
		{
			alarmNeedProcessFlag = 0;
			if(alarmStatusFlag == 0)
			{
				alarmGOGOGO();
			}
			else
			{
				alarmStop();
			}
		}
		else if(!(voiceLightAlarmFlag && alarmExceededFlag))
		{
			alarmStop();
		}

	}
	
}




//ϵͳ��ʼ��
void SYS_Configuration(void)
{
	//--**********--�ڵĴ��벻��ɾ�� �������ϵ����Ҫ�ֶ���λһ��
	//				�˰�Ӳ����·����(�ʵ���ʱ���ȴ��������������)
	//--********************************-----
	u32 i,j;
	for(i = 0; i <= 65535; i++ )
	{
		for(j = 0; j <= 50; j++ );
	}
	SystemInit();
	//--********************************-----
	
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//����NVIC�жϷ���2  ��ռ2λ:��Ӧ2λ 
	AT24C02_I2C_Configuration();
	DelayInit();
	OLED_Init();
	ds18b20_init();
	UART1_init();
	myKey_Init();
	
	//DelayS(2);
	TIM4_Int_Init(1000,360);//5ms OneTime
	alarm_Init();
 
}



void TIM4_IRQHandler(void)   //TIM4�ж�
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)  //���TIM3�����жϷ������
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update  );  //���TIMx�����жϱ�־ 
			
		//TIM_Cmd(TIM4, DISABLE); 
		TIM4_Timer5MsCounter++;
		if(TIM4_Timer5MsCounter % samplingPeriod5ms == 0)
		{
			
			OLED_CurveNeedDrawFlag = 1;
			DS18B20_NeedUpdataFlag = 1;
			
			//httpNeedPostFlag = 1;
			
		}
		if(TIM4_Timer5MsCounter % 10 == 0)
		{
			alarmNeedProcessFlag = 1;
		}
		if(TIM4_Timer5MsCounter % 200 == 0)
		{
			httpNeedPostFlag = 1;
		}
		if(TIM4_Timer5MsCounter >= 65500)
		{
			TIM4_Timer5MsCounter = 0;
		}
	}
}


//����ʱ��ѡ��ΪAPB1��2������APB1Ϊ36M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//һ���жϵ�ʱ��Ϊt��t = (arr * psc / APB1*2) * 1000 ms
void TIM4_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //ʱ��ʹ��
	
	TIM_DeInit(TIM4);
	//��ʱ��TIM4��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler = psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE ); //ʹ��ָ����TIM4�ж�,��������ж�
 
	//�ж����ȼ�NVIC����
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;  //TIM4�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  //��ռ���ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;  //�����ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���
 
 
	TIM_Cmd(TIM4, ENABLE);  //ʹ��TIM4					 
}


void alarm_Init(void)
{
	//PA1 LED
	//PC13 Alarm
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_13;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	
}


void alarmStop(void)
{
	GPIO_SetBits(GPIOC,GPIO_Pin_13);
	GPIO_ResetBits(GPIOA,GPIO_Pin_1);
	alarmStatusFlag = 0;
}

void alarmGOGOGO(void)
{
	GPIO_ResetBits(GPIOC,GPIO_Pin_13);
	GPIO_SetBits(GPIOA,GPIO_Pin_1);
	alarmStatusFlag = 1; 
}



  


void readValue()
{

	TEMPHIGH = *((float*)AT24C04_Read_Page(AT24C04_PAGE0, TEMPHIGH_ADDR, 4));
	TEMPLOW = *((float*)AT24C04_Read_Page(AT24C04_PAGE0, TEMPLOW_ADDR, 4));
	samplingPeriod5ms = *((u8*)AT24C04_Read_Page(AT24C04_PAGE0, samplingPeriod5ms_ADDR, 1));
	alarmTEMPHIGH = *((float*)AT24C04_Read_Page(AT24C04_PAGE0, alarmTEMPHIGH_ADDR, 4));
	alarmTEMPLOW = *((float*)AT24C04_Read_Page(AT24C04_PAGE0, alarmTEMPLOW_ADDR, 4));
	
	wifiFlag = AT24C04_Read_Byte(AT24C04_PAGE0, wifiFlag_ADDR);
	voiceLightAlarmFlag = AT24C04_Read_Byte(AT24C04_PAGE0, voiceLightAlarmFlag_ADDR);
	
}










