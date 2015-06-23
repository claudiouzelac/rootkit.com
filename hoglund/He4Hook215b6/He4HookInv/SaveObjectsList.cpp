#include "SaveObjectsList.h"
#include "../CommonClasses/KShieldDirectory/KShieldDirectoryTree.h"
#include "../CommonClasses/KDLinkedList/KDLinkedList.h"

KHEAP               hKHeapSOFileList = NULL;
KDLinkedList*       pSaveList = NULL;
KMUTEX              LockMutexToFileSaveList;
KShieldDirectoryTree* pDirTree = NULL;


BOOLEAN GetUpdateOptions(ULONG dwAccessType, PACCESS_MASK lpDesiredAccess, PULONG lpShareAccess, PULONG lpCreateDisposition, PULONG lpCreateOptions)
{
  if (!_MmIsAddressValid(lpDesiredAccess))
    return FALSE;
  if (!_MmIsAddressValid(lpShareAccess))
    return FALSE;
  if (!_MmIsAddressValid(lpCreateOptions))
    return FALSE;

  dwAccessType &= ~FILE_ACC_TYPE_EXCHANGE;

  if (dwAccessType == 0 || dwAccessType == FILE_ACC_TYPE_VISIBLE)
    return FALSE;

//
// FILE_ACC_TYPE_READ
//
  if (
      (dwAccessType & FILE_ACC_TYPE_READ)   &&
      (dwAccessType & FILE_ACC_TYPE_WRITE)  &&
      (dwAccessType & FILE_ACC_TYPE_DELETE)
     )
  {
    return TRUE;
  }
  if (
      (dwAccessType & FILE_ACC_TYPE_READ)   &&
      !(dwAccessType & FILE_ACC_TYPE_WRITE) &&
      !(dwAccessType & FILE_ACC_TYPE_DELETE)
     )
  {
    *lpDesiredAccess &= ~FILE_WRITE_DATA       &
                        ~FILE_WRITE_ATTRIBUTES &
                        ~FILE_WRITE_EA         &
                        ~FILE_APPEND_DATA      &
                        ~GENERIC_WRITE         &
//                        ~FILE_GENERIC_WRITE    & 
                        ~DELETE;
    *lpDesiredAccess |= FILE_READ_ACCESS;
    if (!(*lpShareAccess & FILE_SHARE_READ))
      *lpShareAccess = 0;
    if (_MmIsAddressValid(lpCreateDisposition))
    {
      switch (*lpCreateDisposition)
      {
        case FILE_SUPERSEDE:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OPEN:
             break;
        case FILE_CREATE:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OPEN_IF:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OVERWRITE:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OVERWRITE_IF:
             *lpCreateDisposition = FILE_OPEN;
             break;
//        case FILE_MAXIMUM_DISPOSITION:
//             *lpCreateDisposition = FILE_OPEN;
//             break;
      }
    }
    *lpCreateOptions &= ~FILE_DELETE_ON_CLOSE;
    return TRUE;
  }
  if (
      (dwAccessType & FILE_ACC_TYPE_READ)   &&
      (dwAccessType & FILE_ACC_TYPE_WRITE)  &&
      !(dwAccessType & FILE_ACC_TYPE_DELETE)
     )
  {
    *lpDesiredAccess &= ~FILE_DELETE_CHILD &
                        ~DELETE;
    *lpDesiredAccess |= FILE_READ_ACCESS;
    *lpShareAccess &= ~FILE_SHARE_DELETE;
    *lpCreateOptions &= ~FILE_DELETE_ON_CLOSE;
    return TRUE;
  }
  if (
      (dwAccessType & FILE_ACC_TYPE_READ)   &&
      !(dwAccessType & FILE_ACC_TYPE_WRITE) &&
      (dwAccessType & FILE_ACC_TYPE_DELETE)
     )
  {
    *lpDesiredAccess &= ~FILE_WRITE_DATA       &
                        ~FILE_WRITE_ATTRIBUTES &
                        ~FILE_WRITE_EA         &
                        ~FILE_APPEND_DATA      &
//                        ~FILE_GENERIC_WRITE    &
                        ~GENERIC_WRITE;
    *lpDesiredAccess |= FILE_READ_ACCESS;
    *lpShareAccess &= ~FILE_SHARE_WRITE;
    if (_MmIsAddressValid(lpCreateDisposition))
    {
      switch (*lpCreateDisposition)
      {
        case FILE_SUPERSEDE:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OPEN:
             break;
        case FILE_CREATE:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OPEN_IF:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OVERWRITE:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OVERWRITE_IF:
             *lpCreateDisposition = FILE_OPEN;
             break;
//        case FILE_MAXIMUM_DISPOSITION:
//             *lpCreateDisposition = FILE_OPEN;
//             break;
      }
    }
    return TRUE;
  }
  
//
// FILE_ACC_TYPE_WRITE
//    
  if (
      !(dwAccessType & FILE_ACC_TYPE_READ)  &&
      (dwAccessType & FILE_ACC_TYPE_WRITE)  &&
      !(dwAccessType & FILE_ACC_TYPE_DELETE)
     )
  {
    *lpDesiredAccess &= ~FILE_READ_DATA       &       
                        ~FILE_EXECUTE         &
                        ~FILE_READ_ATTRIBUTES & 
                        ~FILE_READ_EA         &
                        ~GENERIC_READ         &
                        ~GENERIC_EXECUTE      &
                        ~FILE_LIST_DIRECTORY  &
//                        ~FILE_GENERIC_EXECUTE &     // ????
//                        ~FILE_GENERIC_READ    &     // ????
                        ~DELETE;
    *lpDesiredAccess |= FILE_WRITE_ACCESS;
    if (!(*lpShareAccess & FILE_SHARE_WRITE))
      *lpShareAccess = 0;
    *lpCreateOptions &= ~FILE_DELETE_ON_CLOSE;
    return TRUE;
  }
  if (
      !(dwAccessType & FILE_ACC_TYPE_READ)  &&
      (dwAccessType & FILE_ACC_TYPE_WRITE)  &&
      (dwAccessType & FILE_ACC_TYPE_DELETE)
     )
  {
    *lpDesiredAccess &= ~FILE_READ_DATA       &       
                        ~FILE_EXECUTE         &
                        ~FILE_READ_ATTRIBUTES & 
                        ~FILE_READ_EA         &
                        ~GENERIC_READ         &
                        ~GENERIC_EXECUTE      &
//                        ~FILE_GENERIC_EXECUTE &     // ????
//                        ~FILE_GENERIC_READ    &     // ????
                        ~FILE_LIST_DIRECTORY;
    *lpDesiredAccess |= FILE_WRITE_ACCESS;
    *lpShareAccess &= ~FILE_SHARE_READ;
    return TRUE;
  }

//
// FILE_ACC_TYPE_DELETE
//
  if (
      !(dwAccessType & FILE_ACC_TYPE_READ)  &&
      !(dwAccessType & FILE_ACC_TYPE_WRITE) &&
      (dwAccessType & FILE_ACC_TYPE_DELETE)
     )
  {
    *lpDesiredAccess = DELETE;
    *lpShareAccess &= ~FILE_SHARE_DELETE;
    if (_MmIsAddressValid(lpCreateDisposition))
    {
      switch (*lpCreateDisposition)
      {
        case FILE_SUPERSEDE:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OPEN:
             break;
        case FILE_CREATE:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OPEN_IF:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OVERWRITE:
             *lpCreateDisposition = FILE_OPEN;
             break;
        case FILE_OVERWRITE_IF:
             *lpCreateDisposition = FILE_OPEN;
             break;
//        case FILE_MAXIMUM_DISPOSITION:
//             *lpCreateDisposition = FILE_OPEN;
//             break;
      }
    }
    return TRUE;
  }

  return TRUE;
}

