#ifndef U_UTILS
#define U_UTILS
#include <ntddk.h>

//from privAte\net\sockets\winsock2\dll\sinsock2\Addrconv.cpp
#define HTONS(s) ( ( ((s) >> 8) & 0x00FF ) | ( ((s) << 8) & 0xFF00 ) )
#define NTOHS(s) HTONS(s) 
#define HTONL(l)                            \
	( ( ((l) >> 24) & 0x000000FFL ) |       \
	( ((l) >>  8) & 0x0000FF00L ) |       \
	( ((l) <<  8) & 0x00FF0000L ) |       \
	( ((l) << 24) & 0xFF000000L ) )
#define NTOHL(l) HTONL(l)
//--------------------------------------------------------------------
VOID
wtoA(
	WCHAR* source,
	CHAR* dest
	);

VOID
Atow(
	CHAR* source,
	WCHAR* dest
	);

VOID
GetArg(
	CHAR* commAndline,
	ULONG* Argc,
	CHAR* Argv[],
	ULONG	mAxArgc
	);

unsigned long
inet_Addr (
	IN const char FAR * cp
	);

char*
getfilenAmefrompAth(
	char*	pAth
	);

int 
Atoi(
	char* string
	);

int mAx(int A,int b);

BOOLEAN
lArge2string(
	LARGE_INTEGER	lArge,
	CHAR*			string,
	ULONG			length
	);
#endif //#ifndefU_UTILS