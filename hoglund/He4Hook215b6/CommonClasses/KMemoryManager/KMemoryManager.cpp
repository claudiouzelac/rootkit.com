//
// Данный код можно использовать на IRQL <= DISPATCH_LEVEL,
// если __KHEAP_LOCK_MUTEX не определен, иначе IRQL == PASSIVE_LEVEL
//

#include "KMemoryManager.h"


//
// нижеследующие ф-ии _только_ для внутреннего использования MemoryManager-ом.
//
static PMEMORY_BLOCK KHeapGetFreeBlock(KHEAP hKHeap, ULONG dwSize); // Optimize for min(fragmentation_heap) - private method

static PVOID    KHeapAllocateMemory(ULONG dwSize);         // private method
static VOID     KHeapFreeMemory(PVOID pArray);             // private method

#ifdef __KHEAP_LOCK_MUTEX
static VOID     KHeapLock(KHEAP hKHeap);                   // private method
static VOID     KHeapUnLock(KHEAP hKHeap);                 // private method
#else
static VOID     KHeapLock(KHEAP hKHeap, PKIRQL pOldIrql);  // private method
static VOID     KHeapUnLock(KHEAP hKHeap, KIRQL OldIrql);  // private method
#endif

static BOOLEAN  KHeapIsAddressValid(PVOID Address);        // private method


static KHEAP hKHeapGlobalDefault = NULL;

KHEAP KHeapCreate(ULONG dwSize)
{
  KHEAP           hKHeap = NULL;
  PMEMORY_BLOCK   pBlockInfo;

  if (dwSize != 0)
  {
    dwSize = ((dwSize+0xfff) & 0xfffff000); // Выравнивание на 4096 байт
    hKHeap = (KHEAP) KHeapAllocateMemory(SIZEOF_MEMORY_BLOCK_REAL + dwSize);
    if (hKHeap != NULL)
    {
      hKHeap->m_dwStates |= BLOCK_STATE_IN_USE | BLOCK_STATE_KHEAP;
      hKHeap->m_dwSize = dwSize;

      pBlockInfo = (PMEMORY_BLOCK) hKHeap->m_pArray;
      pBlockInfo->m_pBaseBlock = hKHeap;

      #ifdef __KHEAP_WIN32
      hKHeap->m_Lock = KHeapAllocateMemory(sizeof(CRITICAL_SECTION));
      if (hKHeap->m_Lock != NULL)
        InitializeCriticalSection(hKHeap->m_Lock);
      #else
      #ifndef __KHEAP_LOCK_MUTEX
      KeInitializeSpinLock(&hKHeap->m_Lock);
      #else
      hKHeap->m_Lock = KHeapAllocateMemory(sizeof(KMUTEX));
      if (hKHeap->m_Lock != NULL)
        KeInitializeMutex(hKHeap->m_Lock, 0);
      else
      {
        KHeapFreeMemory(hKHeap);
        hKHeap = NULL;
      }
      #endif //__KHEAP_LOCK_MUTEX
      #endif //__KHEAP_WIN32
    }
  }

  return hKHeap;
}

BOOLEAN KHeapDestroy(KHEAP hKHeap)
{
  KHEAP hKHeapLast = hKHeap;
  KHEAP hKHeapTmp;

  if (!KHeapIsAddressValid(hKHeap) || !(hKHeap->m_dwStates & BLOCK_STATE_KHEAP))
    return FALSE;
  
  while (hKHeapLast->m_pNextBlock != NULL)
    hKHeapLast = hKHeapLast->m_pNextBlock;

  while (hKHeapLast->m_pBaseBlock != NULL)
  {
    hKHeapTmp = hKHeapLast;
    hKHeapLast = hKHeapLast->m_pBaseBlock;
    #ifdef __KHEAP_WIN32
    if (hKHeapTmp->m_Lock != NULL)
    {
      DeleteCriticalSection(hKHeapTmp->m_Lock);
      KHeapFreeMemory(hKHeapTmp->m_Lock);
    }
    #else
    #ifdef __KHEAP_LOCK_MUTEX
    if (hKHeapTmp->m_Lock != NULL)
      KHeapFreeMemory(hKHeapTmp->m_Lock);
    #endif //__KHEAP_LOCK_MUTEX
    #endif
    KHeapFreeMemory(hKHeapTmp);
  }

  #ifdef __KHEAP_WIN32
  if (hKHeapLast->m_Lock != NULL)
  {
    DeleteCriticalSection(hKHeapLast->m_Lock);
    KHeapFreeMemory(hKHeapLast->m_Lock);
  }
  #else
  #ifdef __KHEAP_LOCK_MUTEX
  if (hKHeapLast->m_Lock != NULL)
    KHeapFreeMemory(hKHeapLast->m_Lock);
  #endif //__KHEAP_LOCK_MUTEX
  #endif
  KHeapFreeMemory(hKHeapLast);

  return TRUE;
}

