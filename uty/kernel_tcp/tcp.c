///
//	uty@uaty
///
#include <ndis.h>
#include "sock.h"
#include "tcp.h"
#include "utils.h"
#include "TcpIpHdr.h"

extern NDIS_HANDLE		m_ourPAcketPoolHAndle;
extern NDIS_HANDLE		m_ourBufferPoolHAndle;
extern NDIS_HANDLE		g_BindAdAptHAndle;

tcp_seq	tcp_iss = 1;		/* tcp initial send seq # */

/* patchable/settable parameters for tcp */
int 	tcp_mssdflt = TCP_MSS;
int 	tcp_rttdflt = TCPTV_SRTTDFLT / PR_SLOWHZ;

int	tcprexmtthresh = 3;

/*
 * Flags used when sending segments in tcp_output.
 * Basic flags (TH_RST,TH_ACK,TH_SYN,TH_FIN) are totally
 * determined by state, with the proviso that TH_FIN is sent only
 * if all data queued for output is included in the segment.
 */
unsigned char	tcp_outflAgs[TCP_NSTATES] = {
    TH_RST|TH_ACK, 0, TH_SYN, TH_SYN|TH_ACK,
    TH_ACK, TH_ACK,
    TH_FIN|TH_ACK, TH_FIN|TH_ACK, TH_FIN|TH_ACK, TH_ACK, TH_ACK,
};
/*-----------------------------------------------------------------------------*/
USHORT
checksum(
	USHORT	*buff,
	ULONG	size
	);

int
tcp_output(
	struct sock* psock
	);

void
tcp_cAnceltimers(
	struct sock*	psock
	);
/*-----------------------------------------------------------------------------*/
struct sock*
sock_lookup(
	unsigned long	sAddr,
	unsigned short	sport,
	unsigned long	dAddr,
	unsigned short	dport
	)
{
	int i;
	for(i = 0;i < SOCK_MAX;i++){
		if(	tcp_conn_pool[i].daddr == NTOHL(sAddr) &&
			tcp_conn_pool[i].dport == NTOHS(sport) &&
			tcp_conn_pool[i].saddr == NTOHL(dAddr) &&
			tcp_conn_pool[i].sport == NTOHS(dport)){
			return &tcp_conn_pool[i];
		}
	}
	/* If there is no connection,check the listening sock
	 * As it's SYN pAcket
	 */
	for(i = 0;i < SOCK_MAX;i++){
		if(	tcp_conn_pool[i].state == TCPS_LISTEN	&&
			tcp_conn_pool[i].sport == NTOHS(dport)	){
			/*
			int j;
			for(j = 0;j < SOCK_MAX;j++){
				if (tcp_conn_pool[j].state == TCPS_CLOSED){
					tcp_conn_pool[j].state = TCPS_LISTEN;
					tcp_conn_pool[j].sport = HTONS(dport);
					return &tcp_conn_pool[j];
				}
			}
			*/
			return &tcp_conn_pool[i];
		}
	}
	return NULL;
}
/*-----------------------------------------------------------------------------*/
/* 
 * 把得到的数据存入接收缓存中
 */
int
sbAppend(
	struct	sock* psock,
	PIPHDR	pIpHdr,
	PVOID	pdAtA,
	int		dAtAlen
	)
{
	PTCPHDR		pTcpHdr = NULL;
	pTcpHdr		= (PTCPHDR)((UCHAR*)pIpHdr + pIpHdr->ihl * 4);

	//这里还需要判断下缓存是否有足够空间 bug
	//debug
	DbgPrint("in sbAppend\n");
	//

	strncpy(&(psock->rcv_buff[psock->rcv_count]),(char*)pdAtA,dAtAlen);
	psock->rcv_count += dAtAlen;
	return 0;
}
/*-----------------------------------------------------------------------------*/
/*
 * 返回接收缓存剩余的大小
 */
int
sbspAce(
	struct sock* psock
	)
{
	return 65535 - psock->rcv_count;
}
/*-----------------------------------------------------------------------------*/
/* 
 * this routine wAke up the recvive threAd, r meAns reAd
 */
int
sorwAkeup(
	struct sock* psock
	)
{
	//debug
	DbgPrint("in sorwAkeup\n");
	//
	KeSetEvent(&(psock->rcv_event),0,FALSE);
	return 0;
}
/*-----------------------------------------------------------------------------*/
/* 
 * this routine wAke up the send threAd, w meAns write
 */
int
sowwAkeup(
	struct sock* psock
	)
{
	KeSetEvent(&(psock->snd_event),0,FALSE);
	return 0;
}
/*-----------------------------------------------------------------------------*/
void
soisconnected(
	struct sock*	psock
	)
{
	//debug
	DbgPrint("in soisconnected\n");
	//
	KeSetEvent(&(psock->accept_event),0,FALSE);
}
/*-----------------------------------------------------------------------------*/
/*
 * Close a TCP control block:
 *	discard all space held by the tcp
 *	discard internet protocol block
 *	wake up any sleepers
 */
struct sock*
tcp_close(
	struct sock*	psock
	)
{
	struct _dAtA_entry *pdAtA,*ptemp;

	//debug
	//__asm int 3;
	//

	//debug
	DbgPrint("in tcp_close\n");
	//
	KeReleaseSpinLock(&psock->sock_spin,psock->oldirql);
	KeSetEvent(&(psock->rcv_event),0,FALSE);
	KeSetEvent(&(psock->snd_event),0,FALSE);

	/* free the reassembly queue, if any */
	pdAtA = psock->inseq_queue;
	while (pdAtA != NULL){
		ptemp = pdAtA->next;
		ExFreePool(pdAtA);
		pdAtA = ptemp;
	}
	memset(psock,0,sizeof(struct sock));
	
	//新的socket的初始化
	KeInitializeEvent(
		&psock->connect_event,
		SynchronizationEvent,
		FALSE
		);
	KeInitializeEvent(
		&psock->accept_event,
		SynchronizationEvent,
		FALSE
		);
	
	KeInitializeSpinLock(&(psock->sock_spin));

	return ((struct sock*)0);
}
/*-----------------------------------------------------------------------------*/
/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */
struct sock*
tcp_drop(
	struct sock*	psock,
	int		errno
	)
{
	if (TCPS_HAVERCVDSYN(psock->state)){
		psock->state = TCPS_CLOSED;
		tcp_output(psock);
	}
	return tcp_close(psock);
}
/*-----------------------------------------------------------------------------*/
int
tcp_reAss(
	struct sock*	psock,
	struct _IPHDR*	pIpHdr,
	PVOID			dAtA,
	int				dAtAlen
	)
{
	int	i;
	int flAgs;
	struct _dAtA_entry *pdAtA;
	struct _dAtA_entry *pnew;
	PTCPHDR		pTcpHdr = NULL;


	//debug
	DbgPrint("in tcp_reAss\n");
	//
	pTcpHdr		= (PTCPHDR)((UCHAR*)pIpHdr + pIpHdr->ihl * 4);
	/*
	 * Find a segment which begins after this one does.
	 */

	pdAtA = psock->inseq_queue;

	if (pdAtA == NULL)
		goto Addnew;

	while(pdAtA != NULL){
		if (SEQ_GT(NTOHL(pdAtA->pTcpHdr->seq),NTOHL(pTcpHdr->seq)))
			break;
		pdAtA = pdAtA->next;
	}
	/*
	 * If there is a preceding segment, it may provide some of
	 * our data already.  If so, drop the data from the incoming
	 * segment.  If it provides all of our data, drop us.
	 */
	pdAtA = pdAtA->prev;
	if(SEQ_LT(NTOHL(pdAtA->pTcpHdr->seq),NTOHL(pTcpHdr->seq))){
		i = NTOHL(pdAtA->pTcpHdr->seq) + pdAtA->dAtAlen - NTOHL(pTcpHdr->seq);
		if (i > 0){
			if (i >= dAtAlen){
				return 0;
			}
			//m_Adj(m,i);
			//pTcpHdr->doff += i/4;
			dAtA = (UCHAR*)dAtA + i;
			dAtAlen -= i;
			pTcpHdr->seq = HTONL(NTOHL(pTcpHdr->seq) + i);
		}
		pdAtA = pdAtA->next;
	}
	
	while(pdAtA->next != NULL){
		
		i = NTOHL(pTcpHdr->seq) + dAtAlen - NTOHL(pdAtA->pTcpHdr->seq);
		if (i <= 0)
			break;
		if (i < pdAtA->dAtAlen){
			pdAtA->pTcpHdr->seq = HTONL(NTOHL(pdAtA->pTcpHdr->seq) + i);
			//pdAtA->pTcpHdr->doff += i/4;
			pdAtA->dAtAlen -= i;
			pdAtA->dAtA = (UCHAR*)(pdAtA->dAtA) + i;
			break;
		}
		if (pdAtA->prev != NULL){
			pdAtA->prev->next = pdAtA->next;
		}
		if (pdAtA->next != NULL){
			pdAtA->next->prev = pdAtA->prev;
		}
		pdAtA = pdAtA->next;
		ExFreePool(pdAtA->prev);
	}
	
Addnew:
	/*
	 * Stick new segment in its place.
	 */

	pnew = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct _dAtA_entry),'ytU');
	RtlCopyMemory(pnew->buffer,pIpHdr,NTOHS(pIpHdr->tot_len));
	pnew->pIpHdr	= pIpHdr;
	pnew->pTcpHdr	= pTcpHdr;
	
	if (pdAtA == NULL){
		pdAtA = pnew;
		pdAtA->prev = NULL;
		pdAtA->next	= NULL;
	} else{
		pdAtA->next = pnew;
		pnew->prev	= pdAtA;
		pnew->next	= NULL;
		pnew->dAtA	= dAtA;
		pnew->dAtAlen = dAtAlen;
	}


