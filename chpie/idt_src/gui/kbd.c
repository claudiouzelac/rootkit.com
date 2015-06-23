//
// kbd.c : GUI
//       : Communication with a Driver idt_hook.sys
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

			hDriver = CreateFile("\\\\.\\IDT_HOOK", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, &temp); // thread

			break;

		case WM_COMMAND:
			switch( LOWORD( wParam ) )
			{
				case IDC_STRUCT:
					DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_DETAIL ), NULL, DetailsDlgProc, WM_INITDIALOG );
					break;

				default:
					break;
			}
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

int CALLBACK DetailsDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	DWORD temp;
	KINTERRUPT dwOut;
	char string[256], buffer[50], sizeC = 0;

	switch( uMsg )
	{
		case WM_INITDIALOG:
			DeviceIoControl(hDriver, IOCTL_KINTERRUPT, NULL, 0, &dwOut, sizeof(KINTERRUPT), &temp, 0);

			sprintf(buffer, "%x - Type : %x\n", 0, dwOut.Type);
			strcpy(string, buffer);
			sprintf(buffer, "%x - Size : %x\n", sizeC += sizeof(dwOut.Type), dwOut.Size); 
			strcat(string, buffer);
			sprintf(buffer, "%x - InterruptListEntry : %x\n", sizeC += sizeof(dwOut.Size), dwOut.InterruptListEntry);
			strcat(string, buffer);
			sprintf(buffer, "%x - ServiceRoutine : %x\n", sizeC += sizeof(dwOut.InterruptListEntry), dwOut.ServiceRoutine); 
			strcat(string, buffer);
			sprintf(buffer, "%x - ServiceContext : %x\n", sizeC += sizeof(dwOut.ServiceRoutine), dwOut.ServiceContext); 
			strcat(string, buffer);
			sprintf(buffer, "%x - SpinLock : %x\n", sizeC += sizeof(dwOut.ServiceContext), dwOut.SpinLock); 
			strcat(string, buffer);
			sprintf(buffer, "%x - TickCount : %x\n", sizeC += sizeof(dwOut.SpinLock), dwOut.TickCount); 
			strcat(string, buffer);
			sprintf(buffer, "%x - ActualLock : %x\n", sizeC += sizeof(dwOut.TickCount), dwOut.ActualLock); 
			strcat(string, buffer);
			sprintf(buffer, "%x - DispatchAddress : %x\n", sizeC += sizeof(dwOut.ActualLock), dwOut.DispatchAddress); 
			strcat(string, buffer);
			sprintf(buffer, "%x - Vector : %x\n", sizeC += sizeof(dwOut.DispatchAddress), dwOut.Vector); 
			strcat(string, buffer);
			sprintf(buffer, "%x - Irql : %x\n", sizeC += sizeof(dwOut.Vector), dwOut.Irql); 
			strcat(string, buffer);
			sprintf(buffer, "%x - SynchronizeIrql : %x\n", sizeC += sizeof(dwOut.Irql), dwOut.SynchronizeIrql);
			strcat(string, buffer);
			sprintf(buffer, "%x - FloatingSave : %x\n", sizeC += sizeof(dwOut.SynchronizeIrql), dwOut.FloatingSave);
			strcat(string, buffer);
			sprintf(buffer, "%x - Connected : %x\n", sizeC += sizeof(dwOut.FloatingSave), dwOut.Connected);
			strcat(string, buffer);
			sprintf(buffer, "%x - Number : %x\n", sizeC += sizeof(dwOut.Connected), dwOut.Number);
			strcat(string, buffer);
			sprintf(buffer, "%x - ShareVector : %x\n", sizeC += sizeof(dwOut.Number), dwOut.ShareVector);
			strcat(string, buffer);
			sprintf(buffer, "%x - Mode : %x\n", sizeC += sizeof(dwOut.ShareVector), dwOut.Mode);
			strcat(string, buffer);
			sprintf(buffer, "%x - ServiceCount : %x\n", sizeC += sizeof(dwOut.Mode), dwOut.ServiceCount);
			strcat(string, buffer);
			sprintf(buffer, "%x - DispatchCount : %x\n", sizeC += sizeof(dwOut.ServiceCount), dwOut.DispatchCount);
			strcat(string, buffer);
			sprintf(buffer, "%x - DispatchCode : %x\n", sizeC += sizeof(dwOut.DispatchCount), dwOut.DispatchCode);	
			strcat(string, buffer);

			SetDlgItemText(hDlg, IDC_INFORMATION, string);

			break;

		case WM_CLOSE:
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
		for (i = 0; i <= 7; i++)
		{
			szBuffer[i] = ( data[0] & 0x80 ? '1' : '0');
			stBuffer[i] = ( data[1] & 0x80 ? '1' : '0');
			data[0] = data[0] << 1;
			data[1] = data[1] << 1;
		}

		szBuffer[i] = stBuffer[i] = '\0';
		ListView_InsertItem ( hList, &lvI );
		ListView_SetItemText( hList, 0, 1, stBuffer);
		ListView_SetItemText( hList, 0, 2, KeyboardSignal(buffer[0], escape, scBuffer, sizeof(char[10])));		
		ListView_SetItemText( hList, 0, 3, (buffer[0] & 0x80 ? "\"break\"" : "\"make\"") );

	}

	CloseHandle(hEvent);

	return 0;
}							// ThreadProc