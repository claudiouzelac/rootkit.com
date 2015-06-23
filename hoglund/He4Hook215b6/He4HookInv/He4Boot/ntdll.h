//ntdll.h
//API Windows NT

#ifndef _NTDLL_H_
 #define _NTDLL_H_

#ifdef _MSC_VER
 #pragma pack(push,8)
#endif //_MSC_VER

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IN
 #define IN
#endif //IN

#ifndef OUT
 #define OUT
#endif //OUT

#ifndef OPTIONAL
 #define OPTIONAL
#endif //OPTIONAL

#if defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC) && !defined(MIDL_PASS)
 #define DECLSPEC_IMPORT __declspec(dllimport)
#else
 #define DECLSPEC_IMPORT
#endif

#if defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC) && !defined(MIDL_PASS)
 #define DECLSPEC_EXPORT __declspec(dllexport)
#else
 #define DECLSPEC_EXPORT
#endif

#if (_MSC_VER>=800) || defined(_STDCALL_SUPPORTED)
 #define NTAPI __stdcall
#else
 #define _cdecl
 #define NTAPI
#endif

#if !defined(_NTSYSTEM_)
 #define NTSYSAPI DECLSPEC_IMPORT
#else
 #define NTSYSAPI DECLSPEC_EXPORT
#endif

#ifndef CONST
 #define CONST               const
#endif

#ifndef VOID
 #define VOID void
 typedef char CHAR;
 typedef short SHORT;
 typedef long LONG;
#endif

typedef void *PVOID;    // winnt

#define FALSE   0
#define TRUE    1

#ifndef NULL
 #ifdef __cplusplus
  #define NULL    0
 #else
  #define NULL    ((void *)0)
 #endif
#endif // NULL

#ifndef _WCHAR_T_DEFINED
 typedef unsigned short wchar_t;
 #define _WCHAR_T_DEFINED
#endif //_WCHAR_T_DEFINED

typedef wchar_t WCHAR;
typedef WCHAR *LPWSTR, *PWSTR;
typedef CONST WCHAR *LPCWSTR, *PCWSTR;
typedef CHAR *LPSTR, *PSTR, *PCHAR;
typedef CONST CHAR *LPCSTR, *PCSTR;

#define UNICODE_NULL ((WCHAR)0) // winnt

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef UCHAR *PUCHAR;
typedef USHORT *PUSHORT;
typedef ULONG *PULONG;

typedef unsigned long       DWORD;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef void               *LPVOID;

typedef void *HANDLE;
typedef HANDLE *PHANDLE;
typedef UCHAR BOOLEAN;           // winnt
typedef BOOLEAN *PBOOLEAN;       // winnt
typedef long NTSTATUS;

#ifndef _WINNT_

typedef struct _LARGE_INTEGER {
     ULONG LowPart;
     LONG HighPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct _ULARGE_INTEGER {
     ULONG LowPart;
     ULONG HighPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

typedef LARGE_INTEGER LUID, *PLUID;

#endif //_WINNT_

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;

typedef UNICODE_STRING *PUNICODE_STRING;

NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );


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

//
// Object Attributes structure
//

typedef struct _OBJECT_ATTRIBUTES {
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

#define OBJ_NAME_PATH_SEPARATOR ((WCHAR) L'\\')

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

//
// AccessSystemAcl access type
//

#define ACCESS_SYSTEM_SECURITY           (0x01000000L)

//
// MaximumAllowed access type
//

#define MAXIMUM_ALLOWED                  (0x02000000L)

//
//  These are the generic rights.
//

#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

NTSYSAPI
NTSTATUS
NTAPI
NtClose(
    IN HANDLE Handle
    );


//
// Object Manager Directory Specific Access Rights.
//

#define DIRECTORY_QUERY                 (0x0001)
#define DIRECTORY_TRAVERSE              (0x0002)
#define DIRECTORY_CREATE_OBJECT         (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY   (0x0008)

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

NTSYSAPI
NTSTATUS
NTAPI
NtOpenDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    ); 

typedef struct _OBJECT_NAMETYPE_INFO {               
    UNICODE_STRING ObjectName;
    UNICODE_STRING ObjectType;
} OBJECT_NAMETYPE_INFO, *POBJECT_NAMETYPE_INFO;   

typedef enum _DIRECTORYINFOCLASS {
    ObjectArray,
    ObjectByOne
} DIRECTORYINFOCLASS, *PDIRECTORYINFOCLASS;

#define QUERY_DIRECTORY_BUF_SIZE 0x200

NTSYSAPI
NTSTATUS
NTAPI
NtQueryDirectoryObject(
    IN PHANDLE DirectoryObjectHandle,
    OUT PVOID ObjectInfoBuffer,
    IN ULONG ObjectInfoBufferLength,
    IN DIRECTORYINFOCLASS DirectoryInformationClass,
    IN BOOLEAN First,
    IN OUT PULONG ObjectIndex,
    OUT PULONG LengthReturned
    ); 

NTSYSAPI
NTSTATUS
NTAPI
NtDisplayString(
    IN PUNICODE_STRING DisplayString
    );


//
// Registry Specific Access Rights.
//

#define KEY_QUERY_VALUE         (0x0001)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_ENUMERATE_SUB_KEYS  (0x0008)
#define KEY_NOTIFY              (0x0010)
#define KEY_CREATE_LINK         (0x0020)

#define KEY_READ                ((STANDARD_RIGHTS_READ       |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY)                 \
                                  &                           \
                                 (~SYNCHRONIZE))


