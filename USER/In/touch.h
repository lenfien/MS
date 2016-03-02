#ifndef __TOUCH
#define __TOUCH

#include "com.h"
#include "gui.h"

typedef struct 
{
	bool 			isFree;											//屏幕此时有无从左
	bool			isTouch;									//触屏是否被暂时按下
	bool			isHold;     							//手指一直保留在屏幕上的话会激活这个状态
	PointType touchPoint;								//当前被触摸的点
}TouchStateType;


extern const TouchStateType* Touch_GetState(void);
extern 	void Touch_Init(void);
extern 	s8 Touch_Update(void);

#endif
