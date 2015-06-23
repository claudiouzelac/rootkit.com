///////////////////////////////////////////////////////////
//  UserConsole.cpp
//  Implementation of the Class UserConsole
//  Created on:      24-Dec-2003 11:58:28 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UserConsole.h"
#include "LocalNetwork.h"

UserConsole::UserConsole() :
m_ServerEngine(0)
{
	AttachAllLocalInterfaces();
}



UserConsole::~UserConsole()
{

}

// override this class to do something userful
void UserConsole::OnMessage( CONSOLE_MESSAGE msg, void *param)
{
}
	
void UserConsole::AttachAllLocalInterfaces()
{
	LocalNetwork *nptr = NULL;
	
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_if_t *alldevs;
    pcap_if_t *d;
    u_int num = 0;

	/* Retrieve the device list */
    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        TRACE1("Error in pcap_findalldevs: %s\n", errbuf);
        return;
    }

	/* Print the list */
    for(d = alldevs; d ; d = d->next)
    {
		char _tt[1024];

		nptr = new LocalNetwork();
		nptr->devname = d->name;
		nptr->devdesc = d->description;
		nptr->num = num++;
		
		_snprintf(_tt, 1022, "Local %s", d->description);
		nptr->m_name.assign(_tt); //display name

		m_NetworkDatabase.AddNetwork(nptr);
		TRACE2("[+] Added NIC[%d]: \"%s\" to list\n", nptr->num, d->description);
		
		nptr->StartThreads();
	}

	// Free list
    pcap_freealldevs(alldevs);
}

Network * UserConsole::FindNetwork(char *theName)
{
	return(m_NetworkDatabase.FindNetworkByName(theName));
}
