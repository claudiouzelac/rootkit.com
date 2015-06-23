///////////////////////////////////////////////////////////
//  SniffPacketThread.h
//  Implementation of the Class SniffPacketThread
//  Created on:      24-Dec-2003 11:58:26 AM
///////////////////////////////////////////////////////////

#if !defined(SniffPacketThread_68315815_B1A2_4a38_B6A9_278D30BC223D__INCLUDED_)
#define SniffPacketThread_68315815_B1A2_4a38_B6A9_278D30BC223D__INCLUDED_

#include "EngineThread.h"
#include <ntddndis.h>
class LocalNetwork;

class SniffPacketThread : public EngineThread 
{
protected:
	char m_pbuf[512000];
	LPPACKET m_lpp;
	bool m_enabled;
	HANDLE m_handle;
	
public:
	LocalNetwork *m_parent;
	LPADAPTER m_lpa; //shared w/ sender thread
	
public:
	SniffPacketThread();
	virtual ~SniffPacketThread();

	virtual bool OpenAdapter(char *theAdapterName);
	virtual void Run();
};
#endif // !defined(SniffPacketThread_68315815_B1A2_4a38_B6A9_278D30BC223D__INCLUDED_)