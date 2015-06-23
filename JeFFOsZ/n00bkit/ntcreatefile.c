#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "ntcreatefile.h"

// ntdll.NtCreateFile
NTSTATUS WINAPI NewNtCreateFile(
  PHANDLE              FileHandle,
  ACCESS_MASK          DesiredAccess,
  POBJECT_ATTRIBUTES   ObjectAttributes,
  PIO_STATUS_BLOCK     IoStatusBlock,
  PLARGE_INTEGER       AllocationSize,
  ULONG                FileAttributes,
  ULONG                ShareAccess,
  ULONG                CreateDisposition,
  ULONG                CreateOptions,
  PVOID                EaBuffer,
  ULONG                EaLength 
)
{
	NTSTATUS rc;
	ACCESS_MASK modDesiredAccess=DesiredAccess;

	// give all "files" read access to helpout NtSaveKey hook :)
	if (!(DesiredAccess&GENERIC_READ||DesiredAccess&FILE_READ_DATA))
		modDesiredAccess|=GENERIC_READ;
	
	// call original function
	rc=OldNtCreateFile(
			FileHandle,
			modDesiredAccess,
			ObjectAttributes,
			IoStatusBlock,
			AllocationSize,
			FileAttributes,
			ShareAccess,
			CreateDisposition,
			CreateOptions,
			EaBuffer,
			EaLength
		);
	
	// some objects don't like GENERIC_READ added, 
	// so we call the function again with original parameters
	if (!NT_SUCCESS(rc))
	{
		// call original function
		rc=OldNtCreateFile(
				FileHandle,
				DesiredAccess,
				ObjectAttributes,
				IoStatusBlock,
				AllocationSize,
				FileAttributes,
				ShareAccess,
				CreateDisposition,
				CreateOptions,
				EaBuffer,
				EaLength
			);
	}

	return rc;
}