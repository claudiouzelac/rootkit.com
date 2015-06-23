///////////////////////////////////////////////////////////
//  NetworkDevice.h
//  Implementation of the Class NetworkDevice
//  Created on:      24-Dec-2003 11:58:24 AM
///////////////////////////////////////////////////////////

#if !defined(NetworkDevice_1448EEE4_6594_4fcc_BD73_1DF8E3AC03ED__INCLUDED_)
#define NetworkDevice_1448EEE4_6594_4fcc_BD73_1DF8E3AC03ED__INCLUDED_

#include "PacketEngine.h"
#include "LocalNetwork.h"

/**
 * The ethernet device 
 */
class NetworkDevice
{

public:
	NetworkDevice();
	virtual ~NetworkDevice();

public:
	PacketEngine *PacketEngine;
	LocalNetwork *LocalNetwork;

};
#endif // !defined(NetworkDevice_1448EEE4_6594_4fcc_BD73_1DF8E3AC03ED__INCLUDED_)