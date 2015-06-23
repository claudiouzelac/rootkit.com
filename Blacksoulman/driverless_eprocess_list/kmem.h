

#define PAGE_SIZE 0x1000

#define LONG_PTR LONG

#define NtCurrentProcess() ( (HANDLE)(LONG_PTR) -1 ) 

PVOID DisableProt(BOOL mode); 

typedef struct _SRVTABLE {
 PVOID          *ServiceTableBase;
 PVOID ServiceCounterTable;
 unsigned int NumberOfServices;
 PVOID  ArgTable;
} SRVTABLE, *PSRVTABLE;

PSRVTABLE KeServiceDescriptorTable;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _ANSI_STRING {
	WORD Length;
	WORD MaxLength;
	PSTR Buffer;
} ANSI_STRING, *PANSI_STRING, STRING, *PSTRING;

typedef const char *PCSZ, *PCSTR, *LPCSTR;

#define OBJ_CASE_INSENSITIVE 0x00000040L
#define OBJ_KERNEL_HANDLE    0x00000200L

typedef LONG NTSTATUS;
#define STATUS_SUCCESS       (NTSTATUS) 0x00000000L
#define STATUS_ACCESS_DENIED (NTSTATUS) 0xC0000022L

#define MAKE_DWORD(_l, _h) (DWORD) (_l | (_h << 16))

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

// useful macros
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );        \
    (p)->RootDirectory = r;                           \
    (p)->Attributes = a;                              \
    (p)->ObjectName = n;                              \
    (p)->SecurityDescriptor = s;                      \
    (p)->SecurityQualityOfService = NULL;             \
    }

#define INIT_UNICODE(_var,_buffer)            \
        UNICODE_STRING _var = {               \
            sizeof (_buffer) - sizeof (WORD), \
            sizeof (_buffer),                 \
            _buffer }

// callgate info
typedef struct _KGDTENTRY {
   WORD LimitLow;
   WORD BaseLow;
   WORD BaseHigh;
} KGDTENTRY, *PKGDTENTRY;

typedef struct _CALLGATE_DESCRIPTOR {
   USHORT offset_0_15;
   USHORT selector;
   UCHAR  param_count :4;
   UCHAR  some_bits   :4;
   UCHAR  type        :4;
   UCHAR  app_system  :1;
   UCHAR  dpl         :2;
   UCHAR  present     :1;
   USHORT offset_16_31;
} CALLGATE_DESCRIPTOR, *PCALLGATE_DESCRIPTOR;

// section info
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

typedef struct _MAPPING {
/*000*/ PHYSICAL_ADDRESS pAddress;
/*008*/ PVOID            vAddress;
/*00C*/ DWORD            Offset;
/*010*/ } MAPPING, *PMAPPING;

// symlink info
#define SYMBOLIC_LINK_QUERY                              (0x0001)
#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

// process info
// Flink to _EPROCESS
#define TO_EPROCESS(_a) ((DWORD) _a - 0x88)// 0A0 win2k
// Flink to UniqueProcessId
#define TO_PID(_a) (DWORD) ((DWORD) _a - 0x4)
// Flink to ImageFileName
#define TO_PNAME(_a) (PCHAR) ((DWORD) _a + 0x15C)

typedef struct _DISPATCHER_HEADER {
/*000*/ UCHAR Type;
/*001*/ UCHAR Absolute;
/*002*/ UCHAR Size;
/*003*/ UCHAR Inserted;
/*004*/ LONG SignalState;
/*008*/ LIST_ENTRY WaitListHead;
/*010*/ } DISPATCHER_HEADER;

typedef struct _KEVENT {
/*000*/ DISPATCHER_HEADER Header;
/*010*/ } KEVENT, *PKEVENT;

typedef struct _FAST_MUTEX {
/*000*/ LONG Count;
/*004*/ PVOID Owner;
/*008*/ ULONG Contention;
/*00C*/ KEVENT Event;
/*01C*/ ULONG OldIrql;
/*020*/ } FAST_MUTEX, *PFAST_MUTEX;

// the two following definition come from w2k_def.h by Sven B. Schreiber
typedef struct _MMSUPPORT {
/*000*/ LARGE_INTEGER LastTrimTime;
/*008*/ DWORD         LastTrimFaultCount;
/*00C*/ DWORD         PageFaultCount;
/*010*/ DWORD         PeakWorkingSetSize;
/*014*/ DWORD         WorkingSetSize;
/*018*/ DWORD         MinimumWorkingSetSize;
/*01C*/ DWORD         MaximumWorkingSetSize;
/*020*/ PVOID         VmWorkingSetList;
/*024*/ LIST_ENTRY    WorkingSetExpansionLinks;
/*02C*/ BOOLEAN       AllowWorkingSetAdjustment;
/*02D*/ BOOLEAN       AddressSpaceBeingDeleted;
/*02E*/ BYTE          ForegroundSwitchCount;
/*02F*/ BYTE          MemoryPriority;
/*030*/ } MMSUPPORT, *PMMSUPPORT;