//present:
	/*
	 * Present data to user, advancing rcv_nxt through
	 * completed sequence space.
	 */
	pdAtA = psock->inseq_queue;
	if (TCPS_HAVERCVDSYN(psock->state) == 0)
		return 0;
	if(pdAtA == NULL || NTOHL(pTcpHdr->seq) != psock->rcv_nxt)
		return 0;
	if (psock->state == TCPS_SYN_RECEIVED && dAtAlen)
		return 0;
	do{
		psock->rcv_nxt += dAtAlen;
		flAgs = pTcpHdr->fin;
		sbAppend(psock,pdAtA->pIpHdr,pdAtA->dAtA,pdAtA->dAtAlen);
		pdAtA = pdAtA->next;

	}while (pdAtA != NULL && NTOHL(pTcpHdr->seq) == psock->rcv_nxt);

	sorwAkeup(psock);
	return flAgs;
}
/*-----------------------------------------------------------------------------*/
/* 
 * 从发送缓存中删除len个字符,从前面开始删
 */
void
sbdrop(
	struct sock*	psock,
	int		len
	)
{
	if (psock->snd_count == 0){
		//mAy be the syn
		return;
	}
	
	if (len > psock->snd_count){
		//debug
		DbgPrint("psock->snd_count: %d\n",psock->snd_count);
		DbgPrint("len: %d\n",len);
		//
		DbgPrint("ERROR: sbdrop() drop more dAtA then AvAilAble in snd_buff\n");
		//memset(psock->snd_buff,0,65535);
		//psock->snd_count = 0;
	}
	memcpy(psock->snd_buff,(char*)psock->snd_buff + len,psock->snd_count - len);
	psock->snd_count -= len;
}
//-----------------------------------------------------------------------------*/
/*
 * Insert segment ti into reassembly queue of tcp with
 * control block tp.  Return TH_FIN if reassembly now includes
 * a segment with FIN.  The macro form does the common case inline
 * (segment is the next to be received on an established connection,
 * and the queue is empty), avoiding linkage into and removal
 * from the queue and repetition of various conversions.
 * Set DELACK for segments received in order, but ack immediately
 * when segments are out of order (so fast retransmit can work).
 */
void
TCP_REASS(
	struct sock*	psock,
	struct _IPHDR*	pIpHdr,
	PVOID			pdAtA,
	int				dAtAlen
	)
{
	PTCPHDR		pTcpHdr = NULL;

	//debug
	DbgPrint("in TCP_REASS\n");
	//
	pTcpHdr		= (PTCPHDR)((UCHAR*)pIpHdr + pIpHdr->ihl * 4);
	if (NTOHL(pTcpHdr->seq) == psock->rcv_nxt &&
		psock->inseq_queue == NULL &&
		psock->state == TCPS_ESTABLISHED){
		//debug
		//psock->t_flags |= TF_DELACK;
		psock->t_flags |= TF_ACKNOW;

		psock->rcv_nxt += dAtAlen;
		sbAppend(psock,pIpHdr,pdAtA,dAtAlen);
		sorwAkeup(psock);
	} else {
		tcp_reAss(psock,pIpHdr,pdAtA,dAtAlen);
		psock->t_flags |= TF_ACKNOW;
	}
}
/*-----------------------------------------------------------------------------*/
/*
 * Send a single message to the TCP at address specified by
 * the given TCP/IP header.  If m == 0, then we make a copy
 * of the tcpiphdr at ti and send directly to the addressed host.
 * This is used to force keep alive messages out using the TCP
 * template for a connection tp->t_template.  If flags are given
 * then we send a message back to the TCP which originated the
 * segment ti, and discard the mbuf containing it and any other
 * attached mbufs.
 *
 * In any case the ack and sequence number of the transmitted
 * segment are as specified by the parameters.
 */
void
tcp_respond(
	struct	sock*	psock,
	struct	_ETHHDR	*pEthHdr,
	int		keepAlive,
	tcp_seq	Ack,
	tcp_seq	seq,
	int		flAgs
	)
{
	int		tlen;
	int		win = 0;

	PTCPHDR			pTcpHdr = NULL;
	PIPHDR			pIpHdr	= NULL;
	
	
	PVOID			pSendBuffer = NULL;
	PETHHDR			pEthHdrSend	= NULL;
	PIPHDR			pIpHdrSend	= NULL;
	PTCPHDR			pTcpHdrSend	= NULL;
	PVOID			pDAtA		= NULL;


	if(!keepAlive){
		pIpHdr		= (PIPHDR)((UCHAR*)pEthHdr + sizeof(ETHHDR));
		pTcpHdr		= (PTCPHDR)((UCHAR*)pIpHdr + pIpHdr->ihl * 4);
	}

	//debug
	DbgPrint("tcp_response()\n");
	//
	
	if (keepAlive){
		tlen = 0;
		flAgs = TH_ACK;
	} else{
		tlen = 0;
		if (psock){
			win = sbspAce(psock);
		}
	}

	NdisAllocateMemoryWithTag(
		&pSendBuffer,
		MAX_PACKET_SIZE,
		'pmtU'
		);
	pEthHdrSend			= pSendBuffer;
	if (keepAlive){
		NdisMoveMemory(pEthHdrSend->h_dest,psock->dstmAc,6);
		NdisMoveMemory(pEthHdrSend->h_source,psock->srcmAc,6);
	} else{
		NdisMoveMemory(pEthHdrSend->h_dest,pEthHdr->h_source,6);
		NdisMoveMemory(pEthHdrSend->h_source,pEthHdr->h_dest,6);
	}
	pEthHdrSend->h_proto= HTONS(ETH_P_IP);
	
	pIpHdrSend			= (PIPHDR)((UCHAR*)pEthHdrSend + sizeof(ETHHDR));
	pIpHdrSend->ihl		= sizeof(IPHDR)/4;
	pIpHdrSend->version	= 4;
	pIpHdrSend->tos		= 0;
	pIpHdrSend->tot_len	= HTONS(sizeof(IPHDR) + sizeof(TCPHDR) + (USHORT)tlen);
	pIpHdrSend->id		= 0;
	pIpHdrSend->frag_off= 0;
	pIpHdrSend->ttl		= 255;
	pIpHdrSend->protocol= IPPROTO_TCP;
	pIpHdrSend->check	= 0;
	if (keepAlive){
		pIpHdrSend->saddr	= HTONL(psock->saddr);
		pIpHdrSend->daddr	= HTONL(psock->daddr);
	} else{
		pIpHdrSend->saddr	= pIpHdr->daddr;
		pIpHdrSend->daddr	= pIpHdr->saddr;
	}
	pIpHdrSend->check	= checksum((USHORT*)pIpHdrSend,sizeof(IPHDR));
	
	pTcpHdrSend			= (PTCPHDR)((UCHAR*)pIpHdrSend + pIpHdrSend->ihl * 4);
	pDAtA				= (PVOID)((UCHAR*)pTcpHdrSend + sizeof(TCPHDR));
	if (keepAlive){
		pTcpHdrSend->source = HTONS(psock->dport);
		pTcpHdrSend->dest	= HTONS(psock->sport);
	} else{
		pTcpHdrSend->source	= pTcpHdr->dest;
		pTcpHdrSend->dest	= pTcpHdr->source;
	}
	pTcpHdrSend->doff	= sizeof(TCPHDR)/4;
	pTcpHdrSend->check	= 0;
	pTcpHdrSend->urg_ptr= 0;



	pTcpHdrSend->seq		= HTONL(seq);
	pTcpHdrSend->ack_seq	= HTONL(Ack);
	pTcpHdrSend->rst = flAgs & TH_RST;
	pTcpHdrSend->ack = flAgs & TH_ACK;
	pTcpHdrSend->syn = flAgs & TH_SYN;
	pTcpHdrSend->fin = flAgs & TH_FIN;
	

	pTcpHdrSend->window = HTONS((USHORT)win);

	//计算checksum
	{
	PSDHDR			PsdHdr;
	PVOID			pChecksumTempBuffer = NULL;
	PsdHdr.saddr	= pIpHdrSend->saddr;
	PsdHdr.daddr	= pIpHdrSend->daddr;
	PsdHdr.mbz		= 0;
	PsdHdr.ptcl		= IPPROTO_TCP;
	PsdHdr.tcpl		= HTONS(sizeof(TCPHDR) + (USHORT)tlen);

	NdisAllocateMemoryWithTag(
		&pChecksumTempBuffer,
		MAX_PACKET_SIZE,
		'pmtU'
		);
	NdisMoveMemory(pChecksumTempBuffer,&PsdHdr,sizeof(PSDHDR));
	NdisMoveMemory((UCHAR*)pChecksumTempBuffer + sizeof(PSDHDR),pTcpHdrSend,sizeof(TCPHDR) + tlen);
	pTcpHdrSend->check	= checksum((USHORT*)pChecksumTempBuffer,sizeof(PSDHDR)+sizeof(TCPHDR)+tlen);
	NdisFreeMemory(
		pChecksumTempBuffer,
		0,
		0
		);
	}

	
	//发包
	{
	PNDIS_BUFFER	pNdisBuffer = NULL;
	PNDIS_PACKET	pNdisPAcket = NULL;
	NTSTATUS		stAtus;
	NdisAllocateBuffer(
		&stAtus,
		&pNdisBuffer,
		m_ourBufferPoolHAndle,
		pSendBuffer,
		sizeof(ETHHDR)+sizeof(IPHDR)+sizeof(TCPHDR)+tlen
		);
	if (stAtus != NDIS_STATUS_SUCCESS){
		DbgPrint("ERROR:NdisAllocAteBuffer fAiled\n");
		return;
	}
	NdisAllocatePacket(
		&stAtus,
		&pNdisPAcket,
		m_ourPAcketPoolHAndle
		);
	if (stAtus != NDIS_STATUS_SUCCESS){
		DbgPrint("ERROR:NdisAllocAtePAcket fAiled\n");
		return;
	}
	NdisChainBufferAtFront(
		pNdisPAcket,
		pNdisBuffer
		);
	if (g_BindAdAptHAndle == NULL){
		DbgPrint("ERROR: BindAdAptHAndle is NULL\n");
		return;
	}
	NdisSendPackets(
		g_BindAdAptHAndle,
		&pNdisPAcket,
		1
		);
	}


}
/*-----------------------------------------------------------------------------*/
/*
 * Collect new round-trip time estimate
 * and update averages and current timeout.
 */
