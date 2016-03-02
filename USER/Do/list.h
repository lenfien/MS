#ifndef __LIST
#define __LIST

#include "stm32f10x.h"


typedef struct ListNodeTag
{
	void *value;
	struct ListNodeTag* prev;
	struct ListNodeTag* next;
}ListNodeType;

typedef ListNodeType* ListNode;

typedef struct
{
	u32  			amount;
	ListNode 	header;
	ListNode 	end;
}ListType;

typedef ListType* List;

List List_Create(void);
u32	 List_Delete(List list, void *value);
u32	 List_Append(List list, void *value);
void List_printf(List list);

#endif
