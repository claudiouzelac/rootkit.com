#include "FileSystemHook.h"
#include "../CommonClasses/KInterlockedCounter/KInterlockedCounter.h"

//#define EXCHANGE_FILE_RECURSIVE

extern ULONG dwDeviceState;

KHEAP               hKHeapFSDefault = NULL;

KInterlockedCounter*  FileSystemBusy = NULL;

NTSTATUS (*RealCreateFile)(OUT PHANDLE, IN ACCESS_MASK, IN POBJECT_ATTRIBUTES, OUT PIO_STATUS_BLOCK, \
                           IN PLARGE_INTEGER, IN ULONG, IN ULONG, IN ULONG, IN ULONG, IN PVOID, IN ULONG);
NTSTATUS (*RealOpenFile)(OUT PHANDLE, IN ACCESS_MASK, IN POBJECT_ATTRIBUTES, OUT PIO_STATUS_BLOCK, IN ULONG, IN ULONG);

NTSTATUS (*RealZwQueryDirectoryFile)(IN HANDLE, IN HANDLE, IN PIO_APC_ROUTINE, IN PVOID, OUT PIO_STATUS_BLOCK, OUT PVOID, \
                                   IN ULONG, IN FILE_INFORMATION_CLASS, IN BOOLEAN, IN PUNICODE_STRING, IN BOOLEAN);
//NTSTATUS (*RealClose)(IN HANDLE  Handle);

ULONG dwSaveLowDword_ICF = 0, dwSaveHiDword_ICF = 0;
ULONG dwHookIoCreateFile = (ULONG) HookIoCreateFile;

__declspec(naked) HookDataIoCreateFile(void)
{
  __asm jmp dword ptr [dwHookIoCreateFile];  // mov     ax, 25ffh  ; jmp dword ptr [...]
  __asm nop   // Align
  __asm nop   //
}

#ifdef __HOOK_NTQUERYDIRECTORYFILE
ULONG dwSaveLowDword_NQDF = 0, dwSaveHiDword_NQDF = 0;
ULONG dwHookNtQueryDirectoryFile = (ULONG) HookNtQueryDirectoryFile;

__declspec(naked) HookDataNtQueryDirectoryFile(void)
{
  __asm jmp dword ptr [dwHookNtQueryDirectoryFile];  // mov     ax, 25ffh  ; jmp dword ptr [...]
  __asm nop   // Align
  __asm nop   //
}
#endif //__HOOK_NTQUERYDIRECTORYFILE

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
    )
{
  NTSTATUS              NtStatus;
//  WCHAR                 fullname[MAXPATHLEN];
  WCHAR                *fullname;
  WCHAR                *fullnamedir;
  PFILEINFO             lpFileInfo, lpDirInfo;
  PFILEINFO*            ppDirInfo;
  POBJECT_ATTRIBUTES    lpObjectAttributes;
  PUNICODE_STRING       lpChangeFileNameUnicodeString;
  PWSTR                 lpWstr;
  SHARED_MEMORY         SharedOA, SharedUS, SharedWSTR;
  BOOLEAN               bRes, bPathCorrect = FALSE;
  int                   nFullNameLen;

#ifdef HE4_FS_HOOK_DEBUG
  ANSI_STRING           FileNameAnsi;
  UNICODE_STRING        FileNameUnicodeString;
#endif //HE4_FS_HOOK_DEBUG


  _DbgPrintFs(("IoCreateFile: Started\n"));
//  DbgPrint ("IoCreateFile: Started\n");
  ++FileSystemBusy;

  fullname = (PWSTR)_AllocatePoolFromKHeap(hKHeapFSDefault, MAXPATHLEN*sizeof(WCHAR));
  if (fullname == NULL)
  {
    NtStatus = STATUS_NO_MEMORY;
    --FileSystemBusy;
    return NtStatus;                                                      
  }

  fullname[0] = 0;
  GetObjectNameByObjectAttributes(ObjectAttributes, fullname, sizeof(WCHAR)*MAXPATHLEN);
  _DbgPrintFs(("IoCreateFile: after GetObjectNameByObjectAttributes\n"));

  // Прекратить дальнейшие проверки если открываемый объект не
  // принадлежит файловой системе.
#ifdef __HE4_PROTECT_ONLY_FS_OBJECT // вроде проблемы прекратились - так что нет смысла отсекать запросы к другим объектам
  if (IsNotFileSystemObject(fullname) == TRUE)
  {
    NtStatus = CallRealIoCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize,
                              FileAttributes, ShareAccess, Disposition, CreateOptions, EaBuffer,
                              EaLength, CreateFileType, ExtraCreateParameters, Options);
    --FileSystemBusy;
    FreePoolToKHeap(hKHeapFSDefault, fullname);
    return NtStatus;                                                      
  }
