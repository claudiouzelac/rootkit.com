// standard includes 
#include <winsock2.h>
#include <windows.h>
#include <time.h>

// own includes
#include "ntdll.h"
#include "engine.h"
#include "misc.h"
#pragma comment(lib,"ntdll")

#include "safe.h"

// jmpcode cannot be longer then MAX_FUNC_LEN and may not be longer then the function code (doh)

// jmp table which holds all the possible code to jmp
JMP_TABLE jtJmpTable[]=
{
	{LONG_JMP,sizeof(LONG_JMP)-1,LONG_JMP_OFFSET,engine_LONG_JMP_FUNC_JMP,engine_LONG_JMP_STUB_JMP},
	// this gives a stack overflow on some processes :/
	//{MOVEAX_JMP,sizeof(MOVEAX_JMP)-1,MOVEAX_JMP_OFFSET,engine_MOVEAX_JMP_FUNC_JMP,engine_MOVEAX_JMP_STUB_JMP},
}
,jtJmpTableEnd;

DWORD JMP_TABLE_SIZE=(sizeof(jtJmpTable)/sizeof(jtJmpTableEnd))-1;

// function to calculate jmp address
DWORD engine_LONG_JMP_FUNC_JMP(DWORD dwOne,DWORD dwTwo,DWORD dwThree)
{
	return dwOne-dwTwo-dwThree;
}

// function to calculate jmp address
DWORD engine_LONG_JMP_STUB_JMP(DWORD dwOne,DWORD dwTwo,DWORD dwThree)
{
	return dwOne-(dwTwo+dwThree);
}

// function to calculate jmp address
DWORD engine_MOVEAX_JMP_FUNC_JMP(DWORD dwOne,DWORD dwTwo,DWORD dwThree)
{
	return dwOne;
}

// function to calculate jmp address
DWORD engine_MOVEAX_JMP_STUB_JMP(DWORD dwOne,DWORD dwTwo,DWORD dwThree)
{
	return dwOne;
}

// my getmodulehandlea, uses only ntdll functions 
LPVOID engine_NtGetModuleHandleA(LPSTR lpModuleName)
{
	STRING asModuleName;
	UNICODE_STRING usModuleName;
	HANDLE hModule;

	asModuleName.Buffer=(PCHAR)lpModuleName;
	asModuleName.Length=strlen(lpModuleName);
	asModuleName.MaximumLength=asModuleName.Length;

	if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&usModuleName,&asModuleName,TRUE)))
		return NULL;

	if (!NT_SUCCESS(LdrGetDllHandle(NULL,NULL,&usModuleName,&hModule)))
	{
		RtlFreeUnicodeString(&usModuleName);
		return NULL;
	}

	RtlFreeUnicodeString(&usModuleName);

	return hModule;
}

// my loadlibrary, uses only ntdll functions 
LPVOID engine_NtLoadLibraryA(LPSTR lpModuleName)
{
	STRING asModuleName;
	UNICODE_STRING usModuleName;
	HANDLE hModule;

	asModuleName.Buffer=(PCHAR)lpModuleName;
	asModuleName.Length=strlen(lpModuleName);
	asModuleName.MaximumLength=asModuleName.Length;

	if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&usModuleName,&asModuleName,TRUE)))
		return NULL;

	if (!NT_SUCCESS(LdrLoadDll(NULL,0,&usModuleName,&hModule)))
	{
		RtlFreeUnicodeString(&usModuleName);
		return NULL;
	}

	RtlFreeUnicodeString(&usModuleName);

	return hModule;
}

// my getprocaddress, uses only ntdll functions 
LPVOID engine_NtGetProcAddress(HMODULE hModule,LPSTR lpFunctionName)
{
	STRING asFunctionName;
	LPVOID lpAddress;
	
	asFunctionName.Buffer=(PCHAR)lpFunctionName;
	asFunctionName.Length=strlen(lpFunctionName);
	asFunctionName.MaximumLength=asFunctionName.Length;

	if (!NT_SUCCESS(LdrGetProcedureAddress(hModule,&asFunctionName,0,&lpAddress)))
		return NULL;
	
	return lpAddress;
}

