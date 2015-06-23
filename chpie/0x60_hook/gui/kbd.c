//
// kbd.c : GUI
//       : Communication with a Driver inp.sys
//

#include <stdio.h>
#include "stdafx.h"
#include "ioctls.h"
#include "driver.h"
#include "keyboard.h"
#include "resource.h"

int CALLBACK DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
int CALLBACK DetailsDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
DWORD WINAPI ThreadProc(LPVOID dump);

// Global Variables:
HINSTANCE	hInst;								// current instance
HWND		hList;
HWND		hText;
HANDLE		hDriver;
HANDLE		hThread;
LVITEM		lvI;
char data[2];

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	InstallDriver();
	StartDriver();

	hInst = hInstance;
	DialogBoxParam( hInstance, MAKEINTRESOURCE( IDD_MAIN ), NULL, DlgProc, WM_INITDIALOG );
	
	UninstallDriver();

	return 0;
}


int CALLBACK DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	LVCOLUMN	lvC;
	DWORD temp;

	switch( uMsg )
	{
		case WM_INITDIALOG:
			hList = GetDlgItem( hDlg, IDC_LIST );

			ListView_SetExtendedListViewStyle( hList, LVS_EX_FULLROWSELECT );

			memset( &lvC, 0, sizeof(LVCOLUMN) );
			lvC.mask		= LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
			lvC.iSubItem	= 0;
			
			lvC.cx			= 85;
			lvC.pszText		= "Buffer (0x60)";
			lvC.fmt			= LVCFMT_LEFT;
			ListView_InsertColumn( hList, 0, &lvC );

			lvC.cx			= 83;
			lvC.pszText		= "Flags (0x64)";
			lvC.fmt			= LVCFMT_RIGHT;
			ListView_InsertColumn( hList, 1, &lvC );

			lvC.cx			= 100;
			lvC.pszText		= "Key";
			lvC.fmt			= LVCFMT_RIGHT;
			ListView_InsertColumn( hList, 2, &lvC );

			lvC.cx			= 100;
			lvC.pszText		= "State";
			lvC.fmt			= LVCFMT_LEFT;
			ListView_InsertColumn( hList, 3, &lvC );

			hDriver = CreateFile("\\\\.\\INP", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, &temp); // thread

			break;

		case WM_COMMAND:
			break;

		case WM_CLOSE:
			CloseHandle(hDriver);
			EndDialog( hDlg, 0 );
			break;
		
		default:
			break;
	}

	return 0;
}

DWORD WINAPI ThreadProc(LPVOID dump)
{							// ThreadProc
	DWORD temp;
	HANDLE hEvent;
	unsigned char data[2], buffer[2], scBuffer[10], szBuffer[10], stBuffer[10];
	unsigned char escape = 0x00;

	int currentLine = 0, i;

	memset( &lvI, 0, sizeof(LVITEM) );
	lvI.mask		= LVIF_TEXT;
	lvI.iSubItem	= 0;
	lvI.pszText		= szBuffer;

	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	while (1)
	{
		DeviceIoControl(hDriver, IOCTL_REGISTER_EVENT, &hEvent, sizeof(hEvent), NULL, 0, &temp, NULL);
		WaitForSingleObject(hEvent, INFINITE);
		ResetEvent(hEvent);

		DeviceIoControl(hDriver, IOCTL_REQUEST_DATA, NULL, 0, &data, sizeof(char[2]), &temp, NULL);

		buffer[0] = data[0];
		buffer[1] = data[1];

		if (buffer[0] == 0xE0 || buffer[0] == 0xE1)
		{
			escape = buffer[0];

			DeviceIoControl(hDriver, IOCTL_REGISTER_EVENT, &hEvent, sizeof(hEvent), NULL, 0, &temp, NULL);
			WaitForSingleObject(hEvent, INFINITE);
			ResetEvent(hEvent);
			DeviceIoControl(hDriver, IOCTL_REQUEST_DATA, NULL, 0, &data, sizeof(char[2]), &temp, NULL);

			buffer[0] = data[0];
		}
		else
			escape = 0x00;

		// below - visual
		for (i = 7; i >= 0; i--)
		{
			szBuffer[i] = ( data[0] & 0x1 ? '1' : '0');
			stBuffer[i] = ( data[1] & 0x1 ? '1' : '0');
			data[0] = data[0] >> 1;
			data[1] = data[1] >> 1;
		}
		szBuffer[8] = stBuffer[8] = '\0';

		ListView_InsertItem ( hList, &lvI );
		ListView_SetItemText( hList, 0, 1, stBuffer);
		ListView_SetItemText( hList, 0, 2, KeyboardSignal(buffer[0], escape, scBuffer, sizeof(char[10])));		
		ListView_SetItemText( hList, 0, 3, (szBuffer[0] == '1' ? "\"break\"" : "\"make\"") );



	}

	CloseHandle(hEvent);

	return 0;
}							// ThreadProc