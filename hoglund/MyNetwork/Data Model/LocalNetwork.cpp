///////////////////////////////////////////////////////////
//  LocalNetwork.cpp
//  Implementation of the Class LocalNetwork
//  Created on:      24-Dec-2003 11:58:23 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LocalNetwork.h"
#include "NetworkDatabase.h"

LocalNetwork::LocalNetwork()
{

}



LocalNetwork::~LocalNetwork()
{

}

void LocalNetwork::StartThreads()
{
	m_sniffer.m_parent = this;
	m_sniffer.OpenAdapter((char *)devname.c_str());
	m_sender.m_parent = this;
	m_sender.m_lpa = m_sniffer.m_lpa;
	
	// shared from sniffer thread
	// make sure to kill sender before shutting down sniffer
	m_sniffer.Start();
	m_sender.Start();
}

void LocalNetwork::SendPacket( unsigned char *pkt, unsigned int len)
{
	TRACE1("%s: sending a packet to my local network\n", this->devdesc.c_str());
	
	// update the counter
	m_number_tx++;
	m_sender.SendPacket(pkt, len);
}

// we sniffed a packet on the local network
// if the destination MAC is broadcast or non-local, then route this packet
void LocalNetwork::ProcessSniffedPacket(unsigned char *pkt, unsigned int len)
{
	assert(m_database);
	struct ether_header *eth = NULL;
	
	if(len >= sizeof(struct ether_header))
	{
		// Setup ether header
		eth = (struct ether_header *)(pkt);

		// LOCK the database so our source and dest nets don't get
		// deleted out from under us...
		m_database->LockMutex();

		Network *aSourceNet = m_database->FindNetworkByMAC(eth->h_source);
		Network *aDestNet = m_database->FindNetworkByMAC( eth->h_dest );
				
#if 0
		char _g[1024];
		_snprintf(_g, 1022, "sniffed packet, iface %s, SRC MAC %.02X:%.02X:%.02X:%.02X:%.02X:%.02X DEST MAC %.02X:%.02X:%.02X:%.02X:%.02X:%.02X\n", 
			this->devdesc.c_str(),
			eth->h_source[0], eth->h_source[1], eth->h_source[2], 
			eth->h_source[3], eth->h_source[4], eth->h_source[5],
			eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], 
			eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]				
			);
		TRACE0(_g);
#endif		

		////////////////////////////////////////////////////////////////////
		// if there is no network entry for the source network, then the
		// packet must be a locally generated one, and we can safely add
		// a new entry for the MAC in question
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
		
			TRACE1("%s: Added MAC DB entry\n", devdesc.c_str());
		}

		if(aSourceNet == this)
		{
			// the packet was locally generated, update the counter
			m_number_rx++;
		}

		/////////////////////////////////////////////////////////////////
		// if the destination MAC is broadcast, and the source
		// is from our own network,
		// route the packet to all other tunnels except ourselves
		/////////////////////////////////////////////////////////////////
		if(0 == memcmp(eth->h_dest, bcastaddr, 6))
		{
			if(aSourceNet == this)
			{
				TRACE1("%s: routing a locally generated broadcast packet\n", devdesc.c_str()); 
					
				// this is a broadcast packet originating
				// from our local net...
				// route to all other networks
				NETWORK_LIST::iterator i = m_database->m_network_list.begin();
				while(i != m_database->m_network_list.end())
				{
					Network *aNetwork = *i;
	
					// don't send a packet back to the originating interface (loop prevention)
					if(aNetwork != this)
					{
						// remember that each local interface is treated as a 
						// seperate network, thus broadcasts on one interface
						// are forwarded to all other local interfaces.  This
						// transforms the combined local interfaces into a switch
						aNetwork->SendPacket(pkt, len);
					}
					else
					{
						TRACE1("%s: skipping ourselves when routing broadcast packet\n", this->devdesc.c_str());
					}
					i++;
				}
			}
		}
		else
		{
			// this is a directed unicast packet
			if(!aDestNet)
			{
				// we don't know where this packet is supposed to go
				// this happens when:
				// 1. we have not yet observed an ARP REQ/RESPONSE for a given IP
				// 2. the target of the packet is the local network, but we have never
				//    seen the target generate packets yet (so we don't have it's MAC)
				//
				// 3. the target is on a remote network, but the local network already 
				//    has an ARP entry for the target, thus it has not generated an
				//    ARP REQ/RES for us to sniff (this can happen if you delete and
				//    then re-establish a tunnel)
				//
				//		a. a quick-fix is to delete your ARP cache w/ arp -d
				//
				// 4. the target MAC is from a static ARP entry
				// 5. the target MAC is hard coded someplace
				//
				// TODO: In the case of IP, we can artificially generate another ARP
				// request for the destination IP address, thus initializing our
				// tables.
			
			
				// TESTING THIS FIX
				// if we don't know where the packet is supposed to go, and the target
				// is not on our local network, then send the packet to _all_
				// remote networks (just like a broadcast packet)
				// this will result in no response in the destination network if
				// the target is not there so no harm done, but if the target is in
				// the remote network the tables will get updated and everyone will
				// route properly in the future
				if(aSourceNet == this)
				{
					TRACE1("%s: locally generated packet, unknown dest mac, being routed to all remote networks\n", devdesc.c_str());
				
					// this is a unicast packet originating
					// from our local net...
					// route to all other networks
					NETWORK_LIST::iterator i = m_database->m_network_list.begin();
					while(i != m_database->m_network_list.end())
					{
						Network *aNetwork = *i;
		
						// don't send a packet back to the originating interface (loop prevention)
						if(aNetwork != this)
						{
							aNetwork->SendPacket(pkt, len);
						}
						else
						{
							TRACE1("%s: skipping ourselves when routing a unknown unicast packet\n", this->devdesc.c_str());
						}
						i++;
					}
				}
			}
			else if(aDestNet != this) // make sure we don't route to ourselves
			{

				TRACE1("%s: Routing a unicast packet over a tunnel..\n", devdesc.c_str());

				// there is another network that wants this packet
				aDestNet->SendPacket(pkt, len);
			}
		}
	
		// release the database lock OR wedge the whole system
		m_database->UnlockMutex();
	}
}

void LocalNetwork::Shutdown()
{
	m_sniffer.Stop();
	m_sender.Stop();
}