#endif //__HE4_PROTECT_ONLY_FS_OBJECT
  #ifdef HE4_FS_HOOK_DEBUG
  RtlInitUnicodeString(&FileNameUnicodeString, fullname);
  RtlUnicodeStringToAnsiString(&FileNameAnsi, &FileNameUnicodeString, TRUE);
  if(strlen(FileNameAnsi.Buffer))
    {
     DbgPrint ("ZwCreateFile: %s !!!\n", FileNameAnsi.Buffer);
     DbgPrint ("    ZwCreateFile: DesiredAccess = %08X !!!\n", DesiredAccess);
     DbgPrint ("    ZwCreateFile: FileAttributes = %08X !!!\n", FileAttributes);
     DbgPrint ("    ZwCreateFile: ShareAccess = %08X !!!\n", ShareAccess);
     DbgPrint ("    ZwCreateFile: CreateDisposition = %08X !!!\n", Disposition);
     DbgPrint ("    ZwCreateFile: CreateOptions = %08X !!!\n", CreateOptions);
    }
  RtlFreeAnsiString(&FileNameAnsi);
  #endif //HE4_FS_HOOK_DEBUG
 

  // Исправляет путь при открытии дирректории функцией FindFirstFile????
  // В этой ф-ии при сканировании каталога "X:\MyDir", в качестве имени 
  // указывается почему-то "X:\MyDir\*.*". Вполне возможно что "*.*"
  // может изменяться на любой фильтр...
  bPathCorrect = PathCorrection(fullname, DesiredAccess, Disposition, CreateOptions);

  lpFileInfo = NULL;
  lpDirInfo = NULL;

  if (bPathCorrect == FALSE)
    ppDirInfo = &lpDirInfo;
  else
    ppDirInfo = NULL;

  nFullNameLen = wcslen(fullname);
  if (IsFileProtected(hKHeapFSDefault, fullname, &lpFileInfo, ppDirInfo, PsGetCurrentThread(), PsGetCurrentProcess()))
  {
    if (lpFileInfo)
    {
      _DbgPrintFs(("IoCreateFile: FindProtectedFile is TRUE !!!\n"));
//      DbgPrint ("IoCreateFile: lpFileInfo->dwAccessType = %x\n", lpFileInfo->dwAccessType);
      if (lpFileInfo->dwAccessType & FILE_ACC_TYPE_EXCHANGE)
      {
        NtStatus = STATUS_ACCESS_VIOLATION;
        if (
            (CreateOptions & FILE_DIRECTORY_FILE) &&
            (CreateOptions & FILE_OPEN_FOR_BACKUP_INTENT) ||
            (CreateFileType != 0) ||
            (ExtraCreateParameters != 0) ||
            (Options != 0)
           )
        {
          NtStatus = CallRealIoCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize,
                              FileAttributes, ShareAccess, Disposition, CreateOptions, EaBuffer,
                              EaLength, CreateFileType, ExtraCreateParameters, Options);
          --FileSystemBusy;
          _DbgPrintFs(("IoCreateFile: END => ntstatus = %x\n", NtStatus));

          FreePoolToKHeap(hKHeapFSDefault, lpFileInfo); 
          if (lpDirInfo)
            FreePoolToKHeap(hKHeapFSDefault, lpDirInfo); 
          FreePoolToKHeap(hKHeapFSDefault, fullname);
          return NtStatus;
        }
        bRes = AllocateSharedMemory(&SharedOA, NonPagedPool, sizeof(OBJECT_ATTRIBUTES));
        if (bRes)
        {
          bRes = AllocateSharedMemory(&SharedUS, NonPagedPool, sizeof(UNICODE_STRING));
          if (bRes)
          {
            lpWstr = (PWSTR)(lpFileInfo->szNames+lpFileInfo->dwOffsetToUniChangedName);
//            if (lpWstr[lpFileInfo->dwSizeUniChangedName/sizeof(WCHAR)-2] != L'*')
              bRes = AllocateSharedMemory(&SharedWSTR, NonPagedPool, (lpFileInfo->dwSizeUniChangedName));
//            else
//              bRes = AllocateSharedMemory(&SharedWSTR, NonPagedPool, (2 + lpFileInfo->dwSizeUniChangedName + nFullNameLen*sizeof(WCHAR)));
            if (bRes)
            {
//              if (lpWstr[lpFileInfo->dwSizeUniChangedName/sizeof(WCHAR)-2] != L'*')
//              {
                memset(SharedWSTR.m_lpUserMemory, 0, (lpFileInfo->dwSizeUniChangedName));
                memcpy(SharedWSTR.m_lpUserMemory, lpWstr, (lpFileInfo->dwSizeUniChangedName));
//              }
//              else
//              {
//                memset(SharedWSTR.m_lpUserMemory, 0, (2 + lpFileInfo->dwSizeUniChangedName + nFullNameLen*sizeof(WCHAR)));
//                memcpy(SharedWSTR.m_lpUserMemory, lpWstr, (lpFileInfo->dwSizeUniChangedName-2*sizeof(WCHAR)));

//                lpWstr = fullname + (lpFileInfo->dwSizeUniName/sizeof(WCHAR))-2;
//                if (*lpWstr == L'\\')
//                  lpWstr += 1;
//                wcscat((PWSTR)SharedWSTR.m_lpUserMemory, lpWstr);
//              }

              lpObjectAttributes = (POBJECT_ATTRIBUTES) SharedOA.m_lpUserMemory;          
              lpChangeFileNameUnicodeString = (PUNICODE_STRING) SharedUS.m_lpUserMemory;
              lpWstr = (PWSTR) SharedWSTR.m_lpUserMemory;

              RtlInitUnicodeString(lpChangeFileNameUnicodeString, lpWstr);
              InitializeObjectAttributes(lpObjectAttributes, lpChangeFileNameUnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);
     
              #ifdef EXCHANGE_FILE_RECURSIVE
              NtStatus = IoCreateFile(FileHandle, DesiredAccess, lpObjectAttributes, IoStatusBlock, AllocationSize,
                              FileAttributes, ShareAccess, Disposition, CreateOptions, EaBuffer,
                              EaLength, CreateFileType, ExtraCreateParameters, Options);
              #else
              NtStatus = CallRealIoCreateFile(FileHandle, DesiredAccess, lpObjectAttributes, IoStatusBlock, AllocationSize,
                              FileAttributes, ShareAccess, Disposition, CreateOptions, EaBuffer,
                              EaLength, CreateFileType, ExtraCreateParameters, Options);
              #endif //EXCHANGE_FILE_RECURSIVE
//              if(!NT_SUCCESS(NtStatus))
//                {
//                 DbgPrintFs(("HookCreateFile: ERROR => ntstatus = %x\n", NtStatus));
//                }
              FreeSharedMemory(&SharedWSTR);
            }
            FreeSharedMemory(&SharedUS);
          }
          FreeSharedMemory(&SharedOA);
        }
        
        --FileSystemBusy;
        _DbgPrintFs(("IoCreateFile: END => ntstatus = %x\n", NtStatus));

        FreePoolToKHeap(hKHeapFSDefault, lpFileInfo); 
        if (lpDirInfo)
          FreePoolToKHeap(hKHeapFSDefault, lpDirInfo); 
        FreePoolToKHeap(hKHeapFSDefault, fullname);
        return NtStatus;
      }
      else //if (!(lpFileInfo->dwAccessType & FILE_ACC_TYPE_EXCHANGE))
      {
        if (!GetUpdateOptions(lpFileInfo->dwAccessType, &DesiredAccess, &ShareAccess, &Disposition, &CreateOptions))
        {
          NtStatus = STATUS_ACCESS_VIOLATION;
          --FileSystemBusy;
          _DbgPrintFs(("IoCreateFile: END => ntstatus = %x\n", NtStatus));

          FreePoolToKHeap(hKHeapFSDefault, lpFileInfo); 
          if (lpDirInfo)
            FreePoolToKHeap(hKHeapFSDefault, lpDirInfo); 
          FreePoolToKHeap(hKHeapFSDefault, fullname);
          return NtStatus;
        }
      }
      FreePoolToKHeap(hKHeapFSDefault, lpFileInfo); 
    }
    else
    {
      _DbgPrintFs(("IoCreateFile: FindProtectedFile is FALSE !!!\n"));
      NtStatus = STATUS_ACCESS_VIOLATION;
      --FileSystemBusy;

      if (lpDirInfo)
        FreePoolToKHeap(hKHeapFSDefault, lpDirInfo); 
      FreePoolToKHeap(hKHeapFSDefault, fullname);
      return NtStatus;
    }
  }

  //
  //  проверка - можно ли в данном каталоге создавать объекты
  //
  if (lpDirInfo != NULL)
  {
//    DbgPrint ("IoCreateFile: lpDirInfo->dwAccessType = %x\n", lpDirInfo->dwAccessType);
    if (!(lpDirInfo->dwAccessType & FILE_ACC_TYPE_WRITE))
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
        --FileSystemBusy;
        _DbgPrintFs(("IoCreateFile: END => ntstatus = %x\n", NtStatus));
   
        FreePoolToKHeap(hKHeapFSDefault, lpDirInfo); 
        FreePoolToKHeap(hKHeapFSDefault, fullname);
        return NtStatus;
      }
    }
    FreePoolToKHeap(hKHeapFSDefault, lpDirInfo); 
  }

  _DbgPrintFs(("IoCreateFile: before RealIoCreateFile\n"));
  NtStatus = CallRealIoCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize,
                              FileAttributes, ShareAccess, Disposition, CreateOptions, EaBuffer,
                              EaLength, CreateFileType, ExtraCreateParameters, Options);

  FreePoolToKHeap(hKHeapFSDefault, fullname);

  --FileSystemBusy;
  _DbgPrintFs(("IoCreateFile: END => ntstatus = %x\n", NtStatus));

  return NtStatus;                                                      
}

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
    )
{
  NTSTATUS              NtStatus;
  ULONG*                pRealIoCreateFile = (ULONG*) IoCreateFile;
  ULONG*                pHookDataIoCreateFile = (ULONG*) HookDataIoCreateFile;

  __asm cli;

  pRealIoCreateFile[0] = dwSaveLowDword_ICF;
  pRealIoCreateFile[1] = dwSaveHiDword_ICF;

  __asm sti;

  FlushInstuctionCache();

  __try
  {
    NtStatus = IoCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize,
                              FileAttributes, ShareAccess, Disposition, CreateOptions, EaBuffer,
                              EaLength, CreateFileType, ExtraCreateParameters, Options);
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    NtStatus = STATUS_ACCESS_VIOLATION;
  }

  __asm cli;

  pRealIoCreateFile[0] = pHookDataIoCreateFile[0]; // jmp dword ptr [...]
  pRealIoCreateFile[1] = pHookDataIoCreateFile[1];

  __asm sti;

  FlushInstuctionCache();

  return NtStatus;
}

