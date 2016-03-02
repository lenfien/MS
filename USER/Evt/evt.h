
#ifndef __EVENT
#define __EVENT

#include "com.h"

typedef struct EventSourceTag
{
	bool (*IsMeetCondition)(struct EventSourceTag* self, void*);
	void (*ActiveHandler)(struct EventSourceTag* self, void*);
	void (*DeactiveHandler)(struct EventSourceTag* self, void*);
	
	void* privateMember;
}EventSourceType;
typedef EventSourceType* EventSource;
typedef void (*EventHandler)(EventSourceType* self, void*);



void Evt_Init(void);

void Evt_Listen(EventSource es);
void Evt_Abandon(EventSource es);
void Evt_Scan(void *param);

#endif
