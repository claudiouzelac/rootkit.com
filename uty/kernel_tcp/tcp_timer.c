///
//	uty@uaty
///
#include "sock.h"
#include "tcp.h"
#include "tcp_timer.h"


int
tcp_output(
	struct sock* psock
	);

struct sock*
tcp_timers(
	struct sock* psock,
	int timer
	);

struct sock*
tcp_close(
	struct sock*	psock
	);

struct sock*
tcp_drop(
	struct sock*	psock,
	int		errno
	);

void
tcp_setpersist(
	struct sock	*psock
	);

void
tcp_respond(
	struct	sock*	psock,
	struct	_ETHHDR	*pEthHdr,
	int		keepAlive,
	tcp_seq	Ack,
	tcp_seq	seq,
	int		flAgs
	);

int	tcp_keepidle = TCPTV_KEEP_IDLE;
int	tcp_keepintvl = TCPTV_KEEPINTVL;
int	tcp_maxidle;

int	tcp_backoff[TCP_MAXRXTSHIFT + 1] =
    { 1, 2, 4, 8, 16, 32, 64, 64, 64, 64, 64, 64, 64 };

/*
 * Cancel all timers for TCP psock.
 */
void
tcp_cAnceltimers(
	struct sock*	psock
	)
{
	int i;
	for (i = 0; i < TCPT_NTIMERS; i++)
		psock->t_timer[i] = 0;
}
/*-----------------------------------------------------------------------------*/
/*
 * Fast timeout routine for processing delayed acks
 */
VOID
tcp_fAsttimo(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
	int	i;
	LARGE_INTEGER	timeout = {0}; 
	//这里应该设置个标志表示这个sock已经被使用 bug
	for (i = 0;i < SOCK_MAX;i++){
		if (tcp_conn_pool[i].t_flags & TF_DELACK) {
			KeAcquireSpinLock(&tcp_conn_pool[i].sock_spin,&tcp_conn_pool[i].oldirql);
			tcp_conn_pool[i].t_flags &= ~TF_DELACK;
			tcp_conn_pool[i].t_flags |= TF_ACKNOW;
			/*
			KeWaitForSingleObject(
					&(tcp_conn_pool[i].sock_mutex),
					Executive,
					KernelMode,
					FALSE,
					&timeout
					);
					*/
			tcp_output(&tcp_conn_pool[i]);
			if (tcp_conn_pool[i].state != TCPS_CLOSED){
				KeReleaseSpinLock(&tcp_conn_pool[i].sock_spin,tcp_conn_pool[i].oldirql);
			}
			//如果在tcp_v4_do_rcv()中调用了tcp_close,sock_mutex被置0
			//这里先判断是不是已经调用了tcp_close()
			/*
			if (tcp_conn_pool[i].state != TCPS_CLOSED){
				KeReleaseMutex(&(tcp_conn_pool[i].sock_mutex),FALSE);
			}
			*/
		}
	}
}
/*-----------------------------------------------------------------------------*/
/*
 * Tcp protocol timeout routine called every 500 ms.
 * Updates the timers in all active tcb's and
 * causes finite state machine actions if timers expire.
 */
VOID
tcp_slowtimo(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
	int i,j;
	tcp_maxidle = TCPTV_KEEPCNT * tcp_keepintvl;
	
	for (i = 0;i < SOCK_MAX;i++){
		KeAcquireSpinLock(&tcp_conn_pool[i].sock_spin,&tcp_conn_pool[i].oldirql);
		if (tcp_conn_pool[i].state == TCPS_CLOSED)
			continue;
		for (j = 0;j < TCPT_NTIMERS;j++){
			if (tcp_conn_pool[i].t_timer[j] && --(tcp_conn_pool[i].t_timer[j]) == 0){
				//tcp_usrreq();//未实现,直接调用tcp_time
				tcp_timers(&tcp_conn_pool[i],j);
				if (tcp_conn_pool[i].state == TCPS_CLOSED)
					goto sockgone;
			}
		}
		tcp_conn_pool[i].t_idle++;
		if (tcp_conn_pool[i].t_rtt)
			tcp_conn_pool[i].t_rtt++;
sockgone:
		;
		if (tcp_conn_pool[i].state != TCPS_CLOSED){
			KeReleaseSpinLock(&tcp_conn_pool[i].sock_spin,tcp_conn_pool[i].oldirql);
		}
	}
	tcp_iss += TCP_ISSINCR/PR_SLOWHZ;
	//tcp_now++;
}
/*-----------------------------------------------------------------------------*/
/*
 * TCP timer processing.
 */
