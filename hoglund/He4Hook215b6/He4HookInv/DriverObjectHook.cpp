#include "DriverObjectHook.h"
#include "../CommonClasses/KInterlockedCounter/KInterlockedCounter.h"

#define HOOK_QUERY_DIRECTORY_IRP

extern ULONG dwDeviceState;

KInterlockedCounter*  DriverObjectBusy = NULL;

BOOLEAN             bSystemShutdown = FALSE;
BOOLEAN             bDriverObjectUnhookStarted = FALSE;

KBinaryTree*        pKBTreeHookedDriver = NULL;
KBinaryTree*        pKBTreeIrps = NULL;

KMUTEX              LockMutexToHookedDriverTree;
KHEAP               hKHeapDHDefault = NULL;

PVOID               DriverHookThreadObjectPointer = NULL;


VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
  DelHookedDriverFromTree(DriverObject);

  if (_MmIsAddressValid(DriverObject))
  {
    if (_MmIsAddressValid(DriverObject->DriverUnload))
      DriverObject->DriverUnload(DriverObject);
  }
}

NTSTATUS
IntermediateIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
  PIRP_COMPLETION_INFO    pInfo = (PIRP_COMPLETION_INFO) Context;
  PIO_COMPLETION_ROUTINE  pOriginalCompletionRoutine = pInfo->m_pOriginalCompletionRoutine;
  PVOID                   pOriginalContext = pInfo->m_pOriginalContext;
  UCHAR                   OriginalControl = pInfo->m_OriginalControl;
  BOOLEAN                 bSuccess = FALSE, bError = FALSE, bCancel = FALSE;
  NTSTATUS                NtStatus;

  DbgPrint ("Start IntermediateIrpCompletion %08x !!!!\n", Irp->IoStatus.Status);

  if (Irp->IoStatus.Status == STATUS_SUCCESS)
  {
    bSuccess = TRUE;
  }
  else
  {
    if (Irp->IoStatus.Status == STATUS_CANCELLED)
    {
      bCancel = TRUE;
    }
    else
    {
      bError = TRUE;
    }
  }

  if (NT_SUCCESS(Irp->IoStatus.Status))
  {
    NtStatus = IrpCompletion(DeviceObject, Irp, pInfo);
    if (NtStatus != STATUS_SUCCESS)
      Irp->IoStatus.Status = NtStatus;
  }

  FreePoolToKHeap(hKHeapDHDefault, pInfo);

  if (
      (pOriginalCompletionRoutine == NULL) ||
      (OriginalControl == 0) ||
      ((bSuccess == TRUE) && ((OriginalControl & SL_INVOKE_ON_SUCCESS) == FALSE)) ||
      ((bError == TRUE) && ((OriginalControl & SL_INVOKE_ON_ERROR) == FALSE)) ||
      ((bCancel == TRUE) && ((OriginalControl & SL_INVOKE_ON_CANCEL) == FALSE)) 
     )
  {
    if (Irp->PendingReturned)
    {
      IoMarkIrpPending(Irp);
    }
    --DriverObjectBusy;
    return Irp->IoStatus.Status;
  }

  --DriverObjectBusy;
  return pOriginalCompletionRoutine(DeviceObject, Irp, pOriginalContext);
}

NTSTATUS
HighLevelIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
  PIRP_COMPLETION_INFO    pInfo = (PIRP_COMPLETION_INFO) Context;
  PIO_COMPLETION_ROUTINE  pOriginalCompletionRoutine = pInfo->m_pOriginalCompletionRoutine;
  PVOID                   pOriginalContext = pInfo->m_pOriginalContext;
  UCHAR                   OriginalControl = pInfo->m_OriginalControl;
  NTSTATUS                NtStatus;

  //DbgPrint ("Start HighLevelIrpCompletion %08x !!!!\n", Irp->IoStatus.Status);

  if (NT_SUCCESS(Irp->IoStatus.Status))
  {
    NtStatus = IrpCompletion(DeviceObject, Irp, pInfo);
    if (NtStatus != STATUS_SUCCESS)
      Irp->IoStatus.Status = NtStatus;
  }

  FreePoolToKHeap(hKHeapDHDefault, pInfo);

  --DriverObjectBusy;
  if (pOriginalCompletionRoutine != NULL)
    return pOriginalCompletionRoutine(DeviceObject, Irp, pOriginalContext);

  if (Irp->PendingReturned)
  {
    IoMarkIrpPending(Irp);
  }

  /*if (Irp->UserIosb != NULL)
    *Irp->UserIosb = Irp->IoStatus;

  if (Irp->MdlAddress != NULL)
  {
    MmUnlockPages(Irp->MdlAddress);
    IoFreeMdl(Irp->MdlAddress);
  }

  if (Irp->UserEvent != NULL)
    KeSetEvent(Irp->UserEvent, 0, FALSE);

  IoFreeIrp(Irp);

  return STATUS_MORE_PROCESSING_REQUIRED;
  */
  return Irp->IoStatus.Status;
}

