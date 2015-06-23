// standard includes 
#include <winsock2.h>
#include <windows.h>
#include <time.h>

// own includes
#include "ntdll.h"

#include "ntresumethread.h"
#include "ldrloaddll.h"
#include "ldrunloaddll.h"
#include "ntquerysysteminformation.h"
#include "ntquerydirectoryfile.h"
#include "ntvdmcontrol.h"
#include "ntdeviceiocontrolfile.h"
#include "ntenumeratekey.h"
#include "ntenumeratevaluekey.h"
#include "enumservicesstatusa.h"
#include "enumservicegroupw.h"
#include "enumservicesstatusexa.h"
#include "enumservicesstatusexw.h"
#include "ntqueryvolumeinformationfile.h"
#include "ntopenfile.h"
#include "ntcreatefile.h"
#include "ntreadfile.h"
#include "ntreadvirtualmemory.h"
#include "ntqueryvirtualmemory.h"
#include "ntopenprocess.h"
#include "ntsavekey.h"
#include "ntsavemergedkeys.h"

#ifndef __SAFE__
#define __SAFE__

NTSTATUS WINAPI SafeNtResumeThread(HANDLE,PDWORD);
NTSTATUS WINAPI SafeLdrLoadDll(PWCHAR,ULONG,PUNICODE_STRING,PHANDLE);
NTSTATUS WINAPI SafeLdrUnloadDll(HANDLE);
NTSTATUS WINAPI SafeNtQuerySystemInformation(ULONG,PVOID,ULONG,PULONG);
NTSTATUS WINAPI SafeNtQueryDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
NTSTATUS WINAPI SafeNtVdmControl(ULONG,PVOID);
NTSTATUS WINAPI SafeNtDeviceIoControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS WINAPI SafeNtEnumerateKey(HANDLE,ULONG,KEY_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS WINAPI SafeNtEnumerateValueKey(HANDLE,ULONG,KEY_VALUE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS WINAPI SafeNtQueryVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);
NTSTATUS WINAPI SafeNtOpenFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG);
NTSTATUS WINAPI SafeNtCreateFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
NTSTATUS WINAPI SafeNtReadFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS WINAPI SafeNtReadVirtualMemory(HANDLE,PVOID,PVOID,ULONG,PULONG);
NTSTATUS WINAPI SafeNtQueryVirtualMemory(HANDLE,PVOID,MEMORY_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS WINAPI SafeNtOpenProcess(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PCLIENT_ID);
NTSTATUS WINAPI SafeNtSaveKey(HANDLE,HANDLE);
NTSTATUS WINAPI SafeNtSaveMergedKeys(HANDLE,HANDLE,HANDLE);

#endif