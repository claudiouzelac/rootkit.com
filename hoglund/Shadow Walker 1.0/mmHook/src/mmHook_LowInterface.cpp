
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//////////////////// LOW LEVEL INTERFACE FUNCTIONS ///////////////////////////
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
extern "C"
{
	#include "ntddk.h"
}

#include "mmHook.h"
#include "module.h"
#include "idtHook.h"

unsigned long g_OldInt0EHandler = 0;
bool hooked = false;

/**********************************************************************************
* HookMemoryPage - Hooks a memory page by marking it not present
*				   and flushing any entries in the TLB. This ensure
*				   that all subsequent memory accesses will generate
*				   page faults and be filtered by the page fault handler.
*
* Parameters - 
*			  PVOID pExecutePage - pointer to the page that will be used to
*								   on execute access.
*			  PVOID pReadWritePage - pointer to the page that will be used
*								   to load the DTLB on data access.
*			  PVOID pfnCallIntoHookedPage - A void function which will be
*											called from within the page fault handler
*											to load the ITLB on execute accesses.
*			  PVOID pDriverStarts (optional) - Sets the start of the valid range for data
*											   accesses originating from within the hidden page.											   
*			  PVOID pDriverEnds (optional) - Sets the end of the valid range for data
*											 accesses originating from within the hidden page.
* Return - None
***********************************************************************************/
void HookMemoryPage( PVOID pExecutePage, PVOID pReadWritePage, PVOID pfnCallIntoHookedPage, PVOID pDriverStarts, PVOID pDriverEnds )
{	
	HOOKED_LIST_ENTRY HookedPage = {0};
	
	HookedPage.pExecuteView = pExecutePage;
	HookedPage.pReadWriteView = pReadWritePage;
	HookedPage.pfnCallIntoHookedPage = pfnCallIntoHookedPage;
	if( pDriverStarts != NULL)
		HookedPage.pDriverStarts = (ULONG)pDriverStarts;
	else HookedPage.pDriverStarts = (ULONG)pExecutePage;  //set by default if pDriverStart is not specified

	if( pDriverEnds != NULL)
		HookedPage.pDriverEnds = (ULONG)pDriverEnds;
	else
	{	//set by default if pDriverEnds is not specified
		if( IsInLargePage( pExecutePage ) )
			HookedPage.pDriverEnds = (ULONG)HookedPage.pDriverStarts + LARGE_PAGE_SIZE;
		else
			HookedPage.pDriverEnds = (ULONG)HookedPage.pDriverStarts + PAGE_SIZE;
	}//end if

	__asm cli //disable interrupts
	
	if( hooked == false )
	{
		HookInt( &g_OldInt0EHandler, (unsigned long)NewInt0EHandler, 0x0E );
		hooked = true;
	}//end if

	HookedPage.pExecutePte = GetPteAddress( pExecutePage );
	HookedPage.pReadWritePte = GetPteAddress( pReadWritePage );

	//Insert the hooked page into the list
	PushPageIntoHookedList( HookedPage );
		
	//Enable the global page feature to improve performance
	EnableGlobalPageFeature( HookedPage.pExecutePte );

	//Mark the page non present
	MarkPageNotPresent( HookedPage.pExecutePte );

	//Go ahead and flush the TLBs.  We want to guarantee that all subsequent accesses to 
	//this hooked page are filtered through our new page fault handler.
	__asm invlpg pExecutePage

	__asm sti //reenable interrupts

}//end HookMemoryPage



