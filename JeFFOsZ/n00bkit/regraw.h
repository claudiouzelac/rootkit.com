#include <stdio.h>
#include <windows.h>

// Here are some structs from EiNSTeiN's posts. Forward declare it all first.

typedef struct _REGF_BLOCK  REGF_BLOCK,   *PREGF_BLOCK;
typedef struct _HBIN_BLOCK  HBIN_BLOCK,   *PHBIN_BLOCK;
typedef struct _NK_RECORD   NK_RECORD,    *PNK_RECORD;
typedef struct _LF_RECORD   LF_RECORD,    *PLF_RECORD;
typedef struct _LI_RECORD   LI_RECORD,    *PLI_RECORD;
typedef struct _RI_RECORD   RI_RECORD,    *PRI_RECORD;
typedef struct _INDEX_RECORD  INDEX_RECORD, *PINDEX_RECORD;
typedef struct _LF_ENTRY    LF_ENTRY,     *PLF_ENTRY;
typedef struct _VALUE_LIST  VALUE_LIST,   *PVALUE_LIST;
typedef struct _VK_RECORD   VK_RECORD,    *PVK_RECORD;
typedef struct _DATA_RECORD DATA_RECORD,  *PDATA_RECORD;
typedef struct _SK_RECORD   SK_RECORD,    *PSK_RECORD;

#define QWORD __int64

#pragma pack(4)
typedef struct _NK_RECORD {
/*000*/ DWORD         RecordSize; //stored in negative form, see the REALSIZE macro
/*004*/ WORD          Header; //should be nk (0x6B6E)
/*006*/ WORD          KeyFlag; // 2Ch for the root key (the first key after the first header), for all other 20h
/*008*/ QWORD         LastModifiedDate;
/*010*/ DWORD         unk10;
/*014*/ PNK_RECORD    ParentKey_offset; //relative to the first hbin block
/*018*/ DWORD         NumberOfSubKeys; //number of key in the lf-record
/*01C*/ DWORD         unk1C;
/*020*/ union {
/*020*/   PINDEX_RECORD IndexRecord_offset;  //relative to the first hbin block
/*020*/   PLF_RECORD    LfRecord_offset;  //relative to the first hbin block
/*020*/   PLI_RECORD    LiRecord_offset;  //relative to the first hbin block
/*020*/   PRI_RECORD    RiRecord_offset;  //relative to the first hbin block
/*020*/ };
/*024*/ DWORD         unk24;
/*028*/ DWORD         NumberOfValues; //number of values in the value-list
/*02C*/ PVALUE_LIST   ValueList_offset; //relative to the first hbin block
/*030*/ PSK_RECORD    SkRecord_offset; //relative to the first hbin block
/*034*/ PDATA_RECORD  ClassName_offset; //relative to the first hbin block
/*038*/ DWORD         unk38;
/*03C*/ DWORD         unk3C;
/*040*/ DWORD         unk40;
/*044*/ DWORD         unk44;
/*048*/ DWORD         unk48;
/*04C*/ WORD          NameLength;
/*04E*/ WORD          ClassNameLength;
/*050*/ WCHAR         Name[1];    //see NameLength to know the size
                      //the name is stored in ASCII form but doesn't alway terminate by 0x00
} NK_RECORD, *PNK_RECORD;
#pragma pack()

//the size of this block is 20h and is followed by the hive root-key
#pragma pack(4)
typedef struct _HBIN_BLOCK {
/*000*/ DWORD        Header;    //must be hbin (0x6E696268)
/*004*/ DWORD        ToPreviousBlock; //offset of this block - this value = offset of the previous block, should be 0 for the first block
/*008*/ DWORD        ToNextBlock; //offset to the next block, should be 0 for the last block
/*00C*/ DWORD        unkC;
/*010*/ DWORD        unk10;
/*014*/ DWORD        unk14;
/*018*/ DWORD        unk18;
/*01c*/ DWORD        unk1c;
/*020*/ NK_RECORD    RootNkRecord;
} HBIN_BLOCK, *PHBIN_BLOCK;
#pragma pack()

