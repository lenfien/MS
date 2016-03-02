#include <math.h>
#include <stdlib.h>
#include "com.h"
#include "stm32f10x.h"
#include "lcd.h"

/*****************************************************************
LCD/CSӲ�����ӣ�LCD/CS  CE4(NOR/SRAM Bank 4)
*****************************************************************/

/////////////////////////////////////////
#define  HDP	479  /*��*/
#define  HT		531
#define  HPS	43
#define  LPS	8
#define  HPW	10

#define  VDP	271	 /*��*/
#define  VT		288
#define  VPS	12
#define  FPS	4
#define  VPW	10

void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue);	  //д�Ĵ���
u16  LCD_ReadReg(u16 LCD_Reg);						  //���Ĵ���
void LCD_WriteRAM_Prepare(void);					  //RAM
void LCD_WriteCom(u16 LCD_Reg);						  //д����	
//void LCD_WriteRAM(u16 RGB_Code);					  //дRAM
u16  LCD_ReadRAM(void);								  //��RAM
void LCD_PowerOn(void);								  //LCD�ϵ�
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_CtrlLinesConfig(void);
void LCD_FSMCConfig(void);
/* ɫֵ ȫ�ֱ��� */
static  vu16 TextColor = 0x0000, BackColor = 0xFFFF;
/* ��ȡLCD��DeviceCode ȫ�ֱ��� */
u16 DeviceCode;

/****************************************************************************
* ��    �ƣ�void LCD_Init(void)
* ��    �ܣ�LCD����ʼ��
* ��ڲ�������
* ���ڲ�������
* ˵    ����LCD����ʼ��
* ���÷�����LCD_Init();
****************************************************************************/
void LCD_Init(void)
{
	LCD_CtrlLinesConfig();
	LCD_FSMCConfig();

	Delay(5); /* delay 50 ms */ 
	Delay(5); /* delay 50 ms */

  LCD_WriteCom(0x002b);	
	LCD_WriteRAM(0);

	Delay(5); // delay 50 ms 
	LCD_WriteCom(0x00E2);							//PLL multiplier, set PLL clock to 120M
	LCD_WriteRAM(0x001d);							//N=0x36 for 6.5M, 0x23 for 10M crystal
	LCD_WriteRAM(0x0002);
	LCD_WriteRAM(0x0004);
	
	LCD_WriteCom(0x00E0);							//PLL enable
	LCD_WriteRAM(0x0001);
	Delay(5);
	LCD_WriteCom(0x00E0);
	LCD_WriteRAM(0x0003);
	Delay(5);
	LCD_WriteCom(0x0001);  						//software reset
	Delay(5);
	LCD_WriteCom(0x00E6);							//PLL setting for PCLK, depends on resolution
	LCD_WriteRAM(0x0000);
	LCD_WriteRAM(0x00D9);
	LCD_WriteRAM(0x0016);

	LCD_WriteCom(0x00B0);								//LCD SPECIFICATION
	LCD_WriteRAM(0x0020);
	LCD_WriteRAM(0x0000);
	LCD_WriteRAM((HDP>>8)&0X00FF);			//Set HDP
	LCD_WriteRAM(HDP&0X00FF);
  LCD_WriteRAM((VDP>>8)&0X00FF);			//Set VDP
	LCD_WriteRAM(VDP&0X00FF);
  LCD_WriteRAM(0x0000);

	LCD_WriteCom(0x00B4);								//HSYNC
	LCD_WriteRAM((HT>>8)&0X00FF); 			//Set HT
	LCD_WriteRAM(HT&0X00FF);
	LCD_WriteRAM((HPS>>8)&0X00FF);			//Set HPS
	LCD_WriteRAM(HPS&0X00FF);
	LCD_WriteRAM(HPW);									//Set HPW
	LCD_WriteRAM((LPS>>8)&0X00FF); 			//Set HPS
	LCD_WriteRAM(LPS&0X00FF);
	LCD_WriteRAM(0x0000);

	LCD_WriteCom(0x00B6);								//VSYNC
	LCD_WriteRAM((VT>>8)&0X00FF);   		//Set VT
	LCD_WriteRAM(VT&0X00FF);
	LCD_WriteRAM((VPS>>8)&0X00FF); 			//Set VPS
	LCD_WriteRAM(VPS&0X00FF);
	LCD_WriteRAM(VPW);									//Set VPW
	LCD_WriteRAM((FPS>>8)&0X00FF);			//Set FPS
	LCD_WriteRAM(FPS&0X00FF);

	//=============================================

	//=============================================
	LCD_WriteCom(0x00BA);
	LCD_WriteRAM(0x0005);//0x000F);    //GPIO[3:0] out 1

	LCD_WriteCom(0x00B8);
	LCD_WriteRAM(0x0007);    //GPIO3=input, GPIO[2:0]=output
	LCD_WriteRAM(0x0001);    //GPIO0 normal

	LCD_WriteCom(0x0036); 	//rotation
	LCD_WriteRAM(0x0000);

	Delay(5);

	LCD_WriteCom(0x00BE); //set PWM for B/L
	LCD_WriteRAM(0x0006);
	LCD_WriteRAM(0x0080);
	
	LCD_WriteRAM(0x0001);
	LCD_WriteRAM(0x00f0);
	LCD_WriteRAM(0x0000);
	LCD_WriteRAM(0x0000);

	LCD_WriteCom(0x00d0);//���ö�̬����������� 
	LCD_WriteRAM(0x000d);
   
	LCD_WriteCom(0x00F0); //pixel data interface
	LCD_WriteRAM(0x0003); //03:16λ   02:24λ
	
	//LCD_Clear(Red);

	LCD_WriteCom(0x0029); //display on
	  
	Delay(5);
	LCD_Clear(Black);
}

