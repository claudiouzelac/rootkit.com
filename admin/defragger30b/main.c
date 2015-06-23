// main.c - handles gui, drives, threads, outputdata
// defragger 3.0beta for nt/2k/xp free.pages.at/blumetools/ blume1975@web.de

#include "globals.h"
#include "resource.h"
#include "defrag.h"
#include "statbar.h"
#include "list.h"
#include "mft.h"
#include <direct.h>
#include <math.h>
#include <SHLOBJ.H>
//#include <CTL3D.H>

#define WIN32_LEAN_AND_MEAN              // for a smaller exe
#define _WIN32_IE             0x0400     // need some ex-styles for listview
                                         // Comctl32.dll version 4.71 and later
#define LVIF_NORECOMPUTE      0x0800     // for compilers with old commctrl.h
#define LR_VGACOLOR           0x0080
#define LR_SHARED             0x8000
//#define LVS_EX_FULLROWSELECT  0x00000020              // applies to report mode only
//#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54) // optional wParam==mask
//#define LVM_GETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 55) // optional wParam==mask
//#define LVS_EX_CHECKBOXES       0x00000004
#define MM10PERINCH         254         // Tenths of a millimeter per inch.

TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];
TCHAR szDefrag[MAX_LOADSTRING];

HMENU         hMenu;      // menu-handle
HMENU         hFileMenu;
HIMAGELIST    hSmall;     // Handle to image list
BOOL          minimized;
BOOL          maximized;
BOOL          bChangeSize;
BOOL          bUpdating;  // another thing to wait for in CheckActivities()
int           lastleft, lasttop, lastwidth, lastheight;
struct FILEstruct * currentfile;

// protos
ATOM              MyRegisterClass( HINSTANCE );
BOOL              InitInstance( HINSTANCE, int );
BOOL              InitClusterView( HINSTANCE, HWND );
LRESULT CALLBACK  DefaultWndProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK  AboutProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK  ClusterViewProc( HWND, UINT, WPARAM, LPARAM );
void              config_write(void);

void              DoWantedLetters(void *act);
void              DoDefrag(void *act);
void              DefragOneFile(void *dummy);
void              ChangeSize(HWND hWnd);
void              CreateBackBuffer(HWND hWnd);

BOOL              UpdateDrivesMenu(HMENU);
void              UpdateDriveList(void);

void              ErrorMsg(LPTSTR szMsg);
void              AddToClusterList(int img, char *name, ULONGLONG Lcn,
                                   ULONGLONG Len, char *status);
void              OpenVolume(void);
void              SetSizeLimit(int MID);
void              SetBlockSize(int newsize);
void              SetPriority(int procpr);
BOOL              CalcClusterNo(long *xpos, long *ypos);
void              GetSelectedDrives(void);
void              FindFile(void);
void              GetFileInfo(struct FILEstruct *Here, char * Text2, int *img);

// main main
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine, int       nCmdShow )
{
    MSG msg;
    HACCEL hAccelTable;
    int i=0;

    ghInst = hInstance;
    SetCursor(LoadCursor(NULL, IDC_APPSTARTING));

    // Globale Zeichenfolgen initialisieren
    LoadString(ghInst, IDS_MAIN, szTitle, MAX_LOADSTRING);
    LoadString(ghInst, IDC_MAIN, szWindowClass, MAX_LOADSTRING);
    hiMain = LoadImage(ghInst, (LPCTSTR)IDI_SMALL, IMAGE_ICON,0,0,LR_VGACOLOR|LR_SHARED);

    {
      // read commandline
	  char *cmds = CharLower(lpCmdLine);
      bDoDEFRAG=FALSE;
      bDoOPTIMIZE=FALSE;
      bDoBOTH=FALSE;
      bDoDIRDEFRAG=FALSE;
      bAllLocalDisks=FALSE;
	  bQuitAfterJobs = FALSE;
      wantedLetters[0]=0;
      while (cmds && cmds[0] != 0)
         if (cmds[0]=='-') {
            if (cmds[1]=='d' || cmds[1]=='D')
               bDoDEFRAG=TRUE;
            if (cmds[1]=='o' || cmds[1]=='O')
               bDoOPTIMIZE=TRUE;
            if (cmds[1]=='b' || cmds[1]=='B')
               bDoBOTH=TRUE;
            if (cmds[1]=='x' || cmds[1]=='X')
               bAllLocalDisks=TRUE;
            if (cmds[1]=='q' || cmds[1]=='Q')
				bQuitAfterJobs = TRUE;
            cmds+=2;
            if (cmds[0]==32) cmds++;
         } else {
            while ( (cmds[i]>96 && cmds[i]<123) || (cmds[i]>64 && cmds[i]<91) ) {
               if       (cmds[i]>96 && cmds[i]<123) {
                  wantedLetters[i]=cmds[i]-32;
               } else if (cmds[i]>64 && cmds[i]<91) {
                  wantedLetters[i]=cmds[i];
			   }
               i++;
            }
            i++;
            cmds+=i;
         }
    }
    wantedLetters[i]=0;

    // register
	MyRegisterClass(ghInst);
    
	// now initialize the whole thing and show the window
    if( !InitInstance( ghInst, nCmdShow ) ) 
		return FALSE;

    // load key-shortcuts
	hAccelTable = LoadAccelerators(ghInst, (LPCTSTR)IDC_MAIN);

    // determine sysvol-ID
    while ((cWinDir[0]-65) != cDriveLetters[i++]);
    // wich volume to start with ?
	if (wantedLetters[0]==0)
		SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+i-1, 0);   // sysvol
	else
		SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+wantedLetters[0]-65, 0);   // firstt selected volume

    (void*)_beginthread(DoWantedLetters, 0, NULL);

    //SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+1, 0);   // C
    //SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+2, 0);   // D
    //SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+3, 0);   // E
    //SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+4, 0);   // F
    //SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+5, 0);   // G
    //SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+6, 0);   // H
    //SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+7, 0);   // I
    //SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+8, 0);   // J
    //SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+9, 0);   // K

    // main message loop
    while( GetMessage(&msg, NULL, 0, 0) ) {
        if( !TranslateAccelerator (msg.hwnd, hAccelTable, &msg) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }
    return msg.wParam;
}

ATOM MyRegisterClass( HINSTANCE hInstance ) {
	WNDCLASSEX wcex;
	wcex.cbSize        = sizeof(WNDCLASSEX);
	wcex.style         = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc   = (WNDPROC)DefaultWndProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = hInstance;
	wcex.hIcon         = LoadIcon(hInstance, (LPCTSTR)IDI_MAIN);
	wcex.hIconSm       = hiMain;
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = 0x000000;
	wcex.lpszMenuName  = (LPCSTR)IDC_MAIN;
	wcex.lpszClassName = szWindowClass;

	return RegisterClassEx(&wcex);
}

// saves ghInstance, creates main window, sets init variables
BOOL InitInstance( HINSTANCE hInstance, int nCmdShow ) {
    HWND hWnd;
    HMENU hsub;
    int left=0, top=0, dx=0, dy=0; 
    char ini_file[MAX_PATH];
    char buff[20];

    hMenu     = LoadMenu(hInstance, (LPCTSTR)IDC_MAIN);
    hFileMenu = LoadMenu(hInstance, (LPCTSTR)IDC_FILE);
    hsub = GetSubMenu(hMenu, 0);
    DeleteMenu(hsub, 10, MF_BYPOSITION); // delete "Stop" menu-entry

    lstrcpy(ini_file, "win.ini");

    left = CW_USEDEFAULT; top = 0;
    dx = CW_USEDEFAULT; dy = 0;
    left = GetPrivateProfileInt(szTitle,"Left",left,ini_file);
    top = GetPrivateProfileInt(szTitle,"Top",top,ini_file);
    dx = GetPrivateProfileInt(szTitle,"Width",dx,ini_file);
    dy = GetPrivateProfileInt(szTitle,"Height",dy,ini_file);

    lastleft = GetPrivateProfileInt(szTitle,"LastLeft",left,ini_file);
    lasttop = GetPrivateProfileInt(szTitle,"LastTop",top,ini_file);
    lastwidth = GetPrivateProfileInt(szTitle,"LastWidth",dx,ini_file);
    lastheight = GetPrivateProfileInt(szTitle,"LastHeight",dy,ini_file);

    // create main window
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        left, top, dx, dy,
        NULL, hMenu, hInstance, NULL);
    
	if(!hWnd ) return FALSE;

    ghInst= hInstance;
    ghWnd = hWnd;

    hiDir    = LoadImage(ghInst, (LPCTSTR)IDI_DIR   , IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);
    hiFile   = LoadImage(ghInst, (LPCTSTR)IDI_FILE  , IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);
    hiFrag   = LoadImage(ghInst, (LPCTSTR)IDI_FRAG  , IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);
    hiFree   = LoadImage(ghInst, (LPCTSTR)IDI_FREE  , IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);
    hiSystem = LoadImage(ghInst, (LPCTSTR)IDI_SYSTEM, IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);
    hiMFT    = LoadImage(ghInst, (LPCTSTR)IDI_MFT   , IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);
    hiComp   = LoadImage(ghInst, (LPCTSTR)IDI_COMP  , IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);

    ColBG      = GetSysColor(COLOR_BTNFACE);

    hbBG       = CreateSolidBrush(ColBG);
    hpBG       = CreatePen(PS_SOLID, 1, ColBG);

                //  B G R not RGB!!! the MS way of colors
	ColDGreen  = 0x005500; ColGreen   = 0x009900; ColLGreen  = 0x00BB00;
	ColL2Green = 0x00DD00; ColL3Green = 0x00FF00; ColVio     = 0xCC00CC;
	ColDVio    = 0x990099; ColRed     = 0x0000FF; ColDRed    = 0x0000CC;
	ColOck     = 0x00CCFF; ColYellow  = 0x99CCCC; ColLYellow = 0x00FFFF; 
	ColBlue    = 0x990000; ColLBlue   = 0xFF6666; ColL2Blue  = 0xFF9999; 
	ColBrown   = 0x003399; ColWhite   = 0xFFFFFF; ColGray    = 0xBBBBBB; 
	ColBlack   = 0x000000;

	hpDGreen = CreatePen(PS_SOLID, 1, ColDGreen  ); hpGreen  = CreatePen(PS_SOLID, 1, ColGreen   );
	hpLGreen = CreatePen(PS_SOLID, 1, ColLGreen  ); hpL2Green= CreatePen(PS_SOLID, 1, ColL2Green );
	hpL3Green= CreatePen(PS_SOLID, 1, ColL3Green ); hpVio    = CreatePen(PS_SOLID, 1, ColVio     );
	hpDVio   = CreatePen(PS_SOLID, 1, ColVio     ); hpRed    = CreatePen(PS_SOLID, 1, ColRed     );
	hpDRed   = CreatePen(PS_SOLID, 1, ColDRed    ); hpOck    = CreatePen(PS_SOLID, 1, ColOck     ); 
	hpYellow = CreatePen(PS_SOLID, 1, ColYellow  ); hpLYellow= CreatePen(PS_SOLID, 1, ColLYellow ); 
	hpBlue   = CreatePen(PS_SOLID, 1, ColBlue    ); hpLBlue  = CreatePen(PS_SOLID, 1, ColLBlue   ); 
	hpL2Blue = CreatePen(PS_SOLID, 1, ColL2Blue  ); hpBrown  = CreatePen(PS_SOLID, 1, ColBrown   ); 
	hpWhite  = CreatePen(PS_SOLID, 1, ColWhite   ); hpGray   = CreatePen(PS_SOLID, 1, ColGray    ); 
	hpBlack  = CreatePen(PS_SOLID, 1, ColBlack   );

	hbDGreen = CreateSolidBrush(ColDGreen  ); hbGreen  = CreateSolidBrush(ColGreen   );
	hbLGreen = CreateSolidBrush(ColLGreen  ); hbL2Green= CreateSolidBrush(ColL2Green );
	hbL3Green= CreateSolidBrush(ColL3Green ); hbVio    = CreateSolidBrush(ColVio     );
	hbDVio   = CreateSolidBrush(ColDVio    ); hbRed    = CreateSolidBrush(ColRed     );
	hbDRed   = CreateSolidBrush(ColDRed    ); hbOck    = CreateSolidBrush(ColOck     ); 
	hbYellow = CreateSolidBrush(ColYellow  ); hbLYellow= CreateSolidBrush(ColLYellow ); 
	hbBlue   = CreateSolidBrush(ColBlue    ); hbLBlue  = CreateSolidBrush(ColLBlue   ); 
	hbL2Blue = CreateSolidBrush(ColL2Blue  ); hbBrown  = CreateSolidBrush(ColBrown   ); 
	hbWhite  = CreateSolidBrush(ColWhite   ); hbGray   = CreateSolidBrush(ColGray    ); 
	hbBlack  = CreateSolidBrush(ColBlack   );
    
    ghDraw            = NULL;
    ghAnalyze         = NULL;
    ghDefrag          = NULL;
    ghListWnd         = NULL;
    dirlist = NULL; topdirlist = NULL;
    mftlist = NULL; topmftlist = NULL;
    memBM   = NULL;

    bAnalyzed       = FALSE;
	bDumped         = FALSE;
    bStopAction     = FALSE;
    bUpdating       = FALSE;
    
    blocksize = 0;
    SetBlockSize(GetPrivateProfileInt(szTitle,"BlockSize",9,ini_file));

    GetPrivateProfileString(szTitle,"ShowLargeFileMarks","FALSE",buff,32,ini_file);
    if (lstrcmpi(buff, "TRUE")==0) bShowMarks = TRUE;
                                else bShowMarks = FALSE;
    if (!bShowMarks) CheckMenuItem(hMenu, IDM_SHOWMARKS, MF_UNCHECKED);
                else CheckMenuItem(hMenu, IDM_SHOWMARKS, MF_CHECKED); 

    if (blocksize < 5) 
        EnableMenuItem(hMenu, IDM_SHOWMARKS, MF_BYCOMMAND|MF_GRAYED);

    bChangeSize     = FALSE;
    minimized       = FALSE;
    maximized       = FALSE;
    xposlast=0, yposlast=0;
    lstrcpy(cLastSelected, "");
    cActualDrive = -1;
    cTimesRestarted = 0;

    sLimit = GetPrivateProfileInt(szTitle,"SizeLimit",50,ini_file);
    switch (sLimit) {  // crude i know
      case 0:    top=32819; break;
      case 20:   top=32820; break;
      case 50:   top=32821; break;
      case 100:  top=32822; break;
      case 250:  top=32823; break;
      case 500:  top=32824; break;
      case 650:  top=32825; break;
      case 700:  top=32826; break;
      case 800:  top=32827; break;
      case 1000: top=32828; break;
      case 2000: top=32829; break;
      default:   top=32821; break; // 2^63 - 1
    }
    SetSizeLimit(top);
    lastsort=0;

    InitClusterView(hInstance, hWnd);

    GetPrivateProfileString(szTitle,"Maximized","FALSE",buff, 32, ini_file);
    if (lstrcmpi(buff, "TRUE")==0) 
		ShowWindow(hWnd, SW_MAXIMIZE);
	else 
		ShowWindow(hWnd, SW_SHOW);
	
	UpdateWindow(hWnd);
    UpdateDrivesMenu(hMenu);
    SetPriority(IDM_PPNORMAL);

    GetWindowsDirectory((LPSTR) cWinDir, 512);
    lstrcat(cWinDir, "\\");
    GetSystemDirectory((LPSTR) cSysDir, 512);
    lstrcat(cSysDir, "\\");
    lstrcpy(cSysDir2, cSysDir);
    lstrcat(cSysDir2, "config\\");

    return TRUE;
}

