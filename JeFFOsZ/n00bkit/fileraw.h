#define FILE_SIGN 0x454C4946 // ELIF
#define INDX_SIGN 0x58444E49
#define DISK_DEVICE L"\\Device\\HarddiskVolume"
#define RVATOVA(base,offset) ((LPVOID)((DWORD)(base)+(DWORD)(offset)))

typedef struct _NTFS_TIMES
{
	LARGE_INTEGER t_create;
	LARGE_INTEGER t_write;
	LARGE_INTEGER t_mftwrite;
	LARGE_INTEGER t_access;
} NTFS_TIMES, *PNTFS_TIMES;

typedef struct _DIRECTORY_INDEX 
{
	ULONG         ie_number;
	ULONG         unknown1;
	USHORT        reclen;
	USHORT        ie_size;
	ULONG         ie_flag;
	ULONG         ie_fpnumber;
	ULONG         unknown2;
	NTFS_TIMES    ie_ftimes;
	LARGE_INTEGER ie_fallocated;
	LARGE_INTEGER ie_fsize;
	LARGE_INTEGER ie_fflag;
	UCHAR         ie_fnamelen;
	UCHAR         ie_fnametype;
	WCHAR         ie_fname[MAX_PATH];
} DIRECTORY_INDEX, *PDIRECTORY_INDEX;

typedef struct _FIXUP_HDR 
{
	ULONG  fh_magic;
	USHORT fh_foff;
	USHORT fh_fnum;
} FIXUP_HDR, *PFIXUP_HDR;

typedef struct _FILE_RECORD 
{
	FIXUP_HDR     fr_fixup;
	UCHAR         reserved[8];
	USHORT        fr_seqnum;
	USHORT        fr_nlink;
	USHORT        fr_attroff;
	USHORT        fr_flags;	
	ULONG         fr_size;
	ULONG         fr_allocated;
	LARGE_INTEGER fr_mainrec;
	USHORT        fr_attrnum;
} FILE_RECORD, *PFILE_RECORD;

typedef struct _FAT32_ENTRY
{
	UCHAR sec_per_clas;
	ULONG cluster_begin_lba;
	ULONG root_dir;
	ULONG fat_begin_lba;
	ULONG filenumber;
	UCHAR strings[7];
	UCHAR no_of_strings;
} FAT32_ENTRY, *PFAT32_ENTRY;
