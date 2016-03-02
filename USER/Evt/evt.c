#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "list.h"
#include "gui.h"
#include "com.h"
#include "evt.h"

List EventSourceList = 0;

void Evt_Listen(EventSource es)
{
	if(EventSourceList == 0)
	{
		EventSourceList = List_Create();
	}
	
	es->privateMember = (void*)FALSE;
	List_Append(EventSourceList, es);
}


void Evt_Abandon(EventSource es)
{
	List_Delete(EventSourceList, es);
}


void Evt_Scan(void *param)
{
	ListNode node;
	
	if(EventSourceList == 0)
	{
		EventSourceList = List_Create();
	}
	
	assert_param(EventSourList);
	
	node = EventSourceList->header;
	
	while(node)
	{
		EventSource es = (EventSource)node->value;
		node = node->next;
		
		assert_param(es->IsMeetCondition);
		
		if(es->IsMeetCondition(es, param))
		{
			es->privateMember = (void*)1;
			if(es->ActiveHandler)
				es->ActiveHandler(es, param);
		}
		else if(es->privateMember == (void*)1 && es->DeactiveHandler)
		{
			es->DeactiveHandler(es, param);
			es->privateMember = (void*)0;
		}
	}
}






