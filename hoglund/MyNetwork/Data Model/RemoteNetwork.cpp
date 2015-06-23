///////////////////////////////////////////////////////////
//  RemoteNetwork.cpp
//  Implementation of the Class RemoteNetwork
//  Created on:      24-Dec-2003 11:58:25 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RemoteNetwork.h"
#include "NetworkDatabase.h"

RemoteNetwork::RemoteNetwork(SOCKET s) :
m_socket(s),
m_TunnelEngine(0)
{

}



RemoteNetwork::~RemoteNetwork()
{
	if(m_TunnelEngine) delete m_TunnelEngine;	
}

void RemoteNetwork::SendPacket( unsigned char *pkt, unsigned int len)
{
	if(m_TunnelEngine)
	{
		//update the counter
		m_number_tx++;

		m_TunnelEngine->SendPacket(pkt, len);
	}
}

void RemoteNetwork::ProcessRoutedPacket(blackpkt_t *theRoutedPacket)
{
	assert(m_database);
	struct ether_header *eth = NULL;
	unsigned char *pkt = &(theRoutedPacket->pkt[0]);
	int len = theRoutedPacket->len;

	if(len >= sizeof(struct ether_header))
	{
		// Setup ether header
		eth = (struct ether_header *)(pkt);			
		Network *aSourceNet = m_database->FindNetworkByMAC(eth->h_source);
		Network *aDestNet = m_database->FindNetworkByMAC( eth->h_dest );
				
#if 1
		char _g[1024];
		_snprintf(_g, 1022, "got routed packet SRC MAC %.02X:%.02X:%.02X:%.02X:%.02X:%.02X DEST MAC %.02X:%.02X:%.02X:%.02X:%.02X:%.02X\n", 
			eth->h_source[0], eth->h_source[1], eth->h_source[2], 
			eth->h_source[3], eth->h_source[4], eth->h_source[5],
			eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], 
			eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]				
			);
		TRACE0(_g);
#endif		

		////////////////////////////////////////////////////////////////////
		// if there is no network entry for the source network, then the
		// packet must be generated from the remote network 
		// and we can safely add a new entry for the MAC in question
		//
		// dbl check that src MAC isn't broadcast - sometimes this happens
		////////////////////////////////////////////////////////////////////
		if( (0 == aSourceNet)
			&&
			(0 != memcmp(eth->h_source, bcastaddr, 6)))
		{
			aSourceNet = this;
			__int64 aMAC = 0;
			memcpy(&aMAC, eth->h_source, 6);
			m_database->m_network_map[aMAC] = this;
		
			TRACE0("Added MAC DB entry for remote network");
		}

		if(aSourceNet == this)
		{
			// we just got a packet from the remote network
			// update the counter
			m_number_rx++;
		}

		/////////////////////////////////////////////////////////////////
		// if the destination MAC is broadcast
		// route the packet to all other tunnels except ourselves
		/////////////////////////////////////////////////////////////////
		if(0 == memcmp(eth->h_dest, bcastaddr, 6))
		{
			// route to all other networks
			NETWORK_LIST::iterator i = m_database->m_network_list.begin();
			while(i != m_database->m_network_list.end())
			{
				Network *aNetwork = *i;
				
				// don't send a packet back to ourselves
				if(aNetwork != this)
				{
					aNetwork->SendPacket(pkt, len);
				}
				i++;
			}
		}
		else
		{
			// this is not a broadcast packet
			// make sure we don't route to ourselves
			if(	aDestNet 
				&& 
				(aDestNet != this))
			{
				// there is another network that wants this packet
				aDestNet->SendPacket(pkt, len);
			}
		}
	}
}

void RemoteNetwork::StartTunnel()
{
	if(!m_TunnelEngine)
	{
		m_TunnelEngine = new TunnelEngine(m_socket, this);
	}

	// this causes the tunnel to run in it's own thread
	// it will handle send/recv of packets over the tunnel
	// to the remote network
	m_TunnelEngine->Start();
}

void RemoteNetwork::Shutdown()
{
	if(m_TunnelEngine)
	{
		// kill the thread that is handling events for this network
		m_TunnelEngine->Stop();
	}
}