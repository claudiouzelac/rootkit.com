// BASIC DEVICE DRIVER

#include "ntddk.h"
#include <stdio.h>

VOID rootkit_command_thread(PVOID context);
HANDLE gWorkerThread;
PKTIMER	gTimer;
PKDPC	gDPCP;
UCHAR g_key_bits = 0;

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

ULONG WaitForKeyboard()
{
	char _t[255];
	int i = 100;	// number of times to loop
	UCHAR mychar;
	
	DbgPrint("waiting for keyboard to become accecssable\n");
	do
	{
		mychar = READ_PORT_UCHAR( KEYBOARD_PORT_64 );

		KeStallExecutionProcessor(666);

		_snprintf(_t, 253, "WaitForKeyboard::read byte %02X from port 0x64\n", mychar);
		DbgPrint(_t);

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
	
	DbgPrint("draining keyboard buffer\n");
	do
	{
		c = READ_PORT_UCHAR(KEYBOARD_PORT_64);
		
		KeStallExecutionProcessor(666);

		_snprintf(_t, 253, "DrainOutputBuffer::read byte %02X from port 0x64\n", c);
		DbgPrint(_t);

		if(!(c & OBUFFER_FULL)) break;	// if the flag is clear, we go ahead
	
		// gobble up the byte in the output buffer
		c = READ_PORT_UCHAR(KEYBOARD_PORT_60);
		
		_snprintf(_t, 253, "DrainOutputBuffer::read byte %02X from port 0x60\n", c);
		DbgPrint(_t);
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

		_snprintf(_t, 253, "SendKeyboardCommand::sending byte %02X to port 0x60\n", theCommand);
		DbgPrint(_t);

		WRITE_PORT_UCHAR( KEYBOARD_PORT_60, theCommand );
		
		DbgPrint("SendKeyboardCommand::sent\n");
	}
	else
	{
		DbgPrint("SendKeyboardCommand::timeout waiting for keyboard\n");
		return FALSE;
	}
	
	// TODO: wait for ACK or RESEND from keyboard	
	
	return TRUE;
}

void SetLEDS( UCHAR theLEDS )
{
	// setup for setting LEDS
	if(FALSE == SendKeyboardCommand( 0xED ))
	{
		DbgPrint("SetLEDS::error sending keyboard command\n");
	}

	// send the flags for the LEDS
	if(FALSE == SendKeyboardCommand( theLEDS ))
	{
		DbgPrint("SetLEDS::error sending keyboard command\n");
	}
}

VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("ROOTKIT: OnUnload called\n");
	KeCancelTimer( gTimer );
	ExFreePool( gTimer );
	ExFreePool( gDPCP );
}

// called periodically
VOID timerDPC(	IN PKDPC Dpc,
				IN PVOID DeferredContext,
				IN PVOID sys1,
				IN PVOID sys2)
{
	SetLEDS( g_key_bits++ );
	if(g_key_bits > 0x07) g_key_bits = 0;
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	LARGE_INTEGER timeout;

	theDriverObject->DriverUnload  = OnUnload; 

	// these objects must be non paged
	gTimer = ExAllocatePool(NonPagedPool,sizeof(KTIMER));
	gDPCP = ExAllocatePool(NonPagedPool,sizeof(KDPC));

	timeout.QuadPart = -10;

	KeInitializeTimer( gTimer );
	KeInitializeDpc( gDPCP, timerDPC, NULL );

	if(TRUE == KeSetTimerEx( gTimer, timeout, 300, gDPCP))	// 300 ms timer	
	{
		DbgPrint("Timer was already queued..");
	}

	return STATUS_SUCCESS;
}

