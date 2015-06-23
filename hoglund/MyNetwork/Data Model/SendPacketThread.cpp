///////////////////////////////////////////////////////////
//  SendPacketThread.cpp
//  Implementation of the Class SendPacketThread
//  Created on:      24-Dec-2003 11:58:26 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SendPacketThread.h"
#include "LocalNetwork.h"


SendPacketThread::SendPacketThread() :
m_parent(0)
{

}



SendPacketThread::~SendPacketThread()
{

}

void SendPacketThread::SendPacket( unsigned char *pkt, unsigned int len)
{
	blackpkt_t *bpkt = new blackpkt_t;
	bpkt->type = BLACK_SERVER_PACKET;
	bpkt->len = len;
	
	memcpy(&bpkt->pkt, pkt, len);

	LockMutex();
	recvqueue.push(bpkt);
	UnlockMutex();
}

void SendPacketThread::Run()
{
	while(1)
	{
		if(m_stop_flag) break;

		LockMutex();
		while(!recvqueue.empty())
		{
			PACKET pkt;
			blackpkt_t *bpkt = recvqueue.front();
			recvqueue.pop();
			
			if(bpkt)
			{
				pkt.Buffer = bpkt->pkt;
				pkt.Length = bpkt->len;
				
				TRACE2("%s: queue, writing a routed packet, len %d, to local network\n", m_parent->devdesc.c_str(), pkt.Length);
				if(0 == PacketSendPacket(m_lpa, &pkt, false))
				{
					TRACE0("WARNING, PacketSendPacket failed\n");
				}
				delete bpkt;
			}
		}
		UnlockMutex();

		Sleep(1);
	}
}

