// --------------------
// BASIC INTERRUPT HOOK
// --------------------

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

#define NT_INT_SYSTEMSERVICE		0x2E

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
	DbgPrint("ROOTKIT: OnUnload called\n");
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	IDTINFO		idt_info;		// this structure is obtained by calling STORE IDT (sidt)
	IDTENTRY*	idt_entries;	// and then this pointer is obtained from idt_info
	unsigned long	count;

	theDriverObject->DriverUnload  = OnUnload; 

	// load idt_info
	__asm	sidt	idt_info
	
	idt_entries = (IDTENTRY*) MAKELONG(idt_info.LowIDTbase,idt_info.HiIDTbase);

	for(count=0;count < MAX_IDT_ENTRIES;count++)
	{
		char _t[255];
		IDTENTRY *i = &idt_entries[count];
		unsigned long addr = 0;
		addr = MAKELONG(i->LowOffset, i->HiOffset);
		
		_snprintf(_t, 253, "Interrupt %d: ISR 0x%08X", count, addr);
		DbgPrint(_t);
	}

	return STATUS_SUCCESS;
}