void
tcp_xmit_timer(
	register struct sock * psock,
	short rtt
	)
{
	register short delta;

	if (psock->t_srtt != 0) {
		/*
		 * srtt is stored as fixed point with 3 bits after the
		 * binary point (i.e., scaled by 8).  The following magic
		 * is equivalent to the smoothing algorithm in rfc793 with
		 * an alpha of .875 (srtt = rtt/8 + srtt*7/8 in fixed
		 * point).  Adjust rtt to origin 0.
		 */
		delta = rtt - 1 - (psock->t_srtt >> TCP_RTT_SHIFT);
		if ((psock->t_srtt += delta) <= 0)
			psock->t_srtt = 1;
		/*
		 * We accumulate a smoothed rtt variance (actually, a
		 * smoothed mean difference), then set the retransmit
		 * timer to smoothed rtt + 4 times the smoothed variance.
		 * rttvar is stored as fixed point with 2 bits after the
		 * binary point (scaled by 4).  The following is
		 * equivalent to rfc793 smoothing with an alpha of .75
		 * (rttvar = rttvar*3/4 + |delta| / 4).  This replaces
		 * rfc793's wired-in beta.
		 */
		if (delta < 0)
			delta = -delta;
		delta -= (psock->t_rttvar >> TCP_RTTVAR_SHIFT);
		if ((psock->t_rttvar += delta) <= 0)
			psock->t_rttvar = 1;
	} else {
		/* 
		 * No rtt measurement yet - use the unsmoothed rtt.
		 * Set the variance to half the rtt (so our first
		 * retransmit happens at 3*rtt).
		 */
		psock->t_srtt = rtt << TCP_RTT_SHIFT;
		psock->t_rttvar = rtt << (TCP_RTTVAR_SHIFT - 1);
	}
	psock->t_rtt = 0;
	psock->t_rxtshift = 0;

	/*
	 * the retransmit should happen at rtt + 4 * rttvar.
	 * Because of the way we do the smoothing, srtt and rttvar
	 * will each average +1/2 tick of bias.  When we compute
	 * the retransmit timer, we want 1/2 tick of rounding and
	 * 1 extra tick because of +-1/2 tick uncertainty in the
	 * firing of the timer.  The bias will give us exactly the
	 * 1.5 tick we need.  But, because the bias is
	 * statistical, we have to test that we don't drop below
	 * the minimum feasible timer (which is 2 ticks).
	 */
	TCPT_RANGESET(psock->t_rxtcur, TCP_REXMTVAL(psock),
	    psock->t_rttmin, TCPTV_REXMTMAX);
	
	/*
	 * We received an ack for a packet that wasn't retransmitted;
	 * it is probably safe to discard any error indications we've
	 * received recently.  This isn't quite right, but close enough
	 * for now (a route might have failed after we sent a segment,
	 * and the return path might not be symmetrical).
	 */
	//psock->t_softerror = 0;
}
/*-----------------------------------------------------------------------------*/
int
tcp_v4_do_rcv(
	struct	sock* psock,
	char*	pBuffer
	)
{
	PETHHDR		pEthHdr = NULL;
	PIPHDR		pIpHdr	= NULL;
	PTCPHDR		pTcpHdr = NULL;
	PVOID		pdAtA	= NULL;

	int			Acked;
	int			todrop;
	int			needoutput = 0;
	int			ourfinisAcked;
	int			dAtAlen;


	pEthHdr		= (PETHHDR)pBuffer;
	pIpHdr		= (PIPHDR)((UCHAR*)pEthHdr + sizeof(ETHHDR));
	pTcpHdr		= (PTCPHDR)((UCHAR*)pIpHdr + pIpHdr->ihl * 4);
	pdAtA		= (PVOID)((UCHAR*)pTcpHdr + pTcpHdr->doff * 4);


	

	//debug
	//__asm int 3;
	//

	dAtAlen = NTOHS(pIpHdr->tot_len) - pIpHdr->ihl*4 - pTcpHdr->doff *4;

	/*
	 * Segment received on connection.
	 * Reset idle time and keep-alive timer.
	 */
	psock->t_idle = 0;
	psock->t_timer[TCPT_KEEP] = (short)tcp_keepidle;



	/*
	 * HeAder prediction
	 */

	if(psock->state == TCPS_ESTABLISHED &&
		(NTOHL(pTcpHdr->ack_seq) && !pTcpHdr->syn && !pTcpHdr->fin && !pTcpHdr->rst && !pTcpHdr->urg) &&
		NTOHL(pTcpHdr->seq) == psock->rcv_nxt &&
		NTOHS(pTcpHdr->window) == psock->snd_wnd &&
		psock->snd_nxt == psock->snd_max){
		if(dAtAlen == 0){
			if(SEQ_GT(NTOHL(pTcpHdr->ack_seq),psock->snd_una)
				){
				
				//debug
				DbgPrint("in heAder prediction,pure Ack\n");
				//
				/* 
				 * this is A pure Ack for outstAnding dAtA.
				 */
				if (psock->t_rtt &&
						SEQ_GT(NTOHL(pTcpHdr->ack_seq), psock->t_rtseq))
					tcp_xmit_timer(psock, psock->t_rtt);
				Acked = NTOHL(pTcpHdr->ack_seq) - psock->snd_una;
				//sbdrop 从发送缓存中删除被确认过的字节
				sbdrop(psock,Acked);
				psock->snd_una = NTOHL(pTcpHdr->ack_seq);
				/*
				 * If all outstanding data are acked, stop
				 * retransmit timer, otherwise restart timer
				 * using current (possibly backed-off) value.
				 * If process is waiting for space,
				 * wakeup/selwakeup/signal.  If data
				 * are ready to send, let tcp_output
				 * decide between more output or persist.
				 */
				
				if(psock->snd_una == psock->snd_max)
					psock->t_timer[TCPT_REXMT] = 0;
				else if(psock->t_timer[TCPT_PERSIST] == 0)
					psock->t_timer[TCPT_REXMT] = psock->t_rxtcur;
			
				//唤醒调用send的线程
				sowwAkeup(psock);
				if(psock->snd_count)
					tcp_output(psock);
				return 0;
			}else{}
		}
		else if(NTOHL(pTcpHdr->ack_seq) == psock->snd_una &&
			psock->inseq_queue == NULL &&
			dAtAlen <= sbspAce(psock)){
			//debug
			DbgPrint("in heAder prediction,dAtA pAcket\n");
			//
			/*
			 * this is a pure, in-sequence data packet
			 * with nothing on the reassembly queue and
			 * we have enough buffer space to take it.
			 */
			psock->rcv_nxt += dAtAlen;
			/*
			 * Drop TCP, IP headers and TCP options then add data
			 * to socket buffer.
			 */
			//把得到的数据加入到接收缓存
			sbAppend(psock,pIpHdr,pdAtA,dAtAlen);
			//唤醒等待recv的线程
			sorwAkeup(psock);
			//debug
			//psock->t_flags |= TF_DELACK;
			psock->t_flags |= TF_ACKNOW;
			
			return 0;
		}

		//debug
		//DbgPrint("in heAder prediction,not fixed\n");
		//
	}
	
	/*
	 * Calculate amount of space in receive window,
	 * and then do TCP input processing.
	 * Receive window is amount of space in rcv queue,
	 * but not less than advertised window.`
	*/
	{
	int win;
	win = sbspAce(psock);
	if(win < 0)
		win = 0;
	psock->rcv_wnd = mAx(win,(int)(psock->rcv_adv - psock->rcv_nxt));
	//debug
	DbgPrint("psock->rcv_wnd: %d\n",(ULONG)psock->rcv_wnd);
	//
	}



	switch(psock->state){
	/*
	 * If the state is LISTEN then ignore segment if it contains an RST.
	 * If the segment contains an ACK then it is bad and send a RST.
	 * If it does not contain a SYN then it is not interesting; drop it.
	 * Don't bother responding if the destination was a broadcast.
	 * Otherwise initialize tp->rcv_nxt, and tp->irs, select an initial
	 * tp->iss, and send a segment:
	 *     <SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
	 * Also initialize tp->snd_nxt to tp->iss+1 and tp->snd_una to tp->iss.
	 * Fill in remote peer address fields if not previously specified.
	 * Enter SYN_RECEIVED state, and process any other fields of this
	 * segment in this state.
	 */
	case TCPS_LISTEN:{
		if(pTcpHdr->rst)
			goto drop;
		if(pTcpHdr->ack)
			goto dropwithreset;
		if(pTcpHdr->syn == 0)
			goto drop;

		RtlCopyMemory(psock->srcmAc,pEthHdr->h_dest,6);
		RtlCopyMemory(psock->dstmAc,pEthHdr->h_source,6);
		psock->saddr = NTOHL(pIpHdr->daddr);
		psock->sport = NTOHS(pTcpHdr->dest);
		psock->daddr = NTOHL(pIpHdr->saddr);
		psock->dport = NTOHS(pTcpHdr->source);

		psock->iss = tcp_iss;
		tcp_iss += TCP_ISSINCR/2;
		psock->irs = NTOHL(pTcpHdr->seq);
		tcp_sendseqinit(psock);
		tcp_rcvseqinit(psock);
		psock->t_flags |= TF_ACKNOW;
		psock->state = TCPS_SYN_RECEIVED;
		psock->t_timer[TCPT_KEEP] = TCPTV_KEEP_INIT;
		psock->t_maxseg	= TCP_MSS;

		KeInitializeEvent(
			&psock->snd_event,
			SynchronizationEvent,
			TRUE
			);
		KeInitializeEvent(
			&psock->rcv_event,
			SynchronizationEvent,
			FALSE
			);

		goto trimthenstep6;
		}

	/*
	 * If the state is SYN_SENT:
	 *	if seg contains an ACK, but not for our SYN, drop the input.
	 *	if seg contains a RST, then drop the connection.
	 *	if seg does not contain SYN, then drop it.
	 * Otherwise this is an acceptable SYN segment
	 *	initialize tp->rcv_nxt and tp->irs
	 *	if seg contains ack then advance tp->snd_una
	 *	if SYN has been acked change to ESTABLISHED else SYN_RCVD state
	 *	arrange for segment to be acked (eventually)
	 *	continue processing rest of data/controls, beginning with URG
	 */
	case TCPS_SYN_SENT:
		if ((pTcpHdr->ack) &&
			(SEQ_LEQ(pTcpHdr->ack,psock->iss) ||
			 SEQ_GT(pTcpHdr->ack,psock->snd_max)))
			 goto dropwithreset;
		if(pTcpHdr->rst){
			if(pTcpHdr->ack)
				psock = tcp_drop(psock,0);//ECONNREFUSED);
			goto drop;
		}
		if(pTcpHdr->syn == 0)
			goto drop;
		if(pTcpHdr->ack){
			psock->snd_una = NTOHL(pTcpHdr->ack_seq);
			if(SEQ_LT(psock->snd_nxt,psock->snd_una))
				psock->snd_nxt = psock->snd_una;
		}
		psock->t_timer[TCPT_REXMT] = 0;
		psock->irs = NTOHL(pTcpHdr->seq);
		tcp_rcvseqinit(psock);
		psock->t_flags |= TF_ACKNOW;
		if(pTcpHdr->ack && SEQ_GT(psock->snd_una,psock->iss)){
			psock->state = TCPS_ESTABLISHED;
			soisconnected(psock);
			/*
			 * don't recvive dAtA on syn
			 */
			/*
			 * if we didn't have to retransmit the SYN,
			 * use its rtt as our initial srtt & rtt var.
			 */
			if (psock->t_rtt)
				tcp_xmit_timer(psock, psock->t_rtt);
		}else
			psock->state = TCPS_SYN_RECEIVED;

trimthenstep6://trimthenstep6 只用于处理带syn的包
		/*
		 * Advance ti->ti_seq to correspond to first data byte.
		 * If data, trim to stay within window,
		 * dropping FIN if necessary.
		 */
		pTcpHdr->seq = HTONL(NTOHL(pTcpHdr->seq)+1);
		if (dAtAlen > (USHORT)psock->rcv_wnd){
			todrop = dAtAlen - psock->rcv_wnd;
			//裁减超出窗口的数据
			//m_Adj(m,-todrop);
			//pIpHdr->tot_len = HTONS(NTOHS(pIpHdr->tot_len) - (USHORT)todrop);
			dAtAlen -= todrop;
			pTcpHdr->fin = 0;
		}
		psock->snd_wl1 = NTOHL(pTcpHdr->seq) - 1;
		//psock->rcv_up = NTOHL(pTcpHdr->seq);
		goto step6;
	}

	/*
	 * States other than LISTEN or SYN_SENT.
	 * First check timestamp, if present.
	 * Then check that at least some bytes of segment are within 
	 * receive window.  If segment begins before rcv_nxt,
	 * drop leading data (and SYN); if nothing left, just ack.
	 */

	 todrop = psock->rcv_nxt - NTOHL(pTcpHdr->seq);
	 
	 if (todrop > 0){
		 //debug
		 DbgPrint("psock->rcv_nxt = %d\n",(ULONG)psock->rcv_nxt);
		 DbgPrint("NTOHL(pTcpHdr->seq) = %d\n",(ULONG)NTOHL(pTcpHdr->seq));
		 DbgPrint("todrop = psock->rcv_nxt - NTOHL(pTcpHdr->seq);     todrop= %d\n",todrop);
		 //
		 if(pTcpHdr->syn){
			 pTcpHdr->syn = 0;
			 pTcpHdr->seq = HTONL(NTOHL(pTcpHdr->seq)+1);
			 todrop--;
		 }
		 if (todrop >= dAtAlen){
			 /*
			  * If segment is just one to the left of the window,
			  * check two special cases:
			  * 1. Don't toss RST in response to 4.2-style keepalive.
			  * 2. If the only thing to drop is a FIN, we can drop
			  *    it, but check the ACK or we will get into FIN
			  *    wars if our FINs crossed (both CLOSING).
			  * In either case, send ACK to resynchronize,
			  * but keep on processing for RST or ACK.
			  */
			 if (pTcpHdr->fin && todrop == (dAtAlen +1)){
				todrop = dAtAlen;
				pTcpHdr->fin = 0;
				psock->t_flags |= TF_ACKNOW;
			 } else{
				 /*
				  * Handle the case when a bound socket connects
				  * to itself. Allow packets with a SYN and
				  * an ACK to continue with the processing.
				  */
				 if (todrop != 0 || pTcpHdr->ack == 0)
					 goto dropAfterAck;
			 }
		 }else{
			 //一些统计
		 }
		 //m_adj(m,todrop);
		 //just like m_Adj does
		 //m_adj()通过指定特定数目的字节，从前开始削减，以调整mbuf链中的数据。但是不会拷贝数据。
		 //从m指向的mbuf中移走len字节的数据。如果len是正数，则所操作的是紧排在这个mbuf的开始的len字节数据；
		 //否则是紧排在这个mbuf的尾部的len绝对值字节数据。m_adj()纯粹是操作mbuf结构体中的偏移和长度字段。
		 //pTcpHdr->doff += todrop/4;
		 pdAtA = (UCHAR*)pdAtA + todrop;
		 dAtAlen -= todrop;
		 pTcpHdr->seq = HTONL(NTOHL(pTcpHdr->seq) + todrop);
	 }

	 /*
	  * If new data are received on a connection after the
	  * user processes are gone, then RST the other end.
	  */
	 if(psock->state > TCPS_CLOSE_WAIT && dAtAlen){
		 psock = tcp_close(psock);
		 goto dropwithreset;
	 }
	 /*
	  * If segment ends after window, drop trailing data
	  * (and PUSH and FIN); if nothing left, just ACK.
	  */
	 todrop = NTOHL(pTcpHdr->seq) + dAtAlen - (psock->rcv_nxt + psock->rcv_wnd);
	 if (todrop > 0){
		 if (todrop >= dAtAlen){
			 /*
			  * If a new connection request is received
			  * while in TIME_WAIT, drop the old connection
			  * and start over if the sequence numbers
			  * are above the previous ones.
			  */
			 //先不考虑这种情况
			 /*
			 if (pTcpHdr->syn &&
				 psock->state == TCPS_WIME_WAIT &&
				 SEQ_GT(pTcpHdr->seq,psock->tcv_nxt)){
				 //iss = psock->rcv_nxt + TCP_ISSINCR;
				 psock = tcp_close(psock);
				 //sock_lookup(pIpHdr->saddr,pTcpHdr->source,pIpHdr->daddr,pTcpHdr->dest);
				 //goto findpcb;
			 }
			 */
			 /*
			  * If window is closed can only take segments at
			  * window edge, and have to drop data and PUSH from
			  * incoming segments.  Continue processing, but
			  * remember to ack.  Otherwise, drop segment
			  * and ack.
			  */
			 if (psock->rcv_wnd == 0 && NTOHL(pTcpHdr->seq) == psock->rcv_nxt){
				 psock->t_flags |= TF_ACKNOW;
			 }else
				 goto dropAfterAck;
		 }else{
			 //统计
		 }
		 //m_Adj(m,-todrop);
		 //pIpHdr->tot_len = HTONS(NTOHS(pIpHdr->tot_len) - (USHORT)todrop);
		 dAtAlen -= todrop;
		 pTcpHdr->psh = 0;
		 pTcpHdr->fin = 0;
	 }

	 /*
	  * If last ACK falls within this segment's sequence numbers,
	  * record its timestamp.
	  */
	 //先不考虑timestAmp

	 /*
	  * If the RST bit is set examine the state:
	  *    SYN_RECEIVED STATE:
	  *	If passive open, return to LISTEN state.
	  *	If active open, inform user that connection was refused.
	  *    ESTABLISHED, FIN_WAIT_1, FIN_WAIT2, CLOSE_WAIT STATES:
	  *	Inform user that connection was reset, and close tcb.
	  *    CLOSING, LAST_ACK, TIME_WAIT STATES
	  *	Close the tcb.
	  */
	 if (pTcpHdr->rst) switch(psock->state){
		 case TCPS_SYN_RECEIVED:
			 //so->so_error = ECONNREFUSED;
			 goto close;
		 case TCPS_ESTABLISHED:
		 case TCPS_FIN_WAIT_1:
		 case TCPS_FIN_WAIT_2:
		 case TCPS_CLOSE_WAIT:
			 //so->so_error = ECONNRESET;
		close:
			 psock->state = TCPS_CLOSED;
			 psock = tcp_close(psock);
			 goto drop;

		 case TCPS_CLOSING:
		 case TCPS_LAST_ACK:
		 case TCPS_TIME_WAIT:
			 psock = tcp_close(psock);
			 goto drop;
	 }

	 /*
	  * If a SYN is in the window, then this is an
	  * error and we send an RST and drop the connection.
	  */
	 if (pTcpHdr->syn){
		 psock = tcp_drop(psock,0);//ECONNRESET);
		 goto dropwithreset;
	 }
	 /*
	  * If the ACK bit is off we drop the segment and return.
	  */
	 if (pTcpHdr->ack == 0)
		 goto drop;

	 /*
	  * Ack processing.
	  */
	 switch (psock->state){
		 
	 /*
	  * In SYN_RECEIVED state if the ack ACKs our SYN then enter
	  * ESTABLISHED state and continue processing, otherwise
	  * send an RST.
	  */
	 case TCPS_SYN_RECEIVED:
		 if (SEQ_GT(psock->snd_una,NTOHL(pTcpHdr->ack_seq)) ||
			 SEQ_GT(NTOHL(pTcpHdr->ack_seq),psock->snd_max))
			 goto dropwithreset;
		 psock->state = TCPS_ESTABLISHED;
		 soisconnected(psock);

		 /* Do window scaling? */
		 //先不考虑
		 psock->snd_wl1 = NTOHL(pTcpHdr->seq) -1;
		 /* fall into ... */


	/*
	 * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
	 * ACKs.  If the ack is in the range
	 *	tp->snd_una < ti->ti_ack <= tp->snd_max
	 * then advance tp->snd_una to ti->ti_ack and drop
	 * data from the retransmission queue.  If this ACK reflects
	 * more up to date window information we update our window information.
	 */
	 case TCPS_ESTABLISHED:
	 case TCPS_FIN_WAIT_1:
	 case TCPS_FIN_WAIT_2:
	 case TCPS_CLOSE_WAIT:
	 case TCPS_CLOSING:
	 case TCPS_LAST_ACK:
	 case TCPS_TIME_WAIT:

		 if (SEQ_LEQ(NTOHL(pTcpHdr->ack_seq),psock->snd_una)){
			 if (dAtAlen == 0 && NTOHS(pTcpHdr->window) == psock->snd_wnd){
				 /*
				  * If we have outstanding data (other than
				  * a window probe), this is a completely
				  * duplicate ack (ie, window info didn't
				  * change), the ack is the biggest we've
				  * seen and we've seen exactly our rexmt
				  * threshhold of them, assume a packet
				  * has been dropped and retransmit it.
				  * Kludge snd_nxt & the congestion
				  * window so we send only this one
				  * packet.
				  *
				  * We know we're losing at the current
				  * window size so do congestion avoidance
				  * (set ssthresh to half the current window
				  * and pull our congestion window back to
				  * the new ssthresh).
				  *
				  * Dup acks mean that packets have left the
				  * network (they're now cached at the receiver) 
				  * so bump cwnd by the amount in the receiver
				  * to keep a constant cwnd packets in the
				  * network.
				  */
				 //不考虑快速重传 快速恢复
				 if (psock->t_timer[TCPT_REXMT] == 0 ||
					 NTOHL(pTcpHdr->ack_seq) != psock->snd_una){
					 psock->t_dupacks = 0;
				 } else if (++psock->t_dupacks == tcprexmtthresh) {
					tcp_seq onxt = psock->snd_nxt;

					psock->t_timer[TCPT_REXMT] = 0;
					psock->t_rtt = 0;
					psock->snd_nxt = NTOHL(pTcpHdr->ack_seq);
					tcp_output(psock);
					if (SEQ_GT(onxt, psock->snd_nxt))
						psock->snd_nxt = onxt;
					goto drop;
				} else if (psock->t_dupacks > tcprexmtthresh) {
					tcp_output(psock);
					goto drop;
				}
			 }else{
				 psock->t_dupacks = 0;
			 }
			 break; /* beyond ACK processing (to step 6) */
		 }

		 if (SEQ_GT(NTOHL(pTcpHdr->ack_seq),psock->snd_max)){
			 goto dropAfterAck;
		 }
		 Acked = NTOHL(pTcpHdr->ack_seq) - psock->snd_una;

		 /*
		  * If we have a timestamp reply, update smoothed
		  * round trip time.  If no timestamp is present but
		  * transmit timer is running and timed sequence
		  * number was acked, update smoothed round trip time.
		  * Since we now have an rtt measurement, cancel the
		  * timer backoff (cf., Phil Karn's retransmit alg.).
		  * Recompute the initial retransmit timer.
		  */
		 
		 if (psock->t_rtt && SEQ_GT(NTOHL(pTcpHdr->ack_seq), psock->t_rtseq))
			 tcp_xmit_timer(psock,psock->t_rtt);

		 /*
		  * If all outstanding data is acked, stop retransmit
		  * timer and remember to restart (more output or persist).
		  * If there is more data to be acked, restart retransmit
		  * timer, using current (possibly backed-off) value.
		  */
		 if (NTOHL(pTcpHdr->ack_seq) == psock->snd_max){
			 psock->t_timer[TCPT_REXMT] = 0;
			 needoutput = 1;
		 } else if (psock->t_timer[TCPT_PERSIST] == 0)
			 psock->t_timer[TCPT_REXMT] = psock->t_rxtcur;

		 if (Acked > psock->snd_count){
			 psock->snd_wnd -= psock->snd_count;
			 sbdrop(psock,psock->snd_count);
			 ourfinisAcked = 1;
		 } else {
			 sbdrop(psock,Acked);
			 psock->snd_wnd -= Acked;
			 ourfinisAcked = 0;
		 }


		 sowwAkeup(psock);
		 psock->snd_una = NTOHL(pTcpHdr->ack_seq);
		 if (SEQ_LT(psock->snd_nxt,psock->snd_una))
			 psock->snd_nxt = psock->snd_una;
		 
		 switch(psock->state) {

		 /*
		  * In FIN_WAIT_1 STATE in addition to the processing
		  * for the ESTABLISHED state if our FIN is now acknowledged
		  * then enter FIN_WAIT_2.
		  */
		 case TCPS_FIN_WAIT_1:
			 if (ourfinisAcked){
				 /*
				  * If we can't receive any more
				  * data, then closing user can proceed.
				  * Starting the timer is contrary to the
				  * specification, but if we don't get a FIN
				  * we'll hang forever.
				  */
				 //soisdisconnected(psock);
				 psock->t_timer[TCPT_2MSL] = (short)tcp_maxidle;
				 psock->state = TCPS_FIN_WAIT_2;
			 }
			 break;
		/*
		 * In CLOSING STATE in addition to the processing for
		 * the ESTABLISHED state if the ACK acknowledges our FIN
		 * then enter the TIME-WAIT state, otherwise ignore
		 * the segment.
		 */
		 case TCPS_CLOSING:
			 if (ourfinisAcked) {
				 psock->state = TCPS_TIME_WAIT;
				 tcp_cAnceltimers(psock);
				 psock->t_timer[TCPT_2MSL] = 2* TCPTV_MSL;
				 //soisdisconnected(psock);
			 }
			 break;
		/*
		 * In LAST_ACK, we may still be waiting for data to drain
		 * and/or to be acked, as well as for the ack of our FIN.
		 * If our FIN is now acknowledged, delete the TCB,
		 * enter the closed state and return.
		 */
		 case TCPS_LAST_ACK:
			 if (ourfinisAcked) {
				 psock = tcp_close(psock);
				 goto drop;
			 }
			 break;
		/*
		 * In TIME_WAIT state the only thing that should arrive
		 * is a retransmission of the remote FIN.  Acknowledge
		 * it and restart the finack timer.
		 */
		 case TCPS_TIME_WAIT:
			 psock->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			 goto dropAfterAck;
		 }//switch
	 }
step6:

	 /*
	  * Update window information.
	  * Don't look at window if no ACK: TAC's send garbage on first SYN.
	  */
	 if ((pTcpHdr->ack) &&
		 (SEQ_LT(psock->snd_wl1,NTOHL(pTcpHdr->seq)) || psock->snd_wl1 == NTOHL(pTcpHdr->seq) &&
		 (SEQ_LT(psock->snd_wl2,NTOHL(pTcpHdr->ack_seq)) ||
		 psock->snd_wl2 == NTOHL(pTcpHdr->ack_seq) && NTOHS(pTcpHdr->window) > (USHORT)psock->snd_wnd))) {
		 /* keep track of pure window updates */
		 if (dAtAlen == 0 &&
			 psock->snd_wl2 == NTOHL(pTcpHdr->ack_seq) && NTOHS(pTcpHdr->window) > (USHORT)psock->snd_wnd){
			 //nothing 统计
		 }
		 psock->snd_wnd = NTOHS(pTcpHdr->window);
		 psock->snd_wl1 = NTOHL(pTcpHdr->seq);
		 psock->snd_wl2 = NTOHL(pTcpHdr->ack_seq);
		 if (psock->snd_wnd > psock->max_sndwnd)
			 psock->max_sndwnd = psock->snd_wnd;
		 needoutput = 1;
	 }
	 /*
	  * Process segments with URG.
	  */
	 //先不考虑URG

	 goto dodAtA;
dodAtA:

	 /*
	  * Process the segment text, merging it into the TCP sequencing queue,
	  * and arranging for acknowledgment of receipt if necessary.
	  * This process logically involves adjusting tp->rcv_wnd as data
	  * is presented to the user (this happens in tcp_usrreq.c,
	  * case PRU_RCVD).  If a FIN has already been received on this
	  * connection then we just ignore the text.
	  */
	 if ((dAtAlen || (pTcpHdr->fin)) &&
		 TCPS_HAVERCVDFIN(psock->state) == 0){
		 //乱序队列
		 TCP_REASS(psock,pIpHdr,pdAtA,dAtAlen);
	 } else{
		 pTcpHdr->fin = 0;
	 }

	 /*
	  * If FIN is received ACK the FIN and let the user know
	  * that the connection is closing.
	  */
	 if (pTcpHdr->fin){
		 if (TCPS_HAVERCVDFIN(psock->state) == 0){
			 //socAntrcvmore(psock);
			 psock->t_flags |= TF_ACKNOW;
			 psock->rcv_nxt++;
		 }
		 switch(psock->state){
		 /*
		  * In SYN_RECEIVED and ESTABLISHED STATES
		  * enter the CLOSE_WAIT state.
		  */
		 case TCPS_SYN_RECEIVED:
		 case TCPS_ESTABLISHED:
			 psock->state = TCPS_CLOSE_WAIT;
			 break;
		 /*
		  * If still in FIN_WAIT_1 STATE FIN has not been acked so
		  * enter the CLOSING state.
		  */
		 case TCPS_FIN_WAIT_1:
			 psock->state = TCPS_CLOSING;
			 break;
		 /*
		  * In FIN_WAIT_2 state enter the TIME_WAIT state,
		  * starting the time-wait timer, turning off the other 
		  * standard timers.
		  */
		 case TCPS_FIN_WAIT_2:
			 psock->state = TCPS_TIME_WAIT;
			 tcp_cAnceltimers(psock);
			 psock->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			 //soisdisconnected(psock);  //bug
			 break;
		 /*
		  * In TIME_WAIT state restart the 2 MSL time_wait timer.
		  */
		 case TCPS_TIME_WAIT:
			 psock->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			 break;
		 }//switch
	 }

	 //debug
		 //DbgPrint("4. psock->rcv_nxt = %d\n",(ULONG)psock->rcv_nxt);
		 //
	 /*
	  * Return any desired output.
	  */
	 if (needoutput || (psock->t_flags & TF_ACKNOW)){
		 tcp_output(psock);
	 }
	 return 0;

dropAfterAck:
	 /*
	  * Generate an ACK dropping incoming segment if it occupies
	  * sequence space, where the ACK reflects our state.
	  */
	 if (pTcpHdr->rst)
		 goto drop;
	 psock->t_flags |= TF_ACKNOW;
	 tcp_output(psock);

	 return 0;

dropwithreset:
	 /*
	  * Generate a RST, dropping incoming segment.
	  * Make ACK acceptable to originator of segment.
	  * Don't bother to respond if destination was broadcast/multicast.
	  */
	 if (pTcpHdr->ack)
		 tcp_respond(psock,pEthHdr,0,(tcp_seq)0,NTOHL(pTcpHdr->ack_seq),TH_RST);
	 else {
		 if (pTcpHdr->syn)
			 //ti->ti_len++
			 //pIpHdr->tot_len = HTONS(NTOHS(pIpHdr->tot_len) + 1);
			 dAtAlen++;
		 tcp_respond(psock,pEthHdr,0,NTOHL(pTcpHdr->seq) + dAtAlen,(tcp_seq)0,TH_RST|TH_ACK);
	 }
	 
	 return 0;

drop:
	 /*
	  * Drop space held by incoming segment and return.
	  */
	 //我们这里就不需要做什么了

	 return 0;
}
/*-----------------------------------------------------------------------------*/
void
tcp_setpersist(
	struct sock	*psock
	)
{
	int t;
	t = ((psock->t_srtt >> 2) + psock->t_rttvar) >> 1;
	if (psock->t_timer[TCPT_REXMT])
		DbgPrint("ERROR: tcp_output REXMT");
	/*
	 * Start/restart persistance timer.
	 */
	TCPT_RANGESET(psock->t_timer[TCPT_PERSIST],
		t * tcp_backoff[psock->t_rxtshift],
		TCPTV_PERSMIN, TCPTV_PERSMAX);
	if (psock->t_rxtshift < TCP_MAXRXTSHIFT)
		psock->t_rxtshift++;
}
/*-----------------------------------------------------------------------------*/
/*
 * Tcp output routine: figure out what should be sent and send it.
 */