/************************************************************************************
* ��    �ƣ�void LCD_CtrlLinesConfig(void)
* ��    �ܣ�lcd��IO�ڳ�ʼ��(FSMC)
* ��ڲ������� 
* ���ڲ�������
* ˵    ����
* ���÷�����
************************************************************************************/
void LCD_CtrlLinesConfig(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	/* Enable FSMC, GPIOD, GPIOE, GPIOF, GPIOG and AFIO clocks */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE |
	                     RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG |
	                     RCC_APB2Periph_AFIO, ENABLE);
	
	/* Set  PD.00(D2), PD.01(D3), PD.04(NOE)--LCD_RD ������, PD.05(NWE)--LCD_WR��д��,
	      PD.08(D13),PD.09(D14),PD.10(D15),PD.14(D0),PD.15(D1) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | 
								GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15  ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	     /*�����������*/
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	/* Set PE.07(D4), PE.08(D5), PE.09(D6), PE.10(D7), PE.11(D8), PE.12(D9), PE.13(D10),
	 PE.14(D11), PE.15(D12) as alternate function push pull */
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | 
	                            GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | 
	                            GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	/* Set PF.00(A0 (RS)) as alternate function push pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	
	/* Set PG.12 PE2 (LCD_CS) as alternate function push pull - CE3(LCD /CS) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;	     /*�����������*/
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOE, GPIO_Pin_2);
}


