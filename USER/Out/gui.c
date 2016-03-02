
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "stm32f10x.h"

#include "lcd.h"
#include "fonts.h"
#include "gui.h"
#include "com.h"



#define FONTAMOUNT_X		(SCREEN_WIDTH/(Font_Width + Font_HSpace))				//max amount of fonts in x oritation
#define FONTAMOUNT_Y  	(SCREEN_HEIGHT/(Font_Height + FOnt_VSpace))			//max amount of fonts in y oritation
#define TAB_TO_SPACE		2				//'\t' means TAB_TO_SPACE spaces

u16  	Font_Width = 5;
u16		Font_Height = 8;
u16  	Font_HSpace = 0;					//space betwen 2 char in horizont 
u16 	Font_VSpace = 0;					//space between 2 chars in vertical
vu16 	Font_ForeColor = 0xFFFF;	//font's fore color
vu16 	Font_BackColor = 0x0000; 	//font's background color

vu32 CursorPosition_x = 0;	
vu32 CursorPosition_y = 0;

static char buffer[128];
static const unsigned char 	*FontLib = (unsigned char*)&(ascii_5x8[0][0]);
static FontType		CurrentFontType = Ascii_5x8;

/******************************************************************************************/
/*-------------------------------------BASE-----------------------------------------------*/
/******************************************************************************************/
RectType Out_SetWindow(RectType* window)
{
	static RectType preWindow = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
	
	RectType preWindowBack = preWindow;
	preWindow = *window;
	LCD_SetDisplayWindow(window->xStart, window->yStart, window->Width, window->Height);
	return preWindowBack;
}

















/******************************************************************************************/
/*-------------------------------------CONFIG---------------------------------------------*/
/******************************************************************************************/
FontType Out_Config_FontType(FontType type)
{
	FontType preTypeBack = CurrentFontType;
	CurrentFontType = type;
	
	switch(type)
	{
		case Ascii_8x16:
			Font_Width = 8;
			Font_Height = 16;
			Font_HSpace = 1;
			FontLib = (unsigned char*)&(ascii_8x16[0][0]);
			break;
		case Ascii_10x19:
			Font_Width = 10;
			Font_Height = 19;
			FontLib = (unsigned char*)&(ascii_10x19[0][0]);
			break;
		case Ascii_5x8:
			Font_Width = 5;
			Font_Height = 8;
			Font_HSpace = 2;
			FontLib = (unsigned char*)&(ascii_5x8[0][0]);
			break;
		case Ascii_6x12:
			Font_Width = 6;
			Font_Height = 12;
			Font_HSpace = 2;
			FontLib = (unsigned char*)&(ascii_6x12[0][0]);
			break;
		default:
			break;
	}
	
	return preTypeBack;
}

u16 Out_Config_Font_HSpace(u16 space)
{
	u16 old = Font_HSpace;
	Font_HSpace = space;
	return old;
}

u16 Out_Config_Font_VSpace(u16 space)
{
	u16 old = Font_VSpace;
	Font_VSpace = space;
	return old;
}

u16 Out_Config_Font_ForeColor(u16 color)
{
	u16 old = Font_ForeColor;
	Font_ForeColor = color;
	return old;
}

u16 Out_Config_Font_BackColor(u16 color)
{
	u16 old = Font_BackColor;
	Font_BackColor = color;
	return old;
}


u16 Out_Get_StringWidth(const char *str)
{
	return strlen(str) * (Font_Width + Font_HSpace);
}

u16 Out_Get_FontWidth()
{
	return Font_Width; 
}

u16 Out_Get_FontHeight()
{
	return Font_Height;
}






/******************************************************************************************/
/*-------------------------------------TEXT-----------------------------------------------*/
/******************************************************************************************/

