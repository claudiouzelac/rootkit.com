// adv_loader.cpp : Defines the entry point for the console application.
// code adapted from www.sysinternals.com on-demand driver loading code
// --------------------------------------------------------------------
// brought to you by ROOTKIT.COM
// --------------------------------------------------------------------

#include "stdafx.h"
#include <windows.h>
#include <process.h>

void usage(char *p){ printf("Usage:\n%s l\t load driver from c:\\_tech_.sys\n%s u\tunload driver\n", p,p); } 
int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		usage(argv[0]);
		exit(0);
	}
	
	if(*argv[1] == 'l')
	{
		printf("Registering TechTV Rootkit Driver.\n");
		
		SC_HANDLE sh = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if(!sh) 
		{
			puts("error OpenSCManager");
			exit(1);
		}

		SC_HANDLE rh = CreateService(
			sh, 
			"_tech_", 
			"_tech_", 
			SERVICE_ALL_ACCESS, 
			SERVICE_KERNEL_DRIVER, 
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL, 
			"C:\\_tech_.sys", 
			NULL, 
			NULL, 
			NULL, 
			NULL, 
			NULL);

		if(!rh) 
		{
			if (GetLastError() == ERROR_SERVICE_EXISTS) 
			{
				// serive exists
				rh = OpenService(	sh, 
									"_tech_", 
									SERVICE_ALL_ACCESS);

				if(!rh)
				{
					puts("error OpenService");
					CloseServiceHandle(sh);
					exit(1);
				}
			} 
			else 
			{
				puts("error CreateService");
				CloseServiceHandle(sh);
				exit(1);
			}
		}
	}
	else if(*argv[1]=='u')
	{
		SERVICE_STATUS ss;

		printf("Unloading TechTV Rootkit Driver.\n");
		
		SC_HANDLE sh = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if(!sh) 
		{
			puts("error OpenSCManager");
			exit(1);
		}
		SC_HANDLE rh = OpenService(	
							sh, 
							"_tech_", 
							SERVICE_ALL_ACCESS);

		if(!rh) 
		{
			puts("error OpenService");
			CloseServiceHandle(sh);
			exit(1);
		}

		if(!ControlService(rh, SERVICE_CONTROL_STOP, &ss))
		{
			puts("warning: could not stop service");
		}

		if (!DeleteService(rh)) 
		{
			puts("warning: could not delete service");
		}

		CloseServiceHandle(rh);
		CloseServiceHandle(sh);
	}
	else usage(argv[0]);

	return 0;
}

