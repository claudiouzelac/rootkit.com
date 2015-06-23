
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
/////////////////////////////// SUPPORT FUNCTIONS ////////////////////////////
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
extern "C"
{
	#include "ntddk.h"
}

#include "mmHook.h"

HOOKED_LIST_ENTRY* g_HookedPageList[MAX_NUMBER_OF_HOOKED_PAGES] = {0}; //list of hooked pages



/*********************************************************************************
* InsertPageIntoHookedList - Implements a rudimentary hash table. Insertion into the
*							 table of hooked pages is done using the virtual address.
*							 Each entry in the hash table points to a singly linked
*							 list of HOOKED_LIST_ENTRY structures. 
*
* Parameters - 
*			 HOOKED_LIST_ENTRY HookedPage - A filled in structure with information
*											about the hooked page. Used by the page
*											fault handler.
*
* Return -	NONE
*
* NOTE: This scheme only works for kernel memory (upper 2 GB) which is mapped into 
* the virtual address space of all processes. A virtual address is not a unique 
* identifier for the lower 2GB of memory since each process has its own private process
* address space.
**********************************************************************************/
void PushPageIntoHookedList( HOOKED_LIST_ENTRY HookedPage )
{
	HOOKED_LIST_ENTRY* pHookedPage = (HOOKED_LIST_ENTRY*)ExAllocatePool(NonPagedPool,sizeof(HOOKED_LIST_ENTRY));
	memcpy( pHookedPage, &HookedPage, sizeof(HOOKED_LIST_ENTRY) );
	
	//the number of bits we need to address the list of hooked pages
	ULONG index = ((ULONG)pHookedPage->pExecuteView >> NUM_HASH_BITS) & 0x0000007F; 	

	if( g_HookedPageList[index] == NULL )		//no collsions - its the first entry in the list
	{
		pHookedPage->pNextEntry = NULL;
		g_HookedPageList[index] = pHookedPage;
	}
	else //there was a collision on the hash, add it to the end of the list
	{
		pHookedPage->pNextEntry = g_HookedPageList[index];
		g_HookedPageList[index] = pHookedPage;
	}

}//end InsertPageIntoHookedList



/*********************************************************************************
* PopPageFromHookedList - Removes a HOOKED_LIST_ENTRY from the linked list of entries
*						  contained at a given index in the hash table.
*
* Parameters - 
*			index - index into the hash table.
*
* Return -	1 or an error code.
*
* Error Codes: ERROR_EMPTY_LIST	
*
**********************************************************************************/
HOOKED_LIST_ENTRY* PopPageFromHookedList( int index )
{
	if( g_HookedPageList[index] == NULL )		//no collsions - its the first entry in the list
	{
		return (HOOKED_LIST_ENTRY*)ERROR_EMPTY_LIST;
	}

	else //remove the first page in the list
	{
		HOOKED_LIST_ENTRY* pList = g_HookedPageList[index];
		g_HookedPageList[index] = pList->pNextEntry;
		return pList;
	}

}//end RemovePageFromHookedList




/*********************************************************************************
* FreeHookedList - Frees all memory associated with all HOOKED_LIST_ENTRIES stored
* in the hash table's linked lists.
*
* Parameters - None
*
* Return -	1 or an error code.
*
* Error Codes: None
*
**********************************************************************************/
void FreeHookedList( void )
{
	for( int i = 0; i < MAX_NUMBER_OF_HOOKED_PAGES; i++ )
	{
		while( g_HookedPageList[i] != NULL )
		{
			HOOKED_LIST_ENTRY* pListEntry = PopPageFromHookedList( i );
			ExFreePool( pListEntry );
		}//end while
	}//end for

}//end FreeHookedList



