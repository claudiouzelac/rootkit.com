#ifndef __NTPROCESSLIST_H
 #define __NTPROCESSLIST_H

//#define __WIN2K

#ifndef _NTDDK_
#ifndef _NTDLL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef long NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

//
// Thread affinity
//
typedef ULONG KAFFINITY;
typedef KAFFINITY *PKAFFINITY;

//
// Thread priority
//
typedef LONG KPRIORITY;


#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)
#define STATUS_ILLEGAL_FUNCTION          ((NTSTATUS)0xC00000AFL)

#ifndef STATUS_NO_MEMORY
 #define STATUS_NO_MEMORY                 ((NTSTATUS)0xC0000017L)    
#endif //STATUS_NO_MEMORY

#ifndef STATUS_MORE_ENTRIES
 #define STATUS_MORE_ENTRIES              ((NTSTATUS)0x00000105L)
#endif //STATUS_MORE_ENTRIES

#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)

//
// These must be converted to LUIDs before use.
//

#define SE_MIN_WELL_KNOWN_PRIVILEGE       (2L)
#define SE_CREATE_TOKEN_PRIVILEGE         (2L)
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE   (3L)
#define SE_LOCK_MEMORY_PRIVILEGE          (4L)
#define SE_INCREASE_QUOTA_PRIVILEGE       (5L)

//
// Unsolicited Input is obsolete and unused.
//

#define SE_UNSOLICITED_INPUT_PRIVILEGE    (6L)

#define SE_MACHINE_ACCOUNT_PRIVILEGE      (6L)
#define SE_TCB_PRIVILEGE                  (7L)
#define SE_SECURITY_PRIVILEGE             (8L)
#define SE_TAKE_OWNERSHIP_PRIVILEGE       (9L)
#define SE_LOAD_DRIVER_PRIVILEGE          (10L)
#define SE_SYSTEM_PROFILE_PRIVILEGE       (11L)
#define SE_SYSTEMTIME_PRIVILEGE           (12L)
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE  (13L)
#define SE_INC_BASE_PRIORITY_PRIVILEGE    (14L)
#define SE_CREATE_PAGEFILE_PRIVILEGE      (15L)
#define SE_CREATE_PERMANENT_PRIVILEGE     (16L)
#define SE_BACKUP_PRIVILEGE               (17L)
#define SE_RESTORE_PRIVILEGE              (18L)
#define SE_SHUTDOWN_PRIVILEGE             (19L)
#define SE_DEBUG_PRIVILEGE                (20L)
#define SE_AUDIT_PRIVILEGE                (21L)
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE   (22L)
#define SE_CHANGE_NOTIFY_PRIVILEGE        (23L)
#define SE_REMOTE_SHUTDOWN_PRIVILEGE      (24L)
#define SE_MAX_WELL_KNOWN_PRIVILEGE       (SE_REMOTE_SHUTDOWN_PRIVILEGE)


//
// Valid values for the Attributes field
//

#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_VALID_ATTRIBUTES    0x000001F2L

// Directory Stuff

#define DIRECTORY_QUERY                 (0x0001)
#define DIRECTORY_TRAVERSE              (0x0002)
#define DIRECTORY_CREATE_OBJECT         (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY   (0x0008)

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

#define SYMBOLIC_LINK_QUERY (0x0001)

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;

typedef UNICODE_STRING *PUNICODE_STRING;