#pragma pack(4)
//the size of this block is alway 1000h (4096.) and is followed by a HBIN block
typedef struct _REGF_BLOCK {
/*0000*/ DWORD        Header; //must be regf (0x66676572)
/*0004*/ DWORD        unk4;
/*0008*/ DWORD        unk8;
/*000C*/ QWORD        LastModifiedDate;
/*0010*/ /*DWORD        unk10; no this is part of previous qword! */
/*0014*/ DWORD        dw14; //alway 1
/*0018*/ DWORD        dw18; //alway 3
/*001C*/ DWORD        dw1C; //alway 0
/*0020*/ DWORD        dw20; //alway 1
/*0024*/ DWORD        FirstKey; //offset of first key (must be 20h)
/*0028*/ DWORD        DataBlocksSize; // size of the file - size of this block
/*002C*/ char         Padding[4052]; //the rest is not important
/*1000*/ HBIN_BLOCK   FirstHbinBlock;
} REGF_BLOCK, *PREGF_BLOCK;
#pragma pack()

typedef struct _LF_ENTRY {
/*000*/ PNK_RECORD    NkRecord_offset; //relative to the first hbin block
/*004*/ union {
/*004*/   CHAR          NkRecord_hint[4]; //this is the 4 first letters of the key name
/*004*/   DWORD         Checksum; //this is the 4 first letters of the key name
/*004*/ };
} LF_ENTRY, *PLF_ENTRY;

typedef struct _LF_RECORD {
/*000*/ DWORD         RecordSize; //stored in negative form, see the REALSIZE macro
/*004*/ WORD          Header; //must be lf (0x666C)
/*006*/ WORD          NumberOfKeys; //number of keys in the record
/*008*/ LF_ENTRY      LfEntrys[1]; //this array have a size of 'NumberOfKeys'
} LF_RECORD, *PLF_RECORD;

typedef struct _LI_RECORD {
/*000*/ DWORD         RecordSize; //stored in negative form, see the REALSIZE macro
/*004*/ WORD          Header; //must be li (0x696C)
/*006*/ WORD          NumberOfKeys; //number of keys in the record
/*008*/ PNK_RECORD    LiEntrys[1]; //this array have a size of 'NumberOfKeys'
} LI_RECORD, *PLI_RECORD;

// An RI record is a list of offsets to LI or LF indexes when a
// key has so many subkeys that it needs two layers of indexing.
typedef struct _RI_RECORD {
/*000*/ DWORD         RecordSize; //stored in negative form, see the REALSIZE macro
/*004*/ WORD          Header; //must be ri (0x6972)
/*006*/ WORD          NumberOfRecords; //number of keys in the record
/*008*/ union {
/*008*/   PINDEX_RECORD IndexEntrys[1]; //this array have a size of 'NumberOfRecords'
/*008*/   PLI_RECORD    LiEntrys[1]; //this array have a size of 'NumberOfRecords'
/*008*/   PLF_RECORD    LfEntrys[1]; //this array have a size of 'NumberOfRecords'
/*008*/ };
} RI_RECORD, *PRI_RECORD;

// This represents the common part of all three index record types. Although
// we don't strictly need it, it makes the code nicer and more robust.
typedef struct _INDEX_RECORD {
/*000*/ DWORD         RecordSize; //stored in negative form, see the REALSIZE macro
/*004*/ WORD          Header; //must be li (0x696C), ri (0x6972), or lf (0x666C)
/*006*/ WORD          NumberOfItems; //number of keys or subrecords in the record
} INDEX_RECORD, *PINDEX_RECORD;