/*********************************************************************************
* NewInt0EHandler - Page fault handler for the memory hook engine (aka. the
*				    guts of this whole thing ;)
*
* Parameters - none
*
* Return -	none
*
**********************************************************************************/
void __declspec( naked ) NewInt0EHandler(void)
{	
	__asm
	{
		pushad
		mov edx, dword ptr [esp+0x20] //PageFault.ErrorCode

		test edx, 0x04 //if the processor was in user mode, pass it down
		jnz PassDown
	
		mov eax,cr2     //faulting virtual address
		cmp eax, HIGHEST_USER_ADDRESS
		jbe PassDown		//we don't hook user addresses, pass it down

		////////////////////////////////////////
		//Determine if it's a hooked page
		/////////////////////////////////////////
		push eax
		call FindPageInHookedList
		mov ebp, eax //pointer to HOOKED_PAGE structure stored in list of hooked pages
		cmp ebp, ERROR_PAGE_NOT_IN_LIST
		jz PassDown  //it's not a hooked page

		///////////////////////////////////////
		//NOTE: At this point we know it's a 
		//hooked page. We also only hook
		//kernel mode pages which are either
		//non paged or locked down in memory
		//so we assume that all page tables
		//are resident to resolve the address
		//from here on out.
		/////////////////////////////////////
		mov eax, cr2
		mov esi, PROCESS_PAGE_DIR_BASE
		mov ebx, eax
		shr ebx, 22
		lea ebx, [esi + ebx*4]	//ebx = pPTE for large page
		test [ebx], 0x80        //check if its a large page, for large pages the PDE contains the physical page frame address
		jnz IsLargePage	

		mov esi, PROCESS_PAGE_TABLE_BASE
		mov ebx, eax
		shr ebx, 12
		lea ebx, [esi + ebx*4]	//ebx = pPTE

IsLargePage:
		
		cmp [esp+0x24], eax	//Is it due to an attepmted execute
		jne LoadDTLB

		////////////////////////////////
		// It's due to an execute. Load
		// up the ITLB.
		///////////////////////////////
		cli							//preserve reentrancy
		or dword ptr [ebx], 0x01	//mark the page present
		call [ebp].pfnCallIntoHookedPage	//load the itlb
		and dword ptr [ebx], 0xFFFFFFFE	//mark the page not present
		sti
		jmp ReturnWithoutPassdown

		////////////////////////////////
		// It's due to a read /write
		// Load up the DTLB 
		///////////////////////////////
		///////////////////////////////
		// Check if the read / write
		// is originating from code
		// on the hidden page.
		///////////////////////////////
LoadDTLB:
		mov edx, [esp+0x24]		//eip
		cmp edx,[ebp].pDriverStarts 
		jb LoadFakeFrame
		cmp edx,[ebp].pDriverEnds
		ja LoadFakeFrame

		/////////////////////////////////
		// If the read /write is originating
		// from code on the hidden page,then 
		// let it go through. The code on the 
		// hidden  page will follow protocol 
		// to clear the TLB after the access.
		////////////////////////////////
		cli
		or dword ptr [ebx], 0x01	   //mark the page present
		mov eax, dword ptr [eax]	   //load the dtlb	
		and dword ptr [ebx], 0xFFFFFFFE //mark the page not present
		sti
		jmp ReturnWithoutPassdown 

		/////////////////////////////////
		// We want to fake out this read
		// write. Our code is not generating
		// it.
		/////////////////////////////////
LoadFakeFrame:
		mov esi, [ebp].pReadWritePte
		mov ecx, dword ptr [esi]		//ecx = PTE of the unpatched page
		
		//First, replace the PTE frame of the patched page with that of the unpatched page
		mov edi, [ebx]
		and edi, 0x00000FFF //preserve the lower 12 bits of the faulting patched page's PTE
		and ecx, 0xFFFFF000 //isolate the physical address of the unpatched page's PTE
		or ecx, edi  
		mov edx, [ebx]		//save the old PTE so we can replace it
		cli					//preserve reentrancy
		mov [ebx], ecx		//replace the patched PTE's physical address w/ the unpatched PTE's

		//load the DTLB
		or dword ptr [ebx], 0x01	//mark the page present
		mov eax, cr2				//faulting virtual address
		mov eax, dword ptr[eax]		//do a data access to load up the DTLB
		and dword ptr [ebx], 0xFFFFFFFE	//mark the page not present again

		//Finally, restore the original PTE of the hooked page.
		mov [ebx], edx
		sti

ReturnWithoutPassDown:
		popad
		add esp,4
		iretd

PassDown:
		popad
		jmp g_OldInt0EHandler
	
	}//end asm
}//end NewInt0E 