NTSTATUS
HookCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize,               // optional //
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer,                              // optional //
    IN ULONG EaLength)
{
  NTSTATUS NtStatus;

  NtStatus = HookIoCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
                              AllocationSize, FileAttributes, ShareAccess, CreateDisposition,
                              CreateOptions, EaBuffer, EaLength, (CREATE_FILE_TYPE)0, 0, 0);
  return NtStatus;
}

NTSTATUS
HookOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions)
{
  NTSTATUS NtStatus;

  NtStatus = HookIoCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
                              0, 0, ShareAccess, FILE_OPEN,
                              OpenOptions, 0, 0, (CREATE_FILE_TYPE)0, 0, 0);

  return NtStatus;
}


#ifdef __HOOK_NTQUERYDIRECTORYFILE
NTSTATUS
CallRealNtQueryDirectoryFile(
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
    )
{
  NTSTATUS              NtStatus;
  ULONG*                pRealNtQueryDirectoryFile = (ULONG*) NtQueryDirectoryFile;
  ULONG*                pHookDataNtQueryDirectoryFile = (ULONG*) HookDataNtQueryDirectoryFile;

  __asm cli;

  pRealNtQueryDirectoryFile[0] = dwSaveLowDword_NQDF;
  pRealNtQueryDirectoryFile[1] = dwSaveHiDword_NQDF;

  __asm sti;

  FlushInstuctionCache();

  try
  {
    NtStatus = NtQueryDirectoryFile(DirectoryFileHandle, EventHandle, ApcRoutine, ApcContext,
                                    IoStatusBlock, Buffer, BufferLength, DirectoryInfoClass,
                                    ByOne, SearchTemplate, Reset);
  }
  except(EXCEPTION_EXECUTE_HANDLER)
  {
    NtStatus = STATUS_ACCESS_VIOLATION;
  }

  __asm cli;

  pRealNtQueryDirectoryFile[0] = pHookDataNtQueryDirectoryFile[0]; // jmp dword ptr [...]
  pRealNtQueryDirectoryFile[1] = pHookDataNtQueryDirectoryFile[1];

  __asm sti;

  FlushInstuctionCache();

  return NtStatus;
}
#endif //__HOOK_NTQUERYDIRECTORYFILE

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
    )
#else
NTSTATUS
HookZwQueryDirectoryFile(
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
    )