NTSTATUS
IrpCompletion(
    IN PDEVICE_OBJECT DeviceObject, // may be NULL ????
    IN PIRP Irp,
    PIRP_COMPLETION_INFO pInfo
    )
{
  PIO_STACK_LOCATION  currentIrpStack;
  NTSTATUS            NtStatus = STATUS_SUCCESS;;
  ULONG               dwMajorFunction, dwMinorFunction;
  #ifdef HOOK_QUERY_DIRECTORY_IRP
  PQUERY_DIRECTORY    pQueryDir;
  #endif //HOOK_QUERY_DIRECTORY_IRP

  currentIrpStack = &pInfo->m_CurrentIrpStack;//IoGetNextIrpStackLocation(Irp);//IoGetCurrentIrpStackLocation(Irp);
  dwMajorFunction = currentIrpStack->MajorFunction;
  dwMinorFunction = currentIrpStack->MinorFunction;

  //if (DeviceObject != currentIrpStack->DeviceObject)
  //  DbgPrint ("Start IrpCompletion \n  (%08x != %08x)!!!!\n", DeviceObject, currentIrpStack->DeviceObject);
  //DbgPrint ("Start IrpCompletion \n  (%04x->%04x) StackCount = %u,  CurrentLocation = %u !!!!\n", dwMajorFunction, dwMinorFunction, Irp->StackCount, Irp->CurrentLocation);

  #ifdef HOOK_QUERY_DIRECTORY_IRP
  if (dwMajorFunction == IRP_MJ_DIRECTORY_CONTROL)
  {
//    DbgPrint ("Start IRP_MJ_DIRECTORY_CONTROL !!!!\n");

    if (dwMinorFunction == IRP_MN_QUERY_DIRECTORY && KeGetCurrentIrql() <= DISPATCH_LEVEL)
    {
//      DbgPrint ("Start IRP_MN_QUERY_DIRECTORY !!!!\n");

      PTREAT_IRP_INFO pTreatIrpInfo = (PTREAT_IRP_INFO)_AllocatePoolFromKHeap(hKHeapDHDefault, sizeof(TREAT_IRP_INFO));
      if (pTreatIrpInfo != NULL)
      {
        PMDL                  pMdl, pMdl_SB, pMdl_UNI;

        pQueryDir = (PQUERY_DIRECTORY)&currentIrpStack->Parameters;

        pInfo->m_QueryDirectoryFileParam.m_pDeviceObject = currentIrpStack->DeviceObject;
        pInfo->m_QueryDirectoryFileParam.m_pFileObject = currentIrpStack->FileObject;
        pInfo->m_QueryDirectoryFileParam.m_pIoStatusBlock = &Irp->IoStatus;

        if (pQueryDir->FileName != NULL)
        {
          memcpy(&pInfo->m_QueryDirectoryFileParam.m_SearchTemplate, pQueryDir->FileName, sizeof(UNICODE_STRING));
        }
        else
        {
          memset(&pInfo->m_QueryDirectoryFileParam.m_SearchTemplate, 0, sizeof(UNICODE_STRING));
        }

        if (Irp->MdlAddress)
          pInfo->m_QueryDirectoryFileParam.m_Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
        else
          pInfo->m_QueryDirectoryFileParam.m_Buffer = Irp->UserBuffer;

        pInfo->m_QueryDirectoryFileParam.m_BufferLength = pQueryDir->Length;
        pInfo->m_QueryDirectoryFileParam.m_DirectoryInfoClass = pQueryDir->FileInformationClass;
        pInfo->m_QueryDirectoryFileParam.m_ByOne = (currentIrpStack->Flags & SL_RETURN_SINGLE_ENTRY) != 0;
        pInfo->m_QueryDirectoryFileParam.m_Reset = (currentIrpStack->Flags & SL_RESTART_SCAN) != 0;
        pInfo->m_QueryDirectoryFileParam.m_Index = (currentIrpStack->Flags & SL_INDEX_SPECIFIED) != 0;
        pInfo->m_QueryDirectoryFileParam.m_dwIndex = pQueryDir->FileIndex;

        if (Irp->MdlAddress == NULL && _MmIsAddressValid(pInfo->m_QueryDirectoryFileParam.m_Buffer))
//        if (_MmIsAddressValid(pQueryDirParam->m_Buffer))
          pInfo->m_QueryDirectoryFileParam.m_Buffer = MapUserAddressToKernel(pInfo->m_QueryDirectoryFileParam.m_Buffer, pInfo->m_QueryDirectoryFileParam.m_BufferLength, &pMdl);
        if (_MmIsAddressValid(pInfo->m_QueryDirectoryFileParam.m_pIoStatusBlock))
          pInfo->m_QueryDirectoryFileParam.m_pIoStatusBlock = (IO_STATUS_BLOCK*)MapUserAddressToKernel(pInfo->m_QueryDirectoryFileParam.m_pIoStatusBlock, sizeof(IO_STATUS_BLOCK), &pMdl_SB);
        if (_MmIsAddressValid(pInfo->m_QueryDirectoryFileParam.m_SearchTemplate.Buffer))
        {   
          pInfo->m_QueryDirectoryFileParam.m_SearchTemplate.Buffer = (PWSTR)MapUserAddressToKernel(pInfo->m_QueryDirectoryFileParam.m_SearchTemplate.Buffer, pInfo->m_QueryDirectoryFileParam.m_SearchTemplate.MaximumLength, &pMdl_UNI);
        }

        KeInitializeEvent(&pTreatIrpInfo->m_Event, NotificationEvent, FALSE);

        pTreatIrpInfo->m_pCurrentThread = PsGetCurrentThread();
        pTreatIrpInfo->m_pCurrentProcess = PsGetCurrentProcess();
        pTreatIrpInfo->m_CurrentIrpStack = pInfo->m_CurrentIrpStack;//NULL;
        pTreatIrpInfo->m_pQueryDirParam = &pInfo->m_QueryDirectoryFileParam;
        pTreatIrpInfo->NtStatus = STATUS_SUCCESS;

        #ifndef __WIN2K
        ++DriverObjectBusy;

        ExInitializeWorkItem(&pTreatIrpInfo->m_WorkItem, (PWORKER_THREAD_ROUTINE)TreatmentIrpThread, pTreatIrpInfo);
        ExQueueWorkItem(&pTreatIrpInfo->m_WorkItem, DelayedWorkQueue);
        KeWaitForSingleObject(&pTreatIrpInfo->m_Event, Executive, KernelMode, FALSE, NULL);
        NtStatus = pTreatIrpInfo->NtStatus;
        #else
        pTreatIrpInfo->m_pWorkItem = IoAllocateWorkItem(/*DeviceObject*/currentIrpStack->DeviceObject);
        if (pTreatIrpInfo->m_pWorkItem != NULL)
        {
          ++DriverObjectBusy;

          IoQueueWorkItem(pTreatIrpInfo->m_pWorkItem, (PIO_WORKITEM_ROUTINE)TreatmentIrpThread, DelayedWorkQueue/*CriticalWorkQueue*/, pTreatIrpInfo);
          KeWaitForSingleObject(&pTreatIrpInfo->m_Event, Executive, KernelMode, FALSE, NULL);
          //IoFreeWorkItem(pTreatIrpInfo->m_pWorkItem);
          NtStatus = pTreatIrpInfo->NtStatus;
        }
        #endif //__WIN2K
        
//        if (Irp->MdlAddress == NULL && pInfo->m_QueryDirectoryFileParam.m_Buffer != NULL)
        if (Irp->MdlAddress == NULL && _MmIsAddressValid(pInfo->m_QueryDirectoryFileParam.m_Buffer))
          UnmapMappedKernelAddress(pMdl);
        if (_MmIsAddressValid(pInfo->m_QueryDirectoryFileParam.m_pIoStatusBlock))
          UnmapMappedKernelAddress(pMdl_SB);
        if (_MmIsAddressValid(pInfo->m_QueryDirectoryFileParam.m_SearchTemplate.Buffer))
          UnmapMappedKernelAddress(pMdl_UNI);
        
        FreePoolToKHeap(hKHeapDHDefault, pTreatIrpInfo);
      }
    }
  }
  #endif //HOOK_QUERY_DIRECTORY_IRP

  return NtStatus;
}