typedef struct _VALUE_LIST {
/*000*/ DWORD         ListSize; //stored in negative form, see the REALSIZE macro
/*004*/ PVK_RECORD    VkRecords_offset[1]; //offset relative to the first hbin block
                      //this array has the size of NumberOfValues field in the owner-nk record
} VALUE_LIST, *PVALUE_LIST;

//if the size of value is lower or equal to a DWORD size
//the data is stored in the DataRecord_offset field itself
//instead of an offset, and bit31 of length is set to 1.
typedef struct _VK_RECORD {
/*000*/ DWORD         RecordSize; //stored in negative form, see the REALSIZE macro
/*004*/ WORD          Header; //must be vk (0x6B76)
/*006*/ WORD          NameLength; //length of the Name[] field
/*008*/ DWORD         DataLength; //length of the data in the data block
/*00C*/ PDATA_RECORD  DataRecord_offset; //offset relative to the first hbin block
/*010*/ DWORD         ValueType; //see the comments below
/*014*/ WORD          Flag; 
/*016*/ WORD          unk16;
/*018*/ WCHAR         Name[1]; //stored in UNICODE or ASCII form but doesn't alway terminate by 0x00
} VK_RECORD, *PVK_RECORD;

typedef struct _DATA_RECORD {
/*000*/ DWORD         RecordSize; //stored in negative form, see the REALSIZE macro
/*004*/ BYTE          Data[1]; //this field has a size of DataLength of the owner-vk record
} DATA_RECORD, *PDATA_RECORD;

typedef struct _SK_RECORD {
/*000*/ DWORD         RecordSize; //stored in negative form, see the REALSIZE macro
/*004*/ WORD          Header; //must be sk (0x6B73)
/*006*/ WORD          unk6;
/*008*/ PSK_RECORD    LastRecord_offset; //relative to the first hbin block
/*00C*/ PSK_RECORD    NextRecord_offset; //relative to the first hbin block
/*010*/ DWORD         UsageCounter; //number of key by wich this block is referenced
/*014*/ DWORD         SizeOfSettings; //size of the following field
/*018*/ BYTE          Settings[1];
} SK_RECORD, *PSK_RECORD;

//REALSIZE - return a positive size from a negative one
#define REALSIZE(x)               ((~(x)) + 1)
#define VALUE_DATA_IN_OFFSET_FLAG (0x80000000)
#define DATA_IN_VK_RECORD(vk)     ((vk)->DataLength & VALUE_DATA_IN_OFFSET_FLAG)
#define REAL_VK_DATA_SIZE(vk)     ((vk)->DataLength & ~VALUE_DATA_IN_OFFSET_FLAG)
#define HBIN_REL(type, offs)      ((type *)(((DWORD)(offs)) + (DWORD)hbin))
#define VALID_HBIN_OFFSET(offs)   ((offs) && ~(DWORD)(offs))
#define HBIN_REL_SIZECHECK(addr)  ((((DWORD)(addr)) - ((DWORD)data)) >= total_size)
#define HBIN_OFFS(ptr)            (((DWORD)(ptr)) - ((DWORD)hbin))

typedef int (*PCLEAN_NAME_TEST)(const void *data, int length);

static void WalkKeyTree (PNK_RECORD,PHBIN_BLOCK,PCLEAN_NAME_TEST,char);
void CleanRegistryFile (void*,PCLEAN_NAME_TEST);
static void WalkHBinRootkey(PHBIN_BLOCK,PHBIN_BLOCK,PCLEAN_NAME_TEST);
static void CleanNK(PNK_RECORD,PHBIN_BLOCK);
static void CleanSK(PSK_RECORD,PHBIN_BLOCK);
static void CleanVK (PVK_RECORD,PHBIN_BLOCK);
static __inline void CleanRI(PRI_RECORD);
static __inline void CleanLF(PLF_RECORD);
static __inline void CleanLI(PLI_RECORD);
static __inline void CleanVL (PVALUE_LIST);
static __inline void CleanDataRecord(PDATA_RECORD);