struct sock*
tcp_timers(
	struct sock* psock,
	int timer
	)
{
	int rexmt;
	
	switch (timer){
	/*
	 * 2 MSL timeout in shutdown went off.  If we're closed but
	 * still waiting for peer to close and connection has been idle
	 * too long, or if 2MSL time is up from TIME_WAIT, delete connection
	 * control block.  Otherwise, check again in a bit.
	 */
	case TCPT_2MSL:
		if (psock->state != TCPS_TIME_WAIT &&
			psock->t_idle <= tcp_maxidle)
			psock->t_timer[TCPT_2MSL] = (short)tcp_keepintvl;
		else
			psock = tcp_close(psock);
		break;

	/*
	 * Retransmission timer went off.  Message has not
	 * been acked within retransmit interval.  Back off
	 * to a longer retransmit interval and retransmit one segment.
	 */
	case TCPT_REXMT:
		if (++psock->t_rxtshift > TCP_MAXRXTSHIFT){
			psock->t_rxtshift = TCP_MAXRXTSHIFT;
			psock = tcp_drop(psock,0);//ETIMEOUT
			break;
		}
		rexmt = TCP_REXMTVAL(psock) * tcp_backoff[psock->t_rxtshift];
		TCPT_RANGESET(psock->t_rxtcur,(short)rexmt,psock->t_rttmin,TCPTV_REXMTMAX);
		psock->t_timer[TCPT_REXMT] = psock->t_rxtcur;
		/*
		 * If losing, let the lower level know and try for
		 * a better route.  Also, if we backed off this far,
		 * our srtt estimate is probably bogus.  Clobber it
		 * so we'll take the next rtt measurement as our srtt;
		 * move the current srtt into rttvar to keep the current
		 * retransmit times until then.
		 */
		
		if (psock->t_rxtshift >TCP_MAXRXTSHIFT / 4){
			//in_losing(psock);
			psock->t_rttvar = (psock->t_srtt >> TCP_RTT_SHIFT);
			psock->t_srtt = 0;
		}
		psock->snd_nxt = psock->snd_una;
		/*
		 * If timing a segment in this window, stop the timer.
		 */
		psock->t_rtt = 0;
		/*
		 * Close the congestion window down to one segment
		 * (we'll open it by one segment for each ack we get).
		 * Since we probably have a window's worth of unacked
		 * data accumulated, this "slow start" keeps us from
		 * dumping all that data as back-to-back packets (which
		 * might overwhelm an intermediate gateway).
		 *
		 * There are two phases to the opening: Initially we
		 * open by one mss on each ack.  This makes the window
		 * size increase exponentially with time.  If the
		 * window is larger than the path can handle, this
		 * exponential growth results in dropped packet(s)
		 * almost immediately.  To get more time between 
		 * drops but still "push" the network to take advantage
		 * of improving conditions, we switch from exponential
		 * to linear window opening at some threshhold size.
		 * For a threshhold, we use half the current window
		 * size, truncated to a multiple of the mss.
		 *
		 * (the minimum cwnd that will give us exponential
		 * growth is 2 mss.  We don't allow the threshhold
		 * to go below this.)
		 */
		/*
		{
		ULONG win = min(psock->snd_wnd,psock->snd_cwnd) / 2 / psock->t_maxseg;
		if (win < 2)
			win = 2;
		psock->snd_cwnd = psock->t_maxseg;
		psock->snd_ssthresh = win * psock->t_maxseg;
		psock->t_dupacks = 0;
		}
		*/
		tcp_output(psock);
		break;
	/*
	 * Persistance timer into zero window.
	 * Force a byte to be output, if possible.
	 */
	case TCPT_PERSIST:
		tcp_setpersist(psock);
		psock->t_force = 1;
		tcp_output(psock);
		psock->t_force = 0;
		break;


	/*
	 * Keep-alive timer went off; send something
	 * or drop connection if idle for too long.
	 */
	case TCPT_KEEP:
		if (psock->state < TCPS_ESTABLISHED)
			goto dropit;
		if (psock->state <= TCPS_CLOSE_WAIT){
			if (psock->t_idle >= tcp_keepidle + tcp_maxidle)
				goto dropit;
			/*
			 * Send a packet designed to force a response
			 * if the peer is up and reachable:
			 * either an ACK if the connection is still alive,
			 * or an RST if the peer has closed the connection
			 * due to timeout or reboot.
			 * Using sequence number tp->snd_una-1
			 * causes the transmitted zero-length segment
			 * to lie outside the receive window;
			 * by the protocol spec, this requires the
			 * correspondent TCP to respond.
			 */
			tcp_respond(psock,NULL,1,psock->rcv_nxt,psock->snd_una - 1,0);
			psock->t_timer[TCPT_KEEP] = (short)tcp_keepintvl;
		} else
			psock->t_timer[TCPT_KEEP] = (short)tcp_keepidle;
		break;
dropit:
		psock = tcp_drop(psock,0);//ETIMEOUT
	}
	return psock;
}
/*-----------------------------------------------------------------------------*/

