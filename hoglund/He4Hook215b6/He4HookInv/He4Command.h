#ifndef __HE4_HOOK_INV_DEVICE_COMMAND
 #define __HE4_HOOK_INV_DEVICE_COMMAND

#define KE_SERVICE_TABLE_INDEX  2
#define HE4_HOOK_INV_VERSION    0x20001005


#define HE4_SERVICE_INDEX_GET_VERSION            0
#define HE4_SERVICE_INDEX_GET_LOCAL_BASE         1
#define HE4_SERVICE_INDEX_DISPATCH_FUNCTION      2
#define HE4_SERVICE_INDEX_GET_UNLOAD_ADDRESS     3
#define HE4_SERVICE_INDEX_GET_CB_TABLE_ADDRESS   4
//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#ifndef CTL_CODE
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//
// Define the method codes for how buffers are passed for I/O and FS controls
//

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//


#define FILE_ANY_ACCESS                  0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

#endif //CTL_CODE

//#define FILE_DEVICE_HE4HOOK      0x00008350

//************************ Device GUI command ***************************//
#define  HE4_DEVICE_VERSION                   0x01     // Get version
#define  HE4_HOOK_FILE_SYSTEM                 0x02     // Hook file system
#define  HE4_ADD_TO_SAVE_LIST                 0x03     // Add files to save list
#define  HE4_DEL_FROM_SAVE_LIST               0x04     // Delete files from save list
#define  HE4_LOCK_SAVE_FILES                  0x05     // Delete thread from unlock list
#define  HE4_UNLOCK_SAVE_FILES                0x06     // Add thread to unlock list
#define  HE4_QUERY_UNLOAD                     0x07     // Unused
#define  HE4_HOOK_REGISTRY                    0x08     // Temporary unused
#define  HE4_UNHOOK_REGISTRY                  0x09     // Temporary unused
#define  HE4_ADD_KEYS_TO_SAVE_LIST            0x0A     // Temporary unused
#define  HE4_DEL_KEYS_FROM_SAVE_LIST          0x0B     // Temporary unused
#define  HE4_CLEAR_SAVE_LIST                  0x0C     // Clear files save list
#define  HE4_GET_SIZE_BY_BYTES_SAVE_LIST      0x0D
#define  HE4_GET_SAVE_LIST                    0x0E     // Get files save list
#define  HE4_LOCK_SAVE_FILES_FOR_ALL_THREADS  0x0F     // Clear unlock list
#define  HE4_GET_SIZE_BY_BYTES_UNLOCK_LIST    0x10
#define  HE4_GET_UNLOCK_LIST                  0x11
#define  HE4_QUERY_STATISTIC                  0x12

//
// Access type
//
#define ACC_TYPE_READ                0x00000001
#define ACC_TYPE_WRITE               0x00000002
#define ACC_TYPE_DELETE              0x00000004
#define ACC_TYPE_VISIBLE             0x00000008


#define FILE_ACC_TYPE_READ           ACC_TYPE_READ   
#define FILE_ACC_TYPE_WRITE          ACC_TYPE_WRITE  
#define FILE_ACC_TYPE_DELETE         ACC_TYPE_DELETE 
#define FILE_ACC_TYPE_VISIBLE        ACC_TYPE_VISIBLE
#define FILE_ACC_TYPE_EXCHANGE       0x00000010
#define FILE_ACC_TYPE_FULL          (\
                                     FILE_ACC_TYPE_READ     | \
                                     FILE_ACC_TYPE_WRITE    | \
                                     FILE_ACC_TYPE_DELETE   | \
                                     FILE_ACC_TYPE_VISIBLE    \
                                    )
#define FILE_ACC_TYPE_FULL_EXCHANGE  (FILE_ACC_TYPE_FULL | FILE_ACC_TYPE_EXCHANGE)

#pragma pack(push)
#pragma pack(1)
typedef struct _FileInfo
{
  ULONG    dwAccessType;

  ULONG    dwSizeAllNamesArea;         // length ALL szNames by bytes with zero end
                                    
  ULONG    dwOffsetToAnsiName;         // offset by bytes from szNames to szName (ASCII) 
  ULONG    dwSizeAnsiName;             // length ASCII szName by bytes with zero end

  ULONG    dwOffsetToUniName;          // offset by bytes from szNames to wszName (UNICODE)
  ULONG    dwSizeUniName;              // length UNICODE wszName by bytes with zero end

  ULONG    dwOffsetToAnsiChangedName;  // offset by bytes from szNames to szChangedName
  ULONG    dwSizeAnsiChangedName;      // length ASCII szChangedName by bytes with zero end

  ULONG    dwOffsetToUniChangedName;   // offset by bytes from szNames to wszChangedName
  ULONG    dwSizeUniChangedName;       // length UNICODE wszChangedName by bytes with zero end

  CHAR     szNames[1];
} FILEINFO, *PFILEINFO;
#pragma pack(pop)

#define SIZEOF_FILEINFO        (sizeof(FILEINFO))
#define SIZEOF_FILEINFO_REAL   (SIZEOF_FILEINFO-sizeof(CHAR))