NTSTATUS DriverObjectDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  PIO_STACK_LOCATION  currentIrpStack;
  NTSTATUS            NtStatus;
  HOOK_DRIVER_INFO    DriverInfo;
  PHOOK_DRIVER_INFO   pDriverInfo;
  BOOLEAN             bInvalidRequest = TRUE;
  ULONG               dwMajorFunction, dwMinorFunction;
  BOOLEAN             bIsRightDeviceType;
  BOOLEAN             bIrpAlreadyTreat = FALSE;
  PTREAT_IRP_INFO     pTreatIrpInfo;

  ++DriverObjectBusy;

  __try
  {
    currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    dwMajorFunction = currentIrpStack->MajorFunction;
    dwMinorFunction = currentIrpStack->MinorFunction;
//    DbgPrint ("StackCount = %u,  CurrentLocation = %u \n", Irp->StackCount, Irp->CurrentLocation);
    bIrpAlreadyTreat = (pKBTreeIrps->Insert(Irp, 0) == NULL);
    bIsRightDeviceType = IsRightDeviceTypeForFunc(DeviceObject->DeviceType, dwMajorFunction);

    DriverInfo.m_pDriverObject = DeviceObject->DriverObject;

//    MUTEX_WAIT(LockMutexToHookedDriverTree);
    pDriverInfo = (PHOOK_DRIVER_INFO)pKBTreeHookedDriver->SearchData(&DriverInfo, 0);
//    MUTEX_RELEASE(LockMutexToHookedDriverTree);
    if (pDriverInfo != NULL)
    {
      if (dwMajorFunction == IRP_MJ_SHUTDOWN)
        bSystemShutdown = TRUE;
      if (pDriverInfo->m_MajorFunction[dwMajorFunction] != NULL)
      {
        NtStatus = STATUS_SUCCESS;
  
        if (
            (dwMajorFunction == IRP_MJ_SHUTDOWN) ||
            (bIrpAlreadyTreat == FALSE) &&
            (bIsRightDeviceType == TRUE)
           )
        {
          #ifdef HOOK_QUERY_DIRECTORY_IRP
          if (dwMajorFunction == IRP_MJ_DIRECTORY_CONTROL)
          {
            if (dwMinorFunction == IRP_MN_QUERY_DIRECTORY)
            {
              PIRP_COMPLETION_INFO pIrpCompletionInfo = (PIRP_COMPLETION_INFO)_AllocatePoolFromKHeap(hKHeapDHDefault, sizeof(IRP_COMPLETION_INFO));
              if (pIrpCompletionInfo != NULL)
              {
                pIrpCompletionInfo->m_pOriginalCompletionRoutine = currentIrpStack->CompletionRoutine;
                pIrpCompletionInfo->m_pOriginalContext = currentIrpStack->Context;
                pIrpCompletionInfo->m_OriginalControl = currentIrpStack->Control;
                pIrpCompletionInfo->m_CurrentIrpStack = *currentIrpStack;

                pIrpCompletionInfo->m_QueryDirectoryFileParam.m_pMajorFunction = pDriverInfo->m_MajorFunction[dwMajorFunction];

                currentIrpStack->Context = pIrpCompletionInfo;
                currentIrpStack->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;

                //DbgPrint ("StackCount = %u,  CurrentLocation = %u \n", Irp->StackCount, Irp->CurrentLocation);
                if (Irp->StackCount == Irp->CurrentLocation)
                {
                  //if (pIrpCompletionInfo->m_pOriginalCompletionRoutine != NULL)
                  {
                    currentIrpStack->CompletionRoutine = HighLevelIrpCompletion;
                    //DbgPrint ("Set HighLevelIrpCompletion !!!!\n");
                    ++DriverObjectBusy;
                  }
                  //else
                  //{
                  //  currentIrpStack->Context = pIrpCompletionInfo->m_pOriginalContext;
                  //  currentIrpStack->Control = pIrpCompletionInfo->m_OriginalControl;
                  //  FreePoolToKHeap(hKHeapDHDefault, pIrpCompletionInfo);
                  //}
                }
                else
                {
                  currentIrpStack->CompletionRoutine = IntermediateIrpCompletion;
                  //DbgPrint ("Set IntermediateIrpCompletion !!!!\n");
                  ++DriverObjectBusy;
                }
              }
            }
          }
          else
          #endif //HOOK_QUERY_DIRECTORY_IRP
          {
            if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
            {
              pTreatIrpInfo = (PTREAT_IRP_INFO)_AllocatePoolFromKHeap(hKHeapDHDefault, sizeof(TREAT_IRP_INFO));
              if (pTreatIrpInfo != NULL)
              {
                KeInitializeEvent(&pTreatIrpInfo->m_Event, NotificationEvent, FALSE);
                pTreatIrpInfo->m_pCurrentThread = PsGetCurrentThread();
                pTreatIrpInfo->m_pCurrentProcess = PsGetCurrentProcess();
                pTreatIrpInfo->m_CurrentIrpStack = *currentIrpStack;
                pTreatIrpInfo->NtStatus = STATUS_SUCCESS;
            
                #ifndef __WIN2K
                ++DriverObjectBusy;
            
                ExInitializeWorkItem(&pTreatIrpInfo->m_WorkItem, (PWORKER_THREAD_ROUTINE)TreatmentIrpThread, pTreatIrpInfo);
                ExQueueWorkItem(&pTreatIrpInfo->m_WorkItem, DelayedWorkQueue);
                KeWaitForSingleObject(&pTreatIrpInfo->m_Event, Executive, KernelMode, FALSE, NULL);
                *currentIrpStack = pTreatIrpInfo->m_CurrentIrpStack;
                NtStatus = pTreatIrpInfo->NtStatus;
                #else
                pTreatIrpInfo->m_pWorkItem = IoAllocateWorkItem(DeviceObject);
                if (pTreatIrpInfo->m_pWorkItem != NULL)
                {
                  ++DriverObjectBusy;
            
                  IoQueueWorkItem(pTreatIrpInfo->m_pWorkItem, (PIO_WORKITEM_ROUTINE)TreatmentIrpThread, DelayedWorkQueue, pTreatIrpInfo);
                  KeWaitForSingleObject(&pTreatIrpInfo->m_Event, Executive, KernelMode, FALSE, NULL);
                  NtStatus = pTreatIrpInfo->NtStatus;
                  *currentIrpStack = pTreatIrpInfo->m_CurrentIrpStack;
                  if (pTreatIrpInfo->m_bGetObjectNameError == TRUE)
                  {
                    bIrpAlreadyTreat = TRUE;
                    pKBTreeIrps->Delete(Irp, 0);
                  }
                }
                #endif //__WIN2K
                
                FreePoolToKHeap(hKHeapDHDefault, pTreatIrpInfo);
              }
            } //if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
          } //if (dwMajorFunction != IRP_MJ_DIRECTORY_CONTROL)
        }
  
        if (NT_SUCCESS(NtStatus))
        {
          bInvalidRequest = FALSE;
          NtStatus = pDriverInfo->m_MajorFunction[dwMajorFunction](DeviceObject, Irp);
        }
      } //if (pDriverInfo->m_MajorFunction[dwMajorFunction] != NULL)
    } //if (pDriverInfo != NULL)
  } // try
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    bInvalidRequest = TRUE;
  }

  if (bIrpAlreadyTreat == FALSE)
    pKBTreeIrps->Delete(Irp, 0);

  if (bInvalidRequest == TRUE)
  {
    NtStatus = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Status = NtStatus;
    Irp->IoStatus.Information = 0;
//    IoCancelIrp(Irp);
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
  }

  --DriverObjectBusy;

  return NtStatus;
}