typedef struct _IO_COUNTERS {
/*000*/ ULONGLONG  ReadOperationCount;
/*008*/ ULONGLONG  WriteOperationCount;
/*010*/ ULONGLONG  OtherOperationCount;
/*018*/ ULONGLONG ReadTransferCount;
/*020*/ ULONGLONG WriteTransferCount;
/*028*/ ULONGLONG OtherTransferCount;
/*030*/ } IO_COUNTERS, *PIO_COUNTERS;

// this is a very simplified version :) of the EPROCESS
// structure.

typedef struct _EPROCESS {
/*000*/ BYTE                   Pcb[0x6C];
/*06C*/ NTSTATUS               ExitStatus;
/*070*/ KEVENT                 LockEvent;
/*080*/ DWORD                  LockCount;
/*084*/ DWORD                  dw084;
/*088*/ LARGE_INTEGER          CreateTime;
/*090*/ LARGE_INTEGER          ExitTime;
/*098*/ PVOID                  LockOwner;
/*09C*/ DWORD                  UniqueProcessId;
/*0A0*/ LIST_ENTRY             ActiveProcessLinks; // see PsActiveListHead
/*0A8*/ DWORD                  QuotaPeakPoolUsage[2]; // NP, P
/*0B0*/ DWORD                  QuotaPoolUsage[2]; // NP, P
/*0B8*/ DWORD                  PagefileUsage;
/*0BC*/ DWORD                  CommitCharge;
/*0C0*/ DWORD                  PeakPagefileUsage;
/*0C4*/ DWORD                  PeakVirtualSize;
/*0C8*/ LARGE_INTEGER          VirtualSize;
/*0D0*/ MMSUPPORT              Vm;
/*100*/ LIST_ENTRY             SessionProcessLinks;
/*108*/ DWORD                  dw108[6];
/*120*/ PVOID                  DebugPort;
/*124*/ PVOID                  ExceptionPort;
/*128*/ PVOID                  ObjectTable;
/*12C*/ PVOID                  Token;
/*130*/ FAST_MUTEX             WorkingSetLock;
/*150*/ DWORD                  WorkingSetPage;
/*154*/ BOOLEAN                ProcessOutswapEnabled;
/*155*/ BOOLEAN                ProcessOutswapped;
/*156*/ BOOLEAN                AddressSpaceInitialized;
/*157*/ BOOLEAN                AddressSpaceDeleted;
/*158*/ FAST_MUTEX             AddressCreationLock;
/*178*/ KSPIN_LOCK             HyperSpaceLock;
/*17C*/ DWORD                  ForkInProgress;
/*180*/ WORD                   VmOperation;
/*182*/ BOOLEAN                ForkWasSuccessful;
/*183*/ BYTE                   MmAgressiveWsTrimMask;
/*184*/ DWORD                  VmOperationEvent;
/*188*/ PVOID                  PaeTop;
/*18C*/ DWORD                  LastFaultCount;
/*190*/ DWORD                  ModifiedPageCount;
/*194*/ PVOID                  VadRoot;
/*198*/ PVOID                  VadHint;
/*19C*/ PVOID                  CloneRoot;
/*1A0*/ DWORD                  NumberOfPrivatePages;
/*1A4*/ DWORD                  NumberOfLockedPages;
/*1A8*/ WORD                   NextPageColor;
/*1AA*/ BOOLEAN                ExitProcessCalled;
/*1AB*/ BOOLEAN                CreateProcessReported;
/*1AC*/ HANDLE                 SectionHandle;
/*1B0*/ PVOID                  Peb;
/*1B4*/ PVOID                  SectionBaseAddress;
/*1B8*/ PVOID                  QuotaBlock;
/*1BC*/ NTSTATUS               LastThreadExitStatus;
/*1C0*/ DWORD                  WorkingSetWatch;
/*1C4*/ HANDLE                 Win32WindowStation;
/*1C8*/ DWORD                  InheritedFromUniqueProcessId;
/*1CC*/ ACCESS_MASK            GrantedAccess;
/*1D0*/ DWORD                  DefaultHardErrorProcessing; // HEM_*
/*1D4*/ DWORD                  LdtInformation;
/*1D8*/ PVOID                  VadFreeHint;
/*1DC*/ DWORD                  VdmObjects;
/*1E0*/ PVOID                  DeviceMap;
/*1E4*/ DWORD                  SessionId;
/*1E8*/ LIST_ENTRY             PhysicalVadList;
/*1F0*/ PVOID                  PageDirectoryPte;
/*1F4*/ DWORD                  dw1F4;
/*1F8*/ DWORD                  PaePageDirectoryPage;
/*1FC*/ CHAR                   ImageFileName[16];
/*20C*/ DWORD                  VmTrimFaultValue;
/*210*/ BYTE                   SetTimerResolution;
/*211*/ BYTE                   PriorityClass;
/*212*/ WORD                   SubSystemVersion;
/*214*/ PVOID                  Win32Process;
/*218*/ PVOID                  Job;
/*21C*/ DWORD                  JobStatus;
/*220*/ LIST_ENTRY             JobLinks;
/*228*/ PVOID                  LockedPagesList;
/*22C*/ PVOID                  SecurityPort;
/*230*/ PVOID                  Wow64;
/*234*/ DWORD                  dw234;
/*238*/ IO_COUNTERS            IoCounters;
/*268*/ DWORD                  CommitChargeLimit;
/*26C*/ DWORD                  CommitChargePeak;
/*270*/ LIST_ENTRY             ThreadListHead;
/*278*/ PVOID                  VadPhysicalPagesBitMap;
/*27C*/ DWORD                  VadPhysicalPages;
/*280*/ DWORD                  AweLock;
/*284*/ } EPROCESS, *PEPROCESS;

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType

    // end_wdm
    ,
    //
    // Note these per session types are carefully chosen so that the appropriate
    // masking still applies as well as MaxPoolType above.
    //

    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,

    // begin_wdm

    } POOL_TYPE;


