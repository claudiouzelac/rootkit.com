#ifndef __HE4_HOOK_INVISIBLE_DEVICE
 #define __HE4_HOOK_INVISIBLE_DEVICE
//#define HE4_DEBUG

extern "C"
{
 #include "ntddk.h"
}

#include "stdio.h"
#include "string.h"

//#define __HE4_NDIS_EXIST
#define HOOK_DRIVER_OBJECT

#include "He4Command.h"
#include "../CommonClasses/Include/NtoskrnlUndoc.h"
#include "../CommonClasses/PeFile/pefile.h"
#include "UnlockClientsList.h"
#include "FileSystemHook.h"
//#include "RegistryHook.h"
#include "../CommonClasses/Misc/Misc.h"
#include "../CommonClasses/KMemoryManager/KMemoryManager.h"

#ifdef HOOK_DRIVER_OBJECT
#include "DriverObjectHook.h"
#endif //HOOK_DRIVER_OBJECT

//
// Print macro that only turns on when debugging is on
//
#ifdef HE4_DEBUG
#define DbgPrintMain(arg) DbgPrint arg
#else
#define DbgPrintMain(arg)
#endif

//***********************************************************************//

//
// The maximum registry path length that will be copied
//
#define MAXPATHLEN     1024


#define HE4_NATIVE_API_COUNT                    5

#define HE4_STATE_HOOK_FILE_SYSTEM              0x00000001
#define HE4_STATE_HOOK_REGISTRY                 0x00000002
#define HE4_STATE_HOOK_FILE_SYSTEM_FIRST        0x00000004
#define HE4_STATE_HOOK_REGISTRY_FIRST           0x00000008
#define HE4_STATE_INHERIT_DATA                  0x00000010
#define HE4_STATE_HOOK_DRIVER_OBJECT            0x00000020


typedef NTSTATUS (__stdcall *DRIVER_ENTRY)(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
typedef BOOLEAN (__stdcall *DRIVER_UNLOAD)(VOID);

extern "C"
__declspec(dllexport)
NTSTATUS _InvisibleDriverEntry(IN PDRIVER_OBJECT DriverObject,
                               IN PUNICODE_STRING RegistryPath);
extern "C"
__declspec(dllexport)
BOOLEAN  _InvisibleDriverUnload(VOID);

BOOLEAN  InstallDriver(IN PDRIVER_OBJECT DriverObject,
                       IN PUNICODE_STRING RegistryPath);

//
// Gate functions to the Zw* functions
//
NTSTATUS _ZwGetLocalBase(OUT ULONG* lpdwBase);
NTSTATUS _ZwGetInvisibleDriverUnloadAddr(OUT ULONG* lpdwAddress);
NTSTATUS _ZwGetCallbackTableAddress(OUT ULONG* pdwCallbackTableAddress);

//
// Implementation Zw* functions
//
NTSTATUS ZwGetVersion(OUT ULONG* lpdwVersion);
NTSTATUS ZwGetLocalBase(OUT ULONG* lpdwAddress);
NTSTATUS ZwDispatchFunction(IN ULONG dwProcessId, IN ULONG dwThreadId, IN ULONG IoControlCode,
                            IN PVOID InputBuffer, IN ULONG InputBufferLength,
                            OUT PVOID OutputBuffer, IN ULONG OutputBufferLength, 
                            OUT ULONG *lpBytesReturned);
NTSTATUS ZwGetInvisibleDriverUnloadAddr(OUT ULONG* lpdwAddress);
NTSTATUS ZwGetCallbackTableAddress(OUT ULONG* pdwCallbackTableAddress);


#define CALLBACK_TABLE_SIZE          0x1000
#define CALLBACK_TABLE_ITEM_SIZE     8
#define CREATE_THREAD_NOTIFY         0
#define CREATE_PROCESS_NOTIFY        1

typedef struct tag_CREATE_CLIENT_WI
{
  WORK_QUEUE_ITEM   m_WorkItem;
  ULONG             m_dwClientId;
} CREATE_CLIENT_WI, *PCREATE_CLIENT_WI;

class KTdiStreamSocket;

typedef struct tag_SERVER_WI
{
  WORK_QUEUE_ITEM   m_WorkItem;
  KEVENT            m_kDestroyEvent;
  KEVENT            m_kExitEvent;
  BOOLEAN           m_bClosed;
  PVOID             m_ServerThreadObjectPointer;
  KTdiStreamSocket* m_pStreamSocket;
} SERVER_WI, *PSERVER_WI;

VOID CreateThreadNotifyRoutine(IN HANDLE ProcessId, IN HANDLE ThreadId, IN BOOLEAN Create);
VOID CreateProcessNotifyRoutine(IN HANDLE ParentProcessId, IN HANDLE ProcessId, IN BOOLEAN Create);

void CreateThreadNotifyThread(IN PCREATE_CLIENT_WI pWorkItem);
void CreateProcessNotifyThread(IN PCREATE_CLIENT_WI pWorkItem);

BOOLEAN SetNotifyRoutines(ULONG dwNotifyIndex, BOOLEAN bFirst);
BOOLEAN ResetNotifyRoutines(ULONG dwNotifyIndex);

BOOLEAN  CreateKHeaps(void);
VOID     DestroyKHeaps(void);

BOOLEAN  He4QueryStatistic(PHE4_STATISTIC_INFO pStatInfo);
#endif //__HE4_HOOK_INVISIBLE_DEVICE
