
#include "ntddk.h"

// important!! place this before ndis.h
#define NDIS40	1

#include "ndis.h"
#include "stdio.h"

//////////////////////////////////////////////
// prototypes for all our network callbacks
//////////////////////////////////////////////
VOID OnOpenAdapterDone	( IN NDIS_HANDLE ProtocolBindingContext, IN NDIS_STATUS Status, IN NDIS_STATUS OpenErrorStatus );
VOID OnCloseAdapterDone	( IN NDIS_HANDLE ProtocolBindingContext, IN NDIS_STATUS Status );
VOID OnSendDone			( IN NDIS_HANDLE ProtocolBindingContext, IN PNDIS_PACKET pPacket, IN NDIS_STATUS Status );
VOID OnTransferDataDone	( IN NDIS_HANDLE ProtocolBindingContext, IN PNDIS_PACKET Packet, IN NDIS_STATUS Status, IN UINT BytesTransferred );
VOID OnResetDone		( IN NDIS_HANDLE ProtocolBindingContext, IN NDIS_STATUS Status );
VOID OnRequestDone		( IN NDIS_HANDLE ProtocolBindingContext, IN PNDIS_REQUEST pRequest, IN NDIS_STATUS Status );
NDIS_STATUS OnReceiveStub      ( IN NDIS_HANDLE ProtocolBindingContext, IN NDIS_HANDLE MacReceiveContext, IN PVOID HeaderBuffer, IN UINT HeaderBufferSize, IN PVOID LookAheadBuffer, IN UINT LookaheadBufferSize, IN UINT PacketSize );
VOID OnReceiveDoneStub	( IN NDIS_HANDLE ProtocolBindingContext );
VOID OnStatus			( IN NDIS_HANDLE ProtocolBindingContext, IN NDIS_STATUS Status, IN PVOID StatusBuffer, IN UINT StatusBufferSize );
VOID OnStatusDone		( IN NDIS_HANDLE ProtocolBindingContext );
VOID OnBindAdapter		( OUT PNDIS_STATUS theStatus, IN NDIS_HANDLE theBindContext, IN PNDIS_STRING theDeviceNameP, IN PVOID theSS1, IN PVOID theSS2 );
VOID OnUnbindAdapter	( OUT PNDIS_STATUS theStatus, IN NDIS_HANDLE theBindContext, IN PNDIS_HANDLE theUnbindContext );
VOID OnUnload			( IN PDRIVER_OBJECT DriverObject );

NDIS_STATUS	OnPNPEvent(	  IN NDIS_HANDLE	ProtocolBindingContext,
						  IN PNET_PNP_EVENT	pNetPnPEvent);

VOID OnProtocolUnload( VOID );
INT	 OnReceivePacket( IN	NDIS_HANDLE				ProtocolBindingContext,
					  IN	PNDIS_PACKET			Packet );

struct UserStruct
{
	ULONG	mData;
} gUserStruct;

