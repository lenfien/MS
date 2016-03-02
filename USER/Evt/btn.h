

#ifndef __BTN
#define __BTN

#include "evt.h"
#include "gui.h"

/**
  *@breif This btn.h provids the Button interface. To use Button, follow the below procedures:
	*       1. Define a ButtonDefType structure, and init it , or use the Button_Get_ButtonDefTypeTemplate then amend it to your porpose
	*				2. invoke Button_Create to create a button defined by ButtonDefType structure
	*				3. invoke Button_Register to register the button to system 
	*				4. if you don't want to use it temporary use the Button_Deregister to disconnect it from system
	*				5. if you don't want to use it permanent use the Button_Delete to distroy it and free the heap
	*@note  button's work needs the providing from Evt_Scan() and 
  */

typedef struct ButtonTag
{
	EventSourceType events;
	
	void (*clickHandler)(struct ButtonTag*, PointType*);
	void (*holdHandler)(struct ButtonTag*, PointType*);
	void (*releaseHandler)(struct ButtonTag*, PointType*);
	void (*downHandler)(struct ButtonTag*, PointType*);
	
	u16						textColor;
	u16 					upColor;
	u16 					downColor;
	RectType 			rect;
	char 					*text;
	bool					isDown;
	bool 					isActive;
}ButtonDefType;

typedef ButtonDefType* Button;
typedef void (*ButtonEventHandler)(struct ButtonTag*, PointType*);

void 		Button_Draw(Button);

ButtonDefType Button_Get_ButtonDefTypeTemplate(void);

Button 	Button_Create(ButtonDefType* binfo);
void 		Button_Delete(Button);

void 		Button_Register(Button btn);
void 		Button_Deregister(Button btn);

#endif
