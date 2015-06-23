#ifndef __SAVEOBJECTSLIST_H
 #define __SAVEOBJECTSLIST_H

extern "C"
{
 #include "ntddk.h"
}

#include "../CommonClasses/KStdLib/krnlstdlib.h"
#include "He4HookInv.h"
#include "../CommonClasses/Misc/Misc.h"
#include "../CommonClasses/KMemoryManager/KMemoryManager.h"


#ifdef HE4_SO_HOOK_DEBUG
#define DbgPrintSO(arg) DbgPrint arg
#else
#define DbgPrintSO(arg)
#endif

BOOLEAN   GetUpdateOptions(ULONG dwAccessType, PACCESS_MASK lpDesiredAccess, PULONG lpShareAccess, PULONG lpCreateDisposition, PULONG lpCreateOptions);
BOOLEAN   CheckFileInfo(PFILEINFO lpFileInfo);
BOOLEAN   DelFileSaveList(PFILEINFOSET lpFileInfoSet);
BOOLEAN   AddFileSaveList(PFILEINFOSET lpFileInfoSet);
BOOLEAN   GetFileListSizeByBytes(PULONG pdwSize);
BOOLEAN   GetFileSaveList(PFILEINFOSET pFileInfoSet);
PFILEINFO FindProtectedFile(PWSTR lpwszFileName, PFILEINFO* ppDirInfo);
BOOLEAN   IsFileProtected(KHEAP hKHeap, PWSTR lpwszFileName, PFILEINFO *lppFileInfo, PFILEINFO *lppDirInfo, PETHREAD pCurrentThread, PEPROCESS pCurrentProcess);
BOOLEAN   IsFileProtectedLight(KHEAP hKHeap, PWSTR lpwszFileName, ULONG* pdwFileAccessType, ULONG* pdwDirAccessType, PETHREAD pCurrentThread, PEPROCESS pCurrentProcess);
void      ClearSaveFileList(void);

#endif //__SAVEOBJECTSLIST_H