int
tcp_output(
	struct sock* psock
	)
{
	int	len,sndwin,rcvwin;
	int off;
	int idle, sendAlot;
	int flAgs;
	int optlen,hdrlen;

	
	PVOID			pSendBuffer = NULL;
	PETHHDR			pEthHdrSend	= NULL;
	PIPHDR			pIpHdrSend	= NULL;
	PTCPHDR			pTcpHdrSend	= NULL;
	PVOID			pDAtA		= NULL;



	/*
	 * Determine length of data that should be transmitted,
	 * and flags that will be used.
	 * If there is some data or critical controls (SYN, RST)
	 * to send, then transmit; otherwise, investigate further.
	 */
	idle = (psock->snd_max == psock->snd_una);
	
AgAin:
	sendAlot	= 0;
	off			= psock->snd_nxt - psock->snd_una;
	sndwin		= psock->snd_wnd;
	flAgs		= tcp_outflAgs[psock->state];

	//debug
	DbgPrint("flAgs: %x\n",flAgs);
	//

	/*
	 * If in persist timeout with window of 0, send 1 byte.
	 * Otherwise, if window is small but nonzero
	 * and timer expired, we will send what we can
	 * and go to transmit state.
	 */
	if (psock->t_force){
		if (sndwin == 0){
			/*
			 * If we still have some data to send, then
			 * clear the FIN bit.  Usually this would
			 * happen below when it realizes that we
			 * aren't sending all the data.  However,
			 * if we have exactly 1 byte of unset data,
			 * then it won't clear the FIN bit below,
			 * and if we are in persist state, we wind
			 * up sending the packet without recording
			 * that we sent the FIN bit.
			 *
			 * We can't just blindly clear the FIN bit,
			 * because if we don't have any more data
			 * to send then the probe will be the FIN
			 * itself.
			 */
			if (off < psock->snd_count)
				flAgs &= ~TH_FIN;
			sndwin = 1;
		} else {
			psock->t_timer[TCPT_PERSIST] = 0;
			psock->t_rxtshift = 0;
		}
	}

	len = min(psock->snd_count,sndwin) - off;
	//debug
	//DbgPrint("in tcp_output, len = min(psock->snd_count,sndwin) - off; len=%d\n",len);
	//
	if (len < 0 ){
		/*
		 * If FIN has been sent but not acked,
		 * but we haven't been called to retransmit,
		 * len will be -1.  Otherwise, window shrank
		 * after we sent into it.  If window shrank to 0,
		 * cancel pending retransmit and pull snd_nxt
		 * back to (closed) window.  We will enter persist
		 * state below.  If the window didn't close completely,
		 * just wait for an ACK.
		 */
		len = 0;
		if (sndwin == 0){
			psock->t_timer[TCPT_REXMT] = 0;
			psock->snd_nxt = psock->snd_una;
		}
	}
	if (len > psock->t_maxseg){
		len = psock->t_maxseg;
		sendAlot = 1;
	}
	if (SEQ_LT(psock->snd_nxt + len,psock->snd_una + psock->snd_count))
		flAgs &= ~TH_FIN;

	rcvwin = sbspAce(psock);

	/*
	 * Sender silly window avoidance.  If connection is idle
	 * and can send all data, a maximum segment,
	 * at least a maximum default-size segment do it,
	 * or are forced, do it; otherwise don't bother.
	 * If peer's buffer is tiny, then send
	 * when window is at least half open.
	 * If retransmitting (possibly after persist timer forced us
	 * to send into a small window), then must resend.
	 */
	if (len){
		if (len == psock->t_maxseg)
			goto send;
		if ((idle || psock->t_flags & TF_NODELAY) &&
			len + off >= psock->snd_count)
			goto send;
		if (psock->t_force)
			goto send;
		if ((unsigned long)len >= psock->max_sndwnd /2)
			goto send;
		if (SEQ_LT(psock->snd_nxt,psock->snd_max))
			goto send;
	}

	/*
	 * Compare available window to amount of window
	 * known to peer (as advertised window less
	 * next expected input).  If the difference is at least two
	 * max size segments, or at least 50% of the maximum possible
	 * window, then want to send a window update to peer.
	 */
	if (rcvwin > 0){
		/* 
		 * "adv" is the amount we can increase the window,
		 * taking into account that we are limited by
		 * TCP_MAXWIN << tp->rcv_scale.
		 */
		//we don't use rcv_scAle for now
		long Adv = min(rcvwin,(long)TCP_MAXWIN) -
			(psock->rcv_adv - psock->rcv_nxt);
		//debug
		//DbgPrint("Adv = min(rcvwin,(long)TCP_MAXWIN) - (psock->rcv_adv - psock->rcv_nxt);  Adv=%d\n",Adv);
		//
		if (Adv >= (long)(2 * psock->t_maxseg)){
			//debug
			DbgPrint("becAuse of  Adv >= (long)(2 * psock->t_maxseg), so send A Ack\n");
			//
			goto send;
		}

		//debug
		DbgPrint("(long)(65535 - psock->rcv_count) : %d\n",(long)(65535 - psock->rcv_count));
		//
		if (2 * Adv >= (long)(65535 - psock->rcv_count)){
			//debug
			DbgPrint("becAuse of 2 * Adv >= (long)(psock->rcv_count), so send A Ack\n");
			//
			goto send;
		}
	}
	/*
	 * Send if we owe peer an ACK.
	 */
	if (psock->t_flags & TF_ACKNOW)
		goto send;
	if (flAgs & (TH_SYN|TH_RST))
		goto send;
	/*
	if (SEQ_GT(psock->snd_up,psock->snd_una))
		goto send;
	*/
	/*
	 * If our state indicates that FIN should be sent
	 * and we have not yet done so, or we're retransmitting the FIN,
	 * then we need to send.
	 */
	if ((flAgs & TH_FIN) &&
		((psock->t_flags & TF_SENTFIN) == 0 || psock->snd_nxt == psock->snd_una))
		goto send;

	/*
	 * TCP window updates are not reliable, rather a polling protocol
	 * using ``persist'' packets is used to insure receipt of window
	 * updates.  The three ``states'' for the output side are:
	 *	idle			not doing retransmits or persists
	 *	persisting		to move a small or zero window
	 *	(re)transmitting	and thereby not persisting
	 *
	 * tp->t_timer[TCPT_PERSIST]
	 *	is set when we are in persist state.
	 * tp->t_force
	 *	is set when we are called to send a persist packet.
	 * tp->t_timer[TCPT_REXMT]
	 *	is set when we are retransmitting
	 * The output side is idle when both timers are zero.
	 *
	 * If send window is too small, there is data to transmit, and no
	 * retransmit or persist is pending, then go to persist state.
	 * If nothing happens soon, send when timer expires:
	 * if window is nonzero, transmit what we can,
	 * otherwise force out a byte.
	 */
	if (psock->snd_count && psock->t_timer[TCPT_REXMT] == 0 &&
		psock->t_timer[TCPT_PERSIST] == 0){
		psock->t_rxtshift = 0;
		tcp_setpersist(psock);
	}
	/*
	 * No reason to send a segment, just return.
	 */
	return 0;

send:
	/*
	 * Before ESTABLISHED, force sending of initial options
	 * unless TCP set not to do any options.
	 * NOTE: we assume that the IP/TCP header plus TCP options
	 * always fit in a single mbuf, leaving room for a maximum
	 * link header, i.e.
	 *	max_linkhdr + sizeof (struct tcpiphdr) + optlen <= MHLEN
	 */
	optlen = 0;
	hdrlen = sizeof(struct _IPHDR) + sizeof(struct _TCPHDR);
	if (flAgs & TH_SYN){
		psock->snd_nxt = psock->iss;
		//先不弄选项 mss
	}

	/*
	 * Adjust data length if insertion of options will
	 * bump the packet length beyond the t_maxseg length.
	 */
	if (len > psock->t_maxseg - optlen){
		len = psock->t_maxseg - optlen;
		sendAlot = 1;
	}
	/*
	* Grab a header mbuf, attaching a copy of data to
	* be transmitted, and initialize the header from
	* the template for sends on this connection.
	*/
	NdisAllocateMemoryWithTag(
		&pSendBuffer,
		MAX_PACKET_SIZE,
		'pmtU'
		);
	memset(pSendBuffer,0,MAX_PACKET_SIZE);
	pEthHdrSend			= pSendBuffer;
	NdisMoveMemory(pEthHdrSend->h_dest,psock->dstmAc,6);
	NdisMoveMemory(pEthHdrSend->h_source,psock->srcmAc,6);
	pEthHdrSend->h_proto= HTONS(ETH_P_IP);
	
	pIpHdrSend			= (PIPHDR)((UCHAR*)pEthHdrSend + sizeof(ETHHDR));
	pIpHdrSend->ihl		= sizeof(IPHDR)/4;
	pIpHdrSend->version	= 4;
	pIpHdrSend->tos		= 0;
	pIpHdrSend->tot_len	= HTONS(sizeof(IPHDR) + sizeof(TCPHDR) + (USHORT)len);
	pIpHdrSend->id		= 0;
	pIpHdrSend->frag_off= 0;
	pIpHdrSend->ttl		= 255;
	pIpHdrSend->protocol= IPPROTO_TCP;
	pIpHdrSend->check	= 0;
	pIpHdrSend->saddr	= HTONL(psock->saddr);
	pIpHdrSend->daddr	= HTONL(psock->daddr);
	pIpHdrSend->check	= checksum((USHORT*)pIpHdrSend,sizeof(IPHDR));
	
	pTcpHdrSend			= (PTCPHDR)((UCHAR*)pIpHdrSend + pIpHdrSend->ihl * 4);
	pDAtA				= (PVOID)((UCHAR*)pTcpHdrSend + sizeof(TCPHDR));
	pTcpHdrSend->source	= HTONS(psock->sport);
	pTcpHdrSend->dest	= HTONS(psock->dport);
	pTcpHdrSend->doff	= sizeof(TCPHDR)/4;
	pTcpHdrSend->check	= 0;
	pTcpHdrSend->urg_ptr= 0;

	if (len){
		/*
		 * If we're sending everything we've got, set PUSH.
		 * (This will keep happy those implementations which only
		 * give data to the user when a buffer fills or
		 * a PUSH comes in.)
		 */	
		NdisMoveMemory(pDAtA,(char*)psock->snd_buff + off,len);

		/*
		if (off + len == psock->snd_count)
			flAgs |= TH_PUSH;
			*/
		//we set push
		flAgs |= TH_PUSH;
	} else {
		//no dAtA
	}
	/*
	 * Fill in fields, remembering maximum advertised
	 * window for use in delaying messages about window sizes.
	 * If resending a FIN, be sure not to use a new sequence number.
	 */
	if ((flAgs & TH_FIN) && ((psock->t_flags & TF_SENTFIN) == 0) &&
		psock->snd_nxt == psock->snd_max)
		psock->snd_nxt--;
	/*
	 * If we are doing retransmissions, then snd_nxt will
	 * not reflect the first unsent octet.  For ACK only
	 * packets, we do not want the sequence number of the
	 * retransmitted packet, we want the sequence number
	 * of the next unsent octet.  So, if there is no data
	 * (and no SYN or FIN), use snd_max instead of snd_nxt
	 * when filling in ti_seq.  But if we are in persist
	 * state, snd_max might reflect one byte beyond the
	 * right edge of the window, so use snd_nxt in that
	 * case, since we know we aren't doing a retransmission.
	 * (retransmit and persist are mutually exclusive...)
	 */
	if (len || (flAgs & (TH_SYN|TH_FIN)) || psock->t_timer[TCPT_PERSIST])
		pTcpHdrSend->seq = HTONL(psock->snd_nxt);
	else
		pTcpHdrSend->seq = HTONL(psock->snd_max);

	pTcpHdrSend->ack_seq = HTONL(psock->rcv_nxt);
	pTcpHdrSend->rst = ((flAgs & TH_RST)?1:0);
	pTcpHdrSend->ack = ((flAgs & TH_ACK)?1:0);
	pTcpHdrSend->syn = ((flAgs & TH_SYN)?1:0);
	pTcpHdrSend->fin = ((flAgs & TH_FIN)?1:0);
	pTcpHdrSend->psh = ((flAgs & TH_PUSH)?1:0);
	//debug
	DbgPrint("flAgs: %x\n",flAgs);
	//
	/*
	 * Calculate receive window.  Don't shrink window,
	 * but avoid silly window syndrome.
	 */
	if (rcvwin < (long)(psock->rcv_count / 4) && rcvwin < (long)psock->t_maxseg)
		rcvwin = 0;
	if (rcvwin > (long)TCP_MAXWIN)
		rcvwin = (long)TCP_MAXWIN;
	if (rcvwin < (long)(psock->rcv_adv - psock->rcv_nxt))
		rcvwin = (long)(psock->rcv_adv - psock->rcv_nxt);
	pTcpHdrSend->window = HTONS((USHORT)rcvwin);

	//计算checksum
	{
	PSDHDR			PsdHdr;
	PVOID			pChecksumTempBuffer = NULL;
	PsdHdr.saddr	= pIpHdrSend->saddr;
	PsdHdr.daddr	= pIpHdrSend->daddr;
	PsdHdr.mbz		= 0;
	PsdHdr.ptcl		= IPPROTO_TCP;
	PsdHdr.tcpl		= HTONS(sizeof(TCPHDR) + (USHORT)len);

	NdisAllocateMemoryWithTag(
		&pChecksumTempBuffer,
		MAX_PACKET_SIZE,
		'pmtU'
		);
	memset(pChecksumTempBuffer,0,MAX_PACKET_SIZE);
	NdisMoveMemory(pChecksumTempBuffer,&PsdHdr,sizeof(PSDHDR));
	//debug
	//DbgPrint("len: %d\n",(USHORT)len);
	//
	NdisMoveMemory((UCHAR*)pChecksumTempBuffer + sizeof(PSDHDR),pTcpHdrSend,sizeof(TCPHDR) + len);
	pTcpHdrSend->check	= checksum((USHORT*)pChecksumTempBuffer,sizeof(PSDHDR)+sizeof(TCPHDR)+len);
	NdisFreeMemory(
		pChecksumTempBuffer,
		0,
		0
		);
	}

	/*
	 * In transmit state, time the transmission and arrange for
	 * the retransmit.  In persist state, just set snd_max.
	 */
	if (psock->t_force == 0 || psock->t_timer[TCPT_PERSIST] ==0){
		tcp_seq stArtseq = psock->snd_nxt;

		/*
		* Advance snd_nxt over sequence space of this segment.
		*/
		if (flAgs & (TH_SYN|TH_FIN)){
			if (flAgs & TH_SYN)
				psock->snd_nxt++;
			if(flAgs & TH_FIN){
				psock->snd_nxt++;
				psock->t_flags |= TF_SENTFIN;
			}
		}
		psock->snd_nxt += len;
		if (SEQ_GT(psock->snd_nxt,psock->snd_max)){
			psock->snd_max = psock->snd_nxt;
			/*
			 * Time this transmission if not a retransmission and
			 * not currently timing anything.
			 */
			if (psock->t_rtt == 0){
				psock->t_rtt = 1;
				psock->t_rtseq = stArtseq;
			}

		}
		/*
		 * Set retransmit timer if not currently set,
		 * and not doing an ack or a keep-alive probe.
		 * Initial value for retransmit timer is smoothed
		 * round-trip time + 2 * round-trip time variance.
		 * Initialize shift counter which is used for backoff
		 * of retransmit time.
		 */
		if (psock->t_timer[TCPT_REXMT] == 0 &&
			psock->snd_nxt != psock->snd_una){
			psock->t_timer[TCPT_REXMT] = psock->t_rxtcur;
			if (psock->t_timer[TCPT_PERSIST]){
				psock->t_timer[TCPT_PERSIST] = 0;
				psock->t_rxtshift = 0;
			}
		}

	} else{
		if (SEQ_GT(psock->snd_nxt + len,psock->snd_max))
			psock->snd_max = psock->snd_nxt + len;
	}
	
	//发包
	{
	PNDIS_BUFFER	pNdisBuffer = NULL;
	PNDIS_PACKET	pNdisPAcket = NULL;
	NTSTATUS		stAtus;
	NdisAllocateBuffer(
		&stAtus,
		&pNdisBuffer,
		m_ourBufferPoolHAndle,
		pSendBuffer,
		sizeof(ETHHDR)+sizeof(IPHDR)+sizeof(TCPHDR)+len
		);
	if (stAtus != NDIS_STATUS_SUCCESS){
		DbgPrint("ERROR:NdisAllocAteBuffer fAiled\n");
		//return 0;
		goto send_fAil;
	}
	NdisAllocatePacket(
		&stAtus,
		&pNdisPAcket,
		m_ourPAcketPoolHAndle
		);
	if (stAtus != NDIS_STATUS_SUCCESS){
		DbgPrint("ERROR:NdisAllocAtePAcket fAiled\n");
		//return 0;
		goto send_fAil;
	}
	NdisChainBufferAtFront(
		pNdisPAcket,
		pNdisBuffer
		);
	if (g_BindAdAptHAndle == NULL){
		DbgPrint("ERROR: BindAdAptHAndle is NULL\n");
		//return 0;
		goto send_fAil;
	}
	NdisSendPackets(
		g_BindAdAptHAndle,//这个变量还未实现
		&pNdisPAcket,
		1
		);
	}

	/*
	 * Data sent (as far as we can tell).
	 * If this advertises a larger window than any other segment,
	 * then remember the size of the advertised window.
	 * Any pending ACK has now been sent.
	 */
	if (rcvwin > 0 && SEQ_GT(psock->rcv_nxt+rcvwin,psock->rcv_adv))
		psock->rcv_adv = psock->rcv_nxt + rcvwin;
	//psock->last_ack_sent = psock->rcv_nxt;
	psock->t_flags &= ~(TF_ACKNOW|TF_DELACK);
	if (sendAlot){
		//debug
		DbgPrint("send A lot !!!   psock->t_maxseg = %d,len = %d\n",psock->t_maxseg,len);
		//
		goto AgAin;
	}

send_fAil:
	return 0;
}
/*-----------------------------------------------------------------------------*/
/*
 * Tcp initialization
 */
