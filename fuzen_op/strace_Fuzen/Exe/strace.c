#include <windows.h>
#include <stdio.h>
#include <winioctl.h>

#include "..\Sys\ioctlcmd.h"
#include "Instdrv.h"


static CHAR ac_driverLabel[] = "strace";
static CHAR ac_driverName[] = "strace.sys";


HANDLE h_Device = INVALID_HANDLE_VALUE;
long   g_PID = 0;
ULONG  g_SysCallLimit = 255;
BOOL   initialized = FALSE;

BOOL LoadDeviceDriver( const char *, const char *, HANDLE *, PDWORD);
BOOL UnloadDeviceDriver( const char *);

DWORD Init()
{
	char ac_driverPath[MAX_PATH];
	DWORD d_error;

	if (GetCurrentDirectory(MAX_PATH, ac_driverPath))
	{
		strncat(ac_driverPath, "\\", MAX_PATH-strlen(ac_driverPath));
		strncat(ac_driverPath, ac_driverName, MAX_PATH-strlen(ac_driverPath));
	}
	LoadDeviceDriver(ac_driverLabel, ac_driverPath ,&h_Device, &d_error);
	if (h_Device == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Unable to Load Driver");
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
		if (kernel_args)
			free(kernel_args);

		return 1;
	}

	return 0;
}


void use()
{
	fprintf(stderr, "\nUSAGE:\n\t\tstrace.exe <PID> <Limit>\n\n");
}


int main (int argc, char **argv)
{

	if (argc != 3)
	{
		use();
		return 1;
	}

	Init();
	setPID(atol(argv[1]), (ULONG)atol(argv[2]));

	UnloadDeviceDriver(ac_driverLabel);

	return 0;
}

