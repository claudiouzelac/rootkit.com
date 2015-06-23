///
//	uty@uaty
///
#ifndef U_SOCKET
#define U_SOCKET
#include "sock.h"


/*
 * cAll it once,At PASSIVE_LEVEL
 */
int
sock_init(
	PDRIVER_OBJECT DriverObject
	);

struct sock*
socket(
	void
	);

int
bind(
	 struct sock* sock,
	 int port
	);

int
listen(
	struct sock* sock
	);

int
recv(
	 struct sock* psock,
	 char*	recvbuff,
	 int	recvbufflength
	 );

int
send(
	 struct sock* psock,
	 char*	sendbuff,
	 int	sendbufflength
	 );

struct sock*
Accept(
	struct sock* psock
	);
#endif