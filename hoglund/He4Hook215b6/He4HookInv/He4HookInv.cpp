#include "He4HookInv.h"
#include "../CommonClasses/KTdiStreamSocket/KTdiStreamSocket.h"
#include "../CommonClasses/KInterlockedCounter/KInterlockedCounter.h"

//#define TRUE_COVERAGE_EXIST

extern KMUTEX              LockMutexToUnlockList;
extern KMUTEX              LockMutexToFileSaveList;
//extern KMUTEX              LockMutexToIoCreateFile;
#ifdef HOOK_DRIVER_OBJECT
//extern KMUTEX              LockMutexToHookedDeviceTree;
extern KMUTEX              LockMutexToHookedDriverTree;
#endif //HOOK_DRIVER_OBJECT
extern KMUTEX              LockMutexToKeySaveList;
extern KMUTEX              LockMutexToKeyOpenList;
//extern NTQUERYDIRECTORYOBJECT pZwQueryDirectoryObject;

//
// Heaps
//
extern KHEAP               hKHeapUnlockList;
extern KHEAP               hKHeapFSDefault;
extern KHEAP               hKHeapSOFileList;
#ifdef HOOK_DRIVER_OBJECT
extern KHEAP               hKHeapDHDefault;
#endif //HOOK_DRIVER_OBJECT
#ifdef __LL_USE_KHEAP
 extern KHEAP               hKHeapLLDefault;
#endif //__LL_USE_KHEAP
#ifdef __MISC_USE_KHEAP
 extern KHEAP               hKHeapMiscDefault;
#endif //__MISC_USE_KHEAP
//#ifdef __BTREE_USE_KHEAP
// extern KHEAP               hKHeapBTreeDefault;
//#endif //__BTREE_USE_KHEAP

ULONG               dwDeviceState = HE4_STATE_INHERIT_DATA;
//extern KSEMAPHORE   FileSystemBusySemaphore;

//KSEMAPHORE          CreateThreadCallbackBusySemaphore;
//KSEMAPHORE          CreateProcessCallbackBusySemaphore;
KInterlockedCounter*  CreateThreadCallbackBusy = NULL;
KInterlockedCounter*  CreateProcessCallbackBusy = NULL;

VOID*               pCallbackTableAddress = 0;

ULONG               dwCreateThreadNotifyRoutineAddress = (ULONG) CreateThreadNotifyRoutine;
ULONG               dwCreateProcessNotifyRoutineAddress = (ULONG) CreateProcessNotifyRoutine;

//#define __TDI_SERVER
#ifdef __TDI_SERVER
PSERVER_WI          pServerWorkItem1 = NULL;
PSERVER_WI          pServerWorkItem2 = NULL;

PSERVER_WI  OpenServer();
BOOLEAN CloseServer(IN PSERVER_WI pServerWorkItem);
void ServerThread(IN PSERVER_WI pWorkItem);
#endif //__TDI_SERVER

/****** Entry routine.  Set everything up. *****/
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
                     IN PUNICODE_STRING RegistryPath)
{
  ULONG dwNtBuildNumber;
//  DEVICE_OBJECT   DeviceObject;
//  PDEVICE_OBJECT  pDeviceObject = &DeviceObject;
//  IRP             Irp;
//  PIRP            pIrp = &Irp;
//  OBJECT_HEADER     ObjectHeader;
//  POBJECT_HEADER    pObjectHeader = &ObjectHeader;
//  int               nSizeOfObjectHeader = sizeof(OBJECT_HEADER);
//  int               nSizeOfSubHeader = sizeof(OBJECT_SUB_HEADER_INFO);

  #ifndef TRUE_COVERAGE_EXIST
  NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
  #else
  NTSTATUS NtStatus = STATUS_SUCCESS;
  #endif //TRUE_COVERAGE_EXIST

//  dwNtBuildNumber = sizeof(DRIVER_OBJECT) + sizeof(DRIVER_EXTENSION); // 0xa8 + 0x14
//  if (dwNtBuildNumber == 0xbc)
//    dwNtBuildNumber = 0;

//  pDeviceObject->DeviceObjectExtension = 1;

//  pIrp->Tail.Overlay.CurrentStackLocation->DeviceObject = 1;

//  _IofCallDriverNative(pDeviceObject, pIrp);
//  HookIofCallDriver(pDeviceObject, pIrp);
//  __asm int 1h
//  GetObjectByPath(L"\\Driver\\Parallel", IoDriverObjectType);

  dwNtBuildNumber = (*NtBuildNumber) & 0x00ffffff;
  #ifndef __WIN2K
  switch (dwNtBuildNumber)
  {
    case 1381: // WinNT 4.0
         break;
    case 2195: // Win2K
         NtStatus = STATUS_NO_SUCH_DEVICE;
    default:
         NtStatus = STATUS_NO_SUCH_DEVICE;
  }
  #else 
  switch (dwNtBuildNumber)
  {
    case 1381: // WinNT 4.0
         NtStatus = STATUS_NO_SUCH_DEVICE;
    case 2195: // Win2K
         break;
    default:
         if (dwNtBuildNumber >= 1946)
           break;
         NtStatus = STATUS_NO_SUCH_DEVICE;
  }
  #endif //__WIN2K

  if (NtStatus != STATUS_NO_SUCH_DEVICE)
  {
    MmQuerySystemSize();
    __try
    {
      if (!InstallDriver(DriverObject, RegistryPath))
        NtStatus = STATUS_NO_SUCH_DEVICE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      NtStatus = STATUS_NO_SUCH_DEVICE;
    }
  }

//  if(!InstallDriver(DriverObject, RegistryPath))
//     return _InvisibleDriverEntry(DriverObject, RegistryPath);
                                                                
  return NtStatus; 
}

BOOLEAN InstallDriver(IN PDRIVER_OBJECT DriverObject,
                      IN PUNICODE_STRING RegistryPath)
{
  PIMAGE_DOS_HEADER      pDriverModule = 0;
  PIMAGE_NT_HEADERS32    pNtHeader = 0;
  PIMAGE_DOS_HEADER      pNewDriverPlace = 0;
  PIMAGE_NT_HEADERS32    pNtHeaderNewPlace = 0;
  PIMAGE_SECTION_HEADER  pReloc = 0;
  PIMAGE_DATA_DIRECTORY  pRelocDir;
  DRIVER_ENTRY           dwFunctionAddr = 0;
  NTSTATUS               NtStatus;
  HANDLE                 hSystemImage;

//  return FALSE;

  #ifndef TRUE_COVERAGE_EXIST
  pDriverModule = (PIMAGE_DOS_HEADER) LocateBase((void *)InstallDriver);
  pNtHeader = (PIMAGE_NT_HEADERS32)(((char *)pDriverModule) + pDriverModule->e_lfanew);
  
  pNewDriverPlace = (PIMAGE_DOS_HEADER)ExAllocatePool(NonPagedPool, (pNtHeader->OptionalHeader.SizeOfImage));
  if (!pNewDriverPlace)
    return FALSE;
  memcpy((PVOID) pNewDriverPlace, (PVOID) pDriverModule, pNtHeader->OptionalHeader.SizeOfImage);

  pNtHeaderNewPlace = (PIMAGE_NT_HEADERS32)(((char *)pNewDriverPlace) + pNewDriverPlace->e_lfanew);
//  __asm { int 3h };
//  pReloc = GetSection(IMAGE_FIRST_SECTION32(pNtHeaderNewPlace), ".reloc", pNtHeaderNewPlace->FileHeader.NumberOfSections);
//  if(!pReloc)
//    {
//     ExFreePool(pNewDriverPlace);
//     return FALSE;
//    }
  
  pRelocDir = &pNtHeaderNewPlace->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
  RelocBuffer((DWORD)pNewDriverPlace, (DWORD)pNewDriverPlace, (DWORD)((DWORD)pNewDriverPlace + pNtHeaderNewPlace->OptionalHeader.SizeOfImage),
              (RELO_HEADER*)(pRelocDir->VirtualAddress+(DWORD)pNewDriverPlace), pRelocDir->Size, (DWORD)((DWORD)pNewDriverPlace-(DWORD)pDriverModule));

  dwFunctionAddr = (DRIVER_ENTRY) NativeGetProcAddress((DWORD)pNewDriverPlace, "__InvisibleDriverEntry@8");
  if (!dwFunctionAddr)
  {
    ExFreePool(pNewDriverPlace);
    return FALSE;
  }
//  ExFreePool(pNewDriverPlace);
//  return FALSE;
//  __asm { int 3h };            

  NtStatus = dwFunctionAddr(DriverObject, RegistryPath);
  if (!NT_SUCCESS(NtStatus))
  {
    ExFreePool(pNewDriverPlace);
    return FALSE;
  }

  #else
  pDriverModule = (PIMAGE_DOS_HEADER) LocateBase((void *)InstallDriver);
  pNewDriverPlace = pDriverModule;
  dwFunctionAddr = (DRIVER_ENTRY) NativeGetProcAddress((DWORD)pNewDriverPlace, "__InvisibleDriverEntry@8");
  if (!dwFunctionAddr)
  {
    return FALSE;
  }

  NtStatus = dwFunctionAddr(DriverObject, RegistryPath);
  if (!NT_SUCCESS(NtStatus))
  {
    return FALSE;
  }
  #endif //TRUE_COVERAGE_EXIST

  return TRUE;
}