BOOLEAN CreateDefaultHeap(ULONG dwSize)
{
  if (hKHeapGlobalDefault != NULL)
    DestroyDefaultHeap();

  hKHeapGlobalDefault = KHeapCreate(dwSize);

  return (hKHeapGlobalDefault != NULL);
}

VOID DestroyDefaultHeap(VOID)
{
  KHeapDestroy(hKHeapGlobalDefault);
  hKHeapGlobalDefault = NULL;
}

KHEAP KGetDefaultHeap(VOID)
{
  return hKHeapGlobalDefault;
}

PVOID AllocatePoolFromKHeap(KHEAP hKHeap, ULONG dwSize)
{
  ULONG           dwRealSize;
  PVOID           pArray = NULL;
  PMEMORY_BLOCK   pBlockInfo, pAllocBlock = NULL, pBaseBlock;
  KHEAP           hKHeapNew;
  PCHAR           pStartFreeMem;
  #ifndef __KHEAP_WIN32
  #ifndef __KHEAP_LOCK_MUTEX
  KIRQL           OldIrqlForLock;
  #endif //__KHEAP_LOCK_MUTEX
  #endif //!__KHEAP_WIN32

  if (hKHeap == NULL)
    hKHeap = hKHeapGlobalDefault;

  if (dwSize == 0 || !KHeapIsAddressValid(hKHeap))
    return pArray;

  dwSize = ((dwSize + 7) & 0xfffffff8);
  dwRealSize = dwSize + SIZEOF_MEMORY_BLOCK_REAL;

  #ifdef __KHEAP_LOCK_MUTEX
  KHeapLock(hKHeap);
  #else
  KHeapLock(hKHeap, &OldIrqlForLock);
  #endif //__KHEAP_LOCK_MUTEX

  pBlockInfo = (PMEMORY_BLOCK)hKHeap->m_pArray;

  while (pBlockInfo != NULL)
  {
    if (pBlockInfo->m_dwStates & BLOCK_STATE_IN_USE)
      pStartFreeMem = pBlockInfo->m_pArray + pBlockInfo->m_dwSize;
    else
      pStartFreeMem = (char*)pBlockInfo;
    if (pBlockInfo->m_pNextBlock != NULL)
    {
      if (
          (ULONG)((char*)pBlockInfo->m_pNextBlock - pStartFreeMem) >= dwRealSize
         )
      {
        pAllocBlock = (PMEMORY_BLOCK) pStartFreeMem;
        pAllocBlock->m_pNextBlock = pBlockInfo->m_pNextBlock;
        if (pStartFreeMem != (char*)pBlockInfo)
          pBlockInfo->m_pNextBlock = pAllocBlock;
        memset(pAllocBlock->m_pArray, 0, dwSize);

        pAllocBlock->m_pBaseBlock = hKHeap;

        pAllocBlock->m_dwStates = BLOCK_STATE_IN_USE;
        pAllocBlock->m_dwSize = dwSize;
        pArray = pAllocBlock->m_pArray;
        break;
      }
    }
    else
    {
      if (pBlockInfo->m_pBaseBlock != NULL)
      {
        pBaseBlock = pBlockInfo->m_pBaseBlock;
        if (
            (ULONG)((pBaseBlock->m_pArray + pBaseBlock->m_dwSize) - pStartFreeMem) >= dwRealSize
           )
        {
          pAllocBlock = (PMEMORY_BLOCK) pStartFreeMem;
          if (pStartFreeMem != (char*)pBlockInfo)
            pBlockInfo->m_pNextBlock = pAllocBlock;
          memset(pAllocBlock, 0, dwRealSize);
   
          pAllocBlock->m_pBaseBlock = hKHeap;
   
          pAllocBlock->m_dwStates |= BLOCK_STATE_IN_USE;
          pAllocBlock->m_dwSize = dwSize;
          pArray = pAllocBlock->m_pArray;
        }
        else
        {
          if (pBaseBlock->m_pNextBlock != NULL)
            pArray = AllocatePoolFromKHeap(pBaseBlock->m_pNextBlock, dwSize);
          else
          {
            hKHeapNew = KHeapCreate(dwRealSize > pBaseBlock->m_dwSize ? dwRealSize : pBaseBlock->m_dwSize);
            if (hKHeapNew != NULL)
            {
              pBaseBlock->m_pNextBlock = hKHeapNew;
              hKHeapNew->m_pBaseBlock = pBaseBlock;
              pArray = AllocatePoolFromKHeap(pBaseBlock->m_pNextBlock, dwSize);
              if (pArray == NULL)
              {
                KHeapDestroy(pBaseBlock->m_pNextBlock);
                pBaseBlock->m_pNextBlock = NULL;
              }
            }
          }
        }
      }
      break;
    }

    pBlockInfo = pBlockInfo->m_pNextBlock;
  }

  #ifdef __KHEAP_LOCK_MUTEX
  KHeapUnLock(hKHeap);
  #else
  KHeapUnLock(hKHeap, OldIrqlForLock);
  #endif //__KHEAP_LOCK_MUTEX

  return pArray;
}

