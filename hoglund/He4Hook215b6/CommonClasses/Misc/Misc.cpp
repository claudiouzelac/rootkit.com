#include "Misc.h"

#ifdef __MISC_USE_KHEAP
 KHEAP               hKHeapMiscDefault = NULL;
#endif //__MISC_USE_KHEAP

//
// Функции для работы с памятью
//

BOOLEAN AllocateSharedMemory(PSHARED_MEMORY lpSharedMemory, POOL_TYPE PoolType, ULONG dwSizeRegion)
{
  if (!_MmIsAddressValid(lpSharedMemory))
    return FALSE;
  if (!dwSizeRegion)
    return FALSE;

  memset(lpSharedMemory, 0, sizeof(SHARED_MEMORY));

  #ifndef __MISC_USE_KHEAP
  lpSharedMemory->m_lpKernelMemory = ExAllocatePool(PoolType, dwSizeRegion);
  #else
  lpSharedMemory->m_lpKernelMemory = (CHAR*) _AllocatePoolFromKHeap(hKHeapMiscDefault, dwSizeRegion);
  #endif //!__MISC_USE_KHEAP
  if (!lpSharedMemory->m_lpKernelMemory)
    return FALSE;

  lpSharedMemory->m_Mdl = IoAllocateMdl(lpSharedMemory->m_lpKernelMemory, dwSizeRegion, FALSE, FALSE, NULL);
  if (!lpSharedMemory->m_Mdl)
  {
    #ifndef __MISC_USE_KHEAP
    ExFreePool(lpSharedMemory->m_lpKernelMemory);
    #else
    FreePoolToKHeap(hKHeapMiscDefault, lpSharedMemory->m_lpKernelMemory);
    #endif //!__MISC_USE_KHEAP
    memset(lpSharedMemory, 0, sizeof(SHARED_MEMORY));
    return FALSE;
  }

  MmBuildMdlForNonPagedPool(lpSharedMemory->m_Mdl);

  lpSharedMemory->m_lpUserPage = MmMapLockedPages(lpSharedMemory->m_Mdl, UserMode);
  lpSharedMemory->m_lpUserMemory = (PVOID) (((ULONG)PAGE_ALIGN(lpSharedMemory->m_lpUserPage))+MmGetMdlByteOffset(lpSharedMemory->m_Mdl));
  if (!_MmIsAddressValid(lpSharedMemory->m_lpUserMemory))
  {
    MmUnmapLockedPages(lpSharedMemory->m_lpUserPage, lpSharedMemory->m_Mdl);
    IoFreeMdl(lpSharedMemory->m_Mdl);
    #ifndef __MISC_USE_KHEAP
    ExFreePool(lpSharedMemory->m_lpKernelMemory);
    #else
    FreePoolToKHeap(hKHeapMiscDefault, lpSharedMemory->m_lpKernelMemory);
    #endif //!__MISC_USE_KHEAP
    memset(lpSharedMemory, 0, sizeof(SHARED_MEMORY));
    return FALSE;
  }
  lpSharedMemory->m_dwSizeRegion = dwSizeRegion;

  return TRUE;
}

BOOLEAN FreeSharedMemory(PSHARED_MEMORY lpSharedMemory)
{
  if (!_MmIsAddressValid(lpSharedMemory))
    return FALSE;
  if (!_MmIsAddressValid(lpSharedMemory->m_lpUserMemory))
    return FALSE;
  if (!_MmIsAddressValid(lpSharedMemory->m_lpKernelMemory))
    return FALSE;

  MmUnmapLockedPages(lpSharedMemory->m_lpUserPage, lpSharedMemory->m_Mdl);
  IoFreeMdl(lpSharedMemory->m_Mdl);
  #ifndef __MISC_USE_KHEAP
  ExFreePool(lpSharedMemory->m_lpKernelMemory);
  #else
  FreePoolToKHeap(hKHeapMiscDefault, lpSharedMemory->m_lpKernelMemory);
  #endif //!__MISC_USE_KHEAP
  memset(lpSharedMemory, 0, sizeof(SHARED_MEMORY));

  return TRUE;
}

PVOID
MapUserAddressToKernel(
   IN PVOID pUserModeAddress,
   IN ULONG ulSize,
   OUT PMDL* ppMdl
   )
{
 PMDL  pUserModeMdl = NULL;
 PVOID pMappedKernelAddr = NULL;

 if (ppMdl == NULL)
   return NULL;

 __try
 {
   pUserModeMdl = IoAllocateMdl(pUserModeAddress, ulSize, FALSE, FALSE, NULL);
   if (pUserModeMdl != NULL)
   {
     MmProbeAndLockPages(pUserModeMdl, KernelMode, IoModifyAccess);
     pMappedKernelAddr = MmMapLockedPages(pUserModeMdl, KernelMode);  
     if (pMappedKernelAddr != NULL)
     {
       pMappedKernelAddr = (PVOID) (((ULONG)PAGE_ALIGN(pMappedKernelAddr))+MmGetMdlByteOffset(pUserModeMdl));
       *ppMdl = pUserModeMdl;
     }
     else
     {
       UnmapMappedKernelAddress(pUserModeMdl);
     }
   }
 }
 __except(EXCEPTION_EXECUTE_HANDLER)
 {
   if (pUserModeMdl != NULL)
     IoFreeMdl(pUserModeMdl);
   pMappedKernelAddr = NULL;
 }
  
 return pMappedKernelAddr;
}


VOID
UnmapMappedKernelAddress(
   IN PMDL pMdl
   )
{
  if (pMdl == NULL)
    return;

  MmUnlockPages(pMdl);
  IoFreeMdl(pMdl);
}

BOOLEAN IsBadWritePtr(PVOID Address, ULONG Length, ULONG Alignment)
{
  __try
  {
    ProbeForWrite(Address, Length, Alignment);
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    return TRUE;
  }
  return FALSE;
}

BOOLEAN _MmIsAddressValid(PVOID Address)
{
  if (!Address)
    return FALSE;
  return MmIsAddressValid(Address);
}

//
// функции для работы с объектами ядра
//

//----------------------------------------------------------------------
//
// GetPointer
//
// Translates a handle to an object pointer.
//
//----------------------------------------------------------------------
POBJECT GetPointer(HANDLE handle)
{
  POBJECT         pKey;
  NTSTATUS        NtStatus;

  //
  // Ignore null handles
  //
  if (!handle && KeGetCurrentIrql() != PASSIVE_LEVEL)
    return NULL;

  //
  // Get the pointer the handle refers to
  //
  NtStatus = ObReferenceObjectByHandle(handle, 0, NULL, KernelMode, &pKey, NULL);
  if (NtStatus != STATUS_SUCCESS)
  {
    DbgPrintMisc(("He4hookInv: GetPointer: Error %08x getting key pointer\n", NtStatus));
    pKey = NULL;
  } 

  return pKey;
}


//----------------------------------------------------------------------
//
// ReleasePointer
//
// Dereferences the object.
//
//----------------------------------------------------------------------
VOID ReleasePointer(POBJECT object)
{
  if (object && KeGetCurrentIrql() == PASSIVE_LEVEL)
    ObDereferenceObject(object);
}