/***************************************************************
* UnhookMemoryPage - Uhooks a memory page by marking the page
*					 present and uninstalling the page fault handler
*
* Parameters - 
*			   PVOID pExecutePage - Pointer to the base address 
*			   of the hooked page that is going to be unhooked
*
* Return - None
****************************************************************/
void UnhookMemoryPage( PVOID pExecutePage )
{
	__asm cli //disable interrupts
	
	PPTE pExecutePte = GetPteAddress( pExecutePage );
	
	MarkPagePresent( pExecutePte );
	
	__asm invlpg pExecutePage
		
	//unhook the page fault handler
	if( hooked == true )
		UnhookInt( g_OldInt0EHandler, 0x0E );

	__asm sti //reenable interrupts
}//end HookMemoryPage



/**********************************************************************************
* RoundDownToPageBoundary - Rounds a given virtual address down to the starting 
*							address of the page it resides on.
*							
* Parameters:
*		PVOID VirtualAddress - An address within the page we wish to obtain the
*							   address for.
*
* Return - The starting address of the page containing VirtualAddress.
**********************************************************************************/
PVOID RoundDownToPageBoundary( PVOID VirtualAddress )
{
	if( IsInLargePage( VirtualAddress ) )	
		return (PVOID)( ( (ULONG)VirtualAddress >> 22 ) << 22 );
	else  
		return (PVOID)( ( (ULONG)VirtualAddress >> 12 ) << 12 );

}//end RoundDownToPageBoundary



/**********************************************************************************
* RoundUpToPageBoundary - Rounds a given virtual address up to the starting 
*						  address of the next page above the one it resides on.
*
* PVOID VirtualAddress - An address within the page we wish to obtain the
*						 starting address for of the next page above it.
*
* Return - The starting address of the page above the one containing VirtualAddress.
**********************************************************************************/
PVOID RoundUpToPageBoundary( PVOID VirtualAddress )
{
	if( IsInLargePage( VirtualAddress ) )	
		return (PVOID)( ( ( (ULONG)VirtualAddress >> 22 ) << 22 ) + LARGE_PAGE_SIZE );
	else  
		return (PVOID)( ( ( (ULONG)VirtualAddress >> 12 ) << 12 ) + PAGE_SIZE );

}//end RoundUpToPageBoundary



/**********************************************************************************
* GetNumberOfPagesInRange - Returns the number of pages contained within a 
*							specified virtual address range.
*
* Parameters:
*	PVOID LoVirtualAddress - Low virtual address in the range.
*	PVOID HiVirtualAddress - High virtual address in the range.
*
* Return - The number of pages within the range of the given virtual addresses or 
*		   an error code.
*
* Error Codes:
*	ERROR_INVALID_PARAMETER - The low virtual address is not the low virtual address
*							  or the high virtual address is not the high virtual address.
*   ERROR_INCONSISTENT_PAGE_SIZE_ACROSS_RANGE - The specified range contains a mixture
*												of pages sizes.
**********************************************************************************/
int GetNumberOfPagesInRange( PVOID LoVirtualAddress, PVOID HiVirtualAddress )
{
	//make sure both virtual addresses belong to the same page size
	//otherwise return an error
	if( LoVirtualAddress > HiVirtualAddress )
		return ERROR_INVALID_PARAMETER;

	HiVirtualAddress = RoundUpToPageBoundary( HiVirtualAddress );
	LoVirtualAddress = RoundDownToPageBoundary( LoVirtualAddress );

	if(  IsInLargePage( HiVirtualAddress ) &&  IsInLargePage( LoVirtualAddress ) )
	{
		if( ( ( (int)HiVirtualAddress - (int)LoVirtualAddress ) %  LARGE_PAGE_SIZE ) == 0 )
			return ( ( (int)HiVirtualAddress - (int)LoVirtualAddress ) /  LARGE_PAGE_SIZE );
		else
			return ( ( (int)HiVirtualAddress - (int)LoVirtualAddress ) /  LARGE_PAGE_SIZE ) + 1;
	}//end if


	else 
	{
		if( !IsInLargePage( HiVirtualAddress ) && !IsInLargePage( LoVirtualAddress ) )
		{
			if( ( ( (int)HiVirtualAddress - (int)LoVirtualAddress ) %  PAGE_SIZE ) == 0 )
				return ( ( (int)HiVirtualAddress - (int)LoVirtualAddress ) /  PAGE_SIZE );
			else
				return ( ( (int)HiVirtualAddress - (int)LoVirtualAddress ) /  PAGE_SIZE ) + 1;
		}//end if
	}//end if

	return ERROR_INCONSISTENT_PAGE_SIZE_ACROSS_RANGE;

}//end GetNumberOfPagesInRange


