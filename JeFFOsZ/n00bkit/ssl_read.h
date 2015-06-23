#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <openssl\ssl.h>
#include "ntdll.h"

typedef int (CDECL* SSL_READ)(SSL *ssl,void *buf,int num);

typedef int (CDECL* SSL_GET_FD)(SSL*);

#ifndef __SSL_READ__
#define __SSL_READ__

SSL_READ OldSSL_read;
int CDECL NewSSL_read(SSL *ssl,void *buf,int num);

#endif 