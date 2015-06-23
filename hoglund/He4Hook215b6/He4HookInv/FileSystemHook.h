#ifndef __FILE_SYSTEM_HOOK_H
 #define __FILE_SYSTEM_HOOK_H

//#define HE4_FS_HOOK_DEBUG
//#define _HE4_FS_HOOK_DEBUG
//#define __HE4_PROTECT_ONLY_FS_OBJECT
//#define __HOOK_NTQUERYDIRECTORYFILE

extern "C"
{
 #include "ntddk.h"
}

#include "../CommonClasses/Include/NtoskrnlUndoc.h"
#include "He4Command.h"
#include "He4HookInv.h"
#include "SaveObjectsList.h"
//#include "LinkedList.h"
#include "../CommonClasses/Misc/Misc.h"
#include "../CommonClasses/KMemoryManager/KMemoryManager.h"


#ifdef HE4_FS_HOOK_DEBUG
#define DbgPrintFs(arg) DbgPrint arg
#else
#define DbgPrintFs(arg)
#endif


#ifdef _HE4_FS_HOOK_DEBUG
#define _DbgPrintFs(arg) DbgPrint arg
#else
#define _DbgPrintFs(arg)
#endif

typedef struct _QUERY_FILE_INFO
{
  PWSTR                  lpszFileName;
  ULONG                  dwSizeFileName;
  PFQD_SmallCommonBlock  lpQueryDirWin32;
  ULONG                  dwSizeQueryDirWin32;
  FILE_INFORMATION_CLASS DirectoryInfoClass;
  PKEVENT                lpNotifyEvent;
  NTSTATUS               NtStatus; 
} QUERY_FILE_INFO, *PQUERY_FILE_INFO;

typedef struct _QUERY_NT_FILE_NAME
{
  PWSTR                  lpszDosFileName;
  ULONG                  dwSizeDosFileName;
  PWSTR                  lpszNtFileName;
  ULONG                  dwSizeNtFileName;
  HANDLE                 hFile;
  OBJECT_ATTRIBUTES      ObjectAttributes;
  UNICODE_STRING         FileNameUnicodeString;
  IO_STATUS_BLOCK        IoStatusBlock;
  PKEVENT                lpNotifyEvent;
  NTSTATUS               NtStatus; 
} QUERY_NT_FILE_NAME, *PQUERY_NT_FILE_NAME;

//typedef struct _ZWCREATEFILE_PARAM
//{
//  PHANDLE              FileHandle;
//  ACCESS_MASK          DesiredAccess;
//  POBJECT_ATTRIBUTES   ObjectAttributes;
//  OUT PIO_STATUS_BLOCK IoStatusBlock;
//  PLARGE_INTEGER       AllocationSize;
//  ULONG                FileAttributes;
//  ULONG                ShareAccess;
//  ULONG                CreateDisposition;
//  ULONG                CreateOptions;
//  PVOID                EaBuffer;
//  ULONG                EaLength;
//  NTSTATUS             NtStatus;
//} ZWCREATEFILE_PARAM, *PZWCREATEFILE_PARAM;

//typedef struct _ZWOPENFILE_PARAM
//{
//  PHANDLE              FileHandle;
//  ACCESS_MASK          DesiredAccess;
//  POBJECT_ATTRIBUTES   ObjectAttributes;
//  PIO_STATUS_BLOCK     IoStatusBlock;
//  ULONG                ShareAccess;
//  ULONG                OpenOptions;
// NTSTATUS             NtStatus;
//} ZWOPENFILE_PARAM, *PZWOPENFILE_PARAM;

NTSTATUS 
CallRealIoCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    IN CREATE_FILE_TYPE CreateFileType,
    IN PVOID ExtraCreateParameters OPTIONAL,
    IN ULONG Options
    );

NTSTATUS 
HookIoCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    IN CREATE_FILE_TYPE CreateFileType,
    IN PVOID ExtraCreateParameters OPTIONAL,
    IN ULONG Options
    );

#ifdef __HOOK_NTQUERYDIRECTORYFILE
NTSTATUS
HookNtQueryDirectoryFile(
    IN HANDLE DirectoryFileHandle,
    IN HANDLE EventHandle,             // optional //
    IN PIO_APC_ROUTINE ApcRoutine,     // optional //
    IN PVOID ApcContext,               // optional //
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    IN FILE_INFORMATION_CLASS DirectoryInfoClass,
    IN BOOLEAN ByOne,
    IN PUNICODE_STRING SearchTemplate, // optional //
    IN BOOLEAN Reset
    );
#endif //__HOOK_NTQUERYDIRECTORYFILE

VOID    HookFileSystem(void);
BOOLEAN UnhookFileSystem(void);

#ifdef __HE4_PROTECT_ONLY_FS_OBJECT
BOOLEAN   IsNotFileSystemObject(PWSTR pwszFullObjectName);
#endif //__HE4_PROTECT_ONLY_FS_OBJECT
BOOLEAN   PathCorrection(PWSTR pwszFullObjectName, ACCESS_MASK DesiredAccess, ULONG Disposition, ULONG Options);

VOID      FillQueryDirectoryBufferItem(PFILEINFO pFileInfo, PFQD_SmallCommonBlock pQueryDir, FILE_INFORMATION_CLASS DirectoryInfoClass);
BOOLEAN   QueryFileInfo(PWSTR lpszFileName, ULONG dwSizeFileName, PFQD_SmallCommonBlock lpQueryDirWin32, ULONG dwSizeQueryDirWin32, FILE_INFORMATION_CLASS DirectoryInfoClass);
void      QueryFileThread(IN PVOID lpContext);

#endif //__FILE_SYSTEM_HOOK_H