void
tcp_init()
{
	int i;
	for(i = 0;i < SOCK_MAX;i++){
		memset(&tcp_conn_pool[i],0,sizeof(struct sock));
		tcp_conn_pool[i].rcv_count	= 0;
		tcp_conn_pool[i].snd_count	= 0;
		tcp_conn_pool[i].t_maxseg	= TCP_MSS;


		/*
		 * Init srtt to TCPTV_SRTTBASE (0), so we can tell that we have no
		 * rtt estimate.  Set rttvar so that srtt + 2 * rttvar gives
		 * reasonable initial retransmit time.
		 */
		tcp_conn_pool[i].t_srtt = TCPTV_SRTTBASE;
		tcp_conn_pool[i].t_rttvar = tcp_rttdflt * PR_SLOWHZ << 2;
		tcp_conn_pool[i].t_rttmin = TCPTV_MIN;
		TCPT_RANGESET(tcp_conn_pool[i].t_rxtcur, 
			((TCPTV_SRTTBASE >> 2) + (TCPTV_SRTTDFLT << 2)) >> 1,
			TCPTV_MIN, TCPTV_REXMTMAX);
		/*
		KeInitializeEvent(
			&tcp_conn_pool[i].snd_event,
			SynchronizationEvent,
			TRUE
			);
		KeInitializeEvent(
			&tcp_conn_pool[i].rcv_event,
			SynchronizationEvent,
			FALSE
			);
			*/
		KeInitializeEvent(
			&tcp_conn_pool[i].connect_event,
			SynchronizationEvent,
			FALSE
			);
		KeInitializeEvent(
			&tcp_conn_pool[i].accept_event,
			SynchronizationEvent,
			FALSE
			);

		KeInitializeSpinLock(&(tcp_conn_pool[i].sock_spin));
	}
	tcp_iss = 1;		/* wrong */
}
/*-----------------------------------------------------------------------------*/