BOOLEAN IsRightDeviceTypeForFunc(DEVICE_TYPE DeviceType, ULONG dwMajorFunction)
{
  BOOLEAN bRes = FALSE;

  if (
      (
       dwMajorFunction == IRP_MJ_CREATE 
      ) ||
      (
       dwMajorFunction == IRP_MJ_CREATE_NAMED_PIPE
      ) ||
      (
       dwMajorFunction == IRP_MJ_CREATE_MAILSLOT
      )
      #ifdef HOOK_QUERY_DIRECTORY_IRP
       ||
      (
       dwMajorFunction == IRP_MJ_DIRECTORY_CONTROL
      ) 
      #endif //HOOK_QUERY_DIRECTORY_IRP
     )
  {
    if (
           DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM
        || DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM
        || DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM
       )
    {
      bRes = TRUE;
    }
    
    if (
           DeviceType == FILE_DEVICE_MAILSLOT
        || DeviceType == FILE_DEVICE_NAMED_PIPE
        || DeviceType == FILE_DEVICE_PARALLEL_PORT
        || DeviceType == FILE_DEVICE_SERIAL_PORT
        || DeviceType == FILE_DEVICE_FILE_SYSTEM //?????
        || DeviceType == FILE_DEVICE_TAPE_FILE_SYSTEM
        #ifdef __WIN2K
        || DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM  //?????
        #endif//__WIN2K
       )
    {
      if (dwMajorFunction == IRP_MJ_DIRECTORY_CONTROL)
        bRes = FALSE;
      else
        bRes = TRUE;
    }
  }

  return bRes;
}

#ifndef __WIN2K
void TreatmentIrpThread(IN PTREAT_IRP_INFO pTreatIrpInfo)
#else
void TreatmentIrpThread(IN PDEVICE_OBJECT DeviceObject, IN PTREAT_IRP_INFO pTreatIrpInfo)
#endif //__WIN2K
{
  PIO_STACK_LOCATION  currentIrpStack = &pTreatIrpInfo->m_CurrentIrpStack;
  PWSTR               pwszFullObjectName;
  NTSTATUS            NtStatus = STATUS_SUCCESS;

  pTreatIrpInfo->m_bGetObjectNameError = FALSE;
  if (bSystemShutdown == TRUE)
  {
    if (DriverHookThreadObjectPointer != NULL)
    {
      KeWaitForSingleObject(DriverHookThreadObjectPointer, UserRequest, KernelMode, FALSE, NULL);
      ObDereferenceObject(DriverHookThreadObjectPointer);
      DriverHookThreadObjectPointer = NULL;
    }
    pTreatIrpInfo->NtStatus = NtStatus;
    --DriverObjectBusy;

    #ifdef __WIN2K
    IoFreeWorkItem(pTreatIrpInfo->m_pWorkItem);
    #endif //__WIN2K
    KeSetEvent(&pTreatIrpInfo->m_Event, 0, FALSE);
    return;
  }

  switch (currentIrpStack->MajorFunction)
  {
    case IRP_MJ_CREATE:
    case IRP_MJ_CREATE_NAMED_PIPE:
    case IRP_MJ_CREATE_MAILSLOT:
         pTreatIrpInfo->m_bGetObjectNameError = TRUE;
         pwszFullObjectName = (PWSTR)_AllocatePoolFromKHeap(hKHeapDHDefault, MAXPATHLEN*sizeof(WCHAR));
         if (pwszFullObjectName != NULL)
         {
           if (GetObjectNameByFileObject(currentIrpStack->FileObject, pwszFullObjectName, MAXPATHLEN*sizeof(WCHAR)-sizeof(WCHAR)) != 0)
           {
             pTreatIrpInfo->m_bGetObjectNameError = FALSE;
             NtStatus = TreatmentCreateObjectIRP(pwszFullObjectName, currentIrpStack, pTreatIrpInfo->m_pCurrentThread, pTreatIrpInfo->m_pCurrentProcess);
           }
           FreePoolToKHeap(hKHeapDHDefault, pwszFullObjectName);
           pwszFullObjectName = NULL;
         }
         break;
    #ifdef HOOK_QUERY_DIRECTORY_IRP
    case IRP_MJ_DIRECTORY_CONTROL:
//         if (currentIrpStack->MinorFunction == IRP_MN_QUERY_DIRECTORY)
         {
           //DbgPrint ("Start Treatment irp !!!!\n");
           pwszFullObjectName = (PWSTR)_AllocatePoolFromKHeap(hKHeapDHDefault, MAXPATHLEN*sizeof(WCHAR));
           if (pwszFullObjectName != NULL)
           {
             ULONG dwDirectoryNameSize = GetObjectNameByFileObject(pTreatIrpInfo->m_pQueryDirParam->m_pFileObject, pwszFullObjectName, MAXPATHLEN*sizeof(WCHAR)-2*sizeof(WCHAR));
             if (dwDirectoryNameSize >= sizeof(WCHAR))
             {
               if (pwszFullObjectName[dwDirectoryNameSize/sizeof(WCHAR)-1] != L'\\')
               {
                 pwszFullObjectName[(dwDirectoryNameSize / sizeof(WCHAR))] = L'\\';
                 dwDirectoryNameSize += sizeof(WCHAR);
                 pwszFullObjectName[(dwDirectoryNameSize / sizeof(WCHAR))] = 0;
               }
               NtStatus = TreatmentQueryDirectoryIRP(pwszFullObjectName, dwDirectoryNameSize, MAXPATHLEN*sizeof(WCHAR), pTreatIrpInfo->m_pQueryDirParam, pTreatIrpInfo->m_pCurrentThread, pTreatIrpInfo->m_pCurrentProcess);
             }
             FreePoolToKHeap(hKHeapDHDefault, pwszFullObjectName);
             pwszFullObjectName = NULL;
           }
         }
         break;
    #endif //HOOK_QUERY_DIRECTORY_IRP
  }

  pTreatIrpInfo->NtStatus = NtStatus;
  --DriverObjectBusy;

  #ifdef __WIN2K
  IoFreeWorkItem(pTreatIrpInfo->m_pWorkItem);
  #endif //__WIN2K
  KeSetEvent(&pTreatIrpInfo->m_Event, 0, FALSE);
}