#endif //__HOOK_NTQUERYDIRECTORYFILE
{
  NTSTATUS              NtStatus;
  WCHAR                *wszFileName;
  #define               SIZE_OF_QDF_FILENAME   (1024*sizeof(WCHAR))
  ULONG                 dwFileNameLength, dwObjectNameLen;
  PWSTR                 pFileName, wszAfterPath;
  PFQD_SmallCommonBlock pQueryDirWin32, pQueryDirWin32Prev;
  int                   i;
  PFILEINFO             pFileInfo;
  BOOLEAN               bBreak = FALSE;

  // debug
//  ULONG                 dwRealSize = 0;
//  ULONG                 dwCountFiles = 0;
//  ULONG                 dwSizeOfItem = 0;
//  ANSI_STRING           FileNameAnsi;
//  UNICODE_STRING        FileNameUnicodeString;


  ++FileSystemBusy;

  wszFileName = (PWSTR)_AllocatePoolFromKHeap(hKHeapFSDefault, 2*SIZE_OF_QDF_FILENAME);
  if (wszFileName == NULL)
  {
    NtStatus = STATUS_NO_MEMORY;
    --FileSystemBusy;
    return NtStatus;                                                      
  }

  _DbgPrintFs(("ZwQueryDirectoryFile: DirectoryFileHandle => %x\n", DirectoryFileHandle));

//  DbgPrint ("ZwQueryDirectoryFile\n");
//  DbgPrint (" DirectoryFileHandle = %x\n EventHandle = %x\n DirectoryInfoClass = %x\n ByOne = %x\n", DirectoryFileHandle, EventHandle, DirectoryInfoClass, ByOne);

  #ifndef __HOOK_NTQUERYDIRECTORYFILE
  NtStatus = RealZwQueryDirectoryFile(DirectoryFileHandle, EventHandle, ApcRoutine, ApcContext,
                                    IoStatusBlock, Buffer, BufferLength, DirectoryInfoClass,
                                    ByOne, SearchTemplate, Reset);
  #else
  NtStatus = CallRealNtQueryDirectoryFile(DirectoryFileHandle, EventHandle, ApcRoutine, ApcContext,
                                    IoStatusBlock, Buffer, BufferLength, DirectoryInfoClass,
                                    ByOne, SearchTemplate, Reset);
  #endif //!__HOOK_NTQUERYDIRECTORYFILE

//  if (!NT_SUCCESS(NtStatus))
//    DbgPrint ("ZwQueryDirectoryFile: NtStatus = %08x\n", NtStatus);
//  if (NtStatus == STATUS_PENDING)
//    DbgPrint ("ZwQueryDirectoryFile: NtStatus = STATUS_PENDING\n");

  if (NT_SUCCESS(NtStatus))
  {
//    DbgPrint ("ZwQueryDirectoryFile\n");
//    switch (DirectoryInfoClass)
//    {
//      case 0:
//           dwSizeOfItem = SIZE_OF_FILE_NAMES_INFORMATION;
//           break;
//      case FileDirectoryInformation:
//           dwSizeOfItem = SIZE_OF_FILE_DIRECTORY_INFORMATION;
//           break;
//      case FileFullDirectoryInformation:
//           dwSizeOfItem = SIZE_OF_FILE_FULL_DIR_INFORMATION;
//           break;
//      case FileBothDirectoryInformation:
//           dwSizeOfItem = SIZE_OF_FILE_BOTH_DIR_INFORMATION;
//           break;
//    }
//    DbgPrint (" sizeof(Item) = %u\n", dwSizeOfItem);
//    DbgPrint (" DirectoryFileHandle = %x\n EventHandle = %x\n DirectoryInfoClass = %x\n ByOne = %x\n", DirectoryFileHandle, EventHandle, DirectoryInfoClass, ByOne);
//    DbgPrint (" IoStatusBlock->Information = %u\n", IoStatusBlock->Information);
//    DbgPrint (" BufferLength = %u\n", BufferLength);

    dwObjectNameLen = GetObjectName(DirectoryFileHandle, wszFileName, SIZE_OF_QDF_FILENAME);
    if (ApcRoutine == NULL && dwObjectNameLen != 0)
    {
//      dwObjectNameLen = dwObjectNameLen / sizeof(WCHAR);
      if (wszFileName[(dwObjectNameLen / sizeof(WCHAR))-1] != L'\\')
      {
//        wcscat(wszFileName, L"\\");
        wszFileName[(dwObjectNameLen / sizeof(WCHAR))] = L'\\';
        dwObjectNameLen += sizeof(WCHAR);
        wszFileName[(dwObjectNameLen / sizeof(WCHAR))] = 0;
      }

//      RtlInitUnicodeString(&FileNameUnicodeString, wszFileName);
//      RtlUnicodeStringToAnsiString(&FileNameAnsi, &FileNameUnicodeString, TRUE);
//      if (strlen(FileNameAnsi.Buffer))
//        DbgPrint (" %s\n", FileNameAnsi.Buffer);
//      RtlFreeAnsiString(&FileNameAnsi);


      wszAfterPath = wszFileName + (dwObjectNameLen / sizeof(WCHAR));

      pQueryDirWin32 = (PFQD_SmallCommonBlock) Buffer;
      pQueryDirWin32Prev = pQueryDirWin32;
      if (_MmIsAddressValid(pQueryDirWin32))
      {
        while (1)
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

//          if ((dwFileNameLength >= (SIZE_OF_QDF_FILENAME-dwObjectNameLen-sizeof(WCHAR))))
//            DbgPrint ("ZwQueryDirectoryFile: dwFileNameLength = %u\n", dwFileNameLength);

          if (pFileName != 0)
          {
            if (dwFileNameLength == 2)
            {
              if (*(WCHAR*)pFileName == L'.')
                pFileName = 0;
            }
            else
            {
              if (dwFileNameLength == 4)
              {
                if (*(ULONG*)pFileName == 0x002e002e) // L".." = 0x002e002e
                  pFileName = 0;
              }
            }
          }

          if (pFileName != 0 && (dwFileNameLength < (SIZE_OF_QDF_FILENAME-dwObjectNameLen-sizeof(WCHAR))))
          {
            //i = dwFileNameLength < (SIZE_OF_QDF_FILENAME-dwObjectNameLen-sizeof(WCHAR)) ? dwFileNameLength : (SIZE_OF_QDF_FILENAME-dwObjectNameLen-sizeof(WCHAR));
            i = dwFileNameLength;
            RtlCopyMemory(wszAfterPath, pFileName, i);
            wszAfterPath[i/2] = 0;

            pFileInfo = 0;

            if (IsFileProtected(hKHeapFSDefault, wszFileName, &pFileInfo, NULL, PsGetCurrentThread(), PsGetCurrentProcess()))
            {
              if (pFileInfo)
              {
                if (
                    (pFileInfo->dwAccessType & FILE_ACC_TYPE_EXCHANGE) ||
                    (pFileInfo->dwAccessType & FILE_ACC_TYPE_VISIBLE)
                   )
                {
                  if (pFileInfo->dwAccessType & FILE_ACC_TYPE_EXCHANGE)
                  {
//                    __asm int 1h
//                    if (((PWSTR)(pFileInfo->szNames+pFileInfo->dwOffsetToUniChangedName))[pFileInfo->dwSizeUniChangedName/sizeof(WCHAR)-2] != L'*')
                      FillQueryDirectoryBufferItem(pFileInfo, pQueryDirWin32, DirectoryInfoClass);
                  }
                }
                else
                {
                  if (ByOne)
                  {
                    Reset = FALSE;

                    #ifdef __HOOK_NTQUERYDIRECTORYFILE
                    NtStatus = HookNtQueryDirectoryFile(DirectoryFileHandle, EventHandle, ApcRoutine, ApcContext,
                                                      IoStatusBlock, Buffer, BufferLength, DirectoryInfoClass,
                                                      ByOne, SearchTemplate, Reset);
                    #else
                    NtStatus = HookZwQueryDirectoryFile(DirectoryFileHandle, EventHandle, ApcRoutine, ApcContext,
                                                      IoStatusBlock, Buffer, BufferLength, DirectoryInfoClass,
                                                      ByOne, SearchTemplate, Reset);
                    #endif //__HOOK_NTQUERYDIRECTORYFILE
                    bBreak = TRUE;
                  }
                  else
                  {
                    bBreak = DeleteItemFromQueryDirectoryBuffer(pQueryDirWin32Prev, pQueryDirWin32, Buffer, BufferLength, IoStatusBlock, DirectoryInfoClass, &NtStatus);
                    pQueryDirWin32 = pQueryDirWin32Prev;
                    if (bBreak == FALSE && Buffer == pQueryDirWin32)
                    {
                      FreePoolToKHeap(hKHeapFSDefault, pFileInfo); 
                      continue;
                    }
                  }
                }
                FreePoolToKHeap(hKHeapFSDefault, pFileInfo); 
              } //if (pFileInfo)
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

                #ifndef __HOOK_NTQUERYDIRECTORYFILE
                NtStatus = RealZwQueryDirectoryFile(DirectoryFileHandle, EventHandle, ApcRoutine, ApcContext,
                                                  IoStatusBlock, Buffer, BufferLength, DirectoryInfoClass,
                                                  ByOne, SearchTemplate, Reset);
                #else
                NtStatus = CallRealNtQueryDirectoryFile(DirectoryFileHandle, EventHandle, ApcRoutine, ApcContext,
                                                  IoStatusBlock, Buffer, BufferLength, DirectoryInfoClass,
                                                  ByOne, SearchTemplate, Reset);
                #endif //!__HOOK_NTQUERYDIRECTORYFILE
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
            }
          }
//          else
//          {
//            DbgPrint (" File Name is long pFileName = %08x, dwFileNameLength = %u\n", pFileName, dwFileNameLength);
//          }

//          dwRealSize += pQueryDirWin32->NextEntryOffset;
//          dwCountFiles++;

          if (pQueryDirWin32->NextEntryOffset == 0)
          {
//            dwRealSize += dwSizeOfItem;
            break;
          }
          pQueryDirWin32Prev = pQueryDirWin32;
          pQueryDirWin32 = (PFQD_SmallCommonBlock)((CHAR*)pQueryDirWin32 + pQueryDirWin32->NextEntryOffset);
        }
      }
    }
//    DbgPrint (" dwRealSize = %u\n", dwRealSize);
//    DbgPrint (" dwCountFiles = %u\n", dwCountFiles);
  }

  --FileSystemBusy;