/*********************************************************************************
* FindPageInHookedList - Determines if there is a hooked page corresponding to a
*						 given virtual address. Used by the page fault handler.
*
* Parameters - 
*			ULONG Virtual Address - An address on a page that we are looking up
*									to determine if the page is hooked or not.
*
* Return -	1 or an error code.
*
* Error Codes: ERROR_HOOKED_PAGE_NOT_IN_LIST	
*
**********************************************************************************/
PHOOKED_LIST_ENTRY FindPageInHookedList( ULONG VirtualAddress)
{
	
	ULONG index = ((ULONG)VirtualAddress >> NUM_HASH_BITS) &  0x0000007F;

	if( g_HookedPageList[index] == NULL )	//the list entry for the hash of the page being looked up is empty
	{
		return (PHOOKED_LIST_ENTRY)ERROR_PAGE_NOT_IN_LIST;
	}//end if
	else //walk the list looking for the page
	{
		HOOKED_LIST_ENTRY* pList = g_HookedPageList[index];
		do
		{
			//determine if we're dealing with a large page
			if( (VirtualAddress >= LOWEST_ADDRESS_IN_LARGE_PAGE_RANGE) && (VirtualAddress <= HIGHEST_ADDRESS_IN_LARGE_PAGE_RANGE) )
			{
				if( (ULONG)pList->pExecuteView == ( ( (ULONG)VirtualAddress >> 22 ) << 22 ) ) //round down to page boundary and see if the VPN's match
					return pList;
			}//end if
			else 
			{
				if( ((ULONG)pList->pExecuteView == ( (ULONG)VirtualAddress >> 12 ) << 12 ) ) //round down to page boundary and see if the VPN's match
					return pList;

				else
				pList = pList->pNextEntry;
			}//end else
		}//end else
		while( pList != NULL );

	}//end else		

	return (HOOKED_LIST_ENTRY*)ERROR_PAGE_NOT_IN_LIST;

}//end FindPageInHookedList




/************************************************************************************
* GetPteAddress - Returns a pointer to the page table entry corresponding to a given 
*				  memory address.
*
* Parameters:
*	PVOID VirtualAddress - Address you wish to acquire a pointer to the page table
*						   entry for.
*
* Return - Pointer to the page table entry for VirtualAddress or an error code.
*
* Error Codes:
*	ERROR_PTE_NOT_PRESENT - The page table for the given virtual address is not 
*							present in memory.
*   ERROR_PAGE_NOT_PRESENT - The page containing the data for the given virtual
*							 address is not present in memory.
***********************************************************************************/
PPTE GetPteAddress( PVOID VirtualAddress )
{
	PPTE pPTE = 0;
	__asm
	{
		cli			//disable interrupts
		pushad
		mov esi, PROCESS_PAGE_DIR_BASE
		mov edx, VirtualAddress
		mov eax, edx
		shr eax, 22
		lea eax, [esi + eax*4]  //pointer to page directory entry (PDE)
		test [eax], 0x80
		jnz Is_Large_Page	//it's a large page
		mov esi, PROCESS_PAGE_TABLE_BASE
		shr edx, 12
		lea eax, [esi + edx*4]  //pointer to page table entry (PTE)
		mov pPTE, eax
		jmp Done

		//NOTE: There is not a page table for large pages because the PTE's are contained in the page directory.
		Is_Large_Page:
		mov pPTE, eax

		Done:
		popad
		sti		//reenable interrupts
	}//end asm

	return pPTE;

}//end GetPteAddress



/***********************************************************************************
* IsInLargePage - Checks to see if a given virtual address resides on a large 4MB
*				  page.
*
* Parameters:
*	PVOID VirtualAddress - A virtual address contained in the page you're checking 
*						   the size for.
*
* Return - True if the given virtual address resides on a large 4MB page and false
*		   otherwise.
***********************************************************************************/
bool IsInLargePage( PVOID VirtualAddress )
{
	bool LargePage = false;

	__asm
	{
		pushad
		mov esi, PROCESS_PAGE_DIR_BASE
		mov edx, VirtualAddress
		mov eax, edx
		shr eax, 22
		lea eax, [esi + eax*4]  //pointer to page directory entry
		test [eax], 0x80
		jnz Is_Large_Page	//it's a large page
		jmp Done

		Is_Large_Page:
		mov LargePage, 1
		
		Done:
		popad
	}//end asm
	return LargePage;

}//end GetPteAddress



/***********************************************************************************
* DisableGlobalPageFeature - Disables the global page bit in a page's PTE entry.  
* When enabled, the global page bit prevents the page mapping from being flushed from the 
* TLB on either a context switch or invocation of the invlpg instruction. By disabling
* it we are able to use the invlpg instruction to selectively flush it.
*
* Parameters:
*	PPTE pPte - Pointer to the PTE for which you wish to enable the global page bit.
*
* Return - none
*
* Uses: eax
*************************************************************************************/
void DisableGlobalPageFeature( PPTE pPte )
{
	__asm
	{
		mov eax, pPte
		and dword ptr [eax], 0xFFFFFEFF //disable global page bit
	}//end asm

}//end DisableGlobalPageFeature



/***********************************************************************************
* EnableGlobalPageFeature - Enables the global page bit in a page's PTE entry.  
* When enabled, the global page bit prevents the page mapping from being flushed
* from the TLB on either a context switch or invocation of the invlpg instruction.
*
* Parameters:
*	PPTE pPte - Pointer to the PTE for which you wish to enable the global page bit.
*
* Return - none
*
* Uses: eax
/***********************************************************************************/
void EnableGlobalPageFeature( PPTE pPte )
{
	__asm
	{
		mov eax, pPte
		or dword ptr [eax], 0x100  //enable global page bit
	}//end asm
}