//******************************************************************//
//********************** Notify routines ***************************//
//******************************************************************//

VOID CreateThreadNotifyRoutine(IN HANDLE ProcessId, IN HANDLE ThreadId, IN BOOLEAN Create)
{
//  PETHREAD                lpEThread;
  PCREATE_CLIENT_WI  pWorkItem;

  DbgPrintMain(("CreateThreadNotifyRoutine: Thread %08x %s\n", ThreadId, (Create == TRUE) ? "Start" : "Stop"));

  //KeReleaseSemaphore(&CreateThreadCallbackBusySemaphore, 0, 1, FALSE);
  ++CreateThreadCallbackBusy;

  if (Create == FALSE)
  {
    pWorkItem = (PCREATE_CLIENT_WI)_AllocatePoolFromKHeap(hKHeapUnlockList, sizeof(CREATE_CLIENT_WI));
    if (pWorkItem != NULL)
    {
      //KeReleaseSemaphore(&CreateThreadCallbackBusySemaphore, 0, 1, FALSE);
      ++CreateThreadCallbackBusy;
      pWorkItem->m_dwClientId = (ULONG)ThreadId;
      ExInitializeWorkItem(&pWorkItem->m_WorkItem, (PWORKER_THREAD_ROUTINE)CreateThreadNotifyThread, (PVOID)pWorkItem);
      ExQueueWorkItem(&pWorkItem->m_WorkItem, DelayedWorkQueue);
    }
  }

  //KeWaitForSingleObject(&CreateThreadCallbackBusySemaphore, Executive, KernelMode, FALSE, NULL);
  --CreateThreadCallbackBusy;

//  if (Create == FALSE && NT_SUCCESS(PsLookupThreadByThreadId((ULONG)ThreadId, &lpEThread)))
//  {
//    DelUnlockList((ULONG)ThreadId, lpEThread);
//    ObDereferenceObject(lpEThread);
//  }
}

void CreateThreadNotifyThread(IN PCREATE_CLIENT_WI pWorkItem)
{
  DelUnlockList((ULONG)pWorkItem->m_dwClientId, HE4_UNLOCK_CLIENT_UNKNOWN);
  FreePoolToKHeap(hKHeapUnlockList, pWorkItem);
  //KeWaitForSingleObject(&CreateThreadCallbackBusySemaphore, Executive, KernelMode, FALSE, NULL);
  --CreateThreadCallbackBusy;
}

VOID CreateProcessNotifyRoutine(IN HANDLE ParentProcessId, IN HANDLE ProcessId, IN BOOLEAN Create)
{
  PCREATE_CLIENT_WI   pWorkItem;

  DbgPrintMain(("CreateProcessNotifyRoutine: Process %08x %s\n", ProcessId, (Create == TRUE) ? "Start" : "Stop"));

  //KeReleaseSemaphore(&CreateProcessCallbackBusySemaphore, 0, 1, FALSE);
  ++CreateProcessCallbackBusy;

  if (Create == FALSE)
  {
    pWorkItem = (PCREATE_CLIENT_WI)_AllocatePoolFromKHeap(hKHeapUnlockList, sizeof(CREATE_CLIENT_WI));
    if (pWorkItem != NULL)
    {
      //KeReleaseSemaphore(&CreateProcessCallbackBusySemaphore, 0, 1, FALSE);
      ++CreateProcessCallbackBusy;
      pWorkItem->m_dwClientId = (ULONG)ProcessId;
      ExInitializeWorkItem(&pWorkItem->m_WorkItem, (PWORKER_THREAD_ROUTINE)CreateProcessNotifyThread, (PVOID)pWorkItem);
      ExQueueWorkItem(&pWorkItem->m_WorkItem, DelayedWorkQueue);
    }

//    DelUnlockList((ULONG)ProcessId, HE4_UNLOCK_CLIENT_UNKNOWN);
  }

  //KeWaitForSingleObject(&CreateProcessCallbackBusySemaphore, Executive, KernelMode, FALSE, NULL);
  --CreateProcessCallbackBusy;
}

void CreateProcessNotifyThread(IN PCREATE_CLIENT_WI pWorkItem)
{
  DelUnlockList((ULONG)pWorkItem->m_dwClientId, HE4_UNLOCK_CLIENT_UNKNOWN);
  FreePoolToKHeap(hKHeapUnlockList, pWorkItem);
  //KeWaitForSingleObject(&CreateProcessCallbackBusySemaphore, Executive, KernelMode, FALSE, NULL);
  --CreateProcessCallbackBusy;
}

//******************************************************************//
//*********** Export function __InvisibleDriverEntry@8 *************//
//******************************************************************//

