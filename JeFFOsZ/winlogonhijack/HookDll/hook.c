////////////////////////////////////////////////////////////////////
//
// This file contains the functions used for API (un)hooking and stub 
// building. Written by JeFFOsZ
//
////////////////////////////////////////////////////////////////////

#include "hook.h"

// Functions that fills a allocated buffer with
// a "jmp address".
VOID BuildJMPBuffer(CHAR* pcJmpBuf,DWORD dwJmpAddr)
{
	pcJmpBuf[0]=(BYTE)JMP;
	pcJmpBuf[1]=(BYTE)(dwJmpAddr & 0xFF);
	pcJmpBuf[2]=(BYTE)((dwJmpAddr >> 8) & 0xFF);
	pcJmpBuf[3]=(BYTE)((dwJmpAddr >> 16) & 0xFF);
	pcJmpBuf[4]=(BYTE)((dwJmpAddr >> 24) & 0xFF);
}

// Write a jump to the address of the given function and returns
// the allocated address for the original code.
LPVOID HookFunctionInCurrentProcess(LPCSTR csModuleName,LPCSTR csFunctionName,LPVOID lpJmpAddress)
{
	LPVOID lpModule,lpFunction,lpStub=NULL;
	DWORD  dwOldProtect,dwBytesWritten,dwBytesRead,dwLength=0;
	MEMORY_BASIC_INFORMATION mbi;
	CHAR   cStub[STUB_SIZE];
	CHAR*  cJmpFunction;
	CHAR*  pReadAddress;
	INT	   s;
	HANDLE hProcess=GetCurrentProcess();
	BOOL   bWrite,bRead;
			
	// Get module address
	lpModule=GetModuleHandleA(csModuleName);
	if (!lpModule)
		return NULL;
	
	// Get function address
	lpFunction=GetProcAddress(lpModule,csFunctionName);
	if (!lpFunction)
		return NULL;
	
	// Get info about the function address
	VirtualQuery(lpFunction,&mbi,sizeof(MEMORY_BASIC_INFORMATION));
	
	// Change the protection for the region
	VirtualProtect(mbi.BaseAddress,mbi.RegionSize,PAGE_EXECUTE_READWRITE,&mbi.Protect);
	
	// Fill stub buffer with nops
	FillMemory(cStub,STUB_SIZE,NOP);

	// Begin reading at the baseaddress of the function
	pReadAddress=lpFunction;
	
	// Get the length of the first instruction(s)
	// This part is done by Z0MBiE's LDE32 v1.05
	s = disasm_main(pReadAddress); // get first instruction length
	while (s!=-1 && dwLength<JMP_SIZE)
	{
		dwLength+=s;
		pReadAddress+=s;
		s = disasm_main(pReadAddress); // next instruction length
	}

	// API code is too short too hook this way (for now ;))
	if (dwLength<JMP_SIZE)
		goto protect;
	
	// Read the first instruction(s) from the function into our stub buffer
	bRead=ReadProcessMemory(hProcess,lpFunction,cStub,dwLength,&dwBytesRead);
	
	// If reading failed or we didn't read enough bytes return NULL
	if (!bRead || dwBytesRead!=dwLength)
		goto protect;

	// Allocate space with read/write access for our "stub"
	lpStub=VirtualAlloc(NULL,STUB_SIZE,MEM_COMMIT|MEM_TOP_DOWN,PAGE_READWRITE);
	if (!lpStub)
		goto protect;
	
	// Build jmp buffer for jumping to original function 
	BuildJMPBuffer(cStub+(STUB_SIZE-JMP_SIZE),((DWORD)lpFunction+dwLength)-((DWORD)lpStub+(STUB_SIZE)));

	// Copy the "stub" buffer to process memory
	bWrite=WriteProcessMemory(hProcess,lpStub,cStub,STUB_SIZE,&dwBytesWritten);
	
	// Check that we really wrote our stub completely
	if ((!bWrite) || (dwBytesWritten!=STUB_SIZE))
		goto free;
		
	// Give our stub same access as the original function
	VirtualProtect(lpStub,STUB_SIZE,mbi.Protect,&dwOldProtect);
	
	// Allocate buffer for code replacement
	cJmpFunction=(CHAR*)malloc(dwLength);

	// Fill it with NULL
	FillMemory(cJmpFunction,dwLength,'\0');
	
	// Prepare jmpcode
	BuildJMPBuffer(cJmpFunction,(DWORD)lpJmpAddress-(DWORD)lpFunction-JMP_SIZE);

	// Write jumpcode into the function.
	bWrite=WriteProcessMemory(hProcess,lpFunction,cJmpFunction,dwLength,
								&dwBytesWritten);
	
	// Free allocated buffer
	free(cJmpFunction);

	// Check that we really wrote our jmpcode completely
	if (!bWrite || dwBytesWritten!=dwLength)
	{
		// Try to fix things, only if we wrote stuff
		if (dwBytesWritten)
			WriteProcessMemory(hProcess,lpFunction,lpStub,dwBytesWritten,
								&dwBytesWritten);
		goto free;
	}
	
	// Flush instruction cache
	FlushInstructionCache(hProcess,mbi.BaseAddress,mbi.RegionSize);

	goto protect;

free:
	// Release the stub buffer
	VirtualFree(lpStub,dwBytesWritten,MEM_RELEASE);

	// Make sure that it is NULL
	lpStub=NULL;

protect:
	// Restore the protection for the region
	VirtualProtect(mbi.BaseAddress,mbi.RegionSize,mbi.Protect,&dwOldProtect);

	// Return the address of our stub
	return lpStub;
}