void Out_Putchar(u32 x, u32 y, const char c)
{
	const unsigned char *pCharLib = FontLib + (c - ' ') * Font_Height * (Font_Width/9 + 1);
	u32 indexHeight = 0, indexWidth;
	u8 lp = *pCharLib;
	u32 msk;
	RectType window, preWindow;
	
	window.xStart = x;
	window.yStart = y;
	window.Width = Font_Width;
	window.Height = Font_Height;
	
	preWindow = Out_SetWindow(&window);
	
	msk = __MASK_IRQ(1);
	
	LCD_WriteRAM_Prepare();
	
	for(indexHeight = 0; indexHeight < Font_Height; indexHeight ++)
	{
		for(indexWidth = 0; indexWidth < Font_Width; indexWidth ++)
		{
			if(indexWidth%8 == 0)
			{
				lp = *(pCharLib++);
			}
			
			if(lp & 0x80)
				LCD_WriteRAM(Font_ForeColor);
			else
				LCD_WriteRAM(Font_BackColor);
			
			lp <<= 1;
		}
	}
	
	Out_SetWindow(&preWindow);
	
	__MASK_IRQ(msk);
}




/****
    *@brief print a char and update the CursorPosition according to the type of c.
	*	  if 'c' is control character such as '\n' '\r' '\t', function will 
	*	  turn it to its corresponding amount of space.
    */

void Out_Printc(u32 x, u32 y, const char c)
{
	CursorPosition_x = x;
	CursorPosition_y = y;
	
	if(c == '\n')
	{
		CursorPosition_x = 0;
		
		CursorPosition_y += Font_Height + Font_VSpace;
		
		if(CursorPosition_y > SCREEN_HEIGHT - Font_Height)
			CursorPosition_y = 0;
		
		return;
	}
	
	if (c == '\t')
	{
		int x_temp = x;
		
		for(x_temp = x + 1; x_temp < SCREEN_WIDTH; x_temp ++)
		{
			if(x_temp % (TAB_TO_SPACE * (Font_Width + Font_HSpace)) == 0)
				break;
			else
			{
				u16 index;
				for(index = CursorPosition_y; index < CursorPosition_y + Font_Height; index ++);
						//Out_Draw_Point(x_temp, index, Font_BackColor);
			}
		}
		
		CursorPosition_x = x_temp;
		
		if(CursorPosition_x > SCREEN_WIDTH - (Font_Width + Font_HSpace))
			CursorPosition_x = 0;
		
		return;
	}
	
	if(c == '\r')
	{
		CursorPosition_x = 0;
		return;
	}
	
	Out_Putchar(x, y, c);
	
	CursorPosition_x +=  Font_Width + Font_HSpace;
	
	if(CursorPosition_x > SCREEN_WIDTH - Font_Width)
	{
			CursorPosition_x = 0;
			CursorPosition_y += Font_Height + Font_VSpace;
	}
	
	if(CursorPosition_y > SCREEN_HEIGHT - Font_Height)
			CursorPosition_y = 0;
}




/**
	*@brief print string
	*@
  */
u32 Out_prints(u32 x, u32 y, const char *str)
{
	u32 len = strlen(str);
	
	CursorPosition_x = x;
	CursorPosition_y = y;
	
	if(len == 0)
		return len;
	
	while(*str)
	{
		Out_Printc(CursorPosition_x, CursorPosition_y, *str);
		str ++;
	}
	
	return len;
}




u32 Out_printf(u32 x, u32 y, const char* format, ...)
{
	u32 len;
	u32 msk;
	va_list arg_list;
	va_start(arg_list, format);
	
	msk = __MASK_IRQ(1);
	len = vsprintf(buffer, format, arg_list);
	Out_prints(x, y, buffer);
	 __MASK_IRQ(msk);
	
	return len;
}

u32 Out_printft(FontType fontType, u16 foreColor, u16 backColor, u32 x, u32 y, const char* format, ...)
{
	u32 len;
	u32 msk;
	FontType oldType;
	u16 oldColor;
	u16 oldBack;
	va_list arg_list;
	va_start(arg_list, format);
	
	msk = __MASK_IRQ(1);
	oldType = Out_Config_FontType(fontType);
	oldColor = Out_Config_Font_ForeColor(foreColor);
	oldBack = Out_Config_Font_BackColor(backColor);
	
	len = vsprintf(buffer, format, arg_list);
	Out_prints(x, y, buffer);
	
	Out_Config_FontType(oldType);
	Out_Config_Font_ForeColor(oldColor);
	Out_Config_Font_BackColor(oldBack);
	
	__MASK_IRQ(msk);
	
	return len;
}


/*************************************************************************/
/*--------------------------Graphic--------------------------------------*/
/*************************************************************************/
/**
  *@brief draw point
	*/
