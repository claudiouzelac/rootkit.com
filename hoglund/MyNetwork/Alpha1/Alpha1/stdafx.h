// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
// C RunTime Header Files
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <assert.h>

#include <pcap.h>
#include <packet32.h>

// TODO: reference additional headers your program requires here
enum _error_code
{
	ERROR_NONE = 0,
	ERROR_SOCKET_FAILED,
	ERROR_CONNECT_FAILED,
	ERROR_LAST
};

typedef long n_long;
typedef short n_short;
typedef long n_time;

#define ETH_ALEN		6		
#define ETH_HLEN		14		
#define ETH_ZLEN		60		
#define ETH_DATA_LEN	1500		
#define ETH_FRAME_LEN	1514		

#define ETH_P_LOOP		0x0060	
#define ETH_P_ECHO		0x0200		
#define ETH_P_PUP		0x0400	
#define ETH_P_IP		0x0800	
#define ETH_P_X25		0x0805		
#define ETH_P_ARP		0x0806	
#define	ETH_P_BPQ		0x08FF		
#define ETH_P_DEC       0x6000          
#define ETH_P_DNA_DL    0x6001          
#define ETH_P_DNA_RC    0x6002          
#define ETH_P_DNA_RT    0x6003         
#define ETH_P_LAT       0x6004         
#define ETH_P_DIAG      0x6005         
#define ETH_P_CUST      0x6006         
#define ETH_P_SCA       0x6007       
#define ETH_P_RARP      0x8035	
#define ETH_P_ATALK		0x809B		
#define ETH_P_AARP		0x80F3	
#define ETH_P_IPX		0x8137		
#define ETH_P_IPV6		0x86DD	
#define ETH_P_802_3		0x0001		
#define ETH_P_AX25		0x0002		
#define ETH_P_ALL		0x0003		
#define ETH_P_802_2		0x0004		
#define ETH_P_SNAP		0x0005		
#define ETH_P_DDCMP     0x0006          
#define ETH_P_WAN_PPP   0x0007          
#define ETH_P_PPP_MP    0x0008          
#define ETH_P_LOCALTALK 0x0009		
#define ETH_P_PPPTALK	0x0010		
#define ETH_P_TR_802_2	0x0011	

struct etherproto {
	char *s;
	u_short p;
};
extern struct etherproto etherproto_db[];

struct ether_header 
{
	unsigned char	h_dest[ETH_ALEN];	
	unsigned char	h_source[ETH_ALEN];	
	unsigned short	h_proto;		
};

// ARP/RARP
///////////////////////////////////////////////////////////////////////
static u_char bcastaddr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

struct arphdr
{
	unsigned short	ar_hrd;		
	unsigned short	ar_pro;		
	unsigned char	ar_hln;		
	unsigned char	ar_pln;		
	unsigned short	ar_op;		
};

#define ARPD_UPDATE	0x01
#define ARPD_LOOKUP	0x02
#define ARPD_FLUSH	0x03

#define ARPHRD_ETHER	0x01

#define ARPOP_REQUEST	0x01
#define ARPOP_REPLY		0x02
#define ARPOP_REVREQUEST	0x03
#define ARPOP_REVREPLY		0x04

struct	ether_arp {
	struct	arphdr ea_hdr;	
	u_char	arp_sha[ETH_ALEN];	
	u_char	arp_spa[4];	
	u_char	arp_tha[ETH_ALEN];	
	u_char	arp_tpa[4];	
};
#define	arp_hrd	ea_hdr.ar_hrd
#define	arp_pro	ea_hdr.ar_pro
#define	arp_hln	ea_hdr.ar_hln
#define	arp_pln	ea_hdr.ar_pln
#define	arp_op	ea_hdr.ar_op

/* IP Header in Little Endian */
struct iphdr {
	u_char	ip_hl:4,		
			ip_v:4;			
	u_char	ip_tos;			
	short	ip_len;			
	u_short	ip_id;			
	short	ip_off;			
#define	IP_DF 0x4000		
#define	IP_MF 0x2000		
	u_char	ip_ttl;			
	u_char	ip_p;			
	u_short	ip_sum;			
	struct	in_addr ip_src,ip_dst;	
};