// get base address from the specified module in the process
HMODULE engine_GetRemoteModuleHandle(HANDLE hProcess,LPSTR lpModule)
{
	PROCESS_BASIC_INFORMATION pbi;
	PEB_LDR_DATA ldrData;
	LDR_MODULE ldrModule;
	UNICODE_STRING usModuleName;
	ANSI_STRING asModuleName;
	PEB Peb;
	LPVOID lpBase;
	LPVOID lpFirst;
	WCHAR* wcModuleName;

	if (!NT_SUCCESS(NtQueryInformationProcess(hProcess,ProcessBasicInformation,&pbi,sizeof(pbi),NULL)))
		return NULL;

	if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,pbi.PebBaseAddress,&Peb,sizeof(Peb),NULL)))
		return NULL;

	if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,Peb.Ldr,&ldrData,sizeof(ldrData),NULL)))
	{	
		// temp workaround ?
		// this shit happens when we call this function in our NtResumethread hook
		// Ldr is NULL then... :/
		// Vista's dllbaseaddress randomization changes per reboot anyway :-)
	
		return engine_NtGetModuleHandleA(lpModule);
	}

	lpFirst=lpBase=ldrData.InLoadOrderModuleList.Flink;

	while (1)
	{
		if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,lpBase,&ldrModule,sizeof(LDR_MODULE),NULL))) 
			break;
		
		if ((DWORD)ldrModule.InLoadOrderModuleList.Flink==(DWORD)lpFirst) 
			break;

		if (!(wcModuleName=(WCHAR*)misc_AllocBuffer(ldrModule.BaseDllName.Length+2))) 
			break;

		RtlZeroMemory(wcModuleName,ldrModule.BaseDllName.Length+2);

		if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,ldrModule.BaseDllName.Buffer,wcModuleName,ldrModule.BaseDllName.Length,NULL))) 
		{
			misc_FreeBuffer(&wcModuleName);
			break;
		}

		usModuleName.Buffer=wcModuleName;
		usModuleName.Length=ldrModule.BaseDllName.Length;
		usModuleName.MaximumLength=ldrModule.BaseDllName.MaximumLength;

		if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&asModuleName,&usModuleName,TRUE)))
		{
			if (strnicmp(asModuleName.Buffer,lpModule,strlen(lpModule))==0)
			{
				RtlFreeAnsiString(&asModuleName);
				misc_FreeBuffer(&wcModuleName);
				return ldrModule.BaseAddress;
			}
		}
				
		RtlFreeAnsiString(&asModuleName);
		misc_FreeBuffer(&wcModuleName);

		lpBase=ldrModule.InLoadOrderModuleList.Flink;
	}

	return NULL;
}

// get address from given procedure in specified module
LPVOID engine_GetRemoteProcAddress(HANDLE hProcess,HMODULE hModule,LPSTR lpFunction)
{
	IMAGE_DOS_HEADER         DosHdr;
    IMAGE_NT_HEADERS         NtHdr;
    IMAGE_EXPORT_DIRECTORY   ExtDir;
	UINT                     uj;
	CHAR*					 dwReadAddr;
	CHAR*					 pcExportAddr;
	CHAR*					 pcFuncName;
	CHAR*					 pcFuncPtr;
	DWORD					 dwFuncAddr;
	CHAR*					 pcBuffer;
	WORD					 wOrd;

	if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,hModule,&DosHdr,sizeof(IMAGE_DOS_HEADER),NULL))) 
		return NULL;

	if (IMAGE_DOS_SIGNATURE==DosHdr.e_magic)
    {
		dwReadAddr=(char*)hModule+DosHdr.e_lfanew;

		if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,dwReadAddr,&NtHdr,sizeof(IMAGE_NT_HEADERS),NULL))) 
			return NULL;

		if(IMAGE_NT_SIGNATURE==NtHdr.Signature||IMAGE_NT_SIGNATURE1==NtHdr.Signature)
        {
			pcExportAddr=(char*)((DWORD)hModule+(DWORD)NtHdr.OptionalHeader.DataDirectory[0].VirtualAddress);

			if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,pcExportAddr,&ExtDir,sizeof(IMAGE_EXPORT_DIRECTORY),NULL)))
				return NULL;

			pcExportAddr=(char*)((DWORD)hModule+(DWORD)ExtDir.AddressOfNames);

			for (uj=0;uj<ExtDir.NumberOfFunctions;uj++)
			{
				if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,pcExportAddr,&pcFuncName,sizeof(char*),NULL)))
					return NULL;

				pcBuffer=(CHAR*)misc_AllocBuffer(255);
				if (!pcBuffer)
					return NULL;

				RtlZeroMemory(pcBuffer,255);
				if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,(char*)((DWORD)hModule+(DWORD)pcFuncName),pcBuffer,254,NULL)))
				{
					misc_FreeBuffer(&pcBuffer);
					return NULL;
				}

				if (stricmp(pcBuffer,lpFunction)==0)
				{
						pcFuncPtr=(char*)((DWORD)hModule+(DWORD)ExtDir.AddressOfNameOrdinals+(uj*sizeof(WORD)));

						if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,pcFuncPtr,&wOrd,sizeof(WORD),NULL)))
						{	
							misc_FreeBuffer(&pcBuffer);
							return NULL;
						}

						pcFuncPtr=(char*)((DWORD)hModule+(DWORD)ExtDir.AddressOfFunctions+(wOrd*sizeof(DWORD)));

						if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,pcFuncPtr,&dwFuncAddr,sizeof(DWORD),NULL)))
						{	
							misc_FreeBuffer(&pcBuffer);
							return NULL;
						}

						misc_FreeBuffer(&pcBuffer);

						return (char*)hModule+dwFuncAddr;
				}

				misc_FreeBuffer(&pcBuffer);
				pcExportAddr+=sizeof(char*);
			}
		}
	}	

	return NULL;
}

