#ifndef __MISC_H
 #define __MISC_H 
//#define HE4_MISC_DEBUG
#define __MISC_USE_KHEAP

extern "C"
{
 #include "ntddk.h"
}

#include "stdio.h"
#include "string.h"

#include "../Include/KTypes.h"
#include "../KStdLib/krnlstdlib.h"
#include "../Include/NtoskrnlUndoc.h"

#ifdef __MISC_USE_KHEAP
 #include "../KMemoryManager/KMemoryManager.h"
#endif //__MISC_USE_KHEAP

//
// Print macro that only turns on when debugging is on
//
#ifdef HE4_MISC_DEBUG
#define DbgPrintMisc(arg) DbgPrint arg
#else
#define DbgPrintMisc(arg)
#endif

typedef PVOID           POBJECT;

//
// Функции для работы с памятью
//
typedef struct _SHARED_MEMORY
{
  ULONG  m_dwSizeRegion;               // into bytes
  PVOID  m_lpKernelMemory;
  PMDL   m_Mdl;
  PVOID  m_lpUserPage;
  PVOID  m_lpUserMemory;
} SHARED_MEMORY, *PSHARED_MEMORY;

BOOLEAN
AllocateSharedMemory(
   IN OUT PSHARED_MEMORY lpSharedMemory,
   IN POOL_TYPE PoolType,
   IN ULONG dwSizeRegion
   );

BOOLEAN
FreeSharedMemory(
   PSHARED_MEMORY lpSharedMemory
   );

PVOID
MapUserAddressToKernel(
   IN PVOID pUserModeAddress,
   IN ULONG ulSize,
   OUT PMDL* ppMdl
   );

VOID
UnmapMappedKernelAddress(
   IN PMDL pMdl
   );

BOOLEAN
IsBadWritePtr(
   IN PVOID Address,
   IN ULONG Length,
   IN ULONG Alignment
   );

BOOLEAN
_MmIsAddressValid(
   IN PVOID Address
   );

//
// функции для работы с объектами ядра
//

POBJECT
GetPointer(
   IN HANDLE handle
   );

VOID
ReleasePointer(
   IN POBJECT object
   );

ULONG
GetObjectName(
   IN HANDLE hObject,
   IN OUT PWSTR lpwszName,
   IN ULONG dwSize
   );

ULONG
GetObjectNameByObjectAttributes(
   IN POBJECT_ATTRIBUTES ObjectAttributes,
   IN OUT PWSTR fullPathName,
   IN ULONG nfullPathNameSize
   );

ULONG
GetObjectNameByFileObject(
   IN PFILE_OBJECT fileObject,
   IN OUT PWSTR fullPathName,
   ULONG nfullPathNameSize
   );

// получает указатель на объект заданного типа (например: IoDriverObjectType)
// по его полному пути (например: \Device\Serial0)
PVOID
GetObjectByPath(
   IN PWSTR pwszObjectName,
   IN PVOID pObjectType
   );

POBJECT_NAME
GetNameOfObject(
   IN PVOID pObject
   );

//
// функции для работы с объектами файловых систем
//

BOOLEAN
GetFileNameNative(
   IN HANDLE hObject,
   IN OUT PWSTR lpwszName,
   IN ULONG dwSize
   );

BOOLEAN
FilemonQueryFileName(
   IN PDEVICE_OBJECT DeviceObject, 
   IN PFILE_OBJECT FileObject,
   IN OUT PFILE_NAME_INFORMATION FileName,
   IN ULONG FileNameLength
   );

NTSTATUS
NativeQueryDirectoryFile(
   IN PDRIVER_DISPATCH pMajorFunction OPTIONAL,
   IN PDEVICE_OBJECT pDeviceObject,
   IN PFILE_OBJECT pFileObject,
   IN OUT PIO_STATUS_BLOCK pIoStatusBlock,
   IN OUT PVOID Buffer,
   IN ULONG BufferLength,
   IN FILE_INFORMATION_CLASS DirectoryInfoClass,
   IN BOOLEAN ByOne,
   IN PUNICODE_STRING pSearchTemplate,
   IN BOOLEAN Reset,
   IN BOOLEAN Index,
   IN ULONG dwIndex
   );

// а-ля IoGetBaseFileSystemDeviceObject(PFILE_OBJECT pFileObject)
PDEVICE_OBJECT
GetVolumeDeviceObject(
   IN PFILE_OBJECT pFileObject
   );

//
// разное
//

BOOLEAN  GetDirectoryFromPath(PWSTR lpwszFullFileName, ULONG dwSize);
BOOLEAN  GetDirectoryFromPathA(CHAR *lpszFullFileName, ULONG dwSize);
int      GetToken(WCHAR *lpInBuf, int dwInBufSize, WCHAR *lpOutBuf, int dwOutBufSize, WCHAR *lpDeliver, int nDeliverCount, int nNumber);


NTQUERYDIRECTORYOBJECT GetPtrToZwQueryDirectoryObject(void);
SHARED_MEMORY* InitQueryObjectNameType(void);
void      DeinitQueryObjectNameType(SHARED_MEMORY* pSharedBuffer);
BOOLEAN   QueryObjectNameType(PVOID pObjTypeInfo, ULONG dwSizeOfObjTypeInfo, HANDLE hDir, PWSTR pwszObjectName, SHARED_MEMORY* pSharedBuffer);
ULONG     DosPathNameToNtPathName(PWSTR pwszDosPath, PWSTR pwszNtPath, ULONG dwSizeNtPathByBytes, ULONG dwRecursiveDeep, PULONG pdwObjectSizeByBytes);

VOID
FlushInstuctionCache(
    VOID
    );

// вовращает low-level DeviceObject в пределах одного DriverObject
// т.е. особого смысла в ней нет...
PDEVICE_OBJECT GetOwnDeviceObject(PDEVICE_OBJECT DeviceObject);
// тоже но из Irp и не привязано к одному DriverObject
PDEVICE_OBJECT GetOwnDeviceObjectFromIrp(PIRP pIrp);

// не работает
PDRIVER_OBJECT CreateInvisibleDriverObject(PVOID pBaseAddress, ULONG dwDriverSize, HANDLE hSystemImage, PDRIVER_INITIALIZE DriverEntry);

HANDLE   LoadDevice(PWSTR pwszDeviceFileName);
NTSTATUS UnloadDevice(HANDLE hSystemImage);

BOOLEAN
DeleteItemFromQueryDirectoryBuffer(
   PFQD_SmallCommonBlock pQueryDirPrev,
   PFQD_SmallCommonBlock pQueryDir,
   PVOID Buffer, ULONG BufferLength,
   PIO_STATUS_BLOCK IoStatusBlock,
   FILE_INFORMATION_CLASS DirectoryInfoClass,
   NTSTATUS* NtStatus
   );

LPSSDT   FindShadowTable(void);
#endif //__MISC_H
