#include "stdafx.h"
#include "driver.h"

BOOL InstallDriver()
{
	SC_HANDLE	hSc, hService;
	char		szPath[MAX_PATH];
	UINT		i;

	GetModuleFileName( NULL, szPath, MAX_PATH );
	for( i = strlen( szPath ); i >= 0; i-- )
	{
		if( szPath[i] == '\\' )
		{
			szPath[i + 1] = 0;
			break;
		}
	}

	strcat( szPath, "inp.sys" );

	hSc = OpenSCManager( 
		NULL, 
		NULL, 
		SC_MANAGER_CREATE_SERVICE );

	if( !hSc )
		return FALSE;

	hService = CreateService( 
		hSc, 
		"INP", 
		"INP", 
		SERVICE_ALL_ACCESS, 
		SERVICE_KERNEL_DRIVER, 
		SERVICE_DEMAND_START, 
		SERVICE_ERROR_NORMAL, 
		szPath, 
		NULL, 
		NULL, 
		NULL, 
		NULL, 
		NULL );

	if( !hService )
	{
		CloseServiceHandle( hSc );
		return ( GetLastError() == ERROR_SERVICE_EXISTS ) ? TRUE : FALSE;
	}
		
	CloseServiceHandle( hSc );
	return TRUE;
}

BOOL StartDriver()
{
	SC_HANDLE	hSc, hService;

	hSc = OpenSCManager( 
		NULL, 
		NULL, 
		SC_MANAGER_CREATE_SERVICE );

	if( hSc == NULL )
		return FALSE;

	hService = OpenService( 
		hSc, 
		"INP", 
		SERVICE_START );

	if( !hService )
	{
		CloseServiceHandle( hSc );
		return FALSE;
	}

	StartService( hService, NULL, NULL );
	return TRUE;
}

BOOL UninstallDriver()
{
	SC_HANDLE		hSc, hService;
	SERVICE_STATUS	ss;

	hSc = OpenSCManager( 
		NULL, 
		NULL, 
		SC_MANAGER_CREATE_SERVICE );

	if( hSc == NULL )
		return FALSE;

	hService = OpenService( 
		hSc, 
		"INP", 
		SERVICE_ALL_ACCESS );

	if( !hService )
	{
		CloseServiceHandle( hSc );
		return FALSE;
	}

	ControlService( hService, SERVICE_CONTROL_STOP, &ss );
	DeleteService( hService );
	return TRUE;
}
