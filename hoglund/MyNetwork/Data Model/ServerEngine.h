///////////////////////////////////////////////////////////
//  ServerEngine.h
//  Implementation of the Class ServerEngine
//  Created on:      24-Dec-2003 11:58:26 AM
///////////////////////////////////////////////////////////

#if !defined(ServerEngine_2E5D1646_48C1_44cf_8F26_9BC542880F83__INCLUDED_)
#define ServerEngine_2E5D1646_48C1_44cf_8F26_9BC542880F83__INCLUDED_

#include "EngineThread.h"
#include "RemoteNetwork.h"

class NetworkDatabase;

/**
 * Accepts connections for new tunnels
 */
class ServerEngine : public EngineThread 
{
protected:
	unsigned short m_tcp_port;
	NetworkDatabase *m_network_db;
	
public:
	ServerEngine(unsigned short tcp_port, NetworkDatabase *theDB);
	virtual ~ServerEngine();

	virtual void Run();
	virtual void Stop();

};

#endif // !defined(ServerEngine_2E5D1646_48C1_44cf_8F26_9BC542880F83__INCLUDED_)