//  _DbgPrintFs(("ZwQueryDirectoryFile: END => ntstatus = %x\n", NtStatus));
//  DbgPrint ("ZwQueryDirectoryFile: END => ntstatus = %x\n", NtStatus);

  FreePoolToKHeap(hKHeapFSDefault, wszFileName);

  return NtStatus;
}

#ifdef __HOOK_NTQUERYDIRECTORYFILE
NTSTATUS
HookZwQueryDirectoryFile(
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
    )
{
  NTSTATUS         NtStatus;

  NtStatus = HookNtQueryDirectoryFile(DirectoryFileHandle, EventHandle, ApcRoutine, ApcContext,
                                    IoStatusBlock, Buffer, BufferLength, DirectoryInfoClass,
                                    ByOne, SearchTemplate, Reset);

  return NtStatus;
}
#endif //__HOOK_NTQUERYDIRECTORYFILE

//NTSTATUS
//HookClose(
//    IN HANDLE  Handle
//    )
//{       
//  NTSTATUS         NtStatus;
//
//  ++FileSystemBusy;
//
////  DbgPrint ("ZwClose: Handle = %x\n", Handle);
//  NtStatus = RealClose(Handle);
//
//  --FileSystemBusy;
//  return NtStatus;
//}

VOID FillQueryDirectoryBufferItem(PFILEINFO pFileInfo, PFQD_SmallCommonBlock pQueryDir, FILE_INFORMATION_CLASS DirectoryInfoClass)
{
  PFQD_SmallCommonBlock pQueryDirWin32;
  ULONG                 QueryDirWin32Length;

  if (
      pFileInfo->dwSizeUniChangedName <= sizeof(WCHAR) ||
      pFileInfo->dwOffsetToUniChangedName == 0
     )
    return;

  QueryDirWin32Length = pFileInfo->dwSizeUniChangedName;
  switch (DirectoryInfoClass)
  {
    case 0:
         return;
    case FileDirectoryInformation:
         QueryDirWin32Length += SIZE_OF_FILE_DIRECTORY_INFORMATION;
         break;
    case FileFullDirectoryInformation:
         QueryDirWin32Length += SIZE_OF_FILE_FULL_DIR_INFORMATION;
         break;
    case FileBothDirectoryInformation:
         QueryDirWin32Length += SIZE_OF_FILE_BOTH_DIR_INFORMATION;
         break;
    default:
         return;
  }
  pQueryDirWin32 = (PFQD_SmallCommonBlock)_AllocatePoolFromKHeap(hKHeapFSDefault, QueryDirWin32Length);
  if (pQueryDirWin32 != NULL)
  {
    if (QueryFileInfo((PWSTR)(pFileInfo->szNames+pFileInfo->dwOffsetToUniChangedName), pFileInfo->dwSizeUniChangedName-sizeof(WCHAR), pQueryDirWin32, QueryDirWin32Length, DirectoryInfoClass))
    {
      memcpy(    
             &(((PFILE_DIRECTORY_INFORMATION)pQueryDir)->CommonBlock.FileAttr),
             &(((PFILE_DIRECTORY_INFORMATION)pQueryDirWin32)->CommonBlock.FileAttr),
             sizeof(FQD_FILE_ATTR)
            );
      switch (DirectoryInfoClass)
      {
//        case FileDirectoryInformation:
//             break;
        case FileFullDirectoryInformation:
             ((PFILE_FULL_DIR_INFORMATION)pQueryDir)->EaSize = ((PFILE_FULL_DIR_INFORMATION)pQueryDirWin32)->EaSize;
             break;
        case FileBothDirectoryInformation:
             ((PFILE_BOTH_DIR_INFORMATION)pQueryDir)->EaSize = ((PFILE_BOTH_DIR_INFORMATION)pQueryDirWin32)->EaSize;
             break;
      }
    }
  }
  FreePoolToKHeap(hKHeapFSDefault, pQueryDirWin32);
}