extern "C"
NTSTATUS _InvisibleDriverEntry(IN PDRIVER_OBJECT DriverObject,
                               IN PUNICODE_STRING RegistryPath)
{
  VOID         *mySSTAT[HE4_NATIVE_API_COUNT];
  BYTE          mySSTPT[HE4_NATIVE_API_COUNT];
  LPSSTAT       lpMySSTAT = NULL;
  LPSSTPT       lpMySSTPT = NULL;
  LPSSDT        lpSSDT = (LPSSDT) KeServiceDescriptorTable;
  LPSSDT        _lpSSDT = NULL; //(LPSSDT) ((char*)KeServiceDescriptorTable-0x230);
  LPSSD         lpSSD;
  LPSSD         _lpSSD;
  ULONG         dwAddress = 0;
  NTSTATUS      NtStatus;
  DRIVER_UNLOAD dwFunctionAddr = 0;
  BOOLEAN       bAlreadyExist = FALSE;
  #ifdef TRUE_COVERAGE_EXIST
  WCHAR         NtPathName[256];
  #endif //TRUE_COVERAGE_EXIST

  MUTEX_INIT(LockMutexToUnlockList);
  MUTEX_INIT(LockMutexToFileSaveList);
//  MUTEX_INIT(LockMutexToIoCreateFile);
  #ifdef HOOK_DRIVER_OBJECT
//  MUTEX_INIT(LockMutexToHookedDeviceTree);
  MUTEX_INIT(LockMutexToHookedDriverTree);
  #endif //HOOK_DRIVER_OBJECT

  DbgPrintMain(("He4HookInv: _InvisibleDriverEntry: CreateKHeaps() - before!!!\n"));
  if (CreateKHeaps() == FALSE)
    return STATUS_UNSUCCESSFUL;
  DbgPrintMain(("He4HookInv: _InvisibleDriverEntry: CreateKHeaps() - after!!!\n"));

  _lpSSDT = FindShadowTable();

  if (_lpSSDT != NULL)
  {
    lpSSD = &(lpSSDT->SystemServiceDescriptors[KE_SERVICE_TABLE_INDEX]);
    _lpSSD = &(_lpSSDT->SystemServiceDescriptors[KE_SERVICE_TABLE_INDEX]);
    if (
        lpSSD->lpSystemServiceTableAddressTable &&
        lpSSD->lpSystemServiceTableParameterTable
       )
    {
      bAlreadyExist = TRUE;
    
      NtStatus = _ZwGetLocalBase(&dwAddress);
      if (!NT_SUCCESS(NtStatus))
      {
        DestroyKHeaps();
        return NtStatus;
      }

      NtStatus = _ZwGetCallbackTableAddress((ULONG*)&pCallbackTableAddress);
      if (!NT_SUCCESS(NtStatus))
      {
        DestroyKHeaps();
        return NtStatus;
      }
      
      dwFunctionAddr = (DRIVER_UNLOAD) NativeGetProcAddress((DWORD)dwAddress, "__InvisibleDriverUnload@0");
      if (!dwFunctionAddr)
      {
        if (!NT_SUCCESS(_ZwGetInvisibleDriverUnloadAddr((PULONG)&dwFunctionAddr)))
        {
          DestroyKHeaps();
          return STATUS_UNSUCCESSFUL;
        }
      }
      
      DbgPrintMain(("He4HookInv: _InvisibleDriverEntry: _InvisibleDriverUnload - start!!!\n"));
      if (dwFunctionAddr() == FALSE)
      {
        DbgPrintMain(("He4HookInv: _InvisibleDriverEntry: _InvisibleDriverUnload = FALSE!!!\n"));
        DestroyKHeaps();
        return STATUS_UNSUCCESSFUL;
      }
      
      KeEnterCriticalRegion();
      
      ExFreePool(lpSSD->lpSystemServiceTableAddressTable);
      ExFreePool(lpSSD->lpSystemServiceTableParameterTable);
      
      lpSSD->lpSystemServiceTableAddressTable = 0;
      lpSSD->lpSystemServiceTableParameterTable = 0;
      lpSSD->dwFirstServiceIndex = 0;
      lpSSD->dwSystemServiceTableNumEntries = 0;
      
      _lpSSD->lpSystemServiceTableAddressTable = 0;
      _lpSSD->lpSystemServiceTableParameterTable = 0;
      _lpSSD->dwFirstServiceIndex = 0;
      _lpSSD->dwSystemServiceTableNumEntries = 0;
      
      #ifndef TRUE_COVERAGE_EXIST
      ExFreePool((PVOID)dwAddress);
      #endif
      
      KeLeaveCriticalRegion();
    }
  }

  mySSTAT[0] = ( void * ) &ZwGetVersion;
  mySSTPT[0] = 0x01 * 0x04;
  mySSTAT[1] = ( void * ) &ZwGetLocalBase;
  mySSTPT[1] = 0x01 * 0x04;
  mySSTAT[2] = ( void * ) &ZwDispatchFunction;
  mySSTPT[2] = 0x08 * 0x04;
  mySSTAT[3] = ( void * ) &ZwGetInvisibleDriverUnloadAddr;
  mySSTPT[3] = 0x01 * 0x04;
  mySSTAT[4] = ( void * ) &ZwGetCallbackTableAddress;
  mySSTPT[4] = 0x01 * 0x04;

  if (!(lpMySSTAT = (LPSSTAT)ExAllocatePoolWithTag(PagedPool/*1*/, HE4_NATIVE_API_COUNT * 0x04,(DWORD)'Ddk')))
  {
    DestroyKHeaps();
    DbgPrintMain(("He4HookInv: _InvisibleDriverEntry: Driver init Error on the ExAllocatePoolWithTag!!!\n"));
    return STATUS_UNSUCCESSFUL;
  }

  if (!(lpMySSTPT = (LPSSTPT)ExAllocatePoolWithTag(PagedPool/*1*/, HE4_NATIVE_API_COUNT * 0x04,(DWORD)'Ddk ')))
  {
    ExFreePool(lpMySSTAT);
    DestroyKHeaps();
    DbgPrintMain(("He4HookInv: _InvisibleDriverEntry: Driver init Error on the ExAllocatePoolWithTag!!!\n"));
    return STATUS_UNSUCCESSFUL;
  }

  if (
      !(memcpy(lpMySSTAT, &mySSTAT, HE4_NATIVE_API_COUNT * 0x04)) ||
      !(memcpy(lpMySSTPT, &mySSTPT, HE4_NATIVE_API_COUNT))
     )
  {
    ExFreePool(lpMySSTPT);
    ExFreePool(lpMySSTAT);
    DestroyKHeaps();
    DbgPrintMain(("He4HookInv: _InvisibleDriverEntry: Driver init Error on the memcpy!!!\n"));
    return STATUS_UNSUCCESSFUL;
  }

  if (!(KeAddSystemServiceTable(lpMySSTAT, FALSE, HE4_NATIVE_API_COUNT, lpMySSTPT, KE_SERVICE_TABLE_INDEX)))
  {
    if (!bAlreadyExist || (_lpSSDT == NULL))
    {
      ExFreePool(lpMySSTPT);
      ExFreePool(lpMySSTAT);
      DestroyKHeaps();
      DbgPrintMain(("He4HookInv: _InvisibleDriverEntry: Driver init Error on the KeAddSystemServiceTable!!!\n"));
      return STATUS_UNSUCCESSFUL;
    }
    KeEnterCriticalRegion();
    lpSSD->lpSystemServiceTableAddressTable = lpMySSTAT;
    lpSSD->lpSystemServiceTableParameterTable = lpMySSTPT;
    lpSSD->dwFirstServiceIndex = 0;
    lpSSD->dwSystemServiceTableNumEntries = HE4_NATIVE_API_COUNT;
    _lpSSD->lpSystemServiceTableAddressTable = lpMySSTAT;
    _lpSSD->lpSystemServiceTableParameterTable = lpMySSTPT;
    _lpSSD->dwFirstServiceIndex = 0;
    _lpSSD->dwSystemServiceTableNumEntries = HE4_NATIVE_API_COUNT;
    KeLeaveCriticalRegion();
  }


  #ifdef TRUE_COVERAGE_EXIST
  HookFileSystem();
  DosPathNameToNtPathName(L"\\DosDevices\\i:\\Util\\NU\\ndd.exe", NtPathName, sizeof(NtPathName), 0);
  #endif //TRUE_COVERAGE_EXIST

  #ifdef __TDI_SERVER
  pServerWorkItem1 = OpenServer();
//  pServerWorkItem2 = OpenServer();
  #endif //__TDI_SERVER

  if (pCallbackTableAddress == 0)
  {
    bAlreadyExist = FALSE;
    pCallbackTableAddress = ExAllocatePool(NonPagedPool, CALLBACK_TABLE_SIZE);
  }

  SetNotifyRoutines(CREATE_THREAD_NOTIFY, (BOOLEAN) !bAlreadyExist);
  SetNotifyRoutines(CREATE_PROCESS_NOTIFY, (BOOLEAN) !bAlreadyExist);

  DbgPrintMain(("He4HookInv: _InvisibleDriverEntry: Driver init is OK!!!\n"));
  return STATUS_SUCCESS;
}

