///////////////////////////////////////////////////////////
//  NetworkDatabase.h
//  Implementation of the Class NetworkDatabase
//  Created on:      24-Dec-2003 11:58:24 AM
///////////////////////////////////////////////////////////

#if !defined(NetworkDatabase_DB37EE6F_A0A5_4832_AAA1_98D17991F7AE__INCLUDED_)
#define NetworkDatabase_DB37EE6F_A0A5_4832_AAA1_98D17991F7AE__INCLUDED_

#include "Network.h"
#include "ThreadsafeObject.h"

#include <map>
#include <list>

typedef std::map<__int64, Network *> MAC_TO_NETWORK_DB;
typedef std::list<Network *> NETWORK_LIST;

/**
 * The list of attached networks.  Associates MAC addresses with the appropriate
 * network.  The local network is included in this database
 */
class NetworkDatabase : public ThreadsafeObject 
{

public:
	NetworkDatabase();
	virtual ~NetworkDatabase();

	virtual void AddNetwork( Network *theNetwork );
	virtual void RemoveNetwork( Network *theNetwork );

public:
	// used to lookup the target network for a given packet
	MAC_TO_NETWORK_DB m_network_map;
	NETWORK_LIST m_network_list;

	Network * FindNetworkByMAC( unsigned char theMAC[6] );
	Network * FindNetworkByName( char *theName );
};
#endif // !defined(NetworkDatabase_DB37EE6F_A0A5_4832_AAA1_98D17991F7AE__INCLUDED_)