BOOLEAN QueryFileInfo(PWSTR lpszFileName, ULONG dwSizeFileName, PFQD_SmallCommonBlock lpQueryDirWin32, ULONG dwSizeQueryDirWin32, FILE_INFORMATION_CLASS DirectoryInfoClass)
{
  WORK_QUEUE_ITEM   WorkItem;
  PWSTR             lpszFile;
  QUERY_FILE_INFO   QueryFileInfo;
  PFQD_SmallCommonBlock lpDirInfo;
  BOOLEAN           bRes = FALSE;
  KEVENT            NtQueryDirEvent;

  if (
      !_MmIsAddressValid(lpszFileName) || !dwSizeFileName ||
      !_MmIsAddressValid(lpQueryDirWin32) || !dwSizeQueryDirWin32
     )
    return bRes;

  lpszFile = (PWSTR)_AllocatePoolFromKHeap(hKHeapFSDefault, dwSizeFileName+sizeof(WCHAR));
  if (!lpszFile)
    return bRes;
  memset(lpszFile, 0, dwSizeFileName+sizeof(WCHAR));
  memcpy(lpszFile, lpszFileName, dwSizeFileName);

//  lpDirInfo = _AllocatePoolFromKHeap(hKHeapFSDefault, dwSizeQueryDirWin32);
//  if (!lpDirInfo)
//  {
//    FreePoolToKHeap(hKHeapFSDefault, lpszFile);
//    return bRes;
//  }

  KeInitializeEvent(&NtQueryDirEvent, NotificationEvent, FALSE);

  lpDirInfo = lpQueryDirWin32;

//  memcpy(lpDirInfo, lpQueryDirWin32, dwSizeQueryDirWin32);

  QueryFileInfo.lpszFileName = lpszFile;
  QueryFileInfo.dwSizeFileName = dwSizeFileName;
  QueryFileInfo.lpQueryDirWin32 = lpDirInfo;
  QueryFileInfo.dwSizeQueryDirWin32 = dwSizeQueryDirWin32;
  QueryFileInfo.DirectoryInfoClass = DirectoryInfoClass;
  QueryFileInfo.lpNotifyEvent = &NtQueryDirEvent;
  QueryFileInfo.NtStatus = -1;

  ExInitializeWorkItem(&WorkItem, QueryFileThread, &QueryFileInfo);//&kEvent);
  ExQueueWorkItem(&WorkItem, DelayedWorkQueue);
  KeWaitForSingleObject(&NtQueryDirEvent, Executive, KernelMode, TRUE, NULL);

  if (NT_SUCCESS(QueryFileInfo.NtStatus))
  {
//    memcpy(lpQueryDirWin32, lpDirInfo, dwSizeQueryDirWin32);
    bRes = TRUE;
  }

//  FreePoolToKHeap(hKHeapFSDefault, lpDirInfo);
  FreePoolToKHeap(hKHeapFSDefault, lpszFile);

  return bRes;
}