struct	ip_timestamp {
	u_char	ipt_code;		
	u_char	ipt_len;		
	u_char	ipt_ptr;		
	u_char	ipt_flg:4,		
		ipt_oflw:4;			
	union ipt_timestamp {
		n_long	ipt_time[1];
		struct	ipt_ta {
			struct in_addr ipt_addr;
			n_long ipt_time;
		} ipt_ta[1];
	} ipt_timestamp;
};

#define	IPOPT_TS_TSONLY		0		
#define	IPOPT_TS_TSANDADDR	1		
#define	IPOPT_TS_PRESPEC	2		

#define	IPOPT_SECUR_UNCLASS	0x0000
#define	IPOPT_SECUR_CONFID	0xf135
#define	IPOPT_SECUR_EFTO	0x789a
#define	IPOPT_SECUR_MMMM	0xbc4d
#define	IPOPT_SECUR_RESTR	0xaf13
#define	IPOPT_SECUR_SECRET	0xd788
#define	IPOPT_SECUR_TOPSECRET	0x6bc5

struct icmphdr {
	u_char	icmp_type;		
	u_char	icmp_code;		
	u_short	icmp_cksum;		
	union {
		u_char ih_pptr;			
		struct in_addr ih_gwaddr;	
		struct ih_idseq {
			n_short	icd_id;
			n_short	icd_seq;
		} ih_idseq;
		int ih_void;
	} icmp_hun;
#define	icmp_pptr	icmp_hun.ih_pptr
#define	icmp_gwaddr	icmp_hun.ih_gwaddr
#define	icmp_id		icmp_hun.ih_idseq.icd_id
#define	icmp_seq	icmp_hun.ih_idseq.icd_seq
#define	icmp_void	icmp_hun.ih_void
	union {
		struct id_ts {
			n_time its_otime;
			n_time its_rtime;
			n_time its_ttime;
		} id_ts;
		struct id_ip  {
			struct iphdr idi_ip;
		} id_ip;
		u_long	id_mask;
		char	id_data[1];
	} icmp_dun;
#define	icmp_otime	icmp_dun.id_ts.its_otime
#define	icmp_rtime	icmp_dun.id_ts.its_rtime
#define	icmp_ttime	icmp_dun.id_ts.its_ttime
#define	icmp_ip		icmp_dun.id_ip.idi_ip
#define	icmp_mask	icmp_dun.id_mask
#define	icmp_data	icmp_dun.id_data
};

// ICMP Types
#define	ICMP_ECHOREPLY		0		
#define ICMP_UNREACH					3  
#define	ICMP_SOURCEQUENCH				4
#define	ICMP_REDIRECT					5	
#define	ICMP_ECHO						8
#define ICMP_ROUTERADVERT				9
#define ICMP_ROUTERSOLICIT				10
#define	ICMP_TIMXCEED					11
#define	ICMP_PARAMPROB					12		
#define	ICMP_TSTAMP						13		
#define	ICMP_TSTAMPREPLY				14		
#define	ICMP_IREQ						15		
#define	ICMP_IREQREPLY					16		
#define	ICMP_MASKREQ					17		
#define	ICMP_MASKREPLY					18		

// ICMP Codes	
#define ICMP_UNREACH_NET                0       
#define ICMP_UNREACH_HOST               1       
#define ICMP_UNREACH_PROTOCOL           2       
#define ICMP_UNREACH_PORT               3       
#define ICMP_UNREACH_NEEDFRAG           4       
#define ICMP_UNREACH_SRCFAIL            5       
#define ICMP_UNREACH_NET_UNKNOWN        6       
#define ICMP_UNREACH_HOST_UNKNOWN       7       
#define ICMP_UNREACH_ISOLATED           8       
#define ICMP_UNREACH_NET_PROHIB         9       
#define ICMP_UNREACH_HOST_PROHIB        10      
#define ICMP_UNREACH_TOSNET             11      
#define ICMP_UNREACH_TOSHOST            12      
#define ICMP_UNREACH_FILTER_PROHIB      13      
#define ICMP_UNREACH_HOST_PRECEDENCE    14      
#define ICMP_UNREACH_PRECEDENCE_CUTOFF  15      
	 	
