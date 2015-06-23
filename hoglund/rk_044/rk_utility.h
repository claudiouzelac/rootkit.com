#ifndef __RK_UTILITY_H__
#define __RK_UTILITY_H__

typedef void (*WORK_FUNCTION)(PVOID);

typedef struct _PROCESS_CONTEXT_WORK_ITEM{
		WORK_FUNCTION	function;
		PVOID			WorkInfo;
		KEVENT			evtWorkDone;
		LIST_ENTRY		ListEntry;
}PROCESS_CONTEXT_WORK_ITEM,*PPROCESS_CONTEXT_WORK_ITEM;



NTSTATUS	ReadRegistry(IN  PUNICODE_STRING theBindingName );

PPROCESS_CONTEXT_WORK_ITEM	CreateRunInProcessContextWorkItem(WORK_FUNCTION,PVOID);
NTSTATUS	QueueWorkToRunInProcessContext(PPROCESS_CONTEXT_WORK_ITEM pWorkItem);
NTSTATUS	DequeuAndRun_RunInProcessContext_WorkItem();
NTSTATUS	WaitForWorkItem(PPROCESS_CONTEXT_WORK_ITEM pWorkItem);

extern LIST_ENTRY		ProcessContextWorkQueueHead;
extern ULONG			SystemProcessId;

#endif