/**********************************************************************************
* AllocateViewOfPage - Allocates a page that is equivalent in size to the page
*					   containing VirtualAddress and optionally copies the contents 
*					   of that page to the newly allocated page.
*
* Parameters:
*		PVOID VirtualAddress - An address within the page we wish to allocate
*							   an equivalent size page for.
*		bool DuplicatePageContents - A true false value indicating weather or not
*									 to copy the contents of the page specified
*									 by the VirtualAddress parameter to the new page.
*		ULONG* pAllocatedMemory - Pointer to a variable used to store the address
*								  of the allocated memory block. Note, that this
*								  block may not be aligned to a page boundary, but
*								  this is the address that must be passed into
*								  DeallocateViewOfPage to free the memory.
*
* Return: A pointer to the newly allocated page aligned to a page boundary or NULL 
*		  if the memory request cannot be satisfied.
*
* NOTES: 
* 1. This function can be used to easily duplicate / generate a clean copy
*    of the page containing the virtual address of a routine that we are hiding. 
* 2. If the page is a large page, it is possible that we will not be able to acquire
*    the 8MB of required, continuous, physical non paged memory. The probability
*    of acquiring it would be increased by loading the driver during boot.
* 3. Why do we need 8MB of memory in the first place when we are only duplicating a
*    4MB page? Well, MmAllocateContiguousMemory does not have any way to gurantee
*	 that the allocated memory will be aligned on a 4MB page boundary. In order to
*	 guarantee that we have 1 full, aligned large page, we must actually allocate 2, 
*    4MB pages.
***********************************************************************************/
PVOID AllocateViewOfPage( PVOID VirtualAddress, bool DuplicatePageContents, ULONG* pAllocatedMemory )
{
	PVOID pNewPage = 0;
	PVOID pPage = RoundDownToPageBoundary( VirtualAddress );
	
	if( IsInLargePage( VirtualAddress ) )
	{
		PHYSICAL_ADDRESS HighestAddress = { HIGHEST_ADDRESS_IN_LARGE_PAGE_RANGE, 0 }; 
		pNewPage = MmAllocateContiguousMemory( 2*LARGE_PAGE_SIZE, HighestAddress );
		*pAllocatedMemory = (ULONG)pNewPage;
		if( pNewPage != NULL && DuplicatePageContents == true)	
		{
			pNewPage = RoundUpToPageBoundary( pNewPage );
			RtlCopyMemory( pNewPage, pPage, LARGE_PAGE_SIZE ); //copy the contents of the page containing VirtualAddress to the new page
		}
		return pNewPage;
	
	}//end if
	else
	{
		pNewPage = MmAllocateNonCachedMemory( PAGE_SIZE );
		*pAllocatedMemory = (ULONG)pNewPage;
		if( pNewPage != NULL && DuplicatePageContents == true)	
			RtlCopyMemory( pNewPage, pPage, PAGE_SIZE ); //copy the contents of the page containing VirtualAddress to the new page
		return pNewPage;
	}//end else

}//end AllocateReadWriteViewOfPage


/**********************************************************************************
* DeallocateReadWriteViewOfPage - Frees memory for a page previously allocated with
*								  AllocateViewOfPage.							  
*
* Parameters:
*		PVOID VirtualAddress - Pointer to previously allocated page.
*
* Return - none
***********************************************************************************/
void DeallocateViewOfPage( PVOID VirtualAddress )
{
	if( IsInLargePage( VirtualAddress) )
	{
		return 	MmFreeContiguousMemory( VirtualAddress );
	}//end if
	else
	{
		return MmFreeNonCachedMemory( VirtualAddress, PAGE_SIZE );
	}//end else

}//end AllocateReadWriteViewOfPage

