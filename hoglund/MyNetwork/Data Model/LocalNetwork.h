///////////////////////////////////////////////////////////
//  LocalNetwork.h
//  Implementation of the Class LocalNetwork
//  Created on:      24-Dec-2003 11:58:23 AM
///////////////////////////////////////////////////////////

#if !defined(LocalNetwork_CFAA3493_242B_4558_8BB5_BDA5EAA46C25__INCLUDED_)
#define LocalNetwork_CFAA3493_242B_4558_8BB5_BDA5EAA46C25__INCLUDED_

#include "Network.h"
#include "SniffPacketThread.h"
#include "SendPacketThread.h"
#include <string>

class LocalNetwork : public Network 
{
protected:
	SniffPacketThread m_sniffer;
	SendPacketThread m_sender;

public:
	LocalNetwork();
	virtual ~LocalNetwork();
	virtual void StartThreads();
	virtual void ProcessSniffedPacket(unsigned char *pkt, unsigned int len);
	std::string devname;
	std::string devdesc;
	unsigned int num;	

	virtual void SendPacket( unsigned char *pkt, unsigned int len);
	virtual void Shutdown();
};
#endif // !defined(LocalNetwork_CFAA3493_242B_4558_8BB5_BDA5EAA46C25__INCLUDED_)