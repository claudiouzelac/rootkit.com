// Alpha1.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Alpha1.h"
#include "../../Data Model/UserConsole.h"
#include "../../Data Model/LocalNetwork.h"
#include <commctrl.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ConfigIP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK	ConfigServer(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

HWND m_WndNetworks;
HWND m_WndFoundObjects;
HWND m_WndTab;

HFONT tahoma_font		= CreateFont( 
										12, 
										0,
										0,
										0,
										FW_DONTCARE,
										FALSE,
										FALSE,
										FALSE,
										ANSI_CHARSET,
										OUT_DEFAULT_PRECIS,
										CLIP_DEFAULT_PRECIS,
										ANTIALIASED_QUALITY,
										DEFAULT_PITCH,
										"tahoma");

// these are for sharing the data from the IPConfig dialog
char g_ip_address[1024];
char g_tcp_port[25];
char g_server_tcp_port[25];
int g_selected_item = 0;

class MyConsole : public UserConsole
{
public:
	MyConsole() 
	{
		WSAData wsaData;
		WSAStartup(MAKEWORD(2,0), &wsaData);
	}
	virtual ~MyConsole() {}

	virtual void OnMessage(CONSOLE_MESSAGE msg, void *param)
	{
		switch(msg)
		{
		case CMSG_INIT:
			{

			}
			break;
		case CMSG_REFRESH:
			{
				ListView_DeleteAllItems(m_WndNetworks);

				// LOCK the database since sniffing threads are using the data
				// also...
				m_NetworkDatabase.LockMutex();
				NETWORK_LIST::iterator i = m_NetworkDatabase.m_network_list.begin();
				while(i != m_NetworkDatabase.m_network_list.end())
				{
					char _tt[1024];
					Network *np = *i;
					
					// add the network to the list
					LV_ITEM lvi;
					lvi.mask = LVIF_TEXT;
					lvi.iSubItem = 0;
					lvi.iItem = 0;
					_snprintf(_tt, 1022, "%s", np->m_name.c_str());
					lvi.pszText = _tt;					
					ListView_InsertItem(m_WndNetworks, &lvi); 
					
					_snprintf(_tt, 1022, "%d/%d", np->m_number_tx, np->m_number_rx);
					lvi.iSubItem = 1;
					lvi.pszText = _tt;					
					ListView_SetItem(m_WndNetworks, &lvi); 
					
					// TODO, detect other settings
					//lvi.iSubItem = 2;
					//lvi.pszText = "*";					
					//ListView_SetItem(m_WndNetworks, &lvi); 
					
					i++;
				}
				// UNLOCK or wedge the whole program
				m_NetworkDatabase.UnlockMutex();
				
				//ListView_SetHotItem(m_WndNetworks, g_selected_item);
			}
			break;
		case CMSG_ADDPEER:
			{
				RemoteNetwork *aRemoteNetwork = m_ClientConnector.ConnectToRemoteNetwork( g_ip_address, g_tcp_port);
				if(aRemoteNetwork)
				{
					char _tt[1024];

					// give the network a name
					_snprintf(_tt, 1022, "Remote to %s", g_ip_address);
					aRemoteNetwork->m_name.assign(_tt); 
					m_NetworkDatabase.AddNetwork(aRemoteNetwork);
					aRemoteNetwork->StartTunnel();
					
					// add an item to the list view
					LV_ITEM lvi;
					lvi.mask = LVIF_TEXT;
					lvi.iSubItem = 0;
					lvi.iItem = 0;
					_snprintf(_tt, 1022, "%s", g_ip_address);
					lvi.pszText = _tt;					
					ListView_InsertItem(m_WndNetworks, &lvi); 
					
					g_ip_address[0]=0;
				}
				else
				{
					MessageBox(NULL, "Could not connect to remote network", "Error", MB_OK);
					g_ip_address[0]=0;
				}
			}
			break;
		case CMSG_REMOVEPEER:
			{
				Network *aNetwork = FindNetwork(g_ip_address);
				if(aNetwork)
				{
					// stop any i/o threads
					aNetwork->Shutdown();
					
					//remove from network database
					m_NetworkDatabase.RemoveNetwork(aNetwork);
					delete aNetwork;
					ListView_DeleteItem(m_WndNetworks, g_selected_item);
				}
				else
				{
					MessageBox(NULL, "Could not remove network", "error", MB_OK);
				}
			}
			break;
		case CMSG_STARTSERVER:
			if(!m_ServerEngine)
			{
				int port = atoi(g_server_tcp_port);
				m_ServerEngine = new ServerEngine( port, &m_NetworkDatabase );
				m_ServerEngine->Start();
			}
			else
			{
				MessageBox(NULL, "Server already running", "Error", MB_OK);
			}			
			break;
		case CMSG_STOPSERVER:			
			if(m_ServerEngine)
			{
				m_ServerEngine->Stop();
				delete m_ServerEngine;
				m_ServerEngine = NULL;
			}
			else
			{
				MessageBox(NULL, "Server not running", "Error", MB_OK);
			}			
			break;
		default:
			break;
		}
	}
} g_console;


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_ALPHA1, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_ALPHA1);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_ALPHA1);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_ALPHA1;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   INITCOMMONCONTROLSEX isx;
   isx.dwSize = sizeof(INITCOMMONCONTROLSEX);
   isx.dwICC = ICC_LISTVIEW_CLASSES;
   InitCommonControlsEx( &isx );

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 400, 200, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   SendMessage(hWnd, WM_SETFONT, (WPARAM)tahoma_font, FALSE);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   RECT SplitterRect;

   GetClientRect(hWnd,&SplitterRect);

   m_WndTab = CreateWindowEx(
					0L,
					WC_TABCONTROL,
					"",
					WS_CHILD | WS_VISIBLE,
					SplitterRect.left, 
					SplitterRect.top, 
					SplitterRect.right, 
					SplitterRect.bottom, 
					hWnd, //parent
					NULL,
					hInstance,
					NULL);

	if (!m_WndTab)
	{
		int err = GetLastError();
		return false;
	}

	// add tabs
	TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.iImage = -1;
	tie.pszText = "Networks";
	TabCtrl_InsertItem(m_WndTab, 0, &tie);
	tie.pszText = "Found Objects";
	TabCtrl_InsertItem(m_WndTab, 1, &tie);

	SendMessage(m_WndTab, WM_SETFONT, (WPARAM)tahoma_font, FALSE);

	ShowWindow(m_WndTab,SW_NORMAL);
	UpdateWindow(m_WndTab);

	TabCtrl_AdjustRect(m_WndTab, FALSE, &SplitterRect);

    m_WndNetworks = CreateWindowEx(
					0L,
					WC_LISTVIEW,
					"",
					LVS_REPORT | WS_CHILD | WS_VISIBLE,
					SplitterRect.left, 
					SplitterRect.top, 
					SplitterRect.right, 
					SplitterRect.bottom, 
					hWnd, //parent
					NULL,
					hInstance,
					NULL);

	if (!m_WndNetworks)
	{
		int err = GetLastError();
		return false;
	}
	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 330;
	lvc.pszText = "The Network";
	lvc.iSubItem = 0;
	ListView_InsertColumn(m_WndNetworks, 0, &lvc);
	
	lvc.cx = 50;
	lvc.pszText = "Tx/Rx";
	lvc.iSubItem = 1;
	ListView_InsertColumn(m_WndNetworks, 1, &lvc);
	
	SendMessage(m_WndNetworks, WM_SETFONT, (WPARAM)tahoma_font, FALSE);

	m_WndFoundObjects = CreateWindowEx(
					0L,
					WC_LISTVIEW,
					"",
					WS_CHILD | LVS_SMALLICON,
					SplitterRect.left, 
					SplitterRect.top, 
					SplitterRect.right, 
					SplitterRect.bottom, 
					hWnd, //parent
					NULL,
					hInstance,
					NULL);

	if (!m_WndFoundObjects)
	{
		int err = GetLastError();
		return false;
	}
	
	// add dummy item
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = 0;
	lvi.iItem = 0;
	lvi.pszText = "Not Implemented...";					
	ListView_InsertItem(m_WndFoundObjects, &lvi); 
	
	// init our console
	g_console.OnMessage(CMSG_INIT, NULL);
	g_console.OnMessage(CMSG_REFRESH, NULL);
	SetTimer(hWnd, 0, 1000, NULL);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
	case WM_TIMER:
		g_console.OnMessage(CMSG_REFRESH, NULL);
		break;
	case WM_NOTIFY:
			{
				LPNMHDR pnmh = (LPNMHDR)lParam;
				if(pnmh)
				{
					if(pnmh->hwndFrom == m_WndNetworks)
					{
						// message from the list view
						if(pnmh->code == LVN_ITEMCHANGED)
						{
							LPNMLISTVIEW lpn = (LPNMLISTVIEW)pnmh;				
							g_selected_item = lpn->iItem;
							ListView_GetItemText(m_WndNetworks, lpn->iItem, 0, g_ip_address, 1022); 
							ListView_SetHotItem(m_WndNetworks, g_selected_item);
						}
					}
					else if(pnmh->hwndFrom == m_WndFoundObjects)
					{
						// message from the list view
						if(pnmh->code == LVN_ITEMCHANGED)
						{
							LPNMLISTVIEW lpn = (LPNMLISTVIEW)pnmh;				
						}
					}
					else if(pnmh->hwndFrom == m_WndTab)
					{
						if(pnmh->code == TCN_SELCHANGE)
						{
							int iSel = TabCtrl_GetCurSel(m_WndTab);
							switch(iSel)
							{
							case 0:
								ShowWindow(m_WndFoundObjects,SW_HIDE);
								ShowWindow(m_WndNetworks,SW_SHOWNORMAL);
								break;
							case 1:
								ShowWindow(m_WndNetworks,SW_HIDE);
								ShowWindow(m_WndFoundObjects,SW_SHOWNORMAL);
								break;
							default:
								TRACE0("error, tab number not implemented!");
								break;
							}
						}
					}
				}
			}
			break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_SERVER_STARTSERVER:
			if(g_console.m_ServerEngine)
			{
				MessageBox(hWnd, "Server is already running", "Error", MB_OK);
			}
			else if(IDOK == DialogBox(hInst, (LPCTSTR)IDD_DIALOG_SERVER, hWnd, (DLGPROC)ConfigServer))
			{
				g_console.OnMessage(CMSG_STARTSERVER, NULL);						
			}
			break;
		case ID_SERVER_STOPSERVER:
			g_console.OnMessage(CMSG_STOPSERVER, NULL);			
			break;
		case ID_PEER_ADDPEER:
			if(IDOK == DialogBox(hInst, (LPCTSTR)IDD_DIALOG_IP, hWnd, (DLGPROC)ConfigIP))
			{
				g_console.OnMessage(CMSG_ADDPEER, NULL);			
			}
			break;
		case ID_PEER_REMOVEPEER:
			g_console.OnMessage(CMSG_REMOVEPEER, NULL);			
			break;
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}


LRESULT CALLBACK ConfigServer(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		// TODO, remember the last few settings
		SetDlgItemText(hDlg, IDC_EDIT_SPORT, "8055");
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK )
		{
			GetDlgItemText(hDlg, IDC_EDIT_SPORT, g_server_tcp_port, 22);
		}

		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK ConfigIP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		// TODO, remember the last few settings
		SetDlgItemText(hDlg, IDC_EDIT_IPADDRESS, "0.0.0.0");
		SetDlgItemText(hDlg, IDC_EDIT_TCPPORT, "8055");
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK )
		{
			GetDlgItemText(hDlg, IDC_EDIT_IPADDRESS, g_ip_address, 1022);
			GetDlgItemText(hDlg, IDC_EDIT_TCPPORT, g_tcp_port, 22);
		}

		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