ULONG GetObjectName(HANDLE hObject, PWSTR lpwszName, ULONG dwSize)
{
  CHAR                   *tmpname;//[0x400];
  PUNICODE_STRING         fullUniName;
  NTSTATUS                NtStatus;
  ULONG                   dwObjectNameLen = 0; //(by bytes)

  DbgPrintMisc(("He4hookInv: GetObjectName started!!!!!!!\n"));

  if (!lpwszName || !dwSize || KeGetCurrentIrql() > PASSIVE_LEVEL)
    return 0;

  //
  // Translate the hkey into a pointer
  //
  lpwszName[0] = 0;

  #ifndef __MISC_USE_KHEAP
  tmpname = (CHAR*) ExAllocatePool(NonPagedPool, dwSize+sizeof(UNICODE_STRING));
  #else
  tmpname = (CHAR*) _AllocatePoolFromKHeap(hKHeapMiscDefault, dwSize+sizeof(UNICODE_STRING));
  #endif //!__MISC_USE_KHEAP
  if (!_MmIsAddressValid(tmpname))
    return 0;
  tmpname[0] = 0;

  NtStatus = ZwQueryObject(hObject, NameObjectInfo, tmpname, dwSize+sizeof(UNICODE_STRING), NULL);

  if (NT_SUCCESS(NtStatus))
  {
    fullUniName = &(((PNAME_OBJECT_INFO)tmpname)->Name);
    dwObjectNameLen = fullUniName->Length;
    if (dwSize < (sizeof(WCHAR)+dwObjectNameLen))
    {
      DbgPrintMisc(("He4hookInv: GetObjectName ended - ERROR!!!!!!!\n"));
      #ifndef __MISC_USE_KHEAP
      ExFreePool(tmpname);
      #else
      FreePoolToKHeap(hKHeapMiscDefault, tmpname);
      #endif //!__MISC_USE_KHEAP
      return 0;
    }
//    memset(lpwszName, 0, (sizeof(WCHAR)+fullUniName->Length));
    memcpy(lpwszName, fullUniName->Buffer, dwObjectNameLen);
    lpwszName[dwObjectNameLen/sizeof(WCHAR)] = 0;
  }

  #ifndef __MISC_USE_KHEAP
  ExFreePool(tmpname);
  #else
  FreePoolToKHeap(hKHeapMiscDefault, tmpname);
  #endif //!__MISC_USE_KHEAP

  DbgPrintMisc(("He4hookInv: GetObjectName ended - OK!!!!!!!\n"));

  return dwObjectNameLen; 
}

ULONG GetObjectNameByObjectAttributes(POBJECT_ATTRIBUTES ObjectAttributes, PWSTR fullPathName, ULONG nfullPathNameSize)
{
  ULONG                   dwDirSize, dwNtPathNameLen;
  int                     nObjectNameLen = 0;
  PWSTR                   DosPathName;
  NTSTATUS                NtStatus;
  PWSTR                   pwszObjectName = NULL;

  if (
      !_MmIsAddressValid(ObjectAttributes) ||
      !_MmIsAddressValid(fullPathName)     ||
      !nfullPathNameSize
     )
    return 0;

  fullPathName[0] = 0;
  dwDirSize = 0;

  if (_MmIsAddressValid(ObjectAttributes->ObjectName))
  {
    if (ObjectAttributes->ObjectName->Length != 0)
    {
      if (_MmIsAddressValid(ObjectAttributes->ObjectName->Buffer))
      {
        pwszObjectName = ObjectAttributes->ObjectName->Buffer;
        nObjectNameLen = ObjectAttributes->ObjectName->Length;
      }
    }
  }

  #ifndef __MISC_USE_KHEAP
  DosPathName = (PWSTR) ExAllocatePool(NonPagedPool, nfullPathNameSize);
  #else
  DosPathName = (PWSTR) _AllocatePoolFromKHeap(hKHeapMiscDefault, nfullPathNameSize);
  #endif //!__MISC_USE_KHEAP
//  DosPathName = (PWSTR)_AllocatePoolFromKHeap(hKHeapFSDefault, nfullPathNameSize);
  if (DosPathName == NULL)
    return 0;

  DosPathName[0] = 0;

  DbgPrintMisc(("  GetFileNameByObjectAttributes: Attributes => %08x !!!\n", ObjectAttributes->Attributes));

  if (ObjectAttributes->RootDirectory)
  {
    dwDirSize = GetObjectName(ObjectAttributes->RootDirectory, DosPathName, nfullPathNameSize);
  }

  if (dwDirSize > 3 && pwszObjectName != NULL)
  {
    if (
        DosPathName[(dwDirSize/sizeof(WCHAR))-1] == L'\\' &&
        (pwszObjectName[0] == L'\\' || pwszObjectName[0] == L'/')
       )
    {
      DosPathName[(dwDirSize/sizeof(WCHAR))-1] = 0;
      dwDirSize -= sizeof(WCHAR);
    }
    else
    {
      if (
          DosPathName[(dwDirSize/sizeof(WCHAR))-1] != L'\\' &&
          (pwszObjectName[0] != L'\\' || pwszObjectName[0] != L'/')
         )
      {
        if (dwDirSize <= (nfullPathNameSize-2*sizeof(WCHAR)))
        {
          DosPathName[(dwDirSize/sizeof(WCHAR))] = L'\\';
          DosPathName[(dwDirSize/sizeof(WCHAR))+1] = 0;
          dwDirSize += sizeof(WCHAR);
        }
      }
    }
  }

  if (pwszObjectName != NULL)
  {
    if (nfullPathNameSize > (nObjectNameLen+dwDirSize))
    {
      memcpy(((PCHAR)DosPathName+dwDirSize), pwszObjectName, nObjectNameLen);
      ((PWSTR)((PCHAR)DosPathName+dwDirSize))[nObjectNameLen/sizeof(WCHAR)] = 0;
    }
  }

  dwNtPathNameLen = DosPathNameToNtPathName(DosPathName, fullPathName, nfullPathNameSize, 255, NULL);

  #ifndef __MISC_USE_KHEAP
  ExFreePool(DosPathName);
  #else
  FreePoolToKHeap(hKHeapMiscDefault, DosPathName);
  #endif //!__MISC_USE_KHEAP
  //FreePoolToKHeap(hKHeapFSDefault, DosPathName);

  return dwNtPathNameLen;
}

ULONG GetObjectNameByFileObject(PFILE_OBJECT fileObject, PWSTR fullPathName, ULONG nfullPathNameSize)
{
  ULONG               pathLen, dwVolumeLen;
  PWSTR               pathOffset;
  PFILE_OBJECT        relatedFileObject;
//  ANSI_STRING         componentName;
  PDEVICE_OBJECT      pDeviceObject;
  PUNICODE_STRING     fullUniName;
  UNICODE_STRING      FileNameUnicodeString;
  ULONG               dwFreeSize = nfullPathNameSize;
  ULONG               dwFullPathLen = 0;

  if (
      KeGetCurrentIrql() > PASSIVE_LEVEL ||
      !_MmIsAddressValid(fileObject) ||
      !_MmIsAddressValid(fullPathName)     ||
      !fileObject
     )
    return 0;

  fullPathName[0] = 0;
  dwVolumeLen = 0;

  __try
  {
    pDeviceObject = GetVolumeDeviceObject(fileObject);
    if (pDeviceObject == NULL)
    {
      pDeviceObject = IoGetRelatedDeviceObject(fileObject);
    }

    fullUniName = (PUNICODE_STRING)fullPathName;
    fullUniName->MaximumLength = (USHORT)(nfullPathNameSize*sizeof(WCHAR) - sizeof(UNICODE_STRING));

    if (NT_SUCCESS(ObQueryNameString(pDeviceObject, (POBJECT_NAME_INFORMATION)fullUniName, fullUniName->MaximumLength, &pathLen)))
    {
      dwVolumeLen = fullUniName->Length;
      memmove(fullPathName, fullUniName->Buffer, dwVolumeLen);
      fullPathName[dwVolumeLen/sizeof(WCHAR)] = 0;
    }
    else
    {
      return 0;
    }

//    strcat(fullPathName, "\\");
    dwFreeSize -= dwVolumeLen;

    if (!fileObject->FileName.Length || fileObject->FileName.Length > dwFreeSize)
    {
//      RtlInitUnicodeString(&FileNameUnicodeString, fullPathName);
//      RtlUnicodeStringToAnsiString( &componentName, &FileNameUnicodeString, TRUE );    
//      DbgPrint ("%s\n", componentName.Buffer);
//      RtlFreeAnsiString( &componentName );

      return wcslen(fullPathName)*sizeof(WCHAR);
    }

  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    return 0;
  }

  __try
  {
    pathLen = fileObject->FileName.Length/sizeof(WCHAR) + 1;

    relatedFileObject = fileObject->RelatedFileObject;
  
    if (fileObject->FileName.Buffer[0] != L'\\' )
    {
      while (relatedFileObject)
      {
        if ((pathLen + relatedFileObject->FileName.Length/sizeof(WCHAR) + 1) >= dwFreeSize)
        {
          break;
        }
        pathLen += relatedFileObject->FileName.Length/sizeof(WCHAR) + 1;
        relatedFileObject = relatedFileObject->RelatedFileObject;
      }
    }

    if (dwFreeSize == nfullPathNameSize)
      wcscpy(fullPathName, L"\\");
  
    pathOffset = fullPathName + dwVolumeLen/sizeof(WCHAR) - 1 + pathLen - fileObject->FileName.Length/sizeof(WCHAR);
    memcpy(pathOffset, fileObject->FileName.Buffer, fileObject->FileName.Length);
    pathOffset[fileObject->FileName.Length/sizeof(WCHAR)] = 0;

    relatedFileObject = fileObject->RelatedFileObject;
  
    if (fileObject->FileName.Buffer[0] != L'\\')
    {
      while (relatedFileObject)
      {
        *(pathOffset - 1) = L'\\';
        pathOffset -= relatedFileObject->FileName.Length/sizeof(WCHAR) + 1;

        if (pathOffset <= fullPathName)
        {
          break;
        }
        memcpy(pathOffset, relatedFileObject->FileName.Buffer, relatedFileObject->FileName.Length);

        relatedFileObject = relatedFileObject->RelatedFileObject;
      }
    }  

    dwFullPathLen = wcslen(fullPathName)*sizeof(WCHAR);

//    if (fullPathName[dwVolumeLen/sizeof(WCHAR)] == L'\\' && fullPathName[dwVolumeLen/sizeof(WCHAR)+1] == L'\\')
//    {
//      memmove(&fullPathName[dwVolumeLen/sizeof(WCHAR)], &fullPathName[dwVolumeLen/sizeof(WCHAR)+1], pathLen*sizeof(WCHAR));
//    }

    pathOffset = wcsstr(fullPathName, L"\\\\");
    while (pathOffset)
    {
      memmove(pathOffset, &pathOffset[1], wcslen(pathOffset)*sizeof(WCHAR));
      pathOffset = wcsstr(pathOffset, L"\\\\");
    }

//    RtlInitUnicodeString(&FileNameUnicodeString, fullPathName);
//    RtlUnicodeStringToAnsiString( &componentName, &FileNameUnicodeString, TRUE );    
//    DbgPrint ("%s\n", componentName.Buffer);
//    RtlFreeAnsiString( &componentName );

  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    DbgPrintMisc(("GetFullPath: EXCEPTION\n"));
  }

  return dwFullPathLen;
}

