#ifndef __NT_OS_KERNEL_UNDOCUMENT__
 #define __NT_OS_KERNEL_UNDOCUMENT__

extern "C"
{

#include "ntddk.h"


#include "KTypes.h"

//#define __WIN2K

typedef PVOID           POBJECT;

// The build number, accessible via the NtBuildNumber variable that is
// exported from the kernel, is a 32-bit value where the high nibble is
// either 'C', for Checked Build, or 'F', for Free Build, and the rest is
// the actual base build number of NT (1381, or 0x565, for NT 4.0 and any
// Service Packs). 

extern PULONG   NtBuildNumber;

#ifndef __KE_SERVICE_DESCRIPTOR_TABLE
 #define __KE_SERVICE_DESCRIPTOR_TABLE
//
// Definition for system call service table
//
typedef struct _SRVTABLE {
        PVOID           *ServiceTable;
        ULONG           LowCall;        
        ULONG           HiCall;
        PVOID           *ArgTable;
} SRVTABLE, *PSRVTABLE;

//
// Pointer to the image of the system service table
//
extern PSRVTABLE KeServiceDescriptorTable;

//
// Macro for easy hook/unhook. On X86 implementations of Zw* functions, the DWORD
// following the first byte is the system call number, so we reach into the Zw function
// passed as a parameter, and pull the number out. This makes system call hooking
// dependent ONLY on the Zw* function implementation not changing.
//
#if defined(_ALPHA_)
#define SYSCALL(_function)  KeServiceDescriptorTable->ServiceTable[ (*(PULONG)_function)  & 0x0000FFFF ]
#else
#define SYSCALL(_function)  KeServiceDescriptorTable->ServiceTable[ *(PULONG)((PUCHAR)_function+1)]
#endif

#endif //__KE_SERVICE_DESCRIPTOR_TABLE

typedef VOID *SSTAT[];  // SSTAT is an array of pointers to the
                        //  service handler addresses of each 
                        // service entry in the SST.
typedef unsigned char SSTPT[];   // SSTPT is an array of bytes containing 
                        // the size of the parameter stack in 
                        // bytes for each service entry in the SST.

typedef SSTAT *LPSSTAT; // LPSSTAT is a pointer to an SSTAT.
typedef SSTPT *LPSSTPT; // LPSSTPT is a pointer to an SSTPT.

typedef struct SystemServiceDescriptor
{
  LPSSTAT lpSystemServiceTableAddressTable;   // Pointer to the 
                                              // Address Table ( SSTAT ) structure of the SST.
  ULONG   dwFirstServiceIndex;                // ( ? ) Always set to FALSE.
  ULONG   dwSystemServiceTableNumEntries;     // Number of entries
                                              //  in the SST.
  LPSSTPT lpSystemServiceTableParameterTable; // Pointer to 
                                              // the Parameter Table
                                              // ( SSTPT ) structure 
                                              // of the SST.
} SSD, *LPSSD;

typedef struct SystemServiceDescriptorTable
{
  SSD   SystemServiceDescriptors[4];   // The array of 4 SSDs.
} SSDT, *LPSSDT;

//
// Definition for KeAddSystemServiceTable call
//
NTSYSAPI
BOOLEAN
NTAPI
KeAddSystemServiceTable(          
    LPSSTAT  lpAddressTable,   // Pointer to the SSTAT
                               // structure of the SST.
    BOOLEAN  bUnknown,         // Unknown. Always set
                               // to FALSE. If you have
                               // any information
                               // regarding this please
                               // let me know.
    ULONG    dwNumEntries,     // Number of entries in the SST.
    LPSSTPT  lpParameterTable, // Pointer to the SSTPT
                               // structure of the SST.
    ULONG    dwTableID         // Index of the SSD to
                               // add the SST to.
    );

//
// Definition for ZwDeleteValueKey call
//
NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
    IN HANDLE hKey,
    IN PUNICODE_STRING UniNameKey
    );

//
// For displaying messages to the Blue Screen
//
NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(
    PUNICODE_STRING Text
    );


//
// Directory control structure
//
//typedef struct _QUERY_DIRECTORY
//{
//  ULONG Length;
//  PUNICODE_STRING FileName;
//  FILE_INFORMATION_CLASS FileInformationClass;
//  ULONG FileIndex;
//} QUERY_DIRECTORY, *PQUERY_DIRECTORY;

