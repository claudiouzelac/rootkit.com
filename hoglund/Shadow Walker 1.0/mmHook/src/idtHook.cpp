extern "C"
{
	#include "ntddk.h"
}

#include "mmHook.h"
#include "idtHook.h"


/*********************************************************************************
* HookInt - Hooks an x86 interrupt in the interrupt descriptor table.
*
* Parameters - 
*			unsigned long pOldHandler - pointer to a variable to store the old
*										interrupt handler
*			unsigned long NewHandler - function pointer to the interrupt handler 
*									   we are installing
*			int IntNumber - number of the interrupt we are hooking (0-255)
*
*
* Return -	true on success, error code on failure
*
* Error Codes - ERROR_PTE_NOT_PRESENT, ERROR_PAGE_NOT_PRESENT
*
* NOTES:
*		1. This routine needs to be modified to work on hyperthreading and multiple CPU
*	       systems which may have more than one IDT.
*		2. Not entirely sure, but the IDT may be write-protected on some platforms.
*		   This will be easy to fix, but I just haven't done it yet.
*
**********************************************************************************/
int HookInt( unsigned long* pOldHandler, unsigned long NewHandler, int IntNumber )
{
	IDT_INFO IdtInfo = {0};

	__asm cli	//disable interrupts

	//get the base address of the IDT
	__asm sidt IdtInfo.wLimit

	//un-write-protect the IDT if it is write protected
	PVOID pInt = &IdtInfo.pIdt[IntNumber];
	PPTE pPteIdt = GetPteAddress( pInt );

	if( ((int)pPteIdt != ERROR_PTE_NOT_PRESENT) && ((int)pPteIdt != ERROR_PAGE_NOT_PRESENT) ) //the idt should never be "not present", but playing it safe
	{
		ULONG oldPteIdt = (ULONG)*pPteIdt;
		MarkPageReadWrite( pPteIdt );
		__asm invlpg pInt 

		//save the old interrupt handler
		*pOldHandler = (unsigned int)IdtInfo.pIdt[IntNumber].OffsetHi << 16
					| IdtInfo.pIdt[IntNumber].OffsetLo;
	
		//install the new handler
		IdtInfo.pIdt[IntNumber].OffsetLo = (unsigned short)NewHandler;
		IdtInfo.pIdt[IntNumber].OffsetHi = (unsigned short)((unsigned int)NewHandler >> 16);	
	
		//restore the old writability of the idt
		(ULONG)*pPteIdt = oldPteIdt;
		__asm invlpg pInt 
	
		__asm sti	//reenable interrupts
		return true;
	}//end if
	
	__asm sti
	return (int)pPteIdt;
}//end HookInt



/*********************************************************************************
* HookInt - Unhooks an x86 interrupt in the interrupt descriptor table.
*
* Parameters - 
*			unsigned long pOldHandler -  the old interrupt handler
*			int IntNumber - number of the interrupt we are unhooking (0-255)
*
* Return -	true on success, error code on failure
*
* Error Codes - ERROR_PTE_NOT_PRESENT, ERROR_PAGE_NOT_PRESENT
*				
* NOTES:
*		1. This routine needs to be modified to work on hyperthreading and multiple CPU
*	       systems which may have more than one IDT.
*		2. Not entirely sure, but the IDT may be write-protected on some platforms.
*		   This will be easy to fix, but I just haven't done it yet.
*
**********************************************************************************/
int UnhookInt( unsigned long OldHandler,  int IntNumber )
{
	IDT_INFO IdtInfo = {0};

	__asm cli	//disable interrupts

	//get the base address of the IDT
	__asm sidt IdtInfo.wLimit

	//un-write-protect the IDT if it is write protected
	PVOID pInt = &IdtInfo.pIdt[IntNumber];
	PPTE pPteIdt = GetPteAddress( pInt );

	if( ((int)pPteIdt != ERROR_PTE_NOT_PRESENT) && ((int)pPteIdt != ERROR_PAGE_NOT_PRESENT) ) //the idt should never be "not present", but playing it safe
	{
		ULONG oldPteIdt = (ULONG)*pPteIdt;
		MarkPageReadWrite( pPteIdt );
		__asm invlpg pInt 

		//restore the old handler
		IdtInfo.pIdt[IntNumber].OffsetLo = (unsigned short)OldHandler;
		IdtInfo.pIdt[IntNumber].OffsetHi = (unsigned short)((unsigned int)OldHandler >> 16);
	
		//restore the old writability of the idt
		(ULONG)*pPteIdt = oldPteIdt;
		__asm invlpg pInt 

		__asm sti	//reenable interrupts
		return true;
	}//end if

	__asm sti
	return (int)pPteIdt;

}//end UnhookInt

