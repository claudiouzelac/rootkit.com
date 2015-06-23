#include <windows.h>
//#include <winbase.h> 
#include <stdio.h>
#include <winioctl.h>

#include "..\Sys\ioctlcmd.h"
#include "Instdrv.h"


static CHAR ac_driverLabel[] = "strace";
static CHAR ac_driverName[] = "strace.sys";


HANDLE h_Device;
long   g_PID = 0;
ULONG  g_SysCallLimit = 255;
BOOL   initialized = FALSE;

// MakePtr is a macro that allows you to easily add to values (including
// pointers) together without dealing with C's pointer arithmetic.  It
// essentially treats the last two parameters as DWORDs.  The first
// parameter is used to typecast the result to the appropriate pointer type.
#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD)(ptr) + (DWORD)(addValue))

#define FUNC_NAME_LEN 30

DWORD Init()
{
	char ac_driverPath[MAX_PATH];
	DWORD d_error;

	h_Device = INVALID_HANDLE_VALUE;
	if (GetCurrentDirectory(MAX_PATH, ac_driverPath))
	{
		strncat(ac_driverPath, "\\", MAX_PATH-strlen(ac_driverPath));
		strncat(ac_driverPath, ac_driverName, MAX_PATH-strlen(ac_driverPath));
	}
	if (!LoadDeviceDriver(ac_driverLabel, ac_driverPath ,&h_Device, &d_error, FALSE))
		LoadDeviceDriver(ac_driverLabel, ac_driverPath ,&h_Device, &d_error, TRUE);

	if (h_Device == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Unable to Load Driver %s\n",ac_driverPath);
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL, 
					  d_error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
					  (LPTSTR) &lpMsgBuf, 0, NULL);

		fprintf(stderr,"%s",lpMsgBuf);

		LocalFree(lpMsgBuf);
		return (1);
	}

	return 0;
}

DWORD setPID(long myPID, ULONG limit) 
{
	DWORD *kernel_args = NULL;
	DWORD d_bytesRead = 0;

	kernel_args = (DWORD *)calloc(1, sizeof(DWORD)*2);
	if (!kernel_args)
	{
		fprintf(stderr, "Memory allocation failed.\n");
		return 1;
	}

	memcpy(kernel_args, &myPID, sizeof(DWORD));
	memcpy(kernel_args + 1, &limit, sizeof(DWORD));
	
	if(!DeviceIoControl(h_Device, IOCTL_STRACE_INIT,
						kernel_args,
						sizeof(DWORD)*2,
						NULL,
						0,
						&d_bytesRead,
						NULL))
	{
		fprintf(stderr, "Error sending PID to monitor and upper limit to Driver.\n");
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL, 
					  GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
					  (LPTSTR) &lpMsgBuf, 0, NULL);

		fprintf(stderr,"%s",lpMsgBuf);

		LocalFree(lpMsgBuf);

		
		if (kernel_args)
			free(kernel_args);
		kernel_args = NULL;

		return 1;
	}
	if (kernel_args)
		free(kernel_args);
	kernel_args = NULL;

	return 0;
}



DWORD getNumSysCalls(DWORD *nSysCalls) 
{
	DWORD *d_numSysCall = NULL;
	DWORD d_bytesRead = 0;

	d_numSysCall = (DWORD *)calloc(1, sizeof(DWORD));
	if (!d_numSysCall)
	{
		fprintf(stderr, "Memory allocation failed.\n");
		return 1;
	}

	if(!DeviceIoControl(h_Device, IOCTL_GET_SYSCALL_NUM,
						NULL,
						0,
						d_numSysCall,
						sizeof(DWORD),
						&d_bytesRead,
						NULL))
	{
		fprintf(stderr, "Error getting the number of System Calls.\n");
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL, 
					  GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
					  (LPTSTR) &lpMsgBuf, 0, NULL);

		fprintf(stderr,"%s",lpMsgBuf);

		LocalFree(lpMsgBuf);

		
		if (d_numSysCall)
			free(d_numSysCall);
		d_numSysCall = NULL;

		return 1;
	}

	if (d_numSysCall)
	{
		*nSysCalls = *d_numSysCall;
		free(d_numSysCall);
		d_numSysCall = NULL;
	}

	return 0;
}