/************************************************************************************
* ��    �ƣ�void LCD_FSMCConfig(void)
* ��    �ܣ�LCD����FSMC��ʼ��
* ��ڲ������� 
* ���ڲ�������
* ˵    ����
* ���÷�����
************************************************************************************/
void LCD_FSMCConfig(void)
{
	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef  FSMC_NORSRAMTimingInitStructure;
	
	/*-- FSMC Configuration ------------------------------------------------------*/
	/*----------------------- SRAM Bank 4 ----------------------------------------*/
	/* FSMC_Bank1_NORSRAM4 configuration */
	FSMC_NORSRAMTimingInitStructure.FSMC_AddressSetupTime = 3;
	FSMC_NORSRAMTimingInitStructure.FSMC_AddressHoldTime = 4;
	FSMC_NORSRAMTimingInitStructure.FSMC_DataSetupTime = 2;
	FSMC_NORSRAMTimingInitStructure.FSMC_BusTurnAroundDuration = 0;
	FSMC_NORSRAMTimingInitStructure.FSMC_CLKDivision = 0;
	FSMC_NORSRAMTimingInitStructure.FSMC_DataLatency = 0;
	FSMC_NORSRAMTimingInitStructure.FSMC_AccessMode = FSMC_AccessMode_A;
	
	/* Color LCD configuration ------------------------------------
	LCD configured as follow:
	- Data/Address MUX = Disable
	- Memory Type = SRAM
	- Data Width = 16bit
	- Write Operation = Enable
	- Extended Mode = Enable
	- Asynchronous Wait = Disable */
	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &FSMC_NORSRAMTimingInitStructure;
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &FSMC_NORSRAMTimingInitStructure;
	
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);  
	
	/* BANK 4 (of NOR/SRAM Bank 1~4) is enabled */
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);
}

/************************************************************************************
* ��    �ƣ�void LCD_WriteCom(u16 LCD_Reg)
* ��    �ܣ�дLCD ����
* ��ڲ�����u16 LCD_Reg������ֵ
* ���ڲ�������
* ˵    ����
* ���÷�����
************************************************************************************/
__INLINE void LCD_WriteCom(u16 LCD_Reg)
{
	/* Write 16-bit Index, then Write Reg */
	LCD->LCD_REG = LCD_Reg;	  
}
/************************************************************************************
* ��    �ƣ�void LCD_WriteRAM(u16 RGB_Code)
* ��    �ܣ�дLCD RAM
* ��ڲ�����u16 RGB_Code����ɫģʽ��5-6-5��
* ���ڲ�������
* ˵    ����
* ���÷�����
************************************************************************************/
//__INLINE void LCD_WriteRAM(u16 LCD_Data)	
//{
	/* Write 16-bit Reg */
	//LCD->LCD_RAM = LCD_Data;
//}

/************************************************************************************
* ��    �ƣ�u16 LCD_ReadReg(u16 LCD_Reg)
* ��    �ܣ��� �Ĵ��� ��ֵ
* ��ڲ�����u8 LCD_Reg        : �Ĵ���
* ���ڲ�����Ҫ�� �Ĵ��� ��ֵ
* ˵    ������ �Ĵ��� ��ֵ
* ���÷�����
************************************************************************************/
u16 LCD_ReadReg(u16 LCD_Reg)
{
	u16 temp;
	/* Write 16-bit Index (then Read Reg) */
	LCD->LCD_REG = LCD_Reg;
	temp = LCD->LCD_RAM;
	temp = LCD->LCD_RAM;
	/* Read 16-bit Reg */
	return temp;  	
}

/************************************************************************************
* ��    �ƣ�void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue)
* ��    �ܣ��Ĵ��� дֵ
* ��ڲ�����u8 LCD_Reg        : �Ĵ���
*           u16 LCD_RegValue  ��Ҫд���ֵ
* ���ڲ�������
* ˵    �����Ĵ��� дֵ
* ���÷�����
************************************************************************************/
__inline void LCD_WriteReg(u16 LCD_Reg ,u16 LCD_RegValue)
{
	/* Write 16-bit Index, then Write Reg */
	LCD->LCD_REG = LCD_Reg;
	/* Write 16-bit Reg */
	LCD->LCD_RAM = LCD_RegValue;	
}

/************************************************************************************
* ��    �ƣ�void LCD_WriteRAM_Prepare(void)
* ��    �ܣ�׼��д RAM
* ��ڲ�������
* ���ڲ�������
* ˵    ����
* ���÷�����
************************************************************************************/
void LCD_WriteRAM_Prepare(void)
{
	LCD_WriteCom(0x002C);
}

