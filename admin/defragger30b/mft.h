// mft.h

//--------------------------------------------------------------------
//                     D E F I N E S 
// File System Control commands related to MFT
#define FSCTL_GET_NTFS_FILE_RECORD      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 26, METHOD_BUFFERED, FILE_ANY_ACCESS) // NTFS_FILE_RECORD_INPUT_BUFFER, NTFS_FILE_RECORD_OUTPUT_BUFFER
// aka FSCTL_READ_MFT_RECORD           0x90068

//--------------------------------------------------------------------
// two types for calling DeviceIoControl 
// with code FSCTL_GET_NTFS_FILE_RECORD
typedef struct {
    LARGE_INTEGER FileReferenceNumber;
} NTFS_FILE_RECORD_INPUT_BUFFER, *PNTFS_FILE_RECORD_INPUT_BUFFER;

typedef struct {
    LARGE_INTEGER FileReferenceNumber;
    DWORD FileRecordLength;
    BYTE  FileRecordBuffer[1];
} NTFS_FILE_RECORD_OUTPUT_BUFFER, *PNTFS_FILE_RECORD_OUTPUT_BUFFER;

// --- MFT structure ---
// NTFS_RECORD_HEADER
//  |
//  |-- ATTRIBUTE_HDR (of type ATTRIBUTE_TYPE)
//      |
//      |-- RESIDENT_ATTRIBUTE
//      |   |
//      |   |-- STANDARD_INFORMATION
//      |   |-- FILENAME_ATTRIBUTE
//      |   |-- AttributeIndexAllocation
//      |       |-- INDEX_RECORD
//      |           |-- RUN_LIST
//      |               |-- ATTRIBUTE_LIST
//      |                   |-- INDEX_HEADER, *PINDEX_HEADER;
//      |                   |-- INDEX_RECORD, *PINDEX_RECORD;
//      |
//      |-- NONRESIDENT_ATTRIBUTE
//          |   
//          |-- STANDARD_INFORMATION
//          |-- FILENAME_ATTRIBUTE
//          |-- INDEX_ROOT, *PINDEX_ROOT;
//              |-- INDEX_HEADER, *PINDEX_HEADER;
//                  |-- INDEX_RECORD, *PINDEX_RECORD;
//
// --- attributes not used ---
//    AttributeObjectId = 0x00000040,
//    AttributeSecurityDescriptor = 0x00000050,
//    AttributeVolumeName = 0x00000060,
//    AttributeVolumeInformation = 0x00000070,
//    AttributeBitmap = 0x000000B0,
//    AttributeReparsePoint = 0x000000C0,
//    AttributeEAInformation = 0x000000D0,
//    AttributeEA = 0x000000E0,
//    AttributePropertySet = 0x000000F0,
//    AttributeLoggedUtilityStream = 0x00000100,
//    AttributeANY = 0x00000EEE

//--------------------------------------------------------------------
// --- MFT structures ---
typedef struct {
    ULONG Type;             /* Magic number 'FILE0' */
    USHORT UsaOffset;       /* Offset to the update sequence */
    USHORT UsaCount;        /* Size in words of Update Sequence Number & Array (S) */
    ULONGLONG Lsn;          /* $LogFile Sequence Number (LSN) */
} NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER;

typedef struct {
    NTFS_RECORD_HEADER RecHdr;
    USHORT SequenceNumber;        /* Sequence number */
    USHORT LinkCount;             /* Hard link count */
    USHORT AttributeOffset;       /* Offset to the first Attribute */
    USHORT Flags;                 /* Flags */
    ULONG BytesInUse;             /* Real size of the FILE record */
    ULONG BytesAllocated;         /* Allocated size of the FILE record */
    ULONGLONG BaseFileRecord;     /* File reference to the base FILE record */
    USHORT NextAttributeNumber;   /* Next Attribute Id */
    USHORT Pading;                /* Align to 4 byte boundary (XP) */
    ULONG MFTRecordNumber;        /* Number of this MFT Record (XP) */
    USHORT UpdateSeqNum;          /*  */
} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;

typedef enum {
    AttributeStandardInformation = 0x00000010,
    AttributeAttributeList = 0x00000020,
    AttributeFileName = 0x00000030,
    AttributeObjectId = 0x00000040,
    AttributeSecurityDescriptor = 0x00000050,
    AttributeVolumeName = 0x00000060,
    AttributeVolumeInformation = 0x00000070,
    AttributeData = 0x00000080,
    AttributeIndexRoot = 0x00000090,
    AttributeIndexAllocation = 0x000000A0,
    AttributeBitmap = 0x000000B0,
    AttributeReparsePoint = 0x000000C0,
    AttributeEAInformation = 0x000000D0,
    AttributeEA = 0x000000E0,
    AttributePropertySet = 0x000000F0,
    AttributeLoggedUtilityStream = 0x00000100,
    AttributeANY = 0x00000EEE
} ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;

typedef struct {
    ATTRIBUTE_TYPE AttributeType;
    ULONG Length;
    BOOLEAN Nonresident;
    UCHAR NameLength;
    USHORT NameOffset;
    USHORT Flags; // 0x0001 = Compressed
    USHORT AttributeNumber;
} ATTRIBUTE_HDR, *PATTRIBUTE_HDR;

typedef struct {
    ULONG AttrLenth;
    USHORT AttrOffset;
    USHORT Flags; // 0x0001 = Indexed
    WCHAR Name[1];
} RESIDENT_ATTRIBUTE, *PRESIDENT_ATTRIBUTE;

