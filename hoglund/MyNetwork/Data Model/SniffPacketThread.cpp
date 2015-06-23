///////////////////////////////////////////////////////////
//  SniffPacketThread.cpp
//  Implementation of the Class SniffPacketThread
//  Created on:      24-Dec-2003 11:58:27 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SniffPacketThread.h"
#include "LocalNetwork.h"

SniffPacketThread::SniffPacketThread() :
m_parent(0),
m_lpa(0),
m_lpp(0),
m_enabled(FALSE),
m_handle(0)
{

}



SniffPacketThread::~SniffPacketThread()
{

}

bool SniffPacketThread::OpenAdapter(char *theAdapterName)
{
	m_lpa = PacketOpenAdapter(theAdapterName);

	if(m_lpa == NULL)
	{
		TRACE0("PacketOpenAdapter failed");
		return(FALSE);
	}

	// Allocate packet mem
	m_lpp = PacketAllocatePacket();
	if( m_lpp == NULL)
	{
		TRACE0("PacketAllocatePacket failed");
		PacketCloseAdapter(m_lpa);
		return(FALSE);
	}

	PacketInitPacket(m_lpp, m_pbuf, 512000);

	// Set Kernel Buffer Size
	if( PacketSetBuff(m_lpa, 512000) == FALSE 
		|| 
		PacketSetReadTimeout(m_lpa, 1000) == FALSE)
	{
		PacketCloseAdapter(m_lpa);
		PacketFreePacket(m_lpp);
		m_enabled = FALSE;
		m_handle = NULL;
		return(FALSE);
	}

	PacketSetHwFilter(m_lpa, NDIS_PACKET_TYPE_PROMISCUOUS);
	
	return(TRUE);
}

void SniffPacketThread::Run()
{
	u_int tlen = 0, offset = 0;
	struct bpf_hdr *hdr = NULL;
		
	while(1)
	{
		if(m_stop_flag) break;

		try
		{
			if(PacketReceivePacket(m_lpa, m_lpp, TRUE) == FALSE)
				continue;
		}
		catch(...)
		{
			TRACE0("Exception in packet receive thread");
			return;
		}
		
		// Grab total length
		tlen = m_lpp->ulBytesReceived;

		// Process all the packets we've been handed
		offset = 0;
		while(offset < tlen)
		{
			hdr = (struct bpf_hdr *)(m_pbuf + offset);
			offset += hdr->bh_hdrlen;
			unsigned char *pkt = (unsigned char *)(m_pbuf + offset);
			u_int len = hdr->bh_caplen;
			
			// Adjust offset to next pkt w/ alignment
			offset = Packet_WORDALIGN(offset + hdr->bh_caplen);
						
			// we don't need to free the packet since it's static
			// for this loop
			m_parent->ProcessSniffedPacket( pkt, len );
		}
		Sleep(1);
	}
}