/****************************************************************************
* ��    �ƣ�void LCD_DisplayPoint(u16 x,u16 y,u16 Color)
* ��    �ܣ���ָ�����껭��
* ��ڲ�����x      ������
*           y      ������
*           Color  �����ɫ
* ���ڲ�������
* ˵    ����
* ���÷�����LCD_DisplayPoint(10,10,Red);
****************************************************************************/
void LCD_DisplayPoint(u16 x, u16 y, u16 Color)
{
	LCD_SetCursor(x, y);  		/*���ù��λ��  */
	LCD_WriteRAM_Prepare();   /*��ʼд��GRAM */
	LCD_WriteRAM(Color);
}

/****************************************************************************
* ��    �ƣ�void LCD_Clear(u16 Color)
* ��    �ܣ�LCD����
* ��ڲ�����u16 Color������ɫֵ
* ���ڲ�������
* ˵    ����LCD����
* ���÷�����void LCD_Clear(Black)
****************************************************************************/
void LCD_Clear(u16 Color)
{
    unsigned int w;

	/*--------����ˢ������--------*/
	/*  X��  */
	LCD_WriteCom(0X002A);	
	LCD_WriteRAM(0);	    
	LCD_WriteRAM(0);
	LCD_WriteRAM(HDP>>8);	    
	LCD_WriteRAM(HDP&0XFF);

	/*  Y��  */
    LCD_WriteCom(0X002B);	
	LCD_WriteRAM(0);	    
	LCD_WriteRAM(0);
	LCD_WriteRAM(VDP>>8);	    
	LCD_WriteRAM(VDP&0X00FF);

	LCD_WriteCom(0X002C);	

	for(w = 0; w < 130560/*272*480*/; w++)
	{
		LCD_WriteRAM(Color);
	}
}

/****************************************************************************
* ��    �ƣ�void LCD_SetCursor(u16 Xpos, u16 Ypos)
* ��    �ܣ����ù��λ��
* ��ڲ�����u16 Xpos : X����
*           u16 Ypos��Y����
* ���ڲ�������
* ˵    �������ù��λ��
* ���÷�����LCD_SetCursor(0��0)
****************************************************************************/
void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
	LCD->LCD_REG = (0x002A);	
	LCD->LCD_RAM = (Xpos>>8);	    
	LCD->LCD_RAM = (Xpos&0x00ff);
	LCD->LCD_RAM = (479>>8);	    
	LCD->LCD_RAM = (479&0x00ff);
  LCD->LCD_REG = (0x002b);	
	LCD->LCD_RAM = (Ypos>>8);	    
	LCD->LCD_RAM = (Ypos&0x00ff);
	LCD->LCD_RAM = (271>>8);	    
	LCD->LCD_RAM = (271&0x00ff);

}

/****************************************************************************
* ��    �ƣ�void LCD_SetDisplayWindow(u16 Xpos, u16 Ypos, u16 Height, u16 Width)
* ��    �ܣ�����һ����ʾ����
* ��ڲ�����u16 Xpos  : ��ʾ����X��
*           u16 Ypos ����ʾ����Y��
*			u16 Height����ʾ���ڵĳ�
*			u16 Width����ʾ���ڵĿ�
* ���ڲ�������
* ˵    ��������һ����ʾ����
* ���÷�����LCD_SetDisplayWindow(0,0,240,320)
****************************************************************************/
void LCD_SetDisplayWindow(u16 Xpos, u16 Ypos, u16 Width, u16 Height)	        //����һ����ʾ����
{
	LCD_WriteCom(0X002A);
	LCD_WriteRAM(Xpos>>8);
	LCD_WriteRAM(Xpos&0X00FF);
	LCD_WriteRAM((Xpos+Width-1)>>8);
	LCD_WriteRAM((Xpos+Width-1)&0X00FF);
	
	LCD_WriteCom(0X002B);	
	LCD_WriteRAM(Ypos>>8);	
	LCD_WriteRAM(Ypos&0X00FF);
	LCD_WriteRAM((Ypos+Height)>>8);	
	LCD_WriteRAM((Ypos+Height)&0X00FF);			
}
