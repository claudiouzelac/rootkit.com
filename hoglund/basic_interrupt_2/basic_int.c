// ---------------------------
// BASIC INTERRUPT HOOK part 2
// this hooks the timer interrupt
// ---------------------------

#include "ntddk.h"
#include <stdio.h>

#define MAKELONG(a, b) ((unsigned long) (((unsigned short) (a)) | ((unsigned long) ((unsigned short) (b))) << 16)) 

#define MAX_IDT_ENTRIES 0xFF

#define NT_INT_UNUSED_A				0x20
#define NT_INT_UNUSED_B				0x22
#define NT_INT_UNUSED_C				0x23
#define NT_INT_UNUSED_D				0x24
#define NT_INT_UNUSED_E				0x25
#define NT_INT_UNUSED_F				0x26
#define NT_INT_UNUSED_G				0x27
#define NT_INT_UNUSED_H				0x28
#define NT_INT_UNUSED_I				0x29

#define NT_INT_TIMER				0x30

unsigned long g_i_count = 0;

///////////////////////////////////////////////////
// IDT structures
///////////////////////////////////////////////////
#pragma pack(1)

// entry in the IDT, this is sometimes called
// an "interrupt gate"
typedef struct
{
	unsigned short LowOffset;
	unsigned short selector;
	unsigned char unused_lo;
	unsigned char segment_type:4;	//0x0E is an interrupt gate
	unsigned char system_segment_flag:1;
	unsigned char DPL:2;	// descriptor privilege level 
	unsigned char P:1; /* present */
	unsigned short HiOffset;
} IDTENTRY;

/* sidt returns idt in this format */
typedef struct
{
	unsigned short IDTLimit;
	unsigned short LowIDTbase;
	unsigned short HiIDTbase;
} IDTINFO;

#pragma pack()

unsigned long old_ISR_pointer;	// better save the old one!!


VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{	
	IDTINFO		idt_info;		// this structure is obtained by calling STORE IDT (sidt)
	IDTENTRY*	idt_entries;	// and then this pointer is obtained from idt_info
	char _t[255];

	// load idt_info
	__asm	sidt	idt_info	
	idt_entries = (IDTENTRY*) MAKELONG(idt_info.LowIDTbase,idt_info.HiIDTbase);

	DbgPrint("ROOTKIT: OnUnload called\n");

	_snprintf(_t, 253, "called %d times", g_i_count);
	DbgPrint(_t);
	
	DbgPrint("UnHooking Interrupt...");

	// restore the original interrupt handler
	__asm cli
	idt_entries[NT_INT_TIMER].LowOffset = (unsigned short) old_ISR_pointer;
	idt_entries[NT_INT_TIMER].HiOffset = (unsigned short)((unsigned long)old_ISR_pointer >> 16);
	__asm sti

	DbgPrint("UnHooking Interrupt complete.");
}

// using stdcall means that this function fixes the stack before returning (opposite of cdecl)
void __stdcall count_syscall( unsigned long system_call_number )
{
	g_i_count++;
}

// naked functions have no prolog/epilog code - they are functionally like the 
// target of a goto statement
__declspec(naked) my_interrupt_hook()
{
	__asm
	{
		push	eax
		call	count_syscall
		jmp		old_ISR_pointer
	}
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	IDTINFO		idt_info;		// this structure is obtained by calling STORE IDT (sidt)
	IDTENTRY*	idt_entries;	// and then this pointer is obtained from idt_info
	IDTENTRY*	i;
	unsigned long 	addr;
	unsigned long	count;
	char _t[255];
	
	theDriverObject->DriverUnload  = OnUnload; 

	// load idt_info
	__asm	sidt	idt_info
	
	idt_entries = (IDTENTRY*) MAKELONG(idt_info.LowIDTbase,idt_info.HiIDTbase);

	for(count=0;count < MAX_IDT_ENTRIES;count++)
	{
		i = &idt_entries[count];
		addr = MAKELONG(i->LowOffset, i->HiOffset);
		
		_snprintf(_t, 253, "Interrupt %d: ISR 0x%08X", count, addr);
		DbgPrint(_t);
	}

	DbgPrint("Hooking Interrupt...");
	// lets hook an interrupt
	// exercise - choose your own interrupt
	old_ISR_pointer = MAKELONG(idt_entries[NT_INT_TIMER].LowOffset,idt_entries[NT_INT_TIMER].HiOffset);
	
// debug, use this if you want some additional info on what is going on
#if 0
	_snprintf(_t, 253, "old address for ISR is 0x%08x", old_ISR_pointer);
	DbgPrint(_t);
	_snprintf(_t, 253, "address of my function is 0x%08x", my_interrupt_hook);
	DbgPrint(_t);
#endif
	
	// remember we disable interrupts while we patch the table
	__asm cli
	idt_entries[NT_INT_TIMER].LowOffset = (unsigned short)my_interrupt_hook;
	idt_entries[NT_INT_TIMER].HiOffset = (unsigned short)((unsigned long)my_interrupt_hook >> 16);
	__asm sti

// debug - use this if you want to check what is now placed in the interrupt vector
#if 0
	i = &idt_entries[NT_INT_TIMER];
	addr = MAKELONG(i->LowOffset, i->HiOffset);
	_snprintf(_t, 253, "Interrupt ISR 0x%08X", addr);
	DbgPrint(_t);	
#endif

	DbgPrint("Hooking Interrupt complete");

	return STATUS_SUCCESS;
}