void config_write(void) {
    RECT rectTemp;
    ULONG  ulStyleBits = GetWindowLong(ghWnd, GWL_STYLE);
    char ini_file[MAX_PATH];
    char string[32];
    int v=0;

    lstrcpy(ini_file, "win.ini");

    GetWindowRect(ghWnd,&rectTemp);
    
	wsprintf(string,"%d",rectTemp.left);
    WritePrivateProfileString(szTitle,"Left",string,ini_file);
    wsprintf(string,"%d",rectTemp.top);
    WritePrivateProfileString(szTitle,"Right",string,ini_file);
    wsprintf(string,"%d",rectTemp.right-rectTemp.left);
    WritePrivateProfileString(szTitle,"Width",string,ini_file);
    wsprintf(string,"%d",rectTemp.bottom-rectTemp.top);
    WritePrivateProfileString(szTitle,"Height",string,ini_file);

	wsprintf(string,"%d",lastleft);
    WritePrivateProfileString(szTitle,"LastLeft",string,ini_file);
    wsprintf(string,"%d",lasttop);
    WritePrivateProfileString(szTitle,"LastTop",string,ini_file);
    wsprintf(string,"%d",lastwidth);
    WritePrivateProfileString(szTitle,"LastWidth",string,ini_file);
    wsprintf(string,"%d",lastheight);
    WritePrivateProfileString(szTitle,"LastHeight",string,ini_file);

    wsprintf(string,"%d",blocksize);
    WritePrivateProfileString(szTitle,"BlockSize",string,ini_file);
    wsprintf(string,"%d",sLimit);
    WritePrivateProfileString(szTitle,"SizeLimit",string,ini_file);

    if (ulStyleBits & WS_MAXIMIZE) wsprintf(string,"TRUE");
                              else wsprintf(string,"FALSE");
    WritePrivateProfileString(szTitle,"Maximized",string,ini_file);

    if (bShowMarks) wsprintf(string,"TRUE");
               else wsprintf(string,"FALSE");
    WritePrivateProfileString(szTitle,"ShowLargeFileMarks",string,ini_file);
}

// make a string from number, with dots (22234 -> 22.234)
char * fmtNumber(__int64 _in, char *cGes) {
    char in[30];
    int c=0, c2=0;
    int l, l1;
    div_t l2;
    sprintf(in, "%I64u", _in);
    l = strlen(in);
    if (l<4) {
        sprintf(cGes, "%I64u", _in);
        return cGes;
    }
    l2 = div(l, 3);
    l1 = l + l2.quot - 1;
    cGes[l1+1] = '\0';
    while (l1>=0) {
        cGes[l1--] = in[--l];
        if ((++c2) % 3==0)
            if (l1!=0) cGes[l1--] = 46; // a dot
                  else cGes[l1--] = 32; // a space
    }
    if (cGes[0]==32) 
      cGes = cGes + 1; // strip first space
    return cGes;
}

// I did not find a suitable strstr_ignorecase function...
int strstrlwr(const char *string1, const char *substring2) {
    unsigned int i=0, j=0, z=0;
    char *x;
    char *s, *subs;

    s    = (char *)malloc(strlen(string1) + 1);
    subs = (char *)malloc(strlen(substring2) + 1);

    lstrcpy(s, string1); lstrcpy(subs, substring2);
    CharLower(s); CharLower(subs);

    x = strstr(s, subs);
    (x) ?  (z = x - s) : (z = -1);

    free(s); 
    free(subs);
    return z;
}

// when redrawing, all other things must wait
BOOL CheckActivities(void) {
    if ( bStopAction ) return TRUE;
    while (bUpdating) 
        Sleep(500); // some dirty work to wait for list-fill-functions
    if ( ghDraw ) WaitForSingleObject(ghDraw, INFINITE);
    return FALSE;
}

// ..
void ResetStatusBar(void) {
    UpdateStatusBar("0 dirs", 1, 0);
    UpdateStatusBar("0 files", 2, 0);
    UpdateStatusBar("0 fragmented", 3, 0);
    UpdateStatusBar(NULL, 4, 0);
    lstrcpy(LastOutLine, "");
}

// verify correct number according to cluster# in clusterviewer
BOOL CalcClusterNo(long *pxpos, long *pypos) {
    char Text[512];    // place to store some text.
    int xpos, ypos, x=0, y=0;
    ULONGLONG Cstart=0;
    ldiv_t dr;

    xpos = (int)*pxpos; ypos = (int)*pypos;

    dr = ldiv(xpos, blocksize); xpos = dr.quot*blocksize;
    dr = ldiv(ypos, blocksize); ypos = dr.quot*blocksize;

    while ( y <= ypos ) { y += blocksize; Cstart += clustersPL; }
    Cstart -= clustersPL;
    while ( x <= xpos ) { x += blocksize; Cstart += clustersPB; }
    Cstart -= clustersPB;

    if ((Cstart >= cDiskClusters) || x>imgWidth)
        return FALSE;

    sprintf(Text, "%I64u", Cstart);
    SetWindowText(GetDlgItem(ghListWnd, IDC_CLUSTERNO), Text);
    *pxpos = (long)xpos; *pypos = (long)ypos;
    return TRUE;
}

// ...
void SetBlockSize(int newsize) {
    HMENU hsubmenu;
    HMENU hsubmenu2;

    if (blocksize==newsize) return;

    // needed to make a new bitmap
    bw         = 0;
    bh         = 0;
	imgWidth   = 0;
	imgHeight  = 0;
    blocksize  = newsize;

    hsubmenu   = GetSubMenu(hMenu, 2);
    hsubmenu2  = GetSubMenu(hsubmenu, 1);
    CheckMenuItem(hsubmenu2, IDM_B1+blocksize-1, MF_UNCHECKED);
    CheckMenuItem(hsubmenu2, IDM_B1+newsize-1, MF_CHECKED);

	// if blocksize too small no need for file-marks
	if (blocksize < 5) EnableMenuItem(hMenu, IDM_SHOWMARKS, MF_BYCOMMAND|MF_GRAYED);
	              else EnableMenuItem(hMenu, IDM_SHOWMARKS, MF_BYCOMMAND|MF_ENABLED);

    if (!memBM) return;
    ChangeSize(ghWnd);
}

// ...
void ChangeSize(HWND hWnd) {
    RECT rt;
    HDC hdc;
    int old_bw, old_bh, check=0;

    if (cDiskClusters==0) return;

    GetClientRect(hWnd, &rt);
    if ((imgWidth==rt.right && imgHeight+23==rt.bottom) ||
        (rt.right==0 || rt.bottom==0) )
        return;

    imgWidth  = rt.right;
    imgHeight = rt.bottom-dist; // dist - distance of drive layout from statusbar

    old_bw = bw; old_bh = bh;

    // calculate vis-values
    bw = (int)((imgWidth          ) / blocksize);
    bh = (int)((imgHeight-dist*2.5) / blocksize); // dist - distance of drive  layout from statusbar
	if (bh<=0) bh=1;
	clustersPB = 1;
	while (bw*bh*clustersPB < cDiskClusters) clustersPB++;
	clustersPL = bw*clustersPB;
    if (old_bw==bw && old_bh==bh) return;

    SetScrollRange(GetDlgItem(ghListWnd, IDC_SCROLL), SB_CTL, 0, bw*bh, TRUE);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    CreateBackBuffer(hWnd);
    hdc = GetDC(hWnd);
    BitBlt(hdc,0,0,imgWidth,imgHeight,memDC,0,0,SRCCOPY);
    ReleaseDC(hWnd,hdc);

    if (ghDraw)
        WaitForSingleObject(ghDraw, INFINITE);

    if (ghVolume)
    ghDraw = (void*)_beginthread(DrawBitMap, 0, NULL);
    if (ghDraw) {
        SetThreadPriority(ghDraw, procprior);
        WaitForSingleObject(ghDraw, INFINITE);
    }
    UpdateStatusBar(NULL, 5, 0);
}

// create bitmap to draw clusters onto
void CreateBackBuffer(HWND hWnd) {
	HDC hdc = GetDC(hWnd);

	if (memDC) { 
	  // delete doublebuffer existing
	  DeleteObject(memBM);
      DeleteDC(memDC);

	  /*
	  // wondering why this does not work
	  SetBitmapDimensionEx(memBM, 
		        (imgWidth  * MM10PERINCH) /
                GetDeviceCaps(memDC, LOGPIXELSX),
				(imgHeight * MM10PERINCH) /
                GetDeviceCaps(memDC, LOGPIXELSY),
				NULL);
      */
	}

	// create the backbuffer
	memDC = CreateCompatibleDC(hdc);
	memBM = CreateCompatibleBitmap(hdc,imgWidth,imgHeight);
	ReleaseDC(hWnd, hdc);

	SelectObject(memDC, memBM);
	SetBkMode(memDC, TRANSPARENT);
	SelectObject(memDC, GetStockObject(DEFAULT_GUI_FONT));
	SetTextColor(memDC, 0x333333);
	SelectObject(memDC, hpBG);
    SelectObject(memDC, hbBG);
    Rectangle(memDC, 0, 0,imgWidth, imgHeight);
    xposlast=0; yposlast=0;
}

// ....
void SetSizeLimit(int MID) {
    HMENU hsub;
    hsub = GetSubMenu(hMenu, 0);

    switch (sLimit) {
        case 0:    CheckMenuItem(hsub, IDM_LNO, MF_UNCHECKED); break;
        case 20:   CheckMenuItem(hsub, IDM_L20, MF_UNCHECKED); break;
        case 50:   CheckMenuItem(hsub, IDM_L50, MF_UNCHECKED); break;
        case 100:  CheckMenuItem(hsub, IDM_L100, MF_UNCHECKED); break;
        case 250:  CheckMenuItem(hsub, IDM_L250, MF_UNCHECKED); break;
        case 500:  CheckMenuItem(hsub, IDM_L500, MF_UNCHECKED); break;
        case 650:  CheckMenuItem(hsub, IDM_L650, MF_UNCHECKED); break;
        case 700:  CheckMenuItem(hsub, IDM_L700, MF_UNCHECKED); break;
        case 800:  CheckMenuItem(hsub, IDM_L800, MF_UNCHECKED); break;
        case 1000: CheckMenuItem(hsub, IDM_L1000, MF_UNCHECKED); break;
        case 2000: CheckMenuItem(hsub, IDM_L2000, MF_UNCHECKED); break;
        default:   CheckMenuItem(hsub, IDM_L50, MF_UNCHECKED); break;
        // 2^63 - 1
    }

    switch (MID) {
        case IDM_LNO: CheckMenuItem(hsub, IDM_LNO, MF_CHECKED);
            sLimit=0;    sizeLimit = 9223372036854775807; break; // 2^63 - 1
        case IDM_L20: CheckMenuItem(hsub, IDM_L20, MF_CHECKED);
            sLimit=20;   sizeLimit = 20971520; break;
        case IDM_L50: CheckMenuItem(hsub, IDM_L50, MF_CHECKED);
            sLimit=50;   sizeLimit = 52428800; break;
        case IDM_L100: CheckMenuItem(hsub, IDM_L100, MF_CHECKED);
            sLimit=100;  sizeLimit = 104857600; break;
        case IDM_L250: CheckMenuItem(hsub, IDM_L250, MF_CHECKED);
            sLimit=250;  sizeLimit = 262144000; break;
        case IDM_L500: CheckMenuItem(hsub, IDM_L500, MF_CHECKED);
            sLimit=500;  sizeLimit = 524288000; break;
        case IDM_L650: CheckMenuItem(hsub, IDM_L650, MF_CHECKED);
            sLimit=650;  sizeLimit = 681574400; break;
        case IDM_L700: CheckMenuItem(hsub, IDM_L700, MF_CHECKED);
            sLimit=700;  sizeLimit = 734003200; break;
        case IDM_L800: CheckMenuItem(hsub, IDM_L800, MF_CHECKED);
            sLimit=800;  sizeLimit = 838860800; break;
        case IDM_L1000: CheckMenuItem(hsub, IDM_L1000, MF_CHECKED);
            sLimit=1000; sizeLimit = 1048576000; break;
        case IDM_L2000: CheckMenuItem(hsub, IDM_L2000, MF_CHECKED);
            sLimit=2000; sizeLimit = 2097152000; break;
    }

    if (ghDraw) WaitForSingleObject(ghDraw, INFINITE);
    if (ghVolume)
    ghDraw = (void*)_beginthread(DrawBitMap, 0, NULL);
    if (ghDraw) {
        SetThreadPriority(ghDraw, procprior);
        WaitForSingleObject(ghDraw, INFINITE);
    }
}

