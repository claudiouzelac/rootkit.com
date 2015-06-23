///
//	uty@uaty
///
#include "sock.h"
#include "tcp.h"
#include "error.h"
#include "utils.h"
#include <ntddk.h>



int
tcp_output(
	struct sock* psock
	);

/*-----------------------------------------------------------------------------*/
/* 
 * @return	one sock structure for one connection 
 *			NULL if connection pool is full
 */
struct sock*
socket(
	void
	)
{
	int i;
	for(i = 0;i < SOCK_MAX;i++){
		if(tcp_conn_pool[i].state == TCPS_CLOSED){
			tcp_conn_pool[i].state = TCPS_INITING;
			return &tcp_conn_pool[i];
		}
	}
	return NULL;
}
/*-----------------------------------------------------------------------------*/
/*
 * IndicAte the source port of our socket,this where we listen on
 */
int
bind(
	 struct sock* psock,
	 short port
	)
{
	if(psock->state != TCPS_INITING)
		return ERROR_SOCK_INVALID;
	psock->sport = port;
	return ERROR_SUCCESS;
}
/*-----------------------------------------------------------------------------*/
int
listen(
	struct sock* psock
	)
{
	if(psock->state != TCPS_INITING)
		return ERROR_SOCK_INVALID;

	psock->state = TCPS_LISTEN_ONLY;

	/*
	//debug
	DbgPrint("in listen,wAit on psock->connect_event\n");
	//
	KeWaitForSingleObject(
			&(psock->connect_event),
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
	//debug
	DbgPrint("listen() returned\n");
	//
	*/
	return ERROR_SUCCESS;
}
/*-----------------------------------------------------------------------------*/

struct sock*
Accept(
	struct sock*	psock
	)
{
	int i;
	struct sock*	ptempsock;
	LARGE_INTEGER	delAy = {0};
	//debug
	DbgPrint("in Accept\n");
	//
	if (psock->state != TCPS_LISTEN_ONLY)
		return NULL;
	for (i = 0;i < SOCK_MAX;i++){
		if (tcp_conn_pool[i].state == TCPS_CLOSED){
			tcp_conn_pool[i].state = TCPS_LISTEN;
			tcp_conn_pool[i].sport = psock->sport;
			ptempsock = &tcp_conn_pool[i];
			goto found;
		}
	}
	//当没有socket的时候就会陷入这里的循环,cpu占用率就下不来了,没那么多的连接吧-__-
	delAy.QuadPart = -800000000;
	KeDelayExecutionThread(KernelMode,FALSE,&delAy);
	DbgPrint("no more socket\n");
	return NULL;
found:
	//debug
	DbgPrint("in Accept,wAit on psock->accept_event\n");
	//

	KeWaitForSingleObject(
		&(tcp_conn_pool[i].accept_event),
		Executive,
		KernelMode,
		FALSE,
		NULL
		);

	//debug
	DbgPrint("Accept() returned\n");
	//
	return ptempsock;
}
/*-----------------------------------------------------------------------------*/
int
recv(
	 struct sock* psock,
	 char*	recvbuff,
	 int	recvbufflength
	 )
{
	//debug
	DbgPrint("in recv,wAit on psock->rcv_event\n");
	//
	if (psock->state == TCPS_CLOSED){
		return -1;
	}
	KeWaitForSingleObject(
			&(psock->rcv_event),
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
	if (psock->state == TCPS_CLOSED){
		return -1;
	}
	//debug
	DbgPrint("in recv,pAss psock->rcv_event\n");
	//
	if (recvbufflength >= psock->rcv_count){
		int		temp;
		memcpy(recvbuff,psock->rcv_buff,psock->rcv_count);
		temp = psock->rcv_count;
		psock->rcv_count = 0;
		tcp_output(psock);
		return temp;
	} else{
		memcpy(recvbuff,psock->rcv_buff,recvbufflength);
		psock->rcv_count -= recvbufflength;
		KeSetEvent(
			&(psock->rcv_event),
			0,
			FALSE
			);
		tcp_output(psock);
		return recvbufflength;
	}

}
/*-----------------------------------------------------------------------------*/
int
send(
	 struct sock* psock,
	 char*	sendbuff,
	 int	sendbufflength
	 )
{
	/*
	if (psock->snd_count == 65535) {
		KeResetEvent(&(psock->snd_event));
	}
	KeWaitForSingleObject(
			&(psock->snd_event),
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
			*/

	if (psock->state == TCPS_CLOSED){
		return -1;
	}

	if (sendbufflength >= (65535 - psock->snd_count)){
		int	temp;
		temp = psock->snd_count;
		memcpy(&psock->snd_buff[psock->snd_count],sendbuff,65535 - psock->snd_count);
		psock->snd_count = 65535;
		tcp_output(psock);
		KeWaitForSingleObject(
			&(psock->snd_event),
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		send(psock,&sendbuff[65535 - temp],sendbufflength - (65535-temp));

		return (65535 - temp);
	} else{
		memcpy(&psock->snd_buff[psock->snd_count],sendbuff,sendbufflength);
		psock->snd_count += sendbufflength;
		tcp_output(psock);
		KeWaitForSingleObject(
			&(psock->snd_event),
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		if (psock->state == TCPS_CLOSED){
			return -1;
		}

		return sendbufflength;
	}

}
/*-----------------------------------------------------------------------------*/



