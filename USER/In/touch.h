#ifndef __TOUCH
#define __TOUCH

#include "com.h"
#include "gui.h"

typedef struct 
{
	bool 			isFree;											//��Ļ��ʱ���޴���
	bool			isTouch;									//�����Ƿ���ʱ����
	bool			isHold;     							//��ָһֱ��������Ļ�ϵĻ��ἤ�����״̬
	PointType touchPoint;								//��ǰ�������ĵ�
}TouchStateType;


extern const TouchStateType* Touch_GetState(void);
extern 	void Touch_Init(void);
extern 	s8 Touch_Update(void);

#endif