// ..
BOOL InitClusterView(HINSTANCE hInstance, HWND hWnd) {
    LV_COLUMN lvC;    // List view column structure.
    char szText[512]; // place to store some text.
    long lStyle;

    ghListWnd = CreateDialog(hInstance,
        (LPCTSTR)IDD_CLUSTERVIEW, hWnd, (DLGPROC)ClusterViewProc);

    lStyle = SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    lStyle = lStyle | LVS_EX_FULLROWSELECT;
    SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lStyle);

    SendMessage(ghListWnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)(HICON)hiMain);
    SetWindowText(GetDlgItem(ghListWnd, IDC_CLUSTERNO), "0");

    // Icons for the Listview
    hSmall = ImageList_Create (16, 16, ILC_COLORDDB, 7, 0);
    ImageList_SetBkColor(hSmall, GetSysColor(COLOR_WINDOW));
    ImageList_AddIcon(hSmall, hiDir);    // 0
    ImageList_AddIcon(hSmall, hiFile);   // 1
    ImageList_AddIcon(hSmall, hiFrag);   // 2
    ImageList_AddIcon(hSmall, hiComp);   // 3
    ImageList_AddIcon(hSmall, hiMFT);    // 4
    ImageList_AddIcon(hSmall, hiFree);   // 5
    ImageList_AddIcon(hSmall, hiSystem); // 6
    ListView_SetImageList(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), hSmall, LVSIL_SMALL);

    // Now initialize the columns
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;
    // Add the columns
    lvC.cx = 170; lstrcpy(szText, "Name(s)");
    lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
    ListView_InsertColumn(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), 0, &lvC);

    lvC.cx = 65;  lstrcpy(szText, "Startcluster");
    lvC.fmt = LVCFMT_RIGHT; // right-align the column.
    ListView_InsertColumn(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), 1, &lvC);

    lvC.cx = 60;  lstrcpy(szText, "Length");
    lvC.fmt = LVCFMT_RIGHT; // right-align the column.
    ListView_InsertColumn(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), 2, &lvC);

    lvC.cx = 130; lstrcpy(szText, "Extra");
    lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
    ListView_InsertColumn(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), 3, &lvC);

    return TRUE;
}

void EnableMenus(int ID) {
    HMENU hsub;
    TCHAR szV[512];
    int i;

    switch (ID) {
    case 0: sprintf(szV, "&Analyze\tF5"); break;
    case 1: sprintf(szV, "Defrag &Fragmented\tF7"); break;
    case 2: sprintf(szV, "Defrag Free &Space\tF8"); break;
    case 3: sprintf(szV, "&Defrag Fragmented && Free Space\tF9"); break;
    case 5: sprintf(szV, "Defrag by sor&ting Directories\tF10"); break;
    default: break;
    }

    hsub = GetSubMenu(hMenu, 0);
    DeleteMenu(hsub, ID, MF_BYPOSITION);
    InsertMenu(hsub, ID, MF_STRING | MF_BYPOSITION | MF_ENABLED, IDM_ANALYZE+ID, szV);

    for (i=0; i<6; i++)
        if (i!=ID)
            EnableMenuItem(hsub, i, MF_BYPOSITION|MF_ENABLED);
    // drive-menu
    EnableMenuItem(hMenu,DRIVE_MENU_NUM,MF_BYPOSITION|MF_ENABLED);
    DrawMenuBar(ghWnd);
}

void DisableMenus(int ID) {
    HMENU hsub;
    TCHAR szV[512];
    int i;

    sprintf(szV, "&Stop\tEsc");
    hsub = GetSubMenu(hMenu, 0);
    DeleteMenu(hsub, ID, MF_BYPOSITION);
    InsertMenu(hsub, ID, MF_STRING | MF_BYPOSITION | MF_ENABLED, IDM_STOP, szV);
    for (i=0; i<6; i++)
        if (i!=ID)
            EnableMenuItem(hsub, i, MF_BYPOSITION|MF_GRAYED);

    // drive-menu
    EnableMenuItem(hMenu,DRIVE_MENU_NUM,MF_BYPOSITION|MF_GRAYED);
    DrawMenuBar(ghWnd);
}

void DoWantedLetters(void *act) {	/**/
	int i=0;
	if (wantedLetters[0]==0)
		wantedLetters[0]=cActualDrive;

	while (wantedLetters[i++]!=0) {
		if (bStopAction) break;

		WaitForSingleObject(ghDefrag, INFINITE);
		//cActualDrive=wantedLetters[i-1]-65;
		if (cActualDrive!=wantedLetters[i-1]-65)
			SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+wantedLetters[i-1]-65, 0);

		if (bStopAction) break;

		if (bDoBOTH) {
			ghDefrag = (void*)_beginthread(DoDefrag, 0, (void *)DEFRAGBOTH);
			WaitForSingleObject(ghDefrag, INFINITE);
		} else 
		if (bDoDEFRAG) {
			ghDefrag = (void*)_beginthread(DoDefrag, 0, (void *)DEFRAGFRAGS);
			WaitForSingleObject(ghDefrag, INFINITE);
		} else
		if (bDoOPTIMIZE) {
			ghDefrag = (void*)_beginthread(DoDefrag, 0, (void *)DEFRAGFREE);
			WaitForSingleObject(ghDefrag, INFINITE);
		} else
		if (bDoDIRDEFRAG) {
			//athread = (void*)_beginthread(DoDefrag, 0, (void *)DEFRAGBYDIRS);
			//WaitForSingleObject(athread, INFINITE);
		} else
			break;
	}
	if (bQuitAfterJobs) 
		SendMessage(ghWnd, WM_DESTROY, 0, 0);
	bQuitAfterJobs = FALSE;
}

void DoDefrag(void *act) {
    int action = (int) act;
	int i;

    FilesMoved=0;

    bStopAction = FALSE;
    if (! bAnalyzed && ghAnalyze==NULL)
		SendMessage(ghWnd, WM_COMMAND, IDM_ANALYZE, 0);

    // wait for analyze
    if ( ghAnalyze )
        WaitForSingleObject(ghAnalyze, INFINITE);

    switch (action) {
    case DEFRAGFRAGS:
        DisableMenus(1);
        ghDefrag = (void*)_beginthread(DefragFragments, 0, NULL);
        SetThreadPriority(ghDefrag, procprior);
        break;
    case DEFRAGFREE:
        DisableMenus(2);
        ghDefrag = (void*)_beginthread(DefragFreeSpace, 0, NULL);
        SetThreadPriority(ghDefrag, procprior);
        break;
    case DEFRAGBOTH:
        DisableMenus(1);
        ghDefrag = (void*)_beginthread(DefragFragments, 0, NULL);

        SetThreadPriority(ghDefrag, procprior);
        WaitForSingleObject(ghDefrag, INFINITE);

        if (CheckActivities()) break;
        // new analyze needed after DEFRAGFREE
        if (!bAnalyzed) {
            SendMessage(ghWnd, WM_COMMAND, IDM_ANALYZE, 0);
            // wait for analyze
            if ( ghAnalyze ) WaitForSingleObject(ghAnalyze, INFINITE);
            if (CheckActivities()) break;
        }

        DisableMenus(2);
        ghDefrag = (void*)_beginthread(DefragFreeSpace, 0, NULL);

        SetThreadPriority(ghDefrag, procprior);
        break;
    case DEFRAGBYDIRS:
        DisableMenus(5);
        ghDefrag = (void*)_beginthread(DefragByDirectories, 0, NULL);

        SetThreadPriority(ghDefrag, procprior);
        break;
    default:
        break;
        }
}

// find the given string in a file from dirlist and add to clusterview
void FindFile(void) {
    char searchbuff[512], buff[512];
    struct FILEstruct *Here;
    int img=-1;
	__int64 totalsize=0;
    unsigned long dcount=0, fcount=0;

    GetWindowText(GetDlgItem(ghListWnd, IDC_FILENAME), searchbuff, 512);
    if (lstrcmp(searchbuff, "")==0) return;

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    bUpdating       = TRUE;
    ListView_DeleteAllItems(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));

    Here = topdirlist;
    while (Here) {
        if (strstrlwr(Here->fileName, searchbuff) >= 0 &&
            (Here->fragid<=1 && Here->fragments>=1) // describing entry
           ) 
		{
            if (Here->filesize == IINVALID)
				dcount++;
			else {
				fcount++;
				totalsize += Here->filesize;
			}
            lstrcpy(buff, "");

            GetFileInfo(Here, buff, &img);
			AddToClusterList(img, Here->fileName, Here->Lcn, Here->Len, buff);
        }
        Here = Here->next;
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    bUpdating       = FALSE;

    if (dcount+fcount > 0) {
        char buff2[50];
		searchbuff[0]=0;
		sprintf(buff, "%d dirs/%d files found (%d)", dcount, fcount, dcount+fcount);
		sprintf(buff2, "size: %s bytes", fmtNumber(totalsize, searchbuff));
		AddToClusterList(-1, buff, LLINVALID, LLINVALID, buff2);
    } else {
        sprintf(buff, "--- nothing found ---");
		AddToClusterList(-1, buff, LLINVALID, LLINVALID, "");
    }
}

void SetPriority(int procpr) {
    DWORD PROCPrior;

	if        (procprior==THREAD_PRIORITY_BELOW_NORMAL)
        CheckMenuItem(hMenu, IDM_PPBELOWNORMAL, MF_UNCHECKED);
    else if (procprior==THREAD_PRIORITY_NORMAL)
        CheckMenuItem(hMenu, IDM_PPNORMAL, MF_UNCHECKED);
    else if (procprior==THREAD_PRIORITY_ABOVE_NORMAL)
        CheckMenuItem(hMenu, IDM_PPABOVENORMAL, MF_UNCHECKED);

    CheckMenuItem(hMenu, procpr, MF_CHECKED);
    if      (procpr==IDM_PPBELOWNORMAL) {
        procprior = THREAD_PRIORITY_BELOW_NORMAL;
		PROCPrior = IDLE_PRIORITY_CLASS;
	} else if (procpr==IDM_PPNORMAL) {
        procprior = THREAD_PRIORITY_NORMAL;
		PROCPrior = NORMAL_PRIORITY_CLASS;
    } else if (procpr==IDM_PPABOVENORMAL) {
        procprior = THREAD_PRIORITY_ABOVE_NORMAL;
		PROCPrior = HIGH_PRIORITY_CLASS;
	}

	SetPriorityClass(GetCurrentProcess(), PROCPrior);
    if (ghAnalyze) SetThreadPriority(ghAnalyze, procprior);
    if (ghDefrag) SetThreadPriority(ghDefrag, procprior);
    if (ghDraw) SetThreadPriority(ghDraw, procprior);
}

void DefragOneFile(void *dummy) {
    struct FILEstruct *Here=NULL;
    char buff[512];

    DisableMenus(1);

    if (MoveFileFragmented(currentfile, &currentfile->Lcn)==currentfile->Len) {
        sprintf(buff, "finished %s", currentfile->fileName);
        OutLine1(buff);
	}

    DrawProgress(-1);
    UpdateStatusBar("", 4, 0);
    ghDefrag = NULL;
    // re-enable menu
    EnableMenus(1);
}