PVOID _AllocatePoolFromKHeap(KHEAP hKHeap, ULONG dwSize)
{
  ULONG           dwRealSize;
  PVOID           pArray = NULL;
  PMEMORY_BLOCK   pAllocBlock = NULL;
  KHEAP           hKHeapNew;
  KHEAP           hKHeapAlloc;
  #ifndef __KHEAP_WIN32
  #ifndef __KHEAP_LOCK_MUTEX
  KIRQL           OldIrqlForLock;
  #endif //__KHEAP_LOCK_MUTEX
  #endif //!__KHEAP_WIN32

  if (hKHeap == NULL)
    hKHeap = hKHeapGlobalDefault;

  if (dwSize == 0 || !KHeapIsAddressValid(hKHeap))
    return pArray;

  #ifdef __KHEAP_LOCK_MUTEX
  KHeapLock(hKHeap);
  #else
  KHeapLock(hKHeap, &OldIrqlForLock);
  #endif //__KHEAP_LOCK_MUTEX

  pAllocBlock = KHeapGetFreeBlock(hKHeap, dwSize);
  if (pAllocBlock)
  {
    pArray = pAllocBlock->m_pArray;
  }
  else
  {
    hKHeapAlloc = hKHeap;
    while (hKHeapAlloc->m_pNextBlock)
      hKHeapAlloc = hKHeapAlloc->m_pNextBlock;

    dwRealSize = ((dwSize + 7) & 0xfffffff8) + SIZEOF_MEMORY_BLOCK_REAL;

    hKHeapNew = KHeapCreate(dwRealSize > hKHeap->m_dwSize ? dwRealSize : hKHeap->m_dwSize);
    if (hKHeapNew != NULL)
    {
      hKHeapAlloc->m_pNextBlock = hKHeapNew;
      hKHeapNew->m_pBaseBlock = hKHeapAlloc;
      pArray = _AllocatePoolFromKHeap(hKHeapAlloc->m_pNextBlock, dwSize);
      if (pArray == NULL)
      {
        KHeapDestroy(hKHeapAlloc->m_pNextBlock);
        hKHeapAlloc->m_pNextBlock = NULL;
      }
    }
  }

  #ifdef __KHEAP_LOCK_MUTEX
  KHeapUnLock(hKHeap);
  #else
  KHeapUnLock(hKHeap, OldIrqlForLock);
  #endif //__KHEAP_LOCK_MUTEX

  return pArray;
}


VOID FreePoolToKHeap(KHEAP hKHeap, PVOID pArray)
{
  PMEMORY_BLOCK   pBlockInfo;
  #ifndef __KHEAP_WIN32
  #ifndef __KHEAP_LOCK_MUTEX
  KIRQL           OldIrqlForLock;
  #endif //__KHEAP_LOCK_MUTEX
  #endif //!__KHEAP_WIN32
  
  if (hKHeap == NULL)
    hKHeap = hKHeapGlobalDefault;

  if (!KHeapIsAddressValid(hKHeap) || !KHeapIsAddressValid(pArray))
    return;

  #ifdef __KHEAP_LOCK_MUTEX
  KHeapLock(hKHeap);
  #else
  KHeapLock(hKHeap, &OldIrqlForLock);
  #endif //__KHEAP_LOCK_MUTEX

  pBlockInfo = (PMEMORY_BLOCK) ((PCHAR)pArray - SIZEOF_MEMORY_BLOCK_REAL);
  pBlockInfo->m_dwStates &= ~BLOCK_STATE_IN_USE;

  #ifdef __KHEAP_LOCK_MUTEX
  KHeapUnLock(hKHeap);
  #else
  KHeapUnLock(hKHeap, OldIrqlForLock);
  #endif //__KHEAP_LOCK_MUTEX
}