//******************************************************************//
//*********** Export function __InvisibleDriverUnload@0 ************//
//******************************************************************//
extern "C"
BOOLEAN _InvisibleDriverUnload(VOID)
{
  BOOLEAN bRes = FALSE;
  BOOLEAN bResDO;
  BOOLEAN bResFS;
  BOOLEAN bResNR;
  BOOLEAN bResServer1 = TRUE;
  BOOLEAN bResServer2 = TRUE;

  DbgPrintMain(("He4HookInv: _InvisibleDriverUnload: Driver unload is START!!!\n"));

  bResNR = ResetNotifyRoutines(CREATE_THREAD_NOTIFY);
  bResNR = (bResNR && ResetNotifyRoutines(CREATE_PROCESS_NOTIFY));

  #ifdef __TDI_SERVER
//  bResServer2 = CloseServer(pServerWorkItem2);
//  if (bResServer2 == TRUE)
//    pServerWorkItem2 = NULL;
  bResServer1 = CloseServer(pServerWorkItem1);
  if (bResServer1 == TRUE)
    pServerWorkItem1 = NULL;
  #endif //__TDI_SERVER

  #ifdef HOOK_DRIVER_OBJECT
  bResDO = UnHookDriverObjects();
  #endif //HOOK_DRIVER_OBJECT

  bResFS = UnhookFileSystem();

  bRes = bResDO && bResFS && bResNR && bResServer1 && bResServer2;

  if (bRes == TRUE)
  {
// Remove unlock list !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ClearUnlockList();
    DbgPrintMain(("He4HookInv: _InvisibleDriverUnload: ClearUnlockList is OK!!!\n"));

// Remove save file list !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ClearSaveFileList();
    DbgPrintMain(("He4HookInv: _InvisibleDriverUnload: ClearSaveFileList is OK!!!\n"));

    DestroyKHeaps();
  }

  DbgPrintMain(("He4HookInv: _InvisibleDriverUnload: Driver unload is OK!!!\n"));

  DbgPrint("He4HookInv: _InvisibleDriverUnload: Driver unload is %s!!!\n", bRes == TRUE ? "OK" : "ERROR");

  return bRes;
}

NTSTATUS _ZwGetLocalBase(OUT ULONG* lpdwBase)
{
  void **lpParameterStack = (void **) &lpdwBase;
  DWORD  dwRet = -1;
  DWORD  dwSericeId = (KE_SERVICE_TABLE_INDEX<<12) + HE4_SERVICE_INDEX_GET_LOCAL_BASE;

  __asm
  {
    mov  eax, [dwSericeId]     
    mov  edx, lpParameterStack
    int  2eh
    mov  [dwRet], eax
  }

  return dwRet;
}

NTSTATUS _ZwGetInvisibleDriverUnloadAddr(OUT ULONG* lpdwAddress)
{
  void **lpParameterStack = (void **) &lpdwAddress;
  DWORD  dwRet = -1;
  DWORD  dwSericeId = (KE_SERVICE_TABLE_INDEX<<12) + HE4_SERVICE_INDEX_GET_UNLOAD_ADDRESS;

  __asm
  {
    mov  eax, [dwSericeId]     
    mov  edx, lpParameterStack
    int  2eh
    mov  [dwRet], eax
  }

  return dwRet;
}

NTSTATUS _ZwGetCallbackTableAddress(OUT ULONG* pdwCallbackTableAddress)
{
  void **lpParameterStack = (void **) &pdwCallbackTableAddress;
  DWORD  dwRet = -1;
  DWORD  dwSericeId = (KE_SERVICE_TABLE_INDEX<<12) + HE4_SERVICE_INDEX_GET_CB_TABLE_ADDRESS;

  __asm
  {
    mov  eax, [dwSericeId]     
    mov  edx, lpParameterStack
    int  2eh
    mov  [dwRet], eax
  }

  return dwRet;
}
//******************************************************************//
//******************* NCI service functions ************************//
//******************************************************************//
NTSTATUS ZwGetVersion(OUT ULONG* lpdwVersion)
{
  DbgPrintMain(("He4HookInv: ZwGetVersion start !!!\n"));

  if (KeGetPreviousMode() != UserMode)
  {
    if (!_MmIsAddressValid(lpdwVersion))
      return STATUS_INVALID_PARAMETER;
  }
  else
  {
    if (IsBadWritePtr(lpdwVersion, sizeof(ULONG), sizeof(UCHAR)))  
      return STATUS_INVALID_PARAMETER;
  }

  *lpdwVersion = HE4_HOOK_INV_VERSION;
  return STATUS_SUCCESS;
}

NTSTATUS ZwGetLocalBase(OUT ULONG* lpdwAddress)
{
  DbgPrintMain(("He4HookInv: ZwGetLocalBase start !!!\n"));

  if (KeGetPreviousMode() != UserMode)
  {
    if (!_MmIsAddressValid(lpdwAddress))
      return STATUS_INVALID_PARAMETER;
  }
  else
  {
    if (IsBadWritePtr(lpdwAddress, sizeof(ULONG), sizeof(UCHAR)))  
      return STATUS_INVALID_PARAMETER;
  }

  *lpdwAddress = LocateBase((void *)ZwGetLocalBase);
  return STATUS_SUCCESS;
}