// --------------------------------
// process messages for main window
// --------------------------------
LRESULT CALLBACK DefaultWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;

    switch( message )
    {
        case WM_CREATE:
            { // OS-related things
                OSVERSIONINFO  osvi;
                char           subver[50];
                osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
                GetVersionEx(&osvi);
                sprintf(cOSVersion, "Windows ");
                switch (osvi.dwPlatformId) {
                case VER_PLATFORM_WIN32_NT:
                    if        (osvi.dwMajorVersion < 5) {
                        sprintf(subver, "NT %d.%d, build %d %s",
                            osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, osvi.szCSDVersion);
                        lstrcat(cOSVersion, subver);
                    } else if (osvi.dwMajorVersion==5 && osvi.dwMinorVersion < 1 ) {
                        sprintf(subver, "2000 %d.%d, build %d %s",
                            osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, osvi.szCSDVersion);
                        lstrcat(cOSVersion, subver);
                    } else if (osvi.dwMajorVersion==5 && osvi.dwMinorVersion >= 1 ) {
                        sprintf(subver, "XP %d.%d, build %d %s",
                            osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, osvi.szCSDVersion);
                        lstrcat(cOSVersion, subver);
                    }
					break;
                case VER_PLATFORM_WIN32_WINDOWS:
                case VER_PLATFORM_WIN32s:
                default:
                    MessageBox(NULL, (LPCTSTR)"Sorry, only running on Windows NT/2000/XP.",
                                     (LPCTSTR) szTitle, MB_OK);
                    exit(1);
                }
            }
            CreateSBar(hWnd);
            break;
        //case WM_GETMINMAXINFO:
        //case WM_WINDOWPOSCHANGED:
        case WM_SIZE:
        case WM_SIZING:
        case WM_WINDOWPOSCHANGING:
            SendMessage(ghStatWnd, message, wParam, lParam);
            break;
        case WM_EXITSIZEMOVE: {
            HDC hdc;
            if (bChangeSize) break;
            ChangeSize(hWnd);
            hdc = GetDC(hWnd);
            BitBlt(hdc,0,0,imgWidth,imgHeight,memDC,0,0,SRCCOPY);
            ReleaseDC(hWnd,hdc);
            break;
            }
        case WM_MOVE:
         {
                HDC hdc = GetDC(hWnd);
                BitBlt(hdc,0,0,imgWidth,imgHeight,memDC,0,0,SRCCOPY);
                ReleaseDC(hWnd,hdc);
                //SendMessage(ghStatWnd, message, wParam, lParam);
                //UpdateStatusBar(NULL, 5, 0);
            } break;
        case WM_SHOWWINDOW:
        //case WM_ACTIVATE:
            if (LOWORD(wParam)==WA_ACTIVE ||      // activation flag
                LOWORD(wParam)==WA_CLICKACTIVE ||
                (BOOL)wParam==TRUE                ) {
                HDC hdc = GetDC(hWnd);
                BitBlt(hdc,0,0,imgWidth,imgHeight,memDC,0,0,SRCCOPY);
                ReleaseDC(hWnd,hdc);
            } break;
        case WM_ERASEBKGND: {
                RECT rt;
                HDC hdc;
                if (bChangeSize) break;

                hdc = GetDC(hWnd);
                GetClientRect(hWnd, &rt);
                BitBlt(hdc,0,0,rt.right,rt.bottom,memDC,0,0,SRCCOPY);
                ReleaseDC(hWnd,hdc);

                hdc = GetDC(hWnd);
                // clear new space
                if (imgWidth+1<rt.right) {
                    rt.left = imgWidth;
                    rt.bottom = rt.bottom-dist;
                    FillRect(hdc, &rt, hbBG);
                }

                if (imgHeight<rt.bottom) {
                    rt.top = imgHeight;
                    rt.bottom = rt.bottom-dist;
					rt.left = 0;
                    FillRect(hdc, &rt, hbBG);
                }
                ReleaseDC(hWnd,hdc);
            } return 1;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0)==SC_MAXIMIZE) {
				RECT rt;
                GetWindowRect(hWnd, &rt);
				lastleft=rt.left; lasttop=rt.top; 
				lastwidth=rt.right-rt.left;
				lastheight=rt.bottom-rt.top;

                maximized = TRUE;
                minimized = FALSE;
				ShowWindow(hWnd, SW_MAXIMIZE);
                ChangeSize(hWnd);
                return 0;
            } else if ((wParam & 0xfff0)==SC_MINIMIZE) {
                ShowWindow(hWnd, SW_MINIMIZE);
                minimized = TRUE;
                return 0;
            } else if ((wParam & 0xfff0)==SC_RESTORE) {
                ShowWindow(hWnd, SW_RESTORE);
				if (!maximized) {
					MoveWindow(ghWnd, lastleft, lasttop, lastwidth, lastheight, TRUE);
				}
                if (minimized) {
                    minimized = FALSE;
                }
                ChangeSize(hWnd);
                return 0;
            }
            UpdateStatusBar(NULL, 5, 0);
            return DefWindowProc( hWnd, message, wParam, lParam );
        case WM_KEYDOWN: //keyboard messages
            if (wParam==VK_RIGHT) {
                long xy = xposlast+blocksize;
                if (CalcClusterNo(&xy, &yposlast)) {
                    DrawNoBlockBound(xposlast, yposlast);
                    if (xposlast+blocksize < bw*blocksize)
                        xposlast += blocksize;
                    DrawBlockBound(xposlast, yposlast);
                }
            }
            if (wParam==VK_LEFT) {
                long xy = xposlast-blocksize;
                if (CalcClusterNo(&xy, &yposlast)) {
                    DrawNoBlockBound(xposlast, yposlast);
                    if (xposlast>1)
                        xposlast -= blocksize;
                    DrawBlockBound(xposlast, yposlast);
                }
            }
            if (wParam==VK_DOWN) {
                long xy = yposlast+blocksize;
                if (CalcClusterNo(&xposlast, &xy)) {
                    DrawNoBlockBound(xposlast, yposlast);
                    if (yposlast+blocksize < bh*(blocksize*2))
                        yposlast += blocksize;
                    DrawBlockBound(xposlast, yposlast);
                }
            }
            if (wParam==VK_UP) {
                long xy = yposlast-blocksize;
                if (CalcClusterNo(&xposlast, &xy)) {
                    DrawNoBlockBound(xposlast, yposlast);
                    if (yposlast>1)
                        yposlast -= blocksize;
                    DrawBlockBound(xposlast, yposlast);
                }
            }

            if (wParam==VK_SPACE) {
                POINT p;
                LPARAM l;
                l = MAKELONG(xposlast, yposlast);

                p.x = xposlast;
                p.y = yposlast;
                SendMessage(ghListWnd, WM_USER, WM_UPDATECLUSTERLIST, l);
            }
            break;
        case WM_LBUTTONDOWN:
                SendMessage(ghListWnd, WM_USER, WM_UPDATECLUSTERLIST, lParam);
        break;
        case WM_RBUTTONDOWN: {
            POINT pnt;
            HMENU hsub;

            pnt.x = LOWORD(lParam);
            pnt.y = HIWORD(lParam);

            if (CalcClusterNo(&pnt.x, &pnt.y)) {
                DrawNoBlockBound(xposlast, yposlast);
                xposlast=pnt.x; yposlast=pnt.y; // remember last block coords
                DrawBlockBound(xposlast, yposlast);

            hsub = GetSubMenu(hMenu, 2);
            TrackPopupMenu(hsub, 0,
               xposlast+ (int)(blocksize*1.1),
               yposlast+ blocksize*4         , 0, hWnd, NULL);
            } else {
            hsub = GetSubMenu(hMenu, 2);
            TrackPopupMenu(hsub, 0, pnt.x, pnt.y, 0, hWnd, NULL);
         }
         }
         break;
      case WM_COMMAND:
            if( (LOWORD(wParam) - MM_DRIVE_NUM) <= 25 &&
                (LOWORD(wParam) - MM_DRIVE_NUM) >= 0 ) {
                lParam = LOWORD(wParam);
                wParam = MM_DRIVE_NUM;
            }
            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);

            // Menueauswahl analysieren:
            switch( wmId ) {

                case IDM_DEFRAGTHIS:
					ghDefrag = (void*)_beginthread(DefragOneFile, 0, NULL);
                   break;
                case IDM_BROWSE: {
                        char param[512];
                        lstrcpy(param, "/e,/select,");
                        lstrcat(param, currentfile->fileName);
                        ShellExecute(ghWnd, "open", "explorer.exe", param, NULL, SW_SHOWNORMAL);
                    } break;
                case IDM_CLUSTERVIEW:
                    {
                  if (IsWindowVisible(ghListWnd)) {
                         DrawNoBlockBound(xposlast, yposlast);
                      ShowWindow(ghListWnd, SW_HIDE);
                  } else
                     SendMessage(hWnd, WM_LBUTTONDOWN, 0, (LPARAM)-1);
               }
                break;
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;
                case IDM_STOP:
                case IDM_ESCAPE:
                    if ( !bStopAction && (ghDefrag || ghAnalyze)) 
					{
                        char buff[512];
                        lstrcpy(buff, "STOPPED - wait for ");
                        lstrcat(buff, LastFile);
                        UpdateStatusBar(buff, 4, 0);
                    }
                    bStopAction = TRUE;
					bQuitAfterJobs = FALSE;
                    break;
                case IDM_ABOUT:
                    CreateDialog(ghInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)AboutProc);
                    break;
                case IDM_B1: SetBlockSize(1); break;
                case IDM_B3: SetBlockSize(3); break;
                case IDM_B5: SetBlockSize(5); break;
                case IDM_B7: SetBlockSize(7); break;
                case IDM_B9: SetBlockSize(9); break;
                case IDM_B11: SetBlockSize(11); break;
                case IDM_B13: SetBlockSize(13); break;
                case IDM_B15: SetBlockSize(15); break;
                case IDM_B17: SetBlockSize(17); break;
                case IDM_B19: SetBlockSize(19); break;
                case IDM_L20:   case IDM_L50:  case IDM_L100:
                case IDM_L250:  case IDM_L500: case IDM_L650:
                case IDM_L700:  case IDM_L800: case IDM_L1000:
                case IDM_L2000: case IDM_LNO:
                    SetSizeLimit(wmId); break;
                case IDM_DRIVEINFO:
                    ShowWindow(ghListWnd, SW_SHOW);
                    SendMessage(ghListWnd, WM_USER, WM_UPDATEDRIVEINFO, 0);
                    break;
                case IDM_DUMPTOFILE:
	                (void*)_beginthread(DumpLayoutToFile, 0, NULL);
                    break;
                case IDM_PPBELOWNORMAL: SetPriority(IDM_PPBELOWNORMAL); break;
                case IDM_PPNORMAL:      SetPriority(IDM_PPNORMAL); break;
                case IDM_PPABOVENORMAL: SetPriority(IDM_PPABOVENORMAL); break;
                case IDM_HIDECV:        ShowWindow(ghListWnd, SW_HIDE); break;
                case IDM_SHOWMARKS:
                    if (bShowMarks) CheckMenuItem(hMenu, IDM_SHOWMARKS, MF_UNCHECKED);
                               else CheckMenuItem(hMenu, IDM_SHOWMARKS, MF_CHECKED);
                    
					bShowMarks = !bShowMarks;
                    if (blocksize>3)
	                if (bShowMarks) 
                        (void*)_beginthread(DrawAllMarks, 0, NULL);
                    else {
                        ghDraw = (void*)_beginthread(DrawBitMap, 0, NULL);
                        if (ghDraw) {
                            SetThreadPriority(ghDraw, procprior);
                            WaitForSingleObject(ghDraw, INFINITE);
                        }
                    }
                    break;
				case IDM_DRIVELIST:
                    ShowWindow(ghListWnd, SW_SHOW);
                    SendMessage(ghListWnd, WM_USER, WM_UPDATEDRIVELIST, 0);
					break;

                case IDM_REFRESH: {
                    int i=0;
                    UpdateDrivesMenu(hMenu);
                    while (cDriveLetters[i++] != cActualDrive);
                    SendMessage(hWnd, WM_COMMAND, MM_DRIVE_NUM+i-1, 0);
                    }
                   break;
                case MM_DRIVE_NUM: {
                    int i=0;
					char buff[512];
                    while (cDriveLetters[i++] != cActualDrive);
                    CheckMenuItem(hMenu, MM_DRIVE_NUM+i-1, MF_UNCHECKED);
                    cActualDrive = cDriveLetters[lParam-wParam];
                    CheckMenuItem(hMenu, lParam          , MF_CHECKED);

					imgWidth=0;
					imgHeight=0;
					OpenVolume();
					ChangeSize(hWnd); //includes DrawBitMap

					if (IsWindowVisible(ghListWnd)) {
					    GetWindowText(ghListWnd, buff, 512);
					    if (strstrlwr(buff, "Drive Information") >= 0)
						    UpdateDriveInformation();
					    else
						    (void*)_beginthread(UpdateClusterList, 0, NULL);
					}
				    return(1);
                }
                case IDM_ANALYZE:
					bStopAction=FALSE;
                    if (ghAnalyze==NULL) {
                        ghAnalyze = (void*)_beginthread(AnalyzeVolume, 0, NULL);
                        if ( ghAnalyze) {
                            bStopAction = FALSE;
                            SetThreadPriority(ghAnalyze, procprior);
                            DisableMenus(0);
                        }
                    }
                   break;
                case IDM_DEFRAGFRAGS:
					bStopAction=FALSE;
					(void*)_beginthread(DoWantedLetters, 0, NULL);
					bDoDEFRAG=TRUE; bDoOPTIMIZE=FALSE; bDoBOTH=FALSE;
                    //(void*)_beginthread(DoDefrag, 0, (void *)DEFRAGFRAGS);
                    break;
                case IDM_DEFRAGFREE:
					bStopAction=FALSE;
                    (void*)_beginthread(DoWantedLetters, 0, NULL);
					bDoDEFRAG=FALSE; bDoOPTIMIZE=TRUE; bDoBOTH=FALSE;
                    //(void*)_beginthread(DoDefrag, 0, (void *)DEFRAGFREE);
                    break;
                case IDM_DIRECTORYDEFRAG:
					bStopAction=FALSE;
                    MessageBox(ghWnd, "coming soon...", "Sorry", MB_OK);
					bDoDEFRAG=FALSE; bDoOPTIMIZE=FALSE; bDoBOTH=FALSE;
                    //(void*)_beginthread(DoDefrag, 0, (void *)DEFRAGBYDIRS);
                    break;
                case IDM_DEFRAG:
					bStopAction=FALSE;
                    (void*)_beginthread(DoWantedLetters, 0, NULL);
					bDoDEFRAG=FALSE; bDoOPTIMIZE=FALSE; bDoBOTH=TRUE;
                    //(void*)_beginthread(DoDefrag, 0, (void *)DEFRAGBOTH);
                    break;
                default:
                    return DefWindowProc( hWnd, message, wParam, lParam );
            } // switch(wmId)
            break;

        case WM_DESTROY:
            config_write();

            // if something is running...
            bStopAction = TRUE;
			bUpdating = FALSE;
            Sleep(500); // waitfor finish

            //if (ghDefrag)   WaitForSingleObject(ghDefrag, INFINITE);
            if (topdirlist) DelDIR(topdirlist);
            if (topmftlist) DelDIRmft(topmftlist);
            if (ghVolume)   CloseHandle(ghVolume);

			ghDraw    = NULL;
			ghAnalyze = NULL;
			ghDefrag  = NULL;
			ghListWnd = NULL;
			dirlist = NULL; topdirlist = NULL;
			mftlist = NULL; topmftlist = NULL;

            // uhoh ... memory freeing ... maybe I f_rg_t s_m_th_ng
            DeleteObject(memBM); DeleteDC(memDC);
            DeleteObject(hpBG); DeleteObject(hbBG);
            DeleteObject(hpDGreen);DeleteObject(hpGreen);DeleteObject(hpLGreen);
            DeleteObject(hpL2Green);DeleteObject(hpL3Green);DeleteObject(hpVio);
            DeleteObject(hpRed);DeleteObject(hpOck);DeleteObject(hpYellow);
            DeleteObject(hpLYellow);DeleteObject(hpBlue);DeleteObject(hpLBlue);
            DeleteObject(hpBrown);DeleteObject(hpWhite);DeleteObject(hpGray);
            DeleteObject(hpBlack);
            DeleteObject(hbGreen);DeleteObject(hbLGreen);DeleteObject(hbL2Green);
            DeleteObject(hbL3Green);DeleteObject(hbVio);DeleteObject(hbRed);
            DeleteObject(hbOck);DeleteObject(hbYellow);DeleteObject(hbLYellow);
            DeleteObject(hbBlue);DeleteObject(hbLBlue);DeleteObject(hbBrown);
            DeleteObject(hbWhite);DeleteObject(hbGray);DeleteObject(hbBlack);
            DestroyMenu(hFileMenu);
            DestroyMenu(hMenu);

            PostQuitMessage(0); // this really is the end
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;
}