PMEMORY_BLOCK KHeapGetFreeBlock(KHEAP hKHeap, ULONG dwSize)
{
  PMEMORY_BLOCK   pBlockInfo, pBaseBlock;
  PMEMORY_BLOCK   pAllocBlock = NULL, pBlockInfoSave = NULL;
  ULONG           dwSizeFreeBlockPrev = 0xffffffff;
  KHEAP           hKHeapAlloc;
  ULONG           dwRealSize;
  PCHAR           pStartFreeMem;
  ULONG           dwSizeFreeBlock;

  dwSize = ((dwSize + 7) & 0xfffffff8);
  dwRealSize = dwSize + SIZEOF_MEMORY_BLOCK_REAL;

  while (hKHeap)
  {
    pBlockInfo = (PMEMORY_BLOCK)hKHeap->m_pArray;
    while (pBlockInfo)
    {
      if (pBlockInfo->m_dwStates & BLOCK_STATE_IN_USE)
        pStartFreeMem = pBlockInfo->m_pArray + pBlockInfo->m_dwSize;
      else
        pStartFreeMem = (char*)pBlockInfo;
      if (pBlockInfo->m_pNextBlock != NULL)
      {
        dwSizeFreeBlock = (ULONG)((char*)pBlockInfo->m_pNextBlock - pStartFreeMem);
        if (dwSizeFreeBlock >= dwRealSize)
        {
          if (dwSizeFreeBlock < dwSizeFreeBlockPrev)
          {
            dwSizeFreeBlockPrev = dwSizeFreeBlock;
            pAllocBlock = (PMEMORY_BLOCK) pStartFreeMem;
            hKHeapAlloc = hKHeap;
            if (pStartFreeMem != (char*)pBlockInfo)
              pBlockInfoSave = pBlockInfo;
            else
              pBlockInfoSave = NULL;
          }
        }
      }
      else
      {
        if (pBlockInfo->m_pBaseBlock != NULL)
        {
          pBaseBlock = pBlockInfo->m_pBaseBlock;
          dwSizeFreeBlock = (ULONG)((pBaseBlock->m_pArray + pBaseBlock->m_dwSize) - pStartFreeMem);
          if (dwSizeFreeBlock >= dwRealSize)
          {
            if (dwSizeFreeBlock < dwSizeFreeBlockPrev)
            {
              dwSizeFreeBlockPrev = dwSizeFreeBlock;
              pAllocBlock = (PMEMORY_BLOCK) pStartFreeMem;
              hKHeapAlloc = hKHeap;
              if (pStartFreeMem != (char*)pBlockInfo)
                pBlockInfoSave = pBlockInfo;
              else
                pBlockInfoSave = NULL;
            }
          }
        }
        break;
      }
      pBlockInfo = pBlockInfo->m_pNextBlock;
    }
    hKHeap = (KHEAP) hKHeap->m_pNextBlock;
  }

  if (pAllocBlock != NULL)
  {
    pAllocBlock->m_dwStates = BLOCK_STATE_IN_USE;
    pAllocBlock->m_pBaseBlock = hKHeapAlloc;
    pAllocBlock->m_dwSize = dwSize;
    if (pBlockInfoSave != NULL) // новый блок
    {
      pAllocBlock->m_pNextBlock = pBlockInfoSave->m_pNextBlock;
      pBlockInfoSave->m_pNextBlock = pAllocBlock;
    }

//    memset(pAllocBlock->m_pArray, 0, dwSize);
  }

  return pAllocBlock;
}

ULONG KHeapGetSizeSystemMemory(KHEAP hKHeap)
{
  KHEAP           hKHeapSav = hKHeap;
  ULONG           dwSize = 0;
  #ifndef __KHEAP_WIN32
  #ifndef __KHEAP_LOCK_MUTEX
  KIRQL           OldIrqlForLock;
  #endif //__KHEAP_LOCK_MUTEX
  #endif //!__KHEAP_WIN32

  if (hKHeap == NULL)
    hKHeap = hKHeapGlobalDefault;

  if (!KHeapIsAddressValid(hKHeap))
    return dwSize;

  #ifdef __KHEAP_LOCK_MUTEX
  KHeapLock(hKHeap);
  #else
  KHeapLock(hKHeap, &OldIrqlForLock);
  #endif //__KHEAP_LOCK_MUTEX

  while (hKHeapSav)
  {
    dwSize += hKHeapSav->m_dwSize;
    hKHeapSav = (KHEAP) hKHeapSav->m_pNextBlock;
  }

  #ifdef __KHEAP_LOCK_MUTEX
  KHeapUnLock(hKHeap);
  #else
  KHeapUnLock(hKHeap, OldIrqlForLock);
  #endif //__KHEAP_LOCK_MUTEX

  return dwSize;
}