NTSTATUS ZwDispatchFunction(IN ULONG dwProcessId, IN ULONG dwThreadId, IN ULONG IoControlCode,
                            IN PVOID InputBuffer, IN ULONG InputBufferLength,
                            OUT PVOID OutputBuffer, IN ULONG OutputBufferLength, 
                            OUT ULONG *lpBytesReturned)
{
  PKEYINFOSET             lpKeyInfoSet;
  PFILEINFOSET            lpFileSaveList;
  PHE4_UNLOCK_SETTING     lpUnLockSetting;
  PHE4_LOCK_SETTING       lpLockSetting;
  PVOID                   lpEClient;
  PUNLOCK_CLIENT_INFO_SET lpUnlockList;

  DbgPrintMain(("He4HookInv: ZwDispatchFunction start !!!\n"));

  if (IsBadWritePtr(lpBytesReturned, sizeof(ULONG), sizeof(UCHAR)))  
    return STATUS_INVALID_PARAMETER;

  *lpBytesReturned = 0;
  switch(IoControlCode)
  {
    case HE4_DEVICE_VERSION:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_DEVICE_VERSION\n"));
         if (
             OutputBufferLength < sizeof(ULONG) ||
             !_MmIsAddressValid(OutputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_DEVICE_VERSION\n"));
           return STATUS_INVALID_PARAMETER;
         }

         if (!IsBadWritePtr(OutputBuffer, sizeof(ULONG), sizeof(UCHAR)))  
           *(ULONG *)OutputBuffer = HE4_HOOK_INV_VERSION;
         *lpBytesReturned = TRUE;
         break;

    case HE4_HOOK_FILE_SYSTEM:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_SET_DRIVES\n"));

         if (
             InputBufferLength != sizeof(ULONG) ||
             OutputBufferLength < sizeof(ULONG) ||
             !_MmIsAddressValid(InputBuffer)    ||
             !_MmIsAddressValid(OutputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_SET_DRIVES\n"));
           return STATUS_INVALID_PARAMETER;
         }

         if (*(ULONG *)InputBuffer != 0)
         {
           *lpBytesReturned = TRUE;
           switch (*(ULONG *)InputBuffer)
           {
             case 1:
                  {
                  if (dwDeviceState & HE4_STATE_HOOK_DRIVER_OBJECT)
                    UnHookDriverObjects();
                  HookFileSystem();
                  }
                  break;
             case 2:
                  {
                  if (dwDeviceState & HE4_STATE_HOOK_FILE_SYSTEM)
                    UnhookFileSystem();
                  HookDriverObjects();
                  }
                  break;
             default:
                  *lpBytesReturned = 0;
                  break;
           }
         }
         else
         {
           if (dwDeviceState & HE4_STATE_HOOK_DRIVER_OBJECT)
           {
             *lpBytesReturned = UnHookDriverObjects();
           }
           else
           {
             if (dwDeviceState & HE4_STATE_HOOK_FILE_SYSTEM)
             {
               *lpBytesReturned = UnhookFileSystem();
             }
             else
             {
               *lpBytesReturned = TRUE;
             }
           } 
         }

         if(!IsBadWritePtr(OutputBuffer, sizeof(ULONG), sizeof(UCHAR)))  
            *(ULONG *)OutputBuffer = *(ULONG *)InputBuffer;
         break;

    case HE4_ADD_TO_SAVE_LIST:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_ADD_TO_SAVE_LIST\n"));

         if (
             InputBufferLength < SIZEOF_FILEINFOSET ||
             !_MmIsAddressValid(InputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_ADD_TO_SAVE_LIST\n"));
           return STATUS_INVALID_PARAMETER;
         }
         lpFileSaveList = (PFILEINFOSET)InputBuffer;
         if (AddFileSaveList(lpFileSaveList))
           *lpBytesReturned = TRUE;

         break;

    case HE4_DEL_FROM_SAVE_LIST:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_DEL_TO_SAVE_LIST\n"));

         if (
             InputBufferLength < sizeof(SIZEOF_FILEINFOSET) ||
             !_MmIsAddressValid(InputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_DEL_TO_SAVE_LIST\n"));
           return STATUS_INVALID_PARAMETER;
         }
         lpFileSaveList = (PFILEINFOSET)InputBuffer;
         if (DelFileSaveList(lpFileSaveList))
           *lpBytesReturned = TRUE;

         break;

    case HE4_CLEAR_SAVE_LIST:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_CLEAR_SAVE_LIST\n"));

         ClearSaveFileList();
         *lpBytesReturned = TRUE;

         break;

    case HE4_GET_SIZE_BY_BYTES_SAVE_LIST:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_GET_SIZE_BY_BYTES_SAVE_LIST\n"));

         if (
             OutputBufferLength < sizeof(ULONG) ||
             !_MmIsAddressValid(OutputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_GET_SIZE_BY_BYTES_SAVE_LIST\n"));
           return STATUS_INVALID_PARAMETER;
         }

         if (!IsBadWritePtr(OutputBuffer, sizeof(ULONG), sizeof(UCHAR)))  
         {
           if (GetFileListSizeByBytes((ULONG *)OutputBuffer) == TRUE)
             *lpBytesReturned = TRUE;
         }

         break;

    case HE4_GET_SAVE_LIST:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_GET_SAVE_LIST\n"));

         if (
             OutputBufferLength < SIZEOF_FILEINFOSET ||
             !_MmIsAddressValid(OutputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_GET_SAVE_LIST\n"));
           return STATUS_INVALID_PARAMETER;
         }
         lpFileSaveList = (PFILEINFOSET)OutputBuffer;

         if (!IsBadWritePtr(OutputBuffer, OutputBufferLength, sizeof(UCHAR)))  
         {
           if (GetFileSaveList(lpFileSaveList))
             *lpBytesReturned = TRUE;
         }

         break;

    case HE4_GET_SIZE_BY_BYTES_UNLOCK_LIST:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_GET_SIZE_BY_BYTES_UNLOCK_LIST\n"));

         if (
             OutputBufferLength < sizeof(ULONG) ||
             !_MmIsAddressValid(OutputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_GET_SIZE_BY_BYTES_UNLOCK_LIST\n"));
           return STATUS_INVALID_PARAMETER;
         }

         if (!IsBadWritePtr(OutputBuffer, sizeof(ULONG), sizeof(UCHAR)))  
         {
           if (GetUnlockListSizeByBytes((ULONG *)OutputBuffer) == TRUE)
             *lpBytesReturned = TRUE;
         }

         break;

    case HE4_GET_UNLOCK_LIST:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_GET_UNLOCK_LIST\n"));

         if (
             OutputBufferLength < SIZEOF_UNLOCK_CLIENT_INFO_SET ||
             !_MmIsAddressValid(OutputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_GET_UNLOCK_LIST\n"));
           return STATUS_INVALID_PARAMETER;
         }
         lpUnlockList = (PUNLOCK_CLIENT_INFO_SET)OutputBuffer;

         if (!IsBadWritePtr(OutputBuffer, OutputBufferLength, sizeof(UCHAR)))  
         {
           if (GetUnlockList(lpUnlockList))
             *lpBytesReturned = TRUE;
         }

         break;

    case HE4_LOCK_SAVE_FILES_FOR_ALL_THREADS:

         ClearUnlockList();
         *lpBytesReturned = TRUE;

         break;

    case HE4_LOCK_SAVE_FILES:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_LOCK_SAVE_FILES Thread - %08X\n", PsGetCurrentThread()));

         if (
             InputBufferLength != sizeof(HE4_LOCK_SETTING) ||
             !_MmIsAddressValid(InputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_UNLOCK_SAVE_FILES\n"));
           return STATUS_INVALID_PARAMETER;
         }

         lpLockSetting = (PHE4_LOCK_SETTING)InputBuffer;
         if (lpLockSetting->m_dwClientId == HE4_UNLOCK_CURRENT_CLIENT)
         {
           if (lpLockSetting->m_dwForProcess == 0)
             lpEClient = PsGetCurrentThread();
           else
             lpEClient = PsGetCurrentProcess();
         }
         else
         {
           if (lpLockSetting->m_dwForProcess == 0)
           {
             if (!NT_SUCCESS(PsLookupThreadByThreadId(lpLockSetting->m_dwClientId, (PETHREAD*) &lpEClient)))
             {
               return STATUS_INVALID_PARAMETER;
             }
             ObDereferenceObject(lpEClient);
           }
           else
           {
             if (!NT_SUCCESS(PsLookupProcessByProcessId(lpLockSetting->m_dwClientId, (PEPROCESS*) &lpEClient)))
             {
               return STATUS_INVALID_PARAMETER;
             }
             ObDereferenceObject(lpEClient);
           }
         }

         if (DelUnlockList(lpLockSetting->m_dwClientId, lpEClient))
           *lpBytesReturned = TRUE;

         break;

    case HE4_UNLOCK_SAVE_FILES:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_UNLOCK_SAVE_FILES Thread - %08X\n", PsGetCurrentThread()));
         DbgPrintMain (("   InputBufferLength = %08X  InputBuffer = %08X\n", InputBufferLength, InputBuffer));

         if (
             InputBufferLength != sizeof(HE4_UNLOCK_SETTING) ||
             !_MmIsAddressValid(InputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_UNLOCK_SAVE_FILES\n"));
           return STATUS_INVALID_PARAMETER;
         }

         lpUnLockSetting = (PHE4_UNLOCK_SETTING)InputBuffer;
         if (lpUnLockSetting->m_dwClientId == HE4_UNLOCK_CURRENT_CLIENT)
         {
           if (!(lpUnLockSetting->m_dwUnlockFlags & HE4_UNLOCK_FOR_PROCESS))
             lpEClient = PsGetCurrentThread();
           else
             lpEClient = PsGetCurrentProcess();
         }
         else
         {
           if (!(lpUnLockSetting->m_dwUnlockFlags & HE4_UNLOCK_FOR_PROCESS))
           {
             if (!NT_SUCCESS(PsLookupThreadByThreadId(lpUnLockSetting->m_dwClientId, (PETHREAD*) &lpEClient)))
             {
               return STATUS_INVALID_PARAMETER;
             }
             ObDereferenceObject(lpEClient);
           }
           else
           {
             if (!NT_SUCCESS(PsLookupProcessByProcessId(lpUnLockSetting->m_dwClientId, (PEPROCESS*) &lpEClient)))
             {
               return STATUS_INVALID_PARAMETER;
             }
             ObDereferenceObject(lpEClient);
           }
         }

         if (AddUnlockList(lpUnLockSetting->m_dwClientId, lpEClient, lpUnLockSetting->m_dwUnlockFlags))
           *lpBytesReturned = TRUE;
         
         break;

    case HE4_QUERY_STATISTIC:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_QUERY_STATISTIC \n"));

         if (
             OutputBufferLength < sizeof(HE4_STATISTIC_INFO) ||
             !_MmIsAddressValid(OutputBuffer)
            )
         {
           DbgPrintMain (("He4HookInv: ZwDispatchFunction: Error HE4_QUERY_STATISTIC\n"));
           return STATUS_INVALID_PARAMETER;
         }

         if (He4QueryStatistic((PHE4_STATISTIC_INFO)OutputBuffer) == TRUE)
           *lpBytesReturned = TRUE;

         break;

    case HE4_HOOK_REGISTRY:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_HOOK_REGISTRY \n"));
         return STATUS_INVALID_DEVICE_REQUEST;
         break;

    case HE4_UNHOOK_REGISTRY:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_UNHOOK_REGISTRY \n"));
         return STATUS_INVALID_DEVICE_REQUEST;
         break;

    case HE4_ADD_KEYS_TO_SAVE_LIST:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_ADD_KEYS_TO_SAVE_LIST \n"));
         return STATUS_INVALID_DEVICE_REQUEST;
         break;
    
    case HE4_DEL_KEYS_FROM_SAVE_LIST:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: HE4_DEL_KEYS_FROM_SAVE_LIST \n"));
         return STATUS_INVALID_DEVICE_REQUEST;
         break;

    default:
         DbgPrintMain (("He4HookInv: ZwDispatchFunction: unknown function IRP_MJ_DEVICE_CONTROL\n"));
         return STATUS_INVALID_DEVICE_REQUEST;
         break;
  }
  return STATUS_SUCCESS;
}

NTSTATUS ZwGetInvisibleDriverUnloadAddr(OUT ULONG* lpdwAddress)
{
  DbgPrintMain(("He4HookInv: ZwGetInvisibleDriverUnloadAddr start !!!\n"));

  if (KeGetPreviousMode() != UserMode)
  {
    if (!_MmIsAddressValid(lpdwAddress))
      return STATUS_INVALID_PARAMETER;
  }
  else
  {
    if (IsBadWritePtr(lpdwAddress, sizeof(ULONG), sizeof(UCHAR)))  
      return STATUS_INVALID_PARAMETER;
  }

  *lpdwAddress = (ULONG)_InvisibleDriverUnload;
  return STATUS_SUCCESS;
}

NTSTATUS ZwGetCallbackTableAddress(OUT ULONG* pdwCallbackTableAddress)
{
  DbgPrintMain(("He4HookInv: ZwGetCallbackTableAddress start !!!\n"));

  if (KeGetPreviousMode() != UserMode)
  {
    if (!_MmIsAddressValid(pdwCallbackTableAddress))
      return STATUS_INVALID_PARAMETER;
  }
  else
  {
    if (IsBadWritePtr(pdwCallbackTableAddress, sizeof(ULONG), sizeof(UCHAR)))  
      return STATUS_INVALID_PARAMETER;
  }

  *pdwCallbackTableAddress = (ULONG)pCallbackTableAddress;
  return STATUS_SUCCESS;
}

__declspec(naked) SetCreateThreadNotifyRoutine(void)
{
  __asm jmp dword ptr [dwCreateThreadNotifyRoutineAddress];  // mov     ax, 25ffh  ; jmp dword ptr [...]
  __asm nop   // Align
  __asm nop   //
}

__declspec(naked) SetCreateProcessNotifyRoutine(void)
{
  __asm jmp dword ptr [dwCreateProcessNotifyRoutineAddress];  // mov     ax, 25ffh  ; jmp dword ptr [...]
  __asm nop   // Align
  __asm nop   //
}

BOOLEAN SetNotifyRoutines(ULONG dwNotifyIndex, BOOLEAN bFirst)
{
  BOOLEAN bRes = FALSE;
  PVOID   pCallbackItem;

  if (_MmIsAddressValid(pCallbackTableAddress) == TRUE)
  {
    __asm cli
    pCallbackItem = (CHAR*)pCallbackTableAddress + dwNotifyIndex * CALLBACK_TABLE_ITEM_SIZE;
    switch (dwNotifyIndex)
    {
      case CREATE_THREAD_NOTIFY:
           //KeInitializeSemaphore(&CreateThreadCallbackBusySemaphore, 0, MAXLONG);
           if (CreateThreadCallbackBusy == NULL)
           {
             CreateThreadCallbackBusy = new KInterlockedCounter(0);
             if (CreateThreadCallbackBusy != NULL)
             {
               bRes = TRUE;
               memcpy(pCallbackItem, SetCreateThreadNotifyRoutine, CALLBACK_TABLE_ITEM_SIZE);
               if (bFirst)
               {
                 if (!NT_SUCCESS(PsSetCreateThreadNotifyRoutine((PCREATE_THREAD_NOTIFY_ROUTINE)pCallbackItem)))
                 {
                   delete CreateThreadCallbackBusy;
                   CreateThreadCallbackBusy = NULL;
                   bRes = FALSE;
                 }
               } 
             }
           }
           else
           {
             bRes = TRUE;
           }
           break;
      case CREATE_PROCESS_NOTIFY:
           //KeInitializeSemaphore(&CreateProcessCallbackBusySemaphore, 0, MAXLONG);
           if (CreateProcessCallbackBusy == NULL)
           {
             CreateProcessCallbackBusy = new KInterlockedCounter(0);
             if (CreateProcessCallbackBusy != NULL)
             {
               bRes = TRUE;
               memcpy(pCallbackItem, SetCreateProcessNotifyRoutine, CALLBACK_TABLE_ITEM_SIZE);
               if (bFirst)
               {
                 if (!NT_SUCCESS(PsSetCreateProcessNotifyRoutine((PCREATE_PROCESS_NOTIFY_ROUTINE)pCallbackItem, FALSE)))
                 {
                   delete CreateProcessCallbackBusy;
                   CreateProcessCallbackBusy = NULL;
                   bRes = FALSE;
                 }
               }
             }
           }
           else
           {
             bRes = TRUE;
           }
           break;
    }
    __asm sti
  }

  return bRes;
}

__declspec(naked) ClearCreateThreadNotifyRoutine(void)
{
  __asm ret   0ch  // 3 bytes
  __asm nop   // Align
  __asm nop   // 
  __asm nop   //
  __asm nop   // 
  __asm nop   // 
}

__declspec(naked) ClearCreateProcessNotifyRoutine(void)
{
  __asm ret   0ch  // 3 bytes
  __asm nop   // Align
  __asm nop   // 
  __asm nop   //
  __asm nop   // 
  __asm nop   // 
}

BOOLEAN ResetNotifyRoutines(ULONG dwNotifyIndex)
{
  BOOLEAN bRes = FALSE;
  PVOID   pCallbackItem;

  if (_MmIsAddressValid(pCallbackTableAddress) == TRUE)
  {
    __asm cli
    pCallbackItem = (CHAR*)pCallbackTableAddress + dwNotifyIndex * CALLBACK_TABLE_ITEM_SIZE;
    switch (dwNotifyIndex)
    {
      case CREATE_THREAD_NOTIFY:
           memcpy(pCallbackItem, ClearCreateThreadNotifyRoutine, CALLBACK_TABLE_ITEM_SIZE);
           //if (KeReadStateSemaphore(&CreateThreadCallbackBusySemaphore) == 0)
           if (CreateThreadCallbackBusy != NULL)
           {
             if (CreateThreadCallbackBusy->CompareExchange(0, 0) == TRUE)
             {
               delete CreateThreadCallbackBusy;
               CreateThreadCallbackBusy = NULL;
               bRes = TRUE;
             }
           }
           else
           {
             bRes = TRUE;
           }
           break;
      case CREATE_PROCESS_NOTIFY:
           //
           // этот callback можно удалить системными средствами однако я решил этим не пользоваться (пока)
           // if (!NT_SUCCESS(PsSetCreateProcessNotifyRoutine((PCREATE_PROCESS_NOTIFY_ROUTINE)pCallbackItem), TRUE))
           //   bRes = FALSE;
           //
           memcpy(pCallbackItem, ClearCreateProcessNotifyRoutine, CALLBACK_TABLE_ITEM_SIZE);
           //if (KeReadStateSemaphore(&CreateProcessCallbackBusySemaphore) == 0)
           if (CreateProcessCallbackBusy != NULL)
           {
             if (CreateProcessCallbackBusy->CompareExchange(0, 0) == TRUE)
             {
               delete CreateProcessCallbackBusy;
               CreateProcessCallbackBusy = NULL;
               bRes = TRUE;
             }
           }
           else
           {
             bRes = TRUE;
           }
           break;
    }
    __asm sti
  }

  return bRes;
}

BOOLEAN CreateKHeaps(void)
{
  hKHeapUnlockList = NULL;
  hKHeapFSDefault = NULL;
  hKHeapSOFileList = NULL;
  #ifdef __LL_USE_KHEAP
  hKHeapLLDefault = NULL;
  #endif //__LL_USE_KHEAP
  #ifdef __MISC_USE_KHEAP
  hKHeapMiscDefault = NULL;
  #endif //__MISC_USE_KHEAP

  CreateDefaultHeap(1024*32);

  hKHeapUnlockList = KHeapCreate(1024*4);
  if (hKHeapUnlockList == NULL)
    return FALSE;

  hKHeapFSDefault = KHeapCreate(1024*32);
  if (hKHeapFSDefault == NULL)
  {
    DestroyKHeaps();
    return FALSE;
  }

  hKHeapSOFileList = KHeapCreate(1024*16);
  if (hKHeapSOFileList == NULL)
  {
    DestroyKHeaps();
    return FALSE;
  }

  #ifdef __LL_USE_KHEAP
  hKHeapLLDefault = KHeapCreate(1024*8);
  if (hKHeapLLDefault == NULL)
  {
    DestroyKHeaps();
    return FALSE;
  }
  #endif //__LL_USE_KHEAP

  #ifdef __MISC_USE_KHEAP
  hKHeapMiscDefault = KHeapCreate(1024*32);
  if (hKHeapMiscDefault == NULL)
  {
    DestroyKHeaps();
    return FALSE;
  }
  #endif //__MISC_USE_KHEAP

//  #ifdef __BTREE_USE_KHEAP
//  hKHeapBTreeDefault = KHeapCreate(1024*16);
//  if (hKHeapBTreeDefault == NULL)
//  {
//    DestroyKHeaps();
//    return FALSE;
//  }
//  #endif //__BTREE_USE_KHEAP

  #ifdef HOOK_DRIVER_OBJECT
  hKHeapDHDefault = KHeapCreate(1024*16);
  if (hKHeapDHDefault == NULL)
  {
    DestroyKHeaps();
    return FALSE;
  }
  #endif //HOOK_DRIVER_OBJECT

  return TRUE;
}

VOID DestroyKHeaps(void)
{
  if (hKHeapUnlockList != NULL)
    KHeapDestroy(hKHeapUnlockList);
  hKHeapUnlockList = NULL;

  if (hKHeapFSDefault != NULL)
    KHeapDestroy(hKHeapFSDefault);
  hKHeapFSDefault = NULL;

  if (hKHeapSOFileList != NULL)
    KHeapDestroy(hKHeapSOFileList);
  hKHeapSOFileList = NULL;

  #ifdef __LL_USE_KHEAP
  if (hKHeapLLDefault != NULL)
    KHeapDestroy(hKHeapLLDefault);
  hKHeapLLDefault = NULL;
  #endif //__LL_USE_KHEAP

  #ifdef __MISC_USE_KHEAP
  if (hKHeapMiscDefault != NULL)
    KHeapDestroy(hKHeapMiscDefault);
  hKHeapMiscDefault = NULL;
  #endif //__MISC_USE_KHEAP

//  #ifdef __BTREE_USE_KHEAP
//  if (hKHeapBTreeDefault != NULL)
//    KHeapDestroy(hKHeapBTreeDefault);
//  hKHeapBTreeDefault = NULL;
//  #endif //__BTREE_USE_KHEAP

  #ifdef HOOK_DRIVER_OBJECT
  if (hKHeapDHDefault != NULL)
    KHeapDestroy(hKHeapDHDefault);
  hKHeapDHDefault = NULL;
  #endif //HOOK_DRIVER_OBJECT

  DestroyDefaultHeap();
}

BOOLEAN He4QueryStatistic(PHE4_STATISTIC_INFO pStatInfo)
{
  BOOLEAN        bRes = FALSE;
  PHEAP_INFO_SET pHeapInfoSet;

  if (_MmIsAddressValid(pStatInfo))
  {
    pHeapInfoSet = &(pStatInfo->m_HeapInfoSet);

    pHeapInfoSet->m_DefaultHeapInfo.m_dwSystemMemoryUsage = KHeapGetSizeSystemMemory(KGetDefaultHeap());
    pHeapInfoSet->m_DefaultHeapInfo.m_dwHeapMemoryUsage = KHeapGetSizeUsageMemory(KGetDefaultHeap());

    pHeapInfoSet->m_UnlockListHeapInfo.m_dwSystemMemoryUsage = KHeapGetSizeSystemMemory(hKHeapUnlockList);
    pHeapInfoSet->m_UnlockListHeapInfo.m_dwHeapMemoryUsage = KHeapGetSizeUsageMemory(hKHeapUnlockList);

    pHeapInfoSet->m_FSDefaultHeapInfo.m_dwSystemMemoryUsage = KHeapGetSizeSystemMemory(hKHeapFSDefault);
    pHeapInfoSet->m_FSDefaultHeapInfo.m_dwHeapMemoryUsage = KHeapGetSizeUsageMemory(hKHeapFSDefault);

    pHeapInfoSet->m_SOFileListHeapInfo.m_dwSystemMemoryUsage = KHeapGetSizeSystemMemory(hKHeapSOFileList);
    pHeapInfoSet->m_SOFileListHeapInfo.m_dwHeapMemoryUsage = KHeapGetSizeUsageMemory(hKHeapSOFileList);

    #ifdef __LL_USE_KHEAP
    pHeapInfoSet->m_LLDefaultHeapInfo.m_dwSystemMemoryUsage = KHeapGetSizeSystemMemory(hKHeapLLDefault);
    pHeapInfoSet->m_LLDefaultHeapInfo.m_dwHeapMemoryUsage = KHeapGetSizeUsageMemory(hKHeapLLDefault);
    #else
    pHeapInfoSet->m_LLDefaultHeapInfo.m_dwSystemMemoryUsage = 0;
    pHeapInfoSet->m_LLDefaultHeapInfo.m_dwHeapMemoryUsage = 0;
    #endif //__LL_USE_KHEAP

    #ifdef __MISC_USE_KHEAP
    pHeapInfoSet->m_MiscDefaultHeapInfo.m_dwSystemMemoryUsage = KHeapGetSizeSystemMemory(hKHeapMiscDefault);
    pHeapInfoSet->m_MiscDefaultHeapInfo.m_dwHeapMemoryUsage = KHeapGetSizeUsageMemory(hKHeapMiscDefault);
    #else
    pHeapInfoSet->m_MiscDefaultHeapInfo.m_dwSystemMemoryUsage = 0;
    pHeapInfoSet->m_MiscDefaultHeapInfo.m_dwHeapMemoryUsage = 0;
    #endif //__MISC_USE_KHEAP

    #ifdef HOOK_DRIVER_OBJECT
    pHeapInfoSet->m_DHDefaultHeapInfo.m_dwSystemMemoryUsage = KHeapGetSizeSystemMemory(hKHeapDHDefault);
    pHeapInfoSet->m_DHDefaultHeapInfo.m_dwHeapMemoryUsage = KHeapGetSizeUsageMemory(hKHeapDHDefault);
    #else
    pHeapInfoSet->m_DHDefaultHeapInfo.m_dwSystemMemoryUsage = 0;
    pHeapInfoSet->m_DHDefaultHeapInfo.m_dwHeapMemoryUsage = 0;
    #endif //HOOK_DRIVER_OBJECT

//    #ifdef __BTREE_USE_KHEAP
//    pHeapInfoSet->m_BTreeDefaultHeapInfo.m_dwSystemMemoryUsage = KHeapGetSizeSystemMemory(hKHeapBTreeDefault);
//    pHeapInfoSet->m_BTreeDefaultHeapInfo.m_dwHeapMemoryUsage = KHeapGetSizeUsageMemory(hKHeapBTreeDefault);
//    #else
//    pHeapInfoSet->m_BTreeDefaultHeapInfo.m_dwSystemMemoryUsage = 0;
//    pHeapInfoSet->m_BTreeDefaultHeapInfo.m_dwHeapMemoryUsage = 0;
//    #endif //__BTREE_USE_KHEAP

    bRes = TRUE;
  }

  return bRes;
}


#ifdef __TDI_SERVER
PSERVER_WI OpenServer()
{
  PSERVER_WI pServerWorkItem = (PSERVER_WI)_AllocatePoolFromKHeap(NULL, sizeof(SERVER_WI));
  if (pServerWorkItem != NULL)
  {
    KeInitializeEvent(&pServerWorkItem->m_kDestroyEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&pServerWorkItem->m_kExitEvent, NotificationEvent, FALSE);
    pServerWorkItem->m_bClosed = FALSE;
    pServerWorkItem->m_ServerThreadObjectPointer = NULL;

    HANDLE hThread;
    NTSTATUS NtStatus = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, (PKSTART_ROUTINE) ServerThread, (PVOID) pServerWorkItem);
    if (NT_SUCCESS(NtStatus))
    {
      NtStatus = ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, NULL, KernelMode, &pServerWorkItem->m_ServerThreadObjectPointer, NULL);
      if (!NT_SUCCESS(NtStatus))
      {
        FreePoolToKHeap(NULL, pServerWorkItem);
        pServerWorkItem = NULL;
      }
      ZwClose(hThread);
    }
    //ExInitializeWorkItem(&pServerWorkItem->m_WorkItem, (PWORKER_THREAD_ROUTINE)ServerThread, (PVOID)pServerWorkItem);
    //ExQueueWorkItem(&pServerWorkItem->m_WorkItem, DelayedWorkQueue);
  }

  return pServerWorkItem;
}

BOOLEAN CloseServer(IN PSERVER_WI pServerWorkItem)
{
  BOOLEAN bRes;
  if (pServerWorkItem != NULL)
  {
    KeSetEvent(&pServerWorkItem->m_kDestroyEvent, 0, FALSE);

    //if (pServerWorkItem->m_pStreamSocket != NULL)
    //  pServerWorkItem->m_pStreamSocket->Disconnect();

    bRes = pServerWorkItem->m_bClosed;

    if (bRes == TRUE)
    {
      //KeWaitForSingleObject(&pServerWorkItem->m_kExitEvent, Executive, KernelMode, FALSE, NULL);
      KeWaitForSingleObject(pServerWorkItem->m_ServerThreadObjectPointer, Executive/*UserRequest*/, KernelMode, FALSE, NULL);
      ObDereferenceObject(pServerWorkItem->m_ServerThreadObjectPointer);
      FreePoolToKHeap(NULL, pServerWorkItem);
      pServerWorkItem = NULL;
    }
  }
  else
  {
    bRes = TRUE;
  }

  return bRes;
}

void ServerThread(IN PSERVER_WI pWorkItem)
{
  static int nListenPort = 0x2000;
  LARGE_INTEGER   Time;

  pWorkItem->m_pStreamSocket = new KTdiStreamSocket();
  KTdiStreamSocket* pStreamSocket = pWorkItem->m_pStreamSocket;
  if (pStreamSocket != NULL)
  {

    if (pStreamSocket->Open(nListenPort++))
    {
      DbgPrint ("He4HookInv: pStreamSocket->Open is OK!!!\n");
      
      if (pStreamSocket->Bind())
        DbgPrint ("He4HookInv: pStreamSocket->Bind is OK!!!\n");
      else
        DbgPrint ("He4HookInv: pStreamSocket->Bind is ERROR!!!\n");
      
//      if (pStreamSocket->Connect(41769, 0x7f000001))
//      //if (pStreamSocket->Connect(5101, 0x0a005113))
//      //if (pStreamSocket->Connect(5101, 0x7f000001))
//      {
//        DbgPrint ("He4HookInv: pStreamSocket->Connect is OK!!!\n");
//        
//        ULONG dwRes = pStreamSocket->Send("Test TDI send", sizeof("Test TDI send")-1);
//        if (dwRes == 0)
//          DbgPrint ("He4HookInv: pStreamSocket->Send is ERROR!!!\n");
//        else
//          DbgPrint ("He4HookInv: pStreamSocket->Send is OK - %u!!!\n", dwRes);
//
//        KeWaitForSingleObject(&pWorkItem->m_kDestroyEvent, Executive, KernelMode, FALSE, NULL);
//        
//        pStreamSocket->Disconnect();
//      }
//      else
//      {
//        DbgPrint ("He4HookInv: pStreamSocket->Connect is ERROR!!!\n");
//      }
      
      
      UCHAR           Byte[100];
      ULONG           dwCount;

      Time.QuadPart = 10 * 1000 * 10;
      Time.QuadPart = -Time.QuadPart;

      //if (pStreamSocket->Listen() == TRUE)
      //  DbgPrint ("He4HookInv: pStreamSocket->Listen is OK!!!\n");
      //else
      //  DbgPrint ("He4HookInv: pStreamSocket->Listen is ERROR!!!\n");

      //KeWaitForSingleObject(&pWorkItem->m_kDestroyEvent, Suspended, KernelMode, TRUE, NULL);
      
      while (
                (KeWaitForSingleObject(&pWorkItem->m_kDestroyEvent, Suspended, KernelMode, TRUE, &Time) == STATUS_TIMEOUT)
             && (pStreamSocket->Listen() == TRUE)
            )
      {
        DbgPrint ("He4HookInv: pStreamSocket->Listen is OK!!!\n");

        Time.QuadPart = 10 * 1000 * 10;
        Time.QuadPart = -Time.QuadPart;
        
        while (pStreamSocket->IsConnected() == FALSE && KeWaitForSingleObject(&pWorkItem->m_kDestroyEvent, Suspended, KernelMode, TRUE, &Time) == STATUS_TIMEOUT)
        {
          pStreamSocket->Accept(1000);
          Time.QuadPart = 10 * 1000 * 10;
          Time.QuadPart = -Time.QuadPart;
        }

        Time.QuadPart = 10 * 1000 * 10;
        Time.QuadPart = -Time.QuadPart;
        while (pStreamSocket->IsConnected() == TRUE && KeWaitForSingleObject(&pWorkItem->m_kDestroyEvent, Suspended, KernelMode, TRUE, &Time) == STATUS_TIMEOUT)
        {
          dwCount = pStreamSocket->Receive(Byte, sizeof(Byte));
          if (dwCount != 0)
            pStreamSocket->Send(Byte, dwCount);

          Time.QuadPart = 10 * 1000 * 10;
          Time.QuadPart = -Time.QuadPart;
        }

        //pStreamSocket->Disconnect();
      }
      
      //pStreamSocket->Disconnect();
      
      //else
      //{
      //  DbgPrint ("He4HookInv: pStreamSocket->Listen is ERROR!!!\n");
      //}
      
      
      //if (pStreamSocket->Unbind())
      //  DbgPrint ("He4HookInv: pStreamSocket->Unbind is OK!!!\n");
      //else
      //  DbgPrint ("He4HookInv: pStreamSocket->Unbind is ERROR!!!\n");
      
      do
      {
        pWorkItem->m_bClosed = pStreamSocket->Close();
        if (pWorkItem->m_bClosed == FALSE)
        {
          Time.QuadPart = 10 * 1000 * 10;
          Time.QuadPart = -Time.QuadPart;
          KeWaitForSingleObject(&pWorkItem->m_kExitEvent, Suspended, KernelMode, TRUE, &Time);
        }
      }
      while (pWorkItem->m_bClosed == FALSE);
    }
    else
    {
      pWorkItem->m_bClosed = TRUE;
      DbgPrint ("He4HookInv: pStreamSocket->Open is ERROR!!!\n");
    }

    delete pStreamSocket;
    pWorkItem->m_pStreamSocket = NULL;
    pWorkItem->m_bClosed = TRUE;
  }
  else
  {
    pWorkItem->m_bClosed = TRUE;
  }

  KeSetEvent(&pWorkItem->m_kExitEvent, 0, FALSE);
  PsTerminateSystemThread(STATUS_SUCCESS);
}
#endif //__TDI_SERVER