#ifndef ListView_SetCheckState 
#define ListView_SetCheckState(h, i, f) \
        ListView_SetItemState(h, i, INDEXTOSTATEIMAGEMASK((f) + 1), LVIS_STATEIMAGEMASK) 
#endif

void UpdateDriveList(void) {
    HMENU          hDrivesMenu;
    int            NumMenuItems=0, i;
    DWORD          dwLoop;
	char           buff[128], buff2[128], buff3[128], buff4[128];
    char           firstcActualDrive = cActualDrive;
    __int64        fV=0;
    long           lStyle;

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    bUpdating       = TRUE;

    lStyle = SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    lStyle = lStyle | LVS_EX_CHECKBOXES;
    SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lStyle);

	ListView_DeleteAllItems(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));
    SetWindowText(ghListWnd, "Select Drives");
	SetWindowText(GetDlgItem(ghListWnd, IDC_DEFRAG), "SET");
	EnableWindow(GetDlgItem(ghListWnd, IDC_DEFRAG), TRUE);
	
	ShowWindow(GetDlgItem(ghListWnd, IDC_STATIC0), SW_HIDE);
	ShowWindow(GetDlgItem(ghListWnd, IDC_FILENAME), SW_HIDE);
	ShowWindow(GetDlgItem(ghListWnd, IDC_CLUSTERNO), SW_HIDE);
	ShowWindow(GetDlgItem(ghListWnd, IDC_SCROLL), SW_HIDE);
	ShowWindow(GetDlgItem(ghListWnd, IDC_FIND), SW_HIDE);
	ShowWindow(GetDlgItem(ghListWnd, IDC_BROWSE), SW_HIDE);

	hDrivesMenu = GetSubMenu( hMenu, DRIVE_MENU_NUM);
    NumMenuItems = GetMenuItemCount(hDrivesMenu);
    for( dwLoop = (DWORD)NumMenuItems; dwLoop > 0; dwLoop--) {
		cActualDrive = cDriveLetters[dwLoop-1];
		if (
			(GetMenuState(hDrivesMenu, MM_DRIVE_NUM+dwLoop, MF_BYPOSITION) & MF_GRAYED)
		    == MF_GRAYED &&
			! _chdrive(cActualDrive+1)
			)
		{
			GetPartitionInfo();
			
			if ((strstrlwr(cFileSystem, "NTFS") >= 0 || 
				 strstrlwr(cFileSystem, "FAT")  >= 0    ))
			{
				if (cBytesTotal>1073741824) sprintf(buff2, "%.1f GB", ((float)cBytesTotal)/1000/1000/1000, buff2);
									   else sprintf(buff2, "%.2f MB", ((float)cBytesTotal)/1024/1000, buff2);
				if (cBytesFree>1073741824)  sprintf(buff3, "%.1f GB", ((float)(cBytesFree-fV))/1000/1000/1000, buff3);
									   else sprintf(buff3, "%.2f MB", ((float)(cBytesFree-fV))/1024/1000, buff3);
				
				if (cBytesFree<=0) {
					//sprintf(buff, "%c: \"%s\" [%s]", cDriveLetters[dwLoop-1]+'A', cVolName, cFileSystem);
					//AddToClusterList(0, buff, -1, -1, "");
				} else {
					sprintf(buff, "%c: \"%s\" [%s]", cDriveLetters[dwLoop-1]+'A', cVolName, cFileSystem);
					sprintf(buff4, "%s (%s free)", buff2, buff3);
					AddToClusterList(-1, buff, -1, -1, buff4);

					for (i=0; i<lstrlen(wantedLetters); i++) {
						if (wantedLetters[i]==cActualDrive+65) {
							ListView_SetCheckState(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), 0, TRUE);
						}
					}
				}
			}
		}
	}

    cActualDrive = firstcActualDrive;
	GetPartitionInfo();

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    bUpdating = FALSE;
}

// read in the driveletters, type, name, size ...
BOOL UpdateDrivesMenu(HMENU hMenu) {
    HMENU          hDrivesMenu;
    int            NumMenuItems=0;
    char           DriveNo=0;
    DWORD          dwLoop;
    DWORD          dwDriveMask;
    DWORD          dwCount;
    LPTSTR         lpszRootPathName=TEXT("?:\\");
    LPTSTR         lpParse;
    static  LPTSTR lpDriveStrings = NULL;
    UINT           dType;
    int            fixedcount=0, i;
    char           firstcActualDrive = cActualDrive;
	char           buff[50], buff2[50], buff3[50];
    static char    volumeName[] = "\\\\.\\A:";
	BOOL           markForProcessing=FALSE;

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    bUpdating       = TRUE;

    // Remove list of drive menu items from Drive menu, if any.
    hDrivesMenu = GetSubMenu( hMenu, DRIVE_MENU_NUM);
    NumMenuItems = GetMenuItemCount(hDrivesMenu);
    // Delete previous menu items
    for (dwLoop = (DWORD)NumMenuItems-1; dwLoop > 2; dwLoop--)
        DeleteMenu(hDrivesMenu, dwLoop, MF_BYPOSITION);

	for (dwLoop=0; dwLoop<26; dwLoop++)
		cDriveLetters[dwLoop]=-1;

    dwCount = GetLogicalDriveStrings( 0, lpDriveStrings);
    lpDriveStrings = (LPTSTR) malloc ((dwCount + 1) * sizeof(TCHAR));
    GetLogicalDriveStrings( dwCount, lpDriveStrings);

    lpParse = lpDriveStrings;
    dwDriveMask = GetLogicalDrives();
    dwCount = 0;
    NumMenuItems = 1;
    if (bAllLocalDisks) wantedLetters[0]=0;

    while (lpParse[0]>='A' && lpParse[0]<='Z') {
      if (dwDriveMask & 1){   // drive exists - Insert
            dType = GetDriveType(lpParse);
         
			markForProcessing = FALSE;
            cActualDrive = lpParse[0]-65;
            cFileSystem[0]='\0';
            cVolName[0]='\0';

			for (i=0; i < lstrlen(wantedLetters); i++)
				if (wantedLetters[i]-65 == cActualDrive) {
					markForProcessing=TRUE;
					break;
				}

			switch (dType)
			{
				case DRIVE_CDROM:
					wsprintf(buff, "  %s (CD/DVD)", lpParse);
					InsertMenu(hDrivesMenu, NumMenuItems+2,
							   MF_STRING | MF_BYPOSITION | MF_GRAYED,
							   MM_DRIVE_NUM + NumMenuItems, buff);
					// MF_GRAYED
					break;
				case DRIVE_FIXED:
				case DRIVE_RAMDISK:
				case DRIVE_REMOTE:
				case DRIVE_REMOVABLE:
					{
						GetPartitionInfo();
						if (bAllLocalDisks && dType==DRIVE_FIXED) 
							wantedLetters[fixedcount++]=lpParse[0];
   
						// pass out NON-supported Filesystems
						if ((strstrlwr(cFileSystem, "NTFS") >= 0 || 
							 strstrlwr(cFileSystem, "FAT")  >= 0    ))
						{
							if (cBytesTotal>1073741824) sprintf(buff2, "%.1f GB", ((float)cBytesTotal)/1024/1024/1024, buff2);
												   else sprintf(buff2, "%.0f MB", ((float)cBytesTotal)/1024/1024, buff2);
							if (cBytesFree >1073741824) sprintf(buff3, "%.1f GB", ((float)(cBytesFree))/1024/1024/1024, buff3);
												   else sprintf(buff3, "%.0f MB", ((float)(cBytesFree))/1024/1024, buff3);
							if (markForProcessing)
								wsprintf(buff, "%c &%s \"%s\" [%s] %s (%s free)",
										149, lpParse, cVolName, cFileSystem, buff2, buff3);
							else
								wsprintf(buff, "   &%s \"%s\" [%s] %s (%s free)",
										lpParse, cVolName, cFileSystem, buff2, buff3);
							InsertMenu(hDrivesMenu, NumMenuItems+2,
								MF_STRING | MF_BYPOSITION | MF_ENABLED,
								MM_DRIVE_NUM + NumMenuItems, buff);

						} else {
						    //wsprintf(buff, "  %s - no medium", lpParse);
							//wsprintf(buff, "  %s - not supported", lpParse);
							wsprintf(buff, "  %s - none", lpParse);
							InsertMenu(hDrivesMenu, NumMenuItems+2,
							   MF_STRING | MF_BYPOSITION | MF_GRAYED,
							   MM_DRIVE_NUM + NumMenuItems, buff);
						}
					}
					break;
			} // switch GetDriveType

            NumMenuItems++;
            dwCount++;
            lpszRootPathName++;
            cDriveLetters[NumMenuItems-1]=DriveNo;
            lpParse += lstrlen(lpParse)+1;
      }
      DriveNo++;
      dwDriveMask >>= 1;
   }
   cActualDrive = firstcActualDrive;
   SetCursor(LoadCursor(NULL, IDC_ARROW));
   bUpdating = FALSE;

   return TRUE;
}

// Open the volume
void OpenVolume( void ) {
    static char volumeName[] = "\\\\.\\A:";
    char buff[40];
    HMENU hsub;
    int numflushed;

    // needed to make a new bitmap
    bw         = 0;
    bh         = 0;
    bAnalyzed  = FALSE;
    bDumped    = FALSE;

    if( ghVolume && ghVolume != INVALID_HANDLE_VALUE )
        CloseHandle(ghVolume);

    numflushed = _flushall();

    // open the volume
    volumeName[4] = cActualDrive + 'A';
    ghVolume = CreateFile(
        volumeName, GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, 
        FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_NO_BUFFERING, 
        NULL );

    if( ghVolume==INVALID_HANDLE_VALUE ) {
      HandleWin32Error(GetLastError(), "on DiskOpen!");   
      return;
    }

    hsub = GetSubMenu(hMenu, 2);
    DeleteMenu(hsub, IDM_DUMPTOFILE, MF_BYCOMMAND);

    sprintf(buff, "E&xport dirlist to %c:\\defragger.txt", cActualDrive+65);
    InsertMenu(hsub, IDM_DUMPTOFILE, MF_STRING | MF_BYCOMMAND | MF_ENABLED, IDM_DUMPTOFILE, buff);

    GetPartitionInfo();
    GetPartitionInfoDetails();

    return;
}

