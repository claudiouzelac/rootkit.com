///////////////////////////////////////////////////////////
//  RoutedPacket.h
//  Implementation of the Class RoutedPacket
//  Created on:      24-Dec-2003 11:58:25 AM
///////////////////////////////////////////////////////////

#if !defined(RoutedPacket_3CB61F6D_5E49_4364_B937_F5723407A770__INCLUDED_)
#define RoutedPacket_3CB61F6D_5E49_4364_B937_F5723407A770__INCLUDED_

#include <queue>

typedef struct blackpkt
{
	unsigned short type;
	unsigned short len;
	unsigned char pkt[9999];
} blackpkt_t;
typedef std::queue<blackpkt_t *> pktqueue_t;

// always have this set to the len of the header above the data portion
#define BPKT_HEADER_LEN 4

// Pkt modes
enum
{
	BLACK_CLIENT_PACKET,
	BLACK_SERVER_PACKET
};

#endif // !defined(RoutedPacket_3CB61F6D_5E49_4364_B937_F5723407A770__INCLUDED_)