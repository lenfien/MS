#include <stdio.h>
#include <stdlib.h>

#include "gui.h"
#include "btn.h"
#include "evt.h"
#include "com.h"
#include "touch.h"

static ButtonDefType ButtonDefTypeTemplate = 
{{0, 0, 0, 0}, 0, 0, 0, 0, White, Red, Black, {0, 0, 50, 20}, "BtnTmp", FALSE, FALSE};

ButtonDefType Button_Get_ButtonDefTypeTemplate()
{
	ButtonDefType t = ButtonDefTypeTemplate;
	return t;
}

static bool Button_IsMeetCondition(EventSource evt, void* param)
{
	Button btn = (Button)evt;
	PointType* p = (PointType*)param;
	
	assert_param(btn && p);
	
	if(Touch_GetState()->isTouch == FALSE)
		return FALSE;
	
	return (bool)((p->x > btn->rect.xStart && (p->x < (btn->rect.xStart + btn->rect.Width))) && 
		((p->y > btn->rect.yStart) && (p->y < (btn->rect.yStart + btn->rect.Height))));
}


static void Button_ActiveHandler(EventSource evt, void* param)
{
	Button btn = (Button)evt;
	PointType* p = (PointType*)param;
	
	if(btn->isDown)
	{
		if(btn->holdHandler)
			btn->holdHandler(btn, p);
	}
	else
	{
		btn->isDown = TRUE;
		Button_Draw(btn);
		
		if(btn->downHandler)
			btn->downHandler(btn, p);
	}
}

static void Button_DeactiveHandler(EventSource evt, void* param)
{
	Button btn = (Button)evt;
	PointType* p = (PointType*)param;
	
	if(btn->isDown)
	{
		if(btn->releaseHandler)
		{
			btn->releaseHandler(btn, p);
		}
		
		if(btn->clickHandler)
		{
			btn->clickHandler(btn, p);
		}
		
		btn->isDown = FALSE;
		Button_Draw(btn);
	}
}

void Button_Draw(Button btn)
{
	u16 oldBackColor = Out_Config_Font_BackColor(btn->isDown ? btn->downColor : btn->upColor);
	u16 oldForeColor = Out_Config_Font_ForeColor(btn->textColor);
	
	Out_Draw_FRectangle(&btn->rect, btn->isDown ? btn->downColor : btn->upColor);
	
	Out_prints(btn->rect.xStart + (btn->rect.Width - Out_Get_StringWidth(btn->text))/2, 
	btn->rect.yStart + (btn->rect.Height - Out_Get_FontHeight())/2,
	btn->text);
	
	Out_Config_Font_BackColor(oldBackColor);
	Out_Config_Font_ForeColor(oldForeColor);	
}


Button Button_Create(ButtonDefType* binfo)
{
	Button btn = (Button)malloc(sizeof(ButtonDefType));
	
	*btn = *binfo;
	
	btn->isDown = FALSE;
	btn->isActive = FALSE;
	
	btn->rect.Width = MAX(binfo->rect.Width, Out_Get_StringWidth(btn->text));
	btn->rect.Height = MAX(binfo->rect.Height, Out_Get_FontHeight());
	
	//init system part
	btn->events.IsMeetCondition = Button_IsMeetCondition;
	btn->events.ActiveHandler = Button_ActiveHandler;
	btn->events.DeactiveHandler = Button_DeactiveHandler;
	btn->events.privateMember = (void*)0;
	
	return btn;
}

void Button_Delete(Button btn)
{
	assert_param(btn);
	if(btn->isActive)
		Evt_Abandon((EventSource)btn);
	free(btn);
}

void Button_Register(Button btn)
{
	if(btn->isActive == FALSE)
		Evt_Listen((EventSource)btn);
	btn->isActive = TRUE;
}

void Button_Deregister(Button btn)
{
	if(btn->isActive)
		Evt_Abandon((EventSource)btn);
	
	btn->isActive = FALSE;
}

