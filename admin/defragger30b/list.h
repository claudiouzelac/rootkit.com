// list.h - declares the dirlist and sorting constants

unsigned long listlen;
unsigned long mftlistlen;

// codes for sorting the list
#define byName        1
#define byLcnAsc      2 
#define byLcnDesc     3
#define byFragments   4
#define bySizeAsc     5
#define bySizeDesc    6

struct FILEstruct {
    char *fileName;
    __int64 filesize;
    __int64 MFTid;
    __int64 fragid;
    ULONGLONG fragments;
	ULONGLONG Lcn;
	ULONGLONG Len;
	ULONGLONG Vcn;
    BOOL free;
    BOOL fragmented;
    BOOL compressed;
    BOOL sparse;
    BOOL locked;
	struct FILEstruct *next;
	struct FILEstruct *prev;
};

struct MFTstruct {
    char *fileName;
    __int64 filesize;
    __int64 fragid;
    __int64 MFTid;
    ULONGLONG fragments;
	ULONGLONG Lcn;
	ULONGLONG Len;
	ULONGLONG Vcn;
    BOOL fragmented;
	struct MFTstruct *next;
};

struct FILEstruct *InsDIR(char *name, __int64 size, ULONGLONG Lcn, ULONGLONG Len, ULONGLONG Vcn, ULONGLONG fragments, ULONGLONG fragid,
						  BOOL free, BOOL fragmented, BOOL compressed, BOOL sparse, BOOL locked, __int64 MFTid);
struct MFTstruct *InsMFT(char *name, __int64 size, ULONGLONG Lcn, ULONGLONG Len, ULONGLONG Vcn, ULONGLONG fragments, ULONGLONG fragid,
						  BOOL fragmented, BOOL locked, __int64 MFTid);
void   DelDIR(struct FILEstruct *);
void   DelDIRmft(struct MFTstruct *top);
struct FILEstruct *CopyDIR(struct FILEstruct *top);
struct FILEstruct *SortDict (register struct FILEstruct *chain3, 
                             long int listlength, 
                             int sortItem);
