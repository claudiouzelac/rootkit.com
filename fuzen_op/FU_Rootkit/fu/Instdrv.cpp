///////////////////////////////////////////////////////////////////////////////////////
// Filename Instdrv.cpp
// 
// Author: Sysinternals who adapted it from Microsoft's DDK then stolen by Fuzen.
//         No really, buy Mark Russinovich's book because he rocks.
//
// Date:    5/27/2003
// Version: 1.0
 
#include <windows.h>
#include <winuser.h>
#include <stdlib.h>
#include <string.h>

BOOL LoadDeviceDriver( const char * Name, const char * Path, 
					  HANDLE * lphDevice, PDWORD Error );
BOOL UnloadDeviceDriver( const char * Name );
BOOL InstallDriver( IN SC_HANDLE, IN LPCTSTR, IN LPCTSTR);
BOOL StartDriver( IN SC_HANDLE, IN LPCTSTR);
BOOL OpenDevice( IN LPCTSTR, HANDLE *);
BOOL StopDriver( IN SC_HANDLE, IN LPCTSTR);
BOOL RemoveDriver( IN SC_HANDLE, IN LPCTSTR);

/****************************************************************************
*
*    FUNCTION: InstallDriver( IN SC_HANDLE, IN LPCTSTR, IN LPCTSTR)
*
*    PURPOSE: Creates a driver service.
*
****************************************************************************/
BOOL InstallDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName, IN LPCTSTR ServiceExe )
{
    SC_HANDLE  schService;

    //
    // NOTE: This creates an entry for a standalone driver. If this
    //       is modified for use with a driver that requires a Tag,
    //       Group, and/or Dependencies, it may be necessary to
    //       query the registry for existing driver information
    //       (in order to determine a unique Tag, etc.).
    //

    schService = CreateService( SchSCManager,          // SCManager database
                                DriverName,           // name of service
                                DriverName,           // name to display
                                SERVICE_ALL_ACCESS,    // desired access
                                SERVICE_KERNEL_DRIVER, // service type
                                SERVICE_DEMAND_START,  // start type
                                SERVICE_ERROR_NORMAL,  // error control type
                                ServiceExe,            // service's binary
                                NULL,                  // no load ordering group
                                NULL,                  // no tag identifier
                                NULL,                  // no dependencies
                                NULL,                  // LocalSystem account
                                NULL                   // no password
                                );
    if ( schService == NULL )
        return FALSE;

    CloseServiceHandle( schService );

    return TRUE;
}


/****************************************************************************
*
*    FUNCTION: StartDriver( IN SC_HANDLE, IN LPCTSTR)
*
*    PURPOSE: Starts the driver service.
*
****************************************************************************/
BOOL StartDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName )
{
    SC_HANDLE  schService;
    BOOL       ret;

    schService = OpenService( SchSCManager,
                              DriverName,
                              SERVICE_ALL_ACCESS
                              );
    if ( schService == NULL )
        return FALSE;

    ret = StartService( schService, 0, NULL )
       || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING 
	   || GetLastError() == ERROR_SERVICE_DISABLED;

    CloseServiceHandle( schService );
    return ret;
}



/****************************************************************************
*
*    FUNCTION: OpenDevice( IN LPCTSTR, HANDLE *)
*
*    PURPOSE: Opens the device and returns a handle if desired.
*
****************************************************************************/
BOOL OpenDevice( IN LPCTSTR DriverName, HANDLE * lphDevice )
{
    TCHAR    completeDeviceName[64];
    HANDLE   hDevice;

    //
    // Create a \\.\XXX device name that CreateFile can use
    //
    // NOTE: We're making an assumption here that the driver
    //       has created a symbolic link using it's own name
    //       (i.e. if the driver has the name "XXX" we assume
    //       that it used IoCreateSymbolicLink to create a
    //       symbolic link "\DosDevices\XXX". Usually, there
    //       is this understanding between related apps/drivers.
    //
    //       An application might also peruse the DEVICEMAP
    //       section of the registry, or use the QueryDosDevice
    //       API to enumerate the existing symbolic links in the
    //       system.
    //

	if( (GetVersion() & 0xFF) >= 5 ) {

		//
		// We reference the global name so that the application can
		// be executed in Terminal Services sessions on Win2K
		//
		wsprintf( completeDeviceName, TEXT("\\\\.\\Global\\%s"), DriverName );

	} else {

		wsprintf( completeDeviceName, TEXT("\\\\.\\%s"), DriverName );
	}

    hDevice = CreateFile( completeDeviceName,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                          );
    if ( hDevice == ((HANDLE)-1) )
        return FALSE;

	// If user wants handle, give it to them.  Otherwise, just close it.
	if ( lphDevice )
		*lphDevice = hDevice;
	else
	    CloseHandle( hDevice );

    return TRUE;
}


/****************************************************************************
*
*    FUNCTION: StopDriver( IN SC_HANDLE, IN LPCTSTR)
*
*    PURPOSE: Has the configuration manager stop the driver (unload it)
*
****************************************************************************/
BOOL StopDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName )
{
    SC_HANDLE       schService;
    BOOL            ret;
    SERVICE_STATUS  serviceStatus;

    schService = OpenService( SchSCManager, DriverName, SERVICE_ALL_ACCESS );
    if ( schService == NULL )
        return FALSE;

    ret = ControlService( schService, SERVICE_CONTROL_STOP, &serviceStatus );

    CloseServiceHandle( schService );

    return ret;
}


/****************************************************************************
*
*    FUNCTION: RemoveDriver( IN SC_HANDLE, IN LPCTSTR)
*
*    PURPOSE: Deletes the driver service.
*
****************************************************************************/
BOOL RemoveDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName )
{
    SC_HANDLE  schService;
    BOOL       ret;

    schService = OpenService( SchSCManager,
                              DriverName,
                              SERVICE_ALL_ACCESS
                              );

    if ( schService == NULL )
        return FALSE;

    ret = DeleteService( schService );
    CloseServiceHandle( schService );
    return ret;
}


/****************************************************************************
*
*    FUNCTION: UnloadDeviceDriver( const TCHAR *)
*
*    PURPOSE: Stops the driver and has the configuration manager unload it.
*
****************************************************************************/
BOOL UnloadDeviceDriver( const TCHAR * Name )
{
	SC_HANDLE	schSCManager;

	schSCManager = OpenSCManager(	NULL,                 // machine (NULL == local)
                              		NULL,                 // database (NULL == default)
									SC_MANAGER_ALL_ACCESS // access required
								);

	StopDriver( schSCManager, Name );
	RemoveDriver( schSCManager, Name );
	 
	CloseServiceHandle( schSCManager );

	return TRUE;
}

/****************************************************************************
*
*    FUNCTION: LoadDeviceDriver( const TCHAR, const TCHAR, HANDLE *)
*
*    PURPOSE: Registers a driver with the system configuration manager 
*	 and then loads it.
*
****************************************************************************/
BOOL LoadDeviceDriver( const TCHAR * Name, const TCHAR * Path, 
					  HANDLE * lphDevice, PDWORD Error )
{
	SC_HANDLE	schSCManager;
	BOOL		okay;

	schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

	// Remove old instances
	RemoveDriver( schSCManager, Name );

	// Ignore success of installation: it may already be installed.
	InstallDriver( schSCManager, Name, Path );

	// Ignore success of start: it may already be started.
	StartDriver( schSCManager, Name );

	// Do make sure we can open it.
	okay = OpenDevice( Name, lphDevice );
	*Error = GetLastError();
 	CloseServiceHandle( schSCManager );

	return okay;
}