///
//	uty@uaty
///
#ifndef TCPIPHDR_H
#define TCPIPHDR_H

#include <ntddk.h>

#define ETH_P_IP                                    0x0800          /* Internet Protocol packet     */
#define ETH_P_ARP                                   0x0806          /* Address Resolution packet    */
#define ETH_P_RARP                                  0x8035          /* Reverse Addr Res packet      */
#define ETH_P_8021P                                 0x8100          /* 802.1p                       */
#define ETH_P_DEFAULT                               ETH_P_ARP

/*
 * Protocols
 */
#define IPPROTO_IP              0               /* dummy for IP */
#define IPPROTO_ICMP            1               /* control message protocol */
#define IPPROTO_IGMP            2               /* internet group management protocol */
#define IPPROTO_GGP             3               /* gateway^2 (deprecated) */
#define IPPROTO_TCP             6               /* tcp */
#define IPPROTO_PUP             12              /* pup */
#define IPPROTO_UDP             17              /* user datagram protocol */
#define IPPROTO_IDP             22              /* xns idp */
#define IPPROTO_ND              77              /* UNOFFICIAL net disk proto */

#define IPPROTO_RAW             255             /* raw IP packet */
#define IPPROTO_MAX             256




#define ETH_ALEN	6
#pragma pack(push,1)
typedef
struct _ETHHDR
{
	unsigned char h_dest[ETH_ALEN];/*48位的目标地址的网卡物理地址*/
	unsigned char h_source[ETH_ALEN];/*48位的源地址的物理网卡地址*/
	unsigned short h_proto;/*16位的以太网协议*/
}ETHHDR,*PETHHDR;
#pragma pack(pop)

typedef
struct _IPHDR {
	UCHAR	ihl:4,
		version:4;
	UCHAR	tos;
	USHORT	tot_len;
	USHORT	id;
	USHORT	frag_off;
	UCHAR	ttl;
	UCHAR	protocol;
	USHORT	check;
	ULONG	saddr;
	ULONG	daddr;
	/*The options start here. */
}IPHDR,*PIPHDR;

typedef
struct _TCPHDR {
	USHORT	source;
	USHORT	dest;
	ULONG	seq;
	ULONG	ack_seq;

	USHORT	res1:4,
		doff:4,
		fin:1,
		syn:1,
		rst:1,
		psh:1,
		ack:1,
		urg:1,
		ece:1,
		cwr:1;
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20	

	USHORT	window;
	USHORT	check;
	USHORT	urg_ptr;
}TCPHDR,*PTCPHDR;



typedef struct _PSDHDR               //定义TCP伪首部
{
    unsigned long    saddr;            //源地址
    unsigned long    daddr;            //目的地址
    char            mbz;
    char            ptcl;            //协议类型 
    unsigned short    tcpl;            //TCP长度
}PSDHDR,*PPSDHDR;

#endif //#ifndef TCPIPHDR_H