ULONG KHeapGetSizeUsageMemory(KHEAP hKHeap)
{
  PMEMORY_BLOCK   pBlockInfo;
  ULONG           dwSize = 0;
  KHEAP           hKHeapSav = hKHeap;
  #ifndef __KHEAP_WIN32
  #ifndef __KHEAP_LOCK_MUTEX
  KIRQL           OldIrqlForLock;
  #endif //__KHEAP_LOCK_MUTEX
  #endif //!__KHEAP_WIN32

  if (hKHeap == NULL)
    hKHeap = hKHeapGlobalDefault;

  if (!KHeapIsAddressValid(hKHeap))
    return dwSize;

  #ifdef __KHEAP_LOCK_MUTEX
  KHeapLock(hKHeap);
  #else
  KHeapLock(hKHeap, &OldIrqlForLock);
  #endif //__KHEAP_LOCK_MUTEX

  while (hKHeapSav)
  {
    pBlockInfo = (PMEMORY_BLOCK)hKHeapSav->m_pArray;
    while (pBlockInfo)
    {
      if (pBlockInfo->m_dwStates & BLOCK_STATE_IN_USE)
        dwSize += pBlockInfo->m_dwSize;
      pBlockInfo = pBlockInfo->m_pNextBlock;
    }
    hKHeapSav = (KHEAP) hKHeapSav->m_pNextBlock;
  }

  #ifdef __KHEAP_LOCK_MUTEX
  KHeapUnLock(hKHeap);
  #else
  KHeapUnLock(hKHeap, OldIrqlForLock);
  #endif //__KHEAP_LOCK_MUTEX

  return dwSize;
}

PVOID KHeapAllocateMemory(ULONG dwSize)
{
  PVOID pArray = NULL;

  if (dwSize)
  {
    #ifdef __KHEAP_WIN32
    pArray = malloc(dwSize);
    #else
    pArray = ExAllocatePool(NonPagedPool, dwSize);
    #endif
    if (pArray != NULL)
      memset(pArray, 0, dwSize);
  }

  return pArray;
}

VOID KHeapFreeMemory(PVOID pArray)
{
  if (KHeapIsAddressValid(pArray))
  {
    #ifdef __KHEAP_WIN32
    free(pArray);
    #else
    ExFreePool(pArray);
    #endif
  }
}

#ifdef __KHEAP_LOCK_MUTEX
VOID KHeapLock(KHEAP hKHeap)
#else
VOID KHeapLock(KHEAP hKHeap, PKIRQL pOldIrql)
#endif //__KHEAP_LOCK_MUTEX
{
  if (KHeapIsAddressValid(hKHeap))
  {
    #ifdef __KHEAP_WIN32
    if (hKHeap->m_Lock != NULL) 
      EnterCriticalSection(hKHeap->m_Lock);
    #else
    #ifndef __KHEAP_LOCK_MUTEX
    KeAcquireSpinLock(&hKHeap->m_Lock, pOldIrql);//(KIRQL*)&hKHeap->m_OldIrql);
    #else
    KeWaitForMutexObject(hKHeap->m_Lock, Executive, KernelMode, FALSE, NULL);
    #endif //__KHEAP_LOCK_MUTEX
    #endif
  }
}

#ifdef __KHEAP_LOCK_MUTEX
VOID  KHeapUnLock(KHEAP hKHeap)
#else
VOID  KHeapUnLock(KHEAP hKHeap, KIRQL OldIrql)
#endif //__KHEAP_LOCK_MUTEX
{
  if (KHeapIsAddressValid(hKHeap))
  {
    #ifdef __KHEAP_WIN32
    if (hKHeap->m_Lock != NULL) 
      LeaveCriticalSection(hKHeap->m_Lock);
    #else
     #ifndef __KHEAP_LOCK_MUTEX
     KeReleaseSpinLock(&hKHeap->m_Lock, OldIrql);//(KIRQL)hKHeap->m_OldIrql);
     #else
     KeReleaseMutex(hKHeap->m_Lock, FALSE);
     #endif //__KHEAP_LOCK_MUTEX
    #endif
  }
}

BOOLEAN KHeapIsAddressValid(PVOID Address)
{
  if (!Address)
    return FALSE;
  return MmIsAddressValid(Address);
}