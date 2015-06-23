///////////////////////////////////////////////////////////
//  NetworkDatabase.cpp
//  Implementation of the Class NetworkDatabase
//  Created on:      24-Dec-2003 11:58:24 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NetworkDatabase.h"


NetworkDatabase::NetworkDatabase()
{

}



NetworkDatabase::~NetworkDatabase()
{

}

void NetworkDatabase::AddNetwork( Network *theNetwork )
{
	assert(theNetwork);

	//LockMutex();

	NETWORK_LIST::iterator i = m_network_list.begin();
	while(i != m_network_list.end())
	{
		Network *np = *i;
		if(np == theNetwork)
		{
			TRACE0("Warning: duplicate network entries!");
			UnlockMutex();
			return;
		}
		i++;
	}
	
	//make sure the network can query this database
	theNetwork->m_database = this;

	m_network_list.push_back(theNetwork);
	//UnlockMutex();
}

void NetworkDatabase::RemoveNetwork( Network *theNetwork )
{
	assert(theNetwork);

	LockMutex();
	m_network_list.remove(theNetwork);
	
	// remove ALL MAC references to the target network
	MAC_TO_NETWORK_DB::iterator i = m_network_map.begin();
	while(i != m_network_map.end())
	{
		Network *aNet = (*i).second;
		if(aNet == theNetwork)
		{
			i = m_network_map.erase(i);
		}
		else
		{
			i++;
		}
	}

	UnlockMutex();
	return;
}

Network * NetworkDatabase::FindNetworkByMAC( unsigned char theMAC[6] )
{
	Network *aNet = NULL;
	__int64 aMAC = 0;
	memcpy(&aMAC, theMAC, 6);
	
	//LockMutex();

	MAC_TO_NETWORK_DB::iterator i = m_network_map.find( aMAC );
	if(i != m_network_map.end())
	{
		aNet = (*i).second;
	}
	
	//UnlockMutex();
	return aNet;
}

Network * NetworkDatabase::FindNetworkByName( char *theName )
{
	Network *np = NULL;

	//LockMutex();

	NETWORK_LIST::iterator i = m_network_list.begin();
	while(i != m_network_list.end())
	{
		np = *i;
		if(!strcmp(np->m_name.c_str(), theName))
		{
			break;
		}
		i++;
	}

	//UnlockMutex();

	return np;
}