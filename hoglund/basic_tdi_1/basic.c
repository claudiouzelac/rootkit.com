/////////////////////////////////////////////////////////////////////////
// BASIC TDI DEVICE DRIVER
// 
// (c)2004 Rootkit.com
//
// June 25, 2004 - Initial cut, Greg Hoglund
// 
// Some components based on publically available examples leeched from 
// Thomas Divine and James Antognini
//
/////////////////////////////////////////////////////////////////////////

#include <ntddk.h>
#include <tdikrnl.h>

// these are the 3 big-ones for kernel mode tcp/ip
// -----------------------------------------------
#define DD_TCP_DEVICE_NAME      L"\\Device\\Tcp"
#define DD_UDP_DEVICE_NAME      L"\\Device\\Udp"
#define DD_RAW_IP_DEVICE_NAME   L"\\Device\\RawIp"

// some very useful macros
#define INETADDR(a, b, c, d) (a + (b<<8) + (c<<16) + (d<<24))
#define HTONL(a) (((a&0xFF)<<24) + ((a&0xFF00)<<8) + ((a&0xFF0000)>>8) + ((a&0xFF000000)>>24))  
#define HTONS(a) (((0xFF&a)<<8) + ((0xFF00&a)>>8))
 

static NTSTATUS TDICompletionRoutine(IN PDEVICE_OBJECT theDeviceObject, IN PIRP theIrp, IN PVOID theContextP)
{
	DbgPrint("TDICompletionRoutine called.");

	if(NULL != theContextP)
	{
		DbgPrint("calling KeSetEvent.");

		// must run at <= DISPATCH_LEVEL
		ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
		KeSetEvent((PKEVENT)theContextP, 0, FALSE);
	}
	return STATUS_MORE_PROCESSING_REQUIRED;
}

// we don't use this, it's a place holder
NTSTATUS OnStubDispatch( 
					IN PDEVICE_OBJECT DeviceObject,
					IN PIRP Irp )
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	
	IoCompleteRequest(	Irp,
						IO_NO_INCREMENT );

	return STATUS_SUCCESS;
}

// for unloading the driver
VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("BASIC TDI: OnUnload called\n");
}

// entry point, like main()
NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	int i;
	UNICODE_STRING				TDI_TransportDeviceName;
	OBJECT_ATTRIBUTES           TDI_Object_Attr;
	NTSTATUS					status;
	IO_STATUS_BLOCK             IoStatus;
	PTA_IP_ADDRESS				pSin;
	HANDLE						TDI_Address_Handle;
	HANDLE						TDI_Endpoint_Handle;
	CONNECTION_CONTEXT			contextPlaceholder = NULL;
	ULONG						ulBuffer; 
	PIRP						pIrp;
	PVOID						pConnFileObj;
	PVOID						pAddrFileObj;
	PVOID						pTcpDevObj;
	KEVENT                      AssociateEvent;
	KEVENT						ConnectEvent;
	TA_IP_ADDRESS				RmtIPAddr;
	ULONG						RemoteAddr;                 // Remote IP address (assigned below)
    USHORT						RemotePort;                 // Remote port (assigned below)
	TDI_CONNECTION_INFORMATION  RmtNode;

	// this buffer is described in detail below...
	char						EA_Buffer[	sizeof(FILE_FULL_EA_INFORMATION) + 
											TDI_TRANSPORT_ADDRESS_LENGTH + 
											sizeof(TA_IP_ADDRESS)];
	PFILE_FULL_EA_INFORMATION	pEA_Buffer = (PFILE_FULL_EA_INFORMATION)EA_Buffer;

	DbgPrint("BASIC_TDI is alive!");

	// do this, or you can't unload
	theDriverObject->DriverUnload  = OnUnload; 

	// install a stub for handling usermode communication
	for(i=0;i< IRP_MJ_MAXIMUM_FUNCTION; i++ )
	{
		theDriverObject->MajorFunction[i] = OnStubDispatch;
	}

	//////////////////////////////////////////////////////////
	// open a TDI channel and connect to a machine on the net
	//////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////
	// step one, use ZwCreateFile to create an address
	// object.
	//
	// a note on TDI:  TDI 'providers' create named
	// device objects (i.e, "/device/tcp").  In order
	// to use these, you must open the device by name
	// using ZwCreateFile.  Once you have an open file
	// handle, you can "talk" to this device using
	// IRP's (IO Request Packets).  You create and send
	// IRP's using the IoCallDriver() function.  This is
	// all it takes to use TDI!  ;-)
	///////////////////////////////////////////////////////

	// Build Unicode transport device name.
	RtlInitUnicodeString(	&TDI_TransportDeviceName,               
							DD_TCP_DEVICE_NAME			// the "/device/tcp" string
							);

	// create object attribs
	// must be called at PASSIVE_LEVEL
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	InitializeObjectAttributes(
							&TDI_Object_Attr,			// Attributes (to be initialized);
                            &TDI_TransportDeviceName,
                            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, //for win2k or higher...
                            0,
                            0
							);

	////////////////////////////////////////////////////////
	// Now we encounter the first bit of 'black magic'
	// Some drivers (mostly filesystem drivers) use
	// the 'extended attributes' argument of ZwCreateFile.
	// This argument can point to an arbitrary structure
	// that provides important initialization data to the
	// 'provider' we are opening.  In order to use the
	// structure, you must have full documentation on it.
	// If you can't find the docs for the structure, you 
	// will have a hard time.
	//
	// Unfortunately, very little good documentation exists
	// for TDI, which is why it seems so complicated.
	//
	// We must fill in the extended attributes
	// structure in order to open the tcp handle.
	//
	// The "FILE_FULL_EA_INFORMATION" structure looks like:
	// typedef struct _FILE_FULL_EA_INFORMATION {
	// ULONG  NextEntryOffset;
    // UCHAR  Flags;
    // UCHAR  EaNameLength;
    // USHORT  EaValueLength;
    // CHAR  EaName[1];				<--- set this to TDI_TRANSPORT_ADDRESS followed by an TA_IP_ADDRESS
    // } FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;
	////////////////////////////////////////////////////////
	
	pEA_Buffer->NextEntryOffset = 0;		// only one entry
	pEA_Buffer->Flags = 0;
	
	//The EaName is simply the string "TransportAddress" (defined as  TdiTransportAddress in TDI.H)
	pEA_Buffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;    // Length of TdiTransportAddress, namely of "TransportAddress" string less 1 (ie, without terminator).
	memcpy(	pEA_Buffer->EaName,                                 // Copy TdiTransportAddress, including terminator.
			TdiTransportAddress,
			pEA_Buffer->EaNameLength + 1
			);
	
	// The EaValue is the TA_TRANSPORT_ADDRESS representation of your desired local host IP address and port
	//
	// the TDI_ADDRESS_IP structure is the Kernel equivalent of Winsock SOCKADDR_IN IP address structure
	// the TA_IP_ADDRESS structure contains one or more TDI_ADDRESS_IP structures
	pEA_Buffer->EaValueLength = sizeof(TA_IP_ADDRESS);
	
	// Point to Buffer just after what's been used (ie, after terminator).
	// We write the TA_IP_ADDRESS data to this location
	//
	// the TA_IP_ADDRESS structure looks like:
	// typedef struct _TA_ADDRESS_IP {
    //	LONG  TAAddressCount;
    //	struct  _AddrIp {
    //		USHORT          AddressLength;
    //		USHORT          AddressType;
    //		TDI_ADDRESS_IP  Address[1];
    //	} Address [1];
	// } TA_IP_ADDRESS, *PTA_IP_ADDRESS;
	/////////////////////////////////////////////////////////////////////////
	pSin = (PTA_IP_ADDRESS)	(pEA_Buffer->EaName + pEA_Buffer->EaNameLength + 1); 
	pSin->TAAddressCount = 1;                                                                                                                                                                                                                                     
	pSin->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;                                                                                                                                                                                                       
	pSin->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;                                                                                                                                                                                                           
	pSin->Address[0].Address[0].sin_port = 0;			// local port, use zero to use any available (close them when done or you run out)                                                                                                                                                          
	pSin->Address[0].Address[0].in_addr = 0;			// use 0.0.0.0

	// Ensure remainder of structure is zeroes.
	memset(	pSin->Address[0].Address[0].sin_zero,         
			0,
			sizeof(pSin->Address[0].Address[0].sin_zero)
			);

	// after all this setup, now 'open' the address handle
	// must be called at PASSIVE_LEVEL
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	status = ZwCreateFile(
					   &TDI_Address_Handle,                       
                       GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                       &TDI_Object_Attr,
                       &IoStatus,
                       0,
                       FILE_ATTRIBUTE_NORMAL,
                       FILE_SHARE_READ,
                       FILE_OPEN,
                       0,
                       pEA_Buffer,
                       sizeof(EA_Buffer)
                      );
 
	if(!NT_SUCCESS(status))
	{
		DbgPrint("Failed to open address object, status 0x%08X", status);
		
		//todo, free resources

		return STATUS_UNSUCCESSFUL;
	}

	// get object handle
	// must be called at PASSIVE_LEVEL
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	status = ObReferenceObjectByHandle(
									TDI_Address_Handle,   
                                    FILE_ANY_ACCESS,
                                    0,
                                    KernelMode,
                                    (PVOID *)&pAddrFileObj,
                                    NULL
                                   );


	/////////////////////////////////////////////////////////////////////////////
	// Step 2, open a TDI endpoint
	//
	// This is another call to ZwCreateFile, the only thing that differs here 
	// is the data in the 'magic' EA_Buffer.  Now you can see the most of the
	// real arguments are passed in the EA structure! ;-)
	/////////////////////////////////////////////////////////////////////////////

	// Per Catlin, microsoft.public.development.device.drivers,
	// "question on TDI client, please do help," 2002-10-18.
	ulBuffer =                                     
         FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName) +
         TDI_CONNECTION_CONTEXT_LENGTH + 1              +
         sizeof(CONNECTION_CONTEXT);

	pEA_Buffer = (PFILE_FULL_EA_INFORMATION)ExAllocatePool(NonPagedPool, ulBuffer);
	if(NULL==pEA_Buffer)
	{
		DbgPrint("Failed to allocate buffer");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Use name TdiConnectionContext, which is a string == "ConnectionContext"
	memset(pEA_Buffer, 0, ulBuffer);
	pEA_Buffer->NextEntryOffset = 0;
	pEA_Buffer->Flags = 0;
	pEA_Buffer->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH; //don't incude NULL in length
	memcpy(	pEA_Buffer->EaName,	
			TdiConnectionContext,
			pEA_Buffer->EaNameLength + 1 //include NULL terminator in copy
			);

	// Point to Buffer just after what's been used (ie, after terminator).
	// We write the CONTEXT pointer to this location
	// 
	// A note on context:  A CONNECTION_CONTEXT is just a pointer to a user-defined
	// structure.  This means you can put whatever you want in the structure.
	// The reason it exists is so you can figure out WHICH connection your dealing with
	// when you callbacks for received data, etc.  This is obviously really important!
	//
	// We are only dealing with ONE connection in this example, so we set the context to
	// a dummy value.
	////////////////////////////////////////////////////////////////////////////////////
	pEA_Buffer->EaValueLength = sizeof(CONNECTION_CONTEXT);
	*(CONNECTION_CONTEXT*)(pEA_Buffer->EaName+(pEA_Buffer->EaNameLength + 1)) = (CONNECTION_CONTEXT) contextPlaceholder;                                   

	// ZwCreateFile must run a PASSIVE_LEVEL
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	status = ZwCreateFile(
					   &TDI_Endpoint_Handle,                       
                       GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                       &TDI_Object_Attr,
                       &IoStatus,
                       0,
                       FILE_ATTRIBUTE_NORMAL,
                       FILE_SHARE_READ,
                       FILE_OPEN,
                       0,
                       pEA_Buffer,
                       sizeof(EA_Buffer)
                      );

	if(!NT_SUCCESS(status))
	{
		DbgPrint("Failed to open endpoint, status 0x%08X", status);
		
		//todo, free resources

		return STATUS_UNSUCCESSFUL;
	}

	// get object handle
	// must run a PASSIVE_LEVEL
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	status = ObReferenceObjectByHandle(
				TDI_Endpoint_Handle,
                FILE_ANY_ACCESS,
                0,
                KernelMode,
                (PVOID *)&pConnFileObj,
                NULL
                );


	//////////////////////////////////////////////////////////////
	// Step 3, associate the endpoint with the address
	//////////////////////////////////////////////////////////////
	
	// get the device associated with the address object, in other words, a handle to the TDI driver's device object
	// (i.e, "\Driver\SYMTDI")
	pTcpDevObj = IoGetRelatedDeviceObject(pAddrFileObj);

	//used to wait for an IRP below...
	KeInitializeEvent(&AssociateEvent, NotificationEvent, FALSE);

	// build an IRP to make the association call
	pIrp =
	TdiBuildInternalDeviceControlIrp(	TDI_ASSOCIATE_ADDRESS,
                                        pTcpDevObj,   // TDI driver's device object.
                                        pConnFileObj, // Connection (endpoint) file object.
                                        &AssociateEvent,       // Event to be signalled when Irp completes.
                                        &IoStatus     // I/O status block.
                                       );

     if(NULL==pIrp) 
     {
	    DbgPrint("Could not get an IRP for TDI_ASSOCIATE_ADDRESS");
		return(STATUS_INSUFFICIENT_RESOURCES);
	 }

	 // adds some more data to the IRP
	 TdiBuildAssociateAddress(pIrp,
                              pTcpDevObj,
                              pConnFileObj,           
                              NULL,                   
                              NULL,
                              TDI_Address_Handle
                             );

	 // Send a command to the underlying TDI driver.  This is the essence of our communication
	 // channel to the underlying driver.
	 
	 // set our own completion routine
	 // must run at PASSIVE_LEVEL
	 ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	 IoSetCompletionRoutine( pIrp, TDICompletionRoutine, &AssociateEvent, TRUE, TRUE, TRUE);
	 
	 //make the call
	 // must run at <= DISPATCH_LEVEL
	 ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

	 status = IoCallDriver(pTcpDevObj, pIrp);

	 // wait on the IRP, if required...
     if (STATUS_PENDING==status)
	 {
	    DbgPrint("Waiting on IRP (associate)...");

		// must run at PASSIVE_LEVEL
		ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
		KeWaitForSingleObject(&AssociateEvent, Executive, KernelMode, FALSE, 0);
	 }

	 if ((STATUS_SUCCESS!=status) 
		 && 
		 (STATUS_PENDING!=status))
	 {
		 //something is wrong
		 DbgPrint("IoCallDriver failed (associate), status 0x%08X", status);
		 return STATUS_UNSUCCESSFUL;
	 }

	 if ((STATUS_PENDING==status)
		 &&
		 (STATUS_SUCCESS!=IoStatus.Status))
	 {
		 //something is wrong
		 DbgPrint("Completion of IRP failed (associate), status 0x%08X", IoStatus.Status);
		 return STATUS_UNSUCCESSFUL;
	 }

	////////////////////////////////////////////////////////////////////////////////
	// Step 4. Connect to a remote server!  Woot!
	///////////////////////////////////////////////////////////////////////////////
	KeInitializeEvent(&ConnectEvent, NotificationEvent, FALSE);

	// build an IRP to connect to a remote host
	pIrp =
	TdiBuildInternalDeviceControlIrp(	TDI_CONNECT,
                                        pTcpDevObj,   // TDI driver's device object.
                                        pConnFileObj, // Connection (endpoint) file object.
                                        &ConnectEvent,       // Event to be signalled when Irp completes.
                                        &IoStatus     // I/O status block.
                                       );

	if(NULL==pIrp) 
	{
		DbgPrint("Could not get an IRP for TDI_CONNECT");
		return(STATUS_INSUFFICIENT_RESOURCES);
	}

	// Initialize the IP address structure
	RemotePort = HTONS(445);
	RemoteAddr = INETADDR(192,168,0,223);

	RmtIPAddr.TAAddressCount = 1;
	RmtIPAddr.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
	RmtIPAddr.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
	RmtIPAddr.Address[0].Address[0].sin_port = RemotePort;
	RmtIPAddr.Address[0].Address[0].in_addr = RemoteAddr;

	RmtNode.UserDataLength = 0;
	RmtNode.UserData = 0;
	RmtNode.OptionsLength = 0;
	RmtNode.Options = 0;
	RmtNode.RemoteAddressLength = sizeof(RmtIPAddr);
	RmtNode.RemoteAddress = &RmtIPAddr;

	// add the IP connection data to the IRP
	TdiBuildConnect( pIrp,
                     pTcpDevObj,                      // TDI driver's device object.
                     pConnFileObj,                    // Connection (endpoint) file object.
                     NULL,                            // I/O completion routine.
                     NULL,                            // Context for I/O completion routine.
                     NULL,                            // Address of timeout interval.
                     &RmtNode,                        // Remote-node client address.
                     0                                // (Output) remote-node address.
                    );

	// set our own completion routine
	 // must run at PASSIVE_LEVEL
	 ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	 IoSetCompletionRoutine( pIrp, TDICompletionRoutine, &ConnectEvent, TRUE, TRUE, TRUE);
	 
	 //make the call
	 // must run at <= DISPATCH_LEVEL
	 ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

	// Send the command to the underlying TDI driver
	status = IoCallDriver(pTcpDevObj, pIrp);

	// wait on the IRP, if required...
	if (STATUS_PENDING==status)
	{
		DbgPrint("Waiting on IRP (connect)...");
		KeWaitForSingleObject(&ConnectEvent, Executive, KernelMode, FALSE, 0);
	}

	if ((STATUS_SUCCESS!=status) 
		&& 
		(STATUS_PENDING!=status))
	{
		//something is wrong
		DbgPrint("IoCallDriver failed (connect), status 0x%08X", status);
		return STATUS_UNSUCCESSFUL;
	}

	if ((STATUS_PENDING==status)
		&&
		(STATUS_SUCCESS!=IoStatus.Status))
	{
		//something is wrong
		DbgPrint("Completion of IRP failed (connect), status 0x%08X", IoStatus.Status);
		return STATUS_UNSUCCESSFUL;
	}


	return STATUS_SUCCESS;
}