NTSTATUS TreatmentCreateObjectIRP(PWSTR pwszFullObjectName, PIO_STACK_LOCATION pCurrentIrpStack, PETHREAD pCurrentThread, PEPROCESS pCurrentProcess)
{
  NTSTATUS              NtStatus;
  ULONG                 dwFileAccessType, dwDirAccessType;
  ACCESS_MASK           DesiredAccess;
  ULONG                 ShareAccess;
  ULONG                 Disposition;
  ULONG                 CreateOptions;
  ULONG                 Options;
  ULONG                 FullCreateOptions;

  if (
      pwszFullObjectName == NULL ||
      pCurrentIrpStack == NULL
     )
  {
    NtStatus = STATUS_NO_MEMORY;
    return NtStatus;
  }

  NtStatus = STATUS_SUCCESS;

  Options = pCurrentIrpStack->Parameters.Create.Options;

  if (pCurrentIrpStack->Parameters.Create.SecurityContext)
  {
    DesiredAccess = pCurrentIrpStack->Parameters.Create.SecurityContext->DesiredAccess;
    FullCreateOptions = pCurrentIrpStack->Parameters.Create.SecurityContext->FullCreateOptions;
  }
  else
  {
    DesiredAccess = 0;
    FullCreateOptions = 0;
  }
  ShareAccess = pCurrentIrpStack->Parameters.Create.ShareAccess;
  Disposition = (Options >> 24) & 0xff;
  CreateOptions = Options & FILE_VALID_OPTION_FLAGS; //0x0000ffff;

  if (IsFileProtectedLight(hKHeapDHDefault, pwszFullObjectName, &dwFileAccessType, &dwDirAccessType, pCurrentThread, pCurrentProcess))
  {
    if (dwFileAccessType & FILE_ACC_TYPE_EXCHANGE)
    {
      DbgPrint ("dwFileAccessType - Not implemented yet !!!!\n");
    }
    else
    {
      if (!GetUpdateOptions(dwFileAccessType, &DesiredAccess, &ShareAccess, &Disposition, &CreateOptions))
      {
        NtStatus = STATUS_ACCESS_VIOLATION;
      }
      else
      {
        pCurrentIrpStack->Flags |= SL_FORCE_ACCESS_CHECK;
        if (pCurrentIrpStack->Parameters.Create.SecurityContext)
        {
          pCurrentIrpStack->Parameters.Create.SecurityContext->DesiredAccess = DesiredAccess;
          if (pCurrentIrpStack->Parameters.Create.SecurityContext->AccessState)
          {
            pCurrentIrpStack->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess = DesiredAccess;  
            pCurrentIrpStack->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess = DesiredAccess; 
            pCurrentIrpStack->Parameters.Create.SecurityContext->AccessState->OriginalDesiredAccess = DesiredAccess;   
          }
        }
        pCurrentIrpStack->Parameters.Create.ShareAccess = (USHORT)ShareAccess;
        Options = (Options & 0x00ffffff) | (Disposition<<24);
        Options = (Options & ~FILE_VALID_OPTION_FLAGS) | CreateOptions; //0xffff0000
        pCurrentIrpStack->Parameters.Create.Options = Options;
      }
    }
  }

  //
  //  проверка - можно ли в данном каталоге создавать объекты
  //
  
  if (!(dwDirAccessType & FILE_ACC_TYPE_WRITE))
  {
    if (
        (DesiredAccess & (DELETE | FILE_WRITE_EA)) ||
        (CreateOptions & FILE_DELETE_ON_CLOSE) ||
        Disposition == FILE_SUPERSEDE ||
        Disposition == FILE_CREATE ||
        Disposition == FILE_OPEN_IF ||
        Disposition == FILE_OVERWRITE_IF
       )
    {
      NtStatus = STATUS_ACCESS_VIOLATION;
    }
  }

  return NtStatus;
}