/*
typedef struct _FILE_NAMES_INFORMATION
{
  ULONG NextEntryOffset;
  ULONG FileIndex;
  ULONG FileNameLength;
  WCHAR FileName[ANYSIZE_ARRAY];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

#define SIZE_OF_FILE_NAMES_INFORMATION (sizeof(FILE_NAMES_INFORMATION)-sizeof(WCHAR)*ANYSIZE_ARRAY)

typedef struct tag_FQD_CommonBlock
{
  ULONG   NextEntryOffset;
  ULONG   FileIndex;
  TIME    CreationTime;
  TIME    LastAccessTime;
  TIME    LastWriteTime;
  TIME    ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG   FileAttributes;
  ULONG   FileNameLength;
} FQD_CommonBlock, *PFQD_CommonBlock;

typedef struct _FILE_QUERY_DIRECTORY
{
  ULONG   NextEntryOffset;
  ULONG   FileIndex;
  TIME    CreationTime;
  TIME    LastAccessTime;
  TIME    LastWriteTime;
  TIME    ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG   FileAttributes;
  ULONG   FileNameLength;

  union
  {
    struct
    {
      WCHAR  FileName[ANYSIZE_ARRAY];
    } Class1;
    struct
    {
      ULONG  Unknown2;
      WCHAR  FileName[ANYSIZE_ARRAY];
    } Class2;
    struct
    {
      ULONG  Unknown2;
      USHORT AlternateFileNameLength;
      WCHAR  AlternateFileName[12];
      WCHAR  FileName[ANYSIZE_ARRAY];
    } Class3;
  };
} FILE_QUERY_DIRECTORY, *PFILE_QUERY_DIRECTORY;

#define SIZE_OF_FQD_CLASS1 (sizeof(FQD_CommonBlock))
#define SIZE_OF_FQD_CLASS2 (sizeof(FQD_CommonBlock) + sizeof(FILE_QUERY_DIRECTORY.Class2) - sizeof(WCHAR)*ANYSIZE_ARRAY)
#define SIZE_OF_FQD_CLASS3 (sizeof(FQD_CommonBlock) + sizeof(FILE_QUERY_DIRECTORY.Class3) - sizeof(WCHAR)*ANYSIZE_ARRAY)
*/

#pragma pack(push)
#pragma pack(4)
//
// Directory control structure
//
typedef struct tag_QUERY_DIRECTORY
{
  ULONG Length;
  PUNICODE_STRING FileName;
  FILE_INFORMATION_CLASS FileInformationClass;
  ULONG FileIndex;
} QUERY_DIRECTORY, *PQUERY_DIRECTORY;


typedef struct tag_FQD_SmallCommonBlock
{
  ULONG   NextEntryOffset;
  ULONG   FileIndex;
} FQD_SmallCommonBlock, *PFQD_SmallCommonBlock;

typedef struct tag_FQD_FILE_ATTR
{
  TIME    CreationTime;
  TIME    LastAccessTime;
  TIME    LastWriteTime;
  TIME    ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG   FileAttributes;
} FQD_FILE_ATTR, *PFQD_FILE_ATTR;

typedef struct tag_FQD_CommonBlock
{
  FQD_SmallCommonBlock SmallCommonBlock;
  FQD_FILE_ATTR        FileAttr;
  ULONG                FileNameLength;
} FQD_CommonBlock, *PFQD_CommonBlock;