void QueryFileThread(IN PVOID lpContext)
{
  PQUERY_FILE_INFO   lpQueryFileInfo = (PQUERY_FILE_INFO) lpContext;
  ULONG              i;
  PWSTR              lpFileName = 0;
  PWSTR              lpPathName = 0;
  HANDLE             hDir;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  UNICODE_STRING     DirNameUnicodeString;
  UNICODE_STRING     FileNameUnicodeString;
  IO_STATUS_BLOCK    IoStatusBlock;
//  ULONG              PrevNextEntryOffset;
  ULONG              BufferLength;
  WCHAR              wcSaveSym;

  if (!_MmIsAddressValid(lpQueryFileInfo))
  {
    KeSetEvent(lpQueryFileInfo->lpNotifyEvent, 0, FALSE);
    return;
  }

  lpPathName = lpQueryFileInfo->lpszFileName;
  i = lpQueryFileInfo->dwSizeFileName / sizeof(WCHAR);
  while(i != 0)
  {
    if (lpPathName[i] == L'\\')
    {
      lpFileName = &lpPathName[i+1];
      break;
    }
    i--;
  }
  
  if (!lpFileName)
  {
    KeSetEvent(lpQueryFileInfo->lpNotifyEvent, 0, FALSE);
    return;
  }

//  __asm int 1h
//  if (!GetDirectoryFromPath(lpQueryFileInfo->lpszFileName, lpQueryFileInfo->dwSizeFileName / sizeof(WCHAR)))
//  {
//    KeSetEvent(lpQueryFileInfo->lpNotifyEvent, 0, FALSE);
//    return;
//  }
  wcSaveSym = lpFileName[0];
  lpFileName[0] = 0;

  RtlInitUnicodeString(&DirNameUnicodeString, lpPathName);
  InitializeObjectAttributes(&ObjectAttributes, &DirNameUnicodeString,
                             OBJ_CASE_INSENSITIVE, NULL, NULL);
  lpQueryFileInfo->NtStatus = CallRealIoCreateFile(
                                          &hDir,
                                          FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                          &ObjectAttributes,
                                          &IoStatusBlock,
                                          0,
                                          0,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          FILE_OPEN,
                                          FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                                          0, 0, (CREATE_FILE_TYPE)0, 0, 0);
  lpFileName[0] = wcSaveSym;
  if (!NT_SUCCESS(lpQueryFileInfo->NtStatus))
  {
    KeSetEvent(lpQueryFileInfo->lpNotifyEvent, 0, FALSE);
    return;
  }

//  PrevNextEntryOffset = lpQueryFileInfo->lpQueryDirWin32->NextEntryOffset;
  RtlInitUnicodeString(&FileNameUnicodeString, lpFileName);

  #ifndef __HOOK_NTQUERYDIRECTORYFILE
  lpQueryFileInfo->NtStatus = RealZwQueryDirectoryFile(hDir, 0, NULL, 0, &IoStatusBlock,
                                                     lpQueryFileInfo->lpQueryDirWin32, lpQueryFileInfo->dwSizeQueryDirWin32,
                                                     lpQueryFileInfo->DirectoryInfoClass, TRUE, &FileNameUnicodeString, TRUE
                                                    );
  #else
  lpQueryFileInfo->NtStatus = CallRealNtQueryDirectoryFile(hDir, 0, NULL, 0, &IoStatusBlock,
                                                     lpQueryFileInfo->lpQueryDirWin32, lpQueryFileInfo->dwSizeQueryDirWin32,
                                                     lpQueryFileInfo->DirectoryInfoClass, TRUE, &FileNameUnicodeString, TRUE
                                                    );
  #endif //!__HOOK_NTQUERYDIRECTORYFILE

//  lpQueryFileInfo->lpQueryDirWin32->NextEntryOffset = PrevNextEntryOffset;

  NtClose(hDir);

  KeSetEvent(lpQueryFileInfo->lpNotifyEvent, 0, FALSE);
}

//
// проверка принадлежит ли объект файловой системе.
// по уму надо придумать нормальный способ отсечения
// таких объектов, однако ввиду того что путь до объекта
// Harddisk в WinNt4 и Win2k разный общий метод реализовать
// не получается. Поэтому пока отсекаются запросы к объектам
// типа: \Device\MyDeviceObject. Это нормально работает т.к.
// путь до объекта файловой системы имеет как минимум следующий вид:
// \Device\Harddisk0\Partition0\MyLoveFile - для WinNt4.
//
#ifdef __HE4_PROTECT_ONLY_FS_OBJECT
BOOLEAN IsNotFileSystemObject(PWSTR pwszFullObjectName)
{
  BOOLEAN bRes = FALSE;
  int     nDirLen;

  if (GetDirectoryFromPath(pwszFullObjectName, wcslen(pwszFullObjectName)) != FALSE)
  {
    nDirLen = wcslen(pwszFullObjectName);
    if (wcsncmp(pwszFullObjectName, L"\\Device", nDirLen) == 0)
      bRes = TRUE;
    pwszFullObjectName[nDirLen] = L'\\';
  }

  return bRes;
}
#endif //__HE4_PROTECT_ONLY_FS_OBJECT

//
// Исправляет путь при открытии дирректории функцией FindFirstFile????
// В этой ф-ии при сканировании каталога "X:\MyDir", в качестве имени
// указывается почему-то "X:\MyDir\*.*". Вполне возможно что "*.*"
// может изменяться на любой фильтр...
//
BOOLEAN PathCorrection(PWSTR pwszFullObjectName, ACCESS_MASK DesiredAccess, ULONG Disposition, ULONG Options)
{        
  int     nStrLen;
  int     nStrLenShort;
  int     i;
  PWSTR   fullnamedir;
  BOOLEAN bRes = FALSE;

  if (pwszFullObjectName == NULL)
    return bRes;

  nStrLen = wcslen(pwszFullObjectName);
  if (
      nStrLen>=2 &&
      (
       Options & FILE_DIRECTORY_FILE &&
       Options & FILE_OPEN_FOR_BACKUP_INTENT &&
       DesiredAccess & FILE_LIST_DIRECTORY &&
       Disposition == FILE_OPEN
      )
     )
  {
    bRes = GetDirectoryFromPath(pwszFullObjectName, nStrLen);

//    if (GetDirectoryFromPath(pwszFullObjectName, nStrLen) != FALSE)
//    {
//      nStrLenShort = wcslen(pwszFullObjectName);
//      if ((nStrLen-nStrLenShort) >= 2)
//      {
//        fullnamedir = &pwszFullObjectName[nStrLenShort+1];
//        pwszFullObjectName[nStrLenShort] = L'\\';
//        for (i=0;i<(nStrLen-nStrLenShort);i++)
//        {
//          if (*fullnamedir == L'*' || *fullnamedir == L'?')
//          {
//            pwszFullObjectName[nStrLenShort] = L'\0';
//            bRes = TRUE;
//            break;
//          }
//          fullnamedir++;
//        }
//      }
//    }

  }

  return bRes;
}