int DumpDllExports(int i_numSysCalls)
{
	HMODULE hm_ntdll = NULL;
	PIMAGE_DOS_HEADER dosHeader;
    PIMAGE_NT_HEADERS pNTHeader;
	PIMAGE_EXPORT_DIRECTORY exportDesc;
    DWORD exportsStartRVA = 0;
	DWORD delta = 0;
	DWORD d_bytesRead;

	//char name_array[][FUNC_NAME_LEN];

	static TCHAR ac_dllPath[(MAX_PATH+1) * sizeof (TCHAR)];
	int len = 0;

	len = GetSystemDirectory(ac_dllPath, (MAX_PATH+1)*sizeof(TCHAR));
	strncat(ac_dllPath, "\\ntdll.dll",(MAX_PATH)*sizeof(TCHAR) - len);

	hm_ntdll = LoadLibrary(ac_dllPath);
	if (hm_ntdll == NULL)
	{
		fprintf(stderr,"Error loading NTDLL.DLL.\n");
		return 1;
	}
	dosHeader = (PIMAGE_DOS_HEADER) hm_ntdll;

    pNTHeader = MakePtr( PIMAGE_NT_HEADERS, dosHeader,
                                dosHeader->e_lfanew );
    
	// First, verify that the e_lfanew field gave us a reasonable
    // pointer, then verify the PE signature.
    __try
    {
        if ( pNTHeader->Signature != IMAGE_NT_SIGNATURE )
        {
            return 0;
        }
    }
    __except( TRUE )    // Should only get here if pNTHeader (above) is bogus
    {
		fprintf(stderr,"Bogus NT file header.\n");
        return 1;
    }

    exportsStartRVA = pNTHeader->OptionalHeader.DataDirectory
                            [IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if ( !exportsStartRVA )
	{
		fprintf(stderr,"Error finding the export section of NTDLL.DLL.\n");
        return 1;
	}
   
    exportDesc = (PIMAGE_EXPORT_DIRECTORY) ((exportsStartRVA - delta) + (DWORD) dosHeader);
	DWORD pdw_FunctionNames = (DWORD) dosHeader + (DWORD) exportDesc->AddressOfNames;
	DWORD dw_nameCount = exportDesc->NumberOfNames;
	
	char *name_array = (char *)calloc(i_numSysCalls, FUNC_NAME_LEN);
	if (name_array == NULL)
	{
		fprintf(stderr, "Memory allocation failed.\n");
		return 1;
	}


	for (DWORD count = 0, index = 0; count < dw_nameCount; count++)
    {
		DWORD dw_nameOffset = *((DWORD *)pdw_FunctionNames);
		char *pc_fname = (char *)(DWORD) dosHeader + dw_nameOffset;
	
		if ((pc_fname[0] == 'Z') && (pc_fname[1] == 'w'))
		{
			DWORD dw_addr = (DWORD) GetProcAddress(hm_ntdll, pc_fname);
			DWORD index = *(DWORD*)(dw_addr+1);
			if ((int)index < i_numSysCalls)
			{
				strncpy((char *)(name_array+(index*FUNC_NAME_LEN)),"Nt", FUNC_NAME_LEN-1);
				strncat((char *)(name_array+(index*FUNC_NAME_LEN)),pc_fname+2, FUNC_NAME_LEN-1-strlen((char *)(name_array+(index*FUNC_NAME_LEN))));
			}
			//printf("%s\n",pc_fname);
		}

		pdw_FunctionNames += sizeof(DWORD);
    }

	if(!DeviceIoControl(h_Device, IOCTL_REPORT_SYSCALL_NAMES,
						name_array,
						i_numSysCalls*FUNC_NAME_LEN,
						NULL,
						0,
						&d_bytesRead,
						NULL))
	{
		fprintf(stderr, "Error sending function names to Driver.\n");
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL, 
					  GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
					  (LPTSTR) &lpMsgBuf, 0, NULL);

		fprintf(stderr,"%s",lpMsgBuf);

		LocalFree(lpMsgBuf);

		
		if (name_array)
			free(name_array);
		name_array = NULL;
		return 1;
	}
	if (name_array)
		free(name_array);
	name_array = NULL;
	return 1;
}


void use()
{
	fprintf(stderr, "\nUSAGE:\n\t\tstrace.exe <PID> <Limit>\n\n");
}


int main (int argc, char **argv)
{
	DWORD retval = 0;
	DWORD nSysCalls = 0;

	if (argc != 3)
	{
		use();
		return 1;
	}

	retval = Init();
	if (retval == 1)
		return 1;

	retval = setPID(atol(argv[1]), (ULONG)atol(argv[2]));
	if (retval == 1)
		return 1;

	retval = getNumSysCalls(&nSysCalls);
	if (retval == 1)
		return 1;

	retval = DumpDllExports(nSysCalls);
	if (retval == 1)
		return 1;

	//UnloadDeviceDriver(ac_driverLabel);

	return 0;
}