#define	ICMP_REDIRECT_NET				0		
#define	ICMP_REDIRECT_HOST				1		
#define	ICMP_REDIRECT_TOSNET			2		
#define	ICMP_REDIRECT_TOSHOST			3		
	
#define	ICMP_TIMXCEED_INTRANS			0		
#define	ICMP_TIMXCEED_REASS				1		

#define ICMP_PARAMPROB_BADIPHDR			0
#define ICMP_PARAMPROB_MISSINGOPT		1

#define	ICMP_MAXTYPE					18

// TCP
/////////////////////////////////////////////////////////////////////////
typedef	u_long	tcp_seq;

// TCP header in Little Endian
struct tcphdr {
	u_short	th_sport;		
	u_short	th_dport;		
	tcp_seq	th_seq;			
	tcp_seq	th_ack;			
	u_char	th_x2:4,		
		    th_off:4;		
	u_char	th_flags;
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20
	u_short	th_win;			
	u_short	th_sum;			
	u_short	th_urp;			
};

#define	TCPOPT_EOL		0
#define	TCPOPT_NOP		1
#define	TCPOPT_MAXSEG		2
#define TCPOPT_WSCALE   	3
#define TCPOPT_SACKOK   	4
#define TCPOPT_TIMESTAMP        8

#define EXTRACT_16BITS(p) \
        ((u_short)*((u_char *)(p) + 0) << 8 | \
        (u_short)*((u_char *)(p) + 1))

enum {
  TCP_ESTABLISHED = 1,
  TCP_SYN_SENT,
  TCP_SYN_RECV,
  TCP_FIN_WAIT1,
  TCP_FIN_WAIT2,
  TCP_TIME_WAIT,
  TCP_CLOSE,
  TCP_CLOSE_WAIT,
  TCP_LAST_ACK,
  TCP_LISTEN,
  TCP_CLOSING	
};

// UDP Header
//////////////////////////////////////////////////////////////////////////////
struct udphdr {
  unsigned short	source;
  unsigned short	dest;
  unsigned short	len;
  unsigned short	check;
};

#define EXTRACT_SHORT(p)	((u_short)ntohs(*(u_short *)p))
#define EXTRACT_LONG(p)		(ntohl(*(u_int32 *)p))

struct tok {
	int v;			
	char *s;		
};

struct mtu_discovery {
	short unused;
	short nexthopmtu;
};

struct dnshdr
{
	u_short id;
	u_short flags;
	u_short qcount;
	u_short acount;
	u_short authacount;
	u_short extraacount;
};

// DNS flags 
#define DNS_NOTIMPL     0x0004
#define DNS_SERVFAILED  0x0002
#define DNS_FORMATERR   0x0001

#define TFTP_PORT				69		
#define NAMESERVER_PORT			53
#define KERBEROS_PORT			88		
#define SUNRPC_PORT				111		
#define SNMP_PORT				161		
#define NTP_PORT				123		
#define SNMPTRAP_PORT			162		
#define RIP_PORT				520		
#define KERBEROS_SEC_PORT		750		
#define NMB_PORT				137
#define DGRAM_PORT				138
#define SMB_PORT				139

#define TRACE0(x1) { char _t[1024];_snprintf(_t, 1022, x1); OutputDebugString(_t); }
#define TRACE1(x1, x2) { char _t[1024];_snprintf(_t, 1022, x1, x2); OutputDebugString(_t); }
#define TRACE2(x1, x2, x3) { char _t[1024];_snprintf(_t, 1022, x1, x2, x3); OutputDebugString(_t); }
#define TRACE3(x1, x2, x3, x4) { char _t[1024];_snprintf(_t, 1022, x1, x2, x3, x4); OutputDebugString(_t); }

