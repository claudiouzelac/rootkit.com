#ifndef __K_NEW_H
 #define __K_NEW_H

#include "../KMemoryManager/KMemoryManager.h"

__inline void* __cdecl operator new(size_t size)
{
  return _AllocatePoolFromKHeap(NULL, size);
}

__inline void __cdecl operator delete(void* p)
{
  FreePoolToKHeap(NULL, p);
}

__inline void* __cdecl operator new[](size_t size)
{
  return _AllocatePoolFromKHeap(NULL, size);
}

__inline void __cdecl operator delete[](void* p)
{
  FreePoolToKHeap(NULL, p);
}

__inline void *operator new(size_t size, void* p)
{
  return ((p) ? p : new unsigned char[size]);
}


#endif //__K_NEW_H