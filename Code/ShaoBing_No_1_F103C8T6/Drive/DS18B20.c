////���ʹ�ã�

//int main(void)
//{
//     float t;
//     ds18b20_init();
//     t = ds18b20_read();
//     printf("�¶� = %05.1f", t);
//}


#include "DS18B20.h" 
#include "delay.h"
float DS18B20_Temperature;

#define EnableINT()  
#define DisableINT()

#define DS_PORT   GPIOB           //DS18B20���ӿ�
#define DS_DQIO   GPIO_Pin_0      //GPIOA2

#define DS_RCC_PORT  RCC_APB2Periph_GPIOA

#define DS_PRECISION 0x7f   //�������üĴ��� 1f=9λ; 3f=10λ; 5f=11λ; 7f=12λ;
#define DS_AlarmTH  0x64
#define DS_AlarmTL  0x8a
#define DS_CONVERT_TICK 1000

#define ResetDQ() GPIO_ResetBits(DS_PORT,DS_DQIO)
#define SetDQ()  GPIO_SetBits(DS_PORT,DS_DQIO)
#define GetDQ()  GPIO_ReadInputDataBit(DS_PORT,DS_DQIO)
 
 
enum DS18B20_STATE DS18B20_State = DS18B20_CONNECT_FAILE;

//void DelayUs(u32 Nus) 
//{  
//	SysTick->LOAD=Nus*9;          //ʱ�����       
//	SysTick->CTRL|=0x01;             //��ʼ����     
//	while(!(SysTick->CTRL&(1<<16))); //�ȴ�ʱ�䵽��  
//	SysTick->CTRL=0X00000000;        //�رռ����� 
//	SysTick->VAL=0X00000000;         //��ռ�����      
//} 

 

unsigned char ResetDS18B20(void)
{
	unsigned char resport,i = 0;
	SetDQ();
	DelayUs(50);

	ResetDQ();
	DelayUs(500);  //500us ����ʱ���ʱ�䷶Χ���Դ�480��960΢�룩
	SetDQ();
	DelayUs(40);  //40us
	//resport = GetDQ();
	while(GetDQ())
	{
		DelayMs(10);
		i ++;
		if(i >= 10) 
		{
			DS18B20_State = DS18B20_CONNECT_FAILE;
			return 0;
		}
			
	};  //�ȴ�������� ///////////
	DelayUs(500);  //500us
	SetDQ();
	DS18B20_State = DS18B20_CONNECT_SUCCESS;
	return resport;
}

void DS18B20WriteByte(unsigned char Dat)
{
	unsigned char i;
	for(i=8;i>0;i--)
	{
		ResetDQ();     //��15u���������������ϣ�DS18B20��15-60u����
		DelayUs(5);    //5us
		if(Dat & 0x01)
		SetDQ();
		else
		ResetDQ();
		DelayUs(65);    //65us
		SetDQ();
		DelayUs(2);    //������λ��Ӧ����1us
		Dat >>= 1; 
	} 
}


unsigned char DS18B20ReadByte(void)
{
	unsigned char i,Dat;
	SetDQ();
	DelayUs(5);
	for(i=8;i>0;i--)
	{
		Dat >>= 1;
		ResetDQ();     //�Ӷ�ʱ��ʼ�������ź��߱�����15u�ڣ��Ҳ�������������15u�����
		DelayUs(5);   //5us
		SetDQ();
		DelayUs(5);   //5us
		if(GetDQ())
		Dat|=0x80;
		else
		Dat&=0x7f;  
		DelayUs(65);   //65us
		SetDQ();
	}
	return Dat;
}


void ReadRom(unsigned char *Read_Addr)
{
 unsigned char i;

 DS18B20WriteByte(ReadROM);
  
 for(i=8;i>0;i--)
 {
  *Read_Addr=DS18B20ReadByte();
  Read_Addr++;
 }
}



void DS18B20Init(unsigned char Precision,unsigned char AlarmTH,unsigned char AlarmTL)
{
	u8 i = 0;
	DisableINT();
	ResetDS18B20();
	DS18B20WriteByte(SkipROM); 
	DS18B20WriteByte(WriteScratchpad);
	DS18B20WriteByte(AlarmTL);
	DS18B20WriteByte(AlarmTH);
	DS18B20WriteByte(Precision);

	ResetDS18B20();
	DS18B20WriteByte(SkipROM); 
	DS18B20WriteByte(CopyScratchpad);
	EnableINT();

	while(!GetDQ())
	{
		DelayMs(10);
		i ++;
		if(i >= 10) 
		{
			DS18B20_State = DS18B20_CONNECT_FAILE;
			return ;
		}
			
	};  //�ȴ�������� ///////////
	DS18B20_State = DS18B20_CONNECT_SUCCESS;
}


void DS18B20StartConvert(void)
{
 DisableINT();
 ResetDS18B20();
 DS18B20WriteByte(SkipROM); 
 DS18B20WriteByte(StartConvert); 
 EnableINT();
}

void DS18B20_Configuration(void)
{
 GPIO_InitTypeDef GPIO_InitStructure;
 
 RCC_APB2PeriphClockCmd(DS_RCC_PORT, ENABLE);

 GPIO_InitStructure.GPIO_Pin = DS_DQIO;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; //��©���
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //2Mʱ���ٶ�
 GPIO_Init(DS_PORT, &GPIO_InitStructure);
}


void ds18b20_init(void)
{
 DS18B20_Configuration();
 DS18B20Init(DS_PRECISION, DS_AlarmTH, DS_AlarmTL);
 DS18B20StartConvert();
}


float ds18b20_read(void)
{
	unsigned char DL, DH;
	unsigned short temperatureData;
	float temperature;

	DisableINT();
	DS18B20StartConvert();
	ResetDS18B20();
	DS18B20WriteByte(SkipROM); 
	DS18B20WriteByte(ReadScratchpad);
	DL = DS18B20ReadByte();
	DH = DS18B20ReadByte(); 
	EnableINT();

	temperatureData = DH;
	temperatureData <<= 8;
	temperatureData |= DL;

	temperature = (float)((float)temperatureData * 0.0625); //�ֱ���Ϊ0.0625��

	return  temperature;
}

void DS18B20_UpdataTemp(void)
{
	float temp = 0;
	temp += ds18b20_read();

	DS18B20_Temperature = temp;
}

void DS18B20_UpdataTempx5Average(void)
{
	float temp = 0;
	temp += ds18b20_read();
	temp += ds18b20_read();
	temp += ds18b20_read();
	temp += ds18b20_read();
	temp += ds18b20_read();
	temp = temp / 5;
	DS18B20_Temperature = temp;
}

