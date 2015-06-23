///////////////////////////////////////////////////////////
//  RemoteNetwork.h
//  Implementation of the Class RemoteNetwork
//  Created on:      24-Dec-2003 11:58:25 AM
///////////////////////////////////////////////////////////

#if !defined(RemoteNetwork_664264D2_533B_43d2_BB6B_A0A953DD4000__INCLUDED_)
#define RemoteNetwork_664264D2_533B_43d2_BB6B_A0A953DD4000__INCLUDED_

#include "Network.h"
#include "TunnelEngine.h"

class RemoteNetwork : public Network 
{
protected:
	SOCKET m_socket;
public:
	RemoteNetwork(SOCKET s);
	virtual ~RemoteNetwork();

	virtual void StartTunnel();
	virtual void ProcessRoutedPacket(blackpkt_t *theRoutedPacket);

	virtual void SendPacket( unsigned char *pkt, unsigned int len);
	virtual void Shutdown();

public:
	TunnelEngine *m_TunnelEngine;
};
#endif // !defined(RemoteNetwork_664264D2_533B_43d2_BB6B_A0A953DD4000__INCLUDED_)