// general info about this volume (volume is not open)
void GetPartitionInfo(void) {
    ULARGE_INTEGER freeBytesAvail, totalBytes, freeBytes;
    DWORD dwSectorsPerCluster, dwBytesPerSector, dwFreeClusters, dwTotalClusters;
    char buff[30];

    buff[0]=cActualDrive + 'A'; buff[1]=':'; buff[2]='\\'; buff[3]='\0';

    if (GetVolumeInformation((LPCTSTR)buff, (LPTSTR)cVolName, 32, NULL, NULL, NULL,
                         (LPTSTR)cFileSystem, 16)) 
	{
	    if (lstrlen(cVolName)==0) sprintf(cVolName, "(no name)");

		// determine free space and total capacity
		GetDiskFreeSpaceEx((LPCTSTR)buff, &freeBytesAvail, &totalBytes, &freeBytes);
		cBytesFree   = freeBytes.QuadPart;
		cBytesTotal  = totalBytes.QuadPart;
		cBytesUsed   = cBytesTotal-cBytesFree;

		GetDiskFreeSpace((LPCTSTR)buff, &dwSectorsPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwTotalClusters);
		cDiskClusters = (ULONGLONG)dwTotalClusters;
		cFreeClusters = (ULONGLONG)dwFreeClusters;
		cSectorsPerCluster = dwSectorsPerCluster;
		cBytesPerCluster = dwBytesPerSector * dwSectorsPerCluster;
	}
   
	UpdateStatusBar("not analyzed", 4, 0);
	ResetStatusBar();
}

// special info about volume (volume is open)
void GetPartitionInfoDetails(void) {
    char buff[60], buff2[60], buff3[60];
    DWORD lpBytesReturned;
    STORAGE_DEVICE_NUMBER sdn;
    __int64 fV=0;

    buff2[0]=0; buff3[0]=0;

    RtlZeroMemory(&sdn, sizeof(STORAGE_DEVICE_NUMBER));
    RtlZeroMemory(&gPARTINFO, sizeof(PARTITION_INFORMATION));
    RtlZeroMemory(&gDISKGEOM, sizeof(DISK_GEOMETRY));
    RtlZeroMemory(&gNTFSDATA, sizeof(NTFS_VOLUME_DATA_BUFFER));

    if (DeviceIoControl (ghVolume, 
      IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, 
      &sdn, sizeof (STORAGE_DEVICE_NUMBER), 
      &lpBytesReturned, NULL)==0)
      HandleWin32Error(GetLastError(), "GET_DEVICE_NUMBER failed.");

    cDiskNumber = sdn.DeviceNumber;

    if (DeviceIoControl(ghVolume,
        IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
        &gDISKGEOM, sizeof(DISK_GEOMETRY),
        &lpBytesReturned, NULL)==0)
        HandleWin32Error(GetLastError(), "GET_DISK_GEOMETRY failed.");

    if (DeviceIoControl(ghVolume,
        IOCTL_DISK_GET_PARTITION_INFO, NULL, 0,
        &gPARTINFO, sizeof(PARTITION_INFORMATION)+4,
        &lpBytesReturned, NULL)==0)
        HandleWin32Error(GetLastError(), "GET_PARTITION_INFO failed.");

    cBytesSystem = gPARTINFO.PartitionLength.QuadPart;

    if (gPARTINFO.PartitionType==7 ) {
        if (DeviceIoControl(ghVolume,
            FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0,
            &gNTFSDATA, sizeof(NTFS_VOLUME_DATA_BUFFER),
            &lpBytesReturned, NULL)==0)
            HandleWin32Error(GetLastError(), "GET_NTFS_VOLUME_DATA failed.");

      // nothing I need
      /*if (DeviceIoControl(ghVolume,
         FSCTL_FILESYSTEM_GET_STATISTICS, NULL, 0, 
         &gFSSTATS, sizeof(FILESYSTEM_STATISTICS) + sizeof(NTFS_STATISTICS),
         &lpBytesReturned, NULL)==0)
         HandleWin32Error(GetLastError(), "getfsstats");
      */
    }

    if (gPARTINFO.PartitionType==7) 
      fV = (gNTFSDATA.MftZoneEnd.QuadPart-gNTFSDATA.MftZoneStart.QuadPart)*
            gNTFSDATA.BytesPerCluster+gNTFSDATA.MftValidDataLength.QuadPart;
   
    if (cBytesSystem>1073741824) sprintf(buff2, "%.1f GB", ((float)cBytesSystem)/1024/1024/1024);
                           else sprintf(buff2, "%.0f MB", ((float)cBytesSystem)/1024/1024);
      
    if (cBytesFree>1073741824) sprintf(buff3, "%.1f GB", ((float)(cBytesFree-fV))/1024/1024/1024);
                         else sprintf(buff3, "%.0f MB", ((float)(cBytesFree-fV))/1024/1024);

	//sprintf(buff, "%c: [%s] \"%s\" %s (%s free)", cActualDrive+65, cFileSystem, cVolName, buff2, buff3);
    sprintf(buff, "%c: [%s] %s ", cActualDrive+65, cFileSystem, buff2);
    UpdateStatusBar(buff, 0, 0);
}

// process msg for about
LRESULT CALLBACK AboutProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch( message )
    {
        //case WM_ERASEBKGND:
        //case WM_SHOWWINDOW:
        case WM_ACTIVATE:
        case WM_MOVE:
            {
            HDC hdcDlg;

            hdcDlg = GetDC(hDlg);
            SelectObject(hdcDlg, hpBlack); // pen for all blocks

            // left col
            DrawIconEx(hdcDlg, 15, 17, hiDir, 16, 16, 0, hbBG, 0);
            DrawIconEx(hdcDlg, 15, 30, hiFile, 16, 16, 0, hbBG, 0);
            SelectObject(hdcDlg, hbDGreen);
            Rectangle(hdcDlg, 19, 47, 19+8, 47+8); //102
            DrawIconEx(hdcDlg, 15, 56, hiFrag, 16, 16, 0, hbBG, 0);
            SelectObject(hdcDlg, hbBrown);
            Rectangle(hdcDlg, 19, 73, 19+8, 73+8);

            // right col
            DrawIconEx(hdcDlg, 157, 17, hiMFT, 16, 16, 0, hbBG, 0);
            DrawIconEx(hdcDlg, 157, 30, hiComp, 16, 16, 0, hbBG, 0);
            SelectObject(hdcDlg, hbWhite);
            Rectangle(hdcDlg, 161, 47, 161+8, 47+8);
            SelectObject(hdcDlg, hbL2Blue);
            Rectangle(hdcDlg, 161, 60, 161+8, 60+8);
            SelectObject(hdcDlg, hbLYellow);
            Rectangle(hdcDlg, 161, 73, 161+8, 73+8);

            ReleaseDC(ghWnd, hdcDlg);

         {
            const ticksperday = 1000 * 60 * 60 * 24;
            const ticksperhour = 1000 * 60 * 60;
            const ticksperminute = 1000 * 60;
            const tickspersecond = 1000;
            char buff[50];
            long t;
            int d, h, m, s;
            t = GetTickCount();

            d = (int)(t / ticksperday);
            t = t - d * ticksperday;
            h = (int)(t / ticksperhour);
            t = t - h * ticksperhour;
            m = (int)(t / ticksperminute);
            t = t - m * ticksperminute;
            s = (int)(t / tickspersecond);

            sprintf(buff, "Windows uptime: %dd, %dh, %dm, %ds", d, h, m, s);
            SetDlgItemText(hDlg, IDC_UPTIME, buff);
         }

            return TRUE;
            }
        case WM_INITDIALOG:
            {
            SetDlgItemText(hDlg, IDC_LINK, "free.pages.at/blumetools/");
            SetDlgItemText(hDlg, IDC_OS, cOSVersion);
            ShowWindow(hDlg, SW_SHOW);
            return TRUE;
            }
        case WM_COMMAND:
            if (LOWORD(wParam)==IDOK || LOWORD(wParam)==IDCANCEL)
            {
                DestroyWindow(hDlg);
                //EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}

// process msg for clusterviewer
LRESULT CALLBACK ClusterViewProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    unsigned int event=0;
    LPNMHDR lpnmh;

    switch( message ) {
    case WM_NOTIFY:

        lpnmh = (LPNMHDR) lParam;
        event = lpnmh->code;

        switch (event) {
        case NM_CLICK:
        case NM_RCLICK:
			{
				int    index;
				char   buff[512];
				struct FILEstruct * Here=NULL;
				HMENU hsub;
				POINT pt;
				BOOL enableBtns = FALSE;
				char wndCap[512];
				GetWindowText(ghListWnd, wndCap, 512);

				GetCursorPos(&pt);
				index = ListView_GetNextItem(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), -1, LVNI_SELECTED);

				if (strstrlwr(wndCap, "Select Drives") >= 0)
					break;

				if (index >= 0) {
					ListView_GetItemText(GetDlgItem(ghListWnd, IDC_CLUSTERLIST),
										 index, 0, buff, 512);

					if (strstrlwr(buff, ":\\") > 0) {
						bUpdating = TRUE;
						Here = topdirlist;
						currentfile = NULL;
						while (Here) {
							if (strcmp(Here->fileName, buff)==0) {
								// mark fragments
								if (Here->fragid>=0 && Here->fragments<=1 && !Here->free)
									DrawBlockBounds(Here->Lcn, Here->Lcn+Here->Len, FALSE);
								// remember main dirlist entry
								if (Here->fragid<=1 &&  Here->fragments>=1)
									currentfile = Here;
							}
							Here = Here->next;
						}
						lstrcpy(cLastSelected, buff);

						bUpdating = FALSE;
						lstrcpy(cLastSelected, buff);
						if (!ghDefrag) enableBtns = TRUE;
					} else
						ClearLastSelectedFile();

					if ((ghDefrag && !enableBtns) || (currentfile && currentfile->free))
						ClearLastSelectedFile();

					if (enableBtns && currentfile) {
						EnableMenuItem(hFileMenu, IDM_DEFRAGTHIS, MF_BYCOMMAND);
						EnableMenuItem(hFileMenu, IDM_BROWSE, MF_BYCOMMAND);
						EnableWindow(GetDlgItem(ghListWnd, IDC_DEFRAG), !currentfile->locked | !currentfile->free);
						EnableWindow(GetDlgItem(ghListWnd, IDC_BROWSE), TRUE);
					} else {
						EnableMenuItem(hFileMenu, IDM_DEFRAGTHIS, MF_BYCOMMAND|MF_GRAYED);
						EnableMenuItem(hFileMenu, IDM_BROWSE, MF_BYCOMMAND|MF_GRAYED);
						EnableWindow(GetDlgItem(ghListWnd, IDC_DEFRAG), FALSE);
						EnableWindow(GetDlgItem(ghListWnd, IDC_BROWSE), FALSE);
					}

					if (event==NM_RCLICK) {
						hsub = GetSubMenu(hFileMenu, 0);
						TrackPopupMenu(hsub, 0, pt.x, pt.y, 0, ghWnd, NULL);
					}
				} else {
					ClearLastSelectedFile();
				}

			} 
			break;
        default:
            ;
        }
        break;

    case WM_SYSCOMMAND:
		if ((wParam & 0xfff0)==SC_CLOSE) {
			char wndCap[512];
			GetWindowText(ghListWnd, wndCap, 512);
			if (strstrlwr(wndCap, "Select Drives") >= 0) {
				SendMessage(ghListWnd, WM_COMMAND, IDC_DEFRAG, 0);
				return FALSE;
			}
		}
		break;
	case WM_HSCROLL: {
        char Text[512];
        ULONGLONG Cstart=0;

        // mouseclick generates a second WM_HSCROLL, so skip
        if (LOWORD(wParam)==8) break;

		GetWindowText(GetDlgItem(hDlg, IDC_CLUSTERNO), (LPTSTR)Text, 512);
		sscanf(Text, "%I64u", &Cstart);
        if (LOWORD(wParam)==0)
            if (Cstart >= clustersPB) {
                Cstart -= clustersPB;
            } else return TRUE;

        if (LOWORD(wParam)==1)
            if (Cstart+clustersPB <= cDiskClusters) {
                Cstart += clustersPB;
            } else return TRUE;

        sprintf(Text, "%I64u", Cstart);
        SetWindowText(GetDlgItem(hDlg, IDC_CLUSTERNO), Text);

        (void*)_beginthread(UpdateClusterList, 0, NULL);
        //UpdateClusterList();

        return TRUE;
    }
    case WM_SIZE: {
        RECT rcl; // Rectangle for setting the size of the window.
        int w=0, h=0;
        GetClientRect(hDlg, &rcl);
        w = rcl.right - rcl.left;
        h = rcl.bottom - rcl.top;
        MoveWindow(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), 2, 26, w - 4, h - 28, TRUE);
    }
    break;
    case WM_COMMAND:
    {
        int wmId, wmEvent;
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        if (wmId==0x1B)
            wmId=0x1B;

        switch (wmId) {
         case IDC_CLUSTERLIST:
                ShowWindow(ghListWnd, SW_HIDE);
            break;
         case IDC_DEFRAG:
			 {
				char wndCap[512];
				GetWindowText(ghListWnd, wndCap, 512);
				if (strstrlwr(wndCap, "Select Drives") >= 0) 
				{
					char buff[512];
					int i=0, l=0;
					int items=0;
					
					ShowWindow(ghListWnd, SW_HIDE);

					wantedLetters[0]=0;
					items = ListView_GetItemCount(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));
					for (i=0; i<items; i++) {
						if (ListView_GetCheckState(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), i)) {
							ListView_GetItemText(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), i, 0, buff, 512);
							wantedLetters[l++]=buff[0];
						}
					}
					wantedLetters[l]=0;
					UpdateDrivesMenu(hMenu);
					if (wantedLetters[0]==0)
						CheckMenuItem(hMenu, MM_DRIVE_NUM+cActualDrive, MF_CHECKED);
					else
						SendMessage(ghWnd, WM_COMMAND, MM_DRIVE_NUM+wantedLetters[0]-65, 0);   // firstt selected volume
				} else
					SendMessage(ghWnd, WM_COMMAND, IDM_DEFRAGTHIS, 0);
			 }
             break;
            case IDC_BROWSE:
                SendMessage(ghWnd, WM_COMMAND, IDM_BROWSE, 0);
                break;

            case IDC_FIND:
				FindFile();
                return TRUE;

            case IDC_CLUSTERNO:
                if (wParam==VK_ESCAPE)
                    ShowWindow(ghListWnd, SW_HIDE);
                return TRUE;
            case IDC_FILENAME:
                if (wParam==VK_ESCAPE)
                    ShowWindow(ghListWnd, SW_HIDE);
                return TRUE;

            case IDCANCEL: case VK_ESCAPE:
                ClearLastSelectedFile();
                DrawNoBlockBound(xposlast, yposlast);
                ShowWindow(ghListWnd, SW_HIDE);
                return TRUE;

            case IDM_ESCAPE:
                DrawNoBlockBound(xposlast, yposlast);
                ShowWindow(ghListWnd, SW_HIDE);
                return TRUE;

            default:
                break;
        }
        return TRUE;
    } // case WM_COMMAND
    case WM_USER:
        switch (wParam) {
			case WM_UPDATEDRIVELIST:
				UpdateDriveList();
				break;
			case WM_UPDATEDRIVEINFO:
                UpdateDriveInformation();
                break;
            case WM_UPDATECLUSTERLIST:
            {
                if (lParam >= 0) { // coming from Menu, <0 if not
                    long xpos, ypos;
                    xpos = (int)LOWORD(lParam);  // horizontal position of cursor
                    ypos = (int)HIWORD(lParam);  // vertical position of cursor

                    if (CalcClusterNo(&xpos, &ypos))
						(void*)_beginthread(UpdateClusterList, 0, NULL);
                } else {
					(void*)_beginthread(UpdateClusterList, 0, NULL);
                    //UpdateClusterList();
                    // set dialogpos if block under dialog does not work
                    //RECT rt;
                    //int w=0, h=0;
                    //GetWindowRect(hDlg, &rt);
                    //w = rt.right - rt.left;
                    //h = rt.bottom - rt.top;

                    //if (yposlast > rt.top && yposlast < rt.bottom) {
                    //    MoveWindow(hDlg, rt.left, 100+xposlast+blocksize, w, h, TRUE);
                    //}
                    //if (imgHeight+23<rt.bottom) {
                    //}

                }
                break;
            }
        }
        return TRUE;
   }
   return FALSE;
}

