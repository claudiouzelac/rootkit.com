// Interface unit for the Windows NT Native API
// Copyright (C) 1999, 2000 Marcel van Brakel

unit Native;

interface

uses
 JWaWinBase, JwaWinNT, JwaWinType;

{$I WINDEFINES.INC}

//------------------------------------------------------------------------------

// Temporaries from NTDDK.H to be removed when fully converted.

type
  _CLIENT_ID = record
    UniqueProcess: HANDLE;
    UniqueThread: HANDLE;
  end;
  CLIENT_ID = _CLIENT_ID;
  PCLIENT_ID = ^CLIENT_ID;
  TClientID = CLIENT_ID;
  PClientID = ^TClientID;

  KPRIORITY = LONG;

  _KWAIT_REASON = (
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrPoolAllocation,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrEventPair,
    WrQueue,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    WrRendezvous,
    Spare2,
    Spare3,
    Spare4,
    Spare5,
    Spare6,
    WrKernel,
    MaximumWaitReason);
  KWAIT_REASON = _KWAIT_REASON;

  _VM_COUNTERS = record
    PeakVirtualSize: SIZE_T;
    VirtualSize: SIZE_T;
    PageFaultCount: ULONG;
    PeakWorkingSetSize: SIZE_T;
    WorkingSetSize: SIZE_T;
    QuotaPeakPagedPoolUsage: SIZE_T;
    QuotaPagedPoolUsage: SIZE_T;
    QuotaPeakNonPagedPoolUsage: SIZE_T;
    QuotaNonPagedPoolUsage: SIZE_T;
    PagefileUsage: SIZE_T;
    PeakPagefileUsage: SIZE_T;
  end;
  VM_COUNTERS = _VM_COUNTERS;
  PVM_COUNTERS = ^VM_COUNTERS;

const
  NonPagedPool = 0;
  PagedPool = 1;
  NonPagedPoolMustSucceed = 2;
  DontUseThisType = 3;
  NonPagedPoolCacheAligned = 4;
  PagedPoolCacheAligned = 5;
  NonPagedPoolCacheAlignedMustS = 6;
  MaxPoolType = 7;
  NonPagedPoolSession = 32;
  PagedPoolSession = NonPagedPoolSession + 1;
  NonPagedPoolMustSucceedSession = PagedPoolSession + 1;
  DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1;
  NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1;
  PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1;
  NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1;

type
  POOL_TYPE = NonPagedPool..NonPagedPoolCacheAlignedMustSSession;

  _IO_STATUS_BLOCK = record
    //union {
    Status: NTSTATUS;
    //    PVOID Pointer;
    //}
    Information: ULONG_PTR;
  end;
  IO_STATUS_BLOCK = _IO_STATUS_BLOCK;
  PIO_STATUS_BLOCK = ^IO_STATUS_BLOCK;

const
  ViewShare = 1;
  ViewUnmap = 2;

type
  SECTION_INHERIT = ViewShare..ViewUnmap;

  _THREADINFOCLASS = (
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    MaxThreadInfoClass);
  THREADINFOCLASS = _THREADINFOCLASS;

  KAFFINITY = ULONG;
  PKAFFINITY = ^KAFFINITY;

  PKNORMAL_ROUTINE = procedure (NormalContext, SystemArgument1, SystemArgument2: PVOID); stdcall;

  _PROCESSINFOCLASS = (
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,          // Note: this is kernel mode only
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    MaxProcessInfoClass);
  PROCESSINFOCLASS = _PROCESSINFOCLASS;

  _KPROFILE_SOURCE = (
    ProfileTime,
    ProfileAlignmentFixup,
    ProfileTotalIssues,
    ProfilePipelineDry,
    ProfileLoadInstructions,
    ProfilePipelineFrozen,
    ProfileBranchInstructions,
    ProfileTotalNonissues,
    ProfileDcacheMisses,
    ProfileIcacheMisses,
    ProfileCacheMisses,
    ProfileBranchMispredictions,
    ProfileStoreInstructions,
    ProfileFpInstructions,
    ProfileIntegerInstructions,
    Profile2Issue,
    Profile3Issue,
    Profile4Issue,
    ProfileSpecialInstructions,
    ProfileTotalCycles,
    ProfileIcacheIssues,
    ProfileDcacheAccesses,
    ProfileMemoryBarrierCycles,
    ProfileLoadLinkedIssues,
    ProfileMaximum);
  KPROFILE_SOURCE = _KPROFILE_SOURCE;

  PIO_APC_ROUTINE = procedure (ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; Reserved: ULONG); stdcall;

  _FILE_FULL_EA_INFORMATION = record
    NextEntryOffset: ULONG;
    Flags: UCHAR;
    EaNameLength: UCHAR;
    EaValueLength: USHORT;
    EaName: array [0..0] of CHAR;
  end;
  FILE_FULL_EA_INFORMATION = _FILE_FULL_EA_INFORMATION;
  PFILE_FULL_EA_INFORMATION = ^FILE_FULL_EA_INFORMATION;

  _FSINFOCLASS = (
    FileFsFiller0,
    FileFsVolumeInformation,     // 1
    FileFsLabelInformation,      // 2
    FileFsSizeInformation,       // 3
    FileFsDeviceInformation,     // 4
    FileFsAttributeInformation,  // 5
    FileFsControlInformation,    // 6
    FileFsFullSizeInformation,   // 7
    FileFsObjectIdInformation,   // 8
    FileFsMaximumInformation);
  FS_INFORMATION_CLASS = _FSINFOCLASS;
  PFS_INFORMATION_CLASS = ^FS_INFORMATION_CLASS;

  UUID = GUID;

  _FILE_BASIC_INFORMATION = record
    CreationTime: LARGE_INTEGER;
    LastAccessTime: LARGE_INTEGER;
    LastWriteTime: LARGE_INTEGER;
    ChangeTime: LARGE_INTEGER;
    FileAttributes: ULONG;
  end;
  FILE_BASIC_INFORMATION = _FILE_BASIC_INFORMATION;
  PFILE_BASIC_INFORMATION = ^FILE_BASIC_INFORMATION;

  _FILE_NETWORK_OPEN_INFORMATION = record
    CreationTime: LARGE_INTEGER;
    LastAccessTime: LARGE_INTEGER;
    LastWriteTime: LARGE_INTEGER;
    ChangeTime: LARGE_INTEGER;
    AllocationSize: LARGE_INTEGER;
    EndOfFile: LARGE_INTEGER;
    FileAttributes: ULONG;
  end;
  FILE_NETWORK_OPEN_INFORMATION = _FILE_NETWORK_OPEN_INFORMATION;
  PFILE_NETWORK_OPEN_INFORMATION = ^FILE_NETWORK_OPEN_INFORMATION;

  _FILE_INFORMATION_CLASS = (
    FileFiller0,
    FileDirectoryInformation,     // 1
    FileFullDirectoryInformation, // 2
    FileBothDirectoryInformation, // 3
    FileBasicInformation,         // 4  wdm
    FileStandardInformation,      // 5  wdm
    FileInternalInformation,      // 6
    FileEaInformation,            // 7
    FileAccessInformation,        // 8
    FileNameInformation,          // 9
    FileRenameInformation,        // 10
    FileLinkInformation,          // 11
    FileNamesInformation,         // 12
    FileDispositionInformation,   // 13
    FilePositionInformation,      // 14 wdm
    FileFullEaInformation,        // 15
    FileModeInformation,          // 16
    FileAlignmentInformation,     // 17
    FileAllInformation,           // 18
    FileAllocationInformation,    // 19
    FileEndOfFileInformation,     // 20 wdm
    FileAlternateNameInformation, // 21
    FileStreamInformation,        // 22
    FilePipeInformation,          // 23
    FilePipeLocalInformation,     // 24
    FilePipeRemoteInformation,    // 25
    FileMailslotQueryInformation, // 26
    FileMailslotSetInformation,   // 27
    FileCompressionInformation,   // 28
    FileObjectIdInformation,      // 29
    FileCompletionInformation,    // 30
    FileMoveClusterInformation,   // 31
    FileQuotaInformation,         // 32
    FileReparsePointInformation,  // 33
    FileNetworkOpenInformation,   // 34
    FileAttributeTagInformation,  // 35
    FileTrackingInformation,      // 36
    FileMaximumInformation);
  FILE_INFORMATION_CLASS = _FILE_INFORMATION_CLASS;
  PFILE_INFORMATION_CLASS = ^FILE_INFORMATION_CLASS;

  _FILE_STANDARD_INFORMATION = record
    AllocationSize: LARGE_INTEGER;
    EndOfFile: LARGE_INTEGER;
    NumberOfLinks: ULONG;
    DeletePending: ByteBool;
    Directory: ByteBool;
  end;
  FILE_STANDARD_INFORMATION = _FILE_STANDARD_INFORMATION;
  PFILE_STANDARD_INFORMATION = ^FILE_STANDARD_INFORMATION;

  _FILE_POSITION_INFORMATION = record
    CurrentByteOffset: LARGE_INTEGER;
  end;
  FILE_POSITION_INFORMATION = _FILE_POSITION_INFORMATION;
  PFILE_POSITION_INFORMATION = ^FILE_POSITION_INFORMATION;

  _FILE_ALIGNMENT_INFORMATION = record
    AlignmentRequirement: ULONG;
  end;
  FILE_ALIGNMENT_INFORMATION = _FILE_ALIGNMENT_INFORMATION;
  PFILE_ALIGNMENT_INFORMATION = ^FILE_ALIGNMENT_INFORMATION;

  _KEY_SET_INFORMATION_CLASS = (KeyWriteTimeInformation);
  KEY_SET_INFORMATION_CLASS = _KEY_SET_INFORMATION_CLASS;

  _KEY_INFORMATION_CLASS = (
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation,
    KeyNameInformation);
  KEY_INFORMATION_CLASS = _KEY_INFORMATION_CLASS;

  _KEY_BASIC_INFORMATION = record
		LastWriteTime: LARGE_INTEGER;
		TitleIndex: ULONG;
		NameLength: ULONG;
		Name: array [0..MAX_PATH] of WideChar;
  end;
	KEY_BASIC_INFORMATION = _KEY_BASIC_INFORMATION;
  PKEY_BASIC_INFORMATION = ^KEY_BASIC_INFORMATION;

  _KEY_VALUE_INFORMATION_CLASS = (
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64);
  KEY_VALUE_INFORMATION_CLASS = _KEY_VALUE_INFORMATION_CLASS;

  _KEY_VALUE_BASIC_INFORMATION = record
		TitleIndex: ULONG;
		ValueType: ULONG;
		NameLength: ULONG;
		Name: array [0..MAX_PATH] of WideChar;
  end;
	KEY_VALUE_BASIC_INFORMATION = _KEY_VALUE_BASIC_INFORMATION;
  PKEY_VALUE_BASIC_INFORMATION = ^KEY_VALUE_BASIC_INFORMATION;

  _KEY_VALUE_ENTRY = record
    ValueName: PUNICODE_STRING;
    DataLength: ULONG;
    DataOffset: ULONG;
    Type_: ULONG;
  end;
  KEY_VALUE_ENTRY = _KEY_VALUE_ENTRY;
  PKEY_VALUE_ENTRY = ^KEY_VALUE_ENTRY;

  _DEVICE_POWER_STATE = (
    PowerDeviceUnspecified,
    PowerDeviceD0,
    PowerDeviceD1,
    PowerDeviceD2,
    PowerDeviceD3,
    PowerDeviceMaximum);
  DEVICE_POWER_STATE = _DEVICE_POWER_STATE;
  PDEVICE_POWER_STATE = ^DEVICE_POWER_STATE;

  POWER_ACTION =(
    PowerActionNone,
    PowerActionReserved,
    PowerActionSleep,
    PowerActionHibernate,
    PowerActionShutdown,
    PowerActionShutdownReset,
    PowerActionShutdownOff,
    PowerActionWarmEject);
  PPOWER_ACTION = ^POWER_ACTION;

  _SYSTEM_POWER_STATE = (
    PowerSystemUnspecified,
    PowerSystemWorking,
    PowerSystemSleeping1,
    PowerSystemSleeping2,
    PowerSystemSleeping3,
    PowerSystemHibernate,
    PowerSystemShutdown,
    PowerSystemMaximum);
  SYSTEM_POWER_STATE = _SYSTEM_POWER_STATE;
  PSYSTEM_POWER_STATE = ^SYSTEM_POWER_STATE;

  POWER_INFORMATION_LEVEL = (
    SystemPowerPolicyAc,
    SystemPowerPolicyDc,
    VerifySystemPolicyAc,
    VerifySystemPolicyDc,
    SystemPowerCapabilities,
    SystemBatteryState,
    SystemPowerStateHandler,
    ProcessorStateHandler,
    SystemPowerPolicyCurrent,
    AdministratorPowerPolicy,
    SystemReserveHiberFile,
    ProcessorInformation,
    SystemPowerInformation);

  _RTL_RANGE = record

    //
    // The start of the range
    //
    Start: ULONGLONG;    // Read only

    //
    // The end of the range
    //
    End_: ULONGLONG;      // Read only

    //
    // Data the user passed in when they created the range
    //
    UserData: PVOID;     // Read/Write

    //
    // The owner of the range
    //
    Owner: PVOID;        // Read/Write

    //
    // User defined flags the user specified when they created the range
    //
    Attributes: UCHAR;    // Read/Write

    //
    // Flags (RTL_RANGE_*)
    //
    Flags: UCHAR;       // Read only
  end;
  RTL_RANGE = _RTL_RANGE;
  PRTL_RANGE = ^RTL_RANGE;

const
  RTL_RANGE_SHARED   = $01;
  RTL_RANGE_CONFLICT = $02;

type
  _RTL_RANGE_LIST = record

    //
    // The list of ranges
    //
    ListHead: LIST_ENTRY;

    //
    // These always come in useful
    //
    Flags: ULONG;        // use RANGE_LIST_FLAG_*

    //
    // The number of entries in the list
    //
    Count: ULONG;

    //
    // Every time an add/delete operation is performed on the list this is
    // incremented.  It is checked during iteration to ensure that the list
    // hasn't changed between GetFirst/GetNext or GetNext/GetNext calls
    //
    Stamp: ULONG;
  end;
  RTL_RANGE_LIST = _RTL_RANGE_LIST;
  PRTL_RANGE_LIST = ^RTL_RANGE_LIST;

  _RANGE_LIST_ITERATOR = record
    RangeListHead: PLIST_ENTRY;
    MergedHead: PLIST_ENTRY;
    Current: PVOID;
    Stamp: ULONG;
  end;
  RTL_RANGE_LIST_ITERATOR = _RANGE_LIST_ITERATOR;
  PRTL_RANGE_LIST_ITERATOR = ^RTL_RANGE_LIST_ITERATOR;


// End of NTDDK.H

//==============================================================================
// NT System Services
//==============================================================================

type
  _SYSTEM_INFORMATION_CLASS = (
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemNotImplemented1,
    SystemProcessesAndThreadsInformation,
    SystemCallCounts,
    SystemConfigurationInformation,
    SystemProcessorTimes,
    SystemGlobalFlag,
    SystemNotImplemented2,               
    SystemModuleInformation,             
    SystemLockInformation,               
    SystemNotImplemented3,               
    SystemNotImplemented4,               
    SystemNotImplemented5,               
    SystemHandleInformation,             
    SystemObjectInformation,             
    SystemPagefileInformation,
    SystemInstructionEmulationCounts,    
    SystemInvalidInfoClass1,             
    SystemCacheInformation,
    SystemPoolTagInformation,            
    SystemProcessorStatistics,           
    SystemDpcInformation,                
    SystemNotImplemented6,               
    SystemLoadImage,                     
    SystemUnloadImage,
    SystemTimeAdjustment,
    SystemNotImplemented7,               
    SystemNotImplemented8,               
    SystemNotImplemented9,               
    SystemCrashDumpInformation,          
    SystemExceptionInformation,          
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,     
    SystemContextSwitchInformation,      
    SystemRegistryQuotaInformation,      
    SystemLoadAndCallImage,              
    SystemPrioritySeparation,            
    SystemNotImplemented10,              
    SystemNotImplemented11,              
    SystemInvalidInfoClass2,             
    SystemInvalidInfoClass3,             
    SystemTimeZoneInformation,           
    SystemLookasideInformation,          
    SystemSetTimeSlipEvent,              
    SystemCreateSession,                 
    SystemDeleteSession,                 
    SystemInvalidInfoClass4,             
    SystemRangeStartInformation,         
    SystemVerifierInformation,           
    SystemAddVerifier,
    SystemSessionProcessesInformation);
  SYSTEM_INFORMATION_CLASS = _SYSTEM_INFORMATION_CLASS;