typedef enum _SYSTEM_INFORMATION_CLASS
{
  SystemBasicInformation = 0,
  SystemProcessorInformation,
  SystemTimeZoneInformation,
  SystemTimeInformationInformation,
  SystemUnk4Information, 
  SystemProcessesInformation,
  SystemUnk6Information,
  SystemConfigurationInformation,
  SystemUnk8Information,
  SystemUnk9Information,
  SystemUnk10Information,
  SystemDriversInformation,
  SystemLoadImageInformation = 26,
  SystemUnloadImageInformation = 27,
  SystemLoadAndCallImageInformation = 38
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_TIME_INFORMATION
{
  LARGE_INTEGER liKeBootTime;
  LARGE_INTEGER liKeSystemTime;
  LARGE_INTEGER liExpTimeZoneBias;
  ULONG uCurrentTimeZoneId;
  DWORD dwReserved;
} SYSTEM_TIME_INFORMATION;


typedef struct _NT_FILETIME 
{ // ft 
  DWORD dwLowDateTime; 
  DWORD dwHighDateTime; 
} NT_FILETIME; 

typedef struct _THREAD_INFO
{
  NT_FILETIME   ftCreationTime;
  DWORD         dwUnknown1;
  DWORD         dwStartAddress;
  DWORD         dwOwningPID;
  DWORD         dwThreadID;
  DWORD         dwCurrentPriority;
  DWORD         dwBasePriority;
  DWORD         dwContextSwitches;
  DWORD         dwThreadState;
  DWORD         dwThreadWaitReason;
  DWORD         dwUnknown3;
  DWORD         dwUnknown4;
  DWORD         dwUnknown5;
  DWORD         dwUnknown6;
  DWORD         dwUnknown7;
} THREAD_INFO, *PTHREAD_INFO;

/*typedef struct _THREAD_INFO
{
  NT_FILETIME KernelTime;   // FILETIME ftCreationTime;   //000 + //
  NT_FILETIME UserTime;     // FILETIME                   //008 + //
  NT_FILETIME CreateTime;   // FILETIME                   //010 + //
  DWORD dwThreadID;                                       //014 ? //
  DWORD WaitTime;                                         //018 + //
  DWORD StartAddress;                                     //01C + //
  NT_FILETIME Cid;                                        //020 + //
  DWORD dwThreadState;                                    //024 ? //
  DWORD Priority;                                         //028 + //
  DWORD BasePriority;                                     //02C + //
  DWORD ContextSwitches;                                  //030 + //
  DWORD State;                                            //034 + //
  DWORD WaitReason;                                       //038 + //
  DWORD Unised;                                           //03C ? //
} THREAD_INFO, *PTHREAD_INFO;
*/

typedef struct _PROCESS_INFO
{
  DWORD         dwOffset; // an ofset to the next Process structure // 00h
  DWORD         dwThreadCount;                                      // 04h
  DWORD         dwUnknown1[6];                                       // 08h
  NT_FILETIME   ftCreationTime;                                     // 20h
  NT_FILETIME   ftUserTime;
  NT_FILETIME   ftKernelTime;
  //DWORD         dwUnknown2;                                          // 28h
  //DWORD         dwUnknown3;                                          // 2Ch
  //DWORD         dwUnknown4;                                          // 30h
  //DWORD         dwUnknown5;                                          // 34h
  //WORD          wUnknown6;                                           // 38h
  //WORD          wUnknown6;                                           // 3Ah
  //WCHAR        *pszProcessName;                                     // 3Ch
  UNICODE_STRING ProcessName;                                       // 38h
  DWORD         dwBasePriority;                                     // 40h
  DWORD         dwProcessID;                                        // 44h
  DWORD         dwParentProcessID;                                  // 48h
  DWORD         dwHandleCount;                                      // 4Ch
  DWORD         dwUnkown7;                                          // 50h
  DWORD         dwUnkown8;                                          // 54h
  DWORD         dwVirtualBytesPeak;
  DWORD         dwVirtualBytes;     //dwVirtualSize
  DWORD         dwPageFaultsCountPerSec;
  DWORD         dwWorkingSetPeak;   //PeakWorkingSetSize
  DWORD         dwWorkingSet;       //WorkingSetSize
  DWORD         dwPeekPagedPoolUsage;
  DWORD         dwPagedPool; // kbytes PagedPoolUsage
  DWORD         dwPeekNonPagedPoolUsage;
  DWORD         dwNonPagedPool; // kbytes NonPagedPoolUsage
  DWORD         dwPageFileBytesPeak;
  DWORD         dwPageFileBytes;
  DWORD         dwPrivateBytes;
  NT_FILETIME   ProcessorTime;
  //DWORD         dwUnkown11;
  //DWORD         dwUnkown12;
  DWORD         dwUnknown13;
  DWORD         dwUnknown14;
//  #ifdef __WIN2K                           //  
//  DWORD         dwUnknown15_Win2k[12];     //  exist only win2k
//  #endif //__WIN2K                         //
//  THREAD_INFO   ti[1];                     //  position variable get through GetThreadInfoPtr()
} PROCESS_INFO, *PPROCESS_INFO;              


typedef struct _MODULE_INFO
{
  DWORD         dwUnkown0;                                          // 00h
  DWORD         dwUnkown2;                                          // 04h
  DWORD         dwImageBase;                                        // 08h
  DWORD         dwImageSize;                                        // 0Ch
  DWORD         dwLinkerVersion;    //?????????????????             // 10h
  DWORD         dwUnkown3;                                          // 14h
  DWORD         dwUnkown4;                                          // 18h
  BYTE          lpszModuleFileName[256];                            // 1Ch
} MODULE_INFO, *PMODULE_INFO;

typedef struct _MODULE_LIST
{
  DWORD         dwModuleCount;
  MODULE_INFO   ModuleInfo[1];
} MODULE_LIST, *PMODULE_LIST;

typedef struct _PROCESS_DEBUG_INFORMATION
{
  DWORD         dwUnkown0;                                          // 00h
  DWORD         dwUnkown1;                                          // 04h
  DWORD         dwUnkown2;                                          // 08h
  DWORD         dwUnkown3;                                          // 0Ch
  DWORD         dwUnkown4;                                          // 10h
  DWORD         dwUnkown5;                                          // 14h
  DWORD         dwUnkown6;                                          // 18h
  DWORD         dwUnkown7;                                          // 1Ch
  DWORD         dwUnkown8;                                          // 20h
  DWORD         dwUnkown9;                                          // 24h
  DWORD         dwUnkown10;                                         // 28h
  DWORD         dwUnkown11;                                         // 2Ch - exe hModule
  PMODULE_LIST  lpModules;                                          // 30h
} PROCESS_DEBUG_INFORMATION, *PPROCESS_DEBUG_INFORMATION;

typedef struct _PROCESS_PARAMETRS
{
  ULONG   AllocationSize;
  ULONG   ActualSize;
  ULONG   Flags;
  ULONG   Unknown1;
  HANDLE  InputHandle;
  HANDLE  OutputHandle;
  HANDLE  ErrorHandle;
  UNICODE_STRING Unknown3;         //??????????????????????
  UNICODE_STRING CurrentDirectory;
  HANDLE  CurrentDir;
  //UNICODE_STRING Unknown3;
  UNICODE_STRING SearchPath;        //?????????????????????
  UNICODE_STRING ApplicationName;
  //UNICODE_STRING SearchPath;          //?????????????????????
  UNICODE_STRING CommandLine;
  PVOID   EnvironmentBlock;
  ULONG   Unknown[9];
  UNICODE_STRING Unknown4;
  UNICODE_STRING Unknown5;
  UNICODE_STRING Unknown6;
  UNICODE_STRING Unknown7;
} PROCESS_PARAMETRS, *PPROCESS_PARAMETRS;

typedef struct _PROCESS_ENVIRONMENT_BLOCK
{
  WORD    unknown0;
  WORD    unknown1;  // 0x02 байт, 1 - мы под дебyггеpом, 0 - нет
  DWORD   unknown2;
  void    *pImage;   // 0x08 pImage - yказатель на обpаз пpоги в памяти
  HANDLE  *handles;  // 0x10 handle[6],handle[7],handle[8] - console
                     //      stdin,stdout,stderr всякие хендлы. DLLlist
  PPROCESS_PARAMETRS    pi;       // 0x14 yказатель на стpyктypy, из котоpой заполняется
                     // STARTUPINFO, но она отличается от STARTUPINFO
  HANDLE  hHeap;     // 0x18 process heap
  DWORD   lock;      // 0x1C ???
  void    *ack_lock; // 0x20 Acquire lock func
  void    *rel_lock; // 0x24 Release lock func
  DWORD   *lock_cnt; // 0x28 счётчик lock'ов
  void    *user_cb;  // 0x2C yказатель на фyнкции Callback диспетчеpа
                     // KiUserCallbacksDispatcher(n) вызывает user_sb[n]();
  HANDLE  *heaps;    // 0x88 heaps пpоцесса
  //CRITICAL_SECTION *cs; // 0xA0 yказатель на crit. sect.
  void    *cs;
  DWORD   ver;       // 0xB0 веpсия пpоцесса ??
} PROCESS_ENVIRONMENT_BLOCK, *PPROCESS_ENVIRONMENT_BLOCK;

typedef PROCESS_ENVIRONMENT_BLOCK PEB, *PPEB;

//
// Process Information Classes
//

typedef enum _PROCESSINFOCLASS 
{
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
    MaxProcessInfoClass
} PROCESSINFOCLASS;

//
// Basic Process Information
//  NtQueryInformationProcess using ProcessBasicInfo
//
typedef struct _PROCESS_BASIC_INFORMATION 
{
  NTSTATUS ExitStatus;
  PPEB PebBaseAddress;
  KAFFINITY AffinityMask;
  KPRIORITY BasePriority;
  ULONG UniqueProcessId;
  ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;
typedef PROCESS_BASIC_INFORMATION *PPROCESS_BASIC_INFORMATION;

typedef struct _OBJECT_ATTRIBUTES 
{
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
  PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }

typedef struct _CLIENT_ID 
{
  HANDLE UniqueProcess;
  HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;

typedef ULONG ACCESS_MASK;

#define DELETE                           (0x00010000L)
#define READ_CONTROL                     (0x00020000L)
#define WRITE_DAC                        (0x00040000L)
#define WRITE_OWNER                      (0x00080000L)
#define SYNCHRONIZE                      (0x00100000L)

#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)

#define STANDARD_RIGHTS_READ             (READ_CONTROL)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)

#define STANDARD_RIGHTS_ALL              (0x001F0000L)

#define SPECIFIC_RIGHTS_ALL              (0x0000FFFFL)

typedef struct _PRELATIVE_NAME
{
  UNICODE_STRING Name;
  HANDLE CurrentDir;
} PRELATIVE_NAME, *PPRELATIVE_NAME;

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

typedef struct _OBJECT_NAME_INFORMATION 
{
  UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

//typedef struct _GENERIC_MAPPING 
//{
//  ACCESS_MASK GenericRead;
//  ACCESS_MASK GenericWrite;
//  ACCESS_MASK GenericExecute;
//  ACCESS_MASK GenericAll;
//} GENERIC_MAPPING;
//typedef GENERIC_MAPPING *PGENERIC_MAPPING;

typedef enum _OBJECTINFOCLASS
{
  BaseObjectInfo = 0,
  NameObjectInfo,           // ObjectInformationLength = 0x200;
  TypeObjectInfo,           // ObjectInformationLength = 0x200;
  UnknownObjectInfo,        //
  HandleObjectInfo          // ObjectInformationLength = 0x200;
} OBJECTINFOCLASS;

typedef struct _NAME_OBJECT_INFO
{
  UNICODE_STRING  Name;
} NAME_OBJECT_INFO, *PNAME_OBJECT_INFO;

typedef struct _TYPE_OBJECT_INFO
{
  UNICODE_STRING  Type;
  ULONG           InstanceCount;
  ULONG           HandleCount;
  ULONG           Unknown1[11];
  GENERIC_MAPPING GenericMapping;
  ACCESS_MASK     MaximumAllowed;
  ULONG           Unknown2[4];
} TYPE_OBJECT_INFO, *PTYPE_OBJECT_INFO;

typedef struct _SYSTEM_LOAD_IMAGE
{
  UNICODE_STRING ModuleName;
  PVOID          ModuleBase;
  PVOID          ModuleSection;
  PVOID          EntryPoint;
  PVOID          ExportDirectory;
} SYSTEM_LOAD_IMAGE, *PSYSTEM_LOAD_IMAGE;

typedef struct _SYSTEM_UNLOAD_IMAGE
{
  PVOID          ModuleSection;
} SYSTEM_UNLOAD_IMAGE, *PSYSTEM_UNLOAD_IMAGE;

typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE
{
  UNICODE_STRING ModuleName;
} SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE;

//NtQuerySystemInformation
typedef long (__stdcall *NTQUERYSYSTEMINFORMATION)(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, DWORD SystemInformationLength, PDWORD LehgthReturned);
//RtlCreateQueryDebugBuffer
typedef PPROCESS_DEBUG_INFORMATION (__stdcall *RTLCREATEQUERYDEBUGBUFFER)(DWORD, DWORD);
//RtlDestroyQueryDebugBuffer
typedef long (__stdcall *RTLDESTROYQUERYDEBUGBUFFER)(PPROCESS_DEBUG_INFORMATION);
//RtlQueryProcessDebugInformation
typedef long (__stdcall *RTLQUERYPROCESSDEBUGINFORMATION)(DWORD dwProcessId, DWORD, PPROCESS_DEBUG_INFORMATION lpBuf);
//                                                                             1 
//NtQueryInformationProcess
typedef long (__stdcall *NTQUERYINFORMATIONPROCESS)(HANDLE ProcessHandle,  PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
//NtOpenProcess
typedef long (__stdcall *NTOPENPROCESS)(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);
//RtlDosPathNameToNtPathName_U
typedef long (__stdcall *RTLDOSPATHNAMETONTPATHNAME_U)(PCWSTR DosPathName, PUNICODE_STRING NtPathName, PWSTR* FilePathInNtPathName, PRELATIVE_NAME* RelativeName);
//NtClose
typedef long (__stdcall *NTCLOSE)(HANDLE Handle);
//NtOpenDirectoryObject
typedef long (__stdcall *NTOPENDIRECTORYOBJECT)(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
//NtOpenSymbolicLinkObject
typedef long (__stdcall *NTOPENSYMBOLICLINKOBJECT)(PHANDLE SymbolicLinkHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
//RtlInitUnicodeString
typedef void (__stdcall *RTLINITUUNICODESTRING)(PUNICODE_STRING DestinationString, PCWSTR SourceString);
//NtQueryDirectoryObject
typedef long (__stdcall *NTQUERYDIRECTORYOBJECT)(HANDLE DirectoryObjectHandle, PVOID ObjectInfoBuffer, ULONG ObjectInfoBufferLength, DIRECTORYINFOCLASS DirectoryInformationClass, BOOLEAN First, PULONG ObjectIndex, PULONG LengthReturned);
//NtQuerySymbolicLinkObject
typedef long (__stdcall *NTQUERYSYMBOLICLINKOBJECT)(HANDLE SymbolicLinkHandle, PUNICODE_STRING ObjectNameBuffer, PULONG LengthReturned);
//RtlGetFullPathName_U
typedef long (__stdcall *RTLGETFULLPATHNAME_U)(PWSTR pwszFileName, DWORD dwSizeFullFileName, PWSTR pwszFullFileName, PWSTR *ppwszFullFilePart);
//NtQueryObject
typedef long (__stdcall *NTQUERYOBJECT)(HANDLE ObjectHandle, OBJECTINFOCLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG LengthReturned);
//NtSetSystemInformation
typedef long (__stdcall *NTSETSYSTEMINFORMATION)(SYSTEM_INFORMATION_CLASS SysInfoClass, PVOID pArray, ULONG dwSizeArray);
//RtlAdjustPrivilege
typedef long (__stdcall *RTLADJUSTPRIVILEGE)(ULONG Privilege, BOOLEAN Enable, BOOLEAN CurrentThread, BOOLEAN* Enabled);


#ifdef __cplusplus
}
#endif

#endif //_NTDLL_H_
#endif //_NTDDK_

NTSTATUS NtGetProcessList(PPROCESS_INFO lpProcessInfo, DWORD dwProcessInfoSize, DWORD *lpLehgthReturned);

PPROCESS_DEBUG_INFORMATION NtRtlCreateQueryDebugBuffer(void);
NTSTATUS NtRtlDestroyQueryDebugBuffer(PPROCESS_DEBUG_INFORMATION lpDebugBuffer);
NTSTATUS NtRtlQueryProcessDebugInformation(PPROCESS_DEBUG_INFORMATION lpDebugBuffer, DWORD dwProcessId);
DWORD    NtGetProcessCommandLine(PWSTR lpwszCommandLine, DWORD dwSizeByByte, DWORD dwProcesId, DWORD dwThreadId = 0);
DWORD    GetSymbolicLinkObject(PWSTR pwszDirectoryObject, PWSTR pwszSymLink, PVOID pwszSymLinkObject, DWORD dwSizeOfSymLinkObjectByBytes);
BOOL     QueryObjectNameType(PVOID pObjTypeInfo, DWORD dwSizeOfObjTypeInfo, HANDLE hDir, PWSTR pwszObjectName);
DWORD    DosPathNameToNtPathName(PWSTR pwszDosPath, PWSTR pwszNtPath, DWORD dwSizeNtPathByBytes);
PTHREAD_INFO GetThreadInfoPtr(PPROCESS_INFO pProcessInfo);
DWORD    GetOsVersion(void);
BOOL     LoadDevice(PWSTR pwszDeviceFileName);

#endif //__NTPROCESSLIST_H