// Get size of PE Image in memory
DWORD engine_GetPEImageSize(LPVOID lpPEImage)
{
	PIMAGE_OPTIONAL_HEADER pIo;
	PIMAGE_DOS_HEADER pId;
	DWORD dwOffset;

	pId=(PIMAGE_DOS_HEADER)lpPEImage;
	dwOffset=(DWORD)lpPEImage+(DWORD)pId->e_lfanew+sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER);
	pIo=(PIMAGE_OPTIONAL_HEADER)dwOffset;
	
	return pIo->SizeOfImage;
}

// copy pe image on lpbase from current process to dwpid on the same address
BOOL engine_CopyImageToProcess(HANDLE hProcess,LPVOID lpBase)
{
	DWORD dwSize,dwFree=0;
	LPVOID lpNew=lpBase;
	
	// get PE size
	dwSize=engine_GetPEImageSize(lpBase);

	if (!NT_SUCCESS(NtAllocateVirtualMemory(hProcess,&lpNew,0,&dwSize,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE)))
		return FALSE;

	if (!NT_SUCCESS(NtWriteVirtualMemory(hProcess,lpNew,lpBase,dwSize,NULL)))
	{
		NtFreeVirtualMemory(hProcess,&lpNew,&dwFree,MEM_RELEASE);
		return FALSE;
	}

	return TRUE;
}

BOOL engine_SaveRemoteVar(HANDLE hProcess,LPVOID lpDest,LPVOID lpSrc)
{
	if (NT_SUCCESS(NtWriteVirtualMemory(hProcess,lpDest,lpSrc,sizeof(LPVOID),NULL)))
		return TRUE;

	return FALSE;
}

BOOL engine_LoadRemoteVar(HANDLE hProcess,LPVOID lpSrc,LPVOID lpDest)
{
	if (NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,lpSrc,lpDest,sizeof(LPVOID),NULL)))
		return TRUE;

	return FALSE;
}

// Functions that fills a allocated buffer with
// a "jmp address".
void engine_BuildJMPBuffer(CHAR* pcJmpBuf,DWORD dwJmpAddr,INT iRndJmp)
{
	memcpy(pcJmpBuf,jtJmpTable[iRndJmp].pcJmpCode,jtJmpTable[iRndJmp].iCodeSize);
	memcpy(pcJmpBuf+jtJmpTable[iRndJmp].iOffset,&dwJmpAddr,sizeof(DWORD));
}
// Write a jump to the address of the given function and returns
// the allocated address for the original code.

// a thought: because we use this not only to do a remote hook but also local hooks maybe
// we should supply an module and function address ? or write a new function for local hooks ?

