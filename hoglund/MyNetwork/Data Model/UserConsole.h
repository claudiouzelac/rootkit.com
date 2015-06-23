///////////////////////////////////////////////////////////
//  UserConsole.h
//  Implementation of the Class UserConsole
//  Created on:      24-Dec-2003 11:58:27 AM
///////////////////////////////////////////////////////////

#if !defined(UserConsole_2BC96C72_27C1_4461_ABB7_679D29AE3F2F__INCLUDED_)
#define UserConsole_2BC96C72_27C1_4461_ABB7_679D29AE3F2F__INCLUDED_

#include "NetworkDatabase.h"
#include "ServerEngine.h"
#include "ClientConnector.h"

enum CONSOLE_MESSAGE
{
	CMSG_NONE = 0,
	CMSG_INIT,
	CMSG_ADDPEER,
	CMSG_REMOVEPEER,
	CMSG_STARTSERVER,
	CMSG_STOPSERVER,
	CMSG_REFRESH,
	CMSG_LAST
};

class UserConsole
{
public:
	UserConsole();
	virtual ~UserConsole();
	
	// message handlers
	virtual void OnMessage( CONSOLE_MESSAGE msg, void *param);
	virtual void AttachAllLocalInterfaces();
	virtual Network * FindNetwork(char *theName);

public:
	NetworkDatabase m_NetworkDatabase;
	ClientConnector m_ClientConnector;

	// create a server by building this object
	// your server will accept incoming tunnels
	ServerEngine *m_ServerEngine;
};

#endif // !defined(UserConsole_2BC96C72_27C1_4461_ABB7_679D29AE3F2F__INCLUDED_)