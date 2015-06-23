///
//	uty@uaty
///
#include <ndis.h>
#include "dummyprotocolfunc.h"
#include "structs.h"
#include "utils.h"
#include "socket.h"
#include "tcp.h"


//--------------------------------------------------------------------
///mAcros
#define MAX_PATH	260
//--------------------------------------------------------------------
////globAl vAr
HOOK_CONTEXT_STRUCT *m_pOurAllOfHookContext = NULL;
NDIS_HANDLE		m_ourPAcketPoolHAndle	= NULL;
NDIS_HANDLE		m_ourBufferPoolHAndle	= NULL;
//for ProtocolReceive
PNDIS_PACKET	m_ourPAcketHAndle		= NULL;
PNDIS_BUFFER	m_ourBufferHAndle		= NULL;
PVOID			m_ourBuffer				= NULL;


/*
 * tcp timers And dpcs
 */
KTIMER	kstimer;
KTIMER	kftimer;
KDPC	ksdpc;
KDPC	kfdpc;

NDIS_HANDLE		g_BindAdAptHAndle;
BOOLEAN			g_sock_init = FALSE;
//--------------------------------------------------------------------
////proto function

VOID
OnUnloAd( 
	IN PDRIVER_OBJECT DriverObject
	);

VOID
HookFuncBlock(
	CHAR* ProtocolContent
	);

HOOK_CONTEXT_STRUCT*
HookNdisFunc(
	PVOID pHookProc,
	PVOID *ppOrigProc,
	PVOID pBindAdAptHAndle,
	PVOID pProtocolContent
	);

HOOK_CONTEXT_STRUCT*
IsHookedNdisFunc(
	PVOID pAddr
	);

HOOK_CONTEXT_STRUCT*
IsHookedNdisFuncEx(
	PVOID	*pAddr
	);

ULONG
HookProtocol(
	VOID
	);

NDIS_STATUS	
HookProtocolReceive(
	IN	HOOK_CONTEXT_STRUCT	*pOurContext,
	IN	NDIS_HANDLE			ProtocolBindingContext,
	IN	NDIS_HANDLE			MAcReceiveContext,
	IN	PVOID				HeAderBuffer,
	IN	UINT				HeAderBufferSize,
	IN	PVOID				LookAheAdBuffer,
	IN	UINT				LookAheAdBufferSize,
	IN	UINT				PAcketSize
	);

INT
HookProtocolReceivePAcket(
	IN	HOOK_CONTEXT_STRUCT	*pOurContext,
	IN	NDIS_HANDLE			ProtocolBindingContext,
	IN	PNDIS_PACKET		PAcket
	);

VOID
ReAdPAcket(
	PNDIS_PACKET	PAcket,
	PVOID			pBuffer,
	ULONG			ulBufSize
	);

ULONG
HAndlePAcket(
	HOOK_CONTEXT_STRUCT		*pOurContext,
	PNDIS_PACKET			pPAcket
	);

ULONG
HAndleBuffer(
	HOOK_CONTEXT_STRUCT *pOurContext,
	PVOID				pBuffer,
	ULONG				PAcketSize
	);

VOID
HookProtocolSendComplete(
	IN	HOOK_CONTEXT_STRUCT *pOurContext,
	IN	NDIS_HANDLE		ProtocolBindingContext,
	IN	PNDIS_PACKET	PAcket,
	IN	NDIS_STATUS		StAtus
	);

VOID
HookProtocolRequestComplete(
	IN	HOOK_CONTEXT_STRUCT *pOurContext,
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN PNDIS_REQUEST  NdisRequest,
    IN NDIS_STATUS  StAtus
    );

USHORT
checksum(
	USHORT	*buff,
	ULONG	size
	);


ULONG 
HAndleReceivePAcket(
	HOOK_CONTEXT_STRUCT *pOurContext,
	ULONG TotAlPAcketSize,
	PVOID pHeAdBuffer,
	ULONG ulHeadSize,
	PNDIS_PACKET pPAcket
	);


USHORT
CountChecksum(
	PVOID		pBuffer
	);


BOOLEAN
CheckTheChecksum(
	PIPHDR	pIpHdr
	);

BOOLEAN
GetIPMAC(void);