//----------------------------------------------------------------------
//
// HookFileSystem
//
// Replaces entries in the system service table with pointers to
// our own hook routines. We save off the real routine addresses.
//
//----------------------------------------------------------------------
VOID HookFileSystem(void)
{
  ULONG*                pRealIoCreateFile = (ULONG*) IoCreateFile;
  ULONG*                pHookDataIoCreateFile = (ULONG*) HookDataIoCreateFile;
  #ifdef __HOOK_NTQUERYDIRECTORYFILE
  ULONG*                pRealNtQueryDirectoryFile = (ULONG*) NtQueryDirectoryFile;
  ULONG*                pHookDataNtQueryDirectoryFile = (ULONG*) HookDataNtQueryDirectoryFile;
  #endif //__HOOK_NTQUERYDIRECTORYFILE

  if (!(dwDeviceState & HE4_STATE_HOOK_FILE_SYSTEM))
  {
    FileSystemBusy = new KInterlockedCounter(0);
    if (FileSystemBusy == NULL)
      return;


//    KeInitializeEvent(&NtQueryDirEvent, NotificationEvent, FALSE);
//    MUTEX_INIT(LockMutexToFileSaveList);
    
    //
    // Hook everything
    //
    __asm cli

    dwSaveLowDword_ICF = pRealIoCreateFile[0];
    dwSaveHiDword_ICF = pRealIoCreateFile[1];

    dwHookIoCreateFile = (ULONG) HookIoCreateFile;
    pRealIoCreateFile[0] = pHookDataIoCreateFile[0]; // jmp dword ptr [...]
    pRealIoCreateFile[1] = pHookDataIoCreateFile[1];

    #ifdef __HOOK_NTQUERYDIRECTORYFILE
    dwSaveLowDword_NQDF = pRealNtQueryDirectoryFile[0];
    dwSaveHiDword_NQDF = pRealNtQueryDirectoryFile[1];

    dwHookNtQueryDirectoryFile = (ULONG) HookNtQueryDirectoryFile;
    pRealNtQueryDirectoryFile[0] = pHookDataNtQueryDirectoryFile[0]; // jmp dword ptr [...]
    pRealNtQueryDirectoryFile[1] = pHookDataNtQueryDirectoryFile[1];
    #endif //__HOOK_NTQUERYDIRECTORYFILE

    RealCreateFile = (NTSTATUS (__stdcall *)(void ** , ULONG, OBJECT_ATTRIBUTES*, IO_STATUS_BLOCK* , LARGE_INTEGER* , ULONG, ULONG, ULONG, ULONG, void*, ULONG)) SYSCALL( ZwCreateFile );
    SYSCALL( ZwCreateFile ) = (PVOID) HookCreateFile;

    RealOpenFile = (NTSTATUS (__stdcall *)(void ** ,ULONG, OBJECT_ATTRIBUTES*, IO_STATUS_BLOCK*, ULONG, ULONG))SYSCALL( ZwOpenFile );
    SYSCALL( ZwOpenFile ) = (PVOID) HookOpenFile;

    RealZwQueryDirectoryFile = (NTSTATUS (__stdcall *)(void*, void*, PIO_APC_ROUTINE, void*, IO_STATUS_BLOCK* , void*, ULONG, FILE_INFORMATION_CLASS, BOOLEAN, UNICODE_STRING*, BOOLEAN))SYSCALL( ZwQueryDirectoryFile );
    SYSCALL( ZwQueryDirectoryFile ) = (PVOID) HookZwQueryDirectoryFile;

//    RealClose = SYSCALL( ZwClose );
//    SYSCALL( ZwClose ) = (PVOID) HookClose;

    __asm sti

    dwDeviceState |= HE4_STATE_HOOK_FILE_SYSTEM;
  }

}


//----------------------------------------------------------------------
//
// UnhookFileSystem
//
// Unhooks all registry routines by replacing the hook addresses in 
// the system service table with the real routine addresses that we
// saved off.
//
//----------------------------------------------------------------------
BOOLEAN UnhookFileSystem(void)
{
  NTSTATUS       NtStatus;
  LARGE_INTEGER  TimeOut;
  BOOLEAN        bRes = FALSE;
  ULONG*         pRealIoCreateFile = (ULONG*) IoCreateFile;
  #ifdef __HOOK_NTQUERYDIRECTORYFILE
  ULONG*         pRealNtQueryDirectoryFile = (ULONG*) NtQueryDirectoryFile;
  #endif //__HOOK_NTQUERYDIRECTORYFILE

  TimeOut.QuadPart = 100;
  if (dwDeviceState & HE4_STATE_HOOK_FILE_SYSTEM)
  {
    //
    // Unhook everything
    //
    if (
           (ULONG) SYSCALL(ZwCreateFile) == (ULONG) HookCreateFile
        && (ULONG) SYSCALL(ZwOpenFile) == (ULONG) HookOpenFile
        && (ULONG) SYSCALL(ZwQueryDirectoryFile) == (ULONG) HookZwQueryDirectoryFile
//        && (ULONG) SYSCALL(ZwClose) == (ULONG) HookClose
       )
    {  
      __asm cli

      SYSCALL( ZwCreateFile ) = (PVOID) RealCreateFile;
      SYSCALL( ZwOpenFile ) = (PVOID) RealOpenFile;
      SYSCALL( ZwQueryDirectoryFile ) = (PVOID) RealZwQueryDirectoryFile;
//      SYSCALL( ZwClose ) = (PVOID) RealClose;

      pRealIoCreateFile[0] = dwSaveLowDword_ICF;
      pRealIoCreateFile[1] = dwSaveHiDword_ICF;

      #ifdef __HOOK_NTQUERYDIRECTORYFILE
      pRealNtQueryDirectoryFile[0] = dwSaveLowDword_NQDF;
      pRealNtQueryDirectoryFile[1] = dwSaveHiDword_NQDF;
      #endif //__HOOK_NTQUERYDIRECTORYFILE

      __asm sti

      if (FileSystemBusy->CompareExchange(0, 0) == TRUE)
      {
        delete FileSystemBusy;
        FileSystemBusy = NULL;
        bRes = TRUE;
        dwDeviceState &= ~HE4_STATE_HOOK_FILE_SYSTEM;
      }
    }
  }
  else
  {
    bRes = TRUE;
  }

  return bRes;
}
