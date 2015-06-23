///////////////////////////////////////////////////////////
//  SendPacketThread.h
//  Implementation of the Class SendPacketThread
//  Created on:      24-Dec-2003 11:58:26 AM
///////////////////////////////////////////////////////////

#if !defined(SendPacketThread_8B8A2C19_1571_4624_923B_36E63BF3799D__INCLUDED_)
#define SendPacketThread_8B8A2C19_1571_4624_923B_36E63BF3799D__INCLUDED_

#include "EngineThread.h"
#include "RoutedPacket.h"
class LocalNetwork;

class SendPacketThread : public EngineThread 
{
public:
	LPADAPTER m_lpa;
	LocalNetwork *m_parent;

public:
	SendPacketThread();
	virtual ~SendPacketThread();

	virtual void Run();

	virtual void SendPacket( unsigned char *pkt, unsigned int len);

	//packets to send to local network
	pktqueue_t recvqueue;
};
#endif // !defined(SendPacketThread_8B8A2C19_1571_4624_923B_36E63BF3799D__INCLUDED_)