#ifndef __CANVAS
#define __CANVAS

#include "evt.h"
#include "gui.h"
#include "com.h"

typedef struct CanvasTag
{
	EventSourceType events;
	void (*EnterHandler)(struct CanvasTag*, PointType*);
	void (*LeaveHandler)(struct CanvasTag*, PointType*);
	const char* 		name;
	RectType				rect;
	bool						isEnter;
	u16							backColor;
	bool 						isActive;
}CanvasDefType;

typedef void (*CanvasHandler)(struct CanvasTag*, PointType*);
typedef CanvasDefType* Canvas;

Canvas 	Canvas_Create(CanvasDefType* cvs);
void 		Canvas_Register(Canvas cvs);
void 		Canvas_Deregister(Canvas cvs);
void 		Canvas_Delete(Canvas cvs);
void 		Canvas_Draw(Canvas cvs);

void 		Canvas_Draw_Point(Canvas cvs, u16 x, u16 y, u16 color);
void 		Canvas_Draw_Points(Canvas cvs, PointType* points, u16 amount, u16 color);
void 		Canvas_Draw_Line(Canvas cvs, PointType* pStart, PointType* pEnd, u16 color);
void 		Canvas_Draw_Lines(Canvas cvs, PointType points[], u16 n, u16 color);
void 		Canvas_Draw_DirLine(Canvas cvs, float angle, PointType center, s32 length, u16 color);
void 		Canvas_Draw_Circle(Canvas cvs, PointType center, s32 radius, u16 color);
void 		Canvas_Debug(void);

#endif