PVOID GetObjectByPath(PWSTR pwszObjectName, PVOID pObjectType)
{
  PVOID              pObject = NULL;
  UNICODE_STRING     DeviceName;
  NTSTATUS           NtStatus = 0;
//  ANSI_STRING        FileNameAnsi;

  if (!pwszObjectName || KeGetCurrentIrql() > PASSIVE_LEVEL)
    return pObject;


  RtlInitUnicodeString(&DeviceName, pwszObjectName);

//  RtlUnicodeStringToAnsiString(&FileNameAnsi, &DeviceName, TRUE);
//  if (strlen(FileNameAnsi.Buffer))
//    DbgPrint ("%s\n", FileNameAnsi.Buffer);
//  RtlFreeAnsiString(&FileNameAnsi);


  __try
  {
    NtStatus = ObReferenceObjectByName(&DeviceName, OBJ_CASE_INSENSITIVE, NULL, 0, (POBJECT_TYPE)pObjectType, KernelMode, NULL, (PVOID*)&pObject);
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    NtStatus = STATUS_ACCESS_VIOLATION;
//    DbgPrint ("EXCEPTION into ObReferenceObjectByName\n");
  }
//NTSYSAPI
//NTSTATUS
//NTAPI
//ObReferenceObjectByName(
//    IN PUNICODE_STRING ObjectPath,
//    IN ULONG Attributes,
//    IN PACCESS_STATE PassedAccessState OPTIONAL,
//    IN ACCESS_MASK DesiredAccess OPTIONAL,
//    IN POBJECT_TYPE ObjectType,
//    IN KPROCESSOR_MODE AccessMode,
//    IN OUT PVOID ParseContext OPTIONAL,
//    OUT PVOID *ObjectPtr
//    );  
  if (NT_SUCCESS(NtStatus))
  {
    ObDereferenceObject((PVOID)pObject);
  }
//  DbgPrint ("He4HookInv: GetObjectByPath: pObject = %08X!!!\n", pObject);

  return pObject;
}

POBJECT_NAME GetNameOfObject(PVOID pObject)
{
  POBJECT_NAME    pObjectName = NULL;
  POBJECT_HEADER  pObjectHdr;

  if (_MmIsAddressValid(pObject))
  {
    pObjectHdr = (POBJECT_HEADER) ((PBYTE)pObject - SIZE_OF_OBJECT_HEADER);
    if (_MmIsAddressValid(pObjectHdr))
    {
      if (pObjectHdr->SubHeaderInfo.NameOffset != 0)
      {
        pObjectName = (POBJECT_NAME) ((PBYTE)pObjectHdr - pObjectHdr->SubHeaderInfo.NameOffset);
      }
    }
  }

  return pObjectName;
}

//
// функции для работы с объектами файловых систем
//

BOOLEAN GetFileNameNative(HANDLE hObject, PWSTR lpwszName, ULONG dwSize)
{
  PFILE_OBJECT     pFileObject;
  PDEVICE_OBJECT   pDeviceObject;
  BOOLEAN          bRes = FALSE;
  PFILE_NAME_INFORMATION FileName;

  FileName = (PFILE_NAME_INFORMATION) ExAllocatePool(NonPagedPool, 2*dwSize);
  if (!_MmIsAddressValid(FileName))
    return bRes;

  pFileObject = (PFILE_OBJECT) GetPointer(hObject);
  if (_MmIsAddressValid(pFileObject))
  {
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);
    if (_MmIsAddressValid(pDeviceObject))
    {
      bRes = FilemonQueryFileName(pDeviceObject, pFileObject, FileName, dwSize);
      if (bRes)
      {
        memset(lpwszName, 0, dwSize);
        memcpy(lpwszName, FileName->FileName, FileName->FileNameLength);
      }
    }
    ReleasePointer((POBJECT)pFileObject);
  }

  ExFreePool(FileName);

  return bRes;
}

NTSTATUS FilemonQueryFileNameComplete(PDEVICE_OBJECT DeviceObject,
                                      PIRP Irp,
                                      PVOID Context)
{
  //
  // Copy the status information back into the "user" IOSB.
  //
  *Irp->UserIosb = Irp->IoStatus;

//  if( !NT_SUCCESS(Irp->IoStatus.Status) ) {
//
//      DbgPrint(("   ERROR ON IRP: %x\n", Irp->IoStatus.Status ));
//  }
  
  //
  // Set the user event - wakes up the mainline code doing this.
  //
  KeSetEvent(Irp->UserEvent, 0, FALSE);
  
  //
  // Free the IRP now that we are done with it.
  //
  IoFreeIrp(Irp);
  
  //
  // We return STATUS_MORE_PROCESSING_REQUIRED because this "magic" return value
  // tells the I/O Manager that additional processing will be done by this driver
  // to the IRP - in fact, it might (as it is in this case) already BE done - and
  // the IRP cannot be completed.
  //
  return STATUS_MORE_PROCESSING_REQUIRED;
}


