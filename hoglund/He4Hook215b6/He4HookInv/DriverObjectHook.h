#ifndef __DRIVER_OBJECT_HOOK_H
 #define __DRIVER_OBJECT_HOOK_H

extern "C"
{
 #include "ntddk.h"
}
#include "../CommonClasses/Include/NtoskrnlUndoc.h"

#include "He4Command.h"
#include "He4HookInv.h"
#include "../CommonClasses/Misc/Misc.h"
#include "../CommonClasses/KMemoryManager/KMemoryManager.h"
#include "../CommonClasses/KBinaryTree/KBinaryTree.h"


#ifndef MUTEX_INIT
 #define MUTEX_INIT(v)      KeInitializeMutex( &v, 0 )
#endif
#ifndef MUTEX_WAIT
 #define MUTEX_WAIT(v)      KeWaitForMutexObject( &v, Executive, KernelMode, FALSE, NULL )
#endif
#ifndef MUTEX_RELEASE
#define MUTEX_RELEASE(v)   KeReleaseMutex( &v, FALSE )
#endif

typedef struct tag_QUERY_DIRECTORY_FILE_PARAM
{
  //PIRP                   m_pIrp;
  PDRIVER_DISPATCH       m_pMajorFunction;
  PDEVICE_OBJECT         m_pDeviceObject;
  PFILE_OBJECT           m_pFileObject;
  PIO_STATUS_BLOCK       m_pIoStatusBlock;
  PVOID                  m_Buffer;
  ULONG                  m_BufferLength;
  FILE_INFORMATION_CLASS m_DirectoryInfoClass;
  BOOLEAN                m_ByOne;
  UNICODE_STRING         m_SearchTemplate;
  BOOLEAN                m_Reset;
  BOOLEAN                m_Index;
  ULONG                  m_dwIndex;
} QUERY_DIRECTORY_FILE_PARAM, *PQUERY_DIRECTORY_FILE_PARAM;

typedef struct tag_TREAT_IRP_INFO
{
  #ifndef __WIN2K
  WORK_QUEUE_ITEM   m_WorkItem;
  #else
  PIO_WORKITEM      m_pWorkItem;
  #endif //__WIN2K
  PETHREAD          m_pCurrentThread;
  PEPROCESS         m_pCurrentProcess;
  KEVENT            m_Event;
  //PIO_STACK_LOCATION  m_pCurrentIrpStack;
  IO_STACK_LOCATION m_CurrentIrpStack;
  PQUERY_DIRECTORY_FILE_PARAM m_pQueryDirParam;
  BOOLEAN           m_bGetObjectNameError;
  NTSTATUS          NtStatus;
} TREAT_IRP_INFO, *PTREAT_IRP_INFO;


#define DRV_INFO_STATE_DRIVER_UNLOAD    0x00000001

typedef struct tag_HOOK_DRIVER_INFO
{
//  ULONG               m_dwStates;
  PDRIVER_OBJECT      m_pDriverObject;
  PDRIVER_UNLOAD      m_pDriverUnload; // 
//  PFAST_IO_DISPATCH   m_pFastIoDispatchOriginal;
//  FAST_IO_DISPATCH    m_FastIoDispatchHook;
  PDRIVER_DISPATCH    m_MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} HOOK_DRIVER_INFO, *PHOOK_DRIVER_INFO;

typedef struct tag_IRP_COMPLETION_INFO
{
  PIO_COMPLETION_ROUTINE  m_pOriginalCompletionRoutine;
  PVOID                   m_pOriginalContext;
  UCHAR                   m_OriginalControl;
  IO_STACK_LOCATION       m_CurrentIrpStack;
  //PHOOK_DRIVER_INFO       m_pDriverInfo;
  //ULONG                   m_dwMajorFunction;
  //ULONG                   m_dwMinorFunction;
  QUERY_DIRECTORY_FILE_PARAM m_QueryDirectoryFileParam;
} IRP_COMPLETION_INFO, *PIRP_COMPLETION_INFO;

VOID HookDriverObjects(VOID);
BOOLEAN UnHookDriverObjects(VOID);

VOID
DriverUnload(
     IN  PDRIVER_OBJECT  DriverObject
     );

NTSTATUS
DriverObjectDispatch(
     IN PDEVICE_OBJECT DeviceObject,
     IN PIRP Irp
     );

NTSTATUS
IntermediateIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
HighLevelIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
IrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIRP_COMPLETION_INFO pInfo
    );


BOOLEAN        AddHookedDriverIntoTree(PDRIVER_OBJECT pDriverObject);
BOOLEAN        DelHookedDriverFromTree(PDRIVER_OBJECT pDriverObject);
VOID           ClearHookedDriverTree(VOID);
int            CompareDriver(PHOOK_DRIVER_INFO pDriverInfo1, PHOOK_DRIVER_INFO pDriverInfo2, ULONG dwCompareParam);
BOOLEAN        IsRightDeviceTypeForFunc(DEVICE_TYPE DeviceType, ULONG dwMajorFunction);

void           AddDriverObject(IN PDRIVER_OBJECT pDriverObject);

NTSTATUS       TreatmentCreateObjectIRP(PWSTR pwszFullObjectName, PIO_STACK_LOCATION pCurrentIrpStack, PETHREAD pCurrentThread, PEPROCESS pCurrentProcess);
NTSTATUS       TreatmentQueryDirectoryIRP(PWSTR pwszFullObjectName, ULONG dwObjectNameSize, ULONG dwObjectNameBufferSize, PQUERY_DIRECTORY_FILE_PARAM pQueryDirParam, PETHREAD pCurrentThread, PEPROCESS pCurrentProcess);

#ifndef __WIN2K
void TreatmentIrpThread(IN PTREAT_IRP_INFO pTreatIrpInfo);
#else
void TreatmentIrpThread(IN PDEVICE_OBJECT DeviceObject, IN PTREAT_IRP_INFO pTreatIrpInfo);
#endif //__WIN2K

void ScanObjectDirectory(PWSTR pwszObjectDir, NTQUERYDIRECTORYOBJECT pZwQueryDirectoryObject);

#endif //__DRIVER_OBJECT_HOOK_H