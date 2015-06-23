///////////////////////////////////////////////////////////
//  PacketEngine.h
//  Implementation of the Class PacketEngine
//  Created on:      24-Dec-2003 11:58:25 AM
///////////////////////////////////////////////////////////

#if !defined(PacketEngine_9756B402_D98D_4aa3_BEB8_223160D3A538__INCLUDED_)
#define PacketEngine_9756B402_D98D_4aa3_BEB8_223160D3A538__INCLUDED_

#include "SniffPacketThread.h"
#include "SendPacketThread.h"

class PacketEngine
{

public:
	PacketEngine();
	virtual ~PacketEngine();

public:
	SniffPacketThread *SniffPacketThread;
	SendPacketThread *SendPacketThread;

};
#endif // !defined(PacketEngine_9756B402_D98D_4aa3_BEB8_223160D3A538__INCLUDED_)