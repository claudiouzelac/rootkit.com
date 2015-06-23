#include <afxwin.h>
#include <psapi.h>
#include <atlbase.h>
#include <shellapi.h>
#include <winioctl.h>
#include <afxcmn.h>
#include "resource.h"

#define	COMM_DRIVER_WIN32_DEV_NAME	L"\\DosDevices\\ShadowTableHookDriver"
#define	COMM_DRIVER_DEV_NAME		L"\\Device\\ShadowTableHookDriver"

#define	DEVICE_SHADOW_TABLE_HOOK_DRIVER			0x00008810

#define	IO_UNHOOK_SYSTEM_SERVICES	(ULONG) CTL_CODE(DEVICE_SHADOW_TABLE_HOOK_DRIVER, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS)
#define	IO_HOOK_SYSTEM_SERVICES		(ULONG) CTL_CODE(DEVICE_SHADOW_TABLE_HOOK_DRIVER, 0x802, METHOD_NEITHER, FILE_ANY_ACCESS)
#define	IO_SEND_HIDDEN_PROCESS_ID	(ULONG) CTL_CODE(DEVICE_SHADOW_TABLE_HOOK_DRIVER, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct QLDriver_Extension
{
	HANDLE hCommEvent;
} QL_DRIVER_EXT;

CHAR DriverFileLocation[MAX_PATH];
SC_HANDLE SCManager;
SC_HANDLE Driver;
INT bResult;
HANDLE CommDevice;
BOOL DriverLoaded = FALSE;
LPVOID lpMsgBuf;

VOID GetDriverFileLocation()
{
	CHAR CurrentDirectoryPath[MAX_PATH];
	GetCurrentDirectory(sizeof(CurrentDirectoryPath), CurrentDirectoryPath);

	if(CurrentDirectoryPath[3] != '\0')
	{
		strcat(CurrentDirectoryPath, "\\ShadowTableHookDriver.sys");
	}
	else
		strcat(CurrentDirectoryPath, "ShadowTableHookDriver.sys");

	strcpy(DriverFileLocation, CurrentDirectoryPath);
}

VOID UnloadDriver()
{
	if(CommDevice)
	{
		DWORD dwReturn;

		IsGUIThread(TRUE);

		if (0 == (DeviceIoControl(CommDevice, IO_UNHOOK_SYSTEM_SERVICES, NULL, 0, 
			NULL, 0, &dwReturn, NULL)))
		{
			int error = GetLastError();

			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, 
				GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 
				0, NULL);

			MessageBox(NULL, (LPTSTR)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION);

			return;
		}
	}

	if(CommDevice)
	{
		CloseHandle(CommDevice);
	}

	if (bResult)
	{
		SERVICE_STATUS status;
		::ControlService(Driver, SERVICE_CONTROL_STOP, &status);

		::DeleteService(Driver);

		if (Driver != NULL)
		{
			::CloseServiceHandle(Driver);
			::CloseServiceHandle(SCManager);
			Driver = NULL;
		}
	}

	bResult = FALSE;
}

VOID LoadDriver()
{
	SCManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (NULL != SCManager)
	{
		//GetDriverFileLocation();
		Driver = ::CreateService(SCManager, "ShadowTableHookDriver", "", SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
			"F:\\ShadowTableHook\\ShadowTableHook_Driver\\objchk\\i386\\ShadowTableHook_Driver.sys", NULL, NULL, NULL, NULL, NULL);
			//DriverFileLocation, NULL, NULL, NULL, NULL, NULL);
	}

	if (NULL == Driver)
	{
		if ( (::GetLastError() == ERROR_SERVICE_EXISTS) ||
			    (::GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE) )
		{
			Driver = ::OpenService(SCManager, "ShadowTableHookDriver", SERVICE_ALL_ACCESS);
		}
	}

	if (NULL != Driver)
	{
		SERVICE_STATUS serviceStatus = { 0 };
		bResult = ::StartService(Driver, 0, NULL);
		if (bResult)
		{
			bResult = ::QueryServiceStatus(Driver, &serviceStatus);

			if (SERVICE_RUNNING != serviceStatus.dwCurrentState)
			{
				bResult = FALSE;
			}
		}
		else
		{
			bResult = (::GetLastError() == ERROR_SERVICE_ALREADY_RUNNING);
		}

		if (!bResult)
		{
			::DeleteService(Driver);
			if (Driver != NULL)
			{
				::CloseServiceHandle(Driver);
				::CloseServiceHandle(SCManager);
				Driver = NULL;
			}

			return;
		}
	}
}

class CMainDialog: public CDialog
{
public:
	CMainDialog(UINT ID, CWnd* Owner = NULL);

	virtual BOOL OnInitDialog();
	afx_msg VOID OnHideWindowHandles();
	afx_msg VOID OnShowWindowHandles();
	afx_msg BOOL OnQueryEndSession();
	afx_msg VOID OnClose();

	DECLARE_MESSAGE_MAP()

private:
};

CMainDialog::CMainDialog(UINT ID, CWnd* Owner):
	CDialog(ID, Owner) {}

BOOL CMainDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;
}

