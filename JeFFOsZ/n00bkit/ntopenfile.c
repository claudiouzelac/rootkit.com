#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "ntopenfile.h"

// ntdll.ntopenfile
NTSTATUS WINAPI NewNtOpenFile(
  PHANDLE             FileHandle,
  ACCESS_MASK         DesiredAccess,
  POBJECT_ATTRIBUTES  ObjectAttributes,
  PIO_STATUS_BLOCK    IoStatusBlock,
  ULONG               ShareAccess,
  ULONG               OpenOptions 
)
{
	NTSTATUS rc;
	ACCESS_MASK modDesiredAccess=DesiredAccess;
			
	// give all "files" read access to helpout NtSaveKey hook :)
	// dirty but it works ;)
	if (!(DesiredAccess&GENERIC_READ||DesiredAccess&FILE_READ_DATA))
		modDesiredAccess|=GENERIC_READ;

	// call original function
	rc=OldNtOpenFile(
			FileHandle,
			modDesiredAccess,
			ObjectAttributes,
			IoStatusBlock,
			ShareAccess,
			OpenOptions
		);

	// some objects don't like GENERIC_READ added, 
	// so we call the function again with original parameters
	if (!NT_SUCCESS(rc))
	{
		// call original function
		rc=OldNtOpenFile(
				FileHandle,
				DesiredAccess,
				ObjectAttributes,
				IoStatusBlock,
				ShareAccess,
				OpenOptions
			);
	}

	return rc;
}