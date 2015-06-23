#ifndef __NTOSKRNL_H_
#define __NTOSKRNL_H_

#include <ntddk.h>

// NOTE: x64 is a LONG RVA
extern PULONG *KeServiceDescriptorTable;

typedef 
NTSTATUS
(NTAPI *NT_TERMINATE_PROCESS)
(
    IN  HANDLE ProcessHandle, OPTIONAL 
    IN  NTSTATUS ExitStatus 
);

NTSTATUS 
NTAPI
NtTerminateProcess
(
    IN  HANDLE      ProcessHandle, OPTIONAL 
    IN  NTSTATUS    ExitStatus 
);

typedef 
NTSTATUS
(NTAPI *NT_CREATE_FILE)
(
    OUT PHANDLE             FileHandle,
    IN  ACCESS_MASK         DesiredAccess,
    IN  POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN  PLARGE_INTEGER      AllocationSize, OPTIONAL
    IN  ULONG               FileAttributes,
    IN  ULONG               ShareAccess,
    IN  ULONG               CreateDisposition,
    IN  ULONG               CreateOptions,
    IN  PVOID               EaBuffer, OPTIONAL
    IN  ULONG               EaLength
);

NTSTATUS
NTAPI
NtCreateFile
(
    OUT PHANDLE             FileHandle,
    IN  ACCESS_MASK         DesiredAccess,
    IN  POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN  PLARGE_INTEGER      AllocationSize, OPTIONAL
    IN  ULONG               FileAttributes,
    IN  ULONG               ShareAccess,
    IN  ULONG               CreateDisposition,
    IN  ULONG               CreateOptions,
    IN  PVOID               EaBuffer, OPTIONAL
    IN  ULONG               EaLength
);

typedef 
NTSTATUS
(NTAPI *NT_OPEN_FILE)
(
    PHANDLE             FileHandle,
    ACCESS_MASK         DesiredAccess,
    POBJECT_ATTRIBUTES  ObjectAttributes,
    PIO_STATUS_BLOCK    IoStatusBlock,
    ULONG               ShareAccess,
    ULONG               OpenOptions
);


NTSTATUS
NTAPI
NtOpenFile
(
    PHANDLE             FileHandle,
    ACCESS_MASK         DesiredAccess,
    POBJECT_ATTRIBUTES  ObjectAttributes,
    PIO_STATUS_BLOCK    IoStatusBlock,
    ULONG               ShareAccess,
    ULONG               OpenOptions
);


typedef 
NTSTATUS
(NTAPI *NT_CREATE_PROCESS)
(
    OUT PHANDLE         ProcessHandle, 
    IN  ACCESS_MASK     DesiredAccess, 
    IN  POBJECT_ATTRIBUTES ObjectAttributes, OPTIONAL
    IN  HANDLE          ParentProcess, 
    IN  BOOLEAN         InheritObjectTable, 
    IN  HANDLE          SectionHandle, OPTIONAL
    IN  HANDLE          DebugPort, OPTIONAL
    IN  HANDLE          ExceptionPort OPTIONAL
);


NTSTATUS 
NTAPI 
KeSetAffinityThread  	
(
    IN  PKTHREAD    Thread,
	IN  KAFFINITY  	Affinity
);  

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS    ExitStatus;
    PVOID       TebBaseAddress;
    CLIENT_ID   ClientId;
    KAFFINITY   AffinityMask;
    KPRIORITY   Priority;
    KPRIORITY   BasePriority;
    
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

NTSTATUS
NTAPI
ZwQueryInformationProcess
(
    IN  HANDLE  ProcessHandle,
    IN  ULONG   ProcessInformationClass,
    IN  PVOID   ProcessInformation,
    IN  ULONG   ProcessInformationLength,
    IN  PULONG  ReturnLength
);


#endif // __NTOSKRNL_H_