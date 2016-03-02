#ifndef __LCD_H
#define __LCD_H

#include "stm32f10x.h"


/* LCD Control pins */
#define CtrlPin_NCS    GPIO_Pin_2   /* PB.02 */
#define CtrlPin_RS     GPIO_Pin_7   /* PD.07 */
#define CtrlPin_NWR    GPIO_Pin_15  /* PD.15 */

/* LCD color */
#define White          0xFFFF
#define Black          0x0000
#define Grey           0xF7DE
#define Blue           0x001F
#define Blue2          0x051F
#define Red            0xF800
#define Magenta        0xF81F
#define Green          0x07E0
#define Cyan           0x7FFF
#define Yellow         0xFFE0

#define Line0          0
#define Line1          24
#define Line2          48
#define Line3          72
#define Line4          96
#define Line5          120
#define Line6          144
#define Line7          168
#define Line8          192
#define Line9          216

#define Horizontal     0x00
#define Vertical       0x01

#define Lcd_Light_ON   (*((volatile unsigned long *) 0x40011010) = Lcd_LightPin)
#define Lcd_Light_OFF  (*((volatile unsigned long *) 0x40011014) = Lcd_LightPin)

#define nCsPin  GPIO_Pin_8
#define RsPin   GPIO_Pin_9
#define nWrPin  GPIO_Pin_10
#define nRdPin  GPIO_Pin_11
#define nRstPin GPIO_Pin_12
#define Lcd_LightPin GPIO_Pin_13

typedef struct
{
  vu16 LCD_REG;
  vu16 LCD_RAM;
} LCD_TypeDef;

#define LCD_BASE    ((u32)(0x60000000 | 0x0C000000))
#define LCD         ((LCD_TypeDef *) LCD_BASE)

/*----- High layer function -----*/
void LCD_Init(void);
void LCD_Clear(u16 Color);
void LCD_SetCursor(u16 Xpos, u16 Ypos);
void LCD_SetDisplayWindow(u16 Xpos, u16 Ypos, u16 Width, u16 Height);

/*----- Medium layer function -----*/

void LCD_WriteRAM_Prepare(void);
//void LCD_WriteRAM(u16 RGB_Code);

#define LCD_WriteRAM(c)	(LCD->LCD_RAM = (c))

u16  LCD_ReadRAM(void);
void LCD_PowerOn(void);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);

/*----- Low layer function -----*/
void LCD_CtrlLinesConfig(void);
void LCD_FSMCConfig(void);

#endif