// copy ntdll.lib from Microsoft DDK to current directory
#pragma comment(lib, "ntdll")
#define IMP_SYSCALL __declspec(dllimport) NTSTATUS _stdcall

ULONG	(*DbgPrint)	(    PCH Format,  ...    );




NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
    PUNICODE_STRING UnicodeString
    );

NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
    PANSI_STRING AnsiString
    );
	
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    PANSI_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
    );

NTSYSAPI
VOID
NTAPI
RtlInitAnsiString(
    PANSI_STRING DestinationString,
    PCSZ SourceString
    );

NTSYSAPI
VOID
NTAPI
 RtlInitUnicodeString(
    IN OUT PUNICODE_STRING  DestinationString,
    IN PCWSTR  SourceString
    );


IMP_SYSCALL
NtMapViewOfSection(HANDLE SectionHandle,
                   HANDLE ProcessHandle,
                   PVOID *BaseAddress,
                   ULONG ZeroBits,
                   ULONG CommitSize,
                   PLARGE_INTEGER SectionOffset,
                   PSIZE_T ViewSize,
                   SECTION_INHERIT InheritDisposition,
                   ULONG AllocationType,
                   ULONG Protect);

IMP_SYSCALL
NtUnmapViewOfSection(HANDLE ProcessHandle,
                     PVOID BaseAddress);

IMP_SYSCALL
NtOpenSection(PHANDLE SectionHandle,
              ACCESS_MASK DesiredAccess,
              POBJECT_ATTRIBUTES ObjectAttributes);

IMP_SYSCALL
NtClose(HANDLE Handle);

IMP_SYSCALL
NtCreateSymbolicLinkObject(PHANDLE SymLinkHandle,
                           ACCESS_MASK DesiredAccess,
                           POBJECT_ATTRIBUTES ObjectAttributes,
                           PUNICODE_STRING TargetName);


IMP_SYSCALL
NtCreateProcess(

  OUT PHANDLE           ProcessHandle,
  IN ACCESS_MASK        DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN HANDLE             ParentProcess,
  IN BOOLEAN            InheritObjectTable,
  IN HANDLE             SectionHandle OPTIONAL,
  IN HANDLE             DebugPort OPTIONAL,
  IN HANDLE             ExceptionPort OPTIONAL );

IMP_SYSCALL
LdrLoadDll(

  IN PWCHAR               PathToFile OPTIONAL,
  IN ULONG                Flags OPTIONAL,
  IN PUNICODE_STRING      ModuleFileName,
  OUT PHANDLE             ModuleHandle );

NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandle(

  IN PWORD                pwPath OPTIONAL,
  IN PVOID                Unused OPTIONAL,
  IN PUNICODE_STRING      ModuleFileName,
  OUT PHANDLE             pHModule );


NTSYSAPI
NTSTATUS
NTAPI
LdrUnloadDll(

  IN HANDLE               ModuleHandle );


IMP_SYSCALL
LdrGetProcedureAddress(

  IN HMODULE              ModuleHandle,
  IN PANSI_STRING         FunctionName OPTIONAL,
  IN WORD                 Oridinal OPTIONAL,
  OUT PVOID               *FunctionAddress );