function NtQuerySystemInformation(SystemInformationClass: SYSTEM_INFORMATION_CLASS; SystemInformation: PVOID; SystemInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;

function NtSetSystemInformation(SystemInformationClass: SYSTEM_INFORMATION_CLASS; SystemInformation: PVOID; SystemInformationLength: ULONG): NTSTATUS; stdcall;

type
  _SYSTEM_BASIC_INFORMATION = record // Information Class 0
    Unknown: ULONG;
    MaximumIncrement: ULONG;
    PhysicalPageSize: ULONG;
    NumberOfPhysicalPages: ULONG;
    LowestPhysicalPage: ULONG;
    HighestPhysicalPage: ULONG;
    AllocationGranularity: ULONG;
    LowestUserAddress: ULONG;
    HighestUserAddress: ULONG;
    ActiveProcessors: ULONG;
    NumberProcessors: UCHAR;
  end;
  SYSTEM_BASIC_INFORMATION = _SYSTEM_BASIC_INFORMATION;
  PSYSTEM_BASIC_INFORMATION = ^SYSTEM_BASIC_INFORMATION;

  _SYSTEM_PROCESSOR_INFORMATION = record // Information Class 1
    ProcessorArchitecture: USHORT;
    ProcessorLevel: USHORT;
    ProcessorRevision: USHORT;
    Unknown: USHORT;
    FeatureBits: ULONG;
  end;
  SYSTEM_PROCESSOR_INFORMATION = _SYSTEM_PROCESSOR_INFORMATION;
  PSYSTEM_PROCESSOR_INFORMATION = ^SYSTEM_PROCESSOR_INFORMATION;

  _SYSTEM_PERFORMANCE_INFORMATION = record // Information Class 2
    IdleTime: LARGE_INTEGER;
    ReadTransferCount: LARGE_INTEGER;
    WriteTransferCount: LARGE_INTEGER;
    OtherTransferCount: LARGE_INTEGER;
    ReadOperationCount: ULONG;
    WriteOperationCount: ULONG;
    OtherOperationCount: ULONG;
    AvailablePages: ULONG;
    TotalCommittedPages: ULONG;
    TotalCommitLimit: ULONG;
    PeakCommitment: ULONG;
    PageFaults: ULONG;
    WriteCopyFaults: ULONG;
    TransistionFaults: ULONG;
    Reserved1: ULONG;
    DemandZeroFaults: ULONG;
    PagesRead: ULONG;
    PageReadIos: ULONG;
    Reserved2: array [0..1] of ULONG;
    PagefilePagesWritten: ULONG;
    PagefilePageWriteIos: ULONG;
    MappedFilePagesWritten: ULONG;
    MappedFilePageWriteIos: ULONG;
    PagedPoolUsage: ULONG;
    NonPagedPoolUsage: ULONG;
    PagedPoolAllocs: ULONG;
    PagedPoolFrees: ULONG;
    NonPagedPoolAllocs: ULONG;
    NonPagedPoolFrees: ULONG;
    TotalFreeSystemPtes: ULONG;
    SystemCodePage: ULONG;
    TotalSystemDriverPages: ULONG;
    TotalSystemCodePages: ULONG;
    SmallNonPagedLookasideListAllocateHits: ULONG;
    SmallPagedLookasideListAllocateHits: ULONG;
    Reserved3: ULONG;
    MmSystemCachePage: ULONG;
    PagedPoolPage: ULONG;
    SystemDriverPage: ULONG;
    FastReadNoWait: ULONG;
    FastReadWait: ULONG;
    FastReadResourceMiss: ULONG;
    FastReadNotPossible: ULONG;
    FastMdlReadNoWait: ULONG;
    FastMdlReadWait: ULONG;
    FastMdlReadResourceMiss: ULONG;
    FastMdlReadNotPossible: ULONG;
    MapDataNoWait: ULONG;
    MapDataWait: ULONG;
    MapDataNoWaitMiss: ULONG;
    MapDataWaitMiss: ULONG;
    PinMappedDataCount: ULONG;
    PinReadNoWait: ULONG;
    PinReadWait: ULONG;
    PinReadNoWaitMiss: ULONG;
    PinReadWaitMiss: ULONG;
    CopyReadNoWait: ULONG;
    CopyReadWait: ULONG;
    CopyReadNoWaitMiss: ULONG;
    CopyReadWaitMiss: ULONG;
    MdlReadNoWait: ULONG;
    MdlReadWait: ULONG;
    MdlReadNoWaitMiss: ULONG;
    MdlReadWaitMiss: ULONG;
    ReadAheadIos: ULONG;
    LazyWriteIos: ULONG;
    LazyWritePages: ULONG;
    DataFlushes: ULONG;
    DataPages: ULONG;
    ContextSwitches: ULONG;
    FirstLevelTbFills: ULONG;
    SecondLevelTbFills: ULONG;
    SystemCalls: ULONG;
  end;
  SYSTEM_PERFORMANCE_INFORMATION = _SYSTEM_PERFORMANCE_INFORMATION;
  PSYSTEM_PERFORMANCE_INFORMATION = ^SYSTEM_PERFORMANCE_INFORMATION;

  _SYSTEM_TIME_OF_DAY_INFORMATION = record // Information Class 3
    BootTime: LARGE_INTEGER;
    CurrentTime: LARGE_INTEGER;
    TimeZoneBias: LARGE_INTEGER;
    CurrentTimeZoneId: ULONG;
  end;
  SYSTEM_TIME_OF_DAY_INFORMATION = _SYSTEM_TIME_OF_DAY_INFORMATION;
  PSYSTEM_TIME_OF_DAY_INFORMATION = ^SYSTEM_TIME_OF_DAY_INFORMATION;

  _IO_COUNTERSEX  = record
    ReadOperationCount: LARGE_INTEGER;
    WriteOperationCount: LARGE_INTEGER;
    OtherOperationCount: LARGE_INTEGER;
    ReadTransferCount: LARGE_INTEGER;
    WriteTransferCount: LARGE_INTEGER;
    OtherTransferCount: LARGE_INTEGER;
  end;
  IO_COUNTERSEX = _IO_COUNTERSEX;
  PIO_COUNTERSEX = ^IO_COUNTERSEX;

  THREAD_STATE = (
    StateInitialized,
    StateReady,
    StateRunning,
    StateStandby,
    StateTerminated,
    StateWait,
    StateTransition,
    StateUnknown);

  _SYSTEM_THREADS = record
    KernelTime: LARGE_INTEGER;
    UserTime: LARGE_INTEGER;
    CreateTime: LARGE_INTEGER;
    WaitTime: ULONG;
    StartAddress: PVOID;
    ClientId: CLIENT_ID;
    Priority: KPRIORITY;
    BasePriority: KPRIORITY;
    ContextSwitchCount: ULONG;
    State: THREAD_STATE;
    WaitReason: KWAIT_REASON;
  end;
  SYSTEM_THREADS = _SYSTEM_THREADS;
  PSYSTEM_THREADS = ^SYSTEM_THREADS;
  TSystemThreads = SYSTEM_THREADS;
  PSystemThreads = PSYSTEM_THREADS;  

  _SYSTEM_PROCESSES = record // Information Class 5
    NextEntryDelta: ULONG;
    ThreadCount: ULONG;
    Reserved1: array [0..5] of ULONG;
    CreateTime: LARGE_INTEGER;
    UserTime: LARGE_INTEGER;
    KernelTime: LARGE_INTEGER;
    ProcessName: UNICODE_STRING;
    BasePriority: KPRIORITY;
    ProcessId: ULONG;
    InheritedFromProcessId: ULONG;
    HandleCount: ULONG;
    // next two were Reserved2: array [0..1] of ULONG; thanks to Nico Bendlin
    SessionId: ULONG;
    Reserved2: ULONG;
    VmCounters: VM_COUNTERS;
    IoCounters: IO_COUNTERSEX;  // Windows 2000 only
    Threads: array [0..0] of SYSTEM_THREADS;
  end;
  SYSTEM_PROCESSES = _SYSTEM_PROCESSES;
  PSYSTEM_PROCESSES = ^SYSTEM_PROCESSES;
  TSystemProcesses = SYSTEM_PROCESSES;
  PSystemProcesses = PSYSTEM_PROCESSES;

  _SYSTEM_CALLS_INFORMATION = record // Information Class 6
    Size: ULONG;
    NumberOfDescriptorTables: ULONG;
    NumberOfRoutinesInTable: array [0..0] of ULONG;
    // ULONG CallCounts[];
  end;
  SYSTEM_CALLS_INFORMATION = _SYSTEM_CALLS_INFORMATION;
  PSYSTEM_CALLS_INFORMATION = ^SYSTEM_CALLS_INFORMATION;

  _SYSTEM_CONFIGURATION_INFORMATION = record // Information Class 7
    DiskCount: ULONG;
    FloppyCount: ULONG;
    CdRomCount: ULONG;
    TapeCount: ULONG;
    SerialCount: ULONG;
    ParallelCount: ULONG;
  end;
  SYSTEM_CONFIGURATION_INFORMATION = _SYSTEM_CONFIGURATION_INFORMATION;
  PSYSTEM_CONFIGURATION_INFORMATION = ^SYSTEM_CONFIGURATION_INFORMATION;

  _SYSTEM_PROCESSOR_TIMES = record // Information Class 8
    IdleTime: LARGE_INTEGER;
    KernelTime: LARGE_INTEGER;
    UserTime: LARGE_INTEGER;
    DpcTime: LARGE_INTEGER;
    InterruptTime: LARGE_INTEGER;
    InterruptCount: ULONG;
  end;
  SYSTEM_PROCESSOR_TIMES = _SYSTEM_PROCESSOR_TIMES;
  PSYSTEM_PROCESSOR_TIMES = ^SYSTEM_PROCESSOR_TIMES;

  _SYSTEM_GLOBAL_FLAG = record // Information Class 9
    GlobalFlag: ULONG;
  end;
  SYSTEM_GLOBAL_FLAG = _SYSTEM_GLOBAL_FLAG;
  PSYSTEM_GLOBAL_FLAG = ^SYSTEM_GLOBAL_FLAG;

  _SYSTEM_MODULE_INFORMATION = record // Information Class 11
    Reserved: array [0..1] of ULONG;
    Base: PVOID;
    Size: ULONG;
    Flags: ULONG;
    Index: USHORT;
    Unknown: USHORT;
    LoadCount: USHORT;
    ModuleNameOffset: USHORT;
    ImageName: array [0..255] of CHAR;
  end;
  SYSTEM_MODULE_INFORMATION = _SYSTEM_MODULE_INFORMATION;
  PSYSTEM_MODULE_INFORMATION = ^SYSTEM_MODULE_INFORMATION;
  TSystemModuleInformation = SYSTEM_MODULE_INFORMATION;
  PSystemModuleInformation = PSYSTEM_MODULE_INFORMATION;

  _SYSTEM_LOCK_INFORMATION = record // Information Class 12
    Address: PVOID;
    Type_: USHORT;
    Reserved1: USHORT;
    ExclusiveOwnerThreadId: ULONG;
    ActiveCount: ULONG;
    ContentionCount: ULONG;
    Reserved2: array [0..1] of ULONG;
    NumberOfSharedWaiters: ULONG;
    NumberOfExclusiveWaiters: ULONG;
  end;
  SYSTEM_LOCK_INFORMATION = _SYSTEM_LOCK_INFORMATION;
  PSYSTEM_LOCK_INFORMATION = ^SYSTEM_LOCK_INFORMATION;

  _SYSTEM_HANDLE_TABLE_ENTRY_INFO = record
    UniqueProcessId: ULONG;
    ObjectTypeIndex: UCHAR;
    HandleAttributes: UCHAR;
    HandleValue: USHORT;
    Object_: PVOID;
    GrantedAccess: ULONG;
  end;
  SYSTEM_HANDLE_TABLE_ENTRY_INFO = _SYSTEM_HANDLE_TABLE_ENTRY_INFO;
  PSYSTEM_HANDLE_TABLE_ENTRY_INFO = ^SYSTEM_HANDLE_TABLE_ENTRY_INFO;

  _SYSTEM_HANDLE_INFORMATION = record // Information Class 16
    NumberOfHandles: ULONG;
    Handles: array [0..1] of SYSTEM_HANDLE_TABLE_ENTRY_INFO;
  end;
  SYSTEM_HANDLE_INFORMATION = _SYSTEM_HANDLE_INFORMATION;
  PSYSTEM_HANDLE_INFORMATION = ^SYSTEM_HANDLE_INFORMATION;

  _SYSTEM_OBJECT_TYPE_INFORMATION = record // Information Class 17
    NextEntryOffset: ULONG;
    ObjectCount: ULONG;
    HandleCount: ULONG;
    TypeNumber: ULONG;
    InvalidAttributes: ULONG;
    GenericMapping: GENERIC_MAPPING;
    ValidAccessMask: ACCESS_MASK;
    PoolType: POOL_TYPE;
    Unknown: UCHAR;
    Name: UNICODE_STRING;
  end;
  SYSTEM_OBJECT_TYPE_INFORMATION = _SYSTEM_OBJECT_TYPE_INFORMATION;
  PSYSTEM_OBJECT_TYPE_INFORMATION = ^SYSTEM_OBJECT_TYPE_INFORMATION;

  _SYSTEM_OBJECT_INFORMATION = record
    NextEntryOffset: ULONG;
    Object_: PVOID;
    CreatorProcessId: ULONG;
    Unknown: USHORT;
    Flags: USHORT;
    PointerCount: ULONG;
    HandleCount: ULONG;
    PagedPoolUsage: ULONG;
    NonPagedPoolUsage: ULONG;
    ExclusiveProcessId: ULONG;
    SecurityDescriptor: PSECURITY_DESCRIPTOR;
    Name: UNICODE_STRING;
  end;
  SYSTEM_OBJECT_INFORMATION = _SYSTEM_OBJECT_INFORMATION;
  PSYSTEM_OBJECT_INFORMATION = ^SYSTEM_OBJECT_INFORMATION;

  _SYSTEM_PAGEFILE_INFORMATION = record // Information Class 18
    NextEntryOffset: ULONG;
    CurrentSize: ULONG;
    TotalUsed: ULONG;
    PeakUsed: ULONG;
    FileName: UNICODE_STRING;
  end;
  SYSTEM_PAGEFILE_INFORMATION = _SYSTEM_PAGEFILE_INFORMATION;
  PSYSTEM_PAGEFILE_INFORMATION = ^SYSTEM_PAGEFILE_INFORMATION;
  TSystemPageFileInformation = SYSTEM_PAGEFILE_INFORMATION;
  PSystemPageFileInformation = PSYSTEM_PAGEFILE_INFORMATION;

  _SYSTEM_INSTRUCTION_EMULATION_INFORMATION = record // Info Class 19
    GenericInvalidOpcode: ULONG;
    TwoByteOpcode: ULONG;
    ESprefix: ULONG;
    CSprefix: ULONG;
    SSprefix: ULONG;
    DSprefix: ULONG;
    FSPrefix: ULONG;
    GSprefix: ULONG;
    OPER32prefix: ULONG;
    ADDR32prefix: ULONG;
    INSB: ULONG;
    INSW: ULONG;
    OUTSB: ULONG;
    OUTSW: ULONG;
    PUSHFD: ULONG;
    POPFD: ULONG;
    INTnn: ULONG;
    INTO: ULONG;
    IRETD: ULONG;
    FloatingPointOpcode: ULONG;
    INBimm: ULONG;
    INWimm: ULONG;
    OUTBimm: ULONG;
    OUTWimm: ULONG;
    INB: ULONG;
    INW: ULONG;
    OUTB: ULONG;
    OUTW: ULONG;
    LOCKprefix: ULONG;
    REPNEprefix: ULONG;
    REPprefix: ULONG;
    CLI: ULONG;
    STI: ULONG;
    HLT: ULONG;
  end;
  SYSTEM_INSTRUCTION_EMULATION_INFORMATION = _SYSTEM_INSTRUCTION_EMULATION_INFORMATION;
  PSYSTEM_INSTRUCTION_EMULATION_INFORMATION = ^SYSTEM_INSTRUCTION_EMULATION_INFORMATION;

  _SYSTEM_CACHE_INFORMATION = record // Information Class 21
    SystemCacheWsSize: ULONG;
    SystemCacheWsPeakSize: ULONG;
    SystemCacheWsFaults: ULONG;
    SystemCacheWsMinimum: ULONG;
    SystemCacheWsMaximum: ULONG;
    TransitionSharedPages: ULONG;
    TransitionSharedPagesPeak: ULONG;
    Reserved: array [0..1] of ULONG;
  end;
  SYSTEM_CACHE_INFORMATION = _SYSTEM_CACHE_INFORMATION;
  PSYSTEM_CACHE_INFORMATION = ^SYSTEM_CACHE_INFORMATION;

  _SYSTEM_POOL_TAG_INFORMATION = record // Information Class 22
    Tag: array [0..3] of CHAR;
    PagedPoolAllocs: ULONG;
    PagedPoolFrees: ULONG;
    PagedPoolUsage: ULONG;
    NonPagedPoolAllocs: ULONG;
    NonPagedPoolFrees: ULONG;
    NonPagedPoolUsage: ULONG;
  end;
  SYSTEM_POOL_TAG_INFORMATION = _SYSTEM_POOL_TAG_INFORMATION;
  PSYSTEM_POOL_TAG_INFORMATION = ^SYSTEM_POOL_TAG_INFORMATION;

  _SYSTEM_PROCESSOR_STATISTICS = record // Information Class 23
    ContextSwitches: ULONG;
    DpcCount: ULONG;
    DpcRequestRate: ULONG;
    TimeIncrement: ULONG;
    DpcBypassCount: ULONG;
    ApcBypassCount: ULONG;
  end;
  SYSTEM_PROCESSOR_STATISTICS = _SYSTEM_PROCESSOR_STATISTICS;
  PSYSTEM_PROCESSOR_STATISTICS = ^SYSTEM_PROCESSOR_STATISTICS;

  _SYSTEM_DPC_INFORMATION = record // Information Class 24
    Reserved: ULONG;
    MaximumDpcQueueDepth: ULONG;
    MinimumDpcRate: ULONG;
    AdjustDpcThreshold: ULONG;
    IdealDpcRate: ULONG;
  end;
  SYSTEM_DPC_INFORMATION = _SYSTEM_DPC_INFORMATION;
  PSYSTEM_DPC_INFORMATION = ^SYSTEM_DPC_INFORMATION;

  _SYSTEM_LOAD_IMAGE = record // Information Class 26
    ModuleName: UNICODE_STRING;
    ModuleBase: PVOID;
    Unknown: PVOID;
    EntryPoint: PVOID;
    ExportDirectory: PVOID;
  end;
  SYSTEM_LOAD_IMAGE = _SYSTEM_LOAD_IMAGE;
  PSYSTEM_LOAD_IMAGE = ^SYSTEM_LOAD_IMAGE;

  _SYSTEM_UNLOAD_IMAGE = record // Information Class 27
    ModuleBase: PVOID;
  end;
  SYSTEM_UNLOAD_IMAGE = _SYSTEM_UNLOAD_IMAGE;
  PSYSTEM_UNLOAD_IMAGE = ^SYSTEM_UNLOAD_IMAGE;

  _SYSTEM_QUERY_TIME_ADJUSTMENT = record // Information Class 28
    TimeAdjustment: ULONG;
    MaximumIncrement: ULONG;
    TimeSynchronization: ByteBool;
  end;
  SYSTEM_QUERY_TIME_ADJUSTMENT = _SYSTEM_QUERY_TIME_ADJUSTMENT;
  PSYSTEM_QUERY_TIME_ADJUSTMENT = ^SYSTEM_QUERY_TIME_ADJUSTMENT;

  _SYSTEM_SET_TIME_ADJUSTMENT = record // Information Class 28
    TimeAdjustment: ULONG;
    TimeSynchronization: ByteBool;
  end;
  SYSTEM_SET_TIME_ADJUSTMENT = _SYSTEM_SET_TIME_ADJUSTMENT;
  PSYSTEM_SET_TIME_ADJUSTMENT = ^SYSTEM_SET_TIME_ADJUSTMENT;

  _SYSTEM_CRASH_DUMP_INFORMATION = record // Information Class 32
    CrashDumpSectionHandle: HANDLE;
    Unknown: HANDLE;  // Windows 2000 only
  end;
  SYSTEM_CRASH_DUMP_INFORMATION = _SYSTEM_CRASH_DUMP_INFORMATION;
  PSYSTEM_CRASH_DUMP_INFORMATION = ^SYSTEM_CRASH_DUMP_INFORMATION;

  _SYSTEM_EXCEPTION_INFORMATION = record // Information Class 33
    AlignmentFixupCount: ULONG;
    ExceptionDispatchCount: ULONG;
    FloatingEmulationCount: ULONG;
    Reserved: ULONG;
  end;
  SYSTEM_EXCEPTION_INFORMATION = _SYSTEM_EXCEPTION_INFORMATION;
  PSYSTEM_EXCEPTION_INFORMATION = ^SYSTEM_EXCEPTION_INFORMATION;

  _SYSTEM_CRASH_STATE_INFORMATION = record // Information Class 34
    ValidCrashDump: ULONG;
    Unknown: ULONG;  // Windows 2000 only
  end;
  SYSTEM_CRASH_STATE_INFORMATION = _SYSTEM_CRASH_STATE_INFORMATION;
  PSYSTEM_CRASH_STATE_INFORMATION = ^SYSTEM_CRASH_STATE_INFORMATION;

  _SYSTEM_KERNEL_DEBUGGER_INFORMATION = record // Information Class 35
    DebuggerEnabled: ByteBool;
    DebuggerNotPresent: ByteBool;
  end;
  SYSTEM_KERNEL_DEBUGGER_INFORMATION = _SYSTEM_KERNEL_DEBUGGER_INFORMATION;
  PSYSTEM_KERNEL_DEBUGGER_INFORMATION = ^SYSTEM_KERNEL_DEBUGGER_INFORMATION;

  _SYSTEM_CONTEXT_SWITCH_INFORMATION = record // Information Class 36
    ContextSwitches: ULONG;
    ContextSwitchCounters: array [0..10] of ULONG;
  end;
  SYSTEM_CONTEXT_SWITCH_INFORMATION = _SYSTEM_CONTEXT_SWITCH_INFORMATION;
  PSYSTEM_CONTEXT_SWITCH_INFORMATION = ^SYSTEM_CONTEXT_SWITCH_INFORMATION;

  _SYSTEM_REGISTRY_QUOTA_INFORMATION = record // Information Class 37
    RegistryQuota: ULONG;
    RegistryQuotaInUse: ULONG;
    PagedPoolSize: ULONG;
  end;
  SYSTEM_REGISTRY_QUOTA_INFORMATION = _SYSTEM_REGISTRY_QUOTA_INFORMATION;
  PSYSTEM_REGISTRY_QUOTA_INFORMATION = ^SYSTEM_REGISTRY_QUOTA_INFORMATION;

  _SYSTEM_LOAD_AND_CALL_IMAGE = record // Information Class 38
    ModuleName: UNICODE_STRING;
  end;
  SYSTEM_LOAD_AND_CALL_IMAGE = _SYSTEM_LOAD_AND_CALL_IMAGE;
  PSYSTEM_LOAD_AND_CALL_IMAGE = ^SYSTEM_LOAD_AND_CALL_IMAGE;

  _SYSTEM_PRIORITY_SEPARATION = record // Information Class 39
    PrioritySeparation: ULONG;
  end;
  SYSTEM_PRIORITY_SEPARATION = _SYSTEM_PRIORITY_SEPARATION;
  PSYSTEM_PRIORITY_SEPARATION = ^SYSTEM_PRIORITY_SEPARATION;

  _SYSTEM_TIME_ZONE_INFORMATION = record // Information Class 44
    Bias: LONG;
    StandardName: array [0..31] of WCHAR;
    StandardDate: SYSTEMTIME;
    StandardBias: LONG;
    DaylightName: array [0..31] of WCHAR;
    DaylightDate: SYSTEMTIME;
    DaylightBias: LONG;
  end;
  SYSTEM_TIME_ZONE_INFORMATION = _SYSTEM_TIME_ZONE_INFORMATION;
  PSYSTEM_TIME_ZONE_INFORMATION = ^SYSTEM_TIME_ZONE_INFORMATION;

  _SYSTEM_LOOKASIDE_INFORMATION = record // Information Class 45
    Depth: USHORT;
    MaximumDepth: USHORT;
    TotalAllocates: ULONG;
    AllocateMisses: ULONG;
    TotalFrees: ULONG;
    FreeMisses: ULONG;
    Type_: POOL_TYPE;
    Tag: ULONG;
    Size: ULONG;
  end;
  SYSTEM_LOOKASIDE_INFORMATION = _SYSTEM_LOOKASIDE_INFORMATION;
  PSYSTEM_LOOKASIDE_INFORMATION = ^SYSTEM_LOOKASIDE_INFORMATION;

  _SYSTEM_SET_TIME_SLIP_EVENT = record // Information Class 46
    TimeSlipEvent: HANDLE;
  end;
  SYSTEM_SET_TIME_SLIP_EVENT = _SYSTEM_SET_TIME_SLIP_EVENT;
  PSYSTEM_SET_TIME_SLIP_EVENT = ^SYSTEM_SET_TIME_SLIP_EVENT;

  _SYSTEM_CREATE_SESSION = record // Information Class 47
    Session: ULONG;
  end;
  SYSTEM_CREATE_SESSION = _SYSTEM_CREATE_SESSION;
  PSYSTEM_CREATE_SESSION = ^SYSTEM_CREATE_SESSION;

  _SYSTEM_DELETE_SESSION = record // Information Class 48
    Session: ULONG;
  end;
  SYSTEM_DELETE_SESSION = _SYSTEM_DELETE_SESSION;
  PSYSTEM_DELETE_SESSION = ^SYSTEM_DELETE_SESSION;

  _SYSTEM_RANGE_START_INFORMATION = record // Information Class 50
    SystemRangeStart: PVOID;
  end;
  SYSTEM_RANGE_START_INFORMATION = _SYSTEM_RANGE_START_INFORMATION;
  PSYSTEM_RANGE_START_INFORMATION = ^SYSTEM_RANGE_START_INFORMATION;

  _SYSTEM_POOL_BLOCK = record
    Allocated: ByteBool;
    Unknown: USHORT;
    Size: ULONG;
    Tag: array [0..3] of CHAR;
  end;
  SYSTEM_POOL_BLOCK = _SYSTEM_POOL_BLOCK;
  PSYSTEM_POOL_BLOCK = ^SYSTEM_POOL_BLOCK;

  _SYSTEM_POOL_BLOCKS_INFORMATION = record // Info Classes 14 and 15
    PoolSize: ULONG;
    PoolBase: PVOID;
    Unknown: USHORT;
    NumberOfBlocks: ULONG;
    PoolBlocks: array [0..0] of SYSTEM_POOL_BLOCK;
  end;
  SYSTEM_POOL_BLOCKS_INFORMATION = _SYSTEM_POOL_BLOCKS_INFORMATION;
  PSYSTEM_POOL_BLOCKS_INFORMATION = ^SYSTEM_POOL_BLOCKS_INFORMATION;

  _SYSTEM_MEMORY_USAGE = record
    Name: PVOID;
    Valid: USHORT;
    Standby: USHORT;
    Modified: USHORT;
    PageTables: USHORT;
  end;
  SYSTEM_MEMORY_USAGE = _SYSTEM_MEMORY_USAGE;
  PSYSTEM_MEMORY_USAGE = ^SYSTEM_MEMORY_USAGE;

  _SYSTEM_MEMORY_USAGE_INFORMATION = record // Info Classes 25 and 29
    Reserved: ULONG;
    EndOfData: PVOID;
    MemoryUsage: array [0..0] of SYSTEM_MEMORY_USAGE;
  end;
  SYSTEM_MEMORY_USAGE_INFORMATION = _SYSTEM_MEMORY_USAGE_INFORMATION;
  PSYSTEM_MEMORY_USAGE_INFORMATION = ^SYSTEM_MEMORY_USAGE_INFORMATION;

function NtQuerySystemEnvironmentValue(Name: PUNICODE_STRING; Value: PVOID; ValueLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtSetSystemEnvironmentValue(Name: PUNICODE_STRING; Value: PUNICODE_STRING): NTSTATUS; stdcall;

type
  _SHUTDOWN_ACTION = (
    ShutdownNoReboot,
    ShutdownReboot,
    ShutdownPowerOff);
  SHUTDOWN_ACTION = _SHUTDOWN_ACTION;

function NtShutdownSystem(Action: SHUTDOWN_ACTION): NTSTATUS; stdcall;

type
  _DEBUG_CONTROL_CODE = (
    DebugFiller0,
    DebugGetTraceInformation,
    DebugSetInternalBreakpoint,
    DebugSetSpecialCall,
    DebugClearSpecialCalls,
    DebugQuerySpecialCalls,
    DebugDbgBreakPoint);
  DEBUG_CONTROL_CODE = _DEBUG_CONTROL_CODE;

function NtSystemDebugControl(ControlCode: DEBUG_CONTROL_CODE; InputBuffer: PVOID; InputBufferLength: ULONG; OutputBuffer: PVOID; OutputBufferLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;

type
  _OBJECT_INFORMATION_CLASS = (
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllTypesInformation,
    ObjectHandleInformation);
  OBJECT_INFORMATION_CLASS = _OBJECT_INFORMATION_CLASS;

function NtQueryObject(ObjectHandle: HANDLE; ObjectInformationClass: OBJECT_INFORMATION_CLASS; ObjectInformation: PVOID; ObjectInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtSetInformationObject(ObjectHandle: HANDLE; ObjectInformationClass: OBJECT_INFORMATION_CLASS; ObjectInformation: PVOID; ObjectInformationLength: ULONG): NTSTATUS; stdcall;

type
  _OBJECT_BASIC_INFORMATION = record // Information Class 0
    Attributes: ULONG;
    GrantedAccess: ACCESS_MASK;
    HandleCount: ULONG;
    PointerCount: ULONG;
    PagedPoolUsage: ULONG;
    NonPagedPoolUsage: ULONG;
    Reserved: array [0..2] of ULONG;
    NameInformationLength: ULONG;
    TypeInformationLength: ULONG;
    SecurityDescriptorLength: ULONG;
    CreateTime: LARGE_INTEGER;
  end;

  _OBJECT_NAME_INFORMATION = record // Information Class 1
    Name: UNICODE_STRING;
  end;
  OBJECT_NAME_INFORMATION = _OBJECT_NAME_INFORMATION;
  POBJECT_NAME_INFORMATION = ^OBJECT_NAME_INFORMATION;

  _OBJECT_TYPE_INFORMATION = record // Information Class 2
    Name: UNICODE_STRING;
    ObjectCount: ULONG;
    HandleCount: ULONG;
    Reserved1: array [0..3] of ULONG;
    PeakObjectCount: ULONG;
    PeakHandleCount: ULONG;
    Reserved2: array [0..3] of ULONG;
    InvalidAttributes: ULONG;
    GenericMapping: GENERIC_MAPPING;
    ValidAccess: ULONG;
    Unknown: UCHAR;
    MaintainHandleDatabase: ByteBool;
    Reserved3: array [0..1] of UCHAR;
    PoolType: POOL_TYPE;
    PagedPoolUsage: ULONG;
    NonPagedPoolUsage: ULONG;
  end;
  OBJECT_TYPE_INFORMATION = _OBJECT_TYPE_INFORMATION;
  POBJECT_TYPE_INFORMATION = ^OBJECT_TYPE_INFORMATION;

  _OBJECT_ALL_TYPES_INFORMATION = record // Information Class 3
    NumberOfTypes: ULONG;
    TypeInformation: OBJECT_TYPE_INFORMATION;
  end;
  OBJECT_ALL_TYPES_INFORMATION = _OBJECT_ALL_TYPES_INFORMATION;
  POBJECT_ALL_TYPES_INFORMATION = ^OBJECT_ALL_TYPES_INFORMATION;

  _OBJECT_HANDLE_ATTRIBUTE_INFORMATION = record // Information Class 4
    Inherit: ByteBool;
    ProtectFromClose: ByteBool;
  end;
  OBJECT_HANDLE_ATTRIBUTE_INFORMATION = _OBJECT_HANDLE_ATTRIBUTE_INFORMATION;
  POBJECT_HANDLE_ATTRIBUTE_INFORMATION = ^OBJECT_HANDLE_ATTRIBUTE_INFORMATION;

function NtDuplicateObject(SourceProcessHandle: HANDLE; SourceHandle: HANDLE; TargetProcessHandle: HANDLE; TargetHandle: PHANDLE; DesiredAccess: ACCESS_MASK; Attributes: ULONG; Options: ULONG): NTSTATUS; stdcall;
function NtMakeTemporaryObject(Handle: HANDLE): NTSTATUS; stdcall;
function NtClose(Handle: HANDLE): NTSTATUS; stdcall;
function NtQuerySecurityObject(Handle: HANDLE; RequestedInformation: SECURITY_INFORMATION; SecurityDescriptor: PSECURITY_DESCRIPTOR; SecurityDescriptorLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtSetSecurityObject(Handle: HANDLE; SecurityInformation: SECURITY_INFORMATION; SecurityDescriptor: PSECURITY_DESCRIPTOR): NTSTATUS; stdcall;
function NtCreateDirectoryObject(DirectoryHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtOpenDirectoryObject(DirectoryHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtQueryDirectoryObject(DirectoryHandle: HANDLE; Buffer: PVOID; BufferLength: ULONG; ReturnSingleEntry: ByteBool; RestartScan: ByteBool; Context: PULONG; ReturnLength: PULONG): NTSTATUS; stdcall;

type
  _DIRECTORY_BASIC_INFORMATION = record
    ObjectName: UNICODE_STRING;
    ObjectTypeName: UNICODE_STRING;
  end;
  DIRECTORY_BASIC_INFORMATION = _DIRECTORY_BASIC_INFORMATION;
  PDIRECTORY_BASIC_INFORMATION = ^DIRECTORY_BASIC_INFORMATION;

function NtCreateSymbolicLinkObject(SymbolicLinkHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; TargetName: PUNICODE_STRING): NTSTATUS; stdcall;
function NtOpenSymbolicLinkObject(SymbolicLinkHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtQuerySymbolicLinkObject(SymbolicLinkHandle: HANDLE; TargetName: PUNICODE_STRING; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtAllocateVirtualMemory(ProcessHandle: HANDLE; BaseAddress: PPVOID; ZeroBits: ULONG; AllocationSize: PULONG; AllocationType: ULONG; Protect: ULONG): NTSTATUS; stdcall;
function NtFreeVirtualMemory(ProcessHandle: HANDLE; BaseAddress: PPVOID; FreeSize: PULONG; FreeType: ULONG): NTSTATUS; stdcall;

type
  _MEMORY_INFORMATION_CLASS = (
    MemoryBasicInformation,
    MemoryWorkingSetList,
    MemorySectionName,
    MemoryBasicVlmInformation);
  MEMORY_INFORMATION_CLASS = _MEMORY_INFORMATION_CLASS;

function NtQueryVirtualMemory(ProcessHandle: HANDLE; BaseAddress: PVOID; MemoryInformationClass: MEMORY_INFORMATION_CLASS; MemoryInformation: PVOID; MemoryInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;

type
  _MEMORY_BASIC_INFORMATION = record // Information Class 0
    BaseAddress: PVOID;
    AllocationBase: PVOID;
    AllocationProtect: ULONG;
    RegionSize: ULONG;
    State: ULONG;
    Protect: ULONG;
    Type_: ULONG;
  end;
  MEMORY_BASIC_INFORMATION = _MEMORY_BASIC_INFORMATION;
  PMEMORY_BASIC_INFORMATION = ^MEMORY_BASIC_INFORMATION;

  _MEMORY_WORKING_SET_LIST = record // Information Class 1
    NumberOfPages: ULONG;
    WorkingSetList: array [0..0] of ULONG;
  end;
  MEMORY_WORKING_SET_LIST = _MEMORY_WORKING_SET_LIST;
  PMEMORY_WORKING_SET_LIST = ^MEMORY_WORKING_SET_LIST;

  _MEMORY_SECTION_NAME = record // Information Class 2
    SectionFileName: UNICODE_STRING;
  end;
  MEMORY_SECTION_NAME = _MEMORY_SECTION_NAME;
  PMEMORY_SECTION_NAME = ^MEMORY_SECTION_NAME;

function NtLockVirtualMemory(ProcessHandle: HANDLE; BaseAddress: PPVOID; LockSize: PULONG; LockType: ULONG): NTSTATUS; stdcall;
function NtUnlockVirtualMemory(ProcessHandle: HANDLE; BaseAddress: PPVOID; LockSize: PULONG; LockType: ULONG): NTSTATUS; stdcall;
function NtReadVirtualMemory(ProcessHandle: HANDLE; BaseAddress: PVOID; Buffer: PVOID; BufferLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtWriteVirtualMemory(ProcessHandle: HANDLE; BaseAddress: PVOID; Buffer: PVOID; BufferLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtProtectVirtualMemory(ProcessHandle: HANDLE; BaseAddress: PPVOID; ProtectSize: PULONG; NewProtect: ULONG; OldProtect: PULONG): NTSTATUS; stdcall;
function NtFlushVirtualMemory(ProcessHandle: HANDLE; BaseAddress: PPVOID; FlushSize: PULONG; IoStatusBlock: PIO_STATUS_BLOCK): NTSTATUS; stdcall;
function NtAllocateUserPhysicalPages(ProcessHandle: HANDLE; NumberOfPages: PULONG; PageFrameNumbers: PULONG): NTSTATUS; stdcall;
function NtFreeUserPhysicalPages(ProcessHandle: HANDLE; NumberOfPages: PULONG; PageFrameNumbers: PULONG): NTSTATUS; stdcall;
function NtMapUserPhysicalPages(BaseAddress: PVOID; NumberOfPages: PULONG; PageFrameNumbers: PULONG): NTSTATUS; stdcall;
function NtMapUserPhysicalPagesScatter(BaseAddresses: PPVOID; NumberOfPages: PULONG; PageFrameNumbers: PULONG): NTSTATUS; stdcall;
function NtGetWriteWatch(ProcessHandle: HANDLE; Flags: ULONG; BaseAddress: PVOID; RegionSize: ULONG; Buffer: PULONG; BufferEntries: PULONG; Granularity: PULONG): NTSTATUS; stdcall;
function NtResetWriteWatch(ProcessHandle: HANDLE; BaseAddress: PVOID; RegionSize: ULONG): NTSTATUS; stdcall;
function NtCreateSection(SectionHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; SectionSize: PLARGE_INTEGER; Protect: ULONG; Attributes: ULONG; FileHandle: HANDLE): NTSTATUS; stdcall;
function NtOpenSection(SectionHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;

type
  _SECTION_INFORMATION_CLASS = (
    SectionBasicInformation,
    SectionImageInformation);
  SECTION_INFORMATION_CLASS = _SECTION_INFORMATION_CLASS;

function NtQuerySection(SectionHandle: HANDLE; SectionInformationClass: SECTION_INFORMATION_CLASS; SectionInformation: PVOID; SectionInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;

type
  _SECTION_BASIC_INFORMATION = record // Information Class 0
    BaseAddress: PVOID;
    Attributes: ULONG;
    Size: LARGE_INTEGER;
  end;
  SECTION_BASIC_INFORMATION = _SECTION_BASIC_INFORMATION;
  PSECTION_BASIC_INFORMATION = ^SECTION_BASIC_INFORMATION;

  _SECTION_IMAGE_INFORMATION = record // Information Class 1
    EntryPoint: PVOID;
    Unknown1: ULONG;
    StackReserve: ULONG;
    StackCommit: ULONG;
    Subsystem: ULONG;
    MinorSubsystemVersion: USHORT;
    MajorSubsystemVersion: USHORT;
    Unknown2: ULONG;
    Characteristics: ULONG;
    ImageNumber: USHORT;
    Executable: ByteBool;
    Unknown3: UCHAR;
    Unknown4: array [0..2] of ULONG;
  end;
  SECTION_IMAGE_INFORMATION = _SECTION_IMAGE_INFORMATION;
  PSECTION_IMAGE_INFORMATION = ^SECTION_IMAGE_INFORMATION;

function NtExtendSection(SectionHandle: HANDLE; SectionSize: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtMapViewOfSection(SectionHandle: HANDLE; ProcessHandle: HANDLE; BaseAddress: PPVOID; ZeroBits: ULONG; CommitSize: ULONG; SectionOffset: PLARGE_INTEGER; ViewSize: PULONG; InheritDisposition: SECTION_INHERIT; AllocationType: ULONG; Protect: ULONG): NTSTATUS; stdcall;
function NtUnmapViewOfSection(ProcessHandle: HANDLE; BaseAddress: PVOID): NTSTATUS; stdcall;
function NtAreMappedFilesTheSame(Address1: PVOID; Address2: PVOID): NTSTATUS; stdcall;

type
  _USER_STACK = record
    FixedStackBase: PVOID;
    FixedStackLimit: PVOID;
    ExpandableStackBase: PVOID;
    ExpandableStackLimit: PVOID;
    ExpandableStackBottom: PVOID;
  end;
  USER_STACK = _USER_STACK;
  PUSER_STACK = ^USER_STACK;

function NtCreateThread(ThreadHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; ProcessHandle: HANDLE; ClientId: PCLIENT_ID; ThreadContext: PCONTEXT; UserStack: PUSER_STACK; CreateSuspended: ByteBool): NTSTATUS; stdcall;
function NtOpenThread(ThreadHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; ClientId: PCLIENT_ID): NTSTATUS; stdcall;
function NtTerminateThread(ThreadHandle: HANDLE; ExitStatus: NTSTATUS): NTSTATUS; stdcall;
function NtQueryInformationThread(ThreadHandle: HANDLE; ThreadInformationClass: THREADINFOCLASS; ThreadInformation: PVOID; ThreadInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtSetInformationThread(ThreadHandle: HANDLE; ThreadInformationClass: THREADINFOCLASS; ThreadInformation: PVOID; ThreadInformationLength: ULONG): NTSTATUS; stdcall;

type
  _THREAD_BASIC_INFORMATION = record // Information Class 0
    ExitStatus: NTSTATUS;
    TebBaseAddress: PNT_TIB;
    ClientId: CLIENT_ID;
    AffinityMask: KAFFINITY;
    Priority: KPRIORITY;
    BasePriority: KPRIORITY;
  end;
  THREAD_BASIC_INFORMATION = _THREAD_BASIC_INFORMATION;
  PTHREAD_BASIC_INFORMATION = ^THREAD_BASIC_INFORMATION;

function NtSuspendThread(ThreadHandle: HANDLE; PreviousSuspendCount: PULONG): NTSTATUS; stdcall;
function NtResumeThread(ThreadHandle: HANDLE; PreviousSuspendCount: PULONG): NTSTATUS; stdcall;
function NtGetContextThread(ThreadHandle: HANDLE; Context: PCONTEXT): NTSTATUS; stdcall;
function NtSetContextThread(ThreadHandle: HANDLE; Context: PCONTEXT): NTSTATUS; stdcall;
function NtQueueApcThread(ThreadHandle: HANDLE; ApcRoutine: PKNORMAL_ROUTINE; ApcContext: PVOID; Argument1: PVOID; Argument2: PVOID): NTSTATUS; stdcall;
function NtTestAlert: NTSTATUS; stdcall;
function NtAlertThread(ThreadHandle: HANDLE): NTSTATUS; stdcall;
function NtAlertResumeThread(ThreadHandle: HANDLE; PreviousSuspendCount: PULONG): NTSTATUS; stdcall;
function NtRegisterThreadTerminatePort(PortHandle: HANDLE): NTSTATUS; stdcall;
function NtImpersonateThread(ThreadHandle: HANDLE; TargetThreadHandle: HANDLE; SecurityQos: PSECURITY_QUALITY_OF_SERVICE): NTSTATUS; stdcall;
function NtImpersonateAnonymousToken(ThreadHandle: HANDLE): NTSTATUS; stdcall;
function NtCreateProcess(ProcessHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; InheritFromProcessHandle: HANDLE; InheritHandles: ByteBool; SectionHandle: HANDLE; DebugPort: HANDLE; ExceptionPort: HANDLE): NTSTATUS; stdcall;
function NtOpenProcess(ProcessHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; ClientId: PCLIENT_ID): NTSTATUS; stdcall;
function NtTerminateProcess(ProcessHandle: HANDLE; ExitStatus: NTSTATUS): NTSTATUS; stdcall;
function NtQueryInformationProcess(ProcessHandle: HANDLE; ProcessInformationClass: PROCESSINFOCLASS; ProcessInformation: PVOID; ProcessInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtSetInformationProcess(ProcessHandle: HANDLE; ProcessInformationClass: PROCESSINFOCLASS; ProcessInformation: PVOID; ProcessInformationLength: ULONG): NTSTATUS; stdcall;

type
  _PROCESS_PRIORITY_CLASS = record // Information Class 18
    Foreground: ByteBool;
    PriorityClass: UCHAR;
  end;
  PROCESS_PRIORITY_CLASS = _PROCESS_PRIORITY_CLASS;
  PPROCESS_PRIORITY_CLASS = ^PROCESS_PRIORITY_CLASS;

  _PROCESS_PARAMETERS = record
    AllocationSize: ULONG;
    Size: ULONG;
    Flags: ULONG;
    Zero: ULONG;
    Console: LONG;
    ProcessGroup: ULONG;
    hStdInput: HANDLE;
    hStdOutput: HANDLE;
    hStdError: HANDLE;
    CurrentDirectoryName: UNICODE_STRING;
    CurrentDirectoryHandle: HANDLE;
    DllPath: UNICODE_STRING;
    ImageFile: UNICODE_STRING;
    CommandLine: UNICODE_STRING;
    Environment: PWSTR;
    dwX: ULONG;
    dwY: ULONG;
    dwXSize: ULONG;
    dwYSize: ULONG;
    dwXCountChars: ULONG;
    dwYCountChars: ULONG;
    dwFillAttribute: ULONG;
    dwFlags: ULONG;
    wShowWindow: ULONG;
    WindowTitle: UNICODE_STRING;
    Desktop: UNICODE_STRING;
    Reserved: UNICODE_STRING;
    Reserved2: UNICODE_STRING;
  end;
  PROCESS_PARAMETERS = _PROCESS_PARAMETERS;
  PPROCESS_PARAMETERS = ^PROCESS_PARAMETERS;
  PPPROCESS_PARAMETERS = ^PPROCESS_PARAMETERS;

function RtlCreateProcessParameters(ProcessParameters: PPPROCESS_PARAMETERS; ImageFile: PUNICODE_STRING; DllPath: PUNICODE_STRING; CurrentDirectory: PUNICODE_STRING; CommandLine: PUNICODE_STRING; CreationFlags: ULONG; WindowTitle: PUNICODE_STRING; Desktop: PUNICODE_STRING; Reserved: PUNICODE_STRING; Reserved2: PUNICODE_STRING): NTSTATUS; stdcall;
function RtlDestroyProcessParameters(ProcessParameters: PPROCESS_PARAMETERS): NTSTATUS; stdcall;

type
  _DEBUG_BUFFER = record
    SectionHandle: HANDLE;
    SectionBase: PVOID;
    RemoteSectionBase: PVOID;
    SectionBaseDelta: ULONG;
    EventPairHandle: HANDLE;
    Unknown: array [0..1] of ULONG;
    RemoteThreadHandle: HANDLE;
    InfoClassMask: ULONG;
    SizeOfInfo: ULONG;
    AllocatedSize: ULONG;
    SectionSize: ULONG;
    ModuleInformation: PVOID;
    BackTraceInformation: PVOID;
    HeapInformation: PVOID;
    LockInformation: PVOID;
    Reserved: array [0..7] of PVOID;
  end;
  DEBUG_BUFFER = _DEBUG_BUFFER;
  PDEBUG_BUFFER = ^DEBUG_BUFFER;

const
  PDI_MODULES     = $01;
  PDI_BACKTRACE   = $02;
  PDI_HEAPS       = $04;
  PDI_HEAP_TAGS	  = $08;
  PDI_HEAP_BLOCKS = $10;
  PDI_LOCKS       = $20;

type
  _DEBUG_MODULE_INFORMATION = record // c.f. SYSTEM_MODULE_INFORMATION
    Reserved: array [0..1] of ULONG;
    Base: ULONG;
    Size: ULONG;
    Flags: ULONG;
    Index: USHORT;
    Unknown: USHORT;
    LoadCount: USHORT;
    ModuleNameOffset: USHORT;
    ImageName: array [0..255] of CHAR;
  end;
  DEBUG_MODULE_INFORMATION = _DEBUG_MODULE_INFORMATION;
  PDEBUG_MODULE_INFORMATION = ^DEBUG_MODULE_INFORMATION;

  _DEBUG_HEAP_INFORMATION = record
    Base: ULONG;
    Flags: ULONG;
    Granularity: USHORT;
    Unknown: USHORT;
    Allocated: ULONG;
    Committed: ULONG;
    TagCount: ULONG;
    BlockCount: ULONG;
    Reserved: array [0..6] of ULONG;
    Tags: PVOID;
    Blocks: PVOID;
  end;
  DEBUG_HEAP_INFORMATION = _DEBUG_HEAP_INFORMATION;
  PDEBUG_HEAP_INFORMATION = ^DEBUG_HEAP_INFORMATION;

  _DEBUG_LOCK_INFORMATION = record // c.f. SYSTEM_LOCK_INFORMATION
    Address: PVOID;
    Type_: USHORT;
    CreatorBackTraceIndex: USHORT;
    OwnerThreadId: ULONG;
    ActiveCount: ULONG;
    ContentionCount: ULONG;
    EntryCount: ULONG;
    RecursionCount: ULONG;
    NumberOfSharedWaiters: ULONG;
    NumberOfExclusiveWaiters: ULONG;
  end;
  DEBUG_LOCK_INFORMATION = _DEBUG_LOCK_INFORMATION;
  PDEBUG_LOCK_INFORMATION = ^DEBUG_LOCK_INFORMATION;

function RtlCreateQueryDebugBuffer(Size: ULONG; EventPair: ByteBool): PDEBUG_BUFFER; stdcall;
function RtlQueryProcessDebugInformation(ProcessId: ULONG; DebugInfoClassMask: ULONG; DebugBuffer: PDEBUG_BUFFER): NTSTATUS; stdcall;
function RtlDestroyQueryDebugBuffer(DebugBuffer: PDEBUG_BUFFER): NTSTATUS; stdcall;
function NtCreateJobObject(JobHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtOpenJobObject(JobHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtTerminateJobObject(JobHandle: HANDLE; ExitStatus: NTSTATUS): NTSTATUS; stdcall;
function NtAssignProcessToJobObject(JobHandle: HANDLE; ProcessHandle: HANDLE): NTSTATUS; stdcall;
function NtQueryInformationJobObject(JobHandle: HANDLE; JobInformationClass: JOBOBJECTINFOCLASS; JobInformation: PVOID; JobInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtSetInformationJobObject(JobHandle: HANDLE; JobInformationClass: JOBOBJECTINFOCLASS; JobInformation: PVOID; JobInformationLength: ULONG): NTSTATUS; stdcall;
function NtCreateToken(TokenHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; Type_: TOKEN_TYPE; AuthenticationId: PLUID; ExpirationTime: PLARGE_INTEGER; User: PTOKEN_USER; Groups: PTOKEN_GROUPS; Privileges: PTOKEN_PRIVILEGES; Owner: PTOKEN_OWNER; PrimaryGroup: PTOKEN_PRIMARY_GROUP; DefaultDacl: PTOKEN_DEFAULT_DACL; Source: PTOKEN_SOURCE): NTSTATUS; stdcall;
function NtOpenProcessToken(ProcessHandle: HANDLE; DesiredAccess: ACCESS_MASK; TokenHandle: PHANDLE): NTSTATUS; stdcall;
function NtOpenThreadToken(ThreadHandle: HANDLE; DesiredAccess: ACCESS_MASK; OpenAsSelf: ByteBool; TokenHandle: PHANDLE): NTSTATUS; stdcall;
function NtDuplicateToken(ExistingTokenHandle: HANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; EffectiveOnly: ByteBool; TokenType: TOKEN_TYPE; NewTokenHandle: PHANDLE): NTSTATUS; stdcall;
function NtFilterToken(ExistingTokenHandle: HANDLE; Flags: ULONG; SidsToDisable: PTOKEN_GROUPS; PrivilegesToDelete: PTOKEN_PRIVILEGES; SidsToRestricted: PTOKEN_GROUPS; NewTokenHandle: PHANDLE): NTSTATUS; stdcall;
function NtAdjustPrivilegesToken(TokenHandle: HANDLE; DisableAllPrivileges: ByteBool; NewState: PTOKEN_PRIVILEGES; BufferLength: ULONG; PreviousState: PTOKEN_PRIVILEGES; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtAdjustGroupsToken(TokenHandle: HANDLE; ResetToDefault: ByteBool; NewState: PTOKEN_GROUPS; BufferLength: ULONG; PreviousState: PTOKEN_GROUPS; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtQueryInformationToken(TokenHandle: HANDLE; TokenInformationClass: TOKEN_INFORMATION_CLASS; TokenInformation: PVOID; TokenInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtSetInformationToken(TokenHandle: HANDLE; TokenInformationClass: TOKEN_INFORMATION_CLASS; TokenInformation: PVOID; TokenInformationLength: ULONG): NTSTATUS; stdcall;
function NtWaitForSingleObject(Handle: HANDLE; Alertable: ByteBool; Timeout: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtSignalAndWaitForSingleObject(HandleToSignal: HANDLE; HandleToWait: HANDLE; Alertable: ByteBool; Timeout: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtWaitForMultipleObjects(HandleCount: ULONG; Handles: PHANDLE; WaitType: WAIT_TYPE; Alertable: ByteBool; Timeout: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtCreateTimer(TimerHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; TimerType: TIMER_TYPE): NTSTATUS; stdcall;
function NtOpenTimer(TimerHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtCancelTimer(TimerHandle: HANDLE; PreviousState: PBOOLEAN): NTSTATUS; stdcall;

type
  PTIMER_APC_ROUTINE = procedure (TimerContext: PVOID; TimerLowValue: ULONG; TimerHighValue: LONG); stdcall;

function NtSetTimer(TimerHandle: HANDLE; DueTime: PLARGE_INTEGER; TimerApcRoutine: PTIMER_APC_ROUTINE; TimerContext: PVOID; Resume: ByteBool; Period: LONG; PreviousState: PBOOLEAN): NTSTATUS; stdcall;

type
  _TIMER_INFORMATION_CLASS = (TimerBasicInformation);
  TIMER_INFORMATION_CLASS = _TIMER_INFORMATION_CLASS;

function NtQueryTimer(TimerHandle: HANDLE; TimerInformationClass: TIMER_INFORMATION_CLASS; TimerInformation: PVOID; TimerInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;

type
  _TIMER_BASIC_INFORMATION = record
    TimeRemaining: LARGE_INTEGER;
    SignalState: ByteBool;
  end;
  TIMER_BASIC_INFORMATION = _TIMER_BASIC_INFORMATION;
  PTIMER_BASIC_INFORMATION = ^TIMER_BASIC_INFORMATION;

function NtCreateEvent(EventHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; EventType: EVENT_TYPE; InitialState: ByteBool): NTSTATUS; stdcall;
function NtOpenEvent(EventHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtSetEvent(EventHandle: HANDLE; PreviousState: PULONG): NTSTATUS; stdcall;
function NtPulseEvent(EventHandle: HANDLE; PreviousState: PULONG): NTSTATUS; stdcall;
function NtResetEvent(EventHandle: HANDLE; PreviousState: PULONG): NTSTATUS; stdcall;
function NtClearEvent(EventHandle: HANDLE): NTSTATUS; stdcall;

type
  _EVENT_INFORMATION_CLASS = (EventBasicInformation);
  EVENT_INFORMATION_CLASS = _EVENT_INFORMATION_CLASS;

function NtQueryEvent(EventHandle: HANDLE; EventInformationClass: EVENT_INFORMATION_CLASS; EventInformation: PVOID; EventInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;

type
  _EVENT_BASIC_INFORMATION = record
    EventType: EVENT_TYPE;
    SignalState: LONG;
  end;
  EVENT_BASIC_INFORMATION = _EVENT_BASIC_INFORMATION;
  PEVENT_BASIC_INFORMATION = ^EVENT_BASIC_INFORMATION;

function NtCreateSemaphore(SemaphoreHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; InitialCount: LONG; MaximumCount: LONG): NTSTATUS; stdcall;
function NtOpenSemaphore(SemaphoreHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtReleaseSemaphore(SemaphoreHandle: HANDLE; ReleaseCount: LONG; PreviousCount: PLONG): NTSTATUS; stdcall;

type
  _SEMAPHORE_INFORMATION_CLASS = (SemaphoreBasicInformation);
  SEMAPHORE_INFORMATION_CLASS = _SEMAPHORE_INFORMATION_CLASS;

function NtQuerySemaphore(SemaphoreHandle: HANDLE; SemaphoreInformationClass: SEMAPHORE_INFORMATION_CLASS; SemaphoreInformation: PVOID; SemaphoreInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;

type
  _SEMAPHORE_BASIC_INFORMATION = record
    CurrentCount: LONG;
    MaximumCount: LONG;
  end;
  SEMAPHORE_BASIC_INFORMATION = _SEMAPHORE_BASIC_INFORMATION;
  PSEMAPHORE_BASIC_INFORMATION = ^SEMAPHORE_BASIC_INFORMATION;

function NtCreateMutant(MutantHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; InitialOwner: ByteBool): NTSTATUS; stdcall;
function NtOpenMutant(MutantHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtReleaseMutant(MutantHandle: HANDLE; PreviousState: PULONG): NTSTATUS; stdcall;

type
  _MUTANT_INFORMATION_CLASS = (MutantBasicInformation);
  MUTANT_INFORMATION_CLASS = _MUTANT_INFORMATION_CLASS;

function NtQueryMutant(MutantHandle: HANDLE; MutantInformationClass: MUTANT_INFORMATION_CLASS; MutantInformation: PVOID; MutantInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;

type
  _MUTANT_BASIC_INFORMATION = record
    SignalState: LONG;
    Owned: ByteBool;
    Abandoned: ByteBool;
  end;
  MUTANT_BASIC_INFORMATION = _MUTANT_BASIC_INFORMATION;
  PMUTANT_BASIC_INFORMATION = ^MUTANT_BASIC_INFORMATION;

function NtCreateIoCompletion(IoCompletionHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; NumberOfConcurrentThreads: ULONG): NTSTATUS; stdcall;
function NtOpenIoCompletion(IoCompletionHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtSetIoCompletion(IoCompletionHandle: HANDLE; CompletionKey: ULONG; CompletionValue: ULONG; Status: NTSTATUS; Information: ULONG): NTSTATUS; stdcall;
function NtRemoveIoCompletion(IoCompletionHandle: HANDLE; CompletionKey: PULONG; CompletionValue: PULONG; IoStatusBlock: PIO_STATUS_BLOCK; Timeout: PLARGE_INTEGER): NTSTATUS; stdcall;

type
  _IO_COMPLETION_INFORMATION_CLASS = (IoCompletionBasicInformation);
  IO_COMPLETION_INFORMATION_CLASS = _IO_COMPLETION_INFORMATION_CLASS;

function NtQueryIoCompletion(IoCompletionHandle: HANDLE; IoCompletionInformationClass: IO_COMPLETION_INFORMATION_CLASS; IoCompletionInformation: PVOID; IoCompletionInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;

type
  _IO_COMPLETION_BASIC_INFORMATION = record
    SignalState: LONG;
  end;
  IO_COMPLETION_BASIC_INFORMATION = _IO_COMPLETION_BASIC_INFORMATION;
  PIO_COMPLETION_BASIC_INFORMATION = ^IO_COMPLETION_BASIC_INFORMATION;

function NtCreateEventPair(EventPairHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtOpenEventPair(EventPairHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtWaitLowEventPair(EventPairHandle: HANDLE): NTSTATUS; stdcall;
function NtWaitHighEventPair(EventPairHandle: HANDLE): NTSTATUS; stdcall;
function NtSetLowWaitHighEventPair(EventPairHandle: HANDLE): NTSTATUS; stdcall;
function NtSetHighWaitLowEventPair(EventPairHandle: HANDLE): NTSTATUS; stdcall;
function NtSetLowEventPair(EventPairHandle: HANDLE): NTSTATUS; stdcall;
function NtSetHighEventPair(EventPairHandle: HANDLE): NTSTATUS; stdcall;
function NtQuerySystemTime(CurrentTime: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtSetSystemTime(NewTime: PLARGE_INTEGER; OldTime: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtQueryPerformanceCounter(PerformanceCount: PLARGE_INTEGER; PerformanceFrequency: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtSetTimerResolution(RequestedResolution: ULONG; Set_: ByteBool; ActualResolution: PULONG): NTSTATUS; stdcall;
function NtQueryTimerResolution(CoarsestResolution: PULONG; FinestResolution: PULONG; ActualResolution: PULONG): NTSTATUS; stdcall;
function NtDelayExecution(Alertable: ByteBool; Interval: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtYieldExecution: NTSTATUS; stdcall;
function NtGetTickCount: ULONG; stdcall;
function NtCreateProfile(ProfileHandle: PHANDLE; ProcessHandle: HANDLE; Base: PVOID; Size: ULONG; BucketShift: ULONG; Buffer: PULONG; BufferLength: ULONG; Source: KPROFILE_SOURCE; ProcessorMask: ULONG): NTSTATUS; stdcall;
function NtSetIntervalProfile(Interval: ULONG; Source: KPROFILE_SOURCE): NTSTATUS; stdcall;
function NtQueryIntervalProfile(Source: KPROFILE_SOURCE; Interval: PULONG): NTSTATUS; stdcall;
function NtStartProfile(ProfileHandle: HANDLE): NTSTATUS; stdcall;
function NtStopProfile(ProfileHandle: HANDLE): NTSTATUS; stdcall;

type
  _PORT_MESSAGE = record
    DataSize: USHORT;
    MessageSize: USHORT;
    MessageType: USHORT;
    VirtualRangesOffset: USHORT;
    ClientId: CLIENT_ID;
    MessageId: ULONG;
    SectionSize: ULONG;
    // UCHAR Data[];
  end;
  PORT_MESSAGE = _PORT_MESSAGE;
  PPORT_MESSAGE = ^PORT_MESSAGE;

  _LPC_TYPE = (
    LPC_NEW_MESSAGE,           // A new message
    LPC_REQUEST,               // A request message
    LPC_REPLY,                 // A reply to a request message
    LPC_DATAGRAM,              //
    LPC_LOST_REPLY,            //
    LPC_PORT_CLOSED,           // Sent when port is deleted
    LPC_CLIENT_DIED,           // Messages to thread termination ports
    LPC_EXCEPTION,             // Messages to thread exception port
    LPC_DEBUG_EVENT,           // Messages to thread debug port
    LPC_ERROR_EVENT,           // Used by ZwRaiseHardError
    LPC_CONNECTION_REQUEST);   // Used by ZwConnectPort
  LPC_TYPE = _LPC_TYPE;

  _PORT_SECTION_WRITE = record
    Length: ULONG;
    SectionHandle: HANDLE;
    SectionOffset: ULONG;
    ViewSize: ULONG;
    ViewBase: PVOID;
    TargetViewBase: PVOID;
  end;
  PORT_SECTION_WRITE = _PORT_SECTION_WRITE;
  PPORT_SECTION_WRITE = ^PORT_SECTION_WRITE;

  _PORT_SECTION_READ = record
    Length: ULONG;
    ViewSize: ULONG;
    ViewBase: ULONG;
  end;
  PORT_SECTION_READ = _PORT_SECTION_READ;
  PPORT_SECTION_READ = ^PORT_SECTION_READ;

function NtCreatePort(PortHandle: PHANDLE; ObjectAttributes: POBJECT_ATTRIBUTES; MaxDataSize: ULONG; MaxMessageSize: ULONG; Reserved: ULONG): NTSTATUS; stdcall;
function NtCreateWaitablePort(PortHandle: PHANDLE; ObjectAttributes: POBJECT_ATTRIBUTES; MaxDataSize: ULONG; MaxMessageSize: ULONG; Reserved: ULONG): NTSTATUS; stdcall;
function NtConnectPort(PortHandle: PHANDLE; PortName: PUNICODE_STRING; SecurityQos: PSECURITY_QUALITY_OF_SERVICE; WriteSection: PPORT_SECTION_WRITE; ReadSection: PPORT_SECTION_READ; MaxMessageSize: PULONG; ConnectData: PVOID; ConnectDataLength: PULONG): NTSTATUS; stdcall;
function NtSecureConnectPort(PortHandle: PHANDLE; PortName: PUNICODE_STRING; SecurityQos: PSECURITY_QUALITY_OF_SERVICE; WriteSection: PPORT_SECTION_WRITE; ServerSid: PSID; ReadSection: PPORT_SECTION_READ; MaxMessageSize: PULONG; ConnectData: PVOID; ConnectDataLength: PULONG): NTSTATUS; stdcall;
function NtListenPort(PortHandle: HANDLE; Message: PPORT_MESSAGE): NTSTATUS; stdcall;
function NtAcceptConnectPort(PortHandle: PHANDLE; PortIdentifier: ULONG; Message: PPORT_MESSAGE; Accept: ByteBool; WriteSection: PPORT_SECTION_WRITE; ReadSection: PPORT_SECTION_READ): NTSTATUS; stdcall;
function NtCompleteConnectPort(PortHandle: HANDLE): NTSTATUS; stdcall;
function NtRequestPort(PortHandle: HANDLE; RequestMessage: PPORT_MESSAGE): NTSTATUS; stdcall;
function NtRequestWaitReplyPort(PortHandle: HANDLE; RequestMessage: PPORT_MESSAGE; ReplyMessage: PPORT_MESSAGE): NTSTATUS; stdcall;
function NtReplyPort(PortHandle: HANDLE; ReplyMessage: PPORT_MESSAGE): NTSTATUS; stdcall;
function NtReplyWaitReplyPort(PortHandle: HANDLE; ReplyMessage: PPORT_MESSAGE): NTSTATUS; stdcall;
function NtReplyWaitReceivePort(PortHandle: HANDLE; PortIdentifier: PULONG; ReplyMessage: PPORT_MESSAGE; Message: PPORT_MESSAGE): NTSTATUS; stdcall;
function NtReplyWaitReceivePortEx(PortHandle: HANDLE; PortIdentifier: PULONG; ReplyMessage: PPORT_MESSAGE; Message: PPORT_MESSAGE; Timeout: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtReadRequestData(PortHandle: HANDLE; Message: PPORT_MESSAGE; Index: ULONG; Buffer: PVOID; BufferLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtWriteRequestData(PortHandle: HANDLE; Message: PPORT_MESSAGE; Index: ULONG; Buffer: PVOID; BufferLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;

type
  _PORT_INFORMATION_CLASS = (PortBasicInformation);
  PORT_INFORMATION_CLASS = _PORT_INFORMATION_CLASS;

function NtQueryInformationPort(PortHandle: HANDLE; PortInformationClass: PORT_INFORMATION_CLASS; PortInformation: PVOID; PortInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;

type
  _PORT_BASIC_INFORMATION = record
  end;
  PORT_BASIC_INFORMATION = _PORT_BASIC_INFORMATION;
  PPORT_BASIC_INFORMATION = ^PORT_BASIC_INFORMATION;

function NtImpersonateClientOfPort(PortHandle: HANDLE; Message: PPORT_MESSAGE): NTSTATUS; stdcall;
function NtCreateFile(FileHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; IoStatusBlock: PIO_STATUS_BLOCK; AllocationSize: PLARGE_INTEGER; FileAttributes: ULONG; ShareAccess: ULONG; CreateDisposition: ULONG; CreateOptions: ULONG; EaBuffer: PVOID; EaLength: ULONG): NTSTATUS; stdcall;
function NtOpenFile(FileHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; IoStatusBlock: PIO_STATUS_BLOCK; ShareAccess: ULONG; OpenOptions: ULONG): NTSTATUS; stdcall;
function NtDeleteFile(ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtFlushBuffersFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK): NTSTATUS; stdcall;
function NtCancelIoFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK): NTSTATUS; stdcall;
function NtReadFile(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; Buffer: PVOID; Length: ULONG; ByteOffset: PLARGE_INTEGER; Key: PULONG): NTSTATUS; stdcall;
function NtWriteFile(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; Buffer: PVOID; Length: ULONG; ByteOffset: PLARGE_INTEGER; Key: PULONG): NTSTATUS; stdcall;
function NtReadFileScatter(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; Buffer: PFILE_SEGMENT_ELEMENT; Length: ULONG; ByteOffset: PLARGE_INTEGER; Key: PULONG): NTSTATUS; stdcall;
function NtWriteFileGather(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; Buffer: PFILE_SEGMENT_ELEMENT; Length: ULONG; ByteOffset: PLARGE_INTEGER; Key: PULONG): NTSTATUS; stdcall;
function NtLockFile(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; LockOffset: PULARGE_INTEGER; LockLength: PULARGE_INTEGER; Key: ULONG; FailImmediately: ByteBool; ExclusiveLock: ByteBool): NTSTATUS; stdcall;
function NtUnlockFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK; LockOffset: PULARGE_INTEGER; LockLength: PULARGE_INTEGER; Key: ULONG): NTSTATUS; stdcall;
function NtDeviceIoControlFile(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; IoControlCode: ULONG; InputBuffer: PVOID; InputBufferLength: ULONG; OutputBuffer: PVOID; OutputBufferLength: ULONG): NTSTATUS; stdcall;
function NtFsControlFile(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; FsControlCode: ULONG; InputBuffer: PVOID; InputBufferLength: ULONG; OutputBuffer: PVOID; OutputBufferLength: ULONG): NTSTATUS; stdcall;
function NtNotifyChangeDirectoryFile(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; Buffer: PFILE_NOTIFY_INFORMATION; BufferLength: ULONG; NotifyFilter: ULONG; WatchSubtree: ByteBool): NTSTATUS; stdcall;

type
  _FILE_GET_EA_INFORMATION = record
    NextEntryOffset: ULONG;
    EaNameLength: UCHAR;
    EaName: array [0..0] of CHAR;
  end;
  FILE_GET_EA_INFORMATION = _FILE_GET_EA_INFORMATION;
  PFILE_GET_EA_INFORMATION = ^FILE_GET_EA_INFORMATION;

function NtQueryEaFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK; Buffer: PFILE_FULL_EA_INFORMATION; BufferLength: ULONG; ReturnSingleEntry: ByteBool; EaList: PFILE_GET_EA_INFORMATION; EaListLength: ULONG; EaIndex: PULONG; RestartScan: ByteBool): NTSTATUS; stdcall;
function NtSetEaFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK; Buffer: PFILE_FULL_EA_INFORMATION; BufferLength: ULONG): NTSTATUS; stdcall;
function NtCreateNamedPipeFile(FileHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; IoStatusBlock: PIO_STATUS_BLOCK; ShareAccess: ULONG; CreateDisposition: ULONG; CreateOptions: ULONG; TypeMessage: ByteBool; ReadmodeMessage: ByteBool; Nonblocking: ByteBool; MaxInstances: ULONG; InBufferSize: ULONG; OutBufferSize: ULONG; DefaultTimeout: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtCreateMailslotFile(FileHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; IoStatusBlock: PIO_STATUS_BLOCK; CreateOptions: ULONG; Unknown: ULONG; MaxMessageSize: ULONG; ReadTimeout: PLARGE_INTEGER): NTSTATUS; stdcall;
function NtQueryVolumeInformationFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK; VolumeInformation: PVOID; VolumeInformationLength: ULONG; VolumeInformationClass: FS_INFORMATION_CLASS): NTSTATUS; stdcall;
function NtSetVolumeInformationFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK; Buffer: PVOID; BufferLength: ULONG; VolumeInformationClass: FS_INFORMATION_CLASS): NTSTATUS; stdcall;

type
  _FILE_FS_VOLUME_INFORMATION = record
    VolumeCreationTime: LARGE_INTEGER;
    VolumeSerialNumber: ULONG;
    VolumeLabelLength: ULONG;
    Unknown: UCHAR;
    VolumeLabel: array [0..0] of WCHAR;
  end;
  FILE_FS_VOLUME_INFORMATION = _FILE_FS_VOLUME_INFORMATION;
  PFILE_FS_VOLUME_INFORMATION = ^FILE_FS_VOLUME_INFORMATION;

  _FILE_FS_LABEL_INFORMATION = record
    VolumeLabelLength: ULONG;
    VolumeLabel: WCHAR;
  end;
  FILE_FS_LABEL_INFORMATION = _FILE_FS_LABEL_INFORMATION;
  PFILE_FS_LABEL_INFORMATION = ^FILE_FS_LABEL_INFORMATION;

  _FILE_FS_SIZE_INFORMATION = record
    TotalAllocationUnits: LARGE_INTEGER;
    AvailableAllocationUnits: LARGE_INTEGER;
    SectorsPerAllocationUnit: ULONG;
    BytesPerSector: ULONG;
  end;
  FILE_FS_SIZE_INFORMATION = _FILE_FS_SIZE_INFORMATION;
  PFILE_FS_SIZE_INFORMATION = ^FILE_FS_SIZE_INFORMATION;

  _FILE_FS_ATTRIBUTE_INFORMATION = record
    FileSystemFlags: ULONG;
    MaximumComponentNameLength: ULONG;
    FileSystemNameLength: ULONG;
    FileSystemName: array [0..0] of WCHAR
  end;
  FILE_FS_ATTRIBUTE_INFORMATION = _FILE_FS_ATTRIBUTE_INFORMATION;
  PFILE_FS_ATTRIBUTE_INFORMATION = ^FILE_FS_ATTRIBUTE_INFORMATION;

  _FILE_FS_CONTROL_INFORMATION = record
    Reserved: array [0..2] of LARGE_INTEGER;
    DefaultQuotaThreshold: LARGE_INTEGER;
    DefaultQuotaLimit: LARGE_INTEGER;
    QuotaFlags: ULONG;
  end;
  FILE_FS_CONTROL_INFORMATION = _FILE_FS_CONTROL_INFORMATION;
  PFILE_FS_CONTROL_INFORMATION = ^FILE_FS_CONTROL_INFORMATION;

  _FILE_FS_FULL_SIZE_INFORMATION = record
    TotalQuotaAllocationUnits: LARGE_INTEGER;
    AvailableQuotaAllocationUnits: LARGE_INTEGER;
    AvailableAllocationUnits: LARGE_INTEGER;
    SectorsPerAllocationUnit: ULONG;
    BytesPerSector: ULONG;
  end;
  FILE_FS_FULL_SIZE_INFORMATION = _FILE_FS_FULL_SIZE_INFORMATION;
  PFILE_FS_FULL_SIZE_INFORMATION = ^FILE_FS_FULL_SIZE_INFORMATION;

  _FILE_FS_OBJECT_ID_INFORMATION = record
    VolumeObjectId: UUID;
    VolumeObjectIdExtendedInfo: array [0..11] of ULONG;
  end;
  FILE_FS_OBJECT_ID_INFORMATION = _FILE_FS_OBJECT_ID_INFORMATION;
  PFILE_FS_OBJECT_ID_INFORMATION = ^FILE_FS_OBJECT_ID_INFORMATION;

  _FILE_USER_QUOTA_INFORMATION = record
    NextEntryOffset: ULONG;
    SidLength: ULONG;
    ChangeTime: LARGE_INTEGER;
    QuotaUsed: LARGE_INTEGER;
    QuotaThreshold: LARGE_INTEGER;
    QuotaLimit: LARGE_INTEGER;
    Sid: array [0..0] of SID;
  end;
  FILE_USER_QUOTA_INFORMATION = _FILE_USER_QUOTA_INFORMATION;
  PFILE_USER_QUOTA_INFORMATION = ^FILE_USER_QUOTA_INFORMATION;

  _FILE_QUOTA_LIST_INFORMATION = record
    NextEntryOffset: ULONG;
    SidLength: ULONG;
    Sid: array [0..0] of SID;
  end;
  FILE_QUOTA_LIST_INFORMATION = _FILE_QUOTA_LIST_INFORMATION;
  PFILE_QUOTA_LIST_INFORMATION = ^FILE_QUOTA_LIST_INFORMATION;

function NtQueryQuotaInformationFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK; Buffer: PFILE_USER_QUOTA_INFORMATION; BufferLength: ULONG; ReturnSingleEntry: ByteBool; QuotaList: PFILE_QUOTA_LIST_INFORMATION; QuotaListLength: ULONG; ResumeSid: PSID; RestartScan: ByteBool): NTSTATUS; stdcall;
function NtSetQuotaInformationFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK; Buffer: PFILE_USER_QUOTA_INFORMATION; BufferLength: ULONG): NTSTATUS; stdcall;
function NtQueryAttributesFile(ObjectAttributes: POBJECT_ATTRIBUTES; FileInformation: PFILE_BASIC_INFORMATION): NTSTATUS; stdcall;
function NtQueryFullAttributesFile(ObjectAttributes: POBJECT_ATTRIBUTES; FileInformation: PFILE_NETWORK_OPEN_INFORMATION): NTSTATUS; stdcall;
function NtQueryInformationFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK; FileInformation: PVOID; FileInformationLength: ULONG; FileInformationClass: FILE_INFORMATION_CLASS): NTSTATUS; stdcall;
function NtSetInformationFile(FileHandle: HANDLE; IoStatusBlock: PIO_STATUS_BLOCK; FileInformation: PVOID; FileInformationLength: ULONG; FileInformationClass: FILE_INFORMATION_CLASS): NTSTATUS; stdcall;
function NtQueryDirectoryFile(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; FileInformation: PVOID; FileInformationLength: ULONG; FileInformationClass: FILE_INFORMATION_CLASS; ReturnSingleEntry: ByteBool; FileName: PUNICODE_STRING; RestartScan: ByteBool): NTSTATUS; stdcall;

type
  _FILE_DIRECTORY_INFORMATION = record // Information Class 1
    NextEntryOffset: ULONG;
    Unknown: ULONG;
    CreationTime: LARGE_INTEGER;
    LastAccessTime: LARGE_INTEGER;
    LastWriteTime: LARGE_INTEGER;
    ChangeTime: LARGE_INTEGER;
    EndOfFile: LARGE_INTEGER;
    AllocationSize: LARGE_INTEGER;
    FileAttributes: ULONG;
    FileNameLength: ULONG;
    FileName: array [0..0] of WCHAR
  end;
  FILE_DIRECTORY_INFORMATION = _FILE_DIRECTORY_INFORMATION;
  PFILE_DIRECTORY_INFORMATION = ^FILE_DIRECTORY_INFORMATION;

  _FILE_FULL_DIRECTORY_INFORMATION = record // Information Class 2
    NextEntryOffset: ULONG;
    Unknown: ULONG;
    CreationTime: LARGE_INTEGER;
    LastAccessTime: LARGE_INTEGER;
    LastWriteTime: LARGE_INTEGER;
    ChangeTime: LARGE_INTEGER;
    EndOfFile: LARGE_INTEGER;
    AllocationSize: LARGE_INTEGER;
    FileAttributes: ULONG;
    FileNameLength: ULONG;
    EaInformationLength: ULONG;
    FileName: array [0..0] of WCHAR
  end;
  FILE_FULL_DIRECTORY_INFORMATION = _FILE_FULL_DIRECTORY_INFORMATION;
  PFILE_FULL_DIRECTORY_INFORMATION = ^FILE_FULL_DIRECTORY_INFORMATION;

  _FILE_BOTH_DIRECTORY_INFORMATION = record // Information Class 3
    NextEntryOffset: ULONG;
    Unknown: ULONG;
    CreationTime: LARGE_INTEGER;
    LastAccessTime: LARGE_INTEGER;
    LastWriteTime: LARGE_INTEGER;
    ChangeTime: LARGE_INTEGER;
    EndOfFile: LARGE_INTEGER;
    AllocationSize: LARGE_INTEGER;
    FileAttributes: ULONG;
    FileNameLength: ULONG;
    EaInformationLength: ULONG;
    AlternateNameLength: UCHAR;
    AlternateName: array [0..11] of WCHAR;
    FileName: array [0..0] of WCHAR;
  end;
  FILE_BOTH_DIRECTORY_INFORMATION = _FILE_BOTH_DIRECTORY_INFORMATION;
  PFILE_BOTH_DIRECTORY_INFORMATION = ^FILE_BOTH_DIRECTORY_INFORMATION;

  _FILE_INTERNAL_INFORMATION = record // Information Class 6
    FileId: LARGE_INTEGER;
  end;
  FILE_INTERNAL_INFORMATION = _FILE_INTERNAL_INFORMATION;
  PFILE_INTERNAL_INFORMATION = ^FILE_INTERNAL_INFORMATION;

  _FILE_EA_INFORMATION = record // Information Class 7
    EaInformationLength: ULONG;
  end;
  FILE_EA_INFORMATION = _FILE_EA_INFORMATION;
  PFILE_EA_INFORMATION = ^FILE_EA_INFORMATION;

  _FILE_ACCESS_INFORMATION = record // Information Class 8
    GrantedAccess: ACCESS_MASK;
  end;
  FILE_ACCESS_INFORMATION = _FILE_ACCESS_INFORMATION;
  PFILE_ACCESS_INFORMATION = ^FILE_ACCESS_INFORMATION;

  _FILE_NAME_INFORMATION = record // Information Classes 9 and 21
    FileNameLength: ULONG;
    FileName: array [0..0] of WCHAR;
  end;
  FILE_NAME_INFORMATION = _FILE_NAME_INFORMATION;
  PFILE_NAME_INFORMATION = ^FILE_NAME_INFORMATION;
  FILE_ALTERNATE_NAME_INFORMATION = _FILE_NAME_INFORMATION;
  PFILE_ALTERNATE_NAME_INFORMATION = ^FILE_ALTERNATE_NAME_INFORMATION;

  _FILE_LINK_RENAME_INFORMATION = record // Info Classes 10 and 11
    ReplaceIfExists: ByteBool;
    RootDirectory: HANDLE;
    FileNameLength: ULONG;
    FileName: array [0..0] of WCHAR;
  end;
  FILE_LINK_INFORMATION = _FILE_LINK_RENAME_INFORMATION;
  PFILE_LINK_INFORMATION = ^FILE_LINK_INFORMATION;
  FILE_RENAME_INFORMATION = _FILE_LINK_RENAME_INFORMATION;
  PFILE_RENAME_INFORMATION= ^FILE_RENAME_INFORMATION;

  _FILE_NAMES_INFORMATION = record // Information Class 12
    NextEntryOffset: ULONG;
    Unknown: ULONG;
    FileNameLength: ULONG;
    FileName: array [0..0] of WCHAR;
  end;
  FILE_NAMES_INFORMATION = _FILE_NAMES_INFORMATION;
  PFILE_NAMES_INFORMATION = ^FILE_NAMES_INFORMATION;

  _FILE_MODE_INFORMATION = record // Information Class 16
    Mode: ULONG;
  end;
  FILE_MODE_INFORMATION = _FILE_MODE_INFORMATION;
  PFILE_MODE_INFORMATION = ^FILE_MODE_INFORMATION;

  _FILE_ALL_INFORMATION = record // Information Class 18
    BasicInformation: FILE_BASIC_INFORMATION;
    StandardInformation: FILE_STANDARD_INFORMATION;
    InternalInformation: FILE_INTERNAL_INFORMATION;
    EaInformation: FILE_EA_INFORMATION;
    AccessInformation: FILE_ACCESS_INFORMATION;
    PositionInformation: FILE_POSITION_INFORMATION;
    ModeInformation: FILE_MODE_INFORMATION;
    AlignmentInformation: FILE_ALIGNMENT_INFORMATION;
    NameInformation: FILE_NAME_INFORMATION;
  end;
  FILE_ALL_INFORMATION = _FILE_ALL_INFORMATION;
  PFILE_ALL_INFORMATION = ^FILE_ALL_INFORMATION;

  _FILE_ALLOCATION_INFORMATION = record // Information Class 19
    AllocationSize: LARGE_INTEGER;
  end;
  FILE_ALLOCATION_INFORMATION = _FILE_ALLOCATION_INFORMATION;
  PFILE_ALLOCATION_INFORMATION = ^FILE_ALLOCATION_INFORMATION;

  _FILE_STREAM_INFORMATION = record // Information Class 22
    NextEntryOffset: ULONG;
    StreamNameLength: ULONG;
    EndOfStream: LARGE_INTEGER;
    AllocationSize: LARGE_INTEGER;
    StreamName: array [0..0] of WCHAR;
  end;
  FILE_STREAM_INFORMATION = _FILE_STREAM_INFORMATION;
  PFILE_STREAM_INFORMATION = ^FILE_STREAM_INFORMATION;

  _FILE_PIPE_INFORMATION = record // Information Class 23
    ReadModeMessage: ULONG;
    WaitModeBlocking: ULONG;
  end;
  FILE_PIPE_INFORMATION = _FILE_PIPE_INFORMATION;
  PFILE_PIPE_INFORMATION = ^FILE_PIPE_INFORMATION;

  _FILE_PIPE_LOCAL_INFORMATION = record // Information Class 24
    MessageType: ULONG;
    Unknown1: ULONG;
    MaxInstances: ULONG;
    CurInstances: ULONG;
    InBufferSize: ULONG;
    Unknown2: ULONG;
    OutBufferSize: ULONG;
    Unknown3: array [0..1] of ULONG;
    ServerEnd: ULONG;
  end;
  FILE_PIPE_LOCAL_INFORMATION = _FILE_PIPE_LOCAL_INFORMATION;
  PFILE_PIPE_LOCAL_INFORMATION = ^FILE_PIPE_LOCAL_INFORMATION;

  _FILE_PIPE_REMOTE_INFORMATION = record // Information Class 25
    CollectDataTimeout: LARGE_INTEGER;
    MaxCollectionCount: ULONG;
  end;
  FILE_PIPE_REMOTE_INFORMATION = _FILE_PIPE_REMOTE_INFORMATION;
  PFILE_PIPE_REMOTE_INFORMATION = ^FILE_PIPE_REMOTE_INFORMATION;

  _FILE_MAILSLOT_QUERY_INFORMATION = record // Information Class 26
    MaxMessageSize: ULONG;
    Unknown: ULONG;
    NextSize: ULONG;
    MessageCount: ULONG;
    ReadTimeout: LARGE_INTEGER;
  end;
  FILE_MAILSLOT_QUERY_INFORMATION = _FILE_MAILSLOT_QUERY_INFORMATION;
  PFILE_MAILSLOT_QUERY_INFORMATION = ^FILE_MAILSLOT_QUERY_INFORMATION;

  _FILE_MAILSLOT_SET_INFORMATION = record // Information Class 27
    ReadTimeout: LARGE_INTEGER;
  end;
  FILE_MAILSLOT_SET_INFORMATION = _FILE_MAILSLOT_SET_INFORMATION;
  PFILE_MAILSLOT_SET_INFORMATION = ^FILE_MAILSLOT_SET_INFORMATION;

  _FILE_COMPRESSION_INFORMATION = record // Information Class 28
    CompressedSize: LARGE_INTEGER;
    CompressionFormat: USHORT;
    CompressionUnitShift: UCHAR;
    Unknown: UCHAR;
    ClusterSizeShift: UCHAR;
  end;
  FILE_COMPRESSION_INFORMATION = _FILE_COMPRESSION_INFORMATION;
  PFILE_COMPRESSION_INFORMATION = ^FILE_COMPRESSION_INFORMATION;

  _FILE_COMPLETION_INFORMATION = record // Information Class 30
    IoCompletionHandle: HANDLE;
    CompletionKey: ULONG;
  end;
  FILE_COMPLETION_INFORMATION = _FILE_COMPLETION_INFORMATION;
  PFILE_COMPLETION_INFORMATION = ^FILE_COMPLETION_INFORMATION;

function NtCreateKey(KeyHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES; TitleIndex: ULONG; Class_: PUNICODE_STRING; CreateOptions: ULONG; Disposition: PULONG): NTSTATUS; stdcall;
function NtOpenKey(KeyHandle: PHANDLE; DesiredAccess: ACCESS_MASK; ObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtDeleteKey(KeyHandle: HANDLE): NTSTATUS; stdcall;
function NtFlushKey(KeyHandle: HANDLE): NTSTATUS; stdcall;
function NtSaveKey(KeyHandle: HANDLE; FileHandle: HANDLE): NTSTATUS; stdcall;
function NtSaveMergedKeys(KeyHandle1: HANDLE; KeyHandle2: HANDLE; FileHandle: HANDLE): NTSTATUS; stdcall;
function NtRestoreKey(KeyHandle: HANDLE; FileHandle: HANDLE; Flags: ULONG): NTSTATUS; stdcall;
function NtLoadKey(KeyObjectAttributes: POBJECT_ATTRIBUTES; FileObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtLoadKey2(KeyObjectAttributes: POBJECT_ATTRIBUTES; FileObjectAttributes: POBJECT_ATTRIBUTES; Flags: ULONG): NTSTATUS; stdcall;
function NtUnloadKey(KeyObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtReplaceKey(NewFileObjectAttributes: POBJECT_ATTRIBUTES; KeyHandle: HANDLE; OldFileObjectAttributes: POBJECT_ATTRIBUTES): NTSTATUS; stdcall;
function NtSetInformationKey(KeyHandle: HANDLE; KeyInformationClass: KEY_SET_INFORMATION_CLASS; KeyInformation: PVOID; KeyInformationLength: ULONG): NTSTATUS; stdcall;
function NtQueryKey(KeyHandle: HANDLE; KeyInformationClass: KEY_INFORMATION_CLASS; KeyInformation: PVOID; KeyInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;
function NtEnumerateKey(KeyHandle: HANDLE; Index: ULONG; KeyInformationClass: KEY_INFORMATION_CLASS; KeyInformation: PVOID; KeyInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;
function NtNotifyChangeKey(KeyHandle: HANDLE; EventHandle: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; NotifyFilter: ULONG; WatchSubtree: ByteBool; Buffer: PVOID; BufferLength: ULONG; Asynchronous: ByteBool): NTSTATUS; stdcall;
function NtNotifyChangeMultipleKeys(KeyHandle: HANDLE; Flags: ULONG; KeyObjectAttributes: POBJECT_ATTRIBUTES; EventHandle: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; NotifyFilter: ULONG; WatchSubtree: ByteBool; Buffer: PVOID; BufferLength: ULONG; Asynchronous: ByteBool): NTSTATUS; stdcall;
function NtDeleteValueKey(KeyHandle: HANDLE; ValueName: PUNICODE_STRING): NTSTATUS; stdcall;
function NtSetValueKey(KeyHandle: HANDLE; ValueName: PUNICODE_STRING; TitleIndex: ULONG; Type_: ULONG; Data: PVOID; DataSize: ULONG): NTSTATUS; stdcall;
function NtQueryValueKey(KeyHandle: HANDLE; ValueName: PUNICODE_STRING; KeyValueInformationClass: KEY_VALUE_INFORMATION_CLASS; KeyValueInformation: PVOID; KeyValueInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;
function NtEnumerateValueKey(KeyHandle: HANDLE; Index: ULONG; KeyValueInformationClass: KEY_VALUE_INFORMATION_CLASS; KeyValueInformation: PVOID; KeyValueInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;
function NtQueryMultipleValueKey(KeyHandle: HANDLE; ValueList: PKEY_VALUE_ENTRY; NumberOfValues: ULONG; Buffer: PVOID; Length: PULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
function NtPrivilegeCheck(TokenHandle: HANDLE; RequiredPrivileges: PPRIVILEGE_SET; Result: PBOOLEAN): NTSTATUS; stdcall;
function NtPrivilegeObjectAuditAlarm(SubsystemName: PUNICODE_STRING; HandleId: PVOID; TokenHandle: HANDLE; DesiredAccess: ACCESS_MASK; Privileges: PPRIVILEGE_SET; AccessGranted: ByteBool): NTSTATUS; stdcall;
function NtPrivilegedServiceAuditAlarm(SubsystemName: PUNICODE_STRING; ServiceName: PUNICODE_STRING; TokenHandle: HANDLE; Privileges: PPRIVILEGE_SET; AccessGranted: ByteBool): NTSTATUS; stdcall;
function NtAccessCheck(SecurityDescriptor: PSECURITY_DESCRIPTOR; TokenHandle: HANDLE; DesiredAccess: ACCESS_MASK; GenericMapping: PGENERIC_MAPPING; PrivilegeSet: PPRIVILEGE_SET; PrivilegeSetLength: PULONG; GrantedAccess: PACCESS_MASK; AccessStatus: PBOOLEAN): NTSTATUS; stdcall;
function NtAccessCheckAndAuditAlarm(SubsystemName: PUNICODE_STRING; HandleId: PVOID; ObjectTypeName: PUNICODE_STRING; ObjectName: PUNICODE_STRING; SecurityDescriptor: PSECURITY_DESCRIPTOR; DesiredAccess: ACCESS_MASK; GenericMapping: PGENERIC_MAPPING; ObjectCreation: ByteBool; GrantedAccess: PACCESS_MASK; AccessStatus: PBOOLEAN; GenerateOnClose: PBOOLEAN): NTSTATUS; stdcall;
function NtAccessCheckByType(SecurityDescriptor: PSECURITY_DESCRIPTOR; PrincipalSelfSid: PSID; TokenHandle: HANDLE; DesiredAccess: ULONG; ObjectTypeList: POBJECT_TYPE_LIST; ObjectTypeListLength: ULONG; GenericMapping: PGENERIC_MAPPING; PrivilegeSet: PPRIVILEGE_SET; PrivilegeSetLength: PULONG; GrantedAccess: PACCESS_MASK; AccessStatus: PULONG): NTSTATUS; stdcall;
function NtAccessCheckByTypeAndAuditAlarm(SubsystemName: PUNICODE_STRING; HandleId: PVOID; ObjectTypeName: PUNICODE_STRING; ObjectName: PUNICODE_STRING; SecurityDescriptor: PSECURITY_DESCRIPTOR; PrincipalSelfSid: PSID; DesiredAccess: ACCESS_MASK; AuditType: AUDIT_EVENT_TYPE; Flags: ULONG; ObjectTypeList: POBJECT_TYPE_LIST; ObjectTypeListLength: ULONG; GenericMapping: PGENERIC_MAPPING; ObjectCreation: ByteBool; GrantedAccess: PACCESS_MASK; AccessStatus: PULONG; GenerateOnClose: PBOOLEAN): NTSTATUS; stdcall;
function NtAccessCheckByTypeResultList(SecurityDescriptor: PSECURITY_DESCRIPTOR; PrincipalSelfSid: PSID; TokenHandle: HANDLE; DesiredAccess: ACCESS_MASK; ObjectTypeList: POBJECT_TYPE_LIST; ObjectTypeListLength: ULONG; GenericMapping: PGENERIC_MAPPING; PrivilegeSet: PPRIVILEGE_SET; PrivilegeSetLength: PULONG; GrantedAccessList: PACCESS_MASK; AccessStatusList: PULONG): NTSTATUS; stdcall;
function NtAccessCheckByTypeResultListAndAuditAlarm(SubsystemName: PUNICODE_STRING; HandleId: PVOID; ObjectTypeName: PUNICODE_STRING; ObjectName: PUNICODE_STRING; SecurityDescriptor: PSECURITY_DESCRIPTOR; PrincipalSelfSid: PSID; DesiredAccess: ACCESS_MASK; AuditType: AUDIT_EVENT_TYPE; Flags: ULONG; ObjectTypeList: POBJECT_TYPE_LIST; ObjectTypeListLength: ULONG; GenericMapping: PGENERIC_MAPPING; ObjectCreation: ByteBool; GrantedAccessList: PACCESS_MASK; AccessStatusList: PULONG; GenerateOnClose: PULONG): NTSTATUS; stdcall;
function NtAccessCheckByTypeResultListAndAuditAlarmByHandle(SubsystemName: PUNICODE_STRING; HandleId: PVOID; TokenHandle: HANDLE; ObjectTypeName: PUNICODE_STRING; ObjectName: PUNICODE_STRING; SecurityDescriptor: PSECURITY_DESCRIPTOR; PrincipalSelfSid: PSID; DesiredAccess: ACCESS_MASK; AuditType: AUDIT_EVENT_TYPE; Flags: ULONG; ObjectTypeList: POBJECT_TYPE_LIST; ObjectTypeListLength: ULONG; GenericMapping: PGENERIC_MAPPING; ObjectCreation: ByteBool; GrantedAccessList: PACCESS_MASK; AccessStatusList: PULONG; GenerateOnClose: PULONG): NTSTATUS; stdcall;
function NtOpenObjectAuditAlarm(SubsystemName: PUNICODE_STRING; HandleId: PPVOID; ObjectTypeName: PUNICODE_STRING; ObjectName: PUNICODE_STRING; SecurityDescriptor: PSECURITY_DESCRIPTOR; TokenHandle: HANDLE; DesiredAccess: ACCESS_MASK; GrantedAccess: ACCESS_MASK; Privileges: PPRIVILEGE_SET; ObjectCreation: ByteBool; AccessGranted: ByteBool; GenerateOnClose: PBOOLEAN): NTSTATUS; stdcall;
function NtCloseObjectAuditAlarm(SubsystemName: PUNICODE_STRING; HandleId: PVOID; GenerateOnClose: ByteBool): NTSTATUS; stdcall;
function NtDeleteObjectAuditAlarm(SubsystemName: PUNICODE_STRING; HandleId: PVOID; GenerateOnClose: ByteBool): NTSTATUS; stdcall;
function NtRequestWakeupLatency(Latency: LATENCY_TIME): NTSTATUS; stdcall;
function NtRequestDeviceWakeup(DeviceHandle: HANDLE): NTSTATUS; stdcall;
function NtCancelDeviceWakeupRequest(DeviceHandle: HANDLE): NTSTATUS; stdcall;
function NtIsSystemResumeAutomatic: ByteBool; stdcall;

type
  PEXECUTION_STATE = ^EXECUTION_STATE;

function NtSetThreadExecutionState(ExecutionState: EXECUTION_STATE; PreviousExecutionState: PEXECUTION_STATE): NTSTATUS; stdcall;
function NtGetDevicePowerState(DeviceHandle: HANDLE; DevicePowerState: PDEVICE_POWER_STATE): NTSTATUS; stdcall;
function NtSetSystemPowerState(SystemAction: POWER_ACTION; MinSystemState: SYSTEM_POWER_STATE; Flags: ULONG): NTSTATUS; stdcall;
function NtInitiatePowerAction(SystemAction: POWER_ACTION; MinSystemState: SYSTEM_POWER_STATE; Flags: ULONG; Asynchronous: ByteBool): NTSTATUS; stdcall;
function NtPowerInformation(PowerInformationLevel: POWER_INFORMATION_LEVEL; InputBuffer: PVOID; InputBufferLength: ULONG; OutputBuffer: PVOID; OutputBufferLength: ULONG): NTSTATUS; stdcall;
function NtGetPlugPlayEvent(Reserved1: ULONG; Reserved2: ULONG; Buffer: PVOID; BufferLength: ULONG): NTSTATUS; stdcall;
function NtRaiseException(ExceptionRecord: PEXCEPTION_RECORD; Context: PCONTEXT; SearchFrames: ByteBool): NTSTATUS; stdcall;
function NtContinue(Context: PCONTEXT; TestAlert: ByteBool): NTSTATUS; stdcall;

// TODO NOT EXPORTED FROM NTDLL
//function ZwW32Call(RoutineIndex: ULONG; Argument: PVOID; ArgumentLength: ULONG; Result: PPVOID; ResultLength: PULONG): NTSTATUS; stdcall;

function NtCallbackReturn(Result: PVOID; ResultLength: ULONG; Status: NTSTATUS): NTSTATUS; stdcall;

// TODO NOT EXPORTED FROM NTDLL
//function ZwSetLowWaitHighThread: NTSTATUS; stdcall;
//function ZwSetHighWaitLowThread: NTSTATUS; stdcall;

function NtLoadDriver(DriverServiceName: PUNICODE_STRING): NTSTATUS; stdcall;
function NtUnloadDriver(DriverServiceName: PUNICODE_STRING): NTSTATUS; stdcall;
function NtFlushInstructionCache(ProcessHandle: HANDLE; BaseAddress: PVOID; FlushSize: ULONG): NTSTATUS; stdcall;
function NtFlushWriteBuffer: NTSTATUS; stdcall;
function NtQueryDefaultLocale(ThreadOrSystem: ByteBool; Locale: PLCID): NTSTATUS; stdcall;
function NtSetDefaultLocale(ThreadOrSystem: ByteBool; Locale: LCID): NTSTATUS; stdcall;

type
  PLANGID = ^LANGID;

function NtQueryDefaultUILanguage(LanguageId: PLANGID): NTSTATUS; stdcall;
function NtSetDefaultUILanguage(LanguageId: LANGID): NTSTATUS; stdcall;
function NtQueryInstallUILanguage(LanguageId: PLANGID): NTSTATUS; stdcall;
function NtAllocateLocallyUniqueId(Luid: PLUID): NTSTATUS; stdcall;
function NtAllocateUuids(UuidLastTimeAllocated: PLARGE_INTEGER; UuidDeltaTime: PULONG; UuidSequenceNumber: PULONG; UuidSeed: PUCHAR): NTSTATUS; stdcall;
function NtSetUuidSeed(UuidSeed: PUCHAR): NTSTATUS; stdcall;
function NtRaiseHardError(Status: NTSTATUS; NumberOfArguments: ULONG; StringArgumentsMask: ULONG; Arguments: PULONG; MessageBoxType: ULONG; MessageBoxResult: PULONG): NTSTATUS; stdcall;
function NtSetDefaultHardErrorPort(PortHandle: HANDLE): NTSTATUS; stdcall;
function NtDisplayString(Str: PUNICODE_STRING): NTSTATUS; stdcall;
function NtCreatePagingFile(FileName: PUNICODE_STRING; InitialSize: PULARGE_INTEGER; MaximumSize: PULARGE_INTEGER; Reserved: ULONG): NTSTATUS; stdcall;
function NtAddAtom(Str: PWSTR; StringLength: ULONG; Atom: PUSHORT): NTSTATUS; stdcall;
function NtFindAtom(Str: PWSTR; StringLength: ULONG; Atom: PUSHORT): NTSTATUS; stdcall;
function NtDeleteAtom(Atom: USHORT): NTSTATUS; stdcall;

type
  _ATOM_INFORMATION_CLASS = (AtomBasicInformation, AtomListInformation);
  ATOM_INFORMATION_CLASS = _ATOM_INFORMATION_CLASS;

function NtQueryInformationAtom(Atom: USHORT; AtomInformationClass: ATOM_INFORMATION_CLASS; AtomInformation: PVOID; AtomInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;

type
  _ATOM_BASIC_INFORMATION = record
    ReferenceCount: USHORT;
    Pinned: USHORT;
    NameLength: USHORT;
    Name: array [0..0] of WCHAR;
  end;
  ATOM_BASIC_INFORMATION = _ATOM_BASIC_INFORMATION;
  PATOM_BASIC_INFORMATION = ^ATOM_BASIC_INFORMATION;

  _ATOM_LIST_INFORMATION = record
    NumberOfAtoms: ULONG;
    Atoms: array [0..0] of ATOM;
  end;
  ATOM_LIST_INFORMATION = _ATOM_LIST_INFORMATION;
  PATOM_LIST_INFORMATION = ^ATOM_LIST_INFORMATION;

function NtSetLdtEntries(Selector1: ULONG; LdtEntry1: LDT_ENTRY; Selector2: ULONG; LdtEntry2: LDT_ENTRY): NTSTATUS; stdcall;
function NtVdmControl(ControlCode: ULONG; ControlData: PVOID): NTSTATUS; stdcall;

//==============================================================================
// NTFS on disk structure structures
//==============================================================================

type
  _NTFS_RECORD_HEADER = record
    Type_: ULONG;
    UsaOffset: USHORT;
    UsaCount: USHORT;
    Usn: USN;
  end;
  NTFS_RECORD_HEADER = _NTFS_RECORD_HEADER;
  PNTFS_RECORD_HEADER = ^NTFS_RECORD_HEADER;

  _FILE_RECORD_HEADER = record
    Ntfs: NTFS_RECORD_HEADER;
    SequenceNumber: USHORT;
    LinkCount: USHORT;
    AttributesOffset: USHORT;
    Flags: USHORT;               // 0x0001 = InUse, 0x0002 = Directory
    BytesInUse: ULONG;
    BytesAllocated: ULONG;
    BaseFileRecord: ULONGLONG;
    NextAttributeNumber: USHORT;
  end;
  FILE_RECORD_HEADER = _FILE_RECORD_HEADER;
  PFILE_RECORD_HEADER = ^FILE_RECORD_HEADER;

const
  AttributeStandardInformation = $10;
  AttributeAttributeList = $20;
  AttributeFileName = $30;
  AttributeObjectId = $40;
  AttributeSecurityDescriptor = $50;
  AttributeVolumeName = $60;
  AttributeVolumeInformation = $70;
  AttributeData = $80;
  AttributeIndexRoot = $90;
  AttributeIndexAllocation = $A0;
  AttributeBitmap = $B0;
  AttributeReparsePoint = $C0;
  AttributeEAInformation = $D0;
  AttributeEA = $E0;
  AttributePropertySet = $F0;
  AttributeLoggedUtilityStream = $100;

type
  ATTRIBUTE_TYPE = AttributeStandardInformation..AttributeLoggedUtilityStream;
  PATTRIBUTE_TYPE = ^ATTRIBUTE_TYPE;

  _ATTRIBUTE = record
    AttributeType: ATTRIBUTE_TYPE;
    Length: ULONG;
    Nonresident: ByteBool;
    NameLength: UCHAR;
    NameOffset: USHORT;
    Flags: USHORT;               // 0x0001 = Compressed
    AttributeNumber: USHORT;
  end;
  ATTRIBUTE = _ATTRIBUTE;
  PATTRIBUTE = ^ATTRIBUTE;

  _RESIDENT_ATTRIBUTE = record
    Attribute: ATTRIBUTE;
    ValueLength: ULONG;
    ValueOffset: USHORT;
    Flags: USHORT;               // 0x0001 = Indexed
  end;
  RESIDENT_ATTRIBUTE = _RESIDENT_ATTRIBUTE;
  PRESIDENT_ATTRIBUTE = ^RESIDENT_ATTRIBUTE;

  _NONRESIDENT_ATTRIBUTE = record
    Attribute: ATTRIBUTE;
    LowVcn: ULONGLONG;
    HighVcn: ULONGLONG;
    RunArrayOffset: USHORT;
    CompressionUnit: UCHAR;
    AlignmentOrReserved: array [0..4] of UCHAR;
    AllocatedSize: ULONGLONG;
    DataSize: ULONGLONG;
    InitializedSize: ULONGLONG;
    CompressedSize: ULONGLONG;    // Only when compressed
  end;
  NONRESIDENT_ATTRIBUTE = _NONRESIDENT_ATTRIBUTE;
  PNONRESIDENT_ATTRIBUTE = ^NONRESIDENT_ATTRIBUTE;

  _STANDARD_INFORMATION = record
    CreationTime: ULONGLONG;
    ChangeTime: ULONGLONG;
    LastWriteTime: ULONGLONG;
    LastAccessTime: ULONGLONG;
    FileAttributes: ULONG;
    AlignmentOrReservedOrUnknown: array [0..2] of ULONG;
    QuotaId: ULONG;                        // NTFS 3.0 only
    SecurityId: ULONG;                     // NTFS 3.0 only
    QuotaCharge: ULONGLONG;                // NTFS 3.0 only
    Usn: USN;                              // NTFS 3.0 only
  end;
  STANDARD_INFORMATION = _STANDARD_INFORMATION;
  PSTANDARD_INFORMATION = ^STANDARD_INFORMATION;

  _ATTRIBUTE_LIST = record
    AttributeType: ATTRIBUTE_TYPE;
    Length: USHORT;
    NameLength: UCHAR;
    NameOffset: UCHAR;
    LowVcn: ULONGLONG;
    FileReferenceNumber: ULONGLONG;
    AttributeNumber: USHORT;
    AlignmentOrReserved: array [0..2] of USHORT;
  end;
  ATTRIBUTE_LIST = _ATTRIBUTE_LIST;
  PATTRIBUTE_LIST = ^ATTRIBUTE_LIST;

  _FILENAME_ATTRIBUTE = record
    DirectoryFileReferenceNumber: ULONGLONG;
    CreationTime: ULONGLONG;   // Saved when filename last changed
    ChangeTime: ULONGLONG;     // ditto
    LastWriteTime: ULONGLONG;  // ditto
    LastAccessTime: ULONGLONG; // ditto
    AllocatedSize: ULONGLONG;  // ditto
    DataSize: ULONGLONG;       // ditto
    FileAttributes: ULONG;     // ditto
    AlignmentOrReserved: ULONG;
    NameLength: UCHAR;
    NameType: UCHAR;           // 0x01 = Long, 0x02 = Short
    Name: array [0..0] of UCHAR;
  end;
  FILENAME_ATTRIBUTE = _FILENAME_ATTRIBUTE;
  PFILENAME_ATTRIBUTE = ^FILENAME_ATTRIBUTE;

  _OBJECTID_ATTRIBUTE = record
    ObjectId: GUID;
    case Integer of
      0: (
        BirthVolumeId: GUID;
        BirthObjectId: GUID;
        DomainId: GUID);
      1: (
        ExtendedInfo: array [0..47] of UCHAR);
  end;
  OBJECTID_ATTRIBUTE = _OBJECTID_ATTRIBUTE;
  POBJECTID_ATTRIBUTE = ^OBJECTID_ATTRIBUTE;

  _VOLUME_INFORMATION = record
    Unknown: array [0..1] of ULONG;
    MajorVersion: UCHAR;
    MinorVersion: UCHAR;
    Flags: USHORT;
  end;
  VOLUME_INFORMATION = _VOLUME_INFORMATION;
  PVOLUME_INFORMATION = ^VOLUME_INFORMATION;

  _DIRECTORY_INDEX = record
    EntriesOffset: ULONG;
    IndexBlockLength: ULONG;
    AllocatedSize: ULONG;
    Flags: ULONG;         // 0x00 = Small directory, 0x01 = Large directory
  end;
  DIRECTORY_INDEX = _DIRECTORY_INDEX;
  PDIRECTORY_INDEX = ^DIRECTORY_INDEX;

  _DIRECTORY_ENTRY = record
    FileReferenceNumber: ULONGLONG;
    Length: USHORT;
    AttributeLength: USHORT;
    Flags: ULONG;           // 0x01 = Has trailing VCN, 0x02 = Last entry
    // FILENAME_ATTRIBUTE Name;
    // ULONGLONG Vcn;       // VCN in IndexAllocation of earlier entries
  end;
  DIRECTORY_ENTRY = _DIRECTORY_ENTRY;
  PDIRECTORY_ENTRY = ^DIRECTORY_ENTRY;

  _INDEX_ROOT = record
    Type_: ATTRIBUTE_TYPE;
    CollationRule: ULONG;
    BytesPerIndexBlock: ULONG;
    ClustersPerIndexBlock: ULONG;
    DirectoryIndex: DIRECTORY_INDEX;
  end;
  INDEX_ROOT = _INDEX_ROOT;
  PINDEX_ROOT = ^INDEX_ROOT;

  _INDEX_BLOCK_HEADER = record
    Ntfs: NTFS_RECORD_HEADER;
    IndexBlockVcn: ULONGLONG;
    DirectoryIndex: DIRECTORY_INDEX;
  end;
  INDEX_BLOCK_HEADER = _INDEX_BLOCK_HEADER;
  PINDEX_BLOCK_HEADER = ^INDEX_BLOCK_HEADER;

  _REPARSE_POINT = record
    ReparseTag: ULONG;
    ReparseDataLength: USHORT;
    Reserved: USHORT;
    ReparseData: array [0..0] of UCHAR;
  end;
  REPARSE_POINT = _REPARSE_POINT;
  PREPARSE_POINT = ^REPARSE_POINT;

  _EA_INFORMATION = record
    EaLength: ULONG;
    EaQueryLength: ULONG;
  end;
  EA_INFORMATION = _EA_INFORMATION;
  PEA_INFORMATION = ^EA_INFORMATION;

  _EA_ATTRIBUTE = record
     NextEntryOffset: ULONG;
    Flags: UCHAR;
    EaNameLength: UCHAR;
    EaValueLength: USHORT;
    EaName: array [0..0] of CHAR;
    // UCHAR EaData[];
  end;
  EA_ATTRIBUTE = _EA_ATTRIBUTE;
  PEA_ATTRIBUTE = ^EA_ATTRIBUTE;

  _ATTRIBUTE_DEFINITION = record
    AttributeName: array [0..63] of WCHAR;
    AttributeNumber: ULONG;
    Unknown: array [0..1] of ULONG;
    Flags: ULONG;
    MinimumSize: ULONGLONG;
    MaximumSize: ULONGLONG;
  end;
  ATTRIBUTE_DEFINITION = _ATTRIBUTE_DEFINITION;
  PATTRIBUTE_DEFINITION = ^ATTRIBUTE_DEFINITION;

  _BOOT_BLOCK = record
    Jump: array [0..2] of UCHAR;
    Format: array [0..7] of UCHAR;
    BytesPerSector: USHORT;
    SectorsPerCluster: UCHAR;
    BootSectors: USHORT;
    Mbz1: UCHAR;
    Mbz2: USHORT;
    Reserved1: USHORT;
    MediaType: UCHAR;
    Mbz3: USHORT;
    SectorsPerTrack: USHORT;
    NumberOfHeads: USHORT;
    PartitionOffset: ULONG;
    Reserved2: array [0..1] of ULONG;
    TotalSectors: ULONGLONG;
    MftStartLcn: ULONGLONG;
    Mft2StartLcn: ULONGLONG;
    ClustersPerFileRecord: ULONG;
    ClustersPerIndexBlock: ULONG;
    VolumeSerialNumber: ULONGLONG;
    Code: array [0..$1AD] of UCHAR;
    BootSignature: USHORT;
  end;
  BOOT_BLOCK = _BOOT_BLOCK;
  PBOOT_BLOCK = ^BOOT_BLOCK;

//==============================================================================
// Loader API
//==============================================================================

function LdrDisableThreadCalloutsForDll(hModule: HANDLE): NTSTATUS; stdcall;
function LdrGetDllHandle(pwPath: PWORD; pReserved: PVOID; pusPath: PUNICODE_STRING; phModule: PHANDLE): NTSTATUS; stdcall;
function LdrGetProcedureAddress(hModule: HANDLE; dOrdinal: DWORD; psName: PSTRING; ppProcedure: PPVOID): NTSTATUS; stdcall;
function LdrLoadDll(pwPath: PWORD; pdFlags: PDWORD; pusPath: PUNICODE_STRING; phModule: PHANDLE): NTSTATUS; stdcall;
function LdrQueryProcessModuleInformation(psmi: PSYSTEM_MODULE_INFORMATION; dSize: DWORD; pdSize: PDWORD): NTSTATUS; stdcall;
function LdrQueryImageFileExecutionOptions (pusImagePath: PUNICODE_STRING; pwOptionName: PWORD; dRequestedType: DWORD; pData: PVOID; dSize: DWORD; pdSize: PDWORD): NTSTATUS; stdcall;
function LdrUnloadDll(hModule: HANDLE): NTSTATUS; stdcall;

//LdrAccessResource
//LdrAlternateResourcesEnabled
//LdrEnumResources
//LdrFindEntryForAddress
//LdrFindResourceDirectory_U
//LdrFindResource_U
//LdrFlushAlternateResourceModules
//LdrInitializeThunk
//LdrLoadAlternateResourceModule
//LdrProcessRelocationBlock
//LdrShutdownProcess
//LdrShutdownThread
//LdrUnloadAlternateResourceModule
//LdrVerifyImageMatchesChecksum

//==============================================================================
// CSR
//==============================================================================

//CsrAllocateCaptureBuffer
//CsrAllocateMessagePointer
//CsrCaptureMessageBuffer
//CsrCaptureMessageString
//CsrCaptureTimeout
//CsrClientCallServer
//CsrClientConnectToServer
//CsrFreeCaptureBuffer
//CsrIdentifyAlertableThread
//CsrNewThread
//CsrProbeForRead//CsrProbeForWrite
//CsrSetPriorityClass

//==============================================================================
// Debug
//==============================================================================

//DbgPrompt
//DbgSsHandleKmApiMsg
//DbgSsInitialize
//DbgUiConnectToDbg
//DbgUiContinue
//DbgUiWaitStateChange
//DbgUserBreakPoint

//
// Define kernel debugger print prototypes and macros.
//
// N.B. The following function cannot be directly imported because there are
//      a few places in the source tree where this function is redefined.
//

//procedure DbgBreakPoint;
//procedure DbgBreakPointWithStatus(Status: ULONG); stdcall;

const
  DBG_STATUS_CONTROL_C       = 1;
  DBG_STATUS_SYSRQ           = 2;
  DBG_STATUS_BUGCHECK_FIRST  = 3;
  DBG_STATUS_BUGCHECK_SECOND = 4;
  DBG_STATUS_FATAL           = 5;
  DBG_STATUS_DEBUG_CONTROL   = 6;

//function DbgPrint(Format: PCH; ...): ULONG; cdecl;
//function DbgPrintReturnControlC(Format: PCH; ...): ULONG; cdecl;

//==============================================================================
// Runtime Library
//==============================================================================

const
  RTL_RANGE_LIST_ADD_IF_CONFLICT     = $00000001;
  RTL_RANGE_LIST_ADD_SHARED          = $00000002;

const
  RTL_RANGE_LIST_SHARED_OK          = $00000001;
  RTL_RANGE_LIST_NULL_CONFLICT_OK   = $00000002;

type
  PRTL_CONFLICT_RANGE_CALLBACK = function (Context: PVOID; Range: PRTL_RANGE): ByteBool; stdcall;

type
  _OSVERSIONINFOW = record
    dwOSVersionInfoSize: ULONG;
    dwMajorVersion: ULONG;
    dwMinorVersion: ULONG;
    dwBuildNumber: ULONG;
    dwPlatformId: ULONG;
    szCSDVersion: array [0..127] of WCHAR;     // Maintenance string for PSS usage
  end;
  OSVERSIONINFOW = _OSVERSIONINFOW;
  POSVERSIONINFOW = ^OSVERSIONINFOW;
  LPOSVERSIONINFOW = ^OSVERSIONINFOW;
  RTL_OSVERSIONINFOW = OSVERSIONINFOW;
  PRTL_OSVERSIONINFOW = ^OSVERSIONINFOW;

  OSVERSIONINFO = OSVERSIONINFOW;
  POSVERSIONINFO = POSVERSIONINFOW;
  LPOSVERSIONINFO = LPOSVERSIONINFOW;

const
  VER_PLATFORM_WIN32s            = 0;
  VER_PLATFORM_WIN32_WINDOWS     = 1;
  VER_PLATFORM_WIN32_NT          = 2;

type
  _RTL_BITMAP = record
    SizeOfBitMap: ULONG;                     // Number of bits in bit map
    Buffer: PULONG;                          // Pointer to the bit map itself
  end;
  RTL_BITMAP = _RTL_BITMAP;
  PRTL_BITMAP = ^RTL_BITMAP;

const
  RTL_REGISTRY_ABSOLUTE    = 0;   // Path is a full path
  RTL_REGISTRY_SERVICES    = 1;   // \Registry\Machine\System\CurrentControlSet\Services
  RTL_REGISTRY_CONTROL     = 2;   // \Registry\Machine\System\CurrentControlSet\Control
  RTL_REGISTRY_WINDOWS_NT  = 3;   // \Registry\Machine\Software\Microsoft\Windows NT\CurrentVersion
  RTL_REGISTRY_DEVICEMAP   = 4;   // \Registry\Machine\Hardware\DeviceMap
  RTL_REGISTRY_USER        = 5;   // \Registry\User\CurrentUser
  RTL_REGISTRY_MAXIMUM     = 6;
  RTL_REGISTRY_HANDLE      = $40000000;    // Low order bits are registry handle
  RTL_REGISTRY_OPTIONAL    = $80000000;    // Indicates the key node is optional

type
  _TIME_FIELDS = record
    Year: CSHORT;         // range [1601...]
    Month: CSHORT;        // range [1..12]
    Day: CSHORT;          // range [1..31]
    Hour: CSHORT;         // range [0..23]
    Minute: CSHORT;       // range [0..59]
    Second: CSHORT;       // range [0..59]
    Milliseconds: CSHORT; // range [0..999]
    Weekday: CSHORT;      // range [0..6] == [Sunday..Saturday]
  end;
  TIME_FIELDS = _TIME_FIELDS;
  PTIME_FIELDS = ^TIME_FIELDS;

type
  _OSVERSIONINFOEXW  =record
    dwOSVersionInfoSize: ULONG;
    dwMajorVersion: ULONG;
    dwMinorVersion: ULONG;
    dwBuildNumber: ULONG;
    dwPlatformId: ULONG;
    szCSDVersion: array [0..127] of WCHAR;     // Maintenance string for PSS usage
    wServicePackMajor: USHORT;
    wServicePackMinor: USHORT;
    wSuiteMask: USHORT;
    wProductType: UCHAR;
    wReserved: UCHAR;
  end;
  OSVERSIONINFOEXW = _OSVERSIONINFOEXW;
  POSVERSIONINFOEXW = ^OSVERSIONINFOEXW;
  LPOSVERSIONINFOEXW = ^OSVERSIONINFOEXW;
  RTL_OSVERSIONINFOEXW = OSVERSIONINFOEXW;
  PRTL_OSVERSIONINFOEXW = ^OSVERSIONINFOEXW;

  OSVERSIONINFOEX = OSVERSIONINFOEXW;
  POSVERSIONINFOEX = POSVERSIONINFOEXW;
  LPOSVERSIONINFOEX = LPOSVERSIONINFOEXW;

//
// RtlVerifyVersionInfo() conditions
//

const
  VER_EQUAL                       = 1;
  VER_GREATER                     = 2;
  VER_GREATER_EQUAL               = 3;
  VER_LESS                        = 4;
  VER_LESS_EQUAL                  = 5;
  VER_AND                         = 6;
  VER_OR                          = 7;

  VER_CONDITION_MASK              = 7;
  VER_NUM_BITS_PER_CONDITION_MASK = 3;

//
// RtlVerifyVersionInfo() type mask bits
//

  VER_MINORVERSION                = $0000001;
  VER_MAJORVERSION                = $0000002;
  VER_BUILDNUMBER                 = $0000004;
  VER_PLATFORMID                  = $0000008;
  VER_SERVICEPACKMINOR            = $0000010;
  VER_SERVICEPACKMAJOR            = $0000020;
  VER_SUITENAME                   = $0000040;
  VER_PRODUCT_TYPE                = $0000080;

//
// RtlVerifyVersionInfo() os product type values
//

  VER_NT_WORKSTATION              = $0000001;
  VER_NT_DOMAIN_CONTROLLER        = $0000002;
  VER_NT_SERVER                   = $0000003;

type
  PRTL_QUERY_REGISTRY_ROUTINE = function (ValueName: PWSTR; ValueType: ULONG;
    ValueData: PVOID; ValueLength: ULONG; Context, EntryContext: PVOID): NTSTATUS; stdcall;

  _RTL_QUERY_REGISTRY_TABLE = record
    QueryRoutine: PRTL_QUERY_REGISTRY_ROUTINE;
    Flags: ULONG;
    Name: PWSTR;
    EntryContext: PVOID;
    DefaultType: ULONG;
    DefaultData: PVOID;
    DefaultLength: ULONG;
  end;
  RTL_QUERY_REGISTRY_TABLE = _RTL_QUERY_REGISTRY_TABLE;
  PRTL_QUERY_REGISTRY_TABLE = ^RTL_QUERY_REGISTRY_TABLE;

  REFGUID = ^GUID;

function RtlAddRange(RangeList: PRTL_RANGE_LIST; Start, End_: ULONGLONG; Attributes: UCHAR; Flags: ULONG; UserData, Owner: PVOID): NTSTATUS; stdcall;
function RtlAnsiStringToUnicodeString(DestinationString: PUNICODE_STRING; SourceString: PANSI_STRING; AllocateDestinationString: ByteBool): NTSTATUS; stdcall;
function RtlAppendUnicodeStringToString(Destination, Source: PUNICODE_STRING): NTSTATUS; stdcall;
function RtlAppendUnicodeToString(Destination: PUNICODE_STRING; Source: LPCWSTR): NTSTATUS; stdcall;
function RtlAreBitsClear(BitMapHeader: PRTL_BITMAP; StartingIndex, Length: ULONG): ByteBool; stdcall;
function RtlAreBitsSet(BitMapHeader: PRTL_BITMAP; StartingIndex, Length: ULONG): ByteBool; stdcall;
procedure RtlAssert(FailedAssertion, FileName: PVOID; LineNumber: ULONG; Message: PCHAR); stdcall;
function RtlCharToInteger(Str: PCSZ; Base: ULONG; Value: PULONG): NTSTATUS; stdcall;
function RtlCheckRegistryKey(RelativeTo: ULONG; Path: PWSTR): NTSTATUS; stdcall;
procedure RtlClearAllBits(BitMapHeader: PRTL_BITMAP); stdcall;
procedure RtlClearBits(BitMapHeader: PRTL_BITMAP; StartingIndex, NumberToClear: ULONG); stdcall;
function RtlCompareMemory(Source1, Source2: LPVOID; Length: SIZE_T): SIZE_T; stdcall;
function RtlCompareString(String1, String2: PSTRING; CaseInSensitive: ByteBool): LONG; stdcall;
function RtlCompareUnicodeString(String1, String2: PUNICODE_STRING; CaseInSensitive: ByteBool): LONG; stdcall;
function RtlConvertLongToLargeInteger(SignedInteger: LONG): LARGE_INTEGER; stdcall
function RtlConvertUlongToLargeInteger(UnsignedInteger: ULONG): LARGE_INTEGER; stdcall;
function RtlCopyRangeList(CopyRangeList: PRTL_RANGE_LIST; RangeList: PRTL_RANGE_LIST): NTSTATUS; stdcall;
procedure RtlCopyString(DestinationString, SourceString: PSTRING); stdcall;
procedure RtlCopyUnicodeString(DestinationString, SourceString: PUNICODE_STRING); stdcall;
function RtlCreateRegistryKey(RelativeTo: ULONG; Path: PWSTR): NTSTATUS; stdcall;
function RtlCreateSecurityDescriptor(SecurityDescriptor: PSECURITY_DESCRIPTOR; Revision: ULONG): NTSTATUS; stdcall;
function RtlDeleteOwnersRanges(RangeList: PRTL_RANGE_LIST; Owner: PVOID): NTSTATUS; stdcall;
function RtlDeleteRange(RangeList: PRTL_RANGE_LIST; Start, End_: ULONGLONG; Owner: PVOID): NTSTATUS; stdcall;
function RtlDeleteRegistryValue(RelativeTo: ULONG; Path, ValueName: LPCWSTR): NTSTATUS; stdcall;
function RtlEqualString(String1, String2: PSTRING; CaseInSensitive: ByteBool): ByteBool; stdcall;
function RtlEqualUnicodeString(String1, String2: PUNICODE_STRING; CaseInSensitive: ByteBool): ByteBool; stdcall;
function RtlExtendedIntegerMultiply(Multiplicand: LARGE_INTEGER; Multiplier: LONG): LARGE_INTEGER; stdcall
function RtlExtendedLargeIntegerDivide(Dividend: LARGE_INTEGER; Divisor: ULONG; Remainder: PULONG): LARGE_INTEGER; stdcall;
function RtlExtendedMagicDivide(Dividend, MagicDivisor: LARGE_INTEGER; ShiftCount: CCHAR): LARGE_INTEGER; stdcall;
procedure RtlFillMemory(Destination: LPVOID; Length: SIZE_T; Fill: UCHAR); stdcall;
function RtlFindClearBits(BitMapHeader: PRTL_BITMAP; NumberToFind, HintIndex: ULONG): ULONG; stdcall;
function RtlFindClearBitsAndSet(BitMapHeader: PRTL_BITMAP; NumberToFind, HintIndex: ULONG): ULONG; stdcall;
function RtlFindLastBackwardRunClear(BitMapHeader: PRTL_BITMAP; FromIndex: ULONG; StartingRunIndex: PULONG): ULONG; stdcall;
function RtlFindLeastSignificantBit(Set_: ULONGLONG): CCHAR; stdcall
function RtlFindLongestRunClear(BitMapHeader: PRTL_BITMAP; StartingIndex: PULONG): ULONG; stdcall;
function RtlFindMostSignificantBit(Set_: ULONGLONG): CCHAR; stdcall;
function RtlFindNextForwardRunClear(BitMapHeader: PRTL_BITMAP; FromIndex: ULONG; StartingRunIndex: PULONG): ULONG; stdcall;
function RtlFindRange(RangeList: PRTL_RANGE_LIST; Minimum, Maximum: ULONGLONG; Length, Alignment, Flags: ULONG; AttributeAvailableMask: UCHAR; Context: PVOID; Callback: PRTL_CONFLICT_RANGE_CALLBACK; Start: PULONGLONG): NTSTATUS; stdcall;
function RtlFindSetBits(BitMapHeader: PRTL_BITMAP; NumberToFind, HintIndex: ULONG): ULONG; stdcall;
function RtlFindSetBitsAndClear(BitMapHeader: PRTL_BITMAP; NumberToFind, HintIndex: ULONG): ULONG; stdcall;
procedure RtlFreeAnsiString(AnsiString: PANSI_STRING); stdcall;
procedure RtlFreeRangeList(RangeList: PRTL_RANGE_LIST); stdcall;
procedure RtlFreeUnicodeString(UnicodeString: PUNICODE_STRING); stdcall;
function RtlGUIDFromString(GuidString: PUNICODE_STRING; Guid: LPGUID): NTSTATUS; stdcall;
procedure RtlGetCallersAddress(CallersAddress, CallersCaller: PPVOID); stdcall;
function RtlGetFirstRange(RangeList: PRTL_RANGE_LIST; Iterator: PRTL_RANGE_LIST_ITERATOR; var Range: PRTL_RANGE): NTSTATUS; stdcall;
function RtlGetNextRange(Iterator: PRTL_RANGE_LIST_ITERATOR; var Range: PRTL_RANGE; MoveForwards: ByteBool): NTSTATUS; stdcall;
function RtlGetVersion(lpVersionInformation: PRTL_OSVERSIONINFOW): NTSTATUS; stdcall;
procedure RtlInitAnsiString(DestinationString: PANSI_STRING; SourceString: PCSZ); stdcall;
procedure RtlInitString(DestinationString: PSTRING; SourceString: PCSZ); stdcall;
procedure RtlInitUnicodeString(DestinationString: PUNICODE_STRING; SourceString: LPCWSTR); stdcall;
procedure RtlInitializeBitMap(BitMapHeader: PRTL_BITMAP; BitMapBuffer: PULONG; SizeOfBitMap: ULONG); stdcall;
procedure RtlInitializeRangeList(RangeList: PRTL_RANGE_LIST); stdcall;
function RtlInt64ToUnicodeString(Value: ULONGLONG; Base: ULONG; Str: PUNICODE_STRING): NTSTATUS; stdcall;
function RtlIntegerToUnicodeString(Value, Base: ULONG; Str: PUNICODE_STRING): NTSTATUS; stdcall;
function RtlInvertRangeList(InvertedRangeList: PRTL_RANGE_LIST; RangeList: PRTL_RANGE_LIST): NTSTATUS; stdcall;
function RtlIsRangeAvailable(RangeList: PRTL_RANGE_LIST; Start, End_: ULONGLONG; Flags: ULONG; AttributeAvailableMask: UCHAR; Context: PVOID; Callback: PRTL_CONFLICT_RANGE_CALLBACK; Available: PBOOLEAN): NTSTATUS; stdcall;
function RtlLargeIntegerArithmeticShift(LargeInteger: LARGE_INTEGER; ShiftCount: CCHAR): LARGE_INTEGER; stdcall;
function RtlLargeIntegerDivide(Dividend, Divisor: LARGE_INTEGER; Remainder: PLARGE_INTEGER): LARGE_INTEGER; stdcall;
function RtlLargeIntegerShiftLeft(LargeInteger: LARGE_INTEGER; ShiftCount: CCHAR): LARGE_INTEGER; stdcall;
function RtlLargeIntegerShiftRight(LargeInteger: LARGE_INTEGER; ShiftCount: CCHAR): LARGE_INTEGER; stdcall;
function RtlLengthSecurityDescriptor(SecurityDescriptor: PSECURITY_DESCRIPTOR): ULONG; stdcall;
procedure RtlMapGenericMask(AccessMask: PACCESS_MASK; GenericMapping: PGENERIC_MAPPING); stdcall;
function RtlMergeRangeLists(MergedRangeList: PRTL_RANGE_LIST; RangeList1, RangeList2: PRTL_RANGE_LIST; Flags: ULONG): NTSTATUS; stdcall;
procedure RtlMoveMemory(Destination, Source: LPVOID; Length: SIZE_T); stdcall;
function RtlNumberOfClearBits(BitMapHeader: PRTL_BITMAP): ULONG; stdcall;
function RtlNumberOfSetBits(BitMapHeader: PRTL_BITMAP): ULONG; stdcall;
function RtlPrefixUnicodeString(String1, String2: PUNICODE_STRING; CaseInSensitive: ByteBool): ByteBool; stdcall;
function RtlQueryRegistryValues(RelativeTo: ULONG; Path: LPCWSTR; QueryTable: PRTL_QUERY_REGISTRY_TABLE; Context, Environment: PVOID): NTSTATUS; stdcall;
procedure RtlSetAllBits(BitMapHeader: PRTL_BITMAP); stdcall;
procedure RtlSetBits(BitMapHeader: PRTL_BITMAP; StartingIndex, NumberToSet: ULONG); stdcall;
function RtlSetDaclSecurityDescriptor(SecurityDescriptor: PSECURITY_DESCRIPTOR; DaclPresent: ByteBool; Dacl: PACL; DaclDefaulted: ByteBool): NTSTATUS; stdcall;
function RtlStringFromGUID(Guid: REFGUID; GuidString: PUNICODE_STRING): NTSTATUS; stdcall;
function RtlTimeFieldsToTime(TimeFields: PTIME_FIELDS; Time: PLARGE_INTEGER): ByteBool; stdcall;
procedure RtlTimeToTimeFields(Time: PLARGE_INTEGER; TimeFields: PTIME_FIELDS); stdcall;
function RtlUnicodeStringToAnsiString(DestinationString: PANSI_STRING; SourceString: PUNICODE_STRING; AllocateDestinationString: ByteBool): NTSTATUS; stdcall;
function RtlUnicodeStringToInteger(Str: PUNICODE_STRING; Base: ULONG; Value: PULONG): NTSTATUS; stdcall;
function RtlUpcaseUnicodeChar(SourceCharacter: WCHAR): WCHAR; stdcall;
function RtlUpcaseUnicodeString(DestinationString: PUNICODE_STRING; SourceString: PCUNICODE_STRING; AllocateDestinationString: ByteBool): NTSTATUS; stdcall;
function RtlUpperChar(Character: CHAR): CHAR; stdcall;
procedure RtlUpperString(DestinationString, SourceString: PSTRING); stdcall;
function RtlValidRelativeSecurityDescriptor(SecurityDescriptorInput: PSECURITY_DESCRIPTOR; SecurityDescriptorLength: ULONG; RequiredInformation: SECURITY_INFORMATION): ByteBool; stdcall;
function RtlValidSecurityDescriptor(SecurityDescriptor: PSECURITY_DESCRIPTOR): ByteBool; stdcall;
function RtlVerifyVersionInfo(VersionInfo: PRTL_OSVERSIONINFOEXW; TypeMask: ULONG; ConditionMask: ULONGLONG): NTSTATUS; stdcall;
function RtlWriteRegistryValue(RelativeTo: ULONG; Path: LPCWSTR; ValueName: LPCWSTR; ValueType: ULONG; ValueData: PVOID; ValueLength: ULONG): NTSTATUS; stdcall;
procedure RtlZeroMemory(Destination: LPVOID; Length: SIZE_T); stdcall;
function RtlxAnsiStringToUnicodeSize(AnsiString: PANSI_STRING): ULONG; stdcall;

implementation

const
  ntdll = 'ntdll.dll';

function NtQuerySystemInformation; external ntdll name 'NtQuerySystemInformation';
function NtSetSystemInformation; external ntdll name 'NtSetSystemInformation';
function NtQuerySystemEnvironmentValue; external ntdll name 'NtQuerySystemEnvironmentValue';
function NtSetSystemEnvironmentValue; external ntdll name 'NtSetSystemEnvironmentValue';
function NtShutdownSystem; external ntdll name 'NtShutdownSystem';
function NtSystemDebugControl; external ntdll name 'NtSystemDebugControl';
function NtQueryObject; external ntdll name 'NtQueryObject';
function NtSetInformationObject; external ntdll name 'NtSetInformationObject';
function NtDuplicateObject; external ntdll name 'NtDuplicateObject';
function NtMakeTemporaryObject; external ntdll name 'NtMakeTemporaryObject';
function NtClose; external ntdll name 'NtClose';
function NtQuerySecurityObject; external ntdll name 'NtQuerySecurityObject';
function NtSetSecurityObject; external ntdll name 'NtSetSecurityObject';
function NtCreateDirectoryObject; external ntdll name 'NtCreateDirectoryObject';
function NtOpenDirectoryObject; external ntdll name 'NtOpenDirectoryObject';
function NtQueryDirectoryObject; external ntdll name 'NtQueryDirectoryObject';
function NtCreateSymbolicLinkObject; external ntdll name 'NtCreateSymbolicLinkObject';
function NtOpenSymbolicLinkObject; external ntdll name 'NtOpenSymbolicLinkObject';
function NtQuerySymbolicLinkObject; external ntdll name 'NtQuerySymbolicLinkObject';
function NtAllocateVirtualMemory; external ntdll name 'NtAllocateVirtualMemory';
function NtFreeVirtualMemory; external ntdll name 'NtFreeVirtualMemory';
function NtQueryVirtualMemory; external ntdll name 'NtQueryVirtualMemory';
function NtLockVirtualMemory; external ntdll name 'NtLockVirtualMemory';
function NtUnlockVirtualMemory; external ntdll name 'NtUnlockVirtualMemory';
function NtReadVirtualMemory; external ntdll name 'NtReadVirtualMemory';
function NtWriteVirtualMemory; external ntdll name 'NtWriteVirtualMemory';
function NtProtectVirtualMemory; external ntdll name 'NtProtectVirtualMemory';
function NtFlushVirtualMemory; external ntdll name 'NtFlushVirtualMemory';
function NtAllocateUserPhysicalPages; external ntdll name 'NtAllocateUserPhysicalPages';
function NtFreeUserPhysicalPages; external ntdll name 'NtFreeUserPhysicalPages';
function NtMapUserPhysicalPages; external ntdll name 'NtMapUserPhysicalPages';
function NtMapUserPhysicalPagesScatter; external ntdll name 'NtMapUserPhysicalPagesScatter';
function NtGetWriteWatch; external ntdll name 'NtGetWriteWatch';
function NtResetWriteWatch; external ntdll name 'NtResetWriteWatch';
function NtCreateSection; external ntdll name 'NtCreateSection';
function NtOpenSection; external ntdll name 'NtOpenSection';
function NtQuerySection; external ntdll name 'NtQuerySection';
function NtExtendSection; external ntdll name 'NtExtendSection';
function NtMapViewOfSection; external ntdll name 'NtMapViewOfSection';
function NtUnmapViewOfSection; external ntdll name 'NtUnmapViewOfSection';
function NtAreMappedFilesTheSame; external ntdll name 'NtAreMappedFilesTheSame';
function NtCreateThread; external ntdll name 'NtCreateThread';
function NtOpenThread; external ntdll name 'NtOpenThread';
function NtTerminateThread; external ntdll name 'NtTerminateThread';
function NtQueryInformationThread; external ntdll name 'NtQueryInformationThread';
function NtSetInformationThread; external ntdll name 'NtSetInformationThread';
function NtSuspendThread; external ntdll name 'NtSuspendThread';
function NtResumeThread; external ntdll name 'NtResumeThread';
function NtGetContextThread; external ntdll name 'NtGetContextThread';
function NtSetContextThread; external ntdll name 'NtSetContextThread';
function NtQueueApcThread; external ntdll name 'NtQueueApcThread';
function NtTestAlert: NTSTATUS; external ntdll name 'NtTestAlert';
function NtAlertThread; external ntdll name 'NtAlertThread';
function NtAlertResumeThread; external ntdll name 'NtAlertResumeThread';
function NtRegisterThreadTerminatePort; external ntdll name 'NtRegisterThreadTerminatePort';
function NtImpersonateThread; external ntdll name 'NtImpersonateThread';
function NtImpersonateAnonymousToken; external ntdll name 'NtImpersonateAnonymousToken';
function NtCreateProcess; external ntdll name 'NtCreateProcess';
function NtOpenProcess; external ntdll name 'NtOpenProcess';
function NtTerminateProcess; external ntdll name 'NtTerminateProcess';
function NtQueryInformationProcess; external ntdll name 'NtQueryInformationProcess';
function NtSetInformationProcess; external ntdll name 'NtSetInformationProcess';
function RtlCreateProcessParameters; external ntdll name 'RtlCreateProcessParameters';
function RtlDestroyProcessParameters; external ntdll name 'RtlDestroyProcessParameters';
function RtlCreateQueryDebugBuffer; external ntdll name 'RtlCreateQueryDebugBuffer';
function RtlQueryProcessDebugInformation; external ntdll name 'RtlQueryProcessDebugInformation';
function RtlDestroyQueryDebugBuffer; external ntdll name 'RtlDestroyQueryDebugBuffer';
function NtCreateJobObject; external ntdll name 'NtCreateJobObject';
function NtOpenJobObject; external ntdll name 'NtOpenJobObject';
function NtTerminateJobObject; external ntdll name 'NtTerminateJobObject';
function NtAssignProcessToJobObject; external ntdll name 'NtAssignProcessToJobObject';
function NtQueryInformationJobObject; external ntdll name 'NtQueryInformationJobObject';
function NtSetInformationJobObject; external ntdll name 'NtSetInformationJobObject';
function NtCreateToken; external ntdll name 'NtCreateToken';
function NtOpenProcessToken; external ntdll name 'NtOpenProcessToken';
function NtOpenThreadToken; external ntdll name 'NtOpenThreadToken';
function NtDuplicateToken; external ntdll name 'NtDuplicateToken';
function NtFilterToken; external ntdll name 'NtFilterToken';
function NtAdjustPrivilegesToken; external ntdll name 'NtAdjustPrivilegesToken';
function NtAdjustGroupsToken; external ntdll name 'NtAdjustGroupsToken';
function NtQueryInformationToken; external ntdll name 'NtQueryInformationToken';
function NtSetInformationToken; external ntdll name 'NtSetInformationToken';
function NtWaitForSingleObject; external ntdll name 'NtWaitForSingleObject';
function NtSignalAndWaitForSingleObject; external ntdll name 'NtSignalAndWaitForSingleObject';
function NtWaitForMultipleObjects; external ntdll name 'NtWaitForMultipleObjects';
function NtCreateTimer; external ntdll name 'NtCreateTimer';
function NtOpenTimer; external ntdll name 'NtOpenTimer';
function NtCancelTimer; external ntdll name 'NtCancelTimer';
function NtSetTimer; external ntdll name 'NtSetTimer';
function NtQueryTimer; external ntdll name 'NtQueryTimer';
function NtCreateEvent; external ntdll name 'NtCreateEvent';
function NtOpenEvent; external ntdll name 'NtOpenEvent';
function NtSetEvent; external ntdll name 'NtSetEvent';
function NtPulseEvent; external ntdll name 'NtPulseEvent';
function NtResetEvent; external ntdll name 'NtResetEvent';
function NtClearEvent; external ntdll name 'NtClearEvent';
function NtQueryEvent; external ntdll name 'NtQueryEvent';
function NtCreateSemaphore; external ntdll name 'NtCreateSemaphore';
function NtOpenSemaphore; external ntdll name 'NtOpenSemaphore';
function NtReleaseSemaphore; external ntdll name 'NtReleaseSemaphore';
function NtQuerySemaphore; external ntdll name 'NtQuerySemaphore';
function NtCreateMutant; external ntdll name 'NtCreateMutant';
function NtOpenMutant; external ntdll name 'NtOpenMutant';
function NtReleaseMutant; external ntdll name 'NtReleaseMutant';
function NtQueryMutant; external ntdll name 'NtQueryMutant';
function NtCreateIoCompletion; external ntdll name 'NtCreateIoCompletion';
function NtOpenIoCompletion; external ntdll name 'NtOpenIoCompletion';
function NtSetIoCompletion; external ntdll name 'NtSetIoCompletion';
function NtRemoveIoCompletion; external ntdll name 'NtRemoveIoCompletion';
function NtQueryIoCompletion; external ntdll name 'NtQueryIoCompletion';
function NtCreateEventPair; external ntdll name 'NtCreateEventPair';
function NtOpenEventPair; external ntdll name 'NtOpenEventPair';
function NtWaitLowEventPair; external ntdll name 'NtWaitLowEventPair';
function NtWaitHighEventPair; external ntdll name 'NtWaitHighEventPair';
function NtSetLowWaitHighEventPair; external ntdll name 'NtSetLowWaitHighEventPair';
function NtSetHighWaitLowEventPair; external ntdll name 'NtSetHighWaitLowEventPair';
function NtSetLowEventPair; external ntdll name 'NtSetLowEventPair';
function NtSetHighEventPair; external ntdll name 'NtSetHighEventPair';
function NtQuerySystemTime; external ntdll name 'NtQuerySystemTime';
function NtSetSystemTime; external ntdll name 'NtSetSystemTime';
function NtQueryPerformanceCounter; external ntdll name 'NtQueryPerformanceCounter';
function NtSetTimerResolution; external ntdll name 'NtSetTimerResolution';
function NtQueryTimerResolution; external ntdll name 'NtQueryTimerResolution';
function NtDelayExecution; external ntdll name 'NtDelayExecution';
function NtYieldExecution: NTSTATUS; external ntdll name 'NtYieldExecution';
function NtGetTickCount: ULONG; external ntdll name 'NtGetTickCount';
function NtCreateProfile; external ntdll name 'NtCreateProfile';
function NtSetIntervalProfile; external ntdll name 'NtSetIntervalProfile';
function NtQueryIntervalProfile; external ntdll name 'NtQueryIntervalProfile';
function NtStartProfile; external ntdll name 'NtStartProfile';
function NtStopProfile; external ntdll name 'NtStopProfile';
function NtCreatePort; external ntdll name 'NtCreatePort';
function NtCreateWaitablePort; external ntdll name 'NtCreateWaitablePort';
function NtConnectPort; external ntdll name 'NtConnectPort';
function NtSecureConnectPort; external ntdll name 'NtSecureConnectPort';
function NtListenPort; external ntdll name 'NtListenPort';
function NtAcceptConnectPort; external ntdll name 'NtAcceptConnectPort';
function NtCompleteConnectPort; external ntdll name 'NtCompleteConnectPort';
function NtRequestPort; external ntdll name 'NtRequestPort';
function NtRequestWaitReplyPort; external ntdll name 'NtRequestWaitReplyPort';
function NtReplyPort; external ntdll name 'NtReplyPort';
function NtReplyWaitReplyPort; external ntdll name 'NtReplyWaitReplyPort';
function NtReplyWaitReceivePort; external ntdll name 'NtReplyWaitReceivePort';
function NtReplyWaitReceivePortEx; external ntdll name 'NtReplyWaitReceivePortEx';
function NtReadRequestData; external ntdll name 'NtReadRequestData';
function NtWriteRequestData; external ntdll name 'NtWriteRequestData';
function NtQueryInformationPort; external ntdll name 'NtQueryInformationPort';
function NtImpersonateClientOfPort; external ntdll name 'NtImpersonateClientOfPort';
function NtCreateFile; external ntdll name 'NtCreateFile';
function NtOpenFile; external ntdll name 'NtOpenFile';
function NtDeleteFile; external ntdll name 'NtDeleteFile';
function NtFlushBuffersFile; external ntdll name 'NtFlushBuffersFile';
function NtCancelIoFile; external ntdll name 'NtCancelIoFile';
function NtReadFile; external ntdll name 'NtReadFile';
function NtWriteFile; external ntdll name 'NtWriteFile';
function NtReadFileScatter; external ntdll name 'NtReadFileScatter';
function NtWriteFileGather; external ntdll name 'NtWriteFileGather';
function NtLockFile; external ntdll name 'NtLockFile';
function NtUnlockFile; external ntdll name 'NtUnlockFile';
function NtDeviceIoControlFile; external ntdll name 'NtDeviceIoControlFile';
function NtFsControlFile; external ntdll name 'NtFsControlFile';
function NtNotifyChangeDirectoryFile; external ntdll name 'NtNotifyChangeDirectoryFile';
function NtQueryEaFile; external ntdll name 'NtQueryEaFile';
function NtSetEaFile; external ntdll name 'NtSetEaFile';
function NtCreateNamedPipeFile; external ntdll name 'NtCreateNamedPipeFile';
function NtCreateMailslotFile; external ntdll name 'NtCreateMailslotFile';
function NtQueryVolumeInformationFile; external ntdll name 'NtQueryVolumeInformationFile';
function NtSetVolumeInformationFile; external ntdll name 'NtSetVolumeInformationFile';
function NtQueryQuotaInformationFile; external ntdll name 'NtQueryQuotaInformationFile';
function NtSetQuotaInformationFile; external ntdll name 'NtSetQuotaInformationFile';
function NtQueryAttributesFile; external ntdll name 'NtQueryAttributesFile';
function NtQueryFullAttributesFile; external ntdll name 'NtQueryFullAttributesFile';
function NtQueryInformationFile; external ntdll name 'NtQueryInformationFile';
function NtSetInformationFile; external ntdll name 'NtSetInformationFile';
function NtQueryDirectoryFile; external ntdll name 'NtQueryDirectoryFile';
function NtCreateKey; external ntdll name 'NtCreateKey';
function NtOpenKey; external ntdll name 'NtOpenKey';
function NtDeleteKey; external ntdll name 'NtDeleteKey';
function NtFlushKey; external ntdll name 'NtFlushKey';
function NtSaveKey; external ntdll name 'NtSaveKey';
function NtSaveMergedKeys; external ntdll name 'NtSaveMergedKeys';
function NtRestoreKey; external ntdll name 'NtRestoreKey';
function NtLoadKey; external ntdll name 'NtLoadKey';
function NtLoadKey2; external ntdll name 'NtLoadKey2';
function NtUnloadKey; external ntdll name 'NtUnloadKey';
function NtReplaceKey; external ntdll name 'NtReplaceKey';
function NtSetInformationKey; external ntdll name 'NtSetInformationKey';
function NtQueryKey; external ntdll name 'NtQueryKey';
function NtEnumerateKey; external ntdll name 'NtEnumerateKey';
function NtNotifyChangeKey; external ntdll name 'NtNotifyChangeKey';
function NtNotifyChangeMultipleKeys; external ntdll name 'NtNotifyChangeMultipleKeys';
function NtDeleteValueKey; external ntdll name 'NtDeleteValueKey';
function NtSetValueKey; external ntdll name 'NtSetValueKey';
function NtQueryValueKey; external ntdll name 'NtQueryValueKey';
function NtEnumerateValueKey; external ntdll name 'NtEnumerateValueKey';
function NtQueryMultipleValueKey; external ntdll name 'NtQueryMultipleValueKey';
function NtPrivilegeCheck; external ntdll name 'NtPrivilegeCheck';
function NtPrivilegeObjectAuditAlarm; external ntdll name 'NtPrivilegeObjectAuditAlarm';
function NtPrivilegedServiceAuditAlarm; external ntdll name 'NtPrivilegedServiceAuditAlarm';
function NtAccessCheck; external ntdll name 'NtAccessCheck';
function NtAccessCheckAndAuditAlarm; external ntdll name 'NtAccessCheckAndAuditAlarm';
function NtAccessCheckByType; external ntdll name 'NtAccessCheckByType';
function NtAccessCheckByTypeAndAuditAlarm; external ntdll name 'NtAccessCheckByTypeAndAuditAlarm';
function NtAccessCheckByTypeResultList; external ntdll name 'NtAccessCheckByTypeResultList';
function NtAccessCheckByTypeResultListAndAuditAlarm; external ntdll name 'NtAccessCheckByTypeResultListAndAuditAlarm';
function NtAccessCheckByTypeResultListAndAuditAlarmByHandle; external ntdll name 'NtAccessCheckByTypeResultListAndAuditAlarmByHandle';
function NtOpenObjectAuditAlarm; external ntdll name 'NtOpenObjectAuditAlarm';
function NtCloseObjectAuditAlarm; external ntdll name 'NtCloseObjectAuditAlarm';
function NtDeleteObjectAuditAlarm; external ntdll name 'NtDeleteObjectAuditAlarm';
function NtRequestWakeupLatency; external ntdll name 'NtRequestWakeupLatency';
function NtRequestDeviceWakeup; external ntdll name 'NtRequestDeviceWakeup';
function NtCancelDeviceWakeupRequest; external ntdll name 'NtCancelDeviceWakeupRequest';
function NtIsSystemResumeAutomatic; external ntdll name 'NtIsSystemResumeAutomatic';
function NtSetThreadExecutionState; external ntdll name 'NtSetThreadExecutionState';
function NtGetDevicePowerState; external ntdll name 'NtGetDevicePowerState';
function NtSetSystemPowerState; external ntdll name 'NtSetSystemPowerState';
function NtInitiatePowerAction; external ntdll name 'NtInitiatePowerAction';
function NtPowerInformation; external ntdll name 'NtPowerInformation';
function NtGetPlugPlayEvent; external ntdll name 'NtGetPlugPlayEvent';
function NtRaiseException; external ntdll name 'NtRaiseException';
function NtContinue; external ntdll name 'NtContinue';
//function NtW32Call; external ntdll name 'UNKNOWN';
function NtCallbackReturn; external ntdll name 'NtCallbackReturn';
//function NtSetLowWaitHighThread: NTSTATUS; external ntdll name 'UNKNOWN';
//function NtSetHighWaitLowThread: NTSTATUS; external ntdll name 'UNKNOWN';
function NtLoadDriver; external ntdll name 'NtLoadDriver';
function NtUnloadDriver; external ntdll name 'NtUnloadDriver';
function NtFlushInstructionCache; external ntdll name 'NtFlushInstructionCache';
function NtFlushWriteBuffer: NTSTATUS; external ntdll name 'NtFlushWriteBuffer';
function NtQueryDefaultLocale; external ntdll name 'NtQueryDefaultLocale';
function NtSetDefaultLocale; external ntdll name 'NtSetDefaultLocale';
function NtQueryDefaultUILanguage; external ntdll name 'NtQueryDefaultUILanguage';
function NtSetDefaultUILanguage; external ntdll name 'NtSetDefaultUILanguage';
function NtQueryInstallUILanguage; external ntdll name 'NtQueryInstallUILanguage';
function NtAllocateLocallyUniqueId; external ntdll name 'NtAllocateLocallyUniqueId';
function NtAllocateUuids; external ntdll name 'NtAllocateUuids';
function NtSetUuidSeed; external ntdll name 'NtSetUuidSeed';
function NtRaiseHardError; external ntdll name 'NtRaiseHardError';
function NtSetDefaultHardErrorPort; external ntdll name 'NtSetDefaultHardErrorPort';
function NtDisplayString; external ntdll name 'NtDisplayString';
function NtCreatePagingFile; external ntdll name 'NtCreatePagingFile';
function NtAddAtom; external ntdll name 'NtAddAtom';
function NtFindAtom; external ntdll name 'NtFindAtom';
function NtDeleteAtom; external ntdll name 'NtDeleteAtom';
function NtQueryInformationAtom; external ntdll name 'NtQueryInformationAtom';
function NtSetLdtEntries; external ntdll name 'NtSetLdtEntries';
function NtVdmControl; external ntdll name 'NtdVdmControl';

function LdrDisableThreadCalloutsForDll; external ntdll name 'LdrDisableThreadCalloutsForDll';
function LdrGetDllHandle; external ntdll name 'LdrGetDllHandle';
function LdrGetProcedureAddress; external ntdll name 'LdrGetProcedureAddress';
function LdrLoadDll; external ntdll name 'LdrLoadDll';
function LdrQueryProcessModuleInformation; external ntdll name 'LdrQueryProcessModuleInformation';
function LdrQueryImageFileExecutionOptions; external ntdll name 'LdrQueryImageFileExecutionOptions';
function LdrUnloadDll; external ntdll name 'LdrUnloadDll';

function RtlAddRange; external ntdll name 'RtlAddRange';
function RtlAnsiStringToUnicodeString; external ntdll name 'RtlAnsiStringToUnicodeString';
function RtlAppendUnicodeStringToString; external ntdll name 'RtlAppendUnicodeStringToString';
function RtlAppendUnicodeToString; external ntdll name 'RtlAppendUnicodeToString';
function RtlAreBitsClear; external ntdll name 'RtlAreBitsClear';
function RtlAreBitsSet; external ntdll name 'RtlAreBitsSet';
procedure RtlAssert; external ntdll name 'RtlAssert';
function RtlCharToInteger; external ntdll name 'RtlCharToInteger';
function RtlCheckRegistryKey; external ntdll name 'RtlCheckRegistryKey';
procedure RtlClearAllBits; external ntdll name 'RtlClearAllBits';
procedure RtlClearBits; external ntdll name 'RtlClearBits';
function RtlCompareMemory; external ntdll name 'RtlCompareMemory';
function RtlCompareString; external ntdll name 'RtlCompareString';
function RtlCompareUnicodeString; external ntdll name 'RtlCompareUnicodeString';
function RtlConvertLongToLargeInteger; external ntdll name 'RtlConvertLongToLargeInteger';
function RtlConvertUlongToLargeInteger; external ntdll name 'RtlConvertUlongToLargeInteger';
function RtlCopyRangeList; external ntdll name 'RtlCopyRangeList';
procedure RtlCopyString; external ntdll name 'RtlCopyString';
procedure RtlCopyUnicodeString; external ntdll name 'RtlCopyUnicodeString';
function RtlCreateRegistryKey; external ntdll name 'RtlCreateRegistryKey';
function RtlCreateSecurityDescriptor; external ntdll name 'RtlCreateSecurityDescriptor';
function RtlDeleteOwnersRanges; external ntdll name 'RtlDeleteOwnersRanges';
function RtlDeleteRange; external ntdll name 'RtlDeleteRange';
function RtlDeleteRegistryValue; external ntdll name 'RtlDeleteRegistryValue';
function RtlEqualString; external ntdll name 'RtlEqualString';
function RtlEqualUnicodeString; external ntdll name 'RtlEqualUnicodeString';
function RtlExtendedIntegerMultiply; external ntdll name 'RtlExtendedIntegerMultiply';
function RtlExtendedLargeIntegerDivide; external ntdll name 'RtlExtendedLargeIntegerDivide';
function RtlExtendedMagicDivide; external ntdll name 'RtlExtendedMagicDivide';
procedure RtlFillMemory; external ntdll name 'RtlFillMemory';
function RtlFindClearBits; external ntdll name 'RtlFindClearBits';
function RtlFindClearBitsAndSet; external ntdll name 'RtlFindClearBitsAndSet';
function RtlFindLastBackwardRunClear; external ntdll name 'RtlFindLastBackwardRunClear';
function RtlFindLeastSignificantBit; external ntdll name 'RtlFindLeastSignificantBit';
function RtlFindLongestRunClear; external ntdll name 'RtlFindLongestRunClear';
function RtlFindMostSignificantBit; external ntdll name 'RtlFindMostSignificantBit';
function RtlFindNextForwardRunClear; external ntdll name 'RtlFindNextForwardRunClear';
function RtlFindRange; external ntdll name 'RtlFindRange';
function RtlFindSetBits; external ntdll name 'RtlFindSetBits';
function RtlFindSetBitsAndClear; external ntdll name 'RtlFindSetBitsAndClear';
procedure RtlFreeAnsiString; external ntdll name 'RtlFreeAnsiString';
procedure RtlFreeRangeList; external ntdll name 'RtlFreeRangeList';
procedure RtlFreeUnicodeString; external ntdll name 'RtlFreeUnicodeString';
function RtlGUIDFromString; external ntdll name 'RtlGUIDFromString';
procedure RtlGetCallersAddress; external ntdll name 'RtlGetCallersAddress';
function RtlGetFirstRange; external ntdll name 'RtlGetFirstRange';
function RtlGetNextRange; external ntdll name 'RtlGetNextRange';
function RtlGetVersion; external ntdll name 'RtlGetVersion';
procedure RtlInitAnsiString; external ntdll name 'RtlInitAnsiString';
procedure RtlInitString; external ntdll name 'RtlInitString';
procedure RtlInitUnicodeString; external ntdll name 'RtlInitUnicodeString';
procedure RtlInitializeBitMap; external ntdll name 'RtlInitializeBitMap';
procedure RtlInitializeRangeList; external ntdll name 'RtlInitializeRangeList';
function RtlInt64ToUnicodeString; external ntdll name 'RtlInt64ToUnicodeString';
function RtlIntegerToUnicodeString; external ntdll name 'RtlIntegerToUnicodeString';
function RtlInvertRangeList; external ntdll name 'RtlInvertRangeList';
function RtlIsRangeAvailable; external ntdll name 'RtlIsRangeAvailable';
function RtlLargeIntegerArithmeticShift; external ntdll name 'RtlLargeIntegerArithmeticShift';
function RtlLargeIntegerDivide; external ntdll name 'RtlLargeIntegerDivide';
function RtlLargeIntegerShiftLeft; external ntdll name 'RtlLargeIntegerShiftLeft';
function RtlLargeIntegerShiftRight; external ntdll name 'RtlLargeIntegerShiftRight';
function RtlLengthSecurityDescriptor; external ntdll name 'RtlLengthSecurityDescriptor';
procedure RtlMapGenericMask; external ntdll name 'RtlMapGenericMask';
function RtlMergeRangeLists; external ntdll name 'RtlMergeRangeLists';
procedure RtlMoveMemory; external ntdll name 'RtlMoveMemory';
function RtlNumberOfClearBits; external ntdll name 'RtlNumberOfClearBits';
function RtlNumberOfSetBits; external ntdll name 'RtlNumberOfSetBits';
function RtlPrefixUnicodeString; external ntdll name 'RtlPrefixUnicodeString';
function RtlQueryRegistryValues; external ntdll name 'RtlQueryRegistryValues';
procedure RtlSetAllBits; external ntdll name 'RtlSetAllBits';
procedure RtlSetBits; external ntdll name 'RtlSetBits';
function RtlSetDaclSecurityDescriptor; external ntdll name 'RtlSetDaclSecurityDescriptor';
function RtlStringFromGUID; external ntdll name 'RtlStringFromGUID';
function RtlTimeFieldsToTime; external ntdll name 'RtlTimeFieldsToTime';
procedure RtlTimeToTimeFields; external ntdll name 'RtlTimeToTimeFields';
function RtlUnicodeStringToAnsiString; external ntdll name 'RtlUnicodeStringToAnsiString';
function RtlUnicodeStringToInteger; external ntdll name 'RtlUnicodeStringToInteger';
function RtlUpcaseUnicodeChar; external ntdll name 'RtlUpcaseUnicodeChar';
function RtlUpcaseUnicodeString; external ntdll name 'RtlUpcaseUnicodeString';
function RtlUpperChar; external ntdll name 'RtlUpperChar';
procedure RtlUpperString; external ntdll name 'RtlUpperString';
function RtlValidRelativeSecurityDescriptor; external ntdll name 'RtlValidRelativeSecurityDescriptor';
function RtlValidSecurityDescriptor; external ntdll name 'RtlValidSecurityDescriptor';
function RtlVerifyVersionInfo; external ntdll name 'RtlVerifyVersionInfo';
function RtlWriteRegistryValue; external ntdll name 'RtlWriteRegistryValue';
procedure RtlZeroMemory; external ntdll name 'RtlZeroMemory';
function RtlxAnsiStringToUnicodeSize; external ntdll name 'RtlxAnsiStringToUnicodeSize';

{ some 300 other RTL functions exported from ntdll but for which i don't have
  a prototype yet. also interesting is ntoskrnl.exe
RtlAbortRXact
RtlAbsoluteToSelfRelativeSD
RtlAcquirePebLock
RtlAcquireResourceExclusive
RtlAcquireResourceShared
RtlAddAccessAllowedAce
RtlAddAccessAllowedAceEx
RtlAddAccessAllowedObjectAce
RtlAddAccessDeniedAce
RtlAddAccessDeniedAceEx
RtlAddAccessDeniedObjectAce
RtlAddAce
RtlAddActionToRXact
RtlAddAtomToAtomTable
RtlAddAttributeActionToRXact
RtlAddAuditAccessAce
RtlAddAuditAccessAceEx
RtlAddAuditAccessObjectAce
RtlAddCompoundAce
RtlAdjustPrivilege
RtlAllocateAndInitializeSid
RtlAllocateHandle
RtlAllocateHeap
RtlAnsiCharToUnicodeChar
RtlAnsiStringToUnicodeSize
RtlAppendAsciizToString
RtlAppendStringToString
RtlApplyRXact
RtlApplyRXactNoFlush
RtlAreAllAccessesGranted
RtlAreAnyAccessesGranted
RtlCallbackLpcClient
RtlCancelTimer
RtlCaptureStackBackTrace
RtlCheckForOrphanedCriticalSections
RtlCompactHeap
RtlCompareMemoryUlong
RtlCompressBuffer
RtlConsoleMultiByteToUnicodeN
RtlConvertExclusiveToShared
RtlConvertPropertyToVariant
RtlConvertSharedToExclusive
RtlConvertSidToUnicodeString
RtlConvertToAutoInheritSecurityObject
RtlConvertUiListToApiList
RtlConvertVariantToProperty
RtlCopyLuid
RtlCopyLuidAndAttributesArray
RtlCopySecurityDescriptor
RtlCopySid
RtlCopySidAndAttributesArray
RtlCreateAcl
RtlCreateAndSetSD
RtlCreateAtomTable
RtlCreateEnvironment
RtlCreateHeap
RtlCreateLpcServer
RtlCreateProcessParameters
RtlCreateQueryDebugBuffer
RtlCreateTagHeap
RtlCreateTimer
RtlCreateTimerQueue
RtlCreateUnicodeString
RtlCreateUnicodeStringFromAsciiz
RtlCreateUserProcess
RtlCreateUserSecurityObject
RtlCreateUserThread
RtlCustomCPToUnicodeN
RtlCutoverTimeToSystemTime
RtlDeNormalizeProcessParams
RtlDebugPrintTimes
RtlDecompressBuffer
RtlDecompressFragment
RtlDefaultNpAcl
RtlDelete
RtlDeleteAce
RtlDeleteAtomFromAtomTable
RtlDeleteCriticalSection
RtlDeleteElementGenericTable
RtlDeleteNoSplay
RtlDeleteResource
RtlDeleteSecurityObject
RtlDeleteTimer
RtlDeleteTimerQueue
RtlDeleteTimerQueueEx
RtlDeregisterWait
RtlDeregisterWaitEx
RtlDestroyAtomTable
RtlDestroyEnvironment
RtlDestroyHandleTable
RtlDestroyHeap
RtlDestroyProcessParameters
RtlDestroyQueryDebugBuffer
RtlDetermineDosPathNameType_U
RtlDnsHostNameToComputerName
RtlDoesFileExists_U
RtlDosPathNameToNtPathName_U
RtlDosSearchPath_U
RtlDowncaseUnicodeString
RtlDumpResource
RtlEmptyAtomTable
RtlEnableEarlyCriticalSectionEventCreation
RtlEnlargedIntegerMultiply
RtlEnlargedUnsignedDivide
RtlEnlargedUnsignedMultiply
RtlEnterCriticalSection
RtlEnumProcessHeaps
RtlEnumerateGenericTable
RtlEnumerateGenericTableWithoutSplaying
RtlEqualComputerName
RtlEqualDomainName
RtlEqualLuid
RtlEqualPrefixSid
RtlEqualSid
RtlEraseUnicodeString
RtlExpandEnvironmentStrings_U
RtlExtendHeap
RtlFillMemoryUlong
RtlFindMessage
RtlFirstFreeAce
RtlFormatCurrentUserKeyPath
RtlFormatMessage
RtlFreeHandle
RtlFreeHeap
RtlFreeOemString
RtlFreeSid
RtlFreeUserThreadStack
RtlGenerate8dot3Name
RtlGetAce
RtlGetCompressionWorkSpaceSize
RtlGetControlSecurityDescriptor
RtlGetCurrentDirectory_U
RtlGetDaclSecurityDescriptor
RtlGetElementGenericTable
RtlGetFullPathName_U
RtlGetGroupSecurityDescriptor
RtlGetLongestNtPathLength
RtlGetNtGlobalFlags
RtlGetNtProductType
RtlGetOwnerSecurityDescriptor
RtlGetProcessHeaps
RtlGetSaclSecurityDescriptor
RtlGetSecurityDescriptorRMControl
RtlGetUserInfoHeap
RtlIdentifierAuthoritySid
RtlImageDirectoryEntryToData
RtlImageNtHeader
RtlImageRvaToSection
RtlImageRvaToVa
RtlImpersonateLpcClient
RtlImpersonateSelf
RtlInitCodePageTable
RtlInitNlsTables
RtlInitializeAtomPackage
RtlInitializeContext
RtlInitializeCriticalSection
RtlInitializeCriticalSectionAndSpinCount
RtlInitializeGenericTable
RtlInitializeHandleTable
RtlInitializeRXact
RtlInitializeResource
RtlInitializeSid
RtlInsertElementGenericTable
RtlIntegerToChar
RtlIsDosDeviceName_U
RtlIsGenericTableEmpty
RtlIsNameLegalDOS8Dot3
RtlIsTextUnicode
RtlIsValidHandle
RtlIsValidIndexHandle
RtlLargeIntegerAdd
RtlLargeIntegerNegate
RtlLargeIntegerSubtract
RtlLargeIntegerToChar
RtlLeaveCriticalSection
RtlLengthRequiredSid
RtlLengthSid
RtlLocalTimeToSystemTime
RtlLockHeap
RtlLookupAtomInAtomTable
RtlLookupElementGenericTable
RtlMakeSelfRelativeSD
RtlMultiByteToUnicodeN
RtlMultiByteToUnicodeSize
RtlNewInstanceSecurityObject
RtlNewSecurityGrantedAccess
RtlNewSecurityObject
RtlNewSecurityObjectEx
RtlNormalizeProcessParams
RtlNtStatusToDosError
RtlNumberGenericTableElements
RtlOemStringToUnicodeSize
RtlOemStringToUnicodeString
RtlOemToUnicodeN
RtlOpenCurrentUser
RtlPcToFileHeader
RtlPinAtomInAtomTable
RtlPrefixString
RtlProtectHeap
RtlQueryAtomInAtomTable
RtlQueryEnvironmentVariable_U
RtlQueryInformationAcl
RtlQueryProcessBackTraceInformation
RtlQueryProcessDebugInformation
RtlQueryProcessHeapInformation
RtlQueryProcessLockInformation
RtlQuerySecurityObject
RtlQueryTagHeap
RtlQueryTimeZoneInformation
RtlQueueWorkItem
RtlRaiseException
RtlRaiseStatus
RtlRandom
RtlReAllocateHeap
RtlRealPredecessor
RtlRealSuccessor
RtlRegisterWait
RtlReleasePebLock
RtlReleaseResource
RtlRemoteCall
RtlResetRtlTranslations
RtlRunDecodeUnicodeString
RtlRunEncodeUnicodeString
RtlSecondsSince1970ToTime
RtlSecondsSince1980ToTime
RtlSelfRelativeToAbsoluteSD
RtlSelfRelativeToAbsoluteSD2
RtlSetAttributesSecurityDescriptor
RtlSetControlSecurityDescriptor
RtlSetCriticalSectionSpinCount
RtlSetCurrentDirectory_U
RtlSetCurrentEnvironment
RtlSetEnvironmentVariable
RtlSetGroupSecurityDescriptor
RtlSetInformationAcl
RtlSetIoCompletionCallback
RtlSetOwnerSecurityDescriptor
RtlSetSaclSecurityDescriptor
RtlSetSecurityDescriptorRMControl
RtlSetSecurityObject
RtlSetSecurityObjectEx
RtlSetThreadPoolStartFunc
RtlSetTimeZoneInformation
RtlSetTimer
RtlSetUnicodeCallouts
RtlSetUserFlagsHeap
RtlSetUserValueHeap
RtlShutdownLpcServer
RtlSizeHeap
RtlSplay
RtlStartRXact
RtlSubAuthorityCountSid
RtlSubAuthoritySid
RtlSubtreePredecessor
RtlSubtreeSuccessor
RtlSystemTimeToLocalTime
RtlTimeToElapsedTimeFields
RtlTimeToSecondsSince1970
RtlTimeToSecondsSince1980
RtlTryEnterCriticalSection
//ULONG FASTCALL RtlUlongByteSwap(IN ULONG Source);
//ULONGLONG FASTCALL RtlUlonglongByteSwap(IN ULONGLONG Source);
RtlUnicodeStringToAnsiSize
RtlUnicodeStringToCountedOemString
RtlUnicodeStringToOemSize
RtlUnicodeStringToOemString
RtlUnicodeToCustomCPN
RtlUnicodeToMultiByteN
RtlUnicodeToMultiByteSize
RtlUnicodeToOemN
RtlUniform
RtlUnlockHeap
RtlUnwind
RtlUpcaseUnicodeStringToAnsiString
RtlUpcaseUnicodeStringToCountedOemString
RtlUpcaseUnicodeStringToOemString
RtlUpcaseUnicodeToCustomCPN
RtlUpcaseUnicodeToMultiByteN
RtlUpcaseUnicodeToOemN
RtlUpdateTimer
RtlUsageHeap
//USHORT FASTCALL RtlUshortByteSwap(IN USHORT Source);
RtlValidAcl
RtlValidSid
RtlValidateHeap
RtlValidateProcessHeaps
RtlWalkFrameChain
RtlWalkHeap
RtlZeroHeap
RtlpNtCreateKey
RtlpNtEnumerateSubKey
RtlpNtMakeTemporaryKey
RtlpNtOpenKey
RtlpNtQueryValueKey
RtlpNtSetValueKey
RtlpUnWaitCriticalSection
RtlpWaitForCriticalSection
RtlxOemStringToUnicodeSize
RtlxUnicodeStringToAnsiSize
RtlxUnicodeStringToOemSize
}
end.

