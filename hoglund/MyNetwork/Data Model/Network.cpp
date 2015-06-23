///////////////////////////////////////////////////////////
//  Network.cpp
//  Implementation of the Class Network
//  Created on:      24-Dec-2003 11:58:24 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Network.h"
#include "UserConsole.h"

Network::Network() :
m_database(0),
m_number_tx(0),
m_number_rx(0)
{

}



Network::~Network(){

}

void Network::SendPacket( unsigned char *pkt, unsigned int len)
{
	// override and do something useful
}

void Network::Shutdown()
{
}
