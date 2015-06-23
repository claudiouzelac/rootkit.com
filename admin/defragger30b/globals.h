// globals.h

#include <windows.h>    // for all Windows applications
#include <stdlib.h>
#include <stdio.h>
#include <commctrl.h>   // for common controls
#include <process.h>    // for _beginthread
#include <SHELLAPI.H>
#include <winioctl.h>   // should work for NTFS_VOLUME_DATA_BUFFER, but it does not!?

#define SZDESCRIPTION "defragger 3.0beta for win nt/2k/xp"

// USER_MESSAGES
#define WM_UPDATECLUSTERLIST 6667
#define WM_UPDATEDRIVEINFO   6668
#define WM_UPDATEDRIVELIST   6669
#define MM_DRIVE_NUM         8301          // ..8326, one for each drive letter
#define DRIVE_MENU_NUM       1             // This is the 'Drives' submenu
#define NUM_POSSIBLE_REMOVABLE_DRIVES   26 // A-Z
#define MAX_LOADSTRING       100
#define PAINTCURSOR          1002
#define DEFRAGFRAGS          1
#define DEFRAGFREE           2
#define DEFRAGBOTH           3
#define DEFRAGBYDIRS         4

typedef unsigned __int64 ULONGLONG;        // for old compilers

// Invalid values
#define LLINVALID   ((ULONGLONG) -1)
#define IINVALID    (  (__int64) -1)

typedef struct {
    LARGE_INTEGER VolumeSerialNumber;
    LARGE_INTEGER NumberSectors;
    LARGE_INTEGER TotalClusters;
    LARGE_INTEGER FreeClusters;
    LARGE_INTEGER TotalReserved;
    DWORD BytesPerSector;
    DWORD BytesPerCluster;
    DWORD BytesPerFileRecordSegment;
    DWORD ClustersPerFileRecordSegment;
    LARGE_INTEGER MftValidDataLength;
    LARGE_INTEGER MftStartLcn;
    LARGE_INTEGER Mft2StartLcn;
    LARGE_INTEGER MftZoneStart;
    LARGE_INTEGER MftZoneEnd;
} NTFS_VOLUME_DATA_BUFFER, *PNTFS_VOLUME_DATA_BUFFER;

PARTITION_INFORMATION          gPARTINFO;
DISK_GEOMETRY                  gDISKGEOM;
NTFS_VOLUME_DATA_BUFFER        gNTFSDATA;
//FILESYSTEM_STATISTICS          gFSSTATS;   // nothing I need
//NTFS_STATISTICS                gNTFSSTATS;
//FAT_STATISTICS                 gFATSTATS

// global handles
HINSTANCE ghInst;            // act Instance
HWND      ghWnd;             // for window
HWND      ghStatWnd;         // statusbar
HWND      ghListWnd;         // Handle to list view window
HANDLE    ghVolume;          // for the current raw volume
HANDLE    ghDraw;            // drawing thread
HANDLE    ghAnalyze;         // analyze thread
HANDLE    ghDefrag;          // a defragger thread

// global variables
struct FILEstruct *dirlist, *topdirlist;      // hold complete DirTree
struct MFTstruct *mftlist, *topmftlist; // hold complete DirTree
BOOL   bAnalyzed, bDumped, bStopAction, bShowMarks, bUpdating;

// global char vars
char cActualDrive, cDriveLetters[26]; // hold current drive letter / all DriveLetters
char cVolName[32], cFileSystem[16], cWinDir[512], cSysDir[512], cSysDir2[512];
char LastOutLine[512], LastFile[512], cLastSelected[512], cOSVersion[70];
char wantedLetters[27];
BOOL bDoDEFRAG, bDoOPTIMIZE, bDoBOTH, bDoDIRDEFRAG, bAllLocalDisks, bQuitAfterJobs;

// essential draw variables
int           imgWidth, imgHeight;
int           blocksize, bw, bh;
int           dist;               // distance of graphic from statusbar
long          xposlast, yposlast; // position of last outlined block

// counter vars start with c
__int64       sizeLimit;
int           sLimit;
int           clustersPB;
unsigned long cDiskNumber;
ULONGLONG     cDiskClusters, cFreeClusters, clustersPL;
__int64       cBytes, cBytesSystem, cBytesFree, cBytesUsed, cBytesTotal;
unsigned long cRootDirs, cDirs, cFiles, cDirsFiles, cFilesToMove;
unsigned long cFragmented, cLocked, cCompressed, cFragmentsAllTogether, FilesMoved;
int           cTimesRestarted;
DWORD         cBytesPerCluster, cSectorsPerCluster;
int           procprior;
int           lastsort;

long int      progress, lastprogress;
int           subDirLevel;

// function for syncing
BOOL          CheckActivities(void);

// functions called as threads
void          DrawBitMap(void *dummy);
void          AnalyzeVolume(void *dummy);
void          DefragFragments(void *dummy);
void          DefragFreeSpace(void *dummy);
void          DefragByDirectories(void *dummy);
void          Defrag(void *dummy);
void          DumpLayoutToFile(void *);
// SDK-Help:
// "A thread that uses functions from the C run-time libraries should
// use the _beginthread and _endthread C run-time functions for thread
// management rather than CreateThread and ExitThread. Failure to do
// so results in small memory leaks when ExitThread is called."

// functions called normal and declared somewhere in main.c or defrag.c
// OutLine0-1: TextOut at line 0-1 over the statusbar
void          OutLine0(char *buff);
void          OutLine1(char *buff);
void          EnableMenus(int ID);
void          DisableMenus(int ID);
void          GetPartitionInfo(void);
void          GetPartitionInfoDetails(void);
void          UpdateClusterList(void *d);
void          UpdateDriveInformation(void);
void          ResetStatusBar(void);
char          *fmtNumber(__int64 _in, char *cGes);
int           strstrlwr(const char *string1, const char *substring2);

// graphic stuff
HDC         memDC;        // memory device context
HBITMAP     memBM;        // memory bitmap (for  memDC) Double buffering data

HICON    hiDir, hiFile, hiFrag, hiComp, hiMFT, hiFree, hiSystem, hiMain;

COLORREF ColBG;
COLORREF ColDGreen  , ColGreen   , ColLGreen  , ColL2Green , ColL3Green , ColVio     , ColDVio;

COLORREF ColRed     , ColDRed    , ColOck     , ColYellow  , ColLYellow , ColBlue    , ColLBlue;
COLORREF ColL2Blue  , ColBrown   , ColWhite   , ColGray    , ColBlack   ;

HPEN        hpBG;
HBRUSH      hbBG;
HPEN        hpDGreen, hpGreen, hpLGreen, hpL2Green, hpL3Green, hpVio, hpDVio, hpRed, hpDRed;
HPEN        hpOck, hpYellow, hpLYellow, hpBlue, hpLBlue, hpL2Blue, hpBrown, hpWhite, hpGray, hpBlack;
HBRUSH      hbDGreen, hbGreen, hbLGreen, hbL2Green, hbL3Green, hbVio, hbDVio, hbRed, hbDRed, hbOck;
HBRUSH      hbYellow, hbLYellow, hbBlue, hbLBlue, hbL2Blue, hbBrown, hbWhite, hbGray, hbBlack;