#ifdef HOOK_QUERY_DIRECTORY_IRP
NTSTATUS TreatmentQueryDirectoryIRP(PWSTR pwszFullObjectName, ULONG dwObjectNameSize, ULONG dwObjectNameBufferSize, PQUERY_DIRECTORY_FILE_PARAM pQueryDirParam, PETHREAD pCurrentThread, PEPROCESS pCurrentProcess)
{
  PWSTR                 pwszAfterPath;
  PFQD_SmallCommonBlock pQueryDirWin32, pQueryDirWin32Prev;
  ULONG                 dwFileNameLength;
  PWSTR                 pFileName;
  ULONG                 dwFileAccessType;
  BOOLEAN               ByOne, Reset;
  BOOLEAN               bBreak = FALSE;
  PVOID                 Buffer;
  ULONG                 BufferLength;
  FILE_INFORMATION_CLASS DirectoryInfoClass;
  NTSTATUS              NtStatus;

  NtStatus = STATUS_SUCCESS;

  __try
  {
    if (
        pwszFullObjectName == NULL ||
        pQueryDirParam == NULL
       )
    {
      NtStatus = STATUS_NO_MEMORY;
      return NtStatus;
    }

    pwszAfterPath = pwszFullObjectName + (dwObjectNameSize / sizeof(WCHAR));

    pQueryDirWin32 = (PFQD_SmallCommonBlock)pQueryDirParam->m_Buffer;
    pQueryDirWin32Prev = pQueryDirWin32;
    if (_MmIsAddressValid(pQueryDirWin32))
    {
      Buffer = (PVOID)pQueryDirWin32;
      BufferLength = pQueryDirParam->m_BufferLength;
      DirectoryInfoClass = pQueryDirParam->m_DirectoryInfoClass;
      ByOne = pQueryDirParam->m_ByOne;
      Reset = pQueryDirParam->m_Reset;
      
      while (_MmIsAddressValid(pQueryDirWin32))
      {
        pFileName = 0;
        dwFileNameLength = 0;
        switch (DirectoryInfoClass)
        {
          case 0:
               dwFileNameLength = ((PFILE_NAMES_INFORMATION)pQueryDirWin32)->FileNameLength;
               pFileName = ((PFILE_NAMES_INFORMATION)pQueryDirWin32)->FileName;
               break;
          case FileDirectoryInformation:
               dwFileNameLength = ((PFILE_DIRECTORY_INFORMATION)pQueryDirWin32)->CommonBlock.FileNameLength;
               pFileName = ((PFILE_DIRECTORY_INFORMATION)pQueryDirWin32)->FileName;
               break;
          case FileFullDirectoryInformation:
               dwFileNameLength = ((PFILE_FULL_DIR_INFORMATION)pQueryDirWin32)->CommonBlock.FileNameLength;
               pFileName = ((PFILE_FULL_DIR_INFORMATION)pQueryDirWin32)->FileName;
               break;
          case FileBothDirectoryInformation:
               dwFileNameLength = ((PFILE_BOTH_DIR_INFORMATION)pQueryDirWin32)->CommonBlock.FileNameLength;
               pFileName = ((PFILE_BOTH_DIR_INFORMATION)pQueryDirWin32)->FileName;
               break;
        }
    
        if (_MmIsAddressValid(pFileName))
        {
          if (dwFileNameLength == sizeof(L'.'))
          {
            if (*(WCHAR*)pFileName == L'.')
              pFileName = 0;
          }
          else
          {
            if (dwFileNameLength == 2*sizeof(L'.'))
            {
              if (*(ULONG*)pFileName == 0x002e002e) // L".." = 0x002e002e
                pFileName = 0;
            }
          }
        }
    
        if (_MmIsAddressValid(pFileName) && (dwFileNameLength < (dwObjectNameBufferSize-dwObjectNameSize-sizeof(WCHAR))))
        {
          if (pFileName[0] == L'\\')
          {
            pFileName = &pFileName[1];
            dwFileNameLength -= sizeof(WCHAR);
          }
          memcpy(pwszAfterPath, pFileName, dwFileNameLength);
          pwszAfterPath[dwFileNameLength/2] = 0;
          
          if (IsFileProtectedLight(hKHeapDHDefault, pwszFullObjectName, &dwFileAccessType, NULL, pCurrentThread, pCurrentProcess))
          {
            if (
                (dwFileAccessType & FILE_ACC_TYPE_EXCHANGE) ||
                (dwFileAccessType & FILE_ACC_TYPE_VISIBLE)
               )
            {
              if (dwFileAccessType & FILE_ACC_TYPE_EXCHANGE)
              {
                DbgPrint ("Not implemented yet !!!!\n");
              }
            }
            else
            {
              if (ByOne)
              {
                DbgPrint ("ByOne = TRUE!!!!\n");
                Reset = FALSE;
                pQueryDirParam->m_Index = FALSE;
    
                NtStatus = NativeQueryDirectoryFile(NULL,
                                                    pQueryDirParam->m_pDeviceObject,
                                                    pQueryDirParam->m_pFileObject,
                                                    pQueryDirParam->m_pIoStatusBlock,
                                                    Buffer,
                                                    BufferLength,
                                                    DirectoryInfoClass,
                                                    ByOne,
                                                    &pQueryDirParam->m_SearchTemplate,
                                                    Reset,
                                                    pQueryDirParam->m_Index,
                                                    //pQueryDirParam->m_dwIndex
                                                    0
                                                    );
                bBreak = TRUE;
              }
              else
              {
                bBreak = DeleteItemFromQueryDirectoryBuffer(pQueryDirWin32Prev, pQueryDirWin32, Buffer, BufferLength, pQueryDirParam->m_pIoStatusBlock, DirectoryInfoClass, &NtStatus);
                pQueryDirWin32 = pQueryDirWin32Prev;
                if (bBreak == FALSE && Buffer == pQueryDirWin32)
                  continue;
              }
            }  //Check (else) pFileInfo->dwAccessType
          } //if (IsFileProtected(...))
          
          if (bBreak)
          {
            if (NtStatus != STATUS_NO_MORE_FILES || ByOne)
            {
              break;
            }
            else
            {
              Reset = FALSE;
              pQueryDirParam->m_Index = FALSE;
    
              NtStatus = NativeQueryDirectoryFile(pQueryDirParam->m_pMajorFunction,
                                                  pQueryDirParam->m_pDeviceObject,
                                                  pQueryDirParam->m_pFileObject,
                                                  pQueryDirParam->m_pIoStatusBlock,
                                                  Buffer,
                                                  BufferLength,
                                                  DirectoryInfoClass,
                                                  ByOne,
                                                  &pQueryDirParam->m_SearchTemplate,
                                                  Reset,
                                                  pQueryDirParam->m_Index,
                                                  //(pQueryDirParam->m_dwIndex)
                                                  0
                                                  );
              if (NT_SUCCESS(NtStatus))
              {
                bBreak = FALSE;
                pQueryDirWin32 = (PFQD_SmallCommonBlock) Buffer;
                pQueryDirWin32Prev = pQueryDirWin32;
                continue;
              }
              else
              {
                break;
              }
            }
          } //if (bBreak)
        } //if (pFileName != 0 && ...))
    
        if (pQueryDirWin32->NextEntryOffset == 0)
        {
//          dwRealSize += dwSizeOfItem;
          break;
        }
        pQueryDirWin32Prev = pQueryDirWin32;
        pQueryDirWin32 = (PFQD_SmallCommonBlock)((CHAR*)pQueryDirWin32 + pQueryDirWin32->NextEntryOffset);
      }
      
    }
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    NtStatus = STATUS_ACCESS_VIOLATION;
    DbgPrint (" Exception\n");
  }

  return NtStatus;
}
#endif //HOOK_QUERY_DIRECTORY_IRP

int CompareDriver(PHOOK_DRIVER_INFO pDriverInfo1, PHOOK_DRIVER_INFO pDriverInfo2, ULONG dwCompareParam)
{
  int nCmp = 0;
  BOOLEAN bValid1, bValid2;

  bValid1 = _MmIsAddressValid(pDriverInfo1);
  bValid2 = _MmIsAddressValid(pDriverInfo2);

  if (!bValid1 && !bValid2)
  {
    nCmp = 0;
  }
  else
  {
    if (bValid1 && !bValid2)
    {
      nCmp = 1;
    }
    else
    {
      if (!bValid1 && bValid2)
      {
        nCmp = -1;
      }
      else
      {
        if ((DWORD)pDriverInfo1->m_pDriverObject > (DWORD)pDriverInfo2->m_pDriverObject)
        {
          nCmp = 1;
        }
        else
        {
          if ((DWORD)pDriverInfo1->m_pDriverObject < (DWORD)pDriverInfo2->m_pDriverObject)
            nCmp = -1;
          else
            nCmp = 0;
        }
      }
    }
  }

  return nCmp;
}