//----------------------------------------------------------------------
//
// FilemonQueryFileName
//
// This function retrieves the "standard" information for the
// underlying file system, asking for the filename in particular.
//
//----------------------------------------------------------------------
BOOLEAN FilemonQueryFileName(PDEVICE_OBJECT DeviceObject, 
                             PFILE_OBJECT FileObject,
                             PFILE_NAME_INFORMATION FileName, ULONG FileNameLength)
{
  PIRP irp;
  KEVENT event;
  IO_STATUS_BLOCK IoStatusBlock;
  PIO_STACK_LOCATION ioStackLocation;

//  DbgPrint(("Getting file name for %x\n", FileObject));

  //
  // Initialize the event
  //
  KeInitializeEvent(&event, SynchronizationEvent, FALSE);

  //
  // Allocate an irp for this request.  This could also come from a 
  // private pool, for instance.
  //
  irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

  if (!irp)
  {
    //
    // Failure!
    //
    return FALSE;
  }
  
  //
  // Build the IRP's main body
  //  
  irp->AssociatedIrp.SystemBuffer = FileName;
  irp->UserEvent = &event;
  irp->UserIosb = &IoStatusBlock;
  irp->Tail.Overlay.Thread = PsGetCurrentThread();
  irp->Tail.Overlay.OriginalFileObject = FileObject;
  irp->RequestorMode = KernelMode;
  irp->Flags = 0;

  //
  // Set up the I/O stack location.
  //
  ioStackLocation = IoGetNextIrpStackLocation(irp);
  ioStackLocation->MajorFunction = IRP_MJ_QUERY_INFORMATION;
  ioStackLocation->DeviceObject = DeviceObject;
  ioStackLocation->FileObject = FileObject;
  ioStackLocation->Parameters.QueryFile.Length = FileNameLength;
  ioStackLocation->Parameters.QueryFile.FileInformationClass = FileNameInformation;

  //
  // Set the completion routine.
  //
  IoSetCompletionRoutine(irp, FilemonQueryFileNameComplete, 0, TRUE, TRUE, TRUE);

  //
  // Send it to the FSD
  //
  (void) IoCallDriver(DeviceObject, irp);

  //
  // Wait for the I/O
  //
  KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, 0);

  //
  // Done! Note that since our completion routine frees the IRP we cannot 
  // touch the IRP now.
  //
  return NT_SUCCESS( IoStatusBlock.Status );
}

NTSTATUS
NativeQueryDirectoryFileComplete(
   PDEVICE_OBJECT DeviceObject,
   PIRP Irp,
   PVOID Context
   )
{
  *Irp->UserIosb = Irp->IoStatus;

  KeSetEvent(Irp->UserEvent, 0, FALSE);
  
  IoFreeIrp(Irp);
  
  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NativeQueryDirectoryFile(
   PDRIVER_DISPATCH pMajorFunction,
   PDEVICE_OBJECT pDeviceObject,
   PFILE_OBJECT pFileObject,
   PIO_STATUS_BLOCK pIoStatusBlock,
   PVOID Buffer,
   ULONG BufferLength,
   FILE_INFORMATION_CLASS DirectoryInfoClass,
   BOOLEAN ByOne,
   PUNICODE_STRING pSearchTemplate,
   BOOLEAN Reset,
   BOOLEAN Index,
   ULONG dwIndex
   )
{
  PIRP irp;
  KEVENT event;
  PIO_STACK_LOCATION ioStackLocation;
  PQUERY_DIRECTORY pQueryDir;

  KeInitializeEvent(&event, SynchronizationEvent, FALSE);

  irp = IoAllocateIrp(pDeviceObject->StackSize, FALSE);
  if (!irp)
    return STATUS_NO_MEMORY;

  irp->MdlAddress = NULL;
  irp->UserBuffer = Buffer;
  irp->AssociatedIrp.SystemBuffer = Buffer;
  irp->UserEvent = &event;
  irp->UserIosb = pIoStatusBlock;
  irp->Tail.Overlay.Thread = PsGetCurrentThread();
  irp->Tail.Overlay.OriginalFileObject = pFileObject;
  irp->RequestorMode = KernelMode;
  irp->Flags = 0;

  ioStackLocation = IoGetNextIrpStackLocation(irp);
  ioStackLocation->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
  ioStackLocation->MinorFunction = IRP_MN_QUERY_DIRECTORY;
  ioStackLocation->DeviceObject = pDeviceObject;
  ioStackLocation->FileObject = pFileObject;
  ioStackLocation->Flags = 0;
  if (ByOne)
    ioStackLocation->Flags |= SL_RETURN_SINGLE_ENTRY;
  if (Reset)
    ioStackLocation->Flags |= SL_RESTART_SCAN;
  if (Index)
    ioStackLocation->Flags |= SL_INDEX_SPECIFIED;

  pQueryDir = (PQUERY_DIRECTORY)&ioStackLocation->Parameters;
  pQueryDir->Length = BufferLength;
  pQueryDir->FileName = pSearchTemplate;
  pQueryDir->FileInformationClass = DirectoryInfoClass;
  pQueryDir->FileIndex = dwIndex;

  IoSetCompletionRoutine(irp, NativeQueryDirectoryFileComplete, 0, TRUE, TRUE, TRUE);

  if (pMajorFunction == NULL)
  {
    (void) IoCallDriver(pDeviceObject, irp);
  }
  else
  {
    --(irp->CurrentLocation);
    if (irp->CurrentLocation <= 0)
    {
      KeBugCheckEx(NO_MORE_IRP_STACK_LOCATIONS, (ULONG)irp, 0, 0, 0);
    }

    --(irp->Tail.Overlay.CurrentStackLocation);

    irp->Tail.Overlay.CurrentStackLocation->DeviceObject = pDeviceObject;
    pMajorFunction(pDeviceObject, irp);
  }

  KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, 0);

  return pIoStatusBlock->Status;
}

PDEVICE_OBJECT GetVolumeDeviceObject(PFILE_OBJECT pFileObject)
{
  PDEVICE_OBJECT pVolumeDeviceObject = NULL;

  if (pFileObject != NULL)
  {
    if (pFileObject->Vpb != NULL)
    {
      pVolumeDeviceObject = pFileObject->Vpb->RealDevice;
    }
    else
    {
      if (pFileObject->DeviceObject != NULL && pFileObject->DeviceObject->Vpb != NULL)
        pVolumeDeviceObject = pFileObject->DeviceObject->Vpb->RealDevice;
    }
  }

  return pVolumeDeviceObject;
}

//
// разное
//

BOOLEAN GetDirectoryFromPath(PWSTR lpwszFullFileName, ULONG dwSize)
{
  ULONG i;

  if (!_MmIsAddressValid(lpwszFullFileName) || !dwSize)
    return FALSE;

  i = dwSize;
  while(1)
  {
    if (i == 0)
      break;
    if (lpwszFullFileName[i] == L'\\')
    {
      lpwszFullFileName[i] = 0;
      break;
    }
    --i;
  }

  return TRUE;
}

BOOLEAN GetDirectoryFromPathA(CHAR *lpszFullFileName, ULONG dwSize)
{
  ULONG i;

  if (!_MmIsAddressValid(lpszFullFileName) || !dwSize)
    return FALSE;

  i = dwSize;
  while(1)
  {
    if (i == 0)
      break;
    if (lpszFullFileName[i] == '\\')
    {
      lpszFullFileName[i] = 0;
      break;
    }
    --i;
  }

  return TRUE;
}

int GetToken(WCHAR *lpInBuf, int dwInBufSize, WCHAR *lpOutBuf, int dwOutBufSize, WCHAR *lpDeliver, int nDeliverCount, int nNumber)
{
  int      Count = 0, k, i, j;
  BOOLEAN  bFlagExit;

  if (!lpOutBuf || !lpInBuf || !lpDeliver)
    return 0;
  *lpOutBuf = 0;
//  memset(lpOutBuf, 0, sizeof(WCHAR)*dwOutBufSize);

  for (i=0; i<dwInBufSize; ++i)
  {
    for (k=0; k<nDeliverCount; ++k)
    {
      if (lpInBuf[i] == lpDeliver[k])
      {
        if (i != 0)
          ++Count;
        if (Count == nNumber)
          ++i;
        break;
      }
    }
    if (Count == nNumber)
    {
      bFlagExit = FALSE;
      for (j=0; j<dwOutBufSize; ++j)
      {
        for (k=0; k<nDeliverCount; ++k)
        {
          if (lpInBuf[i] == lpDeliver[k])
          {
            bFlagExit = TRUE;
            break;
          }
        }
        if (bFlagExit)
          break;
        lpOutBuf[j] = lpInBuf[i];
        ++i;
        if (i >= dwInBufSize) 
        {
          ++j;
          break;
        }
      }
      lpOutBuf[j] = 0;
      return j;
    }
  }

  return 0;
}

