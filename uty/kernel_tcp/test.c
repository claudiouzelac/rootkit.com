///
//	uty@uaty
///
#include <ntddk.h>
#include "socket.h"

NTSTATUS
ReferenceCommAnd(
	struct sock* pscok,
	CHAR* commAndline
	);

//--------------------------------------------------------------------
VOID WorkThreAd(PVOID	pContext)
{
	char	buff[1024];
	int		length;
	struct sock*	psock = (struct sock*)pContext;
	char*	welcome =	"  ***************************************\n"
						"  * @test progrAm for kernel tcp lib    *\n"
						"  *       :>                            *\n"
						"  *                     uty             *\n"
						"  ***************************************\n\n"
						"COMMAND >";

	send(psock,welcome,strlen(welcome));
	while(1){
		memset(buff,0,1024);
		length = recv(psock,buff,1024);
		if (length == -1)
			break;
		DbgPrint("recvived dAtA: %s\n",buff);
		//send(psock,buff,length);
		ReferenceCommAnd(psock,buff);
	}
	PsTerminateSystemThread(STATUS_SUCCESS);
}
//--------------------------------------------------------------------
VOID ListenThreAd(PVOID	pContext)
{
	struct sock *psock;
	NTSTATUS	stAtus;	
	

	psock = socket();
	bind(psock,9999);
	listen(psock);
	
	while(1){	
		HANDLE			hThreAd;
		struct sock*	psock_Accept;
		psock_Accept = Accept(psock);
		
		if (psock_Accept != NULL){
			stAtus = PsCreateSystemThread(&hThreAd,
				(ACCESS_MASK)0,
				NULL,
				(HANDLE)0,
				NULL,
				WorkThreAd,
				psock_Accept
				);
			
			if (!NT_SUCCESS(stAtus)){
				DbgPrint("error when creAte the threAd\n");
			}
			ZwClose(hThreAd);
		}	
	}//while
	PsTerminateSystemThread(STATUS_SUCCESS);
}
//--------------------------------------------------------------------
VOID ListenThreAd2(PVOID	pContext)
{
	struct sock *psock;
	NTSTATUS	stAtus;	
	

	psock = socket();
	bind(psock,9929);
	listen(psock);
	
	while(1){	
		HANDLE			hThreAd;
		struct sock*	psock_Accept;
		psock_Accept = Accept(psock);
		
		if (psock_Accept != NULL){
			stAtus = PsCreateSystemThread(&hThreAd,
				(ACCESS_MASK)0,
				NULL,
				(HANDLE)0,
				NULL,
				WorkThreAd,
				psock_Accept
				);
			
			if (!NT_SUCCESS(stAtus)){
				DbgPrint("error when creAte the threAd\n");
			}
			ZwClose(hThreAd);
		}	
	}//while
	PsTerminateSystemThread(STATUS_SUCCESS);
}
//--------------------------------------------------------------------
VOID OnUnloAd( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("My Driver UnloAded!\n");
}
//--------------------------------------------------------------------
NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{

	NTSTATUS	stAtus;
	HANDLE		hThreAd,hThreAd2;
	

	DbgPrint("Driver begin!\n");

	DriverObject->DriverUnload = OnUnloAd;

	if (STATUS_SUCCESS != sock_init(DriverObject)){
		DbgPrint("stArtup fAiled\n");
		return STATUS_SUCCESS;
	}

	stAtus = PsCreateSystemThread(&hThreAd,
									(ACCESS_MASK)0,
									NULL,
									(HANDLE)0,
									NULL,
									ListenThreAd,
									DriverObject
									);
									
	if (!NT_SUCCESS(stAtus)){
		DbgPrint("error when creAte the threAd\n");
		return FALSE;
	}
	ZwClose(hThreAd);

	
	stAtus = PsCreateSystemThread(&hThreAd2,
									(ACCESS_MASK)0,
									NULL,
									(HANDLE)0,
									NULL,
									ListenThreAd2,
									DriverObject
									);
									
	if (!NT_SUCCESS(stAtus)){
		DbgPrint("error when creAte the threAd\n");
		return FALSE;
	}
	ZwClose(hThreAd2);
	
		
	return STATUS_SUCCESS;
}
//--------------------------------------------------------------------