BOOLEAN AddHookedDriverIntoTree(PDRIVER_OBJECT pDriverObject)
{
  PHOOK_DRIVER_INFO  pHookDriverInfo;
  BOOLEAN            bRes = FALSE;
  int                i;

  if (!_MmIsAddressValid(pDriverObject))
    return bRes;

  MUTEX_WAIT(LockMutexToHookedDriverTree);

  if (!_MmIsAddressValid(pKBTreeHookedDriver))
  {
    pKBTreeHookedDriver = new KBinaryTree((BTREE_COMPARE) CompareDriver);
    if (!_MmIsAddressValid(pKBTreeHookedDriver))
    {
      pKBTreeHookedDriver = NULL;
      MUTEX_RELEASE(LockMutexToHookedDriverTree);
      return bRes;
    }
  }

  pHookDriverInfo = (PHOOK_DRIVER_INFO)_AllocatePoolFromKHeap(hKHeapDHDefault, sizeof(HOOK_DRIVER_INFO));
  if (pHookDriverInfo != NULL)
  {
//    pHookDriverInfo->m_dwStates = 0;
    pHookDriverInfo->m_pDriverObject = pDriverObject;
    pHookDriverInfo->m_pDriverUnload = pDriverObject->DriverUnload;

//    memcpy(&(pHookDriverInfo->m_FastIoDispatchHook), &FastIOHook, sizeof(FastIOHook));
//    pHookDriverInfo->m_pFastIoDispatchOriginal = pDriverObject->FastIoDispatch;
    memcpy(pHookDriverInfo->m_MajorFunction, pDriverObject->MajorFunction, sizeof(pHookDriverInfo->m_MajorFunction));

    if (pKBTreeHookedDriver->Insert(pHookDriverInfo, 0) != NULL)
    {
      __asm cli

      if (_MmIsAddressValid(pHookDriverInfo->m_pDriverUnload))
       pDriverObject->DriverUnload = DriverUnload;

//      if (pDriverObject->FastIoDispatch != NULL)
//        pDriverObject->FastIoDispatch = &(pHookDriverInfo->m_FastIoDispatchHook);
      for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
      {
        if (pDriverObject->MajorFunction[i] != NULL)
          pDriverObject->MajorFunction[i] = DriverObjectDispatch;
      }

      __asm sti

      DbgPrint ("AddHookedDriverIntoTree: Hooked drivers - %u (%08x) !!!\n", pKBTreeHookedDriver->NodesInTree(), pDriverObject);
      bRes = TRUE;
    }
    else
    {
      FreePoolToKHeap(hKHeapDHDefault, pHookDriverInfo);
    }
  }

  MUTEX_RELEASE(LockMutexToHookedDriverTree);

  return bRes;
}

BOOLEAN DelHookedDriverFromTree(PDRIVER_OBJECT pDriverObject)
{
  PHOOK_DRIVER_INFO  pHookDriverInfo = NULL;
  HOOK_DRIVER_INFO   HookDriverInfo;
  BOOLEAN            bRes = FALSE;

  if (!_MmIsAddressValid(pDriverObject))
    return bRes;

  MUTEX_WAIT(LockMutexToHookedDriverTree);

  if (pKBTreeHookedDriver != NULL)
  {
    HookDriverInfo.m_pDriverObject = pDriverObject;
    pHookDriverInfo = (PHOOK_DRIVER_INFO)pKBTreeHookedDriver->SearchData(&HookDriverInfo, 0);
    pKBTreeHookedDriver->Delete(&HookDriverInfo, 0);
  }

  if (_MmIsAddressValid(pHookDriverInfo))
  {
    __asm cli

    pDriverObject->DriverUnload = pHookDriverInfo->m_pDriverUnload;

//    pDriverObject->FastIoDispatch = pHookDriverInfo->m_pFastIoDispatchOriginal;

    memcpy(pDriverObject->MajorFunction, pHookDriverInfo->m_MajorFunction, sizeof(pHookDriverInfo->m_MajorFunction));

    __asm sti 

    FreePoolToKHeap(hKHeapDHDefault, pHookDriverInfo);
  }

  if (pKBTreeHookedDriver != NULL)
    DbgPrint ("DelHookedDriverIntoTree: Hooked drivers - %u (%08x) !!!\n", pKBTreeHookedDriver->NodesInTree(), pDriverObject);

  MUTEX_RELEASE(LockMutexToHookedDriverTree);

  return bRes;
}

VOID ClearHookedDriverTree(VOID)
{
  PHOOK_DRIVER_INFO  pHookDriverInfo;

  if (_MmIsAddressValid(pKBTreeHookedDriver))
  {
    MUTEX_WAIT(LockMutexToHookedDriverTree);

    while (!pKBTreeHookedDriver->IsEmpty())
    {
      pHookDriverInfo = (PHOOK_DRIVER_INFO)pKBTreeHookedDriver->GetRootData();
      if (_MmIsAddressValid(pHookDriverInfo))
      {
        DelHookedDriverFromTree(pHookDriverInfo->m_pDriverObject);
      }  
    }

    delete pKBTreeHookedDriver;
    pKBTreeHookedDriver = NULL;

    MUTEX_RELEASE(LockMutexToHookedDriverTree);
  }
}

void DriverHookThread(PVOID pContext)
{
  LARGE_INTEGER          DelayTime;
  NTQUERYDIRECTORYOBJECT pZwQueryDirectoryObject = 0;
  int                    i;
  PDRIVER_OBJECT         pDriverObject;

  pZwQueryDirectoryObject = GetPtrToZwQueryDirectoryObject();
  if (!_MmIsAddressValid(pZwQueryDirectoryObject))
  {
    PsTerminateSystemThread(STATUS_SUCCESS);
    return;
  }

  if (KeGetCurrentIrql() == PASSIVE_LEVEL)
  {
    #ifndef __WIN2K
    while (bDriverObjectUnhookStarted == FALSE && bSystemShutdown == FALSE)
    #endif //__WIN2K
    {
      ScanObjectDirectory(L"\\Driver", pZwQueryDirectoryObject);

      #ifndef __WIN2K
      if (bDriverObjectUnhookStarted == TRUE || bSystemShutdown == TRUE)
        break;
      #endif //__WIN2K

      ScanObjectDirectory(L"\\FileSystem", pZwQueryDirectoryObject);

      #ifndef __WIN2K
      for (i=0; i<10; ++i)
      {
        if (bDriverObjectUnhookStarted == TRUE || bSystemShutdown == TRUE)
          break;
        DelayTime.QuadPart = 10*1000*1000;   // 1 second
        DelayTime.QuadPart = -DelayTime.QuadPart;
        KeDelayExecutionThread(KernelMode, FALSE, &DelayTime);
      }
      #endif //__WIN2K
    }
  }

  PsTerminateSystemThread(STATUS_SUCCESS);
}