NTQUERYDIRECTORYOBJECT GetPtrToZwQueryDirectoryObject(void)
{
  static NTQUERYDIRECTORYOBJECT pFn = NULL;
  LPSSDT                 lpSSDT;
  LPSSD                  lpSSD;
  ULONG                  dwNtBuildNumber;

  if (pFn == NULL)
  {
    lpSSDT = (LPSSDT) KeServiceDescriptorTable;
    lpSSD = &(lpSSDT->SystemServiceDescriptors[0]);
    
    dwNtBuildNumber = (*NtBuildNumber) & 0x00ffffff;
    switch (dwNtBuildNumber)
    {
      case 1381: // WinNT 4.0
           pFn = (NTQUERYDIRECTORYOBJECT) *((PVOID*)(lpSSD->lpSystemServiceTableAddressTable)+0x66);
           break;
      case 2195: // Win2K
           pFn = (NTQUERYDIRECTORYOBJECT) *((PVOID*)(lpSSD->lpSystemServiceTableAddressTable)+0x7e);
           break;
      default:
           if (dwNtBuildNumber >= 1946)
             pFn = (NTQUERYDIRECTORYOBJECT) *((PVOID*)(lpSSD->lpSystemServiceTableAddressTable)+0x7e);
           else
             pFn = NULL;
           break;
    }
  }

  return pFn;
}


//
// оптимизация по скорости
//
SHARED_MEMORY* InitQueryObjectNameType(void)
{
  BOOLEAN               bRes;
  SHARED_MEMORY*        pSharedBuffer = NULL;

  #ifdef __MISC_USE_KHEAP
  pSharedBuffer = (SHARED_MEMORY*)_AllocatePoolFromKHeap(hKHeapMiscDefault, sizeof(SHARED_MEMORY));
  #else
  pSharedBuffer = ExAllocatePool(NonPagedPool, sizeof(SHARED_MEMORY));
  #endif
  if (pSharedBuffer != NULL)
  {
    bRes = AllocateSharedMemory(pSharedBuffer, NonPagedPool, 2048+4+4);
    if (bRes == FALSE)
    {
      #ifdef __MISC_USE_KHEAP
      FreePoolToKHeap(hKHeapMiscDefault, pSharedBuffer);
      #else
      ExFreePool(pSharedBuffer);
      #endif
      pSharedBuffer = NULL;
    }
  }

  return pSharedBuffer;
}

//
// оптимизация по скорости
//
void DeinitQueryObjectNameType(SHARED_MEMORY* pSharedBuffer)
{
  if (_MmIsAddressValid(pSharedBuffer))
  {
    FreeSharedMemory(pSharedBuffer);
    #ifdef __MISC_USE_KHEAP
    FreePoolToKHeap(hKHeapMiscDefault, pSharedBuffer);
    #else
    ExFreePool(pSharedBuffer);
    #endif
  }
}

BOOLEAN QueryObjectNameType(PVOID pObjTypeInfo, ULONG dwSizeOfObjTypeInfo, HANDLE hDir, PWSTR pwszObjectName, SHARED_MEMORY* pSharedBuffer)
{
  CHAR*                  Buf;
  POBJECT_NAMETYPE_INFO  pObjName;
  ULONG*                 pObjectIndex = 0;
  ULONG*                 pLengthReturned = 0;
  ULONG                  Index = 0;
  BOOLEAN                bFirst = TRUE;
  int                    i;
  NTQUERYDIRECTORYOBJECT pZwQueryDirectoryObject = 0;

  if (
      !_MmIsAddressValid(pObjTypeInfo) ||
      !_MmIsAddressValid(pwszObjectName) ||
      !_MmIsAddressValid(pSharedBuffer)
     )
    return FALSE;
     

  pZwQueryDirectoryObject = GetPtrToZwQueryDirectoryObject();
  if (!_MmIsAddressValid(pZwQueryDirectoryObject))
  {
    return FALSE;
  }

  Buf = (CHAR*)pSharedBuffer->m_lpUserMemory;
  pObjName = (POBJECT_NAMETYPE_INFO) Buf;
  pObjectIndex = (ULONG*)(Buf+2048);
  pLengthReturned = (ULONG*)(Buf+2048+4);

  while (pZwQueryDirectoryObject(hDir, Buf, 2048, ObjectArray, bFirst, pObjectIndex, pLengthReturned) >= 0)
  {
    bFirst = FALSE;
    for (i=0; Index<*pObjectIndex; ++Index, ++i)
    {
      if (!_wcsnicmp(pObjName[i].ObjectName.Buffer, pwszObjectName, pObjName[i].ObjectName.Length/sizeof(WCHAR)))
      {
        *pObjectIndex = Index;
        if (pZwQueryDirectoryObject(hDir, Buf, dwSizeOfObjTypeInfo, ObjectByOne, bFirst, pObjectIndex, pLengthReturned) == STATUS_SUCCESS)
        {
          memcpy(pObjTypeInfo, Buf, *pLengthReturned);
          ((POBJECT_NAMETYPE_INFO)pObjTypeInfo)->ObjectName.Buffer = (PWSTR)((DWORD)pObjTypeInfo+((DWORD)(pObjName->ObjectName.Buffer)-(DWORD)pObjName));
          ((POBJECT_NAMETYPE_INFO)pObjTypeInfo)->ObjectType.Buffer = (PWSTR)((DWORD)pObjTypeInfo+((DWORD)(pObjName->ObjectType.Buffer)-(DWORD)pObjName));
          return TRUE;
        }
        return FALSE;
      }
    }
  }
  
  return FALSE;
}