// handle to the open network adapter
NDIS_HANDLE		gAdapterHandle;
NDIS_HANDLE		gNdisProtocolHandle;
NDIS_EVENT		gCloseWaitEvent;

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	UINT			aMediumIndex = 0;
	NDIS_STATUS		aStatus, anErrorStatus;
	NDIS_MEDIUM    	aMediumArray=NdisMedium802_3;		// we only try 802.3
	UNICODE_STRING	anAdapterName;
	NDIS_PROTOCOL_CHARACTERISTICS	aProtocolChar;
	NDIS_STRING	aProtoName = NDIS_STRING_CONST("ROOTKIT_NET");
	
	DbgPrint("ROOTKIT Loading...");

	/////////////////////////////////////////////////////////////////////////
	// Very Important !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Hard code this binding string to the adapter you wish to sniff
	//
	// obtain this from the registry:
	// HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\NetworkCards
	// -or-
	// HKLM\SYSTEM\CurrentControlSet\Services\TcpIp\Linkage
	//
	//
	/*
			On my system I have the following linkages:

			\Device\{6C0B978B-812D-4621-A30B-FD72F6C446AF}	ORiNOCO Wireless LAN PC Card (5 volt)
			\Device\{E30AAA3E-044E-40D3-A8FE-64CC01F2B9B5}
			\Device\{5436B920-2709-4250-918D-B4ED3BB8CF9A}	Dell TrueMobile 1150 Series Wireless LAN Mini PCI Card
			\Device\{5A6C6428-C5F2-4BA5-A469-49F607B369F2}	1394 Net Adapter
			\Device\{357AC276-D8E7-47BF-954D-F3123D3319BD}	3Com 3C920 Integrated Fast Ethernet Controller (3C905C-TX Compatible)
			\Device\{6D615BDB-A6C2-471D-992E-4C0B431334F1}	1394 Net Adapter
			\Device\{83EE41D0-5088-4CC7-BC99-CEA55D5662D2}	3Com 3C920 Integrated Fast Ethernet Controller (3C905C-TX Compatible)
			\Device\NdisWanIp
			\Device\{147E65D7-4065-4249-8679-F79DB39CFC27}
			\Device\{6AB35A1D-6D0B-45CA-9F1C-CD125F950D6F}
	*/
	//
	// the format of the string is \Device\{GUID}
	/////////////////////////////////////////////////////////////////////////	
	RtlInitUnicodeString( &anAdapterName, L"\\Device\\{D6D262BB-0814-46F9-8AC0-117601895E36}" ); 
	
	// init sync event for close
	NdisInitializeEvent(&gCloseWaitEvent);

	//__asm int 3

	theDriverObject->DriverUnload  = OnUnload; 


	////////////////////////////////////////////////////////////////
	// init network sniffer - this is all standard and
	// documented in the DDK.
	////////////////////////////////////////////////////////////////
	RtlZeroMemory( &aProtocolChar,
			sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
	aProtocolChar.MajorNdisVersion            = 4;
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
	
	aProtocolChar.BindAdapterHandler		  = OnBindAdapter;
	aProtocolChar.UnbindAdapterHandler	      = OnUnbindAdapter;
    aProtocolChar.UnloadHandler				  = OnProtocolUnload;

	aProtocolChar.ReceivePacketHandler	      = OnReceivePacket;
	aProtocolChar.PnPEventHandler			  = OnPNPEvent;
	

	DbgPrint("ROOTKIT: Registering NDIS Protocol\n");

	// we have to register a protocol before we can bind to the
	// MAC
	NdisRegisterProtocol(	&aStatus,
        					&gNdisProtocolHandle,
        					&aProtocolChar,
        					sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

	if (aStatus != NDIS_STATUS_SUCCESS) 
	{
		char _t[255];
		_snprintf(_t, 253, "DriverEntry: ERROR NdisRegisterProtocol failed with error 0x%08X", aStatus); 
		DbgPrint(_t);
		return aStatus;
	}

	// NdisOpenAdapter opens a connection between the protocol
	// and the physical adapter (MAC layer)
	NdisOpenAdapter(
		&aStatus,									// return code
		&anErrorStatus,								// return code
		&gAdapterHandle,							// returns a handle to the binding
		&aMediumIndex,								// ptr to int which is an
													// index into a 'medium' array - indicates what
													// the MAC should be 'viewed' as
		&aMediumArray,								// array of 'medium' types
		1,											// number of elements in the 'medium' array
		gNdisProtocolHandle,						// the handle returned from NdisRegisterProtocol
		&gUserStruct,								// ptr to a user controlled structure, this is up to the programmer
		&anAdapterName,								// name of the adapter to be opened
		0,											// bit mask of options
		NULL);										// ptr to additional info to pass to MacOpenAdapter

	if (aStatus != NDIS_STATUS_PENDING) 
	{
		if(FALSE == NT_SUCCESS(aStatus))
		{
			/////////////////////////////////////////////////////////////////////////////////
			// something bad happened, close everything down
			/////////////////////////////////////////////////////////////////////////////////
			char _t[255];
			_snprintf(_t, 253, "ROOTKIT: NdisOpenAdapter returned an error 0x%08X", aStatus); 
			DbgPrint(_t);
			
			// helpful hint
			if(NDIS_STATUS_ADAPTER_NOT_FOUND == aStatus)
			{
				DbgPrint("NDIS_STATUS_ADAPTER_NOT_FOUND");
			}

			// Remove the protocol or suffer a BSOD!
			NdisDeregisterProtocol( &aStatus, gNdisProtocolHandle);
			if(FALSE == NT_SUCCESS(aStatus))
			{
				DbgPrint("DeregisterProtocol failed!");
			}
			
			// use for winCE -- NdisFreeEvent(gCloseWaitEvent);

			return STATUS_UNSUCCESSFUL;
		}
		else
		{
			OnOpenAdapterDone(
				&gUserStruct,
				aStatus,
				NDIS_STATUS_SUCCESS
				);
		}
	}
	
	return STATUS_SUCCESS;
}

VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	NDIS_STATUS        Status;

	DbgPrint("ROOTKIT: OnUnload called\n");

	NdisResetEvent(&gCloseWaitEvent);

	NdisCloseAdapter(
		&Status, 
		gAdapterHandle);

	// we must wait for this to complete
	// ---------------------------------
	if(Status == NDIS_STATUS_PENDING)
	{
		DbgPrint("rootkit: OnUnload: pending wait event\n");
		NdisWaitEvent(&gCloseWaitEvent, 0);
	}

	NdisDeregisterProtocol( &Status, gNdisProtocolHandle);
	if(FALSE == NT_SUCCESS(Status))
	{
		DbgPrint("DeregisterProtocol failed!");
	}

	// use for winCE -- NdisFreeEvent(gCloseWaitEvent);

	DbgPrint("rootkit: OnUnload: NdisCloseAdapter() done\n");
}