LPVOID engine_HookFunctionInProcess(HANDLE hProcess,LPSTR lpModuleName,LPSTR lpFunctionName,LPVOID lpHookFunctionAddress,PDWORD pdwHookFunctionSize,LPVOID* lpFunctionAddress,INT iRndJmp)
{
	LPVOID lpModule=NULL;
	LPVOID lpFunction=NULL;
	MEMORY_BASIC_INFORMATION mbi;
	CHAR  lpTmpFunction[MAX_FUNC_LEN*2];
	CHAR  lpLocalStub[MAX_FUNC_LEN*3];
	CHAR  lpLocalFunc[MAX_FUNC_LEN*3];
	DWORD dwBytesRead;
	DWORD dwReadLen=0;
	DWORD dwExistingJMP=0;
	DWORD dwStubSize;
	DWORD dwFree=0;
	DWORD dwBytesWritten;
	DWORD dwOldProtect;

	LPVOID lpRemoteStub=NULL;
	INT   iFuncLen;
	PBYTE pReadAddress;
	NTSTATUS ntStatus;

	// Get module address
	lpModule=(LPVOID)engine_GetRemoteModuleHandle(hProcess,lpModuleName);
	if (!lpModule)
		return NULL;
		
	// Get function address
	lpFunction=engine_GetRemoteProcAddress(hProcess,lpModule,lpFunctionName);
	if (!lpFunction)
		return NULL;

	// Get info about the function address
	if (!NT_SUCCESS(SafeNtQueryVirtualMemory(hProcess,lpFunction,MemoryBasicInformation,&mbi,sizeof(mbi),NULL)))
		return NULL;

	// Flush instruction cache
	NtFlushInstructionCache(hProcess,mbi.BaseAddress,mbi.RegionSize);

	// Change the protection for the region
	if (!NT_SUCCESS(NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,PAGE_EXECUTE_READWRITE,&mbi.Protect)))
		return NULL;

	// Fill stub buffer with nops
	RtlFillMemory(lpLocalStub,MAX_FUNC_LEN*3,NOP);

	// Read MAX_FUNC_LEN instruction(s) from the function into our function buffer
	if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,lpFunction,lpTmpFunction,MAX_FUNC_LEN*2,&dwBytesRead)))
	{
		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		return NULL;
	}

	pReadAddress=(PBYTE)lpTmpFunction;

	// check if first opcode in the function is a another jump
	if (*pReadAddress==LONG_JMP_OPCODE)
	{
		// get relative address
		memcpy(&dwExistingJMP,pReadAddress+1,4);
		// get absolute address
		dwExistingJMP=(DWORD)lpFunction+dwExistingJMP;
		// readlen
		dwReadLen=jtJmpTable[RELATIVE_JMP].iCodeSize

		engine_BuildJMPBuffer((CHAR*)lpLocalStub,((DWORD)lpRemoteStub+dwReadLen)-dwExistingJMP,RELATIVE_JMP);

		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		return NULL;
	}

	// Get the length of the first instruction(s)
	// This part is done by Z0MBiE's LDE32 v1.05
	iFuncLen=disasm_main(pReadAddress); // get first instruction length
	while (iFuncLen!=-1 && dwReadLen<(DWORD)jtJmpTable[iRndJmp].iCodeSize)
	{
		dwReadLen+=iFuncLen;
		pReadAddress+=iFuncLen;
		iFuncLen=disasm_main(pReadAddress); // next instruction length
	}

	// API code is too short or too long too hook this way (for now ;))
	if (dwReadLen<(DWORD)jtJmpTable[iRndJmp].iCodeSize||dwReadLen>MAX_FUNC_LEN*2)
	{
		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		return NULL;
	}

	// Read the first instruction(s) from the function into our stub buffer
	if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,lpFunction,lpLocalStub,dwReadLen,&dwBytesRead)))
	{	
		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		return NULL;
	}

	// Allocate space with read/write access for our "stub"
	// note: always use a relative jump for our stub -> RELATIVE_JMP

	dwStubSize=dwReadLen+jtJmpTable[RELATIVE_JMP].iCodeSize;
	if (!NT_SUCCESS(NtAllocateVirtualMemory(hProcess,&lpRemoteStub,0,&dwStubSize,MEM_COMMIT|MEM_TOP_DOWN,PAGE_READWRITE)))
	{	
		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		return NULL;
	}

	// Check
	if (dwStubSize<dwReadLen+jtJmpTable[RELATIVE_JMP].iCodeSize)
	{
		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		
		// Free allocated buffer
		NtFreeVirtualMemory(hProcess,&lpRemoteStub,&dwFree,MEM_RELEASE);
		return NULL;
	}

	engine_BuildJMPBuffer((CHAR*)lpLocalStub+dwReadLen,jtJmpTable[RELATIVE_JMP].jcStub((DWORD)lpFunction+dwReadLen,(DWORD)lpRemoteStub,dwReadLen+jtJmpTable[RELATIVE_JMP].iCodeSize),RELATIVE_JMP);

	// Copy the "stub" buffer to process memory
	if (!NT_SUCCESS(NtWriteVirtualMemory(hProcess,lpRemoteStub,lpLocalStub,dwReadLen+jtJmpTable[RELATIVE_JMP].iCodeSize,&dwBytesWritten)))
	{
		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		
		// Free allocated buffer
		NtFreeVirtualMemory(hProcess,&lpRemoteStub,&dwFree,MEM_RELEASE);
		return NULL;
	}

	// Check
	if (dwBytesWritten<dwReadLen+jtJmpTable[RELATIVE_JMP].iCodeSize)
	{
		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		
		// Free allocated buffer
		NtFreeVirtualMemory(hProcess,&lpRemoteStub,&dwFree,MEM_RELEASE);
		return NULL;
	}
	
	// change access
	if (!NT_SUCCESS(NtProtectVirtualMemory(hProcess,&lpRemoteStub,&dwStubSize,PAGE_EXECUTE_READ,&dwOldProtect)))
	{
		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		
		// Free allocated buffer
		NtFreeVirtualMemory(hProcess,&lpRemoteStub,&dwFree,MEM_RELEASE);
		return NULL;
	}
		
	// Fill it with NOP
	RtlFillMemory(lpLocalFunc,MAX_FUNC_LEN*3,NOP);

	// Prepare jmpcode
	engine_BuildJMPBuffer((CHAR*)lpLocalFunc,jtJmpTable[iRndJmp].jcFunc((DWORD)lpHookFunctionAddress,(DWORD)lpFunction,(DWORD)jtJmpTable[iRndJmp].iCodeSize),iRndJmp);
	
	ntStatus=NtWriteVirtualMemory(hProcess,lpFunction,lpLocalFunc,dwReadLen,&dwBytesWritten);
		
	// Check that we really wrote our jmpcode completely
	if (!NT_SUCCESS(ntStatus) || dwBytesWritten!=dwReadLen)
	{
		// Try to fix stuff
		if (dwBytesWritten)
			NtWriteVirtualMemory(hProcess,lpFunction,lpRemoteStub,dwBytesWritten,&dwBytesWritten);

		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		
		// Free allocated buffer
		NtFreeVirtualMemory(hProcess,&lpRemoteStub,&dwFree,MEM_RELEASE);
		return NULL;
	}
	
	// Restore protection
	NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);

	// Save size of read function length
	if (pdwHookFunctionSize) *pdwHookFunctionSize=dwReadLen;
	// Save address of function
	if (lpFunctionAddress) *lpFunctionAddress=lpFunction;

	return lpRemoteStub;
}