ULONG GetProcessID(LPCSTR TargetProcessName)
{
    DWORD nSize = MAX_PATH;
	HANDLE hCurrentProcess;
	CHAR BaseName[MAX_PATH];
		
    DWORD aProcesses[1024], cbNeeded;
    unsigned int i;

    if (!EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded))
        return FALSE;

   // NumberProcesses = nSize = cbNeeded / sizeof(DWORD);

    for (i = 0; i <= nSize; i++)
	{
		if(NULL != (hCurrentProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, aProcesses[i])))
		{
			GetModuleBaseName(hCurrentProcess, NULL, BaseName, sizeof(BaseName));

			if(0 == stricmp(BaseName, TargetProcessName))
			{
				CloseHandle(hCurrentProcess);

				return aProcesses[i];
			}
		}

		CloseHandle(hCurrentProcess);
	}

	return FALSE;
}

VOID CMainDialog::OnHideWindowHandles()
{
	CHAR ProcessName[MAX_PATH];
	CEdit *pProcessName = (CEdit *)GetDlgItem(IDC_PROCESSNAME);
	CButton *pHideWindowHandlesButton = (CButton *)GetDlgItem(IDHIDEWINDOWHANDLES);
	CButton *pShowWindowHandlesButton = (CButton *)GetDlgItem(IDSHOWWINDOWHANDLES);

	pProcessName->GetWindowText(ProcessName, sizeof(ProcessName));
	strcat(ProcessName, ".exe");

	DWORD HiddenWindowHandleProcessId;
	LPVOID pHiddenWindowHandleProcessId = &HiddenWindowHandleProcessId;

	if(FALSE != (HiddenWindowHandleProcessId = GetProcessID(ProcessName)))
	{
		pHideWindowHandlesButton->EnableWindow(FALSE);
		pShowWindowHandlesButton->EnableWindow(TRUE);
	}
	else
	{
		MessageBox("Process Does Not Exist!", "Error", MB_ICONERROR);
		return;
	}

	if (DriverLoaded)
	{
		UnloadDriver();
		DriverLoaded = FALSE;
	}

	LoadDriver();
	DriverLoaded = TRUE;

	CommDevice = CreateFile("\\\\.\\ShadowTableHookDriver", GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if(CommDevice == INVALID_HANDLE_VALUE)
	{
		int error = GetLastError();

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, 
			GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 
			0, NULL);

		MessageBox((LPTSTR)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION);

		return;
	}

	if(CommDevice)
	{
		DWORD dwReturn;

		if (0 == (DeviceIoControl(CommDevice, IO_SEND_HIDDEN_PROCESS_ID, 
			pHiddenWindowHandleProcessId, sizeof(HiddenWindowHandleProcessId), 
			NULL, 0, &dwReturn, NULL)))
		{
			int error = GetLastError();

			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, 
				GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 
				0, NULL);

			MessageBox((LPCSTR)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION);

			return;
		}
	}

	if(CommDevice)
	{
		DWORD dwReturn;

		IsGUIThread(TRUE);

		if (0 == (DeviceIoControl(CommDevice, IO_HOOK_SYSTEM_SERVICES, NULL, 0, 
			NULL, 0, &dwReturn, NULL)))
		{
			int error = GetLastError();

			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, 
				GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 
				0, NULL);

			MessageBox((LPCSTR)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION);

			return;
		}
	}

}

VOID CMainDialog::OnShowWindowHandles()
{
	CButton *pHideWindowHandlesButton = (CButton *)GetDlgItem(IDHIDEWINDOWHANDLES);
	CButton *pShowWindowHandlesButton = (CButton *)GetDlgItem(IDSHOWWINDOWHANDLES);

	if (DriverLoaded)
	{
		UnloadDriver();
		DriverLoaded = FALSE;
	}

	pHideWindowHandlesButton->EnableWindow(TRUE);
	pShowWindowHandlesButton->EnableWindow(FALSE);

	
}

afx_msg BOOL CMainDialog::OnQueryEndSession()
{
	if (DriverLoaded)
	{
		UnloadDriver();
		DriverLoaded = FALSE;
	}

	ExitProcess(0);
}

afx_msg VOID CMainDialog::OnClose()
{
	if (DriverLoaded)
	{
		UnloadDriver();
		DriverLoaded = FALSE;
	}

	ExitProcess(0);
}

class CApp: public CWinApp
{
public:

	BOOL InitInstance();
	BOOL ExitInstance();

private:
	HANDLE hGlobalMutex;

};

BOOL CApp::InitInstance()
{
	hGlobalMutex = ::CreateMutex(NULL, TRUE, "GlobalMutex");

	switch(::GetLastError())
	{
		case ERROR_SUCCESS:
		{
			INITCOMMONCONTROLSEX initCtrls;

			initCtrls.dwSize = sizeof(initCtrls);
			initCtrls.dwICC = ICC_TAB_CLASSES;
			InitCommonControlsEx(&initCtrls);

			CMainDialog MainDialog(IDD_MAINDIALOG);
			m_pMainWnd = &MainDialog;
			MainDialog.DoModal();

			return TRUE;
		}

		case ERROR_ALREADY_EXISTS:
		{

			return FALSE;
		}

		default:
		{

			return FALSE;
		}
	}
}

BOOL CApp::ExitInstance()
{
	return TRUE;
}

BEGIN_MESSAGE_MAP(CMainDialog, CDialog)
  ON_COMMAND(IDHIDEWINDOWHANDLES, OnHideWindowHandles)
  ON_COMMAND(IDSHOWWINDOWHANDLES, OnShowWindowHandles)
  ON_WM_QUERYENDSESSION()
  ON_WM_CLOSE()
END_MESSAGE_MAP()

CApp App;
