///////////////////////////////////////////////////////////
//  ClientConnector.h
//  Implementation of the Class ClientConnector
//  Created on:      24-Dec-2003 11:58:22 AM
///////////////////////////////////////////////////////////

#if !defined(ClientConnector_99886A0C_6BA5_44fa_AA9E_C26B09E2A2AF__INCLUDED_)
#define ClientConnector_99886A0C_6BA5_44fa_AA9E_C26B09E2A2AF__INCLUDED_

#include "RemoteNetwork.h"

/**
 * Queries and connects to remote networks as a client
 */
class ClientConnector
{
protected:
	DWORD m_lasterror;
public:
	ClientConnector();
	virtual ~ClientConnector();

public:
	// return NULL if error
	// get last error
	RemoteNetwork * ConnectToRemoteNetwork( char *ip_address, char *tcp_port);
	DWORD GetLastError();

};
#endif // !defined(ClientConnector_99886A0C_6BA5_44fa_AA9E_C26B09E2A2AF__INCLUDED_)