VOID 
OnOpenAdapterDone( IN NDIS_HANDLE ProtocolBindingContext, 
				   IN NDIS_STATUS Status, 
				   IN NDIS_STATUS OpenErrorStatus ) 
{
	NDIS_REQUEST      anNdisRequest;
	NDIS_STATUS       anotherStatus;
	ULONG			  aMode = NDIS_PACKET_TYPE_PROMISCUOUS;

	DbgPrint("ROOTKIT: OnOpenAdapterDone called\n");

	if(NT_SUCCESS(OpenErrorStatus))
	{
		// put the card into promiscuous mode	
		anNdisRequest.RequestType = NdisRequestSetInformation;
		anNdisRequest.DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_PACKET_FILTER;
		anNdisRequest.DATA.SET_INFORMATION.InformationBuffer = &aMode;
		anNdisRequest.DATA.SET_INFORMATION.InformationBufferLength = sizeof(ULONG);

		NdisRequest(	&anotherStatus,
						gAdapterHandle,
						&anNdisRequest
						);					
	}
	else
	{
		char _t[255];
		_snprintf(_t, 252, "OnOpenAdapterDone called with error code 0x%08X", OpenErrorStatus); 
		DbgPrint(_t);
	}
}

VOID 
OnCloseAdapterDone( IN NDIS_HANDLE ProtocolBindingContext, 
				    IN NDIS_STATUS Status ) 
{
	DbgPrint("ROOTKIT: OnCloseAdapterDone called\n");

	// sync with unload event
	NdisSetEvent(&gCloseWaitEvent);
}

VOID 
OnSendDone( IN NDIS_HANDLE ProtocolBindingContext, 
		    IN PNDIS_PACKET pPacket, 
			IN NDIS_STATUS Status ) 
{

	DbgPrint("ROOTKIT: OnSendDone called\n");
}