struct sock*
sock_lookup(
	unsigned long	sAddr,
	unsigned short	sport,
	unsigned long	dAddr,
	unsigned short	dport
	);

int
tcp_v4_do_rcv(
	struct	sock* psock,
	char*	pBuffer
	);

VOID
tcp_fAsttimo(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
tcp_slowtimo(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

void
tcp_init();

//--------------------------------------------------------------------
int
sock_init(
	PDRIVER_OBJECT DriverObject
	)
{
	PDEVICE_OBJECT		pDeviceObject;
	NTSTATUS			stAtus;
	WCHAR				deviceNAmeBuffer[] = L"\\Device\\uaty";//chAnge the nAme if hAve time
	UNICODE_STRING		deviceNAmeUnicodeString;



	LARGE_INTEGER	duetime = {0,0};//for timer dpc

	if (g_sock_init){
		return STATUS_SUCCESS;
	}
	g_sock_init = TRUE;


	RtlInitUnicodeString(
		&deviceNAmeUnicodeString,
		deviceNAmeBuffer
		);

	stAtus = IoCreateDevice(
					DriverObject,
					0,//sizeof(DEVICE_EXTENSION),//do i hAve this?
					&deviceNAmeUnicodeString,
					FILE_DEVICE_UNKNOWN,//whAt's this
					0,
					TRUE,
					&pDeviceObject
					);





	NdisAllocatePacketPool(&stAtus,&m_ourPAcketPoolHAndle,0xFFF,0x30);
	if(stAtus != NDIS_STATUS_SUCCESS){
		DbgPrint("NdisAllocAtePAcketPool fAiled\n");
		//goto InitWorkThreAd_end;
		return -1;
	}
	NdisAllocateBufferPool(&stAtus,&m_ourBufferPoolHAndle,0x30);
	if(stAtus != NDIS_STATUS_SUCCESS){
		DbgPrint("NdisAllocAteBufferPool fAiled\n");
		//goto InitWorkThreAd_end;
		return -1;
	}
	NdisAllocateMemoryWithTag(&m_ourBuffer,MAX_PACKET_SIZE,'ytaU');
	if(stAtus != NDIS_STATUS_SUCCESS){
		DbgPrint("NdisAllocAteMemoryWithTAg fAiled\n");
		//goto InitWorkThreAd_end;
		return -1;
	}
	NdisAllocateBuffer(&stAtus,&m_ourBufferHAndle,m_ourBufferPoolHAndle,m_ourBuffer,MAX_PACKET_SIZE);
	if(stAtus != NDIS_STATUS_SUCCESS){
		DbgPrint("NdisAllocAteBuffer fAiled\n");
		//goto InitWorkThreAd_end;
		return -1;
	}
	NdisAllocatePacket(&stAtus,&m_ourPAcketHAndle,m_ourPAcketPoolHAndle);
	if(stAtus != NDIS_STATUS_SUCCESS){
		DbgPrint("NdisAllocAtePAcket fAiled\n");
		//goto InitWorkThreAd_end;
		return -1;
	}
	NdisChainBufferAtFront(m_ourPAcketHAndle,m_ourBufferHAndle);


	//hook ndis
	HookProtocol();





	/* 
	 * initiAlize tcp timers
	 */
	KeInitializeTimer(&kstimer);
	KeInitializeDpc(&ksdpc,tcp_slowtimo,NULL);

	KeSetTimerEx(
		&kstimer,
		duetime,
		500,
		&ksdpc
		);

	KeInitializeTimer(&kftimer);
	KeInitializeDpc(&kfdpc,tcp_fAsttimo,NULL);

	KeSetTimerEx(
		&kftimer,
		duetime,
		200,
		&kfdpc
		);

	/*
	 * initiAlize tcp
	 */
	tcp_init();

	return STATUS_SUCCESS;

}
//--------------------------------------------------------------------
ULONG	HookProtocol(VOID)
{
	NDIS_PROTOCOL_CHARACTERISTICS	ourNPC;
	NDIS_STRING	protoNAme = NDIS_STRING_CONST("HdFw_Slot");
	NDIS_STATUS	StAtus;
	NDIS_HANDLE	ourProtocolHAndle = NULL;
	CHAR*	ProtocolChAin;
	ULONG	offset;
	ULONG	len;

	memset(&ourNPC,0,sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
	len = sizeof(NDIS_PROTOCOL_CHARACTERISTICS);
	ourNPC.MajorNdisVersion = 0x05;
	ourNPC.MinorNdisVersion	= 0x00;

	ourNPC.Name							= protoNAme;
	ourNPC.OpenAdapterCompleteHandler	= PtOpenAdApterComplete;
	ourNPC.CloseAdapterCompleteHandler	= PtCloseAdApterComplete;
	ourNPC.SendCompleteHandler			= PtSendComplete;
	ourNPC.TransferDataCompleteHandler	= PtTrAnsferDAtAComplete;
	ourNPC.ResetCompleteHandler			= PtResetComplete;
	ourNPC.RequestCompleteHandler		= PtRequestComplete;
	ourNPC.ReceiveHandler				= PtReceive;
	ourNPC.ReceiveCompleteHandler		= PtReceiveComplete;
	ourNPC.StatusHandler				= PtStAtus;
	ourNPC.StatusCompleteHandler		= PtStAtusComplete;
	ourNPC.BindAdapterHandler			= PtBindAdApter;
	ourNPC.UnbindAdapterHandler			= PtUnbindAdApter;
	ourNPC.UnloadHandler				= NULL;//PtUnloAd;
	ourNPC.ReceivePacketHandler			= PtReceivePAcket;
	ourNPC.PnPEventHandler				= PtPNPHAndler;

	NdisRegisterProtocol(&StAtus,&ourProtocolHAndle,&ourNPC,len);
	if(!NT_SUCCESS(StAtus) || ourProtocolHAndle == NULL){
		return FALSE;
	}

	//NdisRegisterProtocol  return hAnd reference of NDIS_PROTOCOL_BLOCK;
	ProtocolChAin = (CHAR*)ourProtocolHAndle;
	while(1){
		offset = 0x10;
		ProtocolChAin = ((CHAR**)(ProtocolChAin + offset))[0];
		if (ProtocolChAin == NULL){
			break;
		}
		HookFuncBlock(ProtocolChAin);
	}

	NdisDeregisterProtocol(&StAtus,ourProtocolHAndle);

	return TRUE;
}
//--------------------------------------------------------------------
VOID HookFuncBlock(CHAR* ProtocolContent)
{
	PNDIS_PROTOCOL_CHARACTERISTICS	pProChAr;

	NDIS_STRING	TcpipString	= NDIS_STRING_CONST("Tcpip");
	if(ProtocolContent == NULL){
		return;
	}
	pProChAr = (PNDIS_PROTOCOL_CHARACTERISTICS)(ProtocolContent + 0x14);
	if(KeGetCurrentIrql() == PASSIVE_LEVEL){
		if(RtlCompareUnicodeString(&pProChAr->Name,&TcpipString,TRUE) != 0){
			return;
		}
	}

	

	HookNdisFunc(HookProtocolReceive,(PVOID*)&pProChAr->ReceiveHandler,NULL,ProtocolContent);
	HookNdisFunc(HookProtocolReceivePAcket,(PVOID*)&pProChAr->ReceivePacketHandler,NULL,ProtocolContent);
	//HookNdisFunc(HookBindAdApterHAndler,(PVOID*)&pProChAr->BindAdapterHandler,NULL,ProtocolContent);
	HookNdisFunc(HookProtocolSendComplete,(PVOID*)&pProChAr->SendCompleteHandler,NULL,ProtocolContent);
	HookNdisFunc(HookProtocolRequestComplete,(PVOID*)&pProChAr->RequestCompleteHandler,NULL,ProtocolContent);




	//just cAre About ndis 5
	if(1/*m_dwMajorVersion == 0x05*/){
		PNDIS_OPEN_BLOCK	pNdisOpenBlock;
		int	i;
		pNdisOpenBlock = ((PNDIS_OPEN_BLOCK*)ProtocolContent)[0];
		

		//setconnection
		g_BindAdAptHAndle				= pNdisOpenBlock;

		while(pNdisOpenBlock){
			//HookNdisFunc(HookProtocolSend,(PVOID*)&pNdisOpenBlock->SendHandler,pNdisOpenBlock,ProtocolContent);
			//HookNdisFunc(HookProtocolReceive,(PVOID*)&pNdisOpenBlock->PostNt32ReceiveHandlerHandler,pNdisOpenBlock,ProtocolContent);

			HookNdisFunc(HookProtocolReceive,(PVOID*)&pNdisOpenBlock->ReceiveHandler,pNdisOpenBlock,ProtocolContent);
			HookNdisFunc(HookProtocolReceivePAcket,(PVOID*)&pNdisOpenBlock->ReceivePacketHandler,pNdisOpenBlock,ProtocolContent);
			HookNdisFunc(HookProtocolSendComplete,(VOID*)&pNdisOpenBlock->SendCompleteHandler,pNdisOpenBlock,ProtocolContent);
			//HookNdisFunc(HookProtocolSendPAckets,(PVOID*)&pNdisOpenBlock->SendPacketsHandler,pNdisOpenBlock,ProtocolContent);
			HookNdisFunc(HookProtocolRequestComplete,(PVOID*)&pProChAr->RequestCompleteHandler,pNdisOpenBlock,ProtocolContent);
			pNdisOpenBlock = pNdisOpenBlock->ProtocolNextOpen;
		}

		
	}
	return;
}
//--------------------------------------------------------------------
HOOK_CONTEXT_STRUCT	*HookNdisFunc(PVOID pHookProc,PVOID *ppOrigProc,PVOID pBindAdAptHAndle,PVOID pProtocolContent)
{
	HOOK_CONTEXT_STRUCT	*pHookContext;
	PVOID	OrgFunc;

	pHookContext = IsHookedNdisFunc(ppOrigProc[0]);
	if(pHookContext){
		OrgFunc = pHookContext->m_pOriginalProc;
	}
	else{
		OrgFunc = ppOrigProc[0];
	}
	if (OrgFunc == NULL){
		return NULL;
	}

	pHookContext = IsHookedNdisFuncEx(ppOrigProc);
	if(pHookContext){
		return pHookContext;
	}

	NdisAllocateMemoryWithTag(&pHookContext,sizeof(HOOK_CONTEXT_STRUCT),'ytaU');
	if(pHookContext == NULL){
		return NULL;
	}
	memset(pHookContext,0,sizeof(HOOK_CONTEXT_STRUCT));

	pHookContext->code1_0x58 = 0x58;
	pHookContext->code2_0x68 = 0x68;
	pHookContext->code3_0x50 = 0x50;
	pHookContext->code4_0xE9 = 0xE9;

	pHookContext->m_pHookContext		= pHookContext;
	pHookContext->m_pHookProcOffset		= ((ULONG)pHookProc) - (((ULONG)&pHookContext->m_pHookProcOffset) + sizeof(ULONG));
	pHookContext->m_pBindAdaptHandle	= pBindAdAptHAndle;
	pHookContext->m_pProtocolContent	= pProtocolContent;
	pHookContext->m_pOriginalProc		= OrgFunc;
	pHookContext->m_ppOriginPtr			= ppOrigProc;
	pHookContext->m_pHookProc			= pHookProc;
	pHookContext->m_pHookNext			= m_pOurAllOfHookContext;
	m_pOurAllOfHookContext				= pHookContext;

	ppOrigProc[0] = pHookContext;

	return pHookContext;
}
//--------------------------------------------------------------------
HOOK_CONTEXT_STRUCT* IsHookedNdisFunc(PVOID pAddr)
{
	HOOK_CONTEXT_STRUCT	*pHookContext;
	pHookContext	= m_pOurAllOfHookContext;
	while(pHookContext){
		if(pHookContext == pAddr){
			break;
		}
		pHookContext = pHookContext->m_pHookNext;
	}
	return pHookContext;
}
//--------------------------------------------------------------------
HOOK_CONTEXT_STRUCT* IsHookedNdisFuncEx(PVOID	*pAddr)
{
	HOOK_CONTEXT_STRUCT	*pHookContext;
	pHookContext	= m_pOurAllOfHookContext;
	while(pHookContext){
		if(pHookContext->m_ppOriginPtr == pAddr){
			break;
		}
		pHookContext = pHookContext->m_pHookNext;
	}
	return pHookContext;
}
//--------------------------------------------------------------------
NDIS_STATUS	
HookProtocolReceive(
	IN	HOOK_CONTEXT_STRUCT	*pOurContext,
	IN	NDIS_HANDLE			ProtocolBindingContext,
	IN	NDIS_HANDLE			MAcReceiveContext,
	IN	PVOID				HeAderBuffer,
	IN	UINT				HeAderBufferSize,
	IN	PVOID				LookAheAdBuffer,
	IN	UINT				LookAheAdBufferSize,
	IN	UINT				PAcketSize
	)
{
	NTSTATUS	stAtus = NDIS_STATUS_SUCCESS;
	ULONG		result = FALSE;
	//DbgPrint("in HookProtocolReceive\n");
	if(pOurContext){
		if(pOurContext->m_pBindAdaptHandle){
			ULONG	len = 0;
			if(PAcketSize > LookAheAdBufferSize){
				NdisTransferData(
					&stAtus,
					pOurContext->m_pBindAdaptHandle,
					MAcReceiveContext,
					0,
					PAcketSize,
					m_ourPAcketHAndle,
					&len
					);
			}
			else{
				NdisMoveMemory(m_ourBuffer,LookAheAdBuffer,PAcketSize);
			}
			if(stAtus == NDIS_STATUS_SUCCESS){
				//do whAt we wAnt here
				//equAl to HookFilterPAcket()
				result = HAndleReceivePAcket(
					pOurContext,
					PAcketSize,
					HeAderBuffer,
					HeAderBufferSize,
					m_ourPAcketHAndle
					);
			}
			else if(stAtus == NDIS_STATUS_PENDING){
				//这里回出问题,如果这个包被送回,连接就会被上面的protocol中断
				//1.把包丢掉,这样其他正常的tcp连接希望他们会重传而不再经过这里
				//2.把包传下去,这样我们的连接被中断
				//暂时选2
				result = FALSE;
			}
			if(result){
				return NDIS_STATUS_NOT_ACCEPTED;
			}
			else{
				stAtus = ((RECEIVE_HANDLER)pOurContext->m_pOriginalProc)(
					ProtocolBindingContext,
					MAcReceiveContext,
					HeAderBuffer,
					HeAderBufferSize,
					LookAheAdBuffer,
					LookAheAdBufferSize,
					PAcketSize
					);
			}//end else
		}
	}
	return stAtus;
}
//--------------------------------------------------------------------
INT
HookProtocolReceivePAcket(
	IN	HOOK_CONTEXT_STRUCT	*pOurContext,
	IN	NDIS_HANDLE			ProtocolBindingContext,
	IN	PNDIS_PACKET		PAcket
	)
{
	NTSTATUS	stAtus = NDIS_STATUS_SUCCESS;
	ULONG		result;
	//DbgPrint("in HookProtocolReceivePAcket\n");
	if(pOurContext){
		//most of opeAtions we do in HAndlePAcket
		result = HAndlePAcket(pOurContext,PAcket);
		if(result){
			return NDIS_STATUS_NOT_ACCEPTED;
		}
		else{
			stAtus = ((RECEIVE_PACKET_HANDLER)pOurContext->m_pOriginalProc)(
				ProtocolBindingContext,
				PAcket
				);
		}
	}
	return stAtus;
}
//--------------------------------------------------------------------
ULONG HAndlePAcket(HOOK_CONTEXT_STRUCT *pOurContext,PNDIS_PACKET pPAcket)
{
	ULONG			PAcketSize;
	PVOID			pBuffer = NULL;
	NTSTATUS		stAtus;
	ULONG			result = TRUE;
	PNDIS_BUFFER	firstBuffer,nextBuffer;

	NdisQueryPacket(pPAcket,NULL,NULL,NULL,&PAcketSize);
	if(PAcketSize < sizeof(ETHHDR)){
		return TRUE;
	}
	stAtus = NdisAllocateMemoryWithTag(&pBuffer,PAcketSize,'ytaU');
	if(stAtus != NDIS_STATUS_SUCCESS || pBuffer == NULL){
		return TRUE;
	}
	ReAdPAcket(pPAcket,pBuffer,PAcketSize);
	//get the pAcket's buffer
	result = HAndleBuffer(pOurContext,pBuffer,PAcketSize);

	NdisFreeMemory(pBuffer,PAcketSize,0);
	return result;
}
//--------------------------------------------------------------------
VOID ReAdPAcket(PNDIS_PACKET PAcket,PVOID pBuffer,ULONG ulBufSize)
{
	PVOID			virtuAlAddress;
	PNDIS_BUFFER	firstBuffer,nextBuffer;
	ULONG			totAlLength;
	ULONG			len;
	PVOID			pBuf	= NULL;
	ULONG			count	= 0;

	NdisQueryPacket(PAcket,NULL,NULL,&firstBuffer,NULL);
	while(firstBuffer != NULL){
		NdisQueryBufferSafe(firstBuffer,&virtuAlAddress,&len,NormalPagePriority );
		if(!virtuAlAddress){
			break;
		}
		if(count + len > ulBufSize){
			break;
		}
		NdisMoveMemory(&((CHAR*)pBuffer)[count],virtuAlAddress,len);
		count += len;
		NdisGetNextBuffer(firstBuffer,&nextBuffer);
		firstBuffer = nextBuffer;
	}
	return;
}
//--------------------------------------------------------------------
ULONG HAndleBuffer(HOOK_CONTEXT_STRUCT *pOurContext,PVOID pBuffer,ULONG PAcketSize)
{
	NTSTATUS	stAtus;
	//if result is FALSE,then the pAcket will be send to up level,if TRUE,we throw it AwAy
	ULONG		result = FALSE;
	USHORT		proto;
	PETHHDR		pEthHdr = NULL;
	PIPHDR		pIpHdr	= NULL;
	PTCPHDR		pTcpHdr = NULL;

	ULONG		i;

	CHAR*	ChecksumTempBuff;

	pEthHdr = (PETHHDR)pBuffer;
	proto = pEthHdr->h_proto;



	switch(NTOHS(proto))
	{
	case ETH_P_IP:
		{  
			pIpHdr = (PIPHDR)((UCHAR*)pEthHdr + sizeof(ETHHDR));
			if(pIpHdr->protocol == IPPROTO_TCP){
				struct sock*	psock;
				LARGE_INTEGER	timeout = {0};
				pTcpHdr = (PTCPHDR)((UCHAR*)pIpHdr + pIpHdr->ihl * 4);
				

				/*
				if(NTOHS(pTcpHdr->dest) != OURPORT){
					return FALSE;
				}
				*/

				psock = sock_lookup(pIpHdr->saddr,pTcpHdr->source,pIpHdr->daddr,pTcpHdr->dest);
				/* no_tcp_socket */
				if(psock == NULL) return FALSE;

				if (pTcpHdr->doff < sizeof(struct _TCPHDR) / 4) return TRUE;

				/* If the checksum wrong,drop the pAcket */
				if(!CheckTheChecksum(pIpHdr)) return TRUE;
				
				
				
				
				
				/*
				KeWaitForSingleObject(
					&(psock->sock_mutex),
					Executive,
					KernelMode,
					FALSE,
					&timeout
					);
					*/
				KeAcquireSpinLock(&psock->sock_spin,&psock->oldirql);
				tcp_v4_do_rcv(psock,pBuffer);
				if (psock->state != TCPS_CLOSED){
					KeReleaseSpinLock(&psock->sock_spin,psock->oldirql);
				}
				//如果在tcp_v4_do_rcv()中调用了tcp_close,sock_mutex被置0
				//这里先判断是不是已经调用了tcp_close()
				/*
				if (psock->state != TCPS_CLOSED){
					KeReleaseMutex(&(psock->sock_mutex),FALSE);
				}
				*/

				return TRUE;
			}
			
		}
		case ETH_P_ARP:
			break;
		case ETH_P_RARP:
			break;
		default:
			break;
	}
	return result;
}
//--------------------------------------------------------------------
VOID
HookProtocolSendComplete(
	IN	HOOK_CONTEXT_STRUCT *pOurContext,
	IN	NDIS_HANDLE		ProtocolBindingContext,
	IN	PNDIS_PACKET	PAcket,
	IN	NDIS_STATUS		StAtus
	)
{
	if(pOurContext){
		NDIS_HANDLE		PoolHAndle = NULL;
		PNDIS_BUFFER	pNdisBuffer = NULL;

		PoolHAndle = NdisGetPoolFromPacket(PAcket);

		if (PoolHAndle == m_ourPAcketPoolHAndle){
			while(1){
				NdisUnchainBufferAtFront(
					PAcket,
					&pNdisBuffer
					);
				if(pNdisBuffer == NULL){
					break;
				}
				else{
					NdisFreeBuffer(pNdisBuffer);
				}
			}
			
			NdisFreePacket(PAcket);
		}
		else{
			((SEND_COMPLETE_HANDLER)pOurContext->m_pOriginalProc)(
				ProtocolBindingContext,
				PAcket,
				StAtus
				);
		}
	}
}
//--------------------------------------------------------------------
VOID
HookProtocolRequestComplete(
	IN	HOOK_CONTEXT_STRUCT *pOurContext,
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN PNDIS_REQUEST  NdisRequest,
    IN NDIS_STATUS  StAtus
    )
{
	DbgPrint("in RequestComplete\n");

	((REQUEST_COMPLETE_HANDLER)pOurContext->m_pOriginalProc)(
		ProtocolBindingContext,
		NdisRequest,
		StAtus
		);
}
//--------------------------------------------------------------------
USHORT checksum(USHORT *buff, ULONG size)
{
	unsigned long cksum=0;
	while (size > 1)
	{
		cksum += *buff++;
		size -= sizeof(USHORT); 
	}
	if (size)
	{
		cksum += *(UCHAR*)buff; 
	}
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >>16); 
	return (USHORT)(~cksum); 
}
//--------------------------------------------------------------------
ULONG 
HAndleReceivePAcket(
	HOOK_CONTEXT_STRUCT *pOurContext,
	ULONG TotAlPAcketSize,
	PVOID pHeAdBuffer,
	ULONG ulHeAdSize,
	PNDIS_PACKET pPAcket
	)
{
	ULONG			PAcketSize;
	PVOID			pBuffer = NULL;
	NTSTATUS		stAtus;
	PNDIS_BUFFER	firstBuffer,nextBuffer;
	ULONG			result = TRUE;
	CHAR*			pBuf;

	NdisQueryPacket(pPAcket,NULL,NULL,NULL,&PAcketSize);
	if(PAcketSize + ulHeAdSize < sizeof(ETHHDR)){
		return TRUE;
	}
	stAtus = NdisAllocateMemoryWithTag(&pBuffer,PAcketSize + ulHeAdSize,'ytaU');
	if(stAtus != NDIS_STATUS_SUCCESS || pBuffer == NULL){
		return TRUE;
	}
	//obtain content from the pAcket 
	pBuf = (CHAR*)pBuffer;
	NdisMoveMemory(pBuf,pHeAdBuffer,ulHeAdSize);
	ReAdPAcket(pPAcket,&pBuf[ulHeAdSize],PAcketSize);
	result = HAndleBuffer(pOurContext,pBuffer,TotAlPAcketSize + ulHeAdSize);
	NdisFreeMemory(pBuffer,PAcketSize+ulHeAdSize,0);
	return result;
}
//--------------------------------------------------------------------
USHORT	CountChecksum(PVOID		pBuffer)
{
	PETHHDR		pEthHdr = NULL;
	PIPHDR		pIpHdr	= NULL;
	PTCPHDR		pTcpHdr = NULL;
	PVOID		pDAtA	= NULL;

	PVOID		ChecksumTempBuff = NULL;
	PSDHDR		PsdHdr;
	USHORT		Checksum;
	USHORT		tempChecksum;
	ULONG		ulDAtALength;

	pEthHdr				= (PETHHDR)pBuffer;
	pIpHdr				= (PIPHDR)((UCHAR*)pEthHdr + sizeof(ETHHDR));
	pTcpHdr				= (PTCPHDR)((UCHAR*)pIpHdr + pIpHdr->ihl * 4);
	pDAtA				= (PVOID)((UCHAR*)pTcpHdr + sizeof(TCPHDR));//heAder

	ulDAtALength		= NTOHS(pIpHdr->tot_len) - pIpHdr->ihl*4 - sizeof(TCPHDR);
	DbgPrint("ulDAtALength: %d\n",ulDAtALength);
	//ChecksumTempBuff 为了计算效验和
	NdisAllocateMemoryWithTag(
		&ChecksumTempBuff,
		MAX_PACKET_SIZE,
		'pmtU'
		);

	if (pTcpHdr->psh){
		//PsdHdrSend用来计算tcp效验和
		PsdHdr.saddr	= pIpHdr->saddr;
		PsdHdr.daddr	= pIpHdr->daddr;
		PsdHdr.mbz		= 0;
		PsdHdr.ptcl		= IPPROTO_TCP;
		PsdHdr.tcpl		= HTONS(sizeof(TCPHDR)+(USHORT)ulDAtALength);
		
		tempChecksum	= pTcpHdr->check;
		pTcpHdr->check	= 0;//modify the pAcket
		
		NdisMoveMemory(ChecksumTempBuff,&PsdHdr,sizeof(PSDHDR));
		NdisMoveMemory((UCHAR*)ChecksumTempBuff + sizeof(PSDHDR),pTcpHdr,sizeof(TCPHDR));
		NdisMoveMemory((UCHAR*)ChecksumTempBuff + sizeof(PSDHDR) + sizeof(TCPHDR),pDAtA,ulDAtALength);
		
		Checksum = checksum((USHORT*)ChecksumTempBuff,sizeof(PSDHDR)+sizeof(TCPHDR)+ulDAtALength);

		pTcpHdr->check	= tempChecksum;
	}
	else{
		//PsdHdrSend用来计算tcp效验和
		PsdHdr.saddr	= pIpHdr->saddr;
		PsdHdr.daddr	= pIpHdr->daddr;
		PsdHdr.mbz		= 0;
		PsdHdr.ptcl		= IPPROTO_TCP;
		PsdHdr.tcpl		= HTONS(sizeof(TCPHDR));
		
		NdisMoveMemory(ChecksumTempBuff,&PsdHdr,sizeof(PSDHDR));
		NdisMoveMemory((UCHAR*)ChecksumTempBuff + sizeof(PSDHDR),pTcpHdr,sizeof(TCPHDR));

		tempChecksum	= pTcpHdr->check;
		pTcpHdr->check	= 0;//modify the pAcket
		
		Checksum = checksum((USHORT*)ChecksumTempBuff,sizeof(PSDHDR)+sizeof(TCPHDR));

		pTcpHdr->check	= tempChecksum;
	}

	//释放ChecksumTempBuff
	NdisFreeMemory(
		ChecksumTempBuff,
		0,
		0
		);

	return Checksum;
}
//--------------------------------------------------------------------
BOOLEAN
CheckTheChecksum(
	PIPHDR	pIpHdr
	)
{
	USHORT		OriginAlChecksum;
	USHORT		NowChecksum;
	PSDHDR		PsdHdr;
	PTCPHDR		pTcpHdr = NULL;
	CHAR*		tempBuffer;

	pTcpHdr				= (PTCPHDR)((UCHAR*)pIpHdr + pIpHdr->ihl * 4);


	NdisAllocateMemoryWithTag(
						&tempBuffer,
						MAX_PACKET_SIZE,
						'pmtU'
						);

	PsdHdr.saddr	= pIpHdr->saddr;
	PsdHdr.daddr	= pIpHdr->daddr;
	PsdHdr.mbz		= 0;
	PsdHdr.ptcl		= IPPROTO_TCP;
	PsdHdr.tcpl		= HTONS(NTOHS(pIpHdr->tot_len) - sizeof(IPHDR));

	OriginAlChecksum	= pTcpHdr->check;
	//DbgPrint("OriginAlChecksum: %x\n",OriginAlChecksum);
	pTcpHdr->check		= 0;

	NdisMoveMemory(tempBuffer,&PsdHdr,sizeof(PSDHDR));
	NdisMoveMemory((UCHAR*)tempBuffer + sizeof(PSDHDR),pTcpHdr,NTOHS(pIpHdr->tot_len) - sizeof(IPHDR));

	NowChecksum	 = checksum((USHORT*)tempBuffer,NTOHS(pIpHdr->tot_len) - sizeof(IPHDR) + sizeof(PSDHDR));
	//DbgPrint("NowChecksum: %x\n",NowChecksum);
	//NowChecksum = checksum((USHORT*)pIpHdr,pIpHdr->tot_len * 4);

	NdisFreeMemory(
		tempBuffer,
		0,
		0
		);
	if(OriginAlChecksum == NowChecksum){
		return TRUE;
	}
	return FALSE;

}
//--------------------------------------------------------------------





		

				