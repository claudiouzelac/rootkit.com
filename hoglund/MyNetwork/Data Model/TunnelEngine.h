///////////////////////////////////////////////////////////
//  TunnelEngine.h
//  Implementation of the Class TunnelEngine
//  Created on:      24-Dec-2003 11:58:27 AM
///////////////////////////////////////////////////////////

#if !defined(TunnelEngine_805876DF_0C9B_40af_898D_E3A6714C5596__INCLUDED_)
#define TunnelEngine_805876DF_0C9B_40af_898D_E3A6714C5596__INCLUDED_

#include "EngineThread.h"
#include "RoutedPacket.h"

class RemoteNetwork;

class TunnelEngine : public EngineThread 
{
protected:
	SOCKET m_socket;
	RemoteNetwork *m_parent;
	pktqueue_t sendqueue;
	pktqueue_t recvqueue;

public:
	TunnelEngine(SOCKET s, RemoteNetwork *theParent);
	virtual ~TunnelEngine();

	// handle the send/recv of packets from the remote network
	virtual void Run();
	virtual void ProcessRoutedPacket(blackpkt_t *theRoutedPacket);
	virtual void SendPacket(unsigned char *pkt, unsigned int len);
};
#endif // !defined(TunnelEngine_805876DF_0C9B_40af_898D_E3A6714C5596__INCLUDED_)