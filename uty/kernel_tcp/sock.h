///
//	uty@uaty
///
#ifndef U_SOCK
#define U_SOCK
#include <ntddk.h>
#include "tcp_timer.h"
#include "structs.h"
/*
 * MAX connections
*/
#define SOCK_MAX	5



typedef	unsigned long	tcp_seq;

struct _dAtA_entry
{
	struct	_dAtA_entry *next,*prev;
	char	buffer[MAX_PACKET_SIZE];
	struct	_IPHDR*		pIpHdr;
	struct	_TCPHDR*	pTcpHdr;
	VOID*	dAtA;
	int		dAtAlen;
};

/*
 * This structure really needs to be cleaned up.
 * Most of it is for TCP, and not used by any of
 * the other protocols.
*/
struct sock {

	KSPIN_LOCK	sock_spin;
	KIRQL		oldirql;
	//接收缓存和发送缓存,以后再换成更好的形式
	char	rcv_buff[65535];
	char	snd_buff[65535];
	int		rcv_count;//need initiAlize
	int		snd_count;//need initiAlize


	short	t_timer[TCPT_NTIMERS];	/* tcp timers */
	short	t_rxtshift;		/* log(2) of rexmt exp. backoff */
	short	t_rxtcur;		/* current retransmit value */
	short	t_dupacks;		/* consecutive dup acks recd */
	USHORT	t_maxseg;		/* maximum segment size */
	char	t_force;		/* 1 if forcing out a byte */

/*
 * The following fields are used as in the protocol specification.
 * See RFC783, Dec. 1981, page 21.
 */
/* send sequence variables */
	tcp_seq			snd_una;		/* send unacknowledged */
	tcp_seq			snd_nxt;		/* send next */
	tcp_seq			snd_up;			/* send urgent pointer */
	tcp_seq			snd_wl1;		/* window update seg seq number */
	tcp_seq			snd_wl2;		/* window update seg ack number */
	tcp_seq			iss;			/* initial send sequence number */
	unsigned long	snd_wnd;		/* send window */
/* receive sequence variables */
	unsigned long	rcv_wnd;		/* receive window */
	tcp_seq			rcv_nxt;		/* receive next */
	tcp_seq			rcv_up;			/* receive urgent pointer */
	tcp_seq			irs;			/* initial receive sequence number */
/*
 * Additional variables for this implementation.
 */
/* receive variables */
	tcp_seq			rcv_adv;		/* advertised window */
/* retransmit variables */
	tcp_seq			snd_max;		/* highest sequence number sent;
							 * used to recognize retransmits
							 */
	
	unsigned short			t_flags;
#define	TF_ACKNOW	0x0001		/* ack peer immediately */
#define	TF_DELACK	0x0002		/* ack, but try to delay it */
#define	TF_NODELAY	0x0004		/* don't delay packets to coalesce */
#define	TF_NOOPT	0x0008		/* don't use tcp options */
#define	TF_SENTFIN	0x0010		/* have sent FIN */
#define	TF_REQ_SCALE	0x0020		/* have/will request window scaling */
#define	TF_RCVD_SCALE	0x0040		/* other side has requested scaling */
#define	TF_REQ_TSTMP	0x0080		/* have/will request timestamps */
#define	TF_RCVD_TSTMP	0x0100		/* a timestamp was received in SYN */
#define	TF_SACK_PERMIT	0x0200		/* other side said I could SACK */

	
/*
 * transmit timing stuff.  See below for scale of srtt and rttvar.
 * "Variance" is actually smoothed difference.
 */
	short	t_idle;			/* inactivity time */
	short	t_rtt;			/* round trip time */
	tcp_seq	t_rtseq;		/* sequence number being timed */
	short	t_srtt;			/* smoothed round-trip time */
	short	t_rttvar;		/* variance in round-trip time */
	USHORT	t_rttmin;		/* minimum rtt allowed */
	ULONG	max_sndwnd;		/* largest window peer has offered */
	


	/* RFC 1323 variables */
	UCHAR	snd_scale;		/* window scaling for send window */
	UCHAR	rcv_scale;		/* window scaling for recv window */
	UCHAR	request_r_scale;	/* pending window scaling */
	UCHAR	requested_s_scale;
	ULONG	ts_recent;		/* timestamp echo data */
	ULONG	ts_recent_age;		/* when last updated */
	tcp_seq	last_ack_sent;


	char					dstmAc[6];
	char					srcmAc[6];
	unsigned long			daddr;
	unsigned long			saddr;
	unsigned short			dport;
	unsigned short			sport;

	unsigned long			state;
	unsigned short			window;

struct _dAtA_entry	*inseq_queue;  //need initiAlize

KEVENT  rcv_event;
KEVENT	snd_event;
KEVENT	connect_event;
KEVENT	accept_event;

};

struct sock tcp_conn_pool[SOCK_MAX];

/*
 * TCP sequence numbers are 32 bit integers operated
 * on with modular arithmetic.  These macros can be
 * used to compare such integers.
 */
#define	SEQ_LT(a,b)	((int)((a)-(b)) < 0)
#define	SEQ_LEQ(a,b)	((int)((a)-(b)) <= 0)
#define	SEQ_GT(a,b)	((int)((a)-(b)) > 0)
#define	SEQ_GEQ(a,b)	((int)((a)-(b)) >= 0)

/*
 * Macros to initialize tcp sequence numbers for
 * send and receive from initial send and receive
 * sequence numbers.
 */
#define	tcp_rcvseqinit(psock) \
	(psock)->rcv_adv = (psock)->rcv_nxt = (psock)->irs + 1

#define	tcp_sendseqinit(psock) \
	(psock)->snd_una = (psock)->snd_nxt = (psock)->snd_max = (psock)->snd_up = \
	    (psock)->iss

#define	TCP_ISSINCR	(125*1024)	/* increment for tcp_iss each second */

extern tcp_seq	tcp_iss;		/* tcp initial send seq # */



/*
 * The smoothed round-trip time and estimated variance
 * are stored as fixed point numbers scaled by the values below.
 * For convenience, these scales are also used in smoothing the average
 * (smoothed = (1/scale)sample + ((scale-1)/scale)smoothed).
 * With these scales, srtt has 3 bits to the right of the binary point,
 * and thus an "ALPHA" of 0.875.  rttvar has 2 bits to the right of the
 * binary point, and is smoothed with an ALPHA of 0.75.
 */
#define	TCP_RTT_SCALE		8	/* multiplier for srtt; 3 bits frac. */
#define	TCP_RTT_SHIFT		3	/* shift for srtt; 3 bits frac. */
#define	TCP_RTTVAR_SCALE	4	/* multiplier for rttvar; 2 bits */
#define	TCP_RTTVAR_SHIFT	2	/* multiplier for rttvar; 2 bits */
/*
 * The initial retransmission should happen at rtt + 4 * rttvar.
 * Because of the way we do the smoothing, srtt and rttvar
 * will each average +1/2 tick of bias.  When we compute
 * the retransmit timer, we want 1/2 tick of rounding and
 * 1 extra tick because of +-1/2 tick uncertainty in the
 * firing of the timer.  The bias will give us exactly the
 * 1.5 tick we need.  But, because the bias is
 * statistical, we have to test that we don't drop below
 * the minimum feasible timer (which is 2 ticks).
 * This macro assumes that the value of TCP_RTTVAR_SCALE
 * is the same as the multiplier for rttvar.
 */
#define	TCP_REXMTVAL(tp) \
	(((tp)->t_srtt >> TCP_RTT_SHIFT) + (tp)->t_rttvar)

#endif //U_SOCK