ULONG DosPathNameToNtPathName(PWSTR pwszDosPath, PWSTR pwszNtPath, ULONG dwSizeNtPathByBytes, ULONG dwRecursiveDeep, PULONG pdwObjectSizeByBytes)
{
  ULONG                 dwSizeOfNtPath = 0;
  ULONG                 dwSizeOfDosPath;
  PWSTR                 pwszCurrentDir, pwszPtrCurDir;
  PWSTR                 pwszPtrDosPath, pwszPtr;
  UNICODE_STRING        dirString;
  OBJECT_ATTRIBUTES     dirAttr;
  HANDLE                hDir;
  NTSTATUS              NtStatus;
  char                  Buf[512];
  POBJECT_NAMETYPE_INFO pObjName = (POBJECT_NAMETYPE_INFO) Buf;
  ULONG                 dwRestDosPath, dwFreeCurDir, dwSizeToken;
  ULONG                 _dwSizeNtPathByBytes = dwSizeNtPathByBytes;
  HANDLE                hSymLink;
  UNICODE_STRING        SymLinkString;
  OBJECT_ATTRIBUTES     SymAttr;
  ULONG                 dwSizeSymLinkObj = 0;
  PWSTR                 pwszNewNtPath;
  SHARED_MEMORY*        pSharedBuffer = NULL;

//  return dwSizeOfNtPath;

  DbgPrintMisc(("DosPathNameToNtPathName - start\n"));
 
  if (KeGetCurrentIrql() > PASSIVE_LEVEL)
    return dwSizeOfNtPath;

  if (!_MmIsAddressValid(pwszNtPath))
    return dwSizeOfNtPath;
  memset(pwszNtPath, 0, dwSizeNtPathByBytes);

  dwSizeOfDosPath = wcslen(pwszDosPath);
  if (dwSizeOfDosPath <= 1)
    return dwSizeOfNtPath;

  #ifdef __MISC_USE_KHEAP
  pwszCurrentDir = (PWSTR)_AllocatePoolFromKHeap(hKHeapMiscDefault, sizeof(L"\\??\\")+(dwSizeOfDosPath+1)*sizeof(WCHAR));
  #else
  pwszCurrentDir = ExAllocatePool(NonPagedPool, sizeof(L"\\??\\")+(dwSizeOfDosPath+1)*sizeof(WCHAR));
  #endif
  if(!pwszCurrentDir)
     return dwSizeOfNtPath;

  memset(pwszCurrentDir, 0, sizeof(L"\\??\\")+(dwSizeOfDosPath+1)*sizeof(WCHAR));

  if (pwszDosPath[1] == L':')
    wcscpy(pwszCurrentDir, L"\\??");
  else
    wcscpy(pwszCurrentDir, L"\\");

  wcscpy(pwszNtPath, L"\\");

  pSharedBuffer = InitQueryObjectNameType();  // need for optimize by speed QueryObjectNameType() !!!!!!!

  RtlInitUnicodeString(&dirString, pwszCurrentDir);
  InitializeObjectAttributes(&dirAttr, &dirString, OBJ_CASE_INSENSITIVE, NULL, NULL);
  NtStatus = ZwOpenDirectoryObject(&hDir, DIRECTORY_QUERY, &dirAttr);
  if (NtStatus == STATUS_SUCCESS)
  {
    // хрен его знает - NT понимает оба разделителя '\', '/'
    // однако путь до ядра доходит вроде уже обработанный и все разделители
    // в нем '\'. однако я не уверен - поэтому добавил эту вещь.
    pwszPtr = pwszDosPath;
    while (*pwszPtr)
    {
      if (*pwszPtr == L'/')
        *pwszPtr = L'\\';
      ++pwszPtr;
    }

    if (pwszDosPath[1] == L':')
      wcscat(pwszCurrentDir, L"\\");
    pwszPtrCurDir = pwszCurrentDir + wcslen(pwszCurrentDir);
    pwszPtrDosPath = pwszDosPath;
    if (pwszPtrDosPath[0] == L'\\')
      ++pwszPtrDosPath;
    dwFreeCurDir = ((sizeof(L"\\??\\")/sizeof(WCHAR))+dwSizeOfDosPath+1) - (pwszPtrCurDir - pwszCurrentDir);
    dwRestDosPath = dwSizeOfDosPath - (pwszPtrDosPath - pwszDosPath);
    
    while ((dwSizeToken = GetToken(pwszPtrDosPath, dwRestDosPath,  pwszPtrCurDir, dwFreeCurDir, L"\\", 1, 0)))
    {
      dwFreeCurDir -= dwSizeToken;
      if (dwFreeCurDir != 0)
        --dwFreeCurDir;//sizeof(L'\\');
      dwRestDosPath -= dwSizeToken;
      if (dwRestDosPath != 0)
        --dwRestDosPath;//sizeof(L'\\');

      if (QueryObjectNameType(Buf, sizeof(Buf), hDir, pwszPtrCurDir, pSharedBuffer))
      {
        if (!wcsncmp(pObjName->ObjectType.Buffer, L"Directory", pObjName->ObjectType.Length/sizeof(WCHAR)))
        {
          if (_dwSizeNtPathByBytes < ((wcslen(pObjName->ObjectName.Buffer)+2)*sizeof(WCHAR)))
          {
            dwSizeOfNtPath = 0;
            break;
          }
          wcscat(pwszNtPath, pObjName->ObjectName.Buffer);
          wcscat(pwszNtPath, L"\\");
          _dwSizeNtPathByBytes -= ((wcslen(pObjName->ObjectName.Buffer)+1)*sizeof(WCHAR));

          ZwClose(hDir);
          RtlInitUnicodeString(&dirString, pwszCurrentDir);
          InitializeObjectAttributes(&dirAttr, &dirString, OBJ_CASE_INSENSITIVE, NULL, NULL);
          NtStatus = ZwOpenDirectoryObject(&hDir, DIRECTORY_QUERY, &dirAttr);
          if (NtStatus != STATUS_SUCCESS)
          {
            dwSizeOfNtPath = 0;
            break;
          }
        }
        else
        {
          if (!wcsncmp(pObjName->ObjectType.Buffer, L"SymbolicLink", pObjName->ObjectType.Length/sizeof(WCHAR)))
          {
            dwSizeOfNtPath = 0;

            RtlInitUnicodeString(&SymLinkString, pObjName->ObjectName.Buffer);
            InitializeObjectAttributes(&SymAttr, &SymLinkString, OBJ_CASE_INSENSITIVE, hDir, NULL);
            if (ZwOpenSymbolicLinkObject(&hSymLink, SYMBOLIC_LINK_QUERY, &SymAttr) == STATUS_SUCCESS)
            {
              SymLinkString.Buffer = (PWSTR)pwszNtPath;
              SymLinkString.Length = 0;
              SymLinkString.MaximumLength = (WORD)dwSizeNtPathByBytes - wcslen(pwszPtrDosPath)*sizeof(WCHAR);

              if (ZwQuerySymbolicLinkObject(hSymLink, &SymLinkString, &dwSizeSymLinkObj) == STATUS_SUCCESS)
              {
                if (pwszNtPath[0] == L'\\' && memcmp(pwszDosPath, pwszNtPath, SymLinkString.Length))
                {
                  if (_dwSizeNtPathByBytes < (wcslen(pwszPtrDosPath+dwSizeToken)*sizeof(WCHAR)))
                  {
                    dwSizeOfNtPath = 0;
                    ZwClose(hSymLink);
                    break;
                  }
                  wcscat(pwszNtPath, pwszPtrDosPath+dwSizeToken);
                  #ifdef __MISC_USE_KHEAP
                  pwszNewNtPath = (PWSTR) _AllocatePoolFromKHeap(hKHeapMiscDefault, dwSizeNtPathByBytes);
                  #else
                  pwszNewNtPath= (PWSTR) ExAllocatePool(NonPagedPool, dwSizeNtPathByBytes);
                  #endif
                  if (pwszNewNtPath)
                  {
                    DbgPrintMisc(("DosPathNameToNtPathName - recursive start\n"));
                    if (dwRecursiveDeep != 0)
                    {
                      dwSizeOfNtPath = DosPathNameToNtPathName(pwszNtPath, pwszNewNtPath, dwSizeNtPathByBytes, --dwRecursiveDeep, pdwObjectSizeByBytes);
                      if (dwSizeOfNtPath)
                      {
                        wcscpy(pwszNtPath, pwszNewNtPath);
                      }
                    }
                    #ifdef __MISC_USE_KHEAP
                    FreePoolToKHeap(hKHeapMiscDefault, pwszNewNtPath);
                    #else
                    ExFreePool(pwszNewNtPath);
                    #endif
                  }
                }
              }
              ZwClose(hSymLink);
            }
            break;
          }
          else  // не директория и не ссылка
          {
            if (_MmIsAddressValid(pdwObjectSizeByBytes)) // возвращается длина имени объекта которому принадлежит остаток пути
            {
              *pdwObjectSizeByBytes = wcslen(pwszNtPath)*sizeof(WCHAR);
            }

            wcscat(pwszNtPath, pwszPtrDosPath);
            dwSizeOfNtPath = wcslen(pwszNtPath)*sizeof(WCHAR);
            break;
          }
        }
      }
      else
      {
        dwSizeOfNtPath = 0;
        break;
      }

      wcscat(pwszPtrCurDir, L"\\");
      pwszPtrCurDir += dwSizeToken + 1; // add wcslen(L"\\");
      pwszPtrDosPath += dwSizeToken + 1;
    }
    ZwClose(hDir);
  }

  DeinitQueryObjectNameType(pSharedBuffer);

  #ifdef __MISC_USE_KHEAP
  FreePoolToKHeap(hKHeapMiscDefault, pwszCurrentDir);
  #else
  ExFreePool(pwszCurrentDir);
  #endif

  DbgPrintMisc(("DosPathNameToNtPathName - end\n"));

  return dwSizeOfNtPath;
}

VOID FlushInstuctionCache(VOID)
{
  ULONG                 pNextInstr;

//  __asm _emit 0eah // data into code
  __asm
  {
    push eax
    mov  eax, offset __NextInstr
    mov  [pNextInstr], eax
    pop  eax
    jmp  dword ptr [pNextInstr]
  }
__NextInstr:

  return;
}

