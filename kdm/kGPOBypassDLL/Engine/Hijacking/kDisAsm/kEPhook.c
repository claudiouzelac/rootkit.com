/******************************************************************************
  kEPhook.c		: Entry point hooking engine
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
#include "kEPhook.h"
#include "ZDisasm.h"
#include <tlhelp32.h>				// Tool help 32 functions


FARPROC fCreateToolhelp32Snapshot;
FARPROC nti_fModule32First;
FARPROC nti_fModule32Next;
FARPROC nti_OpenThread;
FARPROC nti_Thread32First;
FARPROC nti_Thread32Next;


int WakeUpProcess(DWORD pid)
{
    HANDLE hSnap, hThread;
	DWORD dPID=pid;
	THREADENTRY32 ThEnt = {0};
	HINSTANCE hInstLib=0;

	OutputString("Waking up current process...\n");

	hInstLib = LoadLibrary("kernel32.dll");

	nti_OpenThread = (FARPROC) GetProcAddress(hInstLib,"OpenThread");

	if(!nti_OpenThread)
	{
		OutputDebugString("!OpenThread\n");
		return 0;
	}


	fCreateToolhelp32Snapshot   = (FARPROC) GetProcAddress(hInstLib,"CreateToolhelp32Snapshot");
	nti_Thread32First			= (FARPROC) GetProcAddress(hInstLib, "Thread32First");
	nti_Thread32Next			= (FARPROC) GetProcAddress(hInstLib, "Thread32Next");
	
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
					OutputString("ResumeThread(%d)\n",ThEnt.th32ThreadID);
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

//		OutputString : Send a formatted string to debugger output
//		todo : handle compatibility with Win 9x version
void OutputString(char* frmstr,...) {
 char buf[1024];
 va_list vargs;
 va_start(vargs, frmstr);
 wvsprintfA(buf, frmstr, vargs); 
 va_end(vargs);
 OutputDebugString((char*)buf);
 return;
} 


int WINAPI HookApi(char* DllName, char* FuncName, DWORD ReplacementFunc, FARPROC* pFunc)
{
  BYTE* CG_Func=NULL;
  FARPROC funcGetProcAddress=NULL;
  int ret=0;


  (*pFunc) = GetProcAddress(GetModuleHandle(DllName), FuncName);
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

void InsertByte(DWORD Addr, unsigned char Byte)
{
	// Check if the calling process owns write access 
	// to this range of memory
	if(!IsBadReadPtr((void*)Addr, (UINT) sizeof(byte)))
		*((byte*) ((DWORD*)Addr)) = Byte;
}
void InsertDword(DWORD Addr, DWORD dWord)
{
	// Check if the calling process owns write access 
	// to this range of memory
	if(!IsBadReadPtr((void*)Addr, (UINT) sizeof(DWORD)))
		*((DWORD*)Addr) = dWord;
}


// Generates a jump to address "To", from address "From"
// From is considered as the memory address of the byte just before jump start
// in memory. In fact, the operand of the jump for a relative jump is To - From - 5
// Sample :
//       |.|jmp|@MyFunc|.|
// size:  1   1   3     1
//        ^ 
//        |
//        +-- From is this address 
void GenJmp(DWORD To, DWORD From)
{
	InsertByte (From+0, 0xe9);			// jmp	...
	InsertDword(From+1, To - From - 5); //		destination - src - 5
}

int ForgeHook(DWORD pAddr, DWORD pAddrToJump, byte **Buffer)
{
  DWORD dSize=0, i=0, CollectedSpace=0, OldProtect=0;
  BYTE* pInstruction;
  DWORD CallGateSize=0;

  // Check parameters
  if(!pAddr || !pAddrToJump || !Buffer)
	return 0;

  // Start disassembling...
  pInstruction = (BYTE*)pAddr;
  // Loop until we get enough place to set a jump (5 bytes)
  while(CollectedSpace < SIZEOFJUMP)
  {
	// Get instruction lenght
	GetInstLenght((DWORD*)pInstruction, &dSize);

	// Jump to next instruction
	pInstruction += dSize;
	
	// Update collected space size
	CollectedSpace += dSize;
  }
  
  // Forge call gate :
  // allocate memory for call gate : stores saved bytes + the jump after hijacking zone
  CallGateSize = (CollectedSpace+SIZEOFJUMP) * sizeof(byte);
  (*Buffer) = (byte*) malloc(CallGateSize * sizeof(byte));
  
  if((*Buffer)==NULL)
	  return 0;	// allocation failed

  // Enforce execute mode for call gate
  VirtualProtect((*Buffer), CallGateSize, PAGE_EXECUTE_READWRITE, &OldProtect);
  FillMemory((*Buffer), CallGateSize, ASMNOP);			// clear call gate with NOPs
  CopyMemory((*Buffer), (void*)pAddr, CollectedSpace);	// copy instructions

  // generate jump to original function + SIZEOFJUMP (strides over jump hook)
  GenJmp( (DWORD)((void*)pAddr) + (DWORD) SIZEOFJUMP,
          (DWORD)     (*Buffer) + (DWORD) CollectedSpace);



  // Forge hook
  // give read write execute read and write rights to memory zone
  VirtualProtect((void*)pAddr, CollectedSpace+SIZEOFJUMP, PAGE_EXECUTE_READWRITE, &OldProtect);
  // clear instructions
  FillMemory((void*)pAddr, CollectedSpace, ASMNOP);
  // generate jump
  GenJmp(pAddrToJump, pAddr);
  // restore previous memory protection
  VirtualProtect((void*)pAddr, CollectedSpace+SIZEOFJUMP, OldProtect, &OldProtect);

 return 1;
}