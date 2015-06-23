// statbar.c
// defragger 3.0beta for nt/2k/xp free.pages.at/blumetools/ blume1975@web.de

#include "globals.h"
#include "statbar.h"            // prototypes specific to statbar.c

#define noOfParts 6
#define SB_SETICON (WM_USER+15)

// create the status bar
BOOL CreateSBar(HWND hwndParent)
{
    int   ptArray[noOfParts], part = 0;   // Array defining the number of parts/sections

    InitCommonControls();   // Initialize the common control library.

    ghStatWnd = CreateStatusWindow(
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        SZDESCRIPTION, hwndParent, IDM_STATUSBAR);

    if (ghStatWnd) {
        RECT rt;
        //SIZE size;

        GetWindowRect(ghStatWnd, &rt);
        dist = (rt.bottom - rt.top);

        //GetTextExtentPoint32(memDC, "T", 1, &size);
        //dist = (size.cx + size.cy)/2;

        for (part=0; part<noOfParts; part++)
            ptArray[part] = 0;

        SendMessage(ghStatWnd, SB_SETPARTS,
            sizeof(ptArray) / sizeof(ptArray[0]),
            (LPARAM)(LPINT)ptArray);

        return TRUE;
    }

    return FALSE;
}

// update statusbar
void UpdateStatusBar(LPSTR lpsz, WORD partNo, WORD displayFlags)
{
    HDC  hdc;
    RECT rt;
    SIZE size;
    CHAR text[255];
    int  ptArray[noOfParts];   // Array defining the number of parts/sections
    int  part=0, dx=0;

    // first set new text in partNo
    SendMessage(ghStatWnd, SB_SETTEXT, partNo | displayFlags, (LPARAM)lpsz);

    // now set width of every part in ptArray...
    hdc = GetDC(ghWnd);
    for (part=0; part<noOfParts-2; part++) {
        SendMessage(ghStatWnd, SB_GETTEXT, part, (LPARAM)&text);
        if (text != NULL) {
            GetTextExtentPoint32(hdc, text, lstrlen(text), &size);
            dx += size.cx+5;
            if (part > 0) // with symbol
                dx += 3*(noOfParts-part) + part*2;
			else
				dx -= 15;
            ptArray[part] = dx;
        }
    }
    ReleaseDC(ghWnd, hdc);

    GetClientRect(ghWnd, &rt);
    ptArray[4] = rt.right - rt.left;

    SendMessage(ghStatWnd, SB_SETPARTS, noOfParts, (LPARAM)(LPINT)ptArray);

    if (partNo==1)
        SendMessage(ghStatWnd, SB_SETICON, partNo|displayFlags, (LPARAM)hiDir);
    if (partNo==2)
        SendMessage(ghStatWnd, SB_SETICON, partNo|displayFlags, (LPARAM)hiFile);
    if (partNo==3)
        SendMessage(ghStatWnd, SB_SETICON, partNo|displayFlags, (LPARAM)hiFrag);
    /*
    if (partNo==5)
        if (lpsz != NULL)
             SendMessage(ghStatWnd, SB_SETICON, partNo|displayFlags, (LPARAM)hiComp);
        else SendMessage(ghStatWnd, SB_SETICON, partNo|displayFlags, (LPARAM)NULL);
    if (partNo==6)
        if (lpsz != NULL)
             SendMessage(ghStatWnd, SB_SETICON, partNo|displayFlags, (LPARAM)hiMFT);
        else SendMessage(ghStatWnd, SB_SETICON, partNo|displayFlags, (LPARAM)NULL);
    */
}
