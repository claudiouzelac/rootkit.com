/*++

Module Name:

    nt.h

Abstract:

    Useful declarations.

Author:

    90210 05-Oct-2004

--*/

#ifndef _NT_
#define _NT_

#ifdef __cplusplus
extern "C" {
#endif

NTSYSAPI
NTSTATUS
NTAPI
NtMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );


NTSYSAPI
NTSTATUS
NTAPI
NtOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    );

NTSYSAPI
NTSTATUS
NTAPI
NtCreateSection(
    PHANDLE  SectionHandle,
    ACCESS_MASK  DesiredAccess,
    POBJECT_ATTRIBUTES  ObjectAttributes,
    PLARGE_INTEGER  MaximumSize OPTIONAL,
    ULONG  SectionPageProtection,
    ULONG  AllocationAttributes,
    HANDLE  FileHandle
    ); 

NTSYSAPI
NTSTATUS 
NTAPI
NtClose(
  IN HANDLE  Handle
    );

NTSYSAPI
NTSTATUS
NTAPI
NtReadFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

NTSTATUS
NTAPI
ZwQuerySystemInformation(    
    DWORD    SystemInformationClass,
    PVOID    SystemInformation,
    ULONG    SystemInformationLength,
    PULONG    ReturnLength
    );

int __cdecl _snwprintf(wchar_t *, size_t, const wchar_t *, ...);

#ifdef __cplusplus
}
#endif

#define    SystemModuleInformation    11

#define SEC_IMAGE         0x1000000     

#include <pshpack1.h>

typedef struct _SYSTEM_MODULE_INFORMATION {//Information Class 11
    ULONG    Reserved[2];
    PVOID    Base;
    ULONG    Size;
    ULONG    Flags;
    USHORT    Index;
    USHORT    Unknown;
    USHORT    LoadCount;
    USHORT    ModuleNameOffset;
    CHAR    ImageName[256];
}SYSTEM_MODULE_INFORMATION,*PSYSTEM_MODULE_INFORMATION;

typedef struct {
    DWORD    dwNumberOfModules;
    SYSTEM_MODULE_INFORMATION    smi;
} MODULES, *PMODULES;

#include <poppack.h>


#endif	// _NT_