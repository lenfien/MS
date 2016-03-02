#ifndef __GUI
#define __GUI

#include "stm32f10x.h"

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

#define SCREEN_WIDTH		480			//screen width
#define SCREEN_HEIGHT		272			//screen height

typedef struct
{
	u32 xStart;
	u32 yStart;
	u32 Width;
	u32 Height;
}RectType;

typedef struct
{
	s32 x, y;
}PointType;

typedef enum
{
	Ascii_5x8,
	Ascii_6x12,
	Ascii_8x16,
	Ascii_10x19
}FontType;

typedef struct
{
	PointType center;
	s32 			radius;
}CircleType;

#define		RGB565(r, g, b)	(((r&0x1F) << 11) | ((g&0x3F) << 5) | (b&0x1F))

RectType 	Out_SetWindow(RectType* window);

void 			Out_Printc(u32 x, u32 y, const char c);
u32 			Out_prints(u32 x, u32 y, const char *str);
u32 			Out_printf(u32 x, u32 y, const char* format, ...);
void			Out_Putchar(u32 x, u32 y, const char c);
u32 			Out_printft(FontType fontType, u16 foreColor, u16 backColor, u32 x, u32 y, const char* format, ...);

void 			Out_Draw_Point(u32 x, u32 y, u16 color);
void 			Out_Draw_Line(s16 xStart, s16 yStart, s16 xEnd, s16 yEnd, u16 color);
void 			Out_Draw_Lines(PointType points[], u16 n, u16 color);
void 			Out_Draw_ERectangle(RectType* rect, u16 borderColor);
void 			Out_Draw_FRectangle(RectType *rect, u16 fillColor);
void 			Out_Draw_Oval(s16 a, s16 b, s16 x, s16 y, u16 color);
void 			Out_Draw_DirLine(float angle, PointType center, s32 length, u16 color);

FontType 		Out_Config_FontType(FontType type);
u16 			Out_Config_Font_HSpace(u16 space);
u16 			Out_Config_Font_VSpace(u16 space);
u16 			Out_Config_Font_ForeColor(u16 color);
u16 			Out_Config_Font_BackColor(u16 color);

u16 			Out_Get_StringWidth(const char *str);
u16 			Out_Get_FontWidth(void);
u16				Out_Get_FontHeight(void);

#include "btn.h"
#include "cavs.h"

#endif
