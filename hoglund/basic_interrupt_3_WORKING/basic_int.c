// ---------------------------
// BASIC INTERRUPT HOOK part 3
// this hooks the entire table
// ---------------------------

#include "ntddk.h"
#include <stdio.h>

//debuggering
//#define _DEBUG

#define MAKELONG(a, b) ((unsigned long) (((unsigned short) (a)) | ((unsigned long) ((unsigned short) (b))) << 16)) 

// set this to the max int you want to hook
#define MAX_IDT_ENTRIES 0xFF

// the starting interrupt for patching
// to 'skip' some troublesome interrupts
// at the beginning of the table (TODO, find out why)
#define START_IDT_OFFSET 0x00

unsigned long g_i_count[MAX_IDT_ENTRIES];
unsigned long old_ISR_pointers[MAX_IDT_ENTRIES];	// better save the old one!!

#ifdef _DEBUG
// debuggering version nops out our 'hook'
// this works w/ no crashes
char jump_template[] = { 
	0x90,										//nop, debug
	0x60,										//pushad
	0x9C,										//pushfd
	0xB8, 0xAA, 0x00, 0x00, 0x00,				//mov eax, AAh
	0x90,										//push eax
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,	//call 08:44332211h
	0x90,										//pop eax
	0x9D,										//popfd
	0x61,										//popad
	0xEA, 0x11, 0x22, 0x33, 0x44, 0x08, 0x00	//jmp 08:44332211h
};
#else
char jump_template[] = { 
	0x90,										//nop, debug
	0x60,										//pushad
	0x9C,										//pushfd
	0xB8, 0xAA, 0x00, 0x00, 0x00,				//mov eax, AAh
	0x50,										//push eax
	0x9A, 0x11, 0x22, 0x33, 0x44, 0x08, 0x00,	//call 08:44332211h
	0x58,										//pop eax
	0x9D,										//popfd
	0x61,										//popad
	0xEA, 0x11, 0x22, 0x33, 0x44, 0x08, 0x00	//jmp 08:44332211h
};
#endif

char * idt_detour_tablebase;
	

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



VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{	
	int i;
	IDTINFO		idt_info;		// this structure is obtained by calling STORE IDT (sidt)
	IDTENTRY*	idt_entries;	// and then this pointer is obtained from idt_info
	char _t[255];

	// load idt_info
	__asm	sidt	idt_info	
	idt_entries = (IDTENTRY*) MAKELONG(idt_info.LowIDTbase,idt_info.HiIDTbase);

	DbgPrint("ROOTKIT: OnUnload called\n");

	for(i=START_IDT_OFFSET;i<MAX_IDT_ENTRIES;i++)
	{
		_snprintf(_t, 253, "interrupt %d called %d times", i, g_i_count[i]);
		DbgPrint(_t);
	}

	DbgPrint("UnHooking Interrupt...");

	for(i=START_IDT_OFFSET;i<MAX_IDT_ENTRIES;i++)
	{
		// restore the original interrupt handler
		__asm cli
		idt_entries[i].LowOffset = (unsigned short) old_ISR_pointers[i];
		idt_entries[i].HiOffset = (unsigned short)((unsigned long)old_ISR_pointers[i] >> 16);
		__asm sti
	}

	
	DbgPrint("UnHooking Interrupt complete.");
}


// using stdcall means that this function fixes the stack before returning (opposite of cdecl)
// interrupt number passed in EAX
void __stdcall count_interrupts(unsigned long inumber)
{
	//todo, may have collisions here?
	unsigned long *aCountP; 
	unsigned long aNumber;

	// due to far call, we need to correct the base pointer
	// the far call pushes a double dword as the return address
	// and I don't know how to make the compiler understand this
	// is a __far __stdcall (or whatever it's called)
	// anyway:
	//
	// [ebp+0Ch] == arg1
	//
	__asm mov eax, [ebp+0Ch]
	__asm mov aNumber, eax
	
	//__asm int 3

	aNumber = aNumber & 0x000000FF;
	aCountP = &g_i_count[aNumber]; 
	InterlockedIncrement(aCountP);
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

	for(count=START_IDT_OFFSET;count<MAX_IDT_ENTRIES;count++)
	{
		g_i_count[count]=0;
	}

	// load idt_info
	__asm	sidt	idt_info
	idt_entries = (IDTENTRY*) MAKELONG(idt_info.LowIDTbase,idt_info.HiIDTbase);

	////////////////////////////////////////////
	// save old idt pointers
	////////////////////////////////////////////
	for(count=START_IDT_OFFSET;count < MAX_IDT_ENTRIES;count++)
	{
		i = &idt_entries[count];
		addr = MAKELONG(i->LowOffset, i->HiOffset);
		
		_snprintf(_t, 253, "Interrupt %d: ISR 0x%08X", count, addr);
		DbgPrint(_t);
	
		old_ISR_pointers[count] = MAKELONG(idt_entries[count].LowOffset,idt_entries[count].HiOffset);
	}

	
	///////////////////////////////////////////
	// setup the detour table
	///////////////////////////////////////////
	idt_detour_tablebase = ExAllocatePool(NonPagedPool, sizeof(jump_template)*256);

	for(count=START_IDT_OFFSET;count<MAX_IDT_ENTRIES;count++)
	{
		int offset = sizeof(jump_template)*count;	
		char *entry_ptr = idt_detour_tablebase + offset;

		// entry_ptr points to the start of our jump code in the detour_table

		// copy the starter code into the template location
		memcpy(entry_ptr, jump_template, sizeof(jump_template));

#ifndef _DEBUG

		// stamp the interrupt number
		entry_ptr[4] = (char)count;

		// stamp the far call to the hook routine
		*( (unsigned long *)(&entry_ptr[10]) ) = (unsigned long)count_interrupts;
#endif

		// stamp the far jump to the original ISR
		*( (unsigned long *)(&entry_ptr[20]) ) = old_ISR_pointers[count];

		// finally, make the interrupt point to our template code
		__asm cli
		idt_entries[count].LowOffset = (unsigned short)entry_ptr;
		idt_entries[count].HiOffset = (unsigned short)((unsigned long)entry_ptr >> 16);
		__asm sti
	}

	DbgPrint("Hooking Interrupt complete");

	return STATUS_SUCCESS;
}
