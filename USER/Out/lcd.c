#include <math.h>
#include <stdlib.h>
#include "com.h"
#include "stm32f10x.h"
#include "lcd.h"

/*****************************************************************
LCD/CS硬件连接：LCD/CS  CE4(NOR/SRAM Bank 4)
*****************************************************************/

/////////////////////////////////////////
#define  HDP	479  /*长*/
#define  HT		531
#define  HPS	43
#define  LPS	8
#define  HPW	10

#define  VDP	271	 /*宽*/
#define  VT		288
#define  VPS	12
#define  FPS	4
#define  VPW	10

void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue);	  //写寄存器
u16  LCD_ReadReg(u16 LCD_Reg);						  //读寄存器
void LCD_WriteRAM_Prepare(void);					  //RAM
void LCD_WriteCom(u16 LCD_Reg);						  //写命令	
//void LCD_WriteRAM(u16 RGB_Code);					  //写RAM
u16  LCD_ReadRAM(void);								  //读RAM
void LCD_PowerOn(void);								  //LCD上电
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_CtrlLinesConfig(void);
void LCD_FSMCConfig(void);
/* 色值 全局变量 */
static  vu16 TextColor = 0x0000, BackColor = 0xFFFF;
/* 读取LCD的DeviceCode 全局变量 */
u16 DeviceCode;

/****************************************************************************
* 名    称：void LCD_Init(void)
* 功    能：LCD屏初始化
* 入口参数：无
* 出口参数：无
* 说    明：LCD屏初始化
* 调用方法：LCD_Init();
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

	LCD_WriteCom(0x00d0);//设置动态背光控制配置 
	LCD_WriteRAM(0x000d);
   
	LCD_WriteCom(0x00F0); //pixel data interface
	LCD_WriteRAM(0x0003); //03:16位   02:24位
	
	//LCD_Clear(Red);

	LCD_WriteCom(0x0029); //display on
	  
	Delay(5);
	LCD_Clear(Black);
}

/************************************************************************************
* 名    称：void LCD_CtrlLinesConfig(void)
* 功    能：lcd屏IO口初始化(FSMC)
* 入口参数：无 
* 出口参数：无
* 说    明：
* 调用方法：
************************************************************************************/
void LCD_CtrlLinesConfig(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	/* Enable FSMC, GPIOD, GPIOE, GPIOF, GPIOG and AFIO clocks */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE |
	                     RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG |
	                     RCC_APB2Periph_AFIO, ENABLE);
	
	/* Set  PD.00(D2), PD.01(D3), PD.04(NOE)--LCD_RD （读）, PD.05(NWE)--LCD_WR（写）,
	      PD.08(D13),PD.09(D14),PD.10(D15),PD.14(D0),PD.15(D1) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | 
								GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15  ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	     /*复用推挽输出*/
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
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;	     /*复用推挽输出*/
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOE, GPIO_Pin_2);
}


/************************************************************************************
* 名    称：void LCD_FSMCConfig(void)
* 功    能：LCD屏的FSMC初始化
* 入口参数：无 
* 出口参数：无
* 说    明：
* 调用方法：
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
* 名    称：void LCD_WriteCom(u16 LCD_Reg)
* 功    能：写LCD 命令
* 入口参数：u16 LCD_Reg：命令值
* 出口参数：无
* 说    明：
* 调用方法：
************************************************************************************/
__INLINE void LCD_WriteCom(u16 LCD_Reg)
{
	/* Write 16-bit Index, then Write Reg */
	LCD->LCD_REG = LCD_Reg;	  
}
/************************************************************************************
* 名    称：void LCD_WriteRAM(u16 RGB_Code)
* 功    能：写LCD RAM
* 入口参数：u16 RGB_Code：颜色模式（5-6-5）
* 出口参数：无
* 说    明：
* 调用方法：
************************************************************************************/
//__INLINE void LCD_WriteRAM(u16 LCD_Data)	
//{
	/* Write 16-bit Reg */
	//LCD->LCD_RAM = LCD_Data;
//}

/************************************************************************************
* 名    称：u16 LCD_ReadReg(u16 LCD_Reg)
* 功    能：读 寄存器 的值
* 入口参数：u8 LCD_Reg        : 寄存器
* 出口参数：要读 寄存器 的值
* 说    明：读 寄存器 的值
* 调用方法：
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
* 名    称：void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue)
* 功    能：寄存器 写值
* 入口参数：u8 LCD_Reg        : 寄存器
*           u16 LCD_RegValue  ：要写入的值
* 出口参数：无
* 说    明：寄存器 写值
* 调用方法：
************************************************************************************/
__inline void LCD_WriteReg(u16 LCD_Reg ,u16 LCD_RegValue)
{
	/* Write 16-bit Index, then Write Reg */
	LCD->LCD_REG = LCD_Reg;
	/* Write 16-bit Reg */
	LCD->LCD_RAM = LCD_RegValue;	
}

/************************************************************************************
* 名    称：void LCD_WriteRAM_Prepare(void)
* 功    能：准备写 RAM
* 入口参数：无
* 出口参数：无
* 说    明：
* 调用方法：
************************************************************************************/
void LCD_WriteRAM_Prepare(void)
{
	LCD_WriteCom(0x002C);
}

/****************************************************************************
* 名    称：void LCD_DisplayPoint(u16 x,u16 y,u16 Color)
* 功    能：在指定座标画点
* 入口参数：x      行座标
*           y      列座标
*           Color  点的颜色
* 出口参数：无
* 说    明：
* 调用方法：LCD_DisplayPoint(10,10,Red);
****************************************************************************/
void LCD_DisplayPoint(u16 x, u16 y, u16 Color)
{
	LCD_SetCursor(x, y);  		/*设置光标位置  */
	LCD_WriteRAM_Prepare();   /*开始写入GRAM */
	LCD_WriteRAM(Color);
}

/****************************************************************************
* 名    称：void LCD_Clear(u16 Color)
* 功    能：LCD清屏
* 入口参数：u16 Color：清屏色值
* 出口参数：无
* 说    明：LCD清屏
* 调用方法：void LCD_Clear(Black)
****************************************************************************/
void LCD_Clear(u16 Color)
{
    unsigned int w;

	/*--------设置刷屏窗口--------*/
	/*  X轴  */
	LCD_WriteCom(0X002A);	
	LCD_WriteRAM(0);	    
	LCD_WriteRAM(0);
	LCD_WriteRAM(HDP>>8);	    
	LCD_WriteRAM(HDP&0XFF);

	/*  Y轴  */
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
* 名    称：void LCD_SetCursor(u16 Xpos, u16 Ypos)
* 功    能：设置光标位置
* 入口参数：u16 Xpos : X轴光标
*           u16 Ypos：Y轴光标
* 出口参数：无
* 说    明：设置光标位置
* 调用方法：LCD_SetCursor(0，0)
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
* 名    称：void LCD_SetDisplayWindow(u16 Xpos, u16 Ypos, u16 Height, u16 Width)
* 功    能：设置一个显示窗口
* 入口参数：u16 Xpos  : 显示窗口X轴
*           u16 Ypos ：显示窗口Y轴
*			u16 Height：显示窗口的长
*			u16 Width：显示窗口的宽
* 出口参数：无
* 说    明：设置一个显示窗口
* 调用方法：LCD_SetDisplayWindow(0,0,240,320)
****************************************************************************/
void LCD_SetDisplayWindow(u16 Xpos, u16 Ypos, u16 Width, u16 Height)	        //设置一个显示窗口
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