typedef struct _FILE_NAMES_INFORMATION
{
  FQD_SmallCommonBlock SmallCommonBlock;
  ULONG FileNameLength;
  WCHAR FileName[ANYSIZE_ARRAY];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

#define SIZE_OF_FILE_NAMES_INFORMATION (sizeof(FILE_NAMES_INFORMATION)-sizeof(WCHAR)*ANYSIZE_ARRAY)

typedef struct _FILE_DIRECTORY_INFORMATION
{
  FQD_CommonBlock CommonBlock;

  WCHAR  FileName[ANYSIZE_ARRAY];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

#define SIZE_OF_FILE_DIRECTORY_INFORMATION (sizeof(FILE_DIRECTORY_INFORMATION)-sizeof(WCHAR)*ANYSIZE_ARRAY)

typedef struct _FILE_FULL_DIR_INFORMATION
{
  FQD_CommonBlock CommonBlock;

  ULONG  EaSize;
  WCHAR  FileName[ANYSIZE_ARRAY];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

#define SIZE_OF_FILE_FULL_DIR_INFORMATION (sizeof(FILE_FULL_DIR_INFORMATION)-sizeof(WCHAR)*ANYSIZE_ARRAY)

typedef struct _FILE_BOTH_DIR_INFORMATION
{
  FQD_CommonBlock CommonBlock;

  ULONG  EaSize;
  USHORT ShortFileNameLength;
//  CCHAR  ShortFileNameLength;
  WCHAR  ShortFileName[12];
  WCHAR  FileName[ANYSIZE_ARRAY];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;
#pragma pack(pop)

#define SIZE_OF_FILE_BOTH_DIR_INFORMATION (sizeof(FILE_BOTH_DIR_INFORMATION)-sizeof(WCHAR)*ANYSIZE_ARRAY)


//
// Definition for ZwOpenFile call
//
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    );

//
// Definition for ZwQueryDirectoryFile call
//
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
    IN HANDLE DirectoryFileHandle,
    IN HANDLE EventHandle OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    IN FILE_INFORMATION_CLASS DirectoryInfoClass,
    IN BOOLEAN ByOne,
    IN PUNICODE_STRING SearchTemplate OPTIONAL,
    IN BOOLEAN Reset
    );

//
// Definition for ZwQueryObject call
//
typedef enum _OBJECTINFOCLASS
{
  BaseObjectInfo = 0,
  NameObjectInfo,           // ObjectInformationLength = 0x200;
  TypeObjectInfo,           // ObjectInformationLength = 0x200;
  UnknownObjectInfo,        //
  HandleObjectInfo          // ObjectInformationLength = 0x200;
} OBJECTINFOCLASS;

//
// Definition for ZwQueryObject call
//
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryObject(
    IN HANDLE ObjectHandle,
    IN OBJECTINFOCLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG ObjectInformationLength,
    OUT PULONG LengthReturned OPTIONAL
    );

typedef struct _BASE_OBJECT_INFO
{
  ULONG HandleAttributes;
  ACCESS_MASK GrantedAccess;
  ULONG HandleCount;
  ULONG ReferenceCount;
  ULONG Unknown[10];
} BASE_OBJECT_INFO, *PBASE_OBJECT_INFO;

typedef struct _NAME_OBJECT_INFO
{
  UNICODE_STRING Name;
} NAME_OBJECT_INFO, *PNAME_OBJECT_INFO;

typedef struct _TYPE_OBJECT_INFO
{
  UNICODE_STRING Type;
  ULONG InstanceCount;
  ULONG HandleCount;
  ULONG Unknown1[11];
  GENERIC_MAPPING GenericMapping;
  ACCESS_MASK MaximumAllowed;
  ULONG Unknown2[4];
} TYPE_OBJECT_INFO, *PTYPE_OBJECT_INFO;

typedef struct _HANDLE_OBJECT_INFO
{
  BOOLEAN Inherit;
  BOOLEAN ProtectFromClose;
} HANDLE_OBJECT_INFO, *PHANDLE_OBJECT_INFO;

NTSYSAPI
NTSTATUS
NTAPI
ObReferenceObjectByName(
    IN PUNICODE_STRING ObjectPath,
    IN ULONG Attributes,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID *ObjectPtr
    );  

NTSYSAPI
VOID
NTAPI
ProbeForWrite(
    IN PVOID Address, 
    IN ULONG Length,  
    IN ULONG Alignment
    ); 

NTSYSAPI
KPROCESSOR_MODE
NTAPI
KeGetPreviousMode(
    );

//
// Definition for ObQueryNameString call
//
//NTSYSAPI
//NTSTATUS
//NTAPI
//ObQueryNameString(
//    POBJECT Object,
//    PUNICODE_STRING Name,
//    ULONG MaximumLength,
//    PULONG ActualLength
//    );

NTSYSAPI
NTSTATUS
NTAPI
ObQueryNameString(
    IN PDEVICE_OBJECT DeviceObject,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG MaximumLength,
    OUT PULONG LengthReturned
    );


typedef struct _OBJECT_NAMETYPE_INFO 
{               
  UNICODE_STRING ObjectName;
  UNICODE_STRING ObjectType;
} OBJECT_NAMETYPE_INFO, *POBJECT_NAMETYPE_INFO;   

typedef enum _DIRECTORYINFOCLASS 
{
  ObjectArray,
  ObjectByOne
} DIRECTORYINFOCLASS, *PDIRECTORYINFOCLASS;

//NTSYSAPI
//NTSTATUS
//NTAPI
//ZwQueryDirectoryObject (
//    IN HANDLE       DirectoryHandle,
//    OUT PVOID       Buffer,
//    IN ULONG        Length,
//    IN BOOLEAN      ReturnSingleEntry,
//    IN BOOLEAN      RestartScan,
//    IN OUT PULONG   Context,
//    OUT PULONG      ReturnLength OPTIONAL
//    );

//NtQueryDirectoryObject
typedef NTSTATUS (__stdcall *NTQUERYDIRECTORYOBJECT)(HANDLE DirectoryObjectHandle, PVOID ObjectInfoBuffer, ULONG ObjectInfoBufferLength, DIRECTORYINFOCLASS DirectoryInformationClass, BOOLEAN First, PULONG ObjectIndex, PULONG LengthReturned);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject (
    OUT PHANDLE             DirectoryHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject (
    OUT PHANDLE             SymbolicLinkHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySymbolicLinkObject (
    IN HANDLE               LinkHandle,
    IN OUT PUNICODE_STRING  LinkTarget,
    OUT PULONG              ReturnedLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
PsLookupThreadByThreadId (
    IN ULONG ulThreadId,
    OUT PETHREAD* ppEThread
    );

NTSYSAPI
NTSTATUS
NTAPI
PsLookupProcessByProcessId (
    IN ULONG ulProcessId,
    OUT PEPROCESS* ppEProcess
    );

#ifndef __WIN2K
typedef struct _FILE_NAME_INFORMATION
{
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
IoCreateFile(
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
    ) ;
#endif //__WIN2K

typedef void*               HINSTANCE;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef void*               PPS_IMPERSONATION_INFO;

typedef struct _TOP_LEVEL_IRP
{
  ULONG ulUnknown0;
  ULONG ulUnknown1;
} TOP_LEVEL_IRP, *PTOP_LEVEL_IRP;

typedef struct _KAPC_STATE           //Size: 0x18
{
  LIST_ENTRY ApcListHead[2]; //0x00
  struct _EPROCESS*  Process; //0x10
  BYTE       KernelApcInProgress; //0x14
  BYTE       KernelApcPending;    //0x15
  BYTE       UserApcPending;      //0x16
  BYTE       Reserved; //0x17
} KAPC_STATE, *PKAPC_STATE;

typedef struct APC_STATE_POINTER //size : 0x8
{
  PKAPC_STATE SavedApcState;   //0x00
  PKAPC_STATE ApcState;        //0x04
} APC_STATE_POINTER;

typedef struct _PEB
{           // Size: 0x1D8
  UCHAR InheritedAddressSpace;                                         /*000*/
  UCHAR ReadImageFileExecOptions;                                      /*001*/
  UCHAR BeingDebugged;                                                 /*002*/
  UCHAR SpareBool;                // Allocation size                   /*003*/
  HANDLE Mutant;                                                       /*004*/
  HINSTANCE ImageBaseAddress; // Instance                              /*008*/
  VOID *Ldr;          // Module list?                                  /*00C*/
  VOID *ProcessParameters;                                             /*010*/
  ULONG SubSystemData;                                                 /*014*/
  HANDLE ProcessHeap;                                                  /*018*/
  KSPIN_LOCK FastPebLock;                                              /*01C*/
  ULONG FastPebLockRoutine;                                            /*020*/
  ULONG FastPebUnlockRoutine;                                          /*024*/
  ULONG EnvironmentUpdateCount;                                        /*028*/
  ULONG KernelCallbackTable;                                           /*02C*/
  LARGE_INTEGER SystemReserved;                                        /*030*/
  ULONG FreeList;                                                      /*038*/
  ULONG TlsExpansionCounter;                                           /*03C*/
  ULONG TlsBitmap;                                                     /*040*/
  LARGE_INTEGER TlsBitmapBits;                                         /*044*/
  ULONG ReadOnlySharedMemoryBase;                                      /*04C*/
  ULONG ReadOnlySharedMemoryHeap;                                      /*050*/
  ULONG ReadOnlyStaticServerData;                                      /*054*/
  ULONG AnsiCodePageData;                                              /*058*/
  ULONG OemCodePageData;                                               /*05C*/
  ULONG UnicodeCaseTableData;                                          /*060*/
  ULONG NumberOfProcessors;                                            /*064*/
  LARGE_INTEGER NtGlobalFlag;     // Address of a local copy           /*068*/
  LARGE_INTEGER CriticalSectionTimeout;                                /*070*/
  ULONG HeapSegmentReserve;                                            /*078*/
  ULONG HeapSegmentCommit;                                             /*07C*/
  ULONG HeapDeCommitTotalFreeThreshold;                                /*080*/
  ULONG HeapDeCommitFreeBlockThreshold;                                /*084*/
  ULONG NumberOfHeaps;                                                 /*088*/
  ULONG MaximumNumberOfHeaps;                                          /*08C*/
  ULONG ProcessHeaps;                                                  /*090*/
  ULONG GdiSharedHandleTable;                                          /*094*/
  ULONG ProcessStarterHelper;                                          /*098*/
  ULONG GdiDCAttributeList;                                            /*09C*/
  KSPIN_LOCK LoaderLock;                                               /*0A0*/
  ULONG OSMajorVersion;                                                /*0A4*/
  ULONG OSMinorVersion;                                                /*0A8*/
  USHORT OSBuildNumber;                                                /*0AC*/
  USHORT OSCSDVersion;                                                 /*0AE*/
  ULONG OSPlatformId;                                                  /*0B0*/
  ULONG ImageSubsystem;                                                /*0B4*/
  ULONG ImageSubsystemMajorVersion;                                    /*0B8*/
  ULONG ImageSubsystemMinorVersion;                                    /*0BC*/
  ULONG ImageProcessAffinityMask;                                      /*0C0*/
  ULONG GdiHandleBuffer[0x22];                                         /*0C4*/
  ULONG PostProcessInitRoutine;                                        /*14C*/
  ULONG TlsExpansionBitmap;                                            /*150*/
  UCHAR TlsExpansionBitmapBits[0x80];                                  /*154*/
  ULONG SessionId;                                                     /*1D4*/
} PEB, *PPEB;


typedef struct _TEB
{           // Size: 0xF88
  NT_TIB NtTib;                                        /*000*/
  VOID*  EnvironmentPointer;                           /*01C*/
  CLIENT_ID ClientId;                                  /*020*/
  HANDLE ActiveRpcHandle;                              /*028*/
  VOID* ThreadLocalStoragePointer;                     /*02C*/
  PEB*  ProcessEnvironmentBlock;  // PEB               /*030*/
  ULONG LastErrorValue;                                /*034*/
  ULONG CountOfOwnedCriticalSections;                  /*038*/
  ULONG CsrClientThread;                               /*03C*/
  ULONG Win32ThreadInfo;                               /*040*/
  UCHAR Win32ClientInfo[0x7C];                         /*044*/
  ULONG WOW32Reserved;                                 /*0C0*/
  ULONG CurrentLocale;                                 /*0C4*/
  ULONG FpSoftwareStatusRegister;                      /*0C8*/
  UCHAR SystemReserved1[0xD8];                         /*0CC*/
  ULONG Spare1;                                        /*1A4*/
  ULONG ExceptionCode;                                 /*1A8*/
  UCHAR SpareBytes1[0x28];                             /*1AC*/
  UCHAR SystemReserved2[0x28];                         /*1D4*/
  UCHAR GdiTebBatch[0x4E0];                            /*1FC*/
  ULONG gdiRgn;                                        /*6DC*/
  ULONG gdiPen;                                        /*6E0*/
  ULONG gdiBrush;                                      /*6E4*/
  CLIENT_ID RealClientId;                              /*6E8*/
  ULONG GdiCachedProcessHandle;                        /*6F0*/
  ULONG GdiClientPID;                                  /*6F4*/
  ULONG GdiClientTID;                                  /*6F8*/
  ULONG GdiThreadLocalInfo;                            /*6FC*/
  UCHAR UserReserved[0x14];                            /*700*/
  UCHAR glDispatchTable[0x460];                        /*714*/
  UCHAR glReserved1[0x68];                             /*B74*/
  ULONG glReserved2;                                   /*BDC*/
  ULONG glSectionInfo;                                 /*BE0*/
  ULONG glSection;                                     /*BE4*/
  ULONG glTable;                                       /*BE8*/
  ULONG glCurrentRC;                                   /*BEC*/
  ULONG glContext;                                     /*BF0*/
  ULONG LastStatusValue;                               /*BF4*/
  LARGE_INTEGER StaticUnicodeString;                   /*BF8*/
  UCHAR StaticUnicodeBuffer[0x20C];                    /*C00*/
  ULONG DeallocationStack;                             /*E0C*/
  UCHAR TlsSlots[0x100];                               /*E10*/
  LARGE_INTEGER TlsLinks;                              /*F10*/
  ULONG Vdm;                                           /*F18*/
  ULONG ReservedForNtRpc;                              /*F1C*/
  LARGE_INTEGER DbgSsReserved;                         /*F20*/
  ULONG HardErrorsAreDisabled;                         /*F28*/
  UCHAR Instrumentation[0x40];                         /*F2C*/
  ULONG WinSockData;                                   /*F6C*/
  ULONG GdiBatchCount;                                 /*F70*/
  ULONG Spare2;                                        /*F74*/
  ULONG Spare3;                                        /*F78*/
  ULONG Spare4;                                        /*F7C*/
  ULONG ReservedForOle;                                /*F80*/
  ULONG WaitingOnLoaderLock;                           /*F84*/
} TEB, *PTEB;

typedef struct _KTHREAD // Size: 0x1B0
{
  DISPATCHER_HEADER  Header;             //00
  LIST_ENTRY         MutantListHead;     //10
  ULONG              InitialStack;       //18
  ULONG              StackLimit;         //1c
  TEB*               Teb;                //20
  VOID*              TlsArray;           //24
  ULONG              KernelStack;        //28
  BYTE               DebugActive;        //2c
  BYTE               State;              //2d
  WORD               Alerted;            //2e
  BYTE               Iopl;               //30
  BYTE               NpxState;           //31
  BYTE               Saturation;         //32
  BYTE               Priority;           //33
  KAPC_STATE         ApcState;           //34
  ULONG              ContextSwitches;    //4c
  ULONG              WaitStatus;         //50
  BYTE               WaitIrql;           //54
  BYTE               WaitMode;           //55
  BYTE               WaitNext;           //56
  BYTE               WaitReason;         //57
  ULONG              WaitBlockList;      //58
  LIST_ENTRY         WaitListEntry;      //5c
  ULONG              WaitTime;           //64
  BYTE               BasePriority;       //68
  BYTE               DecrementCount;     //69
  BYTE               PriorityDecrement;  //6a
  BYTE               Quantum;            //6b
  KWAIT_BLOCK        WaitBlock [4];      //6c
  ULONG              LegoData;           //cc
  ULONG              KernelApcDisable;   //d0
  ULONG              UserAffinity;       //d4
  BYTE               SystemAffinityActive;//d8
  BYTE               Pad [3];            //d9
  ULONG              ServiceTable;       //dc
  ULONG              Queue;              //e0
  ULONG              ApcQueueLock;       //e4
  KTIMER             Timer;              //e8
  LIST_ENTRY         QueueListEntry;     //110
  ULONG              Affinity;           //118
  BYTE               Preempted;          //11c
  BYTE               ProcessReadyQueue;  //11d
  BYTE               KernelStackResident;//11e
  BYTE               NextProcessor;      //11f
  ULONG              CallbackStack;      //120
  TEB*               Win32Thread;        //124
  ULONG              TrapFrame;          //128
  APC_STATE_POINTER  ApcStatePointer;    //12c
  BYTE               EnableStackSwap;    //134
  BYTE               LargeStack;         //135
  BYTE               ResourceIndex;      //136
  BYTE               PreviousMode;       //137
  ULONG              KernelTime;         //138
  ULONG              UserTime;           //13c
  KAPC_STATE         SavedApcState;      //140
  BYTE               Alertable;          //158
  BYTE               ApcStateIndex;      //159
  BYTE               ApcQueueable;       //15a
  BYTE               AutoAlignment;      //15b
  ULONG              StackBase;          //15c
  KAPC               SuspendApc;         //160
  KSEMAPHORE         SuspendSemaphore;   //190
  LIST_ENTRY         ThreadListEntry;    //1a4
  BYTE               FreezeCount;        //1ac
  BYTE               SuspendCount;       //1ad
  BYTE               IdealProcessor;     //1ae
  BYTE               DisableBoost;       //1af
} KTHREAD, * PKTHREAD;


typedef struct _ETHREAD                  //size 0x240
{
  KTHREAD            Tcb;                //0
  TIME               CreateTime;         //1b0
  union
  {
    LARGE_INTEGER    ExitTime;           //1b8
    LARGE_INTEGER    LpcReplyChain;
  };
  union
  {
    ULONG            ExitStatus;         //1c0
    ULONG            OfsChain;
  };
  LIST_ENTRY         PostBlockList;      //1c4
  LIST_ENTRY         TerminationPortList;//1cc
  KSPIN_LOCK         ActiveTimerListLock;//1d4
  LIST_ENTRY         ActiveTimerListHead;//1d8
  CLIENT_ID          Cid;                //1e0
  PLARGE_INTEGER     LpcReplySemaphore;  //1e8
  ULONG              LpcReplyMessage;    //1fc
  ULONG              LpcReplyMessageId;  //200
  ULONG              PerformanceCountLow;//204
  PPS_IMPERSONATION_INFO ImpersonationInfo;//208
  LIST_ENTRY         IrpList;            //20c
  TOP_LEVEL_IRP      TopLevelIrp;        //214
  ULONG              ReadClusterSize;    //21c
  UCHAR              ForwardClusterOnly; //220
  UCHAR              DisablePageFaultClustering;//221
  UCHAR              DeadThread;         //222
  UCHAR              HasTerminated;      //223
  ULONG              EventPair;          //224
  ACCESS_MASK        GrantedAccess;      //228
  ULONG              ThreadsProcess;     //22c
  ULONG              StartAddress;       //230
  union
  {
    ULONG            Win32StartAddress;  //234
    ULONG            LpcReceivedMessageId;
  };
  UCHAR              LpcExitThreadCalled;//238
  UCHAR              HardErrorsAreDisabled;//239
  UCHAR              LpcReceivedMsgIdValid;//23a
  UCHAR              ActiveImpersonationInfo;//23b
  ULONG              PerformanceCountHigh;//23c
} ETHREAD, *PETHREAD;

NTSYSAPI
NTSTATUS
NTAPI
ZwFsControlFile (
    IN HANDLE               FileHandle,
    IN HANDLE               Event OPTIONAL,
    IN PIO_APC_ROUTINE      ApcRoutine OPTIONAL,
    IN PVOID                ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN ULONG                FsControlCode,
    IN PVOID                InputBuffer OPTIONAL,
    IN ULONG                InputBufferLength,
    OUT PVOID               OutputBuffer OPTIONAL,
    IN ULONG                OutputBufferLength
);

//#define ZwNotifyChangeDirectoryFile NtNotifyChangeDirectoryFile

NTSYSAPI
NTSTATUS
NTAPI
NtNotifyChangeDirectoryFile (
    IN HANDLE               FileHandle,
    IN HANDLE               Event OPTIONAL,
    IN PIO_APC_ROUTINE      ApcRoutine OPTIONAL,
    IN PVOID                ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    OUT PVOID               Buffer,
    IN ULONG                Length,
    IN ULONG                CompletionFilter,
    IN BOOLEAN              WatchTree
);

NTSYSAPI
PDEVICE_OBJECT
NTAPI
IoGetBaseFileSystemDeviceObject (
    IN PFILE_OBJECT FileObject
);


NTSYSAPI
NTSTATUS
NTAPI
NtQueryDirectoryFile(
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
    );

//#undef IoCallDriver
//NTSYSAPI
//NTSTATUS 
//NTAPI
//IoCallDriver(
//     IN PDEVICE_OBJECT  DeviceObject,
//     IN OUT PIRP  Irp
//     );

typedef struct _DIR_ITEM
{
  struct _DIR_ITEM*  Next;
  PVOID              Object;
} DIR_ITEM, *PDIR_ITEM;

typedef struct _DIRECTORY
{
  PDIR_ITEM  HashEntries[37];
  PDIR_ITEM  LastHashAccess;     //94h
  ULONG      LastHashResult;     //98h
} DIRECTORY, *PDIRECTORY;


typedef struct _OBJECT_NAME
{
  PDIRECTORY     Directory;          // директория, которой принадлежит объект
  UNICODE_STRING ObjectName;         // Имя объекта
  ULONG          Reserved;           // выравнивание
} OBJECT_NAME, *POBJECT_NAME;


typedef struct _OBJECT_SUB_HEADER_INFO
{
  BYTE     NameOffset      : 8;
  BYTE     HandleDB_Offset : 8;
  BYTE     QuotaOffset     : 8;
  BYTE     U               : 1;
  BYTE     H               : 1;
  BYTE     S               : 1;
  BYTE     P               : 1;
  BYTE     E               : 1;
  BYTE     I               : 1;
  BYTE     M               : 1;
  BYTE     Q               : 1;
} OBJECT_SUB_HEADER_INFO, *POBJECT_SUB_HEADER_INFO;

typedef struct _OBJECT_INFO
{
  DWORD    Attributes;         //00h OBJECT_ATTRIBUTES.Attributes
  HANDLE   RootDirectory;      //04h
  DWORD    Reserved;           //08h - Unknown or Res.
  KPROCESSOR_MODE bMode;       //0ch
  BYTE     Reserved1[3];       //0dh - Alignment
  DWORD    PagedPoolQuota;     //10h
  DWORD    NonPagedPoolQuota;  //14h
  DWORD    QotaInformationSize;//18h - размер SID группы
                               //+ размер DACL (округленные)
  PSECURITY_DESCRIPTOR SelfRelSecDescriptor;
                               //1ch - указатель на Self Relativ.
                               //дескриптор безопасности Из Non Paed Pool
  PSECURITY_QUALITY_OF_SERVICE pSecQual;    //20h
  SECURITY_QUALITY_OF_SERVICE SecQuality;   //24h
                               //30h
} OBJECT_INFO,*POBJECT_INFO;

typedef struct _QUOTA_BLOCK
{
  KSPIN_LOCK QuotaLock;
  DWORD RefCounter;            // для скольких процессов этот блок
  DWORD PeakNonPagedPoolUsage;
  DWORD PeakPagedPoolUsage;
  DWORD NonPagedpoolUsage;
  DWORD PagedPoolUsage;
  DWORD NonPagedPoolLimit;
  DWORD PagedPoolLimit;
  DWORD PeakPagefileUsage;
  DWORD PagefileUsage;
  DWORD PageFileLimit;
} QUOTA_BLOCK,*PQUOTA_BLOCK;
  
typedef struct _OBJECT_HEADER
{
  ULONG RefCounter;                     // число ссылок на объект   00
  ULONG HandleCounter;                  // Число хэндлов            04   
  POBJECT_TYPE ObjectType;              // объект-тип               08      
  OBJECT_SUB_HEADER_INFO SubHeaderInfo; // описано ниже             0c  
  union                                 //                          10
  {
    POBJECT_INFO ObjectInfo;
    PQUOTA_BLOCK pQuotaBlock;
  } a;
  PSECURITY_DESCRIPTOR  SecurityDescriptor;           //             14 Optional
} OBJECT_HEADER, *POBJECT_HEADER;

#define SIZE_OF_OBJECT_HEADER (sizeof(OBJECT_HEADER)) //(0x18)

NTSYSAPI
NTSTATUS
NTAPI
ObCreateObject(
    KPROCESSOR_MODE bMode,           // kernel / user = 0
    POBJECT_TYPE Type,               // Типовой объект = IoDriverObjectType
    POBJECT_ATTRIBUTES Attributes,   // Аттрибуты {0x18, 0, {\Driver\Name}, 0x10, 0, 0}
    BOOLEAN bObjectMode,             // Тип объекта kernel/user = 0
    ULONG Reserved,                  // не используется функцией = 0
    ULONG BodySize,                  // размер тела объекта = 0xbc
    ULONG PagedPoolQuota OPTIONAL,   // если 0 = 0
    ULONG NonPagedPoolQuota OPTIONAL,// то наследуется = 0
    PVOID* pObjectBody               // возвращаемый указатель на тело.
    );

NTSYSAPI
NTSTATUS
NTAPI
ObInsertObject(
    PVOID pObject,                      //Тело
    PACCESS_STATE pAccessState OPTIONAL,  // = 0
    ACCESS_MASK Access,                   // = 1
    ULONG RefCounterDelta OPTIONAL,   //0- default (т.е. 1) = 0
    PVOID  OUT *ObjectExist OPTIONAL, //Если уже существует = 0
    PHANDLE OUT Handle                //хэндл
    );

extern PVOID IoDriverObjectType;

typedef enum _SYSTEMINFOCLASS
{
  SystemBasicInformation,             // 0x002C
  SystemProcessorInformation,         // 0x000C
  SystemPerformanceInformation,       // 0x0138
  SystemTimeInformation,              // 0x0020
  SystemPathInformation,              // not implemented
  SystemProcessInformation,           // 0x00C8+ per process
  SystemCallInformation,              // 0x0018 + (n * 0x0004)
  SystemConfigurationInformation,     // 0x0018
  SystemProcessorCounters,            // 0x0030 per cpu
  SystemGlobalFlag,                   // 0x0004
  SystemInfo10,                       // not implemented
  SystemModuleInformation,            // 0x0004 + (n * 0x011C)
  SystemLockInformation,              // 0x0004 + (n * 0x0024)
  SystemInfo13,                       // not implemented
  SystemPagedPoolInformation,         // checked build only
  SystemNonPagedPoolInformation,      // checked build only
  SystemHandleInformation,            // 0x0004  + (n * 0x0010)
  SystemObjectInformation,            // 0x0038+ + (n * 0x0030+)
  SystemPageFileInformation,          // 0x0018+ per page file
  SystemInstemulInformation,          // 0x0088
  SystemInfo20,                       // invalid info class
  SystemCacheInformation,             // 0x0024
  SystemPoolTagInformation,           // 0x0004 + (n * 0x001C)
  SystemInfo23,                       // 0x0000, or 0x0018 per cpu
  SystemDpcInformation,               // 0x0014
  SystemInfo25,                       // checked build only
  SystemLoadDriver,                   // 0x0018, set mode only
  SystemUnloadDriver,                 // 0x0004, set mode only
  SystemTimeAdjustmentInformation,    // 0x000C, 0x0008 writeable
  SystemInfo29,                       // checked build only
  SystemInfo30,                       // checked build only
  SystemInfo31,                       // checked build only
  SystemCrashDumpInformation,         // 0x0004
  SystemInfo33,                       // 0x0010
  SystemCrashDumpStateInformation,    // 0x0004
  SystemDebuggerInformation,          // 0x0002
  SystemThreadSwitchInformation,      // 0x0030
  SystemRegistryQuotaInformation,     // 0x000C
  SystemAddDriver,                    // 0x0008, set mode only
  SystemPrioritySeparationInformation,// 0x0004, set mode only
  SystemInfo40,                       // not implemented
  SystemInfo41,                       // not implemented
  SystemInfo42,                       // invalid info class
  SystemInfo43,                       // invalid info class
  SystemTimeZoneInformation,          // 0x00AC
  SystemLookasideInformation,         // n * 0x0020
  MaxSystemInfoClass
}
SYSTEMINFOCLASS, *PSYSTEMINFOCLASS, **PPSYSTEMINFOCLASS;

// -----------------------------------------------------------------
// 26: SystemLoadDriver (set mode only)
//     see MmLoadSystemImage()
//     user mode: STATUS_PRIVILEGE_NOT_HELD returned

typedef struct _SYSTEM_LOAD_DRIVER
{
  UNICODE_STRING usImageFile;     // input
  PVOID          pBaseAddress;    // output
  HANDLE         hSystemImage;    // output
  PVOID          pEntryPoint;     // output
  PVOID          pDirectoryEntry; // output
}
        SYSTEM_LOAD_DRIVER,
     * PSYSTEM_LOAD_DRIVER,
    **PPSYSTEM_LOAD_DRIVER;

#define SYSTEM_LOAD_DRIVER_ \
        sizeof (SYSTEM_LOAD_DRIVER)

// -----------------------------------------------------------------
// 27: SystemUnloadDriver (set mode only)
//     see MmUnloadSystemImage()
//     user mode: STATUS_PRIVILEGE_NOT_HELD returned

typedef struct _SYSTEM_UNLOAD_DRIVER
{
  HANDLE hSystemImage;            // received via SystemLoadDriver
}
        SYSTEM_UNLOAD_DRIVER,
     * PSYSTEM_UNLOAD_DRIVER,
    **PPSYSTEM_UNLOAD_DRIVER;

#define SYSTEM_UNLOAD_DRIVER_ \
        sizeof (SYSTEM_UNLOAD_DRIVER)


NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    SYSTEMINFOCLASS sic,
    PVOID           pData,
    ULONG           dSize,
    ULONG*          pdSize
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemInformation(
    SYSTEMINFOCLASS sic,
    PVOID           pData,
    ULONG           dSize
    );

NTSYSAPI
BOOLEAN
NTAPI
IoIsOperationSynchronous(
    IN PIRP Irp
    );

NTSYSAPI
NTSTATUS
NTAPI
NtClose(
    IN HANDLE Handle
    );

/*                  
NTSTATUS 
FASTCALL
_IofCallDriverNative(
     IN PDEVICE_OBJECT  DeviceObject,
     IN OUT PIRP  Irp
     )
{
  --(Irp->CurrentLocation);
  if (Irp->CurrentLocation <= 0)
  {
    DbgPrint ("_IofCallDriverNative: NO_MORE_IRP_STACK_LOCATIONS => DeviceObject = %08x (%u)\n", DeviceObject, Irp->CurrentLocation);
    KeBugCheckEx(NO_MORE_IRP_STACK_LOCATIONS, (ULONG)Irp, 0, 0, 0);
  }

  --(Irp->Tail.Overlay.CurrentStackLocation);

  Irp->Tail.Overlay.CurrentStackLocation->DeviceObject = DeviceObject;
  return DeviceObject->DriverObject->MajorFunction[Irp->Tail.Overlay.CurrentStackLocation->MajorFunction](DeviceObject, Irp);
}
*/

} // extern "C"

#endif //__NT_OS_KERNEL_UNDOCUMENT__
