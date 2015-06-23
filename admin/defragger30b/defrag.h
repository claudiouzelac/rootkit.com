// Defrag.h

//--------------------------------------------------------------------
//                     D E F I N E S 
// File System Control commands related to defragging
#define FSCTL_GET_NTFS_VOLUME_DATA      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 25, METHOD_BUFFERED, FILE_ANY_ACCESS) // NTFS_VOLUME_DATA_BUFFER
#define FSCTL_GET_VOLUME_BITMAP         0x9006F
#define FSCTL_GET_RETRIEVAL_POINTERS    0x90073
#define FSCTL_MOVE_FILE                 0x90074

//--------------------------------------------------------------------
//       F S C T L  S P E C I F I C   T Y P E D E F S  
// This is the definition for a VCN/LCN (virtual cluster/logical cluster)
// mapping pair that is returned in the buffer passed to FSCTL_GET_RETRIEVAL_POINTERS
typedef struct {
    ULONGLONG         Vcn;
    ULONGLONG         Lcn;
} MAPPING_PAIR, *PMAPPING_PAIR;

// This is the definition for the buffer that FSCTL_GET_RETRIEVAL_POINTERS
// returns. It consists of a header followed by mapping pairs
typedef struct {
    ULONG             NumberOfPairs;
    ULONGLONG         StartVcn;
    MAPPING_PAIR      Pair[1];
} GET_RETRIEVAL_DESCRIPTOR, *PGET_RETRIEVAL_DESCRIPTOR;

typedef struct RETRIEVAL_POINTERS_BUFFER {
    DWORD NumberOfPairs;
    LARGE_INTEGER StartVcn;
    struct {
        LARGE_INTEGER    Vcn;
        LARGE_INTEGER    Lcn;
    } Pair[1];
} RETRIEVAL_POINTERS_BUFFER, *PRETRIEVAL_POINTERS_BUFFER;

// This is the definition of the buffer that FSCTL_GET_VOLUME_BITMAP
// returns. It consists of a header followed by the actual bitmap data
typedef struct {
    ULONGLONG         StartLcn;
    ULONGLONG         ClustersToEndOfVol;
    BYTE              Map[1];
} BITMAP_DESCRIPTOR, *PBITMAP_DESCRIPTOR; 

// This is the definition for the data structure that is passed in to
// FSCTL_MOVE_FILE
typedef struct {
     HANDLE            FileHandle; 
     ULONG             Reserved;   
     LARGE_INTEGER     StartVcn; 
     LARGE_INTEGER     TargetLcn;
     ULONG             NumVcns; 
     ULONG             Reserved1;   
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;

typedef struct {
    HANDLE FileHandle;
    LARGE_INTEGER StartingVcn;
    LARGE_INTEGER StartingLcn;
    DWORD ClusterCount;
} MOVE_FILE_DATA, *PMOVE_FILE_DATA;

// protos
void          HandleWin32Error( DWORD, char * );
void          ClearLastSelectedFile(void);
void          DrawProgress(int progress);
void          DrawNoBlockBound(int, int);
void          DrawBlockBound(int, int);
void          DrawBlockBounds(ULONGLONG Cstart, ULONGLONG Cend, BOOL clear);
void          DrawBlockAt(HDC, int, int, HBRUSH);
void          DrawBlocks(ULONGLONG Cstart, ULONGLONG Cend, HBRUSH hb, HPEN hp);
void          DrawClusters(LONGLONG, LONGLONG, HPEN);
void          DrawMarks(ULONGLONG Cstart, ULONGLONG Cend);
void          DrawAllMarks(void *d);
void          DumpBitmap(void);
void          DumpFile(char *filename, __int64 Bytes);
BOOL          IsFileInUse(char *sFile);
ULONGLONG     MoveFileNormal     (struct FILEstruct *Here, ULONGLONG NewLcn);
ULONGLONG     MoveFileFragmented (struct FILEstruct *Here, ULONGLONG *sLcn);