void ScanObjectDirectory(PWSTR pwszObjectDir, NTQUERYDIRECTORYOBJECT pZwQueryDirectoryObject)
{
  int                    i;
  UNICODE_STRING         DirString;
  OBJECT_ATTRIBUTES      DirAttr;
  NTSTATUS               NtStatus;
  HANDLE                 hDir;
  CHAR                   Buf[512];
  WCHAR                  DriverPath[256];
  BOOLEAN                bFirst = TRUE;
  ULONG                  ObjectIndex = 0;
  ULONG                  LengthReturned = 0;
  ULONG                  Index = 0;
  POBJECT_NAMETYPE_INFO  pObjName = (POBJECT_NAMETYPE_INFO) Buf;
  PDRIVER_OBJECT         pDriverObject;

  if (_MmIsAddressValid(pwszObjectDir) && _MmIsAddressValid(pZwQueryDirectoryObject))
  {
    bFirst = TRUE;
    ObjectIndex = 0;
    LengthReturned = 0;
    Index = 0;

    RtlInitUnicodeString(&DirString, pwszObjectDir);
    InitializeObjectAttributes(&DirAttr, &DirString, OBJ_CASE_INSENSITIVE, NULL, NULL);
    NtStatus = ZwOpenDirectoryObject(&hDir, DIRECTORY_QUERY, &DirAttr);
    if (NtStatus == STATUS_SUCCESS)
    {
      while (pZwQueryDirectoryObject(hDir, Buf, sizeof(Buf), ObjectArray, bFirst, &ObjectIndex, &LengthReturned) >= 0)
      {
        if (bDriverObjectUnhookStarted == TRUE || bSystemShutdown == TRUE)
          break;
        bFirst = FALSE;
        for (i=0; Index<ObjectIndex; ++Index, ++i)
        {
          wcscpy(DriverPath, pwszObjectDir);
          wcscat(DriverPath, L"\\");
          if ((sizeof(DriverPath)-wcslen(DriverPath)-sizeof(WCHAR)) > pObjName[i].ObjectName.Length)
          {
            wcscat(DriverPath, pObjName[i].ObjectName.Buffer);
            pDriverObject = (PDRIVER_OBJECT)GetObjectByPath(DriverPath, IoDriverObjectType);
            if (pDriverObject != NULL)
            {
              AddDriverObject(pDriverObject);
            }
          }
        }
      }
      ZwClose(hDir);
    }
  }
}

void AddDriverObject(IN PDRIVER_OBJECT pDriverObject)
{
  int                    i;
  PDEVICE_OBJECT         TopDeviceObject;
  PDEVICE_OBJECT         pDeviceObject;
  PDRIVER_OBJECT         pTargetDriverObject;


  pDeviceObject = pDriverObject->DeviceObject;
  while (pDeviceObject)
  {
    TopDeviceObject = pDeviceObject;
    do
    {
      if (IsRightDeviceTypeForFunc(TopDeviceObject->DeviceType, IRP_MJ_CREATE) == TRUE)
      {
        pTargetDriverObject = TopDeviceObject->DriverObject;
        
        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
        {
          if (pTargetDriverObject->MajorFunction[i] != NULL)
          {
            if (pTargetDriverObject->MajorFunction[i] != DriverObjectDispatch)
            {
              AddHookedDriverIntoTree(pTargetDriverObject);
            }
            break;
          }
        }
      }
      TopDeviceObject = TopDeviceObject->AttachedDevice;
    }
    while (TopDeviceObject);
    pDeviceObject = pDeviceObject->NextDevice;
  }

  return;
}

VOID HookDriverObjects(VOID)
{
  HANDLE                hThread;
  NTSTATUS              NtStatus;

  if (!(dwDeviceState & HE4_STATE_HOOK_DRIVER_OBJECT))
  {
    bDriverObjectUnhookStarted = FALSE;

    DriverObjectBusy = new KInterlockedCounter(0);
    if (DriverObjectBusy == NULL)
      return;

    pKBTreeIrps = new KBinaryTree(NULL);
    if (_MmIsAddressValid(pKBTreeIrps))
    {
      NtStatus = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, (PKSTART_ROUTINE) DriverHookThread, (PVOID) NULL);
      if (NT_SUCCESS(NtStatus))
      {
        NtStatus = ObReferenceObjectByHandle( hThread, THREAD_ALL_ACCESS, NULL, KernelMode, &DriverHookThreadObjectPointer, NULL );
        if (NT_SUCCESS(NtStatus))
        {
          dwDeviceState |= HE4_STATE_HOOK_DRIVER_OBJECT;
        }
        else
        {
          bDriverObjectUnhookStarted = TRUE;
        }
        ZwClose(hThread);
      }

      if (!(dwDeviceState & HE4_STATE_HOOK_DRIVER_OBJECT))
      {
        if (pKBTreeIrps != NULL)
          delete pKBTreeIrps;
        pKBTreeIrps = NULL;
        delete DriverObjectBusy;
        DriverObjectBusy = NULL;
      }
    }
    else
    {
      delete DriverObjectBusy;
      DriverObjectBusy = NULL;
    }
  }
}

BOOLEAN UnHookDriverObjects(VOID)
{
  ULONG*         pRealIofCallDriver = (ULONG*) IofCallDriver;
  BOOLEAN        bRes = TRUE;

  if (dwDeviceState & HE4_STATE_HOOK_DRIVER_OBJECT)
  {
    bDriverObjectUnhookStarted = TRUE;

    if (DriverHookThreadObjectPointer != NULL)
    {
      KeWaitForSingleObject(DriverHookThreadObjectPointer, Executive/*UserRequest*/, KernelMode, FALSE, NULL);
      ObDereferenceObject(DriverHookThreadObjectPointer);
      DriverHookThreadObjectPointer = NULL;
    }

    ClearHookedDriverTree();

    if (DriverObjectBusy->CompareExchange(0, 0) == FALSE)
    {
      bRes = FALSE;
    }
    else
    {
      if (pKBTreeIrps != NULL)
        delete pKBTreeIrps;
      pKBTreeIrps = NULL;

      delete DriverObjectBusy;
      DriverObjectBusy = NULL;
      dwDeviceState &= ~HE4_STATE_HOOK_DRIVER_OBJECT;
    }
  }

  return bRes;
}