#pragma pack(push)
#pragma pack(1)
typedef struct _FileInfoSet
{
  ULONG     dwSize;       // size of FILEINFOSET by bytes
  FILEINFO  FileInfo[1];
} FILEINFOSET, *PFILEINFOSET;
#pragma pack(pop)

#define SIZEOF_FILEINFOSET   (sizeof(FILEINFOSET))

//
// Key information structure
//
#define KEY_LONG_FORMAT              0x00000001
#define KEY_SHORT_FORMAT             0x00000002
#define KEY_SUBKEY                   0x00000004
#define KEY_VALUE                    0x00000008

#define KEY_SUBKEY_LONG_FORMAT       (KEY_SUBKEY | KEY_LONG_FORMAT)
#define KEY_SUBKEY_SHORT_FORMAT      (KEY_SUBKEY | KEY_SHORT_FORMAT)

#define KEY_VALUE_LONG_FORMAT        (KEY_VALUE | KEY_LONG_FORMAT)
#define KEY_VALUE_SHORT_FORMAT       (KEY_VALUE | KEY_SHORT_FORMAT)

#pragma pack(push)
#pragma pack(1)
typedef struct _KeyInfo
{
  ULONG    dwType;
  ULONG    dwSizeName;  // length szName by bytes with zero end
  CHAR     szName[1];
} KEYINFO, *PKEYINFO;
#pragma pack(pop)

#define SIZEOF_KEYINFO   (sizeof(KEYINFO))

#pragma pack(push)
#pragma pack(1)
typedef struct _KeyInfoSet
{
  int      dwSize;         // size of KEYINFOSET by bytes
  KEYINFO  KeyInfo[1];
} KEYINFOSET, *PKEYINFOSET;
#pragma pack(pop)

#define SIZEOF_KEYINFOSET   (sizeof(KEYINFOSET))

#define HE4_UNLOCK_READ           ACC_TYPE_READ    // 0x00000001
#define HE4_UNLOCK_WRITE          ACC_TYPE_WRITE   // 0x00000002
#define HE4_UNLOCK_DELETE         ACC_TYPE_DELETE  // 0x00000004
#define HE4_UNLOCK_VISIBLE        ACC_TYPE_VISIBLE // 0x00000008
#define HE4_UNLOCK_FOR_PROCESS    0x10000000

#define HE4_UNLOCK_CURRENT_THREAD  0xffffffff
#define HE4_UNLOCK_CURRENT_PROCESS 0xffffffff
#define HE4_UNLOCK_CURRENT_CLIENT  0xffffffff

#pragma pack(push)
#pragma pack(1)
typedef struct
{
  ULONG  m_dwClientId; 
  ULONG  m_dwUnlockFlags; 
} HE4_UNLOCK_SETTING, *PHE4_UNLOCK_SETTING;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct
{
  ULONG  m_dwClientId; 
  ULONG  m_dwForProcess; // if m_dwForProcess == 0 then m_dwClientId is ThreadId else m_dwClientId is ProcessId
} HE4_LOCK_SETTING, *PHE4_LOCK_SETTING;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct
{
  ULONG     m_dwUnlockFlags;
  ULONG     m_dwClientId; 
  PVOID     m_pCCB; 
} UNLOCK_CLIENT_INFO, *PUNLOCK_CLIENT_INFO; 

typedef struct
{
  ULONG               m_dwSize;      // size of UNLOCK_THREAD_INFO_SET by bytes
  UNLOCK_CLIENT_INFO  m_CI[1]; 
} UNLOCK_CLIENT_INFO_SET, *PUNLOCK_CLIENT_INFO_SET; 
#pragma pack(pop)

#define SIZEOF_UNLOCK_CLIENT_INFO_SET   (sizeof(UNLOCK_CLIENT_INFO_SET))

#pragma pack(push)
#pragma pack(1)
typedef struct tag_HEAP_INFO
{
  ULONG       m_dwSystemMemoryUsage;
  ULONG       m_dwHeapMemoryUsage;
} HEAP_INFO, *PHEAP_INFO;

typedef struct tag_HEAP_INFO_SET
{
  HEAP_INFO   m_DefaultHeapInfo;
  HEAP_INFO   m_UnlockListHeapInfo;
  HEAP_INFO   m_FSDefaultHeapInfo;
  HEAP_INFO   m_SOFileListHeapInfo;
  HEAP_INFO   m_LLDefaultHeapInfo;
  HEAP_INFO   m_MiscDefaultHeapInfo;
  HEAP_INFO   m_DHDefaultHeapInfo;
//  HEAP_INFO   m_BTreeDefaultHeapInfo;
} HEAP_INFO_SET, *PHEAP_INFO_SET;

typedef struct tag_HE4_STATISTIC_INFO
{
  HEAP_INFO_SET m_HeapInfoSet;
} HE4_STATISTIC_INFO, *PHE4_STATISTIC_INFO;
#pragma pack(pop)

#endif //__HE4_HOOK_INV_DEVICE_COMMAND