// ..
void UpdateDriveInformation(void) {
    char buff[512], buff2[512], buff3[512], buff4[512];
    int img=-1;
    struct FILEstruct *Here=NULL, *merk=NULL, *greatestGap=NULL;
    __int64 fV=0;
    int updatecounter=1;

    long lStyle;

	lStyle = SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	lStyle = lStyle & !LVS_EX_CHECKBOXES;
	SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lStyle);

    lStyle = SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    lStyle = lStyle | LVS_EX_FULLROWSELECT;
    SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lStyle);

	ShowWindow(GetDlgItem(ghListWnd, IDC_STATIC0), SW_SHOW);
	ShowWindow(GetDlgItem(ghListWnd, IDC_FILENAME), SW_SHOW);
	ShowWindow(GetDlgItem(ghListWnd, IDC_CLUSTERNO), SW_SHOW);
	ShowWindow(GetDlgItem(ghListWnd, IDC_SCROLL), SW_SHOW);
	ShowWindow(GetDlgItem(ghListWnd, IDC_FIND), SW_SHOW);
	ShowWindow(GetDlgItem(ghListWnd, IDC_BROWSE), SW_SHOW);
	EnableWindow(GetDlgItem(ghListWnd, IDC_DEFRAG), FALSE);
	SetWindowText(GetDlgItem(ghListWnd, IDC_DEFRAG), "defrag");

    ListView_DeleteAllItems(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));
    SetWindowText(ghListWnd, "Drive Information");

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    bUpdating       = TRUE;
    sprintf(buff2, "");

    ClearLastSelectedFile();

    if (ghAnalyze || ghDefrag) {
        sprintf(buff3, "[not uptodate] Drive Information");
        SetWindowText(ghListWnd, buff3);
		Here = topdirlist;
        //Here = merk = SortDict(CopyDIR(topdirlist), listlen, byFragments);
        //Here = merk = SortDict(CopyDIR(topdirlist), listlen, byName);
    } else {
        sprintf(buff3, "Drive Information");
        SetWindowText(ghListWnd, buff3);
        //Here = topdirlist = SortDict(topdirlist, listlen, byFragments);
        Here = topdirlist = SortDict(topdirlist, listlen, byName);
    }
   
    // files above sizelimit
	Here = topdirlist;
    while (Here) {
        sprintf(buff, "");
        sprintf(buff3, "");
        if (Here->filesize > sizeLimit &&
			Here->fragid<=1            && 
			Here->fragments>=1         &&
            !Here->locked              &&
			!Here->sparse              &&
            !Here->fragmented          && 
            !Here->free                   ) 
		{
            GetFileInfo(Here, buff, &img);
			AddToClusterList(img, Here->fileName, Here->Lcn, Here->Len, buff);
        }

		if (updatecounter++ % 100==0) {
		    UpdateWindow(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));
		    UpdateWindow(ghListWnd);
		    RedrawWindow(ghListWnd, NULL, NULL, RDW_UPDATENOW);
		    Sleep(0);
		}
        Here = Here->next;
    }
    AddToClusterList(-1, "--- files above sizelimit ---", LLINVALID, LLINVALID, "---");

    // locked files
	Here = topdirlist;
    while (Here) {
        sprintf(buff, "");
        sprintf(buff3, "");
        if (Here->locked               &&
            Here->fragid<=1            &&
			Here->fragments>=1
			/*&& Here->fragid<=1 && Here->fragments<=1*/)
		{
            GetFileInfo(Here, buff, &img);
			AddToClusterList(img, Here->fileName, Here->Lcn, Here->Len, buff);
        }

        if (updatecounter++ % 100==0) {
            UpdateWindow(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));
            UpdateWindow(ghListWnd);
            RedrawWindow(ghListWnd, NULL, NULL, RDW_UPDATENOW);
            Sleep(0);
		}
        Here = Here->next;
    }
    AddToClusterList(-1, "--- locked files ---", LLINVALID, LLINVALID, "---");

    // fragmented files
    Here = topdirlist = SortDict(topdirlist, listlen, byFragments);
    while (Here) {
        sprintf(buff, "");
        sprintf(buff3, "");
        if (Here->fragmented           && 
			Here->fragid<=1            && 
			Here->fragments>=1
		   )
		{
            GetFileInfo(Here, buff, &img);
			AddToClusterList(img, Here->fileName, Here->Lcn, Here->Len, buff);
		}

        if (updatecounter++ % 100==0) {
            UpdateWindow(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));
            UpdateWindow(ghListWnd);
            RedrawWindow(ghListWnd, NULL, NULL, RDW_UPDATENOW);
            Sleep(0);
		}
        Here = Here->next;
    }
    AddToClusterList(-1, "--- fragmented files ---", LLINVALID, LLINVALID, "---");

    // system-files
    Here = topdirlist = SortDict(topdirlist, listlen, byName);
	if (gPARTINFO.PartitionType == 7)
    while (Here) {
        sprintf(buff, "");
        sprintf(buff3, "");
        if ( (Here->MFTid > 1 && 
			  Here->MFTid < 27 &&
			  Here->fragid<=1            && 
			  Here->fragments>=1         &&
			  gPARTINFO.PartitionType == 7                     ) || 
			 (gPARTINFO.PartitionType != 7 && Here->locked) &&
			  !Here->free                                      )
		{
			GetFileInfo(Here, buff, &img);
			AddToClusterList(img, Here->fileName, Here->Lcn, Here->Len, buff);
        }

        if (updatecounter++ % 100==0) {
            UpdateWindow(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));
            UpdateWindow(ghListWnd);
            RedrawWindow(ghListWnd, NULL, NULL, RDW_UPDATENOW);
            Sleep(0);
		}
        Here = Here->next;
    }
    if (gPARTINFO.PartitionType == 7) 
    AddToClusterList(-1, "--- system files ---", LLINVALID, LLINVALID, "---");

    // MFT-stuff
	Here = topdirlist;
	if (gPARTINFO.PartitionType == 7)
    while (Here) {
        sprintf(buff, "");
        sprintf(buff3, "");
        if ((Here->MFTid < 2 && Here->MFTid!=LLINVALID &&
			 gPARTINFO.PartitionType == 7                 ) &&    // 
            Here->fragid<=1                                 &&
			Here->fragments>=1                              &&
			! Here->free                                       )
		{
            GetFileInfo(Here, buff, &img);
			AddToClusterList(img, Here->fileName, Here->Lcn, Here->Len, buff);
        }

        if (updatecounter++ % 100==0) {
            UpdateWindow(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));
            UpdateWindow(ghListWnd);
            RedrawWindow(ghListWnd, NULL, NULL, RDW_UPDATENOW);
            Sleep(0);
		}
        Here = Here->next;
    }
    if (gPARTINFO.PartitionType == 7) 
		AddToClusterList(-1, "--- MFT files ---", LLINVALID, LLINVALID, "---");

    if (bAnalyzed) {
        sprintf(buff4, "%s dirs + %s files = %s objects / %d fragmented",
            fmtNumber(cDirs, buff), fmtNumber(cFiles, buff2), fmtNumber(cFiles+cDirs, buff3), cFragmented);
        sprintf(buff, "%d locked / %d compressed / %d fragments at all",
            cLocked, cCompressed, cFragmentsAllTogether);
    } else sprintf(buff4, "drive not analyzed");
    AddToClusterList(-1, buff4, LLINVALID, LLINVALID, buff);

    if (gPARTINFO.PartitionType==7) {
    fV = (gNTFSDATA.MftZoneEnd.QuadPart-gNTFSDATA.MftZoneStart.QuadPart)*
          gNTFSDATA.BytesPerCluster;

    sprintf(buff3, "%s bytes actual free", fmtNumber(cBytesFree-(fV+gNTFSDATA.MftValidDataLength.QuadPart), buff));
    sprintf(buff, "%s MB", fmtNumber((cBytesFree-(fV+gNTFSDATA.MftValidDataLength.QuadPart))/1024/1024, buff));
    sprintf(buff, "%s / %.1f GB (%.1f%%)", buff, 
      (float)(cBytesFree-fV+gNTFSDATA.MftValidDataLength.QuadPart)/1024/1024/1024,
      (float)(cBytesFree-((float)fV+gNTFSDATA.MftValidDataLength.QuadPart))/(float)(cBytesTotal+1.0)*100.0);
    AddToClusterList(-1, buff3, LLINVALID,
        cFreeClusters-(gNTFSDATA.MftZoneEnd.QuadPart-gNTFSDATA.MftZoneStart.QuadPart), buff);

    sprintf(buff3, "%s bytes reserved for MFT", fmtNumber((fV+gNTFSDATA.MftValidDataLength.QuadPart), buff));
    sprintf(buff, "%s MB", fmtNumber((fV+gNTFSDATA.MftValidDataLength.QuadPart)/1024/1024, buff));
    sprintf(buff, "%s / %.1f GB (%.1f%%)", buff, 
      (float)(fV+gNTFSDATA.MftValidDataLength.QuadPart)/1024/1024/1024,
        ((float)(fV+gNTFSDATA.MftValidDataLength.QuadPart)/((float)cBytesTotal+1.0))*100.0);
    AddToClusterList(-1, buff3, LLINVALID, gNTFSDATA.MftZoneEnd.QuadPart-gNTFSDATA.MftZoneStart.QuadPart, buff);
    }

	Here = topdirlist;
    while (Here) {
        if (Here->free)
			if (greatestGap==NULL || greatestGap->Len < Here->Len)
				greatestGap=Here;
        Here = Here->next;
    }

    if (greatestGap) {
	sprintf(buff, "largest free block");
    sprintf(buff4, "%s byte", fmtNumber(greatestGap->filesize, buff4));
    AddToClusterList(-1, buff, greatestGap->Lcn, greatestGap->Len, buff4);
	}

    sprintf(buff3, "%s bytes free", fmtNumber(cBytesFree, buff));
    sprintf(buff, "%s MB", fmtNumber(cBytesFree/1024/1024, buff));
    sprintf(buff, "%s MB / %.1f GB (%.1f%%)", buff, 
      (float)cBytesFree/1024/1024/1024,
        ((float)cBytesFree/((float)cBytesTotal+1.0))*100.0);
    AddToClusterList(-1, buff3, LLINVALID, LLINVALID, buff);

    sprintf(buff3, "%s bytes used", fmtNumber(cBytesUsed, buff));
    sprintf(buff, "%s MB", fmtNumber(cBytesUsed/1024/1024, buff));
    sprintf(buff, "%s / %.1f GB (%.1f%%)", buff,
      (float)cBytesUsed/1024/1024/1024,
        ((float)cBytesUsed/((float)cBytesTotal+1.0))*100.0);
    AddToClusterList(-1, buff3, LLINVALID, (cDiskClusters-cFreeClusters), buff);

    sprintf(buff3, "%s bytes in this partition"  , fmtNumber(cBytesSystem, buff));
    sprintf(buff, "%s bytes available to system", fmtNumber(cBytesTotal, buff));
    AddToClusterList(-1, buff3, LLINVALID, LLINVALID, buff);

    sprintf(buff3, "StartSector %I64u, SectorsTotal %I64u, %d BytesPerSector", 
		(ULONGLONG)(gPARTINFO.StartingOffset.QuadPart/cBytesPerCluster*cSectorsPerCluster),
		(ULONGLONG)(cDiskClusters*cSectorsPerCluster),
		gDISKGEOM.BytesPerSector);
	sprintf(buff, "%I64u TotalClusters, %I64u FreeClusters, %d BytesPerCluster", cDiskClusters, cFreeClusters, cBytesPerCluster);
    AddToClusterList(-1, buff3, LLINVALID, LLINVALID, buff);

    sprintf(buff3, "Disk #%d, Partition #%d, %I64u Cylinders", cDiskNumber, gPARTINFO.PartitionNumber, gDISKGEOM.Cylinders.QuadPart);
    sprintf(buff, "%d TracksPerCylinder, %d SectorsPerTrack", gDISKGEOM.TracksPerCylinder, gDISKGEOM.SectorsPerTrack);
	AddToClusterList(-1, buff3, LLINVALID, LLINVALID, buff);

	sprintf(buff3, "--- Statistic for drive %c:\\ [%s] ---", cActualDrive+65, cFileSystem);
    sprintf(buff, "VolumeName: \"%s\"", cVolName);
    AddToClusterList(-1, buff3, LLINVALID, LLINVALID, buff);

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    bUpdating = FALSE;

    // delete the copied list
    if (merk) DelDIR(merk);
}