BOOL engine_UnHookFunctionInProcess(HANDLE hProcess,LPSTR lpModuleName,LPSTR lpFunctionName,LPVOID lpOldFunctionAddress,DWORD dwFunctionSize)
{
	LPVOID lpModule=NULL;
	LPVOID lpFunction=NULL;
	MEMORY_BASIC_INFORMATION mbi;
	CHAR   lpLocalStub[MAX_FUNC_LEN*2];
	DWORD  dwFree=0;
	DWORD  dwBytesWritten;
	
	// Get module address
	lpModule=(LPVOID)engine_GetRemoteModuleHandle(hProcess,lpModuleName);
	if (!lpModule)
		return FALSE;
		
	// Get function address
	lpFunction=engine_GetRemoteProcAddress(hProcess,lpModule,lpFunctionName);
	if (!lpFunction)
		return FALSE;

	// Get info about the function address
	if (!NT_SUCCESS(SafeNtQueryVirtualMemory(hProcess,lpFunction,MemoryBasicInformation,&mbi,sizeof(mbi),NULL)))
		return FALSE;

	// Flush instruction cache
	NtFlushInstructionCache(hProcess,mbi.BaseAddress,mbi.RegionSize);

	// Change the protection for the region
	if (!NT_SUCCESS(NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,PAGE_EXECUTE_READWRITE,&mbi.Protect)))
		return FALSE;

	// Read old functions instructions
	if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,lpOldFunctionAddress,lpLocalStub,dwFunctionSize,NULL)))
	{
		// restore protection
		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		return FALSE;
	}

	// Restore original function
	if (!NT_SUCCESS(NtWriteVirtualMemory(hProcess,lpFunction,lpLocalStub,dwFunctionSize,&dwBytesWritten)))
	{
		// restore protection
		NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);
		return FALSE;
	}

	// Free stub memory
	NtFreeVirtualMemory(hProcess,&lpOldFunctionAddress,&dwFree,MEM_RELEASE);

	// Restore protection
	NtProtectVirtualMemory(hProcess,&mbi.BaseAddress,&mbi.RegionSize,mbi.Protect,NULL);

	return TRUE;
}