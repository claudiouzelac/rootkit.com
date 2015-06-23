///////////////////////////////////////////////////////////
//  ClientConnector.cpp
//  Implementation of the Class ClientConnector
//  Created on:      24-Dec-2003 11:58:23 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClientConnector.h"


ClientConnector::ClientConnector() :
m_lasterror(0)
{

}



ClientConnector::~ClientConnector(){

}

RemoteNetwork * ClientConnector::ConnectToRemoteNetwork( char *ip_address, char *tcp_port)
{
	SOCKET s;
	struct sockaddr_in saddr;
	
	// Init socket
	if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		TRACE0("socket() failed");
		m_lasterror = ERROR_SOCKET_FAILED;
		return NULL;
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(atoi( tcp_port ));
	saddr.sin_addr.s_addr = inet_addr( ip_address );

	// Connect
	if(connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) == SOCKET_ERROR)
	{
		TRACE0("connect failed");
		m_lasterror = ERROR_CONNECT_FAILED;
		return NULL;
	}

	// build a remote network object
	RemoteNetwork *aRemoteNetwork = new RemoteNetwork(s);
	return aRemoteNetwork;
}

DWORD ClientConnector::GetLastError()
{
	return m_lasterror;
}

