#ifndef __UNLOCK_CLIENTS_LIST_H
 #define __UNLOCK_CLIENTS_LIST_H
//#define __HE4_UNLOCK_DEBUG

extern "C"
{
 #include "ntddk.h"
}

#include "../CommonClasses/Include/KTypes.h"
#include "SaveObjectsList.h"
#include "../CommonClasses/KMemoryManager/KMemoryManager.h"

#ifdef __HE4_UNLOCK_DEBUG
#define DbgPrintUnlock(arg) DbgPrint arg
#else
#define DbgPrintUnlock(arg)
#endif //__HE4_UNLOCK_DEBUG


//******************************************************************//
//************     List unlock threads functions    ****************//
//******************************************************************//
#define HE4_UNLOCK_CLIENT_UNKNOWN  ((PVOID)0xffffffff)

BOOLEAN  AddUnlockList(ULONG dwClientId, PVOID lpCCB, ULONG dwUnlockFlags);
BOOLEAN  DelUnlockList(ULONG dwClientId, PVOID lpCCB);
PUNLOCK_CLIENT_INFO FindUnlockInfo(PETHREAD lpTCB, PEPROCESS lpPCB);
void     ClearUnlockList(void);
BOOLEAN  GetUnlockListSizeByBytes(ULONG* pdwSize);
BOOLEAN  GetUnlockList(PUNLOCK_CLIENT_INFO_SET pUnlockInfoSet);


#endif //__UNLOCK_CLIENTS_LIST_H