// ..
void AddToClusterList(int img, char *name, ULONGLONG Lcn,
                      ULONGLONG Len, char *status) {
    LV_ITEM lvI;
    char szText[512];

    // add the item to the window
    lvI.mask = LVIF_NORECOMPUTE | LVIF_TEXT | LVIF_IMAGE | LVIF_STATE; // | LVIF_PARAM;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.cchTextMax = 512;
    lvI.iImage = img;
    lvI.lParam = 0;
    lvI.iItem = 0;
    lvI.iSubItem = 0;
    lvI.pszText = szText;

    sprintf(szText, "%s", name);
    ListView_InsertItem(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), &lvI);

    if (Lcn==LLINVALID) sprintf(szText, "-");
    else                sprintf(szText, "%I64u", Lcn);
    ListView_SetItemText(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), 0, 1, szText);

    if (Len==LLINVALID) sprintf(szText, "-", Len);
    else                 sprintf(szText, "%I64u", Len);
    ListView_SetItemText(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), 0, 2, szText);

    sprintf(szText, "%s", status);
    ListView_SetItemText(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), 0, 3, szText);
}

void GetFileInfo(struct FILEstruct *Here, char *Text2, int *img) {
	char Text[255], Text3[255];

	if (!Here->free)
	if (Here->fragid<=1 && Here->fragments>=1) {
	    if (Here->fragmented) {
		    sprintf(Text, "%I64u frags ", Here->fragments);
		    lstrcat(Text2, Text); 
			if (Here->filesize!=IINVALID) lstrcat(Text2, "- "); 
		}
	} else if (Here->fragid>=1 && Here->fragments<=1) {
	    sprintf(Text, "frag #%I64u ", Here->fragid);
	    lstrcat(Text2, Text); 
		if (Here->filesize!=IINVALID) lstrcat(Text2, "- "); 
	}

    if (Here->MFTid<2 && Here->MFTid!=LLINVALID)*img = 4; //mft
    else if (Here->locked)                      *img = 6; //locked
    else if (Here->filesize==IINVALID && 
			!Here->fragmented)                  *img = 0; //dir
    else if (Here->compressed)                  *img = 3; //compressed
    else if (Here->free)                        *img = 5; //free
    else if (Here->fragmented)                  *img = 2; //fragmented
    else                                        *img = 1; //normal file

	if (Here->locked) {
	  sprintf(Text, "(locked) - ");
	  lstrcat(Text2, Text); }
	if (Here->sparse) {
	  sprintf(Text, "(sparse) - ");
	  lstrcat(Text2, Text); }
	if (Here->compressed) {
	  sprintf(Text, "(compressed) - ");
	  lstrcat(Text2, Text); }
	if (Here->filesize != IINVALID) {
	  if (Here->fragmented && Here->fragid>=1 && Here->fragments==0 && !Here->sparse)
	    sprintf(Text, "%s byte (of %s byte) ", 
		  fmtNumber(Here->Len*cBytesPerCluster, Text), 
		  fmtNumber(Here->filesize, Text3));
	  else
	    sprintf(Text, "%s byte ", fmtNumber(Here->filesize, Text));
      lstrcat(Text2, Text); }
	if (Here->filesize > sizeLimit &&
	  !Here->free) {
	  sprintf(Text, "[above sizelimit] ");
	  lstrcat(Text2, Text); }
	if (Here->MFTid>=0) {
	  sprintf(Text, "[mft# %I64u] ", Here->MFTid);
	  lstrcat(Text2, Text); }
}
         
void UpdateClusterList(void *d) {
    struct FILEstruct *Here=NULL;
    ULONGLONG Cstart=0, Cend=0, sLcn=0, sLen=0, lastLcn=0, lastLen=0;
    BOOL isfree=FALSE;
    int img=0;
    int xpos=0, ypos=0;
    unsigned long updatecounter=0;
    char Text[512], Text2[512];

    long lStyle;

	lStyle = SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	lStyle = lStyle & !LVS_EX_CHECKBOXES;
	SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lStyle);

    lStyle = SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    lStyle = lStyle | LVS_EX_FULLROWSELECT;
    SendMessage(GetDlgItem(ghListWnd, IDC_CLUSTERLIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lStyle);

	ShowWindow(GetDlgItem(ghListWnd, IDC_STATIC0), SW_SHOW);
	ShowWindow(GetDlgItem(ghListWnd, IDC_FILENAME), SW_SHOW);
	ShowWindow(GetDlgItem(ghListWnd, IDC_CLUSTERNO), SW_SHOW);
	ShowWindow(GetDlgItem(ghListWnd, IDC_SCROLL), SW_SHOW);
	ShowWindow(GetDlgItem(ghListWnd, IDC_FIND), SW_SHOW);
	ShowWindow(GetDlgItem(ghListWnd, IDC_BROWSE), SW_SHOW);
	EnableWindow(GetDlgItem(ghListWnd, IDC_DEFRAG), FALSE);
	SetWindowText(GetDlgItem(ghListWnd, IDC_DEFRAG), "defrag");

	ShowWindow(ghListWnd, SW_SHOW);

    if (bUpdating) return;
    bUpdating       = TRUE;

    // make sure the value in editfield is correct
    GetWindowText(GetDlgItem(ghListWnd, IDC_CLUSTERNO), (LPTSTR)Text, 512);
    sscanf(Text, "%I64u", &Cend);

    // calc xpos and ypos
    while (Cstart < Cend) {
        Cstart += clustersPB;
        xpos += blocksize;
        if (xpos>imgWidth-blocksize) {
            xpos=0;
            ypos+=blocksize;
        }
    }

    if (Cstart >= cDiskClusters) {
        Cstart = cDiskClusters-1;
        xpos -= blocksize;
        if (xpos<0) {
            xpos=(int)(clustersPL)/blocksize;
            ypos-=blocksize;
        }
    }
    Cend = Cstart + clustersPB;
    if (Cend >= cDiskClusters)
        Cend = cDiskClusters;

    sprintf(Text, "%I64u", Cstart);
    SetWindowText(GetDlgItem(ghListWnd, IDC_CLUSTERNO), Text);

    DrawNoBlockBound(xposlast, yposlast);
    xposlast=xpos; yposlast=ypos; // remember last block coords
    DrawBlockBound(xposlast, yposlast);

    ListView_DeleteAllItems(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));
    UpdateWindow(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));

    if (!dirlist) {
        sprintf(Text, "--- not available ---");
        AddToClusterList(-1, Text, LLINVALID, LLINVALID, "");
        return;
    }

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    ClearLastSelectedFile();

    // need the dirlist in REVERSE order for proper list-insert
    // make a copy of the list if something is running
    if (ghAnalyze || ghDefrag) {
        sprintf(Text, "[not uptodate] ClusterViewer  -  %I64u / %I64u clusters",
                Cend - Cstart, cDiskClusters);
        Here = topdirlist;
    } else {
        sprintf(Text, "ClusterViewer  -  %I64u / %I64u clusters",
                Cend - Cstart, cDiskClusters);
		Here = topdirlist = SortDict(topdirlist, listlen, byLcnDesc);
        //Here = topdirlist = SortDict(topdirlist, listlen, byLcnAsc);
		//Here = topdirlist = SortDict(CopyDIR(topdirlist), listlen, byLcnDesc);
        //Here = topdirlist = SortDict(CopyDIR(topdirlist), listlen, byLcnAsc);
    }
    SetWindowText(ghListWnd, Text);

    lastLcn = Cend;
    lastLen = clustersPB;
    sLcn = Cend;
    sLen = 0;

    while (Here) {
		if (Here &&
			Here->Lcn + Here->Len+1 >= Cstart       &&
            Here->Lcn               <  Cend         &&
            Here->Len >= 0                          &&
            Here->Len != LLINVALID                  &&
			Here->Lcn != LLINVALID                  &&
			!(Here->sparse && Here->Lcn==0)         &&
			( Here->fragments<=1 || Here->free || Here->fragid>=1)// &&
           )
        {
			// set Lcn && Len of this pair
			if        (Here->Lcn >= Cstart && Here->Lcn + Here->Len >= Cend
				&& Here->Lcn <= Cend) {
			    // start in, end out
				sLen = Cend - Here->Lcn;
				sLcn = Here->Lcn;
				if (sLen==0) sLen=1;
			} else if (Here->Lcn < Cstart && Here->Lcn + Here->Len <= Cend) {
				// start out, end in
				sLen = (Here->Lcn + Here->Len) - Cstart;
				sLcn = Cstart;
			} else if (Here->Lcn + Here->Len < Cend && Here->Lcn > Cstart) {
				// start in, end in
				sLen = Here->Len;
				sLcn = Here->Lcn;
			} else if (Here->Lcn < Cstart && Here->Lcn + Here->Len > Cend) {
				// start out, end out
				sLen = clustersPB; //Cend - Cstart;
				sLcn = Cstart;
			} else {
				sLcn = Here->Lcn;
				sLen = Here->Len;
			}

            // check if there is a hole
            if (Here->Lcn+Here->Len < lastLcn                &&
				lastLcn - (Here->Lcn+Here->Len) < clustersPB &&
				sLcn+sLen != Here->Lcn + Here->Len                 )
                AddToClusterList(6, "used by System",
                    sLcn+sLen, lastLcn - (sLcn+sLen), "");

			sprintf(Text2, "");
			GetFileInfo(Here, Text2, &img);

            // add some information to extra-field
            if (Here->Lcn < Cstart || Here->Lcn+Here->Len > Cend) {
				sprintf(Text, " [%I64u clust]", Here->Len);
				lstrcat(Text2, Text); 
			}

            if (Here->Lcn < Cstart ) {
                sprintf(Text, " [from %I64u]", Here->Lcn);
                lstrcat(Text2, Text); }
            if (Here->Lcn+Here->Len > Cend) {
                sprintf(Text, " [to %I64u]", Here->Lcn+(Here->Len-1));
                lstrcat(Text2, Text); }

   			if (Here->free) {
				if (lastLcn <= Cend && Here->Lcn + Here->Len < Cend && lastLcn-(Here->Lcn+Here->Len)>0 && lastLen==clustersPB)
					AddToClusterList(6, "used by System", Here->Lcn + Here->Len, lastLcn-(Here->Lcn+Here->Len), "");
				if (Here->free) sprintf(Text, "free Space");
	                       else sprintf(Text, "%s", Here->fileName);
            } else          
		        sprintf(Text, "%s", Here->fileName);

            // now add to list
			if (sLen>0) { // && sLen <= (Cend - Cstart)) {
				if (Here->sparse)
					AddToClusterList(img, Text, LLINVALID, sLen, Text2);
				else 
					AddToClusterList(img, Text, sLcn, sLen, Text2);

				if (updatecounter++ % 100==0) {
				   UpdateWindow(GetDlgItem(ghListWnd, IDC_CLUSTERLIST));
				   UpdateWindow(ghListWnd);
				   RedrawWindow(ghListWnd, NULL, NULL, RDW_UPDATENOW);
				   Sleep(0);
				}
				lastLcn=sLcn;
				lastLen=sLen;
			}
			if (Here->Lcn+Here->Len+1 < Cstart) break; // at start of act block
        }
		Here = Here->next;
    }

    if (lastLcn-1 > Cstart && lastLcn!=0 && sLcn >= Cstart)
		if (lastLcn==LLINVALID)
			AddToClusterList(5, "free Space", Cstart, clustersPB, "");
		else
			AddToClusterList(6, "used by System", Cstart, lastLcn - Cstart, "");
	else if (ListView_GetItemCount(GetDlgItem(ghListWnd, IDC_CLUSTERLIST)) == 0)
				AddToClusterList(6, "used by System", Cstart, Cend-Cstart, "");

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    bUpdating       = FALSE;
}