BOOLEAN CheckFileInfo(PFILEINFO lpFileInfo)
{
  if (!_MmIsAddressValid(lpFileInfo))
    return FALSE;

  if (lpFileInfo->dwOffsetToAnsiName >= lpFileInfo->dwSizeAllNamesArea)
    return FALSE;
  if (lpFileInfo->dwSizeAnsiName+lpFileInfo->dwOffsetToAnsiName > lpFileInfo->dwSizeAllNamesArea)
    return FALSE;

  if (lpFileInfo->dwOffsetToUniName >= lpFileInfo->dwSizeAllNamesArea)
    return FALSE;
  if (lpFileInfo->dwSizeUniName+lpFileInfo->dwOffsetToUniName > lpFileInfo->dwSizeAllNamesArea)
    return FALSE;

  if (lpFileInfo->dwAccessType & FILE_ACC_TYPE_EXCHANGE)
  {
    if (lpFileInfo->dwOffsetToAnsiChangedName >= lpFileInfo->dwSizeAllNamesArea)
      return FALSE;
    if (lpFileInfo->dwSizeAnsiChangedName+lpFileInfo->dwOffsetToAnsiChangedName > lpFileInfo->dwSizeAllNamesArea)
      return FALSE;

    if (lpFileInfo->dwOffsetToUniChangedName >= lpFileInfo->dwSizeAllNamesArea)
      return FALSE;
    if (lpFileInfo->dwSizeUniChangedName+lpFileInfo->dwOffsetToUniChangedName > lpFileInfo->dwSizeAllNamesArea)
      return FALSE;
  }
                            
  return TRUE;
}

BOOLEAN AddFileSaveList(PFILEINFOSET lpFileInfoSet)
{
  PFILEINFO       lpFileInfo, lpFileInfoIn;
  ULONG           dwSize, dwSizeOfItem, dwSizeOfItemNew;
  PWSTR           pwszNtFileName;
  ULONG           dwSizeOfNtFileName;
  ANSI_STRING     FileNameAnsi;
  UNICODE_STRING  FileNameUni;

  if (!_MmIsAddressValid(lpFileInfoSet))
    return FALSE;
  if (lpFileInfoSet->dwSize < SIZEOF_FILEINFOSET)
    return FALSE;

  MUTEX_WAIT(LockMutexToFileSaveList);

  DbgPrintSO(("He4HookInv: AddFileSaveList: Start!!!\n"));
  if (!_MmIsAddressValid(pSaveList))
  {
    pSaveList = new KDLinkedList(NULL);
    if (pSaveList == NULL)
    {
      DbgPrintSO(("He4HookInv: AddFileSaveList: File list create ERROR!!!\n"));
      MUTEX_RELEASE(LockMutexToFileSaveList);
      return FALSE;
    }
  }

  if (!_MmIsAddressValid(pDirTree))
  {
    pDirTree = new KShieldDirectoryTree();
    if (pDirTree == NULL)
    {
      DbgPrintSO(("He4HookInv: AddFileSaveList: KShieldDirectoryTree create ERROR!!!\n"));
      MUTEX_RELEASE(LockMutexToFileSaveList);
      return FALSE;
    }
  }

  DbgPrintSO(("He4HookInv: AddFileSaveList: List created OK!!!\n"));

  dwSize = lpFileInfoSet->dwSize - (SIZEOF_FILEINFOSET - SIZEOF_FILEINFO);
  lpFileInfoIn = &lpFileInfoSet->FileInfo[0];
  while(dwSize > SIZEOF_FILEINFO)
  {
    if (!CheckFileInfo(lpFileInfoIn))
      break;
    dwSizeOfItem = ((SIZEOF_FILEINFO-sizeof(CHAR)) + lpFileInfoIn->dwSizeAllNamesArea);
    if (lpFileInfoIn->dwSizeAnsiName > sizeof(CHAR))
    {
      pwszNtFileName = (PWSTR)_AllocatePoolFromKHeap(hKHeapSOFileList, sizeof(WCHAR)*lpFileInfoIn->dwSizeAnsiName+2048);
      if (pwszNtFileName)
      {
        RtlInitAnsiString(&FileNameAnsi, lpFileInfoIn->szNames+lpFileInfoIn->dwOffsetToAnsiName);
        RtlAnsiStringToUnicodeString(&FileNameUni, &FileNameAnsi, TRUE);
        dwSizeOfNtFileName = DosPathNameToNtPathName(FileNameUni.Buffer, pwszNtFileName, sizeof(WCHAR)*lpFileInfoIn->dwSizeAnsiName+2048, 255, NULL);
        RtlFreeUnicodeString(&FileNameUni);

        if (dwSizeOfNtFileName)
        {
          dwSizeOfNtFileName += sizeof(WCHAR);
          dwSizeOfItemNew = (SIZEOF_FILEINFO-sizeof(CHAR)) + lpFileInfoIn->dwSizeAnsiName + dwSizeOfNtFileName;
          
          #define ADD_NT_PATH   L"\\??\\"
          //L"\\DosDevices\\"
          //L"\\??\\"
          
          if (lpFileInfoIn->dwAccessType & FILE_ACC_TYPE_EXCHANGE)
          {
            if (lpFileInfoIn->dwSizeAnsiChangedName > sizeof(CHAR))
            {
              dwSizeOfItemNew += sizeof(WCHAR)*lpFileInfoIn->dwSizeAnsiChangedName + sizeof(ADD_NT_PATH)-sizeof(WCHAR);
            }
            else
            {
              if (lpFileInfoIn->dwSizeUniChangedName > sizeof(WCHAR))
                dwSizeOfItemNew += lpFileInfoIn->dwSizeUniChangedName;
            }
          }
          
          lpFileInfo = (PFILEINFO)_AllocatePoolFromKHeap(hKHeapSOFileList, dwSizeOfItemNew+sizeof(WCHAR));
          if (lpFileInfo)
          {
            memset(lpFileInfo, 0, dwSizeOfItemNew+sizeof(WCHAR));
            lpFileInfo->dwAccessType = lpFileInfoIn->dwAccessType;
            lpFileInfo->dwSizeAllNamesArea = dwSizeOfItemNew - (SIZEOF_FILEINFO-sizeof(CHAR));
          
            lpFileInfo->dwOffsetToAnsiName = 0;
            lpFileInfo->dwSizeAnsiName = lpFileInfoIn->dwSizeAnsiName;
          
            RtlCopyMemory(lpFileInfo->szNames+lpFileInfo->dwOffsetToAnsiName,
                          lpFileInfoIn->szNames+lpFileInfoIn->dwOffsetToAnsiName,
                          lpFileInfo->dwSizeAnsiName);
          
            lpFileInfo->dwOffsetToUniName = lpFileInfo->dwOffsetToAnsiName + lpFileInfo->dwSizeAnsiName;
            lpFileInfo->dwSizeUniName = dwSizeOfNtFileName;
          
            RtlCopyMemory(lpFileInfo->szNames+lpFileInfo->dwOffsetToUniName, pwszNtFileName, dwSizeOfNtFileName);
          
            if (lpFileInfoIn->dwAccessType & FILE_ACC_TYPE_EXCHANGE)
            {
              if (lpFileInfoIn->dwSizeAnsiChangedName > sizeof(CHAR))
              {
                lpFileInfo->dwOffsetToUniChangedName = lpFileInfo->dwOffsetToUniName + lpFileInfo->dwSizeUniName;
                lpFileInfo->dwSizeUniChangedName = sizeof(WCHAR)*lpFileInfoIn->dwSizeAnsiChangedName+sizeof(ADD_NT_PATH)-sizeof(WCHAR);
          
                RtlInitAnsiString(&FileNameAnsi, lpFileInfoIn->szNames+lpFileInfoIn->dwOffsetToAnsiChangedName);
                RtlAnsiStringToUnicodeString(&FileNameUni, &FileNameAnsi, TRUE);
          
                RtlCopyMemory(lpFileInfo->szNames+lpFileInfo->dwOffsetToUniChangedName, ADD_NT_PATH, sizeof(ADD_NT_PATH));
                RtlCopyMemory((lpFileInfo->szNames+lpFileInfo->dwOffsetToUniChangedName+sizeof(ADD_NT_PATH)-sizeof(WCHAR)), FileNameUni.Buffer,
                              (sizeof(WCHAR)*lpFileInfoIn->dwSizeAnsiChangedName) < (FileNameUni.Length+sizeof(WCHAR)) ? (sizeof(WCHAR)*lpFileInfoIn->dwSizeAnsiChangedName) : (FileNameUni.Length+sizeof(WCHAR)));
                RtlFreeUnicodeString(&FileNameUni);
              }
              else
              {
                if (lpFileInfoIn->dwSizeUniChangedName > sizeof(WCHAR))
                {
                  lpFileInfo->dwOffsetToUniChangedName = lpFileInfo->dwOffsetToUniName + lpFileInfo->dwSizeUniName;
                  lpFileInfo->dwSizeUniChangedName = lpFileInfoIn->dwSizeUniChangedName;
                
                  RtlCopyMemory(
                                lpFileInfo->szNames+lpFileInfo->dwOffsetToUniChangedName,
                                lpFileInfoIn->szNames+lpFileInfoIn->dwOffsetToUniChangedName,
                                lpFileInfo->dwSizeUniChangedName
                               );
                }
                else
                {
                  lpFileInfo->dwAccessType &= ~FILE_ACC_TYPE_EXCHANGE;
                }
              }
            }

            PVOID pContext;
            if (pDirTree->Find((PWSTR)(lpFileInfo->szNames+lpFileInfo->dwOffsetToUniName), &pContext) == NULL)
            {
              DbgPrintSO(("He4HookInv: AddFileSaveList: %s, type = %x\n", lpFileInfo->szNames+lpFileInfo->dwOffsetToAnsiName, lpFileInfo->dwAccessType));
              if (pDirTree->Add((PWSTR)(lpFileInfo->szNames+lpFileInfo->dwOffsetToUniName), (PVOID)lpFileInfo) == TRUE)
              {
                if (pSaveList->AddTailObject(lpFileInfo) == FALSE)
                {
                  pDirTree->Remove((PWSTR)(lpFileInfo->szNames+lpFileInfo->dwOffsetToUniName), NULL);
                  FreePoolToKHeap(hKHeapSOFileList, lpFileInfo); 
                }
              }
              else
              {
                FreePoolToKHeap(hKHeapSOFileList, lpFileInfo); 
              }
            }
            else
            {
              FreePoolToKHeap(hKHeapSOFileList, lpFileInfo); 
            }
          }
        }
        FreePoolToKHeap(hKHeapSOFileList, pwszNtFileName);
      }
    }
    dwSize -= dwSizeOfItem;
    lpFileInfoIn = (PFILEINFO)((PCHAR)lpFileInfoIn + dwSizeOfItem);
  }

  MUTEX_RELEASE(LockMutexToFileSaveList);

  return TRUE;
}

BOOLEAN DelFileSaveList(PFILEINFOSET lpFileInfoSet)
{
  ULONG           dwSize, dwSizeOfItem;
  CHAR           *lpszStr;
  KDLinkedListItem* lpNode;
  PFILEINFO       lpFileInfo, lpFileInfoIn;

  if (!_MmIsAddressValid(pSaveList))
    return FALSE;
  if (!_MmIsAddressValid(pDirTree))
    return FALSE;
  if (!_MmIsAddressValid(lpFileInfoSet))
    return FALSE;
  if (lpFileInfoSet->dwSize < SIZEOF_FILEINFOSET)
    return FALSE;

  MUTEX_WAIT(LockMutexToFileSaveList);

  dwSize = lpFileInfoSet->dwSize - (SIZEOF_FILEINFOSET - SIZEOF_FILEINFO);
  lpFileInfoIn = &lpFileInfoSet->FileInfo[0];
  while (dwSize > SIZEOF_FILEINFO)
  {
    if (!CheckFileInfo(lpFileInfoIn))
      break;
    dwSizeOfItem = ((SIZEOF_FILEINFO-sizeof(CHAR)) + lpFileInfoIn->dwSizeAllNamesArea);
    if (lpFileInfoIn->dwSizeAnsiName > sizeof(CHAR))
    {
      lpNode = pSaveList->GetHead();
      while (_MmIsAddressValid(lpNode))
      {
        lpFileInfo = (PFILEINFO)lpNode->GetObject();
        if (!_MmIsAddressValid(lpFileInfo))
        {
          lpNode = lpNode->GetNext();
          continue;
        }

        lpszStr = lpFileInfoIn->szNames+lpFileInfoIn->dwOffsetToAnsiName;
        
        if (!CompareString(lpFileInfo->szNames+lpFileInfo->dwOffsetToAnsiName, lpszStr, TRUE))
        {
          DbgPrintSO(("He4HookInv: DelFileSaveList: Start delete - %s\n", lpszStr));
          pDirTree->Remove((PWSTR)(lpFileInfo->szNames+lpFileInfo->dwOffsetToUniName), NULL);
          pSaveList->Remove(lpNode);
          lpNode = pSaveList->GetHead();
          DbgPrintSO(("He4HookInv: DelFileSaveList: End delete - %s\n", lpszStr));
      
          FreePoolToKHeap(hKHeapSOFileList, lpFileInfo); 
          continue;
        }
        
        lpNode = lpNode->GetNext();
      }
    }
    dwSize -= dwSizeOfItem;
    lpFileInfoIn = (PFILEINFO)((PCHAR)lpFileInfoIn + dwSizeOfItem);
  }


  MUTEX_RELEASE(LockMutexToFileSaveList);

  return TRUE;
}

//
// возвращает размер занятой списком файлов памяти в байтах
//
BOOLEAN GetFileListSizeByBytes(PULONG pdwSize)
{
  BOOLEAN         bRes = FALSE;
  KDLinkedListItem* pNode;
  PFILEINFO       pFileInfo;

  if (
      KeGetCurrentIrql() == PASSIVE_LEVEL &&
      _MmIsAddressValid(pdwSize) &&
      _MmIsAddressValid(pSaveList)
     )
  {
    *pdwSize = 0;

    MUTEX_WAIT(LockMutexToFileSaveList);

    pNode = pSaveList->GetHead();
    while (_MmIsAddressValid(pNode))
    {
      pFileInfo = (PFILEINFO)pNode->GetObject();
      if (_MmIsAddressValid(pFileInfo))
      {
        *pdwSize += pFileInfo->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL;
      }
      pNode = pNode->GetNext();
    }

    MUTEX_RELEASE(LockMutexToFileSaveList);

    bRes = TRUE;
  }

  return bRes;
}

BOOLEAN GetFileSaveList(PFILEINFOSET pFileInfoSet)
{           
  BOOLEAN         bRes = FALSE;
  KDLinkedListItem* pNode;
  PFILEINFO       pFileInfo, pFileInfoDest;
  ULONG           dwSize;

  if (
      KeGetCurrentIrql() == PASSIVE_LEVEL &&
      _MmIsAddressValid(pSaveList) &&
      _MmIsAddressValid(pFileInfoSet)  &&
      pFileInfoSet->dwSize >= SIZEOF_FILEINFOSET
     )
  {
    dwSize = pFileInfoSet->dwSize;
    memset(pFileInfoSet, 0, dwSize);
    dwSize -= sizeof(ULONG);
    pFileInfoDest = &pFileInfoSet->FileInfo[0];

    MUTEX_WAIT(LockMutexToFileSaveList);

    pNode = pSaveList->GetHead();
    while (_MmIsAddressValid(pNode))
    {
      pFileInfo = (PFILEINFO)pNode->GetObject();
      if (_MmIsAddressValid(pFileInfo))
      {
        if (dwSize >= (pFileInfo->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL))
        {
          memcpy(pFileInfoDest, pFileInfo, (pFileInfo->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL));
//          if (dwSize >= (pFileInfo->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL))
            dwSize -= (pFileInfo->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL);
//          else
//            dwSize = 0;
          pFileInfoDest = (PFILEINFO)((CHAR*)pFileInfoDest + (pFileInfo->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL));
          pFileInfoSet->dwSize += (pFileInfo->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL);
        }
      }
      pNode = pNode->GetNext();
    }

    MUTEX_RELEASE(LockMutexToFileSaveList);

    if (pFileInfoSet->dwSize != 0)
      pFileInfoSet->dwSize += sizeof(ULONG);

    bRes = TRUE;
  }

  return bRes;
}

PFILEINFO FindProtectedFile(PWSTR lpwszFileName, PFILEINFO* ppDirInfo)
{
  PFILEINFO       lpFileInfo = NULL;

  DbgPrintSO(("He4HookInv: FindProtectedFile: Start!!!\n"));

  if (
      KeGetCurrentIrql() > PASSIVE_LEVEL ||
      !_MmIsAddressValid(pDirTree) ||
      !_MmIsAddressValid(lpwszFileName)
     )
  {
    return NULL;
  }
  
//  MUTEX_WAIT(LockMutexToFileSaveList);

  __try
  {
    pDirTree->FindMatchRest(lpwszFileName, (PVOID*) &lpFileInfo, (PVOID*) ppDirInfo);
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    DbgPrintSO(("He4HookInv: FindProtectedFile: EXCEPTION\n"));
  }

//  MUTEX_RELEASE(LockMutexToFileSaveList);

  DbgPrintSO(("He4HookInv: FindProtectedFile: lpFileInfo = %08x\n", lpFileInfo));
  return lpFileInfo;
}

//
// эта ф-я выделяет память в той куче которая ей передается.
// вызывающий эту ф-ю должен освободить память после пользавания 
// из той кучи в которой память взята!!!!
//
BOOLEAN IsFileProtected(KHEAP hKHeap, PWSTR lpwszFileName, PFILEINFO* lppFileInfo, PFILEINFO* lppDirInfo, PETHREAD pCurrentThread, PEPROCESS pCurrentProcess)
{
  PFILEINFO           lpFileInfo, lpFileInfoNew, lpDirInfoNew;
  PUNLOCK_CLIENT_INFO lpUnlockClientInfo = NULL;
  BOOLEAN             bRes = FALSE;
  ULONG               dwSize;

  DbgPrintSO(("He4HookInv: IsFileProtected: Start\n"));

  *lppFileInfo = NULL;

  if (lppDirInfo != NULL)
  {
    *lppDirInfo = NULL;
  }

  lpFileInfo = FindProtectedFile(lpwszFileName, lppDirInfo);

  DbgPrintSO(("He4HookInv: IsFileProtected: before end - %08x\n", lpFileInfo));

  if (lpFileInfo)
  {
    lpFileInfoNew = (PFILEINFO)_AllocatePoolFromKHeap(hKHeap, lpFileInfo->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL);
    if (lpFileInfoNew != NULL)
    {
      memcpy(lpFileInfoNew, lpFileInfo, lpFileInfo->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL);
      *lppFileInfo = lpFileInfoNew;
      bRes = TRUE;
      lpUnlockClientInfo = FindUnlockInfo(pCurrentThread, pCurrentProcess); //(PsGetCurrentThread(), PsGetCurrentProcess());
      if (lpUnlockClientInfo != NULL)
      {
        lpFileInfoNew->dwAccessType |= (lpUnlockClientInfo->m_dwUnlockFlags & ~HE4_UNLOCK_FOR_PROCESS);
        lpFileInfoNew->dwAccessType &= ~FILE_ACC_TYPE_EXCHANGE;
        if ((lpFileInfoNew->dwAccessType & (FILE_ACC_TYPE_FULL)) == (FILE_ACC_TYPE_FULL))
        {
          bRes = FALSE;
          FreePoolToKHeap(hKHeap, lpFileInfoNew); 
        }
      }
    }
  }

  if (lppDirInfo != NULL && *lppDirInfo != NULL)
  {
    lpDirInfoNew = (PFILEINFO)_AllocatePoolFromKHeap(hKHeap, (*lppDirInfo)->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL);
    if (lpDirInfoNew != NULL)
    {
      memcpy(lpDirInfoNew, *lppDirInfo, (*lppDirInfo)->dwSizeAllNamesArea + SIZEOF_FILEINFO_REAL);
      *lppDirInfo = lpDirInfoNew;

      if (lpFileInfo == NULL)
        lpUnlockClientInfo = FindUnlockInfo(pCurrentThread, pCurrentProcess); 

      if (lpUnlockClientInfo != NULL)
      {
        lpDirInfoNew->dwAccessType |= (lpUnlockClientInfo->m_dwUnlockFlags & ~HE4_UNLOCK_FOR_PROCESS);
        lpDirInfoNew->dwAccessType &= ~FILE_ACC_TYPE_EXCHANGE;
        if ((lpDirInfoNew->dwAccessType & (FILE_ACC_TYPE_FULL)) == (FILE_ACC_TYPE_FULL))
        {
          FreePoolToKHeap(hKHeap, lpDirInfoNew); 
          *lppDirInfo = NULL;
        }
      }
    }
  }

  return bRes;
}

BOOLEAN IsFileProtectedLight(KHEAP hKHeap, PWSTR lpwszFileName, ULONG* pdwFileAccessType, ULONG* pdwDirAccessType, PETHREAD pCurrentThread, PEPROCESS pCurrentProcess)
{
  PFILEINFO           lpFileInfo, lpDirInfo;
  PFILEINFO*          lppDirInfo = NULL;
  PUNLOCK_CLIENT_INFO lpUnlockClientInfo = NULL;
  BOOLEAN             bRes = FALSE;
  ULONG               dwSize;

  DbgPrintSO(("He4HookInv: IsFileProtectedLight: Start\n"));

  *pdwFileAccessType = 0;

  if (pdwDirAccessType != NULL)
  {
    *pdwDirAccessType = FILE_ACC_TYPE_FULL;
    lppDirInfo = &lpDirInfo;
    lpDirInfo = NULL;
  }

  lpFileInfo = FindProtectedFile(lpwszFileName, lppDirInfo);

  DbgPrintSO(("He4HookInv: IsFileProtectedLight: before end - %08x\n", lpFileInfo));

  if (lpFileInfo)
  {
    *pdwFileAccessType = lpFileInfo->dwAccessType;
    bRes = TRUE;
    lpUnlockClientInfo = FindUnlockInfo(pCurrentThread, pCurrentProcess); //(PsGetCurrentThread(), PsGetCurrentProcess());
    if (lpUnlockClientInfo != NULL)
    {
      *pdwFileAccessType |= (lpUnlockClientInfo->m_dwUnlockFlags & ~HE4_UNLOCK_FOR_PROCESS);
      *pdwFileAccessType &= ~FILE_ACC_TYPE_EXCHANGE;
      if ((*pdwFileAccessType & (FILE_ACC_TYPE_FULL)) == (FILE_ACC_TYPE_FULL))
      {
        bRes = FALSE;
      }
    }
  }

  if (lppDirInfo != NULL && *lppDirInfo != NULL)
  {
    *pdwDirAccessType = lpDirInfo->dwAccessType;

    if (lpFileInfo == NULL)
      lpUnlockClientInfo = FindUnlockInfo(pCurrentThread, pCurrentProcess); 
    if (lpUnlockClientInfo != NULL)
    {
      *pdwDirAccessType |= (lpUnlockClientInfo->m_dwUnlockFlags & ~HE4_UNLOCK_FOR_PROCESS);
      *pdwDirAccessType &= ~FILE_ACC_TYPE_EXCHANGE;
    }
  }

  return bRes;
}

//******************************************************************//
//********************* List destroy functions *********************//
//******************************************************************//
void ClearSaveFileList(void)
{
  PFILEINFO       lpFileInfo;
  KDLinkedListItem* lpNode;

  __try
  {
    MUTEX_WAIT(LockMutexToFileSaveList);

    if (_MmIsAddressValid(pDirTree))
      delete pDirTree;
    pDirTree = NULL;
    
    if (_MmIsAddressValid(pSaveList))
    {
      lpNode = pSaveList->GetHead();
      while (_MmIsAddressValid(lpNode))
      {
        lpFileInfo = (PFILEINFO)pSaveList->RemoveHead();
        if (_MmIsAddressValid(lpFileInfo))
        {
          FreePoolToKHeap(hKHeapSOFileList, lpFileInfo); 
        }
        lpNode = pSaveList->GetHead();
      }
      delete pSaveList;
      pSaveList = NULL;
    }

    MUTEX_RELEASE(LockMutexToFileSaveList);
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    MUTEX_RELEASE(LockMutexToFileSaveList);
    DbgPrintSO(("He4HookInv: ClearSaveFileList: EXCEPTION\n"));
  }
}
