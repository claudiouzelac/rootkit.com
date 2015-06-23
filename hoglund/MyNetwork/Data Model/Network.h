///////////////////////////////////////////////////////////
//  Network.h
//  Implementation of the Class Network
//  Created on:      24-Dec-2003 11:58:24 AM
///////////////////////////////////////////////////////////

#if !defined(Network_994F6B75_B304_495b_93A7_8E056BD79CE0__INCLUDED_)
#define Network_994F6B75_B304_495b_93A7_8E056BD79CE0__INCLUDED_

#include "ThreadsafeObject.h"
#include <string>

class NetworkDatabase;

class Network : public ThreadsafeObject 
{
public:
	DWORD m_number_tx;	//transmitted to the network
	DWORD m_number_rx;	//received from the network

public:
	Network();
	virtual ~Network();

	virtual void SendPacket( unsigned char *pkt, unsigned int len);
	
	// override to shutdown network connection
	virtual void Shutdown();

	NetworkDatabase *m_database;
	std::string m_name; // a display name for this network (must be unique)
};
#endif // !defined(Network_994F6B75_B304_495b_93A7_8E056BD79CE0__INCLUDED_)