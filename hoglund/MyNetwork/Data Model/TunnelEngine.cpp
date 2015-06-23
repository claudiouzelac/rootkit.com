///////////////////////////////////////////////////////////
//  TunnelEngine.cpp
//  Implementation of the Class TunnelEngine
//  Created on:      24-Dec-2003 11:58:27 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RemoteNetwork.h"
#include "TunnelEngine.h"
#include "UserConsole.h"

TunnelEngine::TunnelEngine(SOCKET s, RemoteNetwork *theParent) :
m_socket(s),
m_parent(theParent)
{

}



TunnelEngine::~TunnelEngine()
{

}

void TunnelEngine::ProcessRoutedPacket(blackpkt_t *theRoutedPacket)
{
	m_parent->ProcessRoutedPacket(theRoutedPacket);
}

void TunnelEngine::SendPacket(unsigned char *pkt, unsigned int len)
{
	blackpkt_t *bpkt = new blackpkt_t;
	bpkt->type = BLACK_SERVER_PACKET;
	bpkt->len = len;
	
	memcpy(&bpkt->pkt, pkt, len);

	LockMutex();
	sendqueue.push(bpkt);
	UnlockMutex();
}

void TunnelEngine::Run()
{
	unsigned int readlen = 0;
	char buf[sizeof(blackpkt_t) + 10]; //add a little padding
	FD_SET cread;
	blackpkt_t *bpkt = NULL;
	
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 5000;

	while(1)
	{
		int ret;
		FD_ZERO(&cread);
		FD_SET(m_socket, &cread);

		if(m_stop_flag)
		{
			break;
		}

		ret = select(1, &cread, NULL, NULL, &tv);
		if( 1 == ret ) 
		{
			readlen = recv(m_socket, buf, sizeof(blackpkt_t), NULL);
			if(SOCKET_ERROR == readlen)
			{
				TRACE1("recv() error %d, shutting down client thread", WSAGetLastError());
				break;
			}
			else if(readlen > 0)
			{
				bpkt = (blackpkt_t *)buf;
				TRACE1("[client ->] Got pkt: len: %d\n", bpkt->len);

				// Process the routed packet
				ProcessRoutedPacket(bpkt);
			}
		}
		else if( 0 == ret )
		{
			// No packets from remote network	
		}
		else
		{
			int err = WSAGetLastError();
			TRACE1("select() error %d, shutting down client thread", err);
			break;
		}

		LockMutex();
		// Packets to send?
		while(!sendqueue.empty())
		{
			bpkt = sendqueue.front();
			sendqueue.pop();

			if(bpkt)
			{
				if(SOCKET_ERROR == send(m_socket, (char *)bpkt, (bpkt->len + BPKT_HEADER_LEN), NULL))
				{
					TRACE1("send() error %d, shutting down client thread", WSAGetLastError());
					delete bpkt;
					break;
				}
				delete bpkt;
			}
		}
		UnlockMutex();

		Sleep(1);
	}
	closesocket(m_socket);

	// make sure we are pulled from the list of available networks
	// FIXME, right now this object is a memleak - we need to housekeep this
	// removal better...
	m_parent->m_database->RemoveNetwork(m_parent);
}

