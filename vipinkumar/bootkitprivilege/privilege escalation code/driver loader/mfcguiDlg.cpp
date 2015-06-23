// mfcguiDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mfcgui.h"
#include "mfcguiDlg.h"

#include <Windows.h>
#include <process.h>
#include <fcntl.h>
#include <stdio.h>
#include <Winsvc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif









#define FILE_DEVICE_UNKNOWN		0x00000022
#define MAX_LOADSTRING			100

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)


// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// The title bar text


HANDLE SysHandle		= INVALID_HANDLE_VALUE;
HANDLE hDriverFile;
HANDLE m_hProcessEvent	= NULL;
bool bGUIActive			= false;
int threads				= 0;





BOOL LoadDeviceDriver( const TCHAR * Name, const TCHAR * Path, HANDLE * lphDevice, PDWORD Error );
BOOL UnloadDeviceDriver( const TCHAR * Name );
BOOL OpenDeviceDriver( const TCHAR * Name, PHANDLE lphDevice );
BOOL InstallDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName, IN LPCTSTR ServiceExe );
BOOL StartDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName );
BOOL StopDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName );
BOOL RemoveDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName );
BOOL OpenDevice( IN LPCTSTR DriverName, HANDLE * lphDevice );



/****************************************************************************
*
*    FUNCTION: LoadDeviceDriver( const TCHAR, const TCHAR, HANDLE *)
*
*    PURPOSE: Registers a driver with the system configuration manager 
*	 and then loads it.
*
****************************************************************************/
BOOL LoadDeviceDriver( const char * Name, const char * Path, 
					  HANDLE * lphDevice, PDWORD Error )
{
	SC_HANDLE	schSCManager;
	BOOL		okay;

	schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

	// Remove previous instance
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
    char    completeDeviceName[64];
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

	if( GetVersion() & 0xFF >= 5 ) {

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
BOOL UnloadDeviceDriver( const char * Name )
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





/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMfcguiDlg dialog

CMfcguiDlg::CMfcguiDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMfcguiDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMfcguiDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMfcguiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMfcguiDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMfcguiDlg, CDialog)
	//{{AFX_MSG_MAP(CMfcguiDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnLoadDriver)
	ON_BN_CLICKED(IDC_BUTTON2, OnUnloadDriver)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMfcguiDlg message handlers

BOOL CMfcguiDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMfcguiDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMfcguiDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMfcguiDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMfcguiDlg::OnLoadDriver() 
{
	// TODO: Add your control notification handler code here
	
		char driverSourcePath[ 512 ];
	DWORD	error;
	char msgbuf[1024];
	BOOL bReturnCode = FALSE;
	DWORD dwBytesReturned = 0;     // byte count

	GetCurrentDirectory( sizeof(driverSourcePath), driverSourcePath );
			wsprintf( driverSourcePath + strlen(driverSourcePath), "\\%s", "hookA.sys" );

			if( !LoadDeviceDriver( "hookA", driverSourcePath, &SysHandle, &error ) )  
			{
				UnloadDeviceDriver("hookA");
				// If we didn't find the driver in the same folder as this Application,
				// see if we can load it from the windows folder.
				//GetWindowsDirectory( driverSourcePath, sizeof(driverSourcePath) );
				//wsprintf( driverSourcePath + strlen(driverSourcePath), "\\system32\\drivers\\%s", "clamrt.sys" );

				if ( !LoadDeviceDriver( "hookA", driverSourcePath, &SysHandle, &error ) )  
				{

					wsprintf( msgbuf, "Error loading %s (%s): %d", "hookA.sys", driverSourcePath, error );
					MessageBox(msgbuf, "Error", MB_OK);
					
				//	return 0;
				}
			}

		

}



void CMfcguiDlg::OnUnloadDriver() 
{
	// TODO: Add your control notification handler code here
	UnloadDeviceDriver("hookA");
	
}