__inline void Out_Draw_Point(u32 x, u32 y, u16 color)
{
	u32 msk = __MASK_IRQ(1);
	LCD_SetCursor(x, y);
	LCD_WriteRAM_Prepare();
	LCD_WriteRAM(color);
	__MASK_IRQ(msk);
}

/**
  *@brief draw line
	*/

void Out_Draw_Line(s16 xStart, s16 yStart, s16 xEnd, s16 yEnd, u16 color)
{
	s16 x, y;
	double k;
	
	if(xStart == xEnd)
	{
		for(y = MIN(yStart, yEnd); y <= MAX(yStart, yEnd); y++)
			Out_Draw_Point(xStart, y, color);
		
		return;
	}
	
	if(ABS(xStart - xEnd) > ABS(yStart - yEnd))
	{
		k = (double)(yEnd - yStart)/(double)(xEnd - xStart);
	
		for(x= MIN(xStart, xEnd); x <= MAX(xStart, xEnd); x++)
		{
			u16 y = k * (x - xStart) + yStart;
			Out_Draw_Point(x, y, color);
		}
	}
	else
	{
		k = (double)(xEnd - xStart)/(double)(yEnd - yStart);
	
		for(y= MIN(yStart, yEnd); y <= MAX(yStart, yEnd); y++)
		{
			u16 x = k * (y - yStart) + xStart;
			Out_Draw_Point(x, y, color);
		}
	}
}

/**
  *@brief draw lines 
  */
void Out_Draw_Lines(PointType points[], u16 n, u16 color)
{
	int index;
	
	if(n <= 1)
		return ;
	
	for(index = 0; index < n - 1; index ++)
	{
		Out_Draw_Line(points[index].x, points[index].y, points[index + 1].x, points[index + 1].y, color);
	}
}

/**
  *@brief draw empty rectangle
  */
void Out_Draw_ERectangle(RectType* rect, u16 borderColor)
{
	PointType points[5]; 
	
	points[0].x = rect->xStart;
	points[0].y = rect->yStart;
	
	points[1].x = rect->xStart + rect->Width - 1;
	points[1].y  = rect->yStart;
	
	points[2].x = rect->xStart + rect->Width - 1;
	points[2].y	= rect->yStart + rect->Height - 1;
	
	points[3].x = rect->xStart;
	points[3].y = rect->yStart + rect->Height;
	
	points[4].x = rect->xStart;
	points[4].y = rect->yStart;
	
	Out_Draw_Lines(points, 5, borderColor);
}

/**
  *@brief draw Fill rectangle
  */
void Out_Draw_FRectangle(RectType *rect, u16 fillColor)
{
	RectType oldWindow = Out_SetWindow(rect);
	u32 amount = rect->Width * rect->Height;
	u32 msk = __MASK_IRQ(1);
	LCD_WriteRAM_Prepare();
	
	while(amount--)
	{
		LCD_WriteRAM(fillColor);
	}
	
	
	
	Out_SetWindow(&oldWindow);
	__MASK_IRQ(msk);
}


/**
	*	draw oval
	*/
void Out_Draw_Oval(s16 a, s16 b, s16 x, s16 y, u16 color)
{
	u16 xIndex = x - a;
  u16 yV = 0;
  u16 _a = a * a;
  u16 _b = b * b;
	
  for(xIndex = x - a; xIndex <= x + a; xIndex ++)
  {
    yV = sqrt((1.0 - (((float)((x - xIndex)*(x - xIndex)))/((float)_a))) * (float)_b);
    Out_Draw_Point(xIndex, y - yV, color);
    Out_Draw_Point(xIndex, y + yV, color);
  }
	
  for(xIndex = y - b; xIndex <= y + b; xIndex ++)
  {
    yV = sqrt((1.0 - (((float)((y - xIndex)*(y - xIndex)))/((float)_b))) * (float)_a);
		Out_Draw_Point(x + yV, xIndex, color);
    Out_Draw_Point(x - yV, xIndex, color);
  }
}



void Out_Draw_DirLine(float angle, PointType center, s32 length, u16 color)
{
	PointType lineEnd;
	
	lineEnd.x = cos(angle/180*3.1415926) * length + center.x;
	lineEnd.y = sin(angle/180*3.1415926) * length + center.y;
	
	Out_Draw_Line(center.x, center.y, lineEnd.x, lineEnd.y, color);
}
	







