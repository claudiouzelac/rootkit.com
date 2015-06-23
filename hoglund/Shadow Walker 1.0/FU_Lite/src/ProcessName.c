///////////////////////////////////////////////////////////////////////////////////////
// Filename ProcessName.c
// 
// Author: fuzen_op
// Email:  fuzen_op@yahoo.com or fuzen_op@rootkit.com
//
// Description: Finds the offset of the process name within an EPROCESS block.
//
// Date:    5/27/2003
// Version: 1.0


#include "ntddk.h"
#include "ProcessName.h"

#pragma data_seg(".text")
///////////////////////////////////////////////////////////////////
// ULONG GetLocationOfProcessName
// Parameters:
//       IN PEPROCESS    pointer to the kernel process block of 
//						 the current process
//       IN CHAR *
// Returns:
//		 OUT int		 offset of process name in EPROCESS structure
//     
// Description: Gets the location if the name of the process in the 
//				kernel process block. This is done because EPROCESS
//				changes between versions of NT/2000/XP. This technique
//				was first done by Sysinternals. They rock! 
//
// Note:        The reason this works is because it walks the list of
//				processes looking in the EPROCESS block for the string
//				"System".

int GetLocationOfProcessName(PEPROCESS CurrentProc, char *sys_name)
{
    int i_offset;
	PLIST_ENTRY plist_active_procs;

	for(i_offset = 0; i_offset < PAGE_SIZE; i_offset++) // This will fail if EPROCESS
												           // grows bigger than PAGE_SIZE
	{
		if((RtlCompareMemory(sys_name, (PVOID)((ULONG)CurrentProc + i_offset), 6)) == 6)
		{
			return i_offset;
		}
	}

	return 0;
}