//PDEVICE_OBJECT GetOwnDeviceObject(PDEVICE_OBJECT DeviceObject)
//{
//  __asm int 1h
//  while (_MmIsAddressValid(DeviceObject))
//  {
//    if (_MmIsAddressValid(DeviceObject->DeviceObjectExtension))
//    {
//      if (_MmIsAddressValid(DeviceObject->DeviceObjectExtension->DeviceObject))
//      {
//        __asm int 1h
//        if (DeviceObject != DeviceObject->DeviceObjectExtension->DeviceObject)
//          DeviceObject = DeviceObject->DeviceObjectExtension->DeviceObject;
//        else
//          break;
//      }
//      else
//        break;
//    }
//    else
//    {
//      break;
//    }
//  }
//
//  return DeviceObject;
//}

PDEVICE_OBJECT GetOwnDeviceObject(PDEVICE_OBJECT DeviceObject)
{
  PDRIVER_OBJECT pDriverObject;
  PDEVICE_OBJECT pDevice, pDeviceAttached;
  BOOLEAN        bFind = FALSE;

  #if 0
  if (!_MmIsAddressValid(DeviceObject))
    return NULL;

  pDriverObject = DeviceObject->DriverObject;

  if (!_MmIsAddressValid(pDriverObject))
    return NULL;

  pDevice = pDriverObject->DeviceObject;
  while (_MmIsAddressValid(pDevice))
  {
    pDeviceAttached = pDevice;
    while (_MmIsAddressValid(pDeviceAttached))
    {
      if (pDeviceAttached->AttachedDevice == DeviceObject)
      {
        DeviceObject = GetOwnDeviceObject(pDeviceAttached);
        bFind = TRUE;
        break;
      }
      pDeviceAttached = pDeviceAttached->AttachedDevice;
    }
    if (bFind == TRUE)
      break;

    pDevice = pDevice->NextDevice;
  }
  #endif

  if (!DeviceObject)
    return NULL;

  pDriverObject = DeviceObject->DriverObject;

  if (!pDriverObject)
    return NULL;

  pDevice = pDriverObject->DeviceObject;
  while (pDevice)
  {
    pDeviceAttached = pDevice;
    while (pDeviceAttached)
    {
      if (pDeviceAttached->AttachedDevice == DeviceObject)
      {
        DeviceObject = GetOwnDeviceObject(pDeviceAttached);
        bFind = TRUE;
        break;
      }
      pDeviceAttached = pDeviceAttached->AttachedDevice;
    }
    if (bFind == TRUE)
      break;

    pDevice = pDevice->NextDevice;
  }

  return DeviceObject;
}

PDEVICE_OBJECT GetOwnDeviceObjectFromIrp(PIRP pIrp)
{
  PDEVICE_OBJECT      pDevice = NULL;
  CHAR                CurrentLocation;
  PIO_STACK_LOCATION  CurrentIrpStack;

  if (pIrp == NULL)
    return pDevice;

  CurrentLocation = pIrp->CurrentLocation;
  CurrentIrpStack = IoGetCurrentIrpStackLocation(pIrp);

  pDevice = CurrentIrpStack->DeviceObject;
  while (--CurrentLocation)
  {
    --CurrentIrpStack;
    if (CurrentIrpStack->DeviceObject != NULL)
      pDevice = CurrentIrpStack->DeviceObject;
  }

  return pDevice;
}

NTSTATUS DefaultDriverObjectDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_INVALID_DEVICE_REQUEST;
}

PDRIVER_OBJECT CreateInvisibleDriverObject(PVOID pBaseAddress, ULONG dwDriverSize, HANDLE hSystemImage, PDRIVER_INITIALIZE DriverEntry)
{
  NTSTATUS          NtStatus;
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR             DriverName[] = L"\\Driver\\HiddenDriver";
  UNICODE_STRING    UniDriverName;
  PDRIVER_OBJECT    pObjectBody = NULL;
  HANDLE            hDriver;
  int               i;

  RtlInitUnicodeString(&UniDriverName, DriverName);
  InitializeObjectAttributes(&ObjectAttributes, NULL/*&UniDriverName*/, OBJ_PERMANENT, NULL, NULL);

  NtStatus = ObCreateObject(KernelMode, (POBJECT_TYPE)IoDriverObjectType, &ObjectAttributes,   
                            KernelMode, 0, sizeof(DRIVER_OBJECT) + sizeof(DRIVER_EXTENSION),
                            0, 0, (PVOID*)&pObjectBody);
  if (NtStatus != STATUS_SUCCESS)
  {
    DbgPrint ("CreateInvisibleDriverObject: NtStatus = %08x \n", NtStatus);
    return NULL;
  }

  memset(pObjectBody, 0, sizeof(DRIVER_OBJECT) + sizeof(DRIVER_EXTENSION));

  pObjectBody->DriverExtension = (PDRIVER_EXTENSION)((PCHAR)pObjectBody + sizeof(DRIVER_OBJECT));
  pObjectBody->DriverExtension->DriverObject = pObjectBody; // ??????

  pObjectBody->DriverStart = pBaseAddress;
  pObjectBody->DriverSize = dwDriverSize;
  pObjectBody->DriverSection = hSystemImage;

  pObjectBody->DriverInit = DriverEntry;

  for (i = 0; i<IRP_MJ_MAXIMUM_FUNCTION; ++i)
    pObjectBody->MajorFunction[i] = DefaultDriverObjectDispatch;

  pObjectBody->Type = IO_TYPE_DRIVER;
  pObjectBody->Size = sizeof(DRIVER_OBJECT);

  NtStatus = ObInsertObject(pObjectBody, NULL, 1, 0, 0, &hDriver);
  if (NtStatus != STATUS_SUCCESS)
  {
    // delete object
    DbgPrint ("CreateInvisibleDriverObject: NtStatus = %08x \n", NtStatus);
    return NULL;
  }

  return pObjectBody;
}

HANDLE LoadDevice(PWSTR pwszDeviceFileName)
{
  SYSTEM_LOAD_DRIVER  LoadDriverInfo;
  NTSTATUS            NtStatus;

  if (pwszDeviceFileName == NULL)
    return NULL;

  memset(&LoadDriverInfo, 0, sizeof(LoadDriverInfo));

  RtlInitUnicodeString(&LoadDriverInfo.usImageFile, pwszDeviceFileName);

  NtStatus = ZwSetSystemInformation(SystemLoadDriver, &LoadDriverInfo, sizeof(LoadDriverInfo));

  DbgPrint ("Loadevice: \n  pBaseAddress = %08x\n  hSystemImage = %08x\n  pEntryPoint = %08x\n  pDirectoryEntry = %08x\n",
                             LoadDriverInfo.pBaseAddress, LoadDriverInfo.hSystemImage, LoadDriverInfo.pEntryPoint, LoadDriverInfo.pDirectoryEntry);

//  if (NtStatus == STATUS_SUCCESS)
//    *((CHAR*)LoadDriverInfo.pBaseAddress) = 'M';

  return LoadDriverInfo.hSystemImage;
}

NTSTATUS UnloadDevice(HANDLE hSystemImage)
{
  SYSTEM_UNLOAD_DRIVER  UnLoadDriverInfo;
  NTSTATUS              NtStatus;

  UnLoadDriverInfo.hSystemImage = hSystemImage;

  NtStatus = ZwSetSystemInformation(SystemUnloadDriver, &UnLoadDriverInfo, sizeof(UnLoadDriverInfo));

  return NtStatus;
}