typedef struct {
    ULONGLONG StartVcn;
    ULONGLONG LastVcn;
    USHORT RunArrayOffset;
    UCHAR CompressionUnit;
    ULONG Padding;
    ULONGLONG AllocatedSize;
    ULONGLONG RealSize;
    ULONGLONG InitializedSize;
    WCHAR Name[1];
} NONRESIDENT_ATTRIBUTE, *PNONRESIDENT_ATTRIBUTE;

typedef struct {
    ULONGLONG CreationTime; 
    ULONGLONG ChangeTime;
    ULONGLONG LastWriteTime; 
    ULONGLONG LastAccessTime; 
    ULONG FileAttributes; 
    ULONG AlignmentOrReservedOrUnknown[3];
    ULONG QuotaId; // NTFS 3.0 only
    ULONG SecurityId; // NTFS 3.0 only
    ULONGLONG QuotaCharge; // NTFS 3.0 only
    USN Usn; // NTFS 3.0 only
} STANDARD_INFORMATION, *PSTANDARD_INFORMATION;

typedef struct {
    ATTRIBUTE_TYPE AttributeType;
    USHORT Length;
    UCHAR NameLength;
    UCHAR NameOffset;
    ULONGLONG LowVcn;
    ULONGLONG FileReferenceNumber;
    USHORT AttributeNumber;
    USHORT AlignmentOrReserved[3];
} ATTRIBUTE_LIST, *PATTRIBUTE_LIST;

typedef struct {
    ULONGLONG DirectoryFileReferenceNumber;
    ULONGLONG CreationTime; // Saved when filename last changed
    ULONGLONG ChangeTime; // ditto
    ULONGLONG LastWriteTime; // ditto
    ULONGLONG LastAccessTime; // ditto
    ULONGLONG AllocatedSize; // ditto
    ULONGLONG RealSize; // ditto
    ULONG Flags; // ditto
    ULONG ForEAsReparse;
    UCHAR NameLength;
    UCHAR NameType; // 0x01 = Long(Windows), 0x02 = Short(DOS), 0x03 = both
    WCHAR Name[1];
} FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE;

typedef struct {
    ATTRIBUTE_TYPE Type;
    ULONG CollationRule;
    ULONG SizeOfIndexAllocEntry;
    BYTE ClustersPerIndexRecord;
    BYTE Align[3];
    ULONG FirstEntryOffset;
    ULONG SizeOfIndexEntries;
    ULONG AllocatedSize;
    BYTE Flags; // 0x00 = Small directory, 0x01 = Large directory
    BYTE Align2[3];
    //DIRECTORY_INDEX DirectoryIndex;
} INDEX_ROOT, *PINDEX_ROOT;

typedef struct {
    ULONG IndxID; //'INDX'
	USHORT UpdateSeqOffset;
	USHORT SizeOfUpdateSeq;
	LONGLONG LogFileSeqNum;
	LONGLONG VCN;
	ULONG BytesToIndexEntries;
	ULONG SizeOfIndexEntries;
	ULONG AllocatedSizeForEntries;
	BOOLEAN NotLeafNode;
	BYTE Align[3];
	USHORT UpdateSeq;
} INDEX_HEADER, *PINDEX_HEADER;

typedef struct {
    LONGLONG MFTReference;
	USHORT Size;
	USHORT FilenameOffset;
	USHORT IndexFlags;
	USHORT Align;
	LONGLONG ParentMFTReference;
	LONGLONG TimeCreated;
	LONGLONG TimeAltered;
	LONGLONG TimeMTFAltr;
	LONGLONG TimeLastRead;
	LONGLONG AllocatedSize;
	LONGLONG RealSize;
	LONGLONG Flags;
	UCHAR NameLength;
    UCHAR NameType; // 0x01 = Long(Windows), 0x02 = Short(DOS)
} INDEX_RECORD, *PINDEX_RECORD;

// unofficial structures I dont need
/*
typedef struct {
    ULONGLONG FileReferenceNumber;
    USHORT Length;
    USHORT AttributeLength;
    ULONG Flags; // 0x01 = Has trailing VCN, 0x02 = Last entry
    // FILENAME_ATTRIBUTE Name;
    // ULONGLONG Vcn; // VCN in IndexAllocation of earlier entries
} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;
*/

/*
typedef struct {
    GUID ObjectId;
    union {
        struct {
            GUID BirthVolumeId;
            GUID BirthObjectId;
            GUID DomainId;
        };
        UCHAR ExtendedInfo[48];
    };
} OBJECTID_ATTRIBUTE, *POBJECTID_ATTRIBUTE;

typedef struct {
    ULONG Unknown[2];
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    USHORT Flags;
} VOLUME_INFORMATION, *PVOLUME_INFORMATION;

typedef struct {
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    UCHAR ReparseData[1];
} REPARSE_POINT, *PREPARSE_POINT;

typedef struct {
    ULONG EaLength;
    ULONG EaQueryLength;
} EA_INFORMATION, *PEA_INFORMATION;

typedef struct {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
    // UCHAR EaData[];
} EA_ATTRIBUTE, *PEA_ATTRIBUTE;
*/


// semi-global vars
__int64 MFTentries, cMFTentry;
// proto for public
void          ReadMFTRecord      (LONGLONG MFTid, char *dir);
