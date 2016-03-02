#include <stdlib.h>
#include <stdio.h>

#include "list.h"




List List_Create()
{
	List list = (List)malloc(sizeof(ListType));
	list->amount = 0;
	list->header = 0;
	list->end = 0;
	return list;
}

u32 List_Delete(List list, void *value)
{
	
	u32 count = 0;
	ListNode node;
	
	assert_param(list != 0);
	
	node = list->header;
	
	while(node != 0)
	{
		ListNode nodeBackup = node;
		ListNode next = node->next;
		ListNode prev = node->prev;
		
		if(nodeBackup->value == value)
		{
			if(nodeBackup == list->header)
			{
				list->header = next;
			}
			
			if(nodeBackup == list->end)
			{
				list->end = prev;
			}
			
			if(prev != 0)
				prev->next = next;
			
			if(next != 0)
				next->prev = prev;
			
			free(nodeBackup);
			
			list->amount --;
			count ++;
		}
		
		node = next;
	}
	
	return count;
}


u32 List_Append(List list, void *value)
{
	ListNode node = (ListNode)malloc(sizeof(ListNodeType));
	
	assert_param(node != 0);
	
	node->value = value;
	node->prev = node->next = 0;
	
	if(list->amount == 0)
	{
		list->header = list->end = node;
	}
	else
	{
		node->prev = list->end;
		list->end->next = node;
		list->end = node;
	}
	
	list->amount ++;
	
	return list->amount;
}

void List_printf(List list)
{
	ListNode header;
	
	assert_param(list != 0);
	
	header = list->header;
	
	//printf("%d:", list->amount);
	
	while(header)
	{
		//printf("%d->", (int)(header->value));
		header = header->next;
	}
	//printf("0\n\r");
}