/***********************************************************************************
* MarkPageNotPresent - Clears the not present bit in a page's PTE so that it appears
*					   not to be physically present in memory. An int 0E fault is
*					   generated on an attempt to access a non present page.
*
* Parameters:
*	PPTE pPte - Pointer to the PTE for the page you wish to mark nonpresent.
*
* Return - none
************************************************************************************/
void MarkPageNotPresent( PPTE pPte )
{	
	__asm
	{
		mov eax, pPte
		and dword ptr [eax], 0xFFFFFFFE	//mark the page not present 
	}//end asm

}//end MarkPageNotPresent


/***********************************************************************************
* MarkPagePresent -  Sets the not present bit in a page's PTE so that it appears
*					 to be physically present in memory. 
* Parameters - 
*	PPTE pPte - Pointer to the PTE for the page you wish to mark present.
*
* Return - none
************************************************************************************/
void MarkPagePresent( PPTE pPte )
{
	__asm
	{
		mov eax, pPte
		or dword ptr [eax], 0x01 //mark the page present
	}//end asm
}//MarkPagePresent



/***********************************************************************************
* MarkPageReadWrite -  Sets the not present bit in a page's PTE so that it appears
*					 to be physically present in memory. 
* Parameters - 
*	PPTE pPte - Pointer to the PTE for the page you wish to mark present.
*
* Return - none
************************************************************************************/
void MarkPageReadWrite( PPTE pPte )
{
	__asm
	{
		mov eax, pPte
		or dword ptr [eax], 0x02 //mark the page present
	}//end asm
}//end MarkPageReadWrite



/***********************************************************************************
* MarkPageReadOnly -  Sets the not present bit in a page's PTE so that it appears
*					 to be physically present in memory. 
* Parameters - 
*	PPTE pPte - Pointer to the PTE for the page you wish to mark present.
*
* Return - none
************************************************************************************/
void MarkPageReadOnly( PPTE pPte )
{
	__asm
	{
		mov eax, pPte
		and dword ptr [eax], 0xFFFFFFFD	//mark the page read only
	}//end asm
}


/****************************************************************************************
* GetPhysicalFrameAddress - Gets the physical address in memory where the page is mapped.
* This corresponds to the 12 - 32 in the page table entry.
*
* Parameters - 
*	PPTE pPte - Pointer to the PTE that you wish to retrieve the physical address from.
*
* Return - The physical address of the page.
******************************************************************************************/
ULONG GetPhysicalFrameAddress( PPTE pPte )
{
	ULONG Frame = 0;

	__asm
	{
		pushad
		mov eax, pPte
		mov ecx, [eax]
		shr ecx, 12		//physical page frame consists of the upper 20 bits
		mov Frame, ecx
		popad
	}//end asm
	return Frame;

}//end GetPageFrame



/*********************************************************************************************
* ReplacePageFrame - Replaces the physical address of a given page with a new physical address
* by modifying the PTE for the page. 
*
* Parameters - 
*	PPTE pPte - Pointer to the PTE that you wish to replace the physical address for.
*	ULONG* oldFrame - Pointer to a variable that will recieve the stored old physical frame
*					  address.
*	ULONG newFrame - The new physical frame address that we are inserting into the PTE.
*
* Return - none
*
* NOTE: This function is the basis of the memory hook: that is, the ability to have a single
* virtual address mapping that points to two separate physical frames (one containing patched data 
* and one that is "clean" / unpatched ). Which mapping is used is, of course, specific to weather
* the given memory access is due to an execute or a read / write. Executes will use the patched
* mapping, while read / writes will used the "clean" mapping. This is possible due to the split
* nature of the Intel TLB (ie. that there are separate TLBs for instructions and data).
*
*******************************************************************************************/
void ReplacePhysicalFrameAddress( PPTE pPte, ULONG* oldFrame, ULONG newFrame)
{
	*oldFrame = GetPhysicalFrameAddress( pPte );
	
	__asm
	{
		pushad
		mov ecx, newFrame
		mov eax, pPte
		mov edi, [eax]
		and edi, 0x00000FFF //preserve the lower 12 bits
		and ecx, 0xFFFFF000 //isolate the physical address of the unpatched page PTE
		or ecx, edi  
		mov [ebx], ecx //replace the PTE's physical frame address w/ the unpatched PTE's
		popad
	}//end asm

}//end ReplacePageFrame




