///////////////////////////////////////////////////////////
//  ServerEngine.cpp
//  Implementation of the Class ServerEngine
//  Created on:      24-Dec-2003 11:58:26 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ServerEngine.h"
#include "NetworkDatabase.h"
#include "UserConsole.h"

ServerEngine::ServerEngine( unsigned short tcp_port, NetworkDatabase *theDB) :
m_tcp_port(tcp_port),
m_network_db(theDB)
{
	assert(theDB);
}

ServerEngine::~ServerEngine()
{

}

void ServerEngine::Stop()
{
	TerminateThread(m_thread_handle, 0);
}

// listen for incoming connections
void ServerEngine::Run()
{
	struct sockaddr_in saddr;
	SOCKET s;
	SOCKET clisock;
	struct sockaddr_in cliaddr;
	int clilen = sizeof(struct sockaddr_in);
		
	
	// Init socket
	if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		TRACE0("socket() failed");
		return;
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(m_tcp_port);
	saddr.sin_addr.s_addr = INADDR_ANY;

	// Connect
	if(bind(s, (struct sockaddr *)&saddr, sizeof(saddr)) == SOCKET_ERROR)
	{
		TRACE0("bind() failed");
		return;
	}
	
	if(listen(s, 5) == SOCKET_ERROR)
	{
		TRACE0("listen() failed");
		return;
	}

	while(1)
	{
		if((clisock = accept(s, (struct sockaddr *)&cliaddr, &clilen)) != INVALID_SOCKET)
		{
			char _connection_name[255];

			TRACE0("--[ CLIENT CONNECTION accepted ]--\n");

			// we just got a new client connection
			// so create a new network connection and init
			// the tunnel
			RemoteNetwork *aNewRemoteNetwork = new RemoteNetwork(clisock);
			
			_snprintf(_connection_name, 253, "Remote from %s", inet_ntoa(cliaddr.sin_addr)); 
			aNewRemoteNetwork->m_name.assign(_connection_name);
			
			// let the networkdb know about it
			m_network_db->AddNetwork(aNewRemoteNetwork);

			// start processing packets
			aNewRemoteNetwork->StartTunnel();
		}
	}
}
