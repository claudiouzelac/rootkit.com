// statbar.h - declarations for the status bar

#define IDM_STATUSBAR       501
#define IDM_TIMER           701
#define TIMER_TIMEOUT       1000

// System Menu string ID's
#define IDS_SYSMENU         900

#define IDS_SCSIZE          SC_SIZE
#define IDS_SCMOVE          SC_MOVE
#define IDS_SCMINIMIZE      SC_MINIMIZE
#define IDS_SCMAXIMIZE      SC_MAXIMIZE
#define IDS_SCCLOSE         SC_CLOSE
#define IDS_SCRESTORE       SC_RESTORE
#define IDS_SCTASKLIST      SC_TASKLIST

// prototypes for status bar
BOOL CreateSBar(HWND);
void UpdateStatusBar(LPSTR, WORD, WORD);
LRESULT MsgTimer(HWND, UINT, WPARAM, LPARAM);
