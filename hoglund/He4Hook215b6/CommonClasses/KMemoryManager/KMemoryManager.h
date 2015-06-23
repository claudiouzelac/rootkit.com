#ifndef __MEMORY_MANAGER_H
 #define __MEMORY_MANAGER_H
//
// Данный код можно использовать на IRQL <= DISPATCH_LEVEL,
// если __KHEAP_LOCK_MUTEX не определен, иначе IRQL == PASSIVE_LEVEL
//

//#define __KHEAP_WIN32
#ifndef __KHEAP_WIN32
// #define __KHEAP_LOCK_MUTEX
#endif //__KHEAP_WIN32

#ifndef __KHEAP_WIN32
extern "C"
{
 #include "ntddk.h"
}

 #include "stdio.h"
#else
 #include <windows.h>
 #include <stdio.h>
#endif //!__KHEAP_WIN32

//
// Для KernelMode убрать
//
#ifdef __KHEAP_WIN32
 #define MmIsAddressValid(Array) (!(Array==NULL))
 #define KSPIN_LOCK               CRITICAL_SECTION*
 #define KIRQL                    DWORD
 #define ULONG                    DWORD
 #define BOOLEAN                  BOOL
#endif //__KHEAP_WIN32
//
//
//

#define BLOCK_STATE_FREE          0x00000000
#define BLOCK_STATE_IN_USE        0x00000001
#define BLOCK_STATE_KHEAP         0x80000000 // блок является описателем кучи

#pragma pack(push)
#pragma pack(1)
typedef struct tag_MEMORY_BLOCK
{
  struct tag_MEMORY_BLOCK*  m_pNextBlock; // Следующий выделенный в куче блок 
  struct tag_MEMORY_BLOCK*  m_pBaseBlock; // Описатель кучи или предидущий блок (для BLOCK_STATE_KHEAP)
  ULONG       m_dwStates;
  ULONG       m_dwSize;                   // размер блока m_pArray
  #ifndef __KHEAP_LOCK_MUTEX
  KSPIN_LOCK  m_Lock;                     // используется при BLOCK_STATE_KHEAP
  //KIRQL       m_OldIrql;                // т.к. KIRQL = UCHAR, то для выравнивания на
  ULONG       m_OldIrql_Align;            // 8-ми байтовую границу сделаем его ULONG (используется при BLOCK_STATE_KHEAP)
  #else
  PKMUTEX     m_Lock;
  ULONG       m_Align;                    // выравнивание на 8 байт.
  #endif //__KHEAP_LOCK_MUTEX
  CHAR        m_pArray[1];
} MEMORY_BLOCK, *PMEMORY_BLOCK;
#pragma pack(pop)

#define SIZEOF_MEMORY_BLOCK      (sizeof(MEMORY_BLOCK))
#define SIZEOF_MEMORY_BLOCK_REAL (SIZEOF_MEMORY_BLOCK-sizeof(CHAR))

typedef PMEMORY_BLOCK KHEAP;

KHEAP    KHeapCreate(ULONG dwSize);
BOOLEAN  KHeapDestroy(KHEAP hKHeap);

BOOLEAN  CreateDefaultHeap(ULONG dwSize);
VOID     DestroyDefaultHeap(VOID);

PVOID    AllocatePoolFromKHeap(KHEAP hKHeap, ULONG dwSize);
VOID     FreePoolToKHeap(KHEAP hKHeap, PVOID pArray);
PVOID    _AllocatePoolFromKHeap(KHEAP hKHeap, ULONG dwSize); // Optimize for min(fragmentation_heap)
ULONG    KHeapGetSizeSystemMemory(KHEAP hKHeap);
ULONG    KHeapGetSizeUsageMemory(KHEAP hKHeap);
KHEAP    KGetDefaultHeap(VOID);
#endif //__MEMORY_MANAGER_H