BOOL UnHookFunctionInCurrentProcess(LPCSTR csModuleName,LPCSTR csFunctionName,LPVOID lpStubAddress)
{
	LPVOID lpModule,lpFunction;
	MEMORY_BASIC_INFORMATION mbiFunction,mbiStub;
	BOOL bWrite,bReturn;
	HANDLE hProcess=GetCurrentProcess();
	DWORD  dwBytesWritten;
	INT iCount;

	// Get module address
	lpModule=GetModuleHandleA(csModuleName);
	if (!lpModule)
		return FALSE;
	
	// Get function address
	lpFunction=GetProcAddress(lpModule,csFunctionName);
	if (!lpFunction)
		return FALSE;

	if (!lpStubAddress)
		return FALSE;

	// Get info about the function address
	VirtualQuery(lpFunction,&mbiFunction,sizeof(MEMORY_BASIC_INFORMATION));
	
	// Change the protection for the region
	VirtualProtect(mbiFunction.BaseAddress,mbiFunction.RegionSize,PAGE_EXECUTE_READWRITE,&mbiFunction.Protect);

	// Get info about the stub address
	VirtualQuery(lpStubAddress,&mbiStub,sizeof(MEMORY_BASIC_INFORMATION));
	
	// Change the protection for the region
	VirtualProtect(mbiStub.BaseAddress,mbiStub.RegionSize,PAGE_EXECUTE_READWRITE,&mbiStub.Protect);
	
	// Read and write stub original function code to the function
	iCount=0;
	while ((iCount<JMP_SIZE) || (((unsigned char)*(unsigned char*)lpFunction)==0))
	{
		bWrite=WriteProcessMemory(hProcess,lpFunction,lpStubAddress,1,&dwBytesWritten);
		if (!bWrite)
		{
			bReturn=FALSE;
			goto protect;
		}
		((unsigned char*)lpFunction)++;
		((unsigned char*)lpStubAddress)++;
		iCount++;
	}

	// Free the stub buffer
	VirtualFree(lpStubAddress,STUB_SIZE,MEM_RELEASE);

	// Stub = NULL
	lpStubAddress=NULL;

	// Everything's ok
	bReturn=TRUE;

protect:
	// Change the protection for the region
	VirtualProtect(mbiFunction.BaseAddress,mbiFunction.RegionSize,mbiFunction.Protect,NULL);
	
	return bReturn;
}