#define KEY_WRITE               ((STANDARD_RIGHTS_WRITE      |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY)         \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_EXECUTE             ((KEY_READ)                   \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_ALL_ACCESS          ((STANDARD_RIGHTS_ALL        |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY         |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY                 |\
                                  KEY_CREATE_LINK)            \
                                  &                           \
                                 (~SYNCHRONIZE))

//
// Open/Create Options
//

#define REG_OPTION_RESERVED         (0x00000000L)   // Parameter is reserved

#define REG_OPTION_NON_VOLATILE     (0x00000000L)   // Key is preserved
                                                    // when system is rebooted

#define REG_OPTION_VOLATILE         (0x00000001L)   // Key is not preserved
                                                    // when system is rebooted

#define REG_OPTION_CREATE_LINK      (0x00000002L)   // Created key is a
                                                    // symbolic link

#define REG_OPTION_BACKUP_RESTORE   (0x00000004L)   // open for backup or restore
                                                    // special access rules
                                                    // privilege required

#define REG_OPTION_OPEN_LINK        (0x00000008L)   // Open symbolic link

#define REG_LEGAL_OPTION            \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_NON_VOLATILE        |\
                 REG_OPTION_VOLATILE            |\
                 REG_OPTION_CREATE_LINK         |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)

//
// Key creation/open disposition
//

#define REG_CREATED_NEW_KEY         (0x00000001L)   // New Registry Key created
#define REG_OPENED_EXISTING_KEY     (0x00000002L)   // Existing Key opened

//
// Key restore flags
//

#define REG_WHOLE_HIVE_VOLATILE     (0x00000001L)   // Restore whole hive volatile
#define REG_REFRESH_HIVE            (0x00000002L)   // Unwind changes to last flush
#define REG_NO_LAZY_FLUSH           (0x00000004L)   // Never lazy flush this hive

//
// Predefined Value Types.
//

#define REG_NONE                    ( 0 )   // No value type
#define REG_SZ                      ( 1 )   // Unicode nul terminated string
#define REG_EXPAND_SZ               ( 2 )   // Unicode nul terminated string
                                            // (with environment variable references)
#define REG_BINARY                  ( 3 )   // Free form binary
#define REG_DWORD                   ( 4 )   // 32-bit number
#define REG_DWORD_LITTLE_ENDIAN     ( 4 )   // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN        ( 5 )   // 32-bit number
#define REG_LINK                    ( 6 )   // Symbolic Link (unicode)
#define REG_MULTI_SZ                ( 7 )   // Multiple Unicode strings
#define REG_RESOURCE_LIST           ( 8 )   // Resource list in the resource map
#define REG_FULL_RESOURCE_DESCRIPTOR ( 9 )  // Resource list in the hardware description
#define REG_RESOURCE_REQUIREMENTS_LIST ( 10 )

//
// Key query structures
//

typedef struct _KEY_BASIC_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_NODE_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   ClassOffset;
    ULONG   ClassLength;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
//          Class[1];           // Variable length string not declared
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef struct _KEY_FULL_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   ClassOffset;
    ULONG   ClassLength;
    ULONG   SubKeys;
    ULONG   MaxNameLen;
    ULONG   MaxClassLen;
    ULONG   Values;
    ULONG   MaxValueNameLen;
    ULONG   MaxValueDataLen;
    WCHAR   Class[1];           // Variable length
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef enum _KEY_INFORMATION_CLASS {
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation
} KEY_INFORMATION_CLASS;

typedef struct _KEY_WRITE_TIME_INFORMATION {
    LARGE_INTEGER LastWriteTime;
} KEY_WRITE_TIME_INFORMATION, *PKEY_WRITE_TIME_INFORMATION;

typedef enum _KEY_SET_INFORMATION_CLASS {
    KeyWriteTimeInformation
} KEY_SET_INFORMATION_CLASS;

//
// Value entry query structures
//

typedef struct _KEY_VALUE_BASIC_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable size
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   DataOffset;
    ULONG   DataLength;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable size
//          Data[1];            // Variable size data not declared
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   DataLength;
    UCHAR   Data[1];            // Variable size
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_ENTRY {
    PUNICODE_STRING ValueName;
    ULONG           DataLength;
    ULONG           DataOffset;
    ULONG           Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation
} KEY_VALUE_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
NtLoadKey(
    IN POBJECT_ATTRIBUTES KeyToLoad,
    IN POBJECT_ATTRIBUTES FileToLoad
    );

NTSYSAPI
NTSTATUS
NTAPI
NtUnloadKey(
    IN POBJECT_ATTRIBUTES KeyToUnLoad
    );

NTSYSAPI
NTSTATUS
NTAPI
NtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );


NTSYSAPI
NTSTATUS
NTAPI
NtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
NtSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    );

NTSYSAPI
NTSTATUS
NTAPI
NtEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

NTSYSAPI
NTSTATUS
NTAPI
NtEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

NTSYSAPI
NTSTATUS
NTAPI
NtFlushKey(
    IN HANDLE KeyHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
NtQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

NTSYSAPI
NTSTATUS
NTAPI
NtQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

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

NTSYSAPI
NTSTATUS
NTAPI
RtlAdjustPrivilege(
    IN ULONG Privilege,
    IN BOOLEAN Enable,
    IN BOOLEAN CurrentThread,
    OUT PBOOLEAN Enabled
    );

typedef struct _PRELATIVE_NAME{
    UNICODE_STRING Name;
    HANDLE CurrentDir;
} PRELATIVE_NAME, *PPRELATIVE_NAME;

NTSYSAPI
NTSTATUS
NTAPI
RtlDosPathNameToNtPathName_U(
    IN PCWSTR DosPathName,
    OUT PUNICODE_STRING NtPathName,
    OUT PWSTR* FilePathInNtPathName OPTIONAL,
    OUT PRELATIVE_NAME* RelativeName OPTIONAL
    );

/**********************************************************************/
NTSYSAPI
NTSTATUS
NTAPI
NtShutdownSystem(
    IN HANDLE KeyHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
    IN HANDLE ObjectHandle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

typedef enum _WAIT_TYPE
{
  WaitAll,
  WaitAny
} WAIT_TYPE;

NTSYSAPI
NTSTATUS
NTAPI
NtWaitForMultipleObjects(
    IN ULONG NumberOfHandles,
    IN PHANDLE ArrayOfHandles,
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

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
  void    *pImage;   // 0x08 pImage - yказатель на обpаз пpоги в пам€ти
  HANDLE  *handles;  // 0x10 handle[6],handle[7],handle[8] - console
                     //      stdin,stdout,stderr вс€кие хендлы. DLLlist
  PPROCESS_PARAMETRS    pi;       // 0x14 yказатель на стpyктypy, из котоpой заполн€етс€
                     // STARTUPINFO, но она отличаетс€ от STARTUPINFO
  HANDLE  hHeap;     // 0x18 process heap
  DWORD   lock;      // 0x1C ???
  void    *ack_lock; // 0x20 Acquire lock func
  void    *rel_lock; // 0x24 Release lock func
  DWORD   *lock_cnt; // 0x28 счЄтчик lock'ов
  void    *user_cb;  // 0x2C yказатель на фyнкции Callback диспетчеpа
                     // KiUserCallbacksDispatcher(n) вызывает user_sb[n]();
  HANDLE  *heaps;    // 0x88 heaps пpоцесса
  //CRITICAL_SECTION *cs; // 0xA0 yказатель на crit. sect.
  void    *cs;
  DWORD   ver;       // 0xB0 веpси€ пpоцесса ??
} PROCESS_ENVIRONMENT_BLOCK, *PPROCESS_ENVIRONMENT_BLOCK;

typedef PROCESS_ENVIRONMENT_BLOCK PEB, *PPEB;

typedef struct _THREAD_ENVIRONMENT_BLOCK
{
    void            *except;       // 0x00 Hачало цепочки exception handlers
    void            *stack_top;    // 0x04 ¬еpхyшка стека
    void            *stack_low;    // 0x08 Hиз стека
    WORD            unk1;          // 0x0c Hевы€снено
    WORD            unk2;          // 0x0e Hевы€снено
    DWORD           unk3;          // 0x10 Hевы€снено
    DWORD           unk4;          // 0x14 Hевы€снено
    void            *self;         // 0x18 ”казатель на себ€
    WORD            flags;         // 0x1c ‘лаги тpеда
    WORD            unk5;          // 0x1e Hевы€снено
    DWORD           Pid;           // 0x20 PID - ID текyщего пpоцесса
    DWORD           Tid;           // 0x24 TID - ID данного тpеда
    WORD            unk6;          // 0x28 Hевы€снено
    WORD            unk7;          // 0x2a Hевы€снено
    LPVOID          *tls_ptr;      // 0x2c ”казатель на TLS
    PROCESS_ENVIRONMENT_BLOCK *peb;// 0x30 process environment block
    DWORD           LastError;     // 0x34 LastError - то, что возвpащает
                                   //      GetLastError();
//ƒальше - тЄмный лес.
    // 0x40 количество сообщений в очеpеди ???
    // 0x44 yказатель на очеpедь сообщений ???
    // 0x54 AppCompatFlags ???
    // 0xC4 локаль текyшего тpеда
//Ѕольшой пpопyск
    // 0xBF4 NTSTATUS - pезyльтат последнего вызова к-либо фyнкции из ntdll
    // 0xBF8 UNICODE_STRING ??? - кака€-то стpока
} THREAD_ENVIRONMENT_BLOCK, *PTHREAD_ENVIRONMENT_BLOCK;

typedef THREAD_ENVIRONMENT_BLOCK TEB, *PTEB;      
/*
 ак эта фyнкци€ сделана:

@94 NtCurrentTeb:

     mov     eax,fs:[0018]
     ret

по смещению +0х30 от начала TEB хpанитс€ 
yказатель на Process Environment Block
*/
typedef struct _CLIENT_ID 
{
  HANDLE UniqueProcess;
  HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;


typedef struct _SECTION_IMAGE_INFORMATION
{
  ULONG  EntryPoint;
  ULONG  Unknown0;
  ULONG  ReservedStackSize;
  ULONG  CommitedStackSize;
  ULONG  SubSystem;
  USHORT SubsystemVersionMinor;
  USHORT SubsystemVersionMajor;
  ULONG  Unknown1;
  ULONG  Characteristics;
  ULONG  Machine;
  ULONG  Unknown2;
  ULONG  Unknown3;
  ULONG  Unknown4;
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

typedef struct _RTL_PROCESS_INFORMATION
{
  ULONG Size;
  HANDLE ProcessHandle;
  HANDLE ThreadHandle;
  CLIENT_ID ClientId;
  SECTION_IMAGE_INFORMATION SectionImageInfo;
} RTL_PROCESS_INFORMATION, *PRTL_PROCESS_INFORMATION;


NTSYSAPI
HANDLE
NTAPI
NtCurrentProcess(
    VOID
    );

NTSYSAPI
PTEB
NTAPI
NtCurrentTeb(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserProcess(
    IN PUNICODE_STRING FileName,
    IN ULONG FileObjectAttributes,
    IN PPROCESS_PARAMETRS ProcessParameters,
    IN PVOID ProcessSecurityDescriptor OPTIONAL,
    IN PVOID ThreadSecurityDescriptor OPTIONAL,
    IN HANDLE ParrentProcess OPTIONAL,
    IN BOOLEAN InheritHandles,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL,
    OUT PRTL_PROCESS_INFORMATION ProcessInfo
    );

NTSYSAPI
NTSTATUS
NTAPI
NtTerminateProcess(
    IN HANDLE ProcessHandle,
    IN ULONG ProcessExitCode
    );

NTSYSAPI
VOID
NTAPI
LdrShutdownProcess(
    VOID
    );

NTSYSAPI
VOID
NTAPI
NtSuspendThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount OPTIONAL
    );

NTSYSAPI
VOID
NTAPI
NtResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateProcessParameters(
    OUT PPROCESS_PARAMETRS* ProcessParameters,
    IN PUNICODE_STRING ApplicationName,
    IN PUNICODE_STRING SearchPaths OPTIONAL,
    IN PUNICODE_STRING CurrentDirectory OPTIONAL,
    IN PUNICODE_STRING CommandLine OPTIONAL,
    IN PVOID EnvironmentBlock OPTIONAL,
    IN PUNICODE_STRING Unknown1 OPTIONAL,
    IN PUNICODE_STRING Unknown2 OPTIONAL,
    IN PUNICODE_STRING Unknown3 OPTIONAL,
    IN PUNICODE_STRING Unknown4 OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyProcessParameters(
    IN PPROCESS_PARAMETRS ProcessParameters
    );

NTSYSAPI
PPROCESS_PARAMETRS
NTAPI
RtlDeNormalizeProcessParams(
    IN PPROCESS_PARAMETRS ProcessParameters
    );

NTSYSAPI
PPROCESS_PARAMETRS
NTAPI
RtlNormalizeProcessParams(
    IN PPROCESS_PARAMETRS ProcessParameters
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U(
    IN PVOID EnvironmentBlock OPTIONAL,
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING VariableValue
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlExpandEnvironmentVariable_U(
    IN PVOID EnvironmentBlock OPTIONAL,
    IN PUNICODE_STRING SourceString,
    OUT PUNICODE_STRING ExpandString,
    OUT PULONG BytesRequired
    );

#define NtGetProcessHeap() \
     (NtCurrentTeb()->peb->hHeap)

#ifndef _CRTIMP
 #define _CRTIMP NTSYSAPI
#endif

#ifndef _INC_STRING
 #ifndef _INC_MEMORY
  _CRTIMP void * __cdecl memmove(void *, const void *, int); //size_t
  _CRTIMP void * __cdecl memcpy(void *, const void *, int); //size_t
  _CRTIMP void * __cdecl memset(void *, int, int); //size_t
  _CRTIMP char *  __cdecl strcpy(char *, const char *);
  _CRTIMP size_t  __cdecl strlen(const char *);
 #endif
#endif

#ifndef _INC_WCHAR
 _CRTIMP int __cdecl swprintf(wchar_t *, const wchar_t *, ...);

 _CRTIMP wchar_t * __cdecl wcscat(wchar_t *, const wchar_t *);
 _CRTIMP wchar_t * __cdecl wcschr(const wchar_t *, wchar_t);
 _CRTIMP int __cdecl wcscmp(const wchar_t *, const wchar_t *);
 _CRTIMP wchar_t * __cdecl wcscpy(wchar_t *, const wchar_t *);
 _CRTIMP size_t __cdecl wcscspn(const wchar_t *, const wchar_t *);
 _CRTIMP size_t __cdecl wcslen(const wchar_t *);
 _CRTIMP wchar_t * __cdecl wcsncat(wchar_t *, const wchar_t *, size_t);
 _CRTIMP int __cdecl wcsncmp(const wchar_t *, const wchar_t *, size_t);
 _CRTIMP wchar_t * __cdecl wcsncpy(wchar_t *, const wchar_t *, size_t);
 _CRTIMP wchar_t * __cdecl wcspbrk(const wchar_t *, const wchar_t *);
 _CRTIMP wchar_t * __cdecl wcsrchr(const wchar_t *, wchar_t);
 _CRTIMP size_t __cdecl wcsspn(const wchar_t *, const wchar_t *);
 _CRTIMP wchar_t * __cdecl wcsstr(const wchar_t *, const wchar_t *);
 _CRTIMP wchar_t * __cdecl wcstok(wchar_t *, const wchar_t *);
#endif

#define HEAP_NO_SERIALIZE               0x00000001      
#define HEAP_GROWABLE                   0x00000002      
#define HEAP_GENERATE_EXCEPTIONS        0x00000004      
#define HEAP_ZERO_MEMORY                0x00000008      
#define HEAP_REALLOC_IN_PLACE_ONLY      0x00000010      
#define HEAP_TAIL_CHECKING_ENABLED      0x00000020      
#define HEAP_FREE_CHECKING_ENABLED      0x00000040      
#define HEAP_DISABLE_COALESCE_ON_FREE   0x00000080      
#define HEAP_CREATE_ALIGN_16            0x00010000      
#define HEAP_CREATE_ENABLE_TRACING      0x00020000      

NTSYSAPI
LPVOID 
NTAPI
RtlAllocateHeap(
    HANDLE hHeap, ULONG dwFlags, ULONG dwBytes
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
    HANDLE hHeap,
    ULONG dwFlags, 
    LPVOID lpMem
    );

#define PAGE_NOACCESS          0x01     
#define PAGE_READONLY          0x02     
#define PAGE_READWRITE         0x04     
#define PAGE_WRITECOPY         0x08     
#define PAGE_EXECUTE           0x10     
#define PAGE_EXECUTE_READ      0x20     
#define PAGE_EXECUTE_READWRITE 0x40     
#define PAGE_EXECUTE_WRITECOPY 0x80     
#define PAGE_GUARD            0x100     
#define PAGE_NOCACHE          0x200     
#define PAGE_WRITECOMBINE     0x400     
#define MEM_COMMIT           0x1000     
#define MEM_RESERVE          0x2000     
#define MEM_DECOMMIT         0x4000     
#define MEM_RELEASE          0x8000     
#define MEM_FREE            0x10000     
#define MEM_PRIVATE         0x20000     
#define MEM_MAPPED          0x40000     
#define MEM_RESET           0x80000     
#define MEM_TOP_DOWN       0x100000     
#define MEM_4MB_PAGES    0x80000000     
#define SEC_FILE           0x800000     
#define SEC_IMAGE         0x1000000     
#define SEC_VLM           0x2000000     
#define SEC_RESERVE       0x4000000     
#define SEC_COMMIT        0x8000000     
#define SEC_NOCACHE      0x10000000     
#define MEM_IMAGE         SEC_IMAGE     

NTSYSAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *RegionAddress,
    IN ULONG ZeroBits, // 0 - 21 ????????????????
    IN OUT PULONG RegionSize,
    IN ULONG AllocationType,
    IN ULONG ProtectionType
    );

NTSYSAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(  
    IN HANDLE ProcessHandle,
    IN PVOID *RegionAddress,
    IN PULONG RegionSize,
    IN ULONG FreeType
    );


NTSYSAPI
VOID 
NTAPI
RtlAcquirePebLock(
    VOID
    );

NTSYSAPI
VOID 
NTAPI
RtlReleasePebLock(
    VOID
    );



//
// Define the base asynchronous I/O argument types
//

typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//


#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe


// begin_winnt

//
// Define access rights to files and directories
//

//
// The FILE_READ_DATA and FILE_WRITE_DATA constants are also defined in
// devioctl.h as FILE_READ_ACCESS and FILE_WRITE_ACCESS. The values for these
// constants *MUST* always be in sync.
// The values are redefined in devioctl.h because they must be available to
// both DOS and NT.
//

#define FILE_READ_DATA            ( 0x0001 )    // file & pipe
#define FILE_LIST_DIRECTORY       ( 0x0001 )    // directory

#define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe
#define FILE_ADD_FILE             ( 0x0002 )    // directory

#define FILE_APPEND_DATA          ( 0x0004 )    // file
#define FILE_ADD_SUBDIRECTORY     ( 0x0004 )    // directory
#define FILE_CREATE_PIPE_INSTANCE ( 0x0004 )    // named pipe

#define FILE_READ_EA              ( 0x0008 )    // file & directory

#define FILE_WRITE_EA             ( 0x0010 )    // file & directory

#define FILE_EXECUTE              ( 0x0020 )    // file
#define FILE_TRAVERSE             ( 0x0020 )    // directory

#define FILE_DELETE_CHILD         ( 0x0040 )    // directory

#define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all

#define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    // all

#define FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)

#define FILE_GENERIC_READ         (STANDARD_RIGHTS_READ     |\
                                   FILE_READ_DATA           |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_READ_EA             |\
                                   SYNCHRONIZE)


#define FILE_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE    |\
                                   FILE_WRITE_DATA          |\
                                   FILE_WRITE_ATTRIBUTES    |\
                                   FILE_WRITE_EA            |\
                                   FILE_APPEND_DATA         |\
                                   SYNCHRONIZE)


#define FILE_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE  |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_EXECUTE             |\
                                   SYNCHRONIZE)

// end_winnt


//
// Define share access rights to files and directories
//

#define FILE_SHARE_READ                 0x00000001  // winnt
#define FILE_SHARE_WRITE                0x00000002  // winnt
#define FILE_SHARE_DELETE               0x00000004  // winnt
#define FILE_SHARE_VALID_FLAGS          0x00000007

//
// Define the file attributes values
//
// Note:  0x00000008 is reserved for use for the old DOS VOLID (volume ID)
//        and is therefore not considered valid in NT.
//
// Note:  0x00000010 is reserved for use for the old DOS SUBDIRECTORY flag
//        and is therefore not considered valid in NT.  This flag has
//        been disassociated with file attributes since the other flags are
//        protected with READ_ and WRITE_ATTRIBUTES access to the file.
//
// Note:  Note also that the order of these flags is set to allow both the
//        FAT and the Pinball File Systems to directly set the attributes
//        flags in attributes words without having to pick each flag out
//        individually.  The order of these flags should not be changed!
//

#define FILE_ATTRIBUTE_READONLY         0x00000001  // winnt
#define FILE_ATTRIBUTE_HIDDEN           0x00000002  // winnt
#define FILE_ATTRIBUTE_SYSTEM           0x00000004  // winnt
#define FILE_ATTRIBUTE_DIRECTORY        0x00000010  // winnt
#define FILE_ATTRIBUTE_ARCHIVE          0x00000020  // winnt
#define FILE_ATTRIBUTE_NORMAL           0x00000080  // winnt
#define FILE_ATTRIBUTE_TEMPORARY        0x00000100  // winnt
#define FILE_ATTRIBUTE_RESERVED0        0x00000200
#define FILE_ATTRIBUTE_RESERVED1        0x00000400
#define FILE_ATTRIBUTE_COMPRESSED       0x00000800  // winnt
#define FILE_ATTRIBUTE_OFFLINE          0x00001000  // winnt
#define FILE_ATTRIBUTE_PROPERTY_SET     0x00002000
#define FILE_ATTRIBUTE_VALID_FLAGS      0x00003fb7
#define FILE_ATTRIBUTE_VALID_SET_FLAGS  0x00003fa7

//
// Define the create disposition values
//

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005


//
// Define the create/open option flags
//

#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080

#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
//UNUSED                                        0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800

#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000


#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_TRANSACTED_MODE                    0x00200000
#define FILE_OPEN_OFFLINE_FILE                  0x00400000

#define FILE_VALID_OPTION_FLAGS                 0x007fffff
#define FILE_VALID_PIPE_OPTION_FLAGS            0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS        0x00000032
#define FILE_VALID_SET_FLAGS                    0x00000036

//
// Define the I/O status information return values for NtCreateFile/NtOpenFile
//

#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_CREATED                    0x00000002
#define FILE_OVERWRITTEN                0x00000003
#define FILE_EXISTS                     0x00000004
#define FILE_DOES_NOT_EXIST             0x00000005

//
// Define special ByteOffset parameters for read and write operations
//

#define FILE_WRITE_TO_END_OF_FILE       0xffffffff
#define FILE_USE_FILE_POINTER_POSITION  0xfffffffe


NTSYSAPI
NTSTATUS
NTAPI
NtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    );

#define PIO_APC_ROUTINE void*

NTSYSAPI
NTSTATUS
NTAPI
NtWriteFile(
  IN HANDLE  FileHandle,
  IN HANDLE  Event  OPTIONAL,
  IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
  IN PVOID  ApcContext  OPTIONAL,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN PVOID  Buffer,
  IN ULONG  Length,
  IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
  IN PULONG  Key  OPTIONAL
  );


/*
calling ntquerysysteminformation with a request type of 16
to get back a list of what appears to be objects and their associated pids. the structure contains 4 DWORDs
 
DWORD pid
DWORD unknown
DWORD object
DWORD unknown
 
I believe one of these unknows contains a flag used to determine the object type. does anyone know the type flags?
*/

typedef enum _SYSTEMINFOCLASS {
    SystemInfoBasic = 0,
    SystemInfoProcessor,
    SystemInfoTimeZone,
    SystemInfoTimeInformation,
    SystemInfoUnk4, 
    SystemInfoProcesses,
    SystemInfoUnk6,
    SystemInfoConfiguration,
    SystemInfoUnk8,
    SystemInfoUnk9,
    SystemInfoUnk10,
    SystemInfoDrivers
} SYSTEMINFOCLASS, *PSYSTEMINFOCLASS;

typedef struct _SYSTEM_TIME_INFORMATION
{
  LARGE_INTEGER liKeBootTime;
  LARGE_INTEGER liKeSystemTime;
  LARGE_INTEGER liExpTimeZoneBias;
  ULONG uCurrentTimeZoneId;
  DWORD dwReserved;
} SYSTEM_TIME_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
    IN SYSTEMINFOCLASS SystemInformationClass, 
    OUT PVOID SystemInformation, 
    IN ULONG SystemInformationLength,
    OUT PULONG LehgthReturned OPTIONAL
    );


typedef struct _FILETIME 
{ // ft 
  ULONG dwLowDateTime; 
  ULONG dwHighDateTime; 
} FILETIME; 


typedef struct _THREAD_INFO
{
  FILETIME      ftCreationTime;
  ULONG         dwUnknown1;
  ULONG         dwStartAddress;
  ULONG         dwOwningPID;
  ULONG         dwThreadID;
  ULONG         dwCurrentPriority;
  ULONG         dwBasePriority;
  ULONG         dwContextSwitches;
  ULONG         dwThreadState;
  ULONG         dwUnknown2;
  ULONG         dwUnknown3;
  ULONG         dwUnknown4;
  ULONG         dwUnknown5;
  ULONG         dwUnknown6;
  ULONG         dwUnknown7;
} THREAD_INFO, *PTHREAD_INFO;

typedef struct _PROCESS_INFO
{
  ULONG         dwOffset; // an ofset to the next Process structure
  ULONG         dwThreadCount;
  ULONG         dwUnkown1[6];
  FILETIME      ftCreationTime;
  ULONG         dwUnkown2;
  ULONG         dwUnkown3;
  ULONG         dwUnkown4;
  ULONG         dwUnkown5;
  //WORD          wUnkown6;                                           // 38h
  //WORD          wUnkown6;                                           // 3Ah
  //WCHAR        *pszProcessName;                                     // 3Ch
  UNICODE_STRING ProcessName;                                       // 38h
  ULONG         dwBasePriority;
  ULONG         dwProcessID;
  ULONG         dwParentProcessID;
  ULONG         dwHandleCount;
  ULONG         dwUnkown7;
  ULONG         dwUnkown8;
  ULONG         dwVirtualBytesPeak;
  ULONG         dwVirtualBytes;
  ULONG         dwPageFaults;
  ULONG         dwWorkingSetPeak;
  ULONG         dwWorkingSet;
  ULONG         dwUnkown9;
  ULONG         dwPagedPool; // kbytes
  ULONG         dwUnkown10;
  ULONG         dwNonPagedPool; // kbytes
  ULONG         dwPageFileBytesPeak;
  ULONG         dwPageFileBytes;
  ULONG         dwPrivateBytes;
  ULONG         dwUnkown11;
  ULONG         dwUnkown12;
  ULONG         dwUnkown13;
  ULONG         dwUnkown14;
  THREAD_INFO   ti[1];
  //struct ThreadInfo ati[1];
} PROCESS_INFO, *PPROCESS_INFO;

NTSYSAPI  
NTSTATUS  
NTAPI  
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    );

NTSYSAPI  
NTSTATUS  
NTAPI  
ZwUnloadDriver(
    IN PUNICODE_STRING DriverServiceName
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteKey(
    IN HANDLE KeyHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
    IN HANDLE hKey,
    IN PUNICODE_STRING UniNameKey
    );

#define EXCEPTION_EXECUTE_HANDLER       1
#define EXCEPTION_CONTINUE_SEARCH       0
#define EXCEPTION_CONTINUE_EXECUTION    -1

#ifdef __cplusplus
}
#endif

#ifdef _MSC_VER
#pragma pack(pop)
#endif //_MSC_VER

#endif //_NTDLL_H_