VOID 
OnTransferDataDone ( IN NDIS_HANDLE thePBindingContext, 
					 IN PNDIS_PACKET thePacketP, 
					 IN NDIS_STATUS theStatus, 
					 IN UINT theBytesTransfered ) 
{
	DbgPrint("ROOTKIT: OnTransferDataDone called\n");
}

/* a packet has arrived */
NDIS_STATUS 
OnReceiveStub( IN NDIS_HANDLE ProtocolBindingContext, /* our open structure */
			   IN NDIS_HANDLE MacReceiveContext, 
			   IN PVOID HeaderBuffer, /* ethernet header */
			   IN UINT HeaderBufferSize, 
			   IN PVOID LookAheadBuffer, /* it is possible to have entire packet in here */
			   IN UINT LookaheadBufferSize, 
			   UINT PacketSize ) 
{
	char _t[255];
	UINT aFrameType = 0;
	
	// report the frame type to the debugger
	memcpy(&aFrameType, ( ((char *)HeaderBuffer) + 12), 2);
	_snprintf(_t, 253, "sniffed frame type %u, packetsize %u", aFrameType, PacketSize);
	DbgPrint(_t);


	// ignore everything
	return NDIS_STATUS_NOT_ACCEPTED;
}

VOID 
OnReceiveDoneStub( IN NDIS_HANDLE ProtocolBindingContext ) 
{
	DbgPrint("ROOTKIT: OnReceiveDoneStub called\n");
    return;
}

VOID 
OnStatus( IN NDIS_HANDLE ProtocolBindingContext, 
		  IN NDIS_STATUS Status, 
		  IN PVOID StatusBuffer, 
		  IN UINT StatusBufferSize ) 
{
    DbgPrint("ROOTKIT: OnStatus called\n");
	return;
}

VOID 
OnStatusDone( IN NDIS_HANDLE ProtocolBindingContext ) 
{
	DbgPrint("ROOTKIT:OnStatusDone called\n");
    return;
}

VOID OnResetDone( IN NDIS_HANDLE ProtocolBindingContext, 
				  IN NDIS_STATUS Status ) 
{   
	DbgPrint("ROOTKIT: OnResetDone called\n");
    return;
}

VOID 
OnRequestDone( IN NDIS_HANDLE ProtocolBindingContext, 
			   IN PNDIS_REQUEST NdisRequest, 
			   IN NDIS_STATUS Status ) 
{
	DbgPrint("ROOTKIT: OnRequestDone called\n");
    return;
}

VOID OnBindAdapter(		OUT PNDIS_STATUS theStatus, 
						IN NDIS_HANDLE theBindContext, 
						IN PNDIS_STRING theDeviceNameP, 
						IN PVOID theSS1, 
						IN PVOID theSS2 )
{
	DbgPrint("ROOTKIT: OnBindAdapter called\n");
    return;
}

VOID OnUnbindAdapter(	OUT PNDIS_STATUS theStatus, 
						IN NDIS_HANDLE theBindContext, 
						IN PNDIS_HANDLE theUnbindContext )
{
	DbgPrint("ROOTKIT: OnUnbindAdapter called\n");
    return;
}

NDIS_STATUS	OnPNPEvent(	  IN NDIS_HANDLE	ProtocolBindingContext,
						  IN PNET_PNP_EVENT	pNetPnPEvent)
{
	DbgPrint("ROOTKIT: PtPnPHandler called");
	return NDIS_STATUS_SUCCESS;
}

VOID OnProtocolUnload( VOID )
{
	DbgPrint("ROOTKIT: OnProtocolUnload called");
	return;
}

INT	 OnReceivePacket( IN	NDIS_HANDLE				ProtocolBindingContext,
					  IN	PNDIS_PACKET			Packet )
{
	DbgPrint("ROOTKIT: OnReceivePacket called\n");
	return 0;
}
