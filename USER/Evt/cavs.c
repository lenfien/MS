#include <stdio.h>
#include <stdlib.h>

#include "cavs.h"
#include "gui.h"
#include "touch.h"
#include "evt.h"
#include "com.h"

static bool Canvas_IsMeetCondition(EventSource es, void *param)
{
	Canvas cvs = (Canvas)es;
	PointType* p = (PointType*)param;
	
	if(Touch_GetState()->isTouch == FALSE)
		return FALSE;
	
	return (bool)((p->x > cvs->rect.xStart && (p->x < (cvs->rect.xStart + cvs->rect.Width))) && 
		((p->y > cvs->rect.yStart) && (p->y < (cvs->rect.yStart + cvs->rect.Height))));
}


static void Canvas_ActiveHandler(EventSource es, void *param)
{

	Canvas cvs = (Canvas)es;
	PointType* p = (PointType*)param;
	
	cvs->isEnter = TRUE;
	p->x -= cvs->rect.xStart;
	p->y -= cvs->rect.yStart;
	
	if(cvs->EnterHandler)
		cvs->EnterHandler(cvs, p);	
}

static void Canvas_DeActiveHandler(EventSource es, void *param)
{
	Canvas cvs = (Canvas)es;
	PointType* p = (PointType*)param;
	
	if(cvs->isEnter)
	{
			p->x -= cvs->rect.xStart;
			p->y -= cvs->rect.yStart;
			
			if(cvs->LeaveHandler)
				cvs->LeaveHandler(cvs, p);
			
			cvs->isEnter = FALSE;
	}
}


Canvas Canvas_Create(CanvasDefType* cvst)
{
	Canvas cvs = (Canvas)malloc(sizeof(CanvasDefType));
	
	assert_param(cvst);
	assert_param(cvs != 0);
	
	*cvs = *cvst;
	
	cvs->events.IsMeetCondition = Canvas_IsMeetCondition;
	cvs->events.ActiveHandler = Canvas_ActiveHandler;
	cvs->events.DeactiveHandler = Canvas_DeActiveHandler;
	cvs->events.privateMember = (void*)0;
	cvs->isEnter = FALSE;
	cvs->isActive = FALSE;
	
	return cvs;
}

void Canvas_Register(Canvas cvs)
{
	assert_param(cvs);
	
	if(cvs->isActive == FALSE)
	{
		Evt_Listen((EventSource)cvs);
		cvs->isActive = TRUE;
	}
}

void Canvas_Deregister(Canvas cvs)
{
	assert_param(cvs);
	if(cvs->isActive)
	{
		Evt_Abandon((EventSource)cvs);
		cvs->isActive = FALSE;
	}
}

void Canvas_Delete(Canvas cvs)
{
	assert_param(cvs);
	Canvas_Deregister(cvs);
	free(cvs);
}


void Canvas_Draw_Point(Canvas cvs, u16 x, u16 y, u16 color)
{
	x += cvs->rect.xStart;
	y += cvs->rect.yStart;
	
	Out_Draw_Point(x, y , color);
}

void Canvas_Draw_Points(Canvas cvs, PointType* points, u16 amount, u16 color)
{
	u16 index;
	
	for(index = 0; index < amount; index ++)
		Canvas_Draw_Point(cvs, points[index].x, points[index].y, color);
}

void Canvas_Draw_Line(Canvas cvs, PointType* pStart, PointType* pEnd, u16 color)
{
	PointType Start = *pStart, End = *pEnd;
	Start.x += cvs->rect.xStart;
	Start.y += cvs->rect.yStart;
	
	End.x += cvs->rect.xStart;
	End.y += cvs->rect.yStart;
	
	Out_Draw_Line(Start.x, Start.y , End.x, End.y, color);
}

void Canvas_Draw_Lines(Canvas cvs, PointType points[], u16 n, u16 color)
{
	u16 index;
	
	for(index = 0; index < n - 1; index ++)
		Canvas_Draw_Line(cvs,  points + index, points + index + 1, color);
}


void Canvas_Draw_DirLine(Canvas cvs, float angle, PointType center, s32 length, u16 color)
{
	assert_param(cvs);
	
	center.x += cvs->rect.xStart;
	center.y += cvs->rect.yStart;
	
	Out_Draw_DirLine(-angle, center, length, color);
}

void Canvas_Draw_Circle(Canvas cvs, PointType center, s32 radius, u16 color)
{
	center.x += cvs->rect.xStart;
	center.y += cvs->rect.yStart;
	
	Out_Draw_Oval(radius, radius, center.x, center.y, color);
}

void Canvas_Draw(Canvas cvs)
{
	Out_Draw_FRectangle(&cvs->rect, cvs->backColor);
}



void Canvas_EnterHandler(Canvas canvas, PointType *point)
{
	static PointType Points[2], prePoint;
	Points[0].x = prePoint.x;
	Points[1].x = prePoint.x;
	Points[0].y = 0;
	Points[1].y = canvas->rect.Height;
	
	Canvas_Draw_Line(canvas, Points, Points + 1, canvas->backColor);
	
	Points[0].x = point->x;
	Points[1].x = point->x;
	Points[0].y = 0;
	Points[1].y = canvas->rect.Height;
	
	Canvas_Draw_Line(canvas, Points, Points + 1, White);
	
	prePoint = *point;
	
	//Out_printf(0, 20, "%s", "Hello");
}

void Canvas_LeaveHandler(Canvas canvas, PointType *point)
{
	static PointType Points[2];
	
	Points[0].x = point->x;
	Points[1].x = point->x;
	Points[0].y = 0;
	Points[1].y = canvas->rect.Height;
	
	Canvas_Draw_Line(canvas, Points, Points + 1, White);
}

void Canvas_Debug()
{
	RectType rect = {10, 20, 250, 200};
	PointType p1 = {75, 32};
	PointType p2 = {35, 20};
	CanvasDefType canvasDefStructure;
	Canvas cvs;
	canvasDefStructure.EnterHandler = Canvas_EnterHandler;
	canvasDefStructure.LeaveHandler = Canvas_LeaveHandler;
	canvasDefStructure.rect = rect;
	canvasDefStructure.backColor = Red;
	
	cvs = Canvas_Create(&canvasDefStructure);
	
	Canvas_Draw(cvs);
	Canvas_Register(cvs);	
	
	Canvas_Draw_Line(cvs, &p2, &p1, White);
}
