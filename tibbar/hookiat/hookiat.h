typedef enum _SYSTEM_INFORMATION_CLASS {
SystemBasicInformation, // 0 Y N
SystemProcessorInformation, // 1 Y N
SystemPerformanceInformation, // 2 Y N
SystemTimeOfDayInformation, // 3 Y N
SystemNotImplemented1, // 4 Y N
SystemProcessesAndThreadsInformation, // 5 Y N
SystemCallCounts, // 6 Y N
SystemConfigurationInformation, // 7 Y N
SystemProcessorTimes, // 8 Y N
SystemGlobalFlag, // 9 Y Y
SystemNotImplemented2, // 10 Y N
SystemModuleInformation, // 11 Y N
SystemLockInformation, // 12 Y N
SystemNotImplemented3, // 13 Y N
SystemNotImplemented4, // 14 Y N
SystemNotImplemented5, // 15 Y N
SystemHandleInformation, // 16 Y N
SystemObjectInformation, // 17 Y N
SystemPagefileInformation, // 18 Y N
SystemInstructionEmulationCounts, // 19 Y N
SystemInvalidInfoClass1, // 20
SystemCacheInformation, // 21 Y Y
SystemPoolTagInformation, // 22 Y N
SystemProcessorStatistics, // 23 Y N
SystemDpcInformation, // 24 Y Y
SystemNotImplemented6, // 25 Y N
SystemLoadImage, // 26 N Y
SystemUnloadImage, // 27 N Y
SystemTimeAdjustment, // 28 Y Y
SystemNotImplemented7, // 29 Y N
SystemNotImplemented8, // 30 Y N
SystemNotImplemented9, // 31 Y N
SystemCrashDumpInformation, // 32 Y N
SystemExceptionInformation, // 33 Y N
SystemCrashDumpStateInformation, // 34 Y Y/N
SystemKernelDebuggerInformation, // 35 Y N
SystemContextSwitchInformation, // 36 Y N
SystemRegistryQuotaInformation, // 37 Y Y
SystemLoadAndCallImage, // 38 N Y
SystemPrioritySeparation, // 39 N Y
SystemNotImplemented10, // 40 Y N
SystemNotImplemented11, // 41 Y N
SystemInvalidInfoClass2, // 42
SystemInvalidInfoClass3, // 43
SystemTimeZoneInformation, // 44 Y N
SystemLookasideInformation, // 45 Y N
SystemSetTimeSlipEvent, // 46 N Y
SystemCreateSession, // 47 N Y
SystemDeleteSession, // 48 N Y
SystemInvalidInfoClass4, // 49
SystemRangeStartInformation, // 50 Y N
SystemVerifierInformation, // 51 Y Y
SystemAddVerifier, // 52 N Y
SystemSessionProcessesInformation // 53 Y N
} SYSTEM_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
IN OUT PVOID SystemInformation,
IN ULONG SystemInformationLength,
OUT PULONG ReturnLength OPTIONAL
);

typedef struct _SYSTEM_MODULE_INFORMATION { // Information Class 11
ULONG Reserved[2];
PVOID Base;
ULONG Size;
ULONG Flags;
USHORT Index;
USHORT Unknown;
USHORT LoadCount;
USHORT ModuleNameOffset;
CHAR ImageName[256];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef unsigned short      			WORD;
typedef unsigned char BYTE;
typedef unsigned long DWORD;

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
  } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;


typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

typedef struct _IMAGE_OPTIONAL_HEADER {
    //
    // Standard fields.
    //

    WORD    Magic;
    BYTE    MajorLinkerVersion;
    BYTE    MinorLinkerVersion;
    DWORD   SizeOfCode;
    DWORD   SizeOfInitializedData;
    DWORD   SizeOfUninitializedData;
    DWORD   AddressOfEntryPoint;
    DWORD   BaseOfCode;
    DWORD   BaseOfData;

    //
    // NT additional fields.
    //

    DWORD   ImageBase;
    DWORD   SectionAlignment;
    DWORD   FileAlignment;
    WORD    MajorOperatingSystemVersion;
    WORD    MinorOperatingSystemVersion;
    WORD    MajorImageVersion;
    WORD    MinorImageVersion;
    WORD    MajorSubsystemVersion;
    WORD    MinorSubsystemVersion;
    DWORD   Win32VersionValue;
    DWORD   SizeOfImage;
    DWORD   SizeOfHeaders;
    DWORD   CheckSum;
    WORD    Subsystem;
    WORD    DllCharacteristics;
    DWORD   SizeOfStackReserve;
    DWORD   SizeOfStackCommit;
    DWORD   SizeOfHeapReserve;
    DWORD   SizeOfHeapCommit;
    DWORD   LoaderFlags;
    DWORD   NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef IMAGE_OPTIONAL_HEADER32             IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER32            PIMAGE_OPTIONAL_HEADER;

typedef struct _IMAGE_THUNK_DATA32 {
    union {
        DWORD ForwarderString;      // PBYTE 
        DWORD Function;             // PDWORD
        DWORD Ordinal;
        DWORD AddressOfData;        // PIMAGE_IMPORT_BY_NAME
    } u1;
} IMAGE_THUNK_DATA32;
typedef IMAGE_THUNK_DATA32 * PIMAGE_THUNK_DATA32;
typedef IMAGE_THUNK_DATA32              IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA32             PIMAGE_THUNK_DATA;

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        DWORD   Characteristics;            // 0 for terminating null import descriptor
        DWORD   OriginalFirstThunk;         // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
    };
    DWORD   TimeDateStamp;                  // 0 if not bound,
                                            // -1 if bound, and real date\time stamp
                                            //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                            // O.W. date/time stamp of DLL bound to (Old BIND)

    DWORD   ForwarderChain;                 // -1 if no forwarders
    DWORD   Name;
    DWORD   FirstThunk;                     // RVA to IAT (if bound this IAT has actual addresses)
} IMAGE_IMPORT_DESCRIPTOR;
typedef IMAGE_IMPORT_DESCRIPTOR UNALIGNED *PIMAGE_IMPORT_DESCRIPTOR;


#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory

typedef unsigned char *PBYTE;

typedef struct _GENERATE_NAME_CONTEXT {
    USHORT  Checksum;
    BOOLEAN CheckSumInserted;
    UCHAR   NameLength;
    WCHAR   NameBuffer[8];
    ULONG   ExtensionLength;
    WCHAR   ExtensionBuffer[4];
    ULONG   LastIndexValue;
} GENERATE_NAME_CONTEXT, *PGENERATE_NAME_CONTEXT;


typedef struct _IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define SEC_IMAGE	0x01000000
#define SEC_BASED 0x00200000
typedef struct _SECTION_IMAGE_INFORMATION {
  PVOID                   EntryPoint;
  ULONG                   StackZeroBits;
  ULONG                   StackReserved;
  ULONG                   StackCommit;
  ULONG                   ImageSubsystem;
  WORD                    SubsystemVersionLow;
  WORD                    SubsystemVersionHigh;
  ULONG                   Unknown1;
  ULONG                   ImageCharacteristics;
  ULONG                   ImageMachineType;
  ULONG                   Unknown2[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;


NTSYSAPI
NTSTATUS
NTAPI
  ZwCreateSection(
    OUT PHANDLE  SectionHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER  MaximumSize OPTIONAL,
    IN ULONG  SectionPageProtection,
    IN ULONG  AllocationAttributes,
    IN HANDLE  FileHandle OPTIONAL
    ); 