BOOLEAN DeleteItemFromQueryDirectoryBuffer(PFQD_SmallCommonBlock pQueryDirPrev, PFQD_SmallCommonBlock pQueryDir, PVOID Buffer, ULONG BufferLength, PIO_STATUS_BLOCK IoStatusBlock, FILE_INFORMATION_CLASS DirectoryInfoClass, NTSTATUS* NtStatus)
{
  BOOLEAN               bRes = FALSE;
  PFQD_SmallCommonBlock pQueryDirSave;
  ULONG                 dwSizeItem;

  if (pQueryDir->NextEntryOffset == 0)
  {
    if (pQueryDir != Buffer)
    {
      pQueryDirPrev->NextEntryOffset = 0;
      if (_MmIsAddressValid(IoStatusBlock))
      {
        dwSizeItem = 0;
        switch (DirectoryInfoClass)
        {
          case 0:
               dwSizeItem = (SIZE_OF_FILE_NAMES_INFORMATION + ((PFILE_NAMES_INFORMATION)pQueryDir)->FileNameLength);
               break;
          case FileDirectoryInformation:
               dwSizeItem = (SIZE_OF_FILE_DIRECTORY_INFORMATION + ((PFILE_DIRECTORY_INFORMATION)pQueryDir)->CommonBlock.FileNameLength);
               break;
          case FileFullDirectoryInformation:
               dwSizeItem = (SIZE_OF_FILE_FULL_DIR_INFORMATION + ((PFILE_FULL_DIR_INFORMATION)pQueryDir)->CommonBlock.FileNameLength);
               break;
          case FileBothDirectoryInformation:
               dwSizeItem = (SIZE_OF_FILE_BOTH_DIR_INFORMATION + ((PFILE_BOTH_DIR_INFORMATION)pQueryDir)->CommonBlock.FileNameLength);
               break;
        }
        if (IoStatusBlock->Information > dwSizeItem)
        {
          IoStatusBlock->Information -= dwSizeItem;
//          memset(pQueryDir, 0, dwSizeItem);
        }
        else
        {
          *NtStatus = STATUS_NO_MORE_FILES;
          IoStatusBlock->Status = STATUS_NO_MORE_FILES;
          IoStatusBlock->Information = 0;
        }
      }
    }
    else
    {
      *NtStatus = STATUS_NO_MORE_FILES;
      if (_MmIsAddressValid(IoStatusBlock))
      {
        IoStatusBlock->Status = STATUS_NO_MORE_FILES;
        IoStatusBlock->Information = 0;
      }
    }
    bRes = TRUE;
  }
  else
  {
    pQueryDirSave = pQueryDir;
    if (_MmIsAddressValid(IoStatusBlock))
    {
      if (IoStatusBlock->Information >= pQueryDir->NextEntryOffset)
        IoStatusBlock->Information -= pQueryDir->NextEntryOffset;
      else
        IoStatusBlock->Information = 0;
    }
    pQueryDir = (PFQD_SmallCommonBlock)((CHAR*)pQueryDir + pQueryDir->NextEntryOffset);
    memmove(pQueryDirSave, pQueryDir, BufferLength - ((CHAR*)pQueryDir-(CHAR*)Buffer));
  }

  return bRes;
}

LPSSDT FindShadowTable(void)
{
  BYTE*       pCheckArea = (BYTE*) KeAddSystemServiceTable;
  int         i;
  PSRVTABLE   pSrvTable = NULL;

  for (i=0; i<100; i++)
  {
    __try
    {
      pSrvTable = *(PSRVTABLE*)pCheckArea;
      if (
          !MmIsAddressValid(pSrvTable)            ||
          (pSrvTable == KeServiceDescriptorTable) ||
          (memcmp(pSrvTable, KeServiceDescriptorTable, sizeof (*pSrvTable)) != 0)
         )
      {
        pCheckArea++;
        pSrvTable = NULL;
      }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      pSrvTable = NULL;
    }
    if (pSrvTable)
      break;
  }

  if (pSrvTable == NULL)
  {
    pSrvTable = (PSRVTABLE)((char*)KeServiceDescriptorTable-0x230);
    if (MmIsAddressValid(pSrvTable))
    {
      __try
      {
        if (memcmp(pSrvTable, KeServiceDescriptorTable, sizeof (*pSrvTable)) != 0)
          pSrvTable = NULL;
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
        pSrvTable = NULL;
      }
    }
    else
    {
      pSrvTable = NULL;
    }
  }

  return (LPSSDT)pSrvTable;
}


#if 0
ULONG GetFullPathbyFileObjectA(PFILE_OBJECT fileObject, PCHAR fullPathName, ULONG nfullPathNameSize)
{
  ULONG               pathLen;
  PCHAR               pathOffset;
  PFILE_OBJECT        relatedFileObject;
  ANSI_STRING         componentName;
  PDEVICE_OBJECT      pDeviceObject;
  PUNICODE_STRING     fullUniName;
  ULONG               dwFreeSize = nfullPathNameSize;

  if (!fullPathName || !nfullPathNameSize)
    return 0;

  fullPathName[0] = 0;

  try
  {
    if (fileObject == 0)
    {
      return 0;
    }

    pDeviceObject = GetVolumeDeviceObject(fileObject);
    if (pDeviceObject == NULL)
    {
      pDeviceObject = IoGetRelatedDeviceObject(fileObject);
    }

    fullUniName = (PUNICODE_STRING)_AllocatePoolFromKHeap(hKHeapDHDefault, nfullPathNameSize*sizeof(WCHAR)+sizeof(UNICODE_STRING));
    fullUniName->MaximumLength = (USHORT)nfullPathNameSize*sizeof(WCHAR);

    if (NT_SUCCESS(ObQueryNameString(pDeviceObject, (POBJECT_NAME_INFORMATION)fullUniName, fullUniName->MaximumLength, &pathLen)))
    {
      RtlUnicodeStringToAnsiString( &componentName, fullUniName, TRUE ); 
      if (componentName.Buffer[0])
      {         
        strncatZ(fullPathName, componentName.Buffer, fullUniName->MaximumLength - 2);
        DbgPrint ("%s\n", fullPathName);
      }
      RtlFreeAnsiString(&componentName);
    }
    else
    {
      return 0;
    }

    if (!fileObject->FileName.Length || fileObject->FileName.Length > nfullPathNameSize)
    {
      return strlen(fullPathName);
    }

//    strcat(fullPathName, "\\");
    dwFreeSize -= strlen(fullPathName);
  }
  except(EXCEPTION_EXECUTE_HANDLER)
  {
    return 0;
  }

  try
  {
    pathLen = fileObject->FileName.Length/sizeof(WCHAR) + 1;

    relatedFileObject = fileObject->RelatedFileObject;
  
    if (fileObject->FileName.Buffer[0] != L'\\' )
    {
      while (relatedFileObject)
      {
        if ((pathLen + relatedFileObject->FileName.Length/sizeof(WCHAR) + 1) >= dwFreeSize)
        {
          break;
        }
        pathLen += relatedFileObject->FileName.Length/sizeof(WCHAR) + 1;
        relatedFileObject = relatedFileObject->RelatedFileObject;
      }
    }

    if (dwFreeSize == nfullPathNameSize)
      sprintf(fullPathName, "\\");
  
    pathOffset = fullPathName + strlen(fullPathName) - sizeof(char) + pathLen - fileObject->FileName.Length/sizeof(WCHAR);
    RtlUnicodeStringToAnsiString( &componentName, &fileObject->FileName, TRUE );    
    strncpy( pathOffset, componentName.Buffer, componentName.Length + 1 );
    RtlFreeAnsiString( &componentName );

    relatedFileObject = fileObject->RelatedFileObject;
  
    if (fileObject->FileName.Buffer[0] != L'\\')
    {
      while (relatedFileObject)
      {
        *(pathOffset - 1) = '\\';
        pathOffset -= relatedFileObject->FileName.Length/sizeof(WCHAR) + 1;

        if (pathOffset <= fullPathName)
        {
          break;
        }
        RtlUnicodeStringToAnsiString(&componentName, &relatedFileObject->FileName, TRUE);
        strncpy(pathOffset, componentName.Buffer, componentName.Length);
        RtlFreeAnsiString(&componentName);

        relatedFileObject = relatedFileObject->RelatedFileObject;
      }
    }  

  }
  except(EXCEPTION_EXECUTE_HANDLER)
  {
     DbgPrint ("He4Hook: GetFullPath: EXCEPTION\n");
  }

//  __asm int 1h
  return strlen(fullPathName);
}
#endif //0