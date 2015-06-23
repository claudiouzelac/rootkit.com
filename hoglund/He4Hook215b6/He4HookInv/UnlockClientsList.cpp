#include "UnlockClientsList.h"
#include "../CommonClasses/KDLinkedList/KDLinkedList.h"

KDLinkedList*       pUnlockList = NULL;
KMUTEX              LockMutexToUnlockList;
KHEAP               hKHeapUnlockList = NULL;

//******************************************************************//
//************     List unlock threads functions    ****************//
//******************************************************************//
BOOLEAN AddUnlockList(ULONG dwClientId, PVOID lpCCB, ULONG dwUnlockFlags)
{
  PUNLOCK_CLIENT_INFO  lpUnlockClientInfo;

  DbgPrintUnlock(("He4Hook: AddUnlockList: Start !\n"));

  lpUnlockClientInfo = (PUNLOCK_CLIENT_INFO)_AllocatePoolFromKHeap(hKHeapUnlockList, sizeof(UNLOCK_CLIENT_INFO));
  if (!lpUnlockClientInfo)
    return FALSE;
  lpUnlockClientInfo->m_dwClientId = dwClientId;
  lpUnlockClientInfo->m_pCCB = lpCCB;
  lpUnlockClientInfo->m_dwUnlockFlags = dwUnlockFlags;


  DbgPrintUnlock(("He4Hook: AddUnlockList: lpUnlockThreadInfo alloc OK !\n"));

  MUTEX_WAIT(LockMutexToUnlockList);
  if (!_MmIsAddressValid(pUnlockList))
  {
    pUnlockList = new KDLinkedList(NULL);
    if (pUnlockList == NULL)
    {
      DbgPrintUnlock(("He4Hook: AddUnlockList: Threads list create ERROR!!!\n"));
      FreePoolToKHeap(hKHeapUnlockList, lpUnlockClientInfo); 
      MUTEX_RELEASE(LockMutexToUnlockList);
      return FALSE;
    }
  }

  DbgPrintUnlock(("He4Hook: AddUnlockList: ListCreate OK !\n"));

  DelUnlockList(dwClientId, lpCCB);

  if (pUnlockList->AddTailObject((PVOID)lpUnlockClientInfo) == FALSE)
    FreePoolToKHeap(hKHeapUnlockList, lpUnlockClientInfo); 

  MUTEX_RELEASE(LockMutexToUnlockList);

  return TRUE;
}

BOOLEAN DelUnlockList(ULONG dwClientId, PVOID lpCCB)
{
  KDLinkedListItem*    lpNode;
  PUNLOCK_CLIENT_INFO  lpUnlockClientInfo;

  if (!_MmIsAddressValid(pUnlockList))
    return FALSE;

  MUTEX_WAIT(LockMutexToUnlockList);

  lpNode = pUnlockList->GetHead();
  while (_MmIsAddressValid(lpNode))
  {
    lpUnlockClientInfo = (PUNLOCK_CLIENT_INFO)lpNode->GetObject();
    if (_MmIsAddressValid(lpUnlockClientInfo))
    {
      if (lpUnlockClientInfo->m_dwClientId == dwClientId)
      {
        if (lpUnlockClientInfo->m_pCCB == lpCCB || lpCCB == HE4_UNLOCK_CLIENT_UNKNOWN)
        {
          pUnlockList->Remove(lpNode);
          FreePoolToKHeap(hKHeapUnlockList, lpUnlockClientInfo); 
          lpNode = pUnlockList->GetHead();
          continue;
        }
      }
    }
    lpNode = lpNode->GetNext();
  }
  MUTEX_RELEASE(LockMutexToUnlockList);
  return TRUE;
}

PUNLOCK_CLIENT_INFO FindUnlockInfo(PETHREAD lpTCB, PEPROCESS lpPCB)
{
  KDLinkedListItem*    lpNode;
  PUNLOCK_CLIENT_INFO  lpUnlockClientInfo,
                       lpUnlockThreadInfo = NULL, lpUnlockProcessInfo = NULL;

  if (
      KeGetCurrentIrql() > PASSIVE_LEVEL ||
      !_MmIsAddressValid(pUnlockList)
     )
    return NULL;

  MUTEX_WAIT(LockMutexToUnlockList);

  lpNode = pUnlockList->GetHead();
  while (_MmIsAddressValid(lpNode))
  {
    lpUnlockClientInfo = (PUNLOCK_CLIENT_INFO)lpNode->GetObject();
    if (_MmIsAddressValid(lpUnlockClientInfo))
    {
      if (lpUnlockClientInfo->m_pCCB == lpTCB && !(lpUnlockClientInfo->m_dwUnlockFlags & HE4_UNLOCK_FOR_PROCESS))
      {
        lpUnlockThreadInfo = lpUnlockClientInfo;
        break;
      }
      else
      {
        if (lpUnlockClientInfo->m_pCCB == lpPCB && (lpUnlockClientInfo->m_dwUnlockFlags & HE4_UNLOCK_FOR_PROCESS))
        {
          lpUnlockProcessInfo = lpUnlockClientInfo;
        }
      }

      if (lpUnlockProcessInfo != NULL && lpUnlockThreadInfo != NULL)
        break;
    }
    lpNode = lpNode->GetNext();
  }
  MUTEX_RELEASE(LockMutexToUnlockList);

  return (lpUnlockThreadInfo == NULL) ? lpUnlockProcessInfo : lpUnlockThreadInfo;
}

void ClearUnlockList(void)
{
  PUNLOCK_CLIENT_INFO lpUnlockInfo;
  KDLinkedListItem*   lpNode;

  __try
  {
    if (_MmIsAddressValid(pUnlockList))
    {
      MUTEX_WAIT(LockMutexToUnlockList);

      lpNode = pUnlockList->GetHead();
      while (_MmIsAddressValid(lpNode))
      {
        lpUnlockInfo = (PUNLOCK_CLIENT_INFO)pUnlockList->RemoveHead();
        if (_MmIsAddressValid(lpUnlockInfo))
        {
          FreePoolToKHeap(hKHeapUnlockList, lpUnlockInfo); 
        }
        lpNode = pUnlockList->GetHead();
      }
      delete pUnlockList;
      pUnlockList = NULL;
      MUTEX_RELEASE(LockMutexToUnlockList);
    }
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    MUTEX_RELEASE(LockMutexToUnlockList);
    DbgPrintUnlock(("He4Hook: ClearUnlockList: EXCEPTION\n"));
  }
}

BOOLEAN GetUnlockListSizeByBytes(ULONG* pdwSize)
{
  BOOLEAN             bRes = FALSE;
  PUNLOCK_CLIENT_INFO lpUnlockInfo;
  KDLinkedListItem*   lpNode;

  if (
      KeGetCurrentIrql() == PASSIVE_LEVEL &&
      _MmIsAddressValid(pdwSize) &&
      _MmIsAddressValid(pUnlockList)
     )
  {
    *pdwSize = 0;

    MUTEX_WAIT(LockMutexToUnlockList);

    lpNode = pUnlockList->GetHead();
    while (_MmIsAddressValid(lpNode))
    {
      lpUnlockInfo = (PUNLOCK_CLIENT_INFO)lpNode->GetObject();
      if (_MmIsAddressValid(lpUnlockInfo))
      {
        *pdwSize += sizeof(UNLOCK_CLIENT_INFO);
      }
      lpNode = lpNode->GetNext();
    }

    MUTEX_RELEASE(LockMutexToUnlockList);

    bRes = TRUE;
  }

  return bRes;
}

BOOLEAN GetUnlockList(PUNLOCK_CLIENT_INFO_SET pUnlockInfoSet)
{
  BOOLEAN             bRes = FALSE;
  PUNLOCK_CLIENT_INFO lpUnlockInfo, pUnlockInfoDest;
  KDLinkedListItem*   lpNode;
  ULONG               dwSize;

  if (
      KeGetCurrentIrql() == PASSIVE_LEVEL &&
      _MmIsAddressValid(pUnlockList) &&
      _MmIsAddressValid(pUnlockInfoSet)  &&
      pUnlockInfoSet->m_dwSize >= SIZEOF_UNLOCK_CLIENT_INFO_SET
     )
  {
    MUTEX_WAIT(LockMutexToUnlockList);

    dwSize = pUnlockInfoSet->m_dwSize;
    memset(pUnlockInfoSet, 0, dwSize);
    dwSize -= sizeof(ULONG);
    pUnlockInfoDest = &pUnlockInfoSet->m_CI[0];

    lpNode = pUnlockList->GetHead();
    while (_MmIsAddressValid(lpNode))
    {
      lpUnlockInfo = (PUNLOCK_CLIENT_INFO)lpNode->GetObject();
      if (_MmIsAddressValid(lpUnlockInfo))
      {
        if (dwSize >= sizeof(UNLOCK_CLIENT_INFO))
        {
          memcpy(pUnlockInfoDest, lpUnlockInfo, sizeof(UNLOCK_CLIENT_INFO));
          dwSize -= sizeof(UNLOCK_CLIENT_INFO);
          pUnlockInfoDest++;
          pUnlockInfoSet->m_dwSize += sizeof(UNLOCK_CLIENT_INFO);
        }
      }
      lpNode = lpNode->GetNext();
    }

    MUTEX_RELEASE(LockMutexToUnlockList);

    if (pUnlockInfoSet->m_dwSize != 0)
      pUnlockInfoSet->m_dwSize += sizeof(ULONG);

    bRes = TRUE;
  }

  return bRes;
}
