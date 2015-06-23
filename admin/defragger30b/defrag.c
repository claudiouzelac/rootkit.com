// defrag.c - handles defragging, analyzing, drawing
// defragger 3.0beta for nt/2k/xp free.pages.at/blumetools/ blume1975@web.de

#include "globals.h"
#include "defrag.h"
#include "mft.h"
#include "statbar.h"
#include "list.h"
#include "resource.h"
#include <wingdi.h>

void          MoveClusters       (PMOVE_FILE_DATA); // win32
ULONGLONG     MovePartOfOpenedFile(HANDLE sourceFile, int fragNo, ULONGLONG startVcn,
                                  ULONGLONG OldLcn, ULONGLONG NewLcn, ULONGLONG fClusters);
void          ReadTreeFAT        (char *Path);
ULONGLONG     GetFreeClusters    (ULONGLONG MinLcn, ULONGLONG MinLen,
                                  ULONGLONG *beginLcn, ULONGLONG *endLcn, BOOL MatchSpace);
void          RenameDirs         (void);
void          DrawSpecialFiles   (void);
void          OpenTheFile        (char * fileName, HANDLE *sourceFile);

#define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all

// Size of the buffer we read file mapping information into.
// The buffer is big enough to hold the 16 bytes that
// come back at the head of the buffer (the number of entries
// and the starting virtual cluster), as well as 512 pairs
// of [virtual cluster, logical cluster] pairs.
#define FILEMAPSIZE (512+2)
//#define FILEMAPSIZE (256+2)

// Size of the bitmap buffer we pass in. Its big enough to
// hold information for the 16-byte header that's returned
// plus the indicated number of bytes, each of which has 8 bits (imagine that!)
#define BITMAPBYTES 8192  //65536  //32768  //4096 //16384  //8192
#define BITMAPSIZE  (BITMAPBYTES+2*sizeof(ULONGLONG))

// Buffer to read file mapping information into
ULONGLONG FileMap[ FILEMAPSIZE ];

// Buffer thats passed to bitmap function
BYTE      BitMap[ BITMAPSIZE ];

// Bit shifting array for efficient processing of the bitmap
BYTE      BitShift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

// OutLine0-1 TextOut at line 0 or 1 over the statusbar
void OutLine0(char *buff) {
    HDC hdc;
    // draw on backbuffer
    SelectObject(memDC, hpBG);
    SelectObject(memDC, hbBG);
    Rectangle(memDC, 0, imgHeight-(dist*2), imgWidth, imgHeight-(dist+6));
    TextOut(memDC, 5, imgHeight-(dist*2), buff, strlen(buff));

	// BitBlt the backbuffer to the screen
    hdc = GetDC(ghWnd);
    BitBlt(hdc, 0, imgHeight-(dist*2), imgWidth, imgHeight-(dist+6), memDC, 0, imgHeight-(dist*2), SRCCOPY);
    ReleaseDC(ghWnd, hdc);
}

void OutLine1(char *buff) {
    HDC hdc;
    // draw on backbuffer
	SelectObject(memDC, hpBG);
    SelectObject(memDC, hbBG);
    Rectangle(memDC, 5, imgHeight-(dist),imgWidth,imgHeight-6);
    TextOut(memDC, 5, imgHeight-(dist), buff, strlen(buff));

	// BitBlt the backbuffer to the screen
	hdc = GetDC(ghWnd);
    BitBlt(hdc, 0, imgHeight-(dist), imgWidth, imgHeight, memDC, 0, imgHeight-(dist), SRCCOPY);
    ReleaseDC(ghWnd, hdc);

    // remember the last output
	lstrcpy(LastOutLine, buff);
}

// translate a Win32 error into its text equivalent
void HandleWin32Error(DWORD ErrorCode, char *zusatz) {
    char buff[255];
    if (ErrorCode==0) return;
	buff[0]=0;
    FormatMessage( FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, ErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)buff, 255, NULL );
	buff[lstrlen(buff)-2]=0;
    lstrcat((LPTSTR)buff, (LPCTSTR)" ");
    lstrcat((LPTSTR)buff, (LPCTSTR)zusatz);
    OutLine1(buff);
    SetLastError(0);
    //MessageBox(ghWnd, (LPCSTR)buff, (LPCTSTR)"Info", MB_OK);
}

// drawing functions
void ClearLastSelectedFile(void) {
    struct FILEstruct * Here=NULL;
	if (blocksize == 1) return;

	if (cLastSelected[0] != '\0') {
        // clear if already something was selected
        bUpdating = TRUE;
        Here = topdirlist;
        while (Here) {
            if (strcmp(Here->fileName, cLastSelected) == 0)
                DrawBlockBounds(Here->Lcn, Here->Lcn+Here->Len, TRUE);
            Here = Here->next;
        }
        bUpdating = FALSE;
        lstrcpy(cLastSelected, "");
    }
}

void DrawNoBlockBound(int x, int y) {
    HDC hdc;
	if (blocksize == 1) return;
	hdc = GetDC(ghWnd);
    SelectObject(hdc, hpBG);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, x, y, x+blocksize+1, y+blocksize+1);
    ReleaseDC(ghWnd, hdc);

    SelectObject(memDC, hpBG);
    SelectObject(memDC, GetStockObject(NULL_BRUSH));
    Rectangle(memDC, x, y, x+blocksize+1, y+blocksize+1);
}

void DrawBlockBound(int x, int y) {
    HDC hdc;
	if (blocksize == 1) return;

    if (!IsWindowVisible(ghListWnd) || !ghDefrag)
	    ClearLastSelectedFile();

	hdc = GetDC(ghWnd);
    SelectObject(hdc, hpBlack);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, x, y, x+blocksize+1, y+blocksize+1);
    ReleaseDC(ghWnd, hdc);

    SelectObject(memDC, hpBlack);
    SelectObject(memDC, GetStockObject(NULL_BRUSH));
    Rectangle(memDC, x, y, x+blocksize+1, y+blocksize+1);
}

void DrawBlockBounds(ULONGLONG Cstart, ULONGLONG Cend, BOOL clear) {
    ULONGLONG ui;
    int x=0, y=0;

    if (blocksize == 1) return;
	if (Cstart > cDiskClusters && Cend > cDiskClusters) return;
    if (Cstart > cDiskClusters) return;
    if (Cend   > cDiskClusters) Cend   = cDiskClusters;

    while (                          y/blocksize*clustersPL <= Cstart )
        y += blocksize;
    y -= blocksize;

    while ( x/blocksize*clustersPB + y/blocksize*clustersPL <= Cstart )
        x += blocksize;
    x -= blocksize;

    if (!clear) DrawBlockBound(x, y);
    else        DrawNoBlockBound(x, y);
    Cstart+=clustersPB;

    for (ui=Cstart; ui<=Cend; ui+=clustersPB) {
        x+=blocksize;
        if (x>imgWidth-blocksize) { x=0; y+=blocksize; }
        if (!clear) DrawBlockBound(x, y);
        else        DrawNoBlockBound(x, y);
    }
}

void DrawMarkLineAt(int x, int y) {
    HDC hdc;
    x++; y++;
    hdc = GetDC(ghWnd);
    SelectObject(hdc, hpYellow);
    SelectObject(hdc, hbYellow);
    MoveToEx(hdc,   x            , y+blocksize/2-1, NULL);
    LineTo(hdc,     x+blocksize-1, y+blocksize/2-1);
    ReleaseDC(ghWnd, hdc);

    SelectObject(memDC, hpYellow);
    SelectObject(memDC, hbYellow);
    MoveToEx(memDC, x            , y+blocksize/2-1, NULL);
    LineTo(memDC,   x+blocksize-1, y+blocksize/2-1);
}

void DrawMarkAt(int x, int y, BOOL anfang) {
    HDC hdc;
    x++; y++;
    hdc = GetDC(ghWnd);
    SelectObject(hdc, hpYellow);
    SelectObject(hdc, hbYellow);
    //senkrecht mittig
    MoveToEx(hdc, x+blocksize/2-1, y+1          , NULL);
    LineTo(hdc,   x+blocksize/2-1, y+blocksize-2);
    SelectObject(memDC, hpYellow);
    SelectObject(memDC, hbYellow);
    //senkrecht mittig
    MoveToEx(memDC, x+blocksize/2-1, y+1          , NULL);
    LineTo(memDC,   x+blocksize/2-1, y+blocksize-2);
    if (anfang) {
        //waagerecht v mitte bis rechts |-
        MoveToEx(hdc, x              , y+blocksize/2-1, NULL);
        LineTo(hdc,   x+blocksize/2-1, y+blocksize/2-1);
        //waagerecht v mitte bis rechts |-
        MoveToEx(memDC, x              , y+blocksize/2-1, NULL);
        LineTo(memDC,   x+blocksize/2-1, y+blocksize/2-1);
    } else {
        //waagerecht v links bis mitte -|
        MoveToEx(memDC, x+blocksize/2-1, y+blocksize/2-1, NULL);
        LineTo(memDC,   x+blocksize  -1, y+blocksize/2-1);
        //waagerecht v links bis mitte -|
        MoveToEx(hdc, x+blocksize/2-1, y+blocksize/2-1, NULL);
        LineTo(hdc,   x+blocksize  -1, y+blocksize/2-1);
    }
    ReleaseDC(ghWnd, hdc);
}

void DrawMarks(ULONGLONG Cstart, ULONGLONG Cend) {
    ULONGLONG ui;
    int x=0, y=0;

    if (blocksize == 1) return;
    if (blocksize < 5) return;
    if (Cstart > cDiskClusters && Cend > cDiskClusters) return;
    if (Cstart > cDiskClusters) return;
    if (Cend   > cDiskClusters) Cend   = cDiskClusters;

    while (                          y/blocksize*clustersPL <= Cstart )
        y += blocksize;
    y -= blocksize;

    while ( x/blocksize*clustersPB + y/blocksize*clustersPL <= Cstart )
        x += blocksize;
    x -= blocksize;

    DrawMarkAt(x, y, 0); // start mark
    Cstart += clustersPB;
    for (ui=Cstart; ui+(clustersPB/2)<=Cend; ui+=clustersPB) {
        x+=blocksize;
        if (x>imgWidth-blocksize) { x=0; y+=blocksize; }
        DrawMarkLineAt(x, y);
    }
    x+=blocksize;
    if (x>imgWidth-blocksize) { x=0; y+=blocksize; }
    DrawMarkAt(x, y, 1); // end mark
}

void DrawClusters(LONGLONG ClusterStart, LONGLONG ClusterEnd, HPEN hp) {
	int x1, y1, x2, y2;
	HDC hdc;

	if (cDiskClusters == 0 || ClusterEnd == 0) return;

	x1 = (int)((ClusterStart * imgWidth * (imgHeight-dist*2) / cDiskClusters ) % (imgWidth) );
	y1 = (int)((ClusterStart * imgWidth * (imgHeight-dist*2) / cDiskClusters ) / imgWidth);
	x2 = (int)((ClusterEnd   * imgWidth * (imgHeight-dist*2) / cDiskClusters ) % (imgWidth) );
	y2 = (int)((ClusterEnd   * imgWidth * (imgHeight-dist*2) / cDiskClusters ) / imgWidth);

	if (x2-x1==0 && y2-y1==0) 
		x2+=1;

	hdc = GetDC(ghWnd);
	SelectObject(hdc,hp);
	SelectObject(memDC,hp);
	while (y1 < y2) {
    	MoveToEx(hdc,x1,y1,NULL);
		LineTo(hdc,imgWidth,y1);
		// memDC again
    	MoveToEx(memDC,x1,y1,NULL);
		LineTo(memDC,imgWidth,y1);
		x1 = 0;
		y1 = y1 + 1;
	}

	MoveToEx(hdc,x1,y1,NULL);
	LineTo(hdc,x2,y2);
	ReleaseDC(ghWnd, hdc);

	// memDC again
	MoveToEx(memDC,x1,y1,NULL);
	LineTo(memDC,x2,y2);
}

void DrawBlockAt(HDC hdc, int x, int y, HBRUSH hb) {
    RECT r;
    x++; y++;
    r.top = y; r.left = x; r.bottom = y+blocksize-1; r.right = x+blocksize-1;

    FillRect(hdc, &r, hb);
	FillRect(memDC, &r, hb);
}

void DrawBlocks(ULONGLONG Cstart, ULONGLONG Cend, HBRUSH hb, HPEN hp) {
    ULONGLONG ui;
    int x=0, y=0;
	HDC hdc;

    if (Cstart > cDiskClusters && Cend > cDiskClusters) return;
    if (Cstart > cDiskClusters) return;
    if (Cend   > cDiskClusters) Cend   = cDiskClusters;

	if (blocksize == 1) {
		DrawClusters(Cstart, Cend, hp);
		return;
    }

    while (                          y/blocksize*clustersPL <= Cstart )
        y += blocksize;
    y -= blocksize;

    while ( x/blocksize*clustersPB + y/blocksize*clustersPL <= Cstart )
        x += blocksize;
    x -= blocksize;

    hdc = GetDC(ghWnd);
	DrawBlockAt(hdc, x, y, hb);
    for (ui=Cstart; ui+(clustersPB/2)<=Cend; ui+=clustersPB) {
        x+=blocksize;
        if (x>imgWidth-blocksize) { x=0; y+=blocksize; }
        DrawBlockAt(hdc, x, y, hb);
    }
	ReleaseDC(ghWnd, hdc);
}

void DrawProgress(int progress) {
    HDC hdc;
    RECT r;
    if (progress == -1) progress = imgWidth;

    r.top = imgHeight-5; r.left = 0; r.bottom = imgHeight; r.right = progress;

    if (progress == imgWidth)
        FillRect(memDC ,&r, hbBG); // clear with BG-color
    else
        FillRect(memDC ,&r, hbBlack);

    hdc = GetDC(ghWnd);
    BitBlt(hdc, 0, imgHeight-5, imgWidth, imgHeight, memDC, 0, imgHeight-5, SRCCOPY);
    ReleaseDC(ghWnd, hdc);
}

// check if the given part of LCN + LEN is inside the MFT
void CheckForMFT(ULONGLONG *cl2, ULONGLONG *nu2) {
    ULONGLONG cluster2=*cl2, numFree2=*nu2;
    ULONGLONG cluster=0, numFree=0, zw=0;
    struct MFTstruct * Here=NULL;

    if (gPARTINFO.PartitionType == 7) {
        Here = topmftlist;
        while (Here) {
            if (cluster2            <= Here->Lcn &&
                cluster2 + numFree2 >  Here->Lcn + Here->Len)
            { // MFT in the middle of free space
                cluster  = cluster2;
                numFree  = Here->Lcn - cluster2;
                if (numFree>cDiskClusters)
					numFree = 0;
                InsDIR("", numFree*cBytesPerCluster, cluster, numFree, 0, LLINVALID, LLINVALID, TRUE, FALSE, FALSE, FALSE, FALSE, IINVALID);
				zw = cluster2;
                cluster2 = Here->Lcn+Here->Len;
                numFree2 = (zw+numFree2) - (Here->Lcn+Here->Len);
            } else

			if ((cluster2          >  Here->Lcn) &&
                (cluster2          <  Here->Lcn+Here->Len)       )
            { // MFT before free space
                numFree2 = numFree2 - (Here->Lcn+Here->Len - cluster2);
                if (numFree2>cDiskClusters)
					numFree2 = 0;
				else if (numFree2 > 0)
                    cluster2 = Here->Lcn+Here->Len;
                else
                    cluster2 = LLINVALID;
            }

            if ( cluster2 != LLINVALID &&
                (cluster2+numFree2 >  Here->Lcn) &&
                (cluster2+numFree2 <  Here->Lcn+Here->Len+1)       )
            { // MFT after free space
                numFree2 = Here->Lcn - cluster2;
                if (numFree2>cDiskClusters)
					numFree2 = 0;
                //cluster2 = MFTZONE[i].start;
            }
            Here = Here->next;
        }
    }
    if (numFree2<0) numFree2=0;
    *cl2=cluster2;
    *nu2=numFree2;
}

// insert an empty entry into dirlist
void InsDIRFree(ULONGLONG cluster2, ULONGLONG numFree2) {
    if (bAnalyzed) return;

    CheckForMFT(&cluster2, &numFree2);
    if (cluster2 != LLINVALID && numFree2 > 0 && numFree2<cDiskClusters)
        InsDIR("", numFree2*cBytesPerCluster, cluster2, numFree2, 0, LLINVALID, LLINVALID, TRUE, FALSE, FALSE, FALSE, FALSE, IINVALID);
}

void OpenTheFile(char * fileName, HANDLE *sourceFile) {
    // Open the file
    *sourceFile = CreateFile(fileName,
        GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING, 0);

    if (*sourceFile == INVALID_HANDLE_VALUE ) {
    *sourceFile = CreateFile(fileName,
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING, 0);
	}

    if (*sourceFile == INVALID_HANDLE_VALUE ) {
        // try again as directory
        *sourceFile = CreateFile(fileName,
            GENERIC_READ,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if( *sourceFile == INVALID_HANDLE_VALUE ) {
			//HandleWin32Error(GetLastError(), fileName);
            return;
        }
    }
}

void MoveClusters(PMOVE_FILE_DATA pmoveFile) {
    DWORD                  status;
    DWORD                  w=0;

	// make the move
    status = DeviceIoControl(ghVolume,
        FSCTL_MOVE_FILE,
        pmoveFile, sizeof(MOVE_FILE_DATA),
        NULL, 0, &w, NULL);

    HandleWin32Error(GetLastError(), "FSCTL_MOVE_FILE");
	if (!status) pmoveFile->ClusterCount = 0;
}

ULONGLONG MovePartOfOpenedFile(HANDLE sourceFile, int fragNo, ULONGLONG startVcn,
                         ULONGLONG OldLcn, ULONGLONG NewLcn, ULONGLONG fClusters) {
    ULONGLONG        Lcn1, Lcn2;
    ULONGLONG        OldLcnFirst = OldLcn, NewLcnFirst = NewLcn;
    ULONGLONG        Offset, Clusters, movedLen=0;
    char             buff[512];
    MOVE_FILE_DATA   moveFile;

    DrawBlockBounds(OldLcnFirst, OldLcnFirst+fClusters, FALSE);
    DrawBlockBounds(NewLcnFirst, NewLcnFirst+fClusters, FALSE);

    Lcn1 = NewLcn;
    Lcn2 = OldLcn;
    Offset = 0;

    while (Offset < fClusters) {
        Clusters = fClusters - Offset;
        if (Lcn2 > Lcn1 && Lcn2 < Lcn1 + Clusters) {
            Clusters = Lcn2 - Lcn1;
        }

		// change down clusters moved in one step to be more flexible
        if (gPARTINFO.PartitionType==7) {
			if (Clusters > 64) Clusters = 64;
		} else {
			if (Clusters > 16) Clusters = 16;
		}

		// info stuff
        if (fragNo == 0)
        if (Lcn2 > Lcn1 && Lcn2 < Lcn1 + Clusters)
            sprintf(buff, "shifting %I64u clusters (%I64u/%I64u) from %I64u to %I64u",
                Clusters, Offset+Clusters, fClusters, OldLcn, Lcn1);
        else if (Clusters == fClusters)
            sprintf(buff, "moving %I64u clusters from %I64u to %I64u",
                Clusters, OldLcn, Lcn1);
        else
            sprintf(buff, "moving %I64u/%I64u clusters from %I64u to %I64u",
                Offset+Clusters, fClusters, OldLcn, Lcn1);
        else
        if (Clusters == fClusters)
            sprintf(buff, "moving fragment #%d - %I64u clusters - from %I64u to %I64u",
                fragNo, Clusters, OldLcn, Lcn1);
        else
            sprintf(buff, "moving fragment #%d - %I64u/%I64u clusters - from %I64u to %I64u",
                fragNo, Offset+Clusters, fClusters, OldLcn, Lcn1);

        OutLine1(buff);

        // graphic things
		// first the from-clusters in yellow
        DrawBlocks(OldLcn, OldLcn+Clusters, hbLYellow, hpLYellow);
        // now the to-clusters in L2Blue
        DrawBlocks(Lcn1, Lcn1+Clusters, hbL2Blue, hpL2Blue);

        ///////////////////////////////////////////////////////
        moveFile.FileHandle           = sourceFile;
        moveFile.StartingVcn.QuadPart = startVcn + Offset;
        moveFile.StartingLcn.QuadPart = Lcn1;
        moveFile.ClusterCount         = (ULONG) Clusters;

        MoveClusters(&moveFile);
        if (moveFile.ClusterCount < Clusters) {
            movedLen = 0;
            break;
            //if (moveFile.NumVcns < Clusters) return 0;
        }
        else movedLen += Clusters;
        ///////////////////////////////////////////////////////

        // now clean the from-clusters
        DrawBlocks(OldLcn, OldLcn+Clusters, hbYellow, hpYellow);
        // now the ready to-clusters
        DrawBlocks(Lcn1, Lcn1+Clusters, hbLBlue, hpLBlue);

        Lcn1 = Lcn1 + Clusters;
        Lcn2 = Lcn2 + Clusters;
        Offset = Offset + Clusters;
        OldLcn = OldLcn + Clusters;
        NewLcn = NewLcn + Clusters;

        // wait for draw or break, but finish small files
        if (bStopAction && fClusters > 200)
            break;
        if ( ghDraw ) WaitForSingleObject(ghDraw, INFINITE);
    }

    DrawBlockBounds(OldLcnFirst, OldLcnFirst+fClusters, TRUE);
    DrawBlockBounds(NewLcnFirst, NewLcnFirst+fClusters, TRUE);
    return (ULONGLONG)(movedLen);
}

ULONGLONG MovePartOfFile(struct FILEstruct *Here, ULONGLONG NewLcn) 
{
	HANDLE sourceFile;
	ULONGLONG movedLen=0;

	OpenTheFile(Here->fileName, &sourceFile);
	if(sourceFile==INVALID_HANDLE_VALUE) return 0;

	movedLen = MovePartOfOpenedFile(sourceFile, (int)(Here->fragid), Here->Vcn, Here->Lcn, NewLcn, Here->Len);
    CloseHandle(sourceFile);

    return (ULONGLONG)(movedLen);
}

ULONGLONG MoveFileNormal(struct FILEstruct *Here, ULONGLONG NewLcn) {
    HANDLE sourceFile;
    ULONGLONG movedLen=0;
	char buff[512];

	OpenTheFile(Here->fileName, &sourceFile);

    if(sourceFile==INVALID_HANDLE_VALUE) return 0;

	if (Here->filesize != IINVALID)
       sprintf(buff, "%.3f MB - %s",
          (float)Here->filesize/1024/1024, Here->fileName);
    else
       sprintf(buff, "%s", Here->fileName);
    UpdateStatusBar(buff, 4, 0);
	lstrcpy(LastFile, Here->fileName);

    // ---do the work----------------------------------
    movedLen = MovePartOfOpenedFile(sourceFile, 0, 0, Here->Lcn, NewLcn, Here->Len);
    CloseHandle(sourceFile);

	if (movedLen == Here->Len)
		Here->Lcn = NewLcn;

    return movedLen;
}

// walk through the fragmented clusters of a file and move them to a free space on disk
ULONGLONG MoveFileFragmented (struct FILEstruct *Here, ULONGLONG *fLcn) {
    PGET_RETRIEVAL_DESCRIPTOR fileMappings;
    HANDLE                    sourceFile;
    ULONGLONG                 startVcn;
    ULONGLONG                 numClusters=0;
    ULONGLONG                 lastLcn=0, lastLen=0;
    ULONGLONG                 moved=0, movedLen=0;
    ULONGLONG                 startLcn=LLINVALID, sLcn;
    ULONGLONG                 ClusterStart=0, ClusterEnd=0, Clusters=0;
    ULONG                     fragNo=0;
    BOOL                      bdir=FALSE;
    int                       i;
    DWORD                     lpBytesReturned;
    DWORD                     err=ERROR_MORE_DATA;
	char                      buff[512];

    sLcn = *fLcn;
    if (Here->fileName[strlen(Here->fileName)-1] == '\\') 
		bdir = TRUE;

	OpenTheFile(Here->fileName, &sourceFile);

    if (sourceFile==INVALID_HANDLE_VALUE) return 0;

    if (Here->filesize != IINVALID)
       sprintf(buff, "file: %.3f MB - %I64u fragments - %s",
          (float)Here->filesize/1024/1024, Here->fragments, Here->fileName);
    else
       sprintf(buff, "dir: %I64u fragments - %s", Here->fragments, Here->fileName);
    UpdateStatusBar(buff, 4, 0);
   	lstrcpy(LastFile, Here->fileName);

    // setup buffer
    fileMappings = (PGET_RETRIEVAL_DESCRIPTOR) FileMap;
    startVcn = 0;

    // Start dumping the mapping information. Go until we hit EOF
    do {
        if (DeviceIoControl(sourceFile,
                FSCTL_GET_RETRIEVAL_POINTERS,
                &startVcn, sizeof(ULONGLONG),
                fileMappings, sizeof(RETRIEVAL_POINTERS_BUFFER),
                &lpBytesReturned, NULL) == 0 )
        {
            err = GetLastError();
            if (err != ERROR_MORE_DATA) break;
        }

        // Loop through the buffer of number/cluster pairs
        startVcn = fileMappings->StartVcn;
        for( i = 0; i < (ULONGLONG) fileMappings->NumberOfPairs; i++ ) {
            if( fileMappings->Pair[i].Lcn == LLINVALID ) {
				// On NT 4.0, a compressed virtual run (0-filled) is
				// identified with a cluster offset of -1
                // compressed file - do nothing
				movedLen += fileMappings->Pair[i].Vcn - startVcn;
            } else {
                // remember first cluster
                if (startLcn == LLINVALID) startLcn = fileMappings->Pair[i].Lcn;
                numClusters = fileMappings->Pair[i].Vcn - startVcn;
                fragNo++;

                // skip first frag - handle in second pass
                if (fileMappings->Pair[i].Lcn != startLcn)
                {
                    // handle the first frag
                    if (lastLcn == startLcn)
                    {   // first cluster of a dir on FAT is not moveable
                        // so skip if next cluster is not free
                        if (bdir && gPARTINFO.PartitionType != 7) {
                            ClusterStart = sLcn;
                            Clusters = GetFreeClusters(ClusterStart+lastLen, Here->Len-lastLen, &ClusterStart, &ClusterEnd, TRUE);
                            if (sLcn + lastLen != ClusterStart+lastLen) {
                                err = 0;
                                break;
                            }
                        } else {
                            ClusterStart = lastLcn;
                            Clusters = GetFreeClusters(ClusterStart+lastLen, Here->Len-lastLen, &ClusterStart, &ClusterEnd, TRUE);
                        }

                        if (sLcn + lastLen == ClusterStart &&
                            Clusters >= Here->Len-lastLen)
                        {
                            // skip first fragment if enough space after it
                            ;
                        } else {
                            // else find new free space
                            ClusterStart = 0;
							Clusters = GetFreeClusters(ClusterStart, Here->Len, &ClusterStart, &ClusterEnd, TRUE);
							if (ClusterStart >= cDiskClusters || Clusters < Here->Len) {
								OutLine1("not enough free space available");
                                CloseHandle(sourceFile);
                                return movedLen;
							}

                            moved = MovePartOfOpenedFile(sourceFile, fragNo-1, 0, lastLcn, ClusterStart, lastLen);
				            if (CheckActivities()) break;

                            if( moved == lastLen ) {
                                movedLen += moved;
                                *fLcn = ClusterStart;
                                ClusterStart += moved;
                            } else {
                                CloseHandle(sourceFile);
                                return movedLen;
                            }
                        }
                    }

                    // handle all following frags
                    moved = MovePartOfOpenedFile(sourceFile, fragNo,
                        startVcn, fileMappings->Pair[i].Lcn,
                        ClusterStart, numClusters);

		            if (CheckActivities() || bStopAction) break;

                    if( moved == numClusters ) {
                        movedLen += numClusters;
                        ClusterStart += moved;
                    } else {
						CloseHandle( sourceFile );
                        return movedLen;
                    }
                }

                // now remember actual cluster
                lastLcn = fileMappings->Pair[i].Lcn;
                lastLen = numClusters;   // on NTFS?
            }
            // move to next fragment of file
            startVcn = fileMappings->Pair[i].Vcn;
        } // for

		if (bStopAction) break;

    } while ((err == ERROR_MORE_DATA) && (startVcn < cDiskClusters));

    if (fragNo==1) {  // this is not a fragmented file, but process anyway
        ClusterStart = 0;
        Clusters = GetFreeClusters(ClusterStart, Here->Len, &ClusterStart, &ClusterEnd, TRUE);
        if (Clusters == 0) return movedLen;
        moved = MovePartOfOpenedFile(sourceFile, fragNo-1, 0, lastLcn, ClusterStart, lastLen);

        if( moved == lastLen ) {
            movedLen += moved;
            *fLcn = ClusterStart;
            ClusterStart += moved;
        } else {
			if (ClusterStart >= cDiskClusters || Clusters < Here->Len) {
				OutLine1("not enough free space available");
                CloseHandle(sourceFile);
                return movedLen;
			}
            CloseHandle(sourceFile);
            return movedLen;
        }
	}
    CloseHandle( sourceFile );

	if (movedLen==Here->Len) {
        FilesMoved++;
        //InsDIRFree(Here->Lcn, Here->Len);
        DumpFile(Here->fileName, Here->filesize);
        //Here = DumpFile(Here->fileName, Here->filesize);
        if (!dirlist->fragmented) {
            dirlist->fragmented = FALSE;
            dirlist->fragments = 1;
            sprintf(buff, "%d fragmented", --cFragmented);
            UpdateStatusBar(buff, 3, 0);
        }
    } else {
        sprintf(buff, "cannot defrag: %s", Here->fileName);
        OutLine1(buff);
        UpdateStatusBar(buff, 4, 0);
    }

    return movedLen;
}

// find free clusters depending on MinLcn, MinLen, and MatchSpace
ULONGLONG GetFreeClusters(ULONGLONG MinLcn, ULONGLONG MinLen,
                          ULONGLONG *beginLcn, ULONGLONG *endLcn, BOOL MatchSpace) {
    PBITMAP_DESCRIPTOR     bitMappings;
    ULONGLONG              startLcn;
    ULONGLONG              nextLcn;
    ULONGLONG              lastLcn=LLINVALID;
    ULONGLONG              cluster;
    ULONGLONG              cluster2=LLINVALID;
    ULONGLONG              numFree=0, numFree2=0;
    ULONGLONG              i;
    DWORD                  lpBytesReturned;
    DWORD                  err;

    // setup buffer
    bitMappings = (PBITMAP_DESCRIPTOR) BitMap;

    // start scanning at the specified cluster offset
    cluster  = 0;
    startLcn = 0;
    nextLcn  = MinLcn;
    if (nextLcn > cDiskClusters-1) 
		return 0;

    cluster = LLINVALID;

    do {
        if (DeviceIoControl(ghVolume,
                FSCTL_GET_VOLUME_BITMAP,
                &nextLcn, sizeof(ULONGLONG),
                bitMappings, BITMAPSIZE,
                &lpBytesReturned, NULL) == 0)
        {
            err = GetLastError();
            if (err != ERROR_MORE_DATA) break;
        }

        // Scan through the returned bitmap info, looking for empty clusters
        startLcn = bitMappings->StartLcn;

        for( i = 0; i < min( bitMappings->ClustersToEndOfVol, 8*BITMAPBYTES); i++ ) {
            if( !(bitMappings->Map[ i/8 ] & BitShift[ i % 8 ])) {
                // Cluster is free -----------------
                if( cluster == LLINVALID ) {
                        cluster = startLcn + i;
                        if (cluster2 == LLINVALID)
                            cluster2 = cluster;
                        numFree = 1;
                    } else {
                        numFree++;
					}
            } else {
                // Cluster is not free ------------------
                if( cluster != LLINVALID ) {
                    if( lastLcn == cluster ) {
                        lastLcn = LLINVALID;
                    } else {

                        if (cluster2 + numFree2+1 < cluster) {
                            numFree2 = 0;
                            cluster2 = cluster;
                        }

                        numFree2 += numFree;

                        CheckForMFT(&cluster2, &numFree2);
                        if (cluster2 >= MinLcn && cluster2 != LLINVALID)
                        if (MatchSpace) {
                            if (numFree2 >= MinLen && cluster2 >= MinLcn) {
                                *beginLcn = cluster2;
                                *endLcn = cluster2 + numFree2-1;
                                if (*endLcn == LLINVALID) 
									*endLcn = cDiskClusters;
                                return numFree2;
                            } else {
                                ; // continue
                            }
                        } else {
                            *beginLcn = cluster2;
                            *endLcn = cluster2 + numFree2-1;
                            if (*endLcn == LLINVALID) *endLcn = cDiskClusters;
                            return numFree2;
                        }

                        lastLcn  = cluster;
                        numFree  = 0;
                        numFree2 = 0;
                        cluster  = LLINVALID;
                        cluster2 = LLINVALID;
                    }
                }
            }
        } // for all clusters in this pair

        // Move to the next block
        nextLcn = bitMappings->StartLcn + i;
    } while ( (err == ERROR_MORE_DATA) && (nextLcn < cDiskClusters));

    if (numFree > 0) {
        *beginLcn = cluster2;
        *endLcn = cluster2 + numFree - 1;
    }
    return numFree;
}

// draw map of the disk in green/lightgreen/white
void DumpBitmap(void) {
    PBITMAP_DESCRIPTOR     bitMappings;
    ULONGLONG              startLcn=0, nextLcn=0;
    ULONGLONG              lastLcn=0, lastCluster=0;
    ULONGLONG              cluster=LLINVALID, cluster2=LLINVALID;
    ULONGLONG              numFree=0, numFree2=0, numFree3=0;
    ULONGLONG              i;
    ULONGLONG              clusterC = 0;
    int                    x=0, y=0;
    DWORD                  lpBytesReturned;
    DWORD                  err;
    int                    divider=4;
	HDC                    hdc;
	struct MFTstruct       *HereMFT=NULL;
	struct FILEstruct      *HereFILE=NULL;

    if (clustersPB<divider) divider = clustersPB;

    if ( !bDumped ) {
		// empty clear delete the dirlist
		if (topdirlist) DelDIR(topdirlist);
		if (topmftlist) DelDIRmft(topmftlist);

		dirlist = NULL;
		topdirlist = NULL;
		mftlist = NULL;
		topmftlist = NULL;

		listlen=0;
		mftlistlen=0;
		cMFTentry = 0;
		MFTentries = 0;
		if (gPARTINFO.PartitionType == 7) {
			char buff[255];
			ULONGLONG MFTZstart=0, MFTZend=0;

			sprintf(buff, "%c:\\", cActualDrive+65);
			ReadMFTRecord(0, buff); // $MFT first to get # of entries

			sprintf(buff, "%c:\\$MFT2", cActualDrive+65);
			MFTZstart = gNTFSDATA.MftZoneStart.QuadPart;
			MFTZend = gNTFSDATA.MftZoneEnd.QuadPart;

			HereMFT = topmftlist;
			while (HereMFT) {
				if (HereMFT->MFTid == 0                     &&
					HereMFT->fragid!=0                      &&
					HereMFT->Lcn + HereMFT->Len > MFTZstart &&
					HereMFT->Lcn + HereMFT->Len < MFTZend      )
				{
					MFTZstart = HereMFT->Lcn + HereMFT->Len;
				}
				if (HereMFT->MFTid == 0                     &&
					HereMFT->fragid!=0                      &&
					HereMFT->Lcn > MFTZstart                &&
					HereMFT->Lcn < MFTZend                     )
				{
					MFTZend = HereMFT->Lcn - 1;
				}
				HereMFT = HereMFT->next;
			}

			HereFILE = topdirlist;
			while (HereFILE) {
				if (HereFILE->MFTid == 0                     &&
					HereFILE->fragid!=0                      &&
					HereFILE->Lcn + HereFILE->Len > MFTZstart &&
					HereFILE->Lcn + HereFILE->Len < MFTZend      )
				{
					MFTZstart = HereFILE->Lcn + HereFILE->Len;
				}
				if (HereFILE->MFTid == 0                     &&
					HereFILE->fragid!=0                      &&
					HereFILE->Lcn > MFTZstart                &&
					HereFILE->Lcn < MFTZend                     )
				{
					MFTZend = HereFILE->Lcn - 1;
				}
				HereFILE = HereFILE->next;
			}

			// MFTZone is always not accessable by normal files, add to mftlist
			InsMFT(buff, (gNTFSDATA.MftZoneEnd.QuadPart - gNTFSDATA.MftZoneStart.QuadPart) *
						  gNTFSDATA.BytesPerCluster, MFTZstart, MFTZend -  MFTZstart, 0, 1, 0, FALSE, FALSE, 0);
			InsDIR(buff, (gNTFSDATA.MftZoneEnd.QuadPart - gNTFSDATA.MftZoneStart.QuadPart) * 
						  gNTFSDATA.BytesPerCluster, MFTZstart, MFTZend -  MFTZstart, 0, 1, 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0);
		}
    }

    // setup buffer
    bitMappings = (PBITMAP_DESCRIPTOR) BitMap;
	hdc = GetDC(ghWnd);
    do {
        if (DeviceIoControl(ghVolume,
                FSCTL_GET_VOLUME_BITMAP,
                &nextLcn, sizeof(ULONGLONG),
                bitMappings, BITMAPSIZE,
                &lpBytesReturned, NULL) == 0 )
        {
            err = GetLastError();
            if (err != ERROR_MORE_DATA) break;
        }

        // Scan through the returned bitmap info, looking for empty clusters
        // starting at offset bitMappings->StartLcn
        startLcn = bitMappings->StartLcn;

        for( i = 0; i < min( bitMappings->ClustersToEndOfVol, 8*BITMAPBYTES); i++ ) {

            if( !(bitMappings->Map[ i/8 ] & BitShift[ i % 8 ])) {
                // Cluster is free -----------------
                if( cluster == LLINVALID ) {
					cluster = startLcn + i;

					if (blocksize == 1)
						DrawClusters(lastCluster, cluster, hpGreen);

                    if (cluster2 == LLINVALID)
                        cluster2 = cluster;
                    numFree = 1;
                    numFree3++;
                } else {
                    numFree++;
                    numFree3++;
                }
            } else {
                // Cluster is not free -----------------
                if( cluster != LLINVALID ) {
                    if( lastLcn == cluster ) {
                        lastLcn = LLINVALID;
                    } else {
                        numFree2 += numFree;
                        if (!bDumped)
                            InsDIRFree(cluster2, numFree2);

						if (blocksize == 1)
							DrawClusters(cluster2, cluster2+numFree2, hpWhite);

                        lastCluster = cluster2+numFree2;
						lastLcn  = cluster;
                        numFree  = 0;
                        numFree2 = 0;
                        cluster  = LLINVALID;
                        cluster2 = LLINVALID;
                    }
                }
            }

            // draw the shit
			if (++clusterC == clustersPB && blocksize > 1) {
                DrawNoBlockBound(x, y);
                if (clustersPB==1 && numFree3<1)
                    DrawBlockAt(hdc, x, y, hbGreen);
                else if (numFree3  >= clustersPB)
                    DrawBlockAt(hdc, x, y, hbWhite);
                else if (numFree3 >              0     &&
                         numFree3 <  (int)(clustersPB/divider)   )
                    DrawBlockAt(hdc, x, y, hbLGreen);
                else if (numFree3 >= (int)(clustersPB/divider)   &&
                         numFree3 <  (int)(clustersPB/divider)*2 )
                    DrawBlockAt(hdc, x, y, hbL2Green);
                else if (numFree3 >= (int)(clustersPB/divider)*2 &&
                         numFree3 <  (int)(clustersPB/divider)*3 )
                    DrawBlockAt(hdc, x, y, hbL2Green);
                else if (numFree3 >= (int)(clustersPB/divider)*3 &&
                         numFree3 <  clustersPB     )
                    DrawBlockAt(hdc, x, y, hbL3Green);
                else
                    DrawBlockAt(hdc, x, y, hbGreen);

                x += blocksize;
                if (x>imgWidth-blocksize) { 
					x=0; y+=blocksize; 
					ReleaseDC(ghWnd, hdc);
					hdc = GetDC(ghWnd);
				}
                clusterC = 0;
                numFree3 = 0;
            }
        } // for all clusters in this pair

        // Move to the next block
        nextLcn = bitMappings->StartLcn + i;

    } while ( (err == ERROR_MORE_DATA) && (nextLcn < cDiskClusters));

    if(clusterC > 0) {
        if (!bDumped && numFree > 0) {
            InsDIRFree(cluster2, numFree);
			lastsort=0;
		}

        // draw last cluster
        if (blocksize > 1) {
		if (numFree3 == clusterC)
            DrawBlockAt(hdc, x, y, hbWhite);
        else if (numFree3 >           0     &&
                 numFree3 <  clusterC/4 )
            DrawBlockAt(hdc, x, y, hbLGreen);
        else if (numFree3 >= clusterC/4*2 &&
                 numFree3 <  clusterC/4*3 )
            DrawBlockAt(hdc, x, y, hbL2Green);
        else
            DrawBlockAt(hdc, x, y, hbGreen);
		} else {
			if (cluster2 != LLINVALID && numFree > 0)
				DrawClusters(cluster2, cluster2+numFree, hpWhite);
			else
				DrawClusters(lastCluster, nextLcn, hpGreen);
		}
	}
	ReleaseDC(ghWnd, hdc);
}

BOOL IsFileInUse(char *fileName) {
    HANDLE sourceFile;
	BOOL ret=FALSE;
   
	OpenTheFile(fileName, &sourceFile);

    if (sourceFile == INVALID_HANDLE_VALUE) {
		int err = GetLastError();

		if (    (err==5)    ||  // acces denied
                (err==32)   ||  // in use by another process
                (err==87)   ||  // wrong parameters
				(err==2)        // file not found
		   )
		    ret = TRUE;
		else if ( (err!=3)  &&   // path not found
				  (err!=87)    ) // wrong parameters
			; //HandleWin32Error(err, "");
	}
    CloseHandle(sourceFile);
    return ret;
}

// walk through the clusters of a file and insert into dirlist
void DumpFile(char *fileName, __int64 Bytes) {
    HANDLE                      sourceFile;
    PGET_RETRIEVAL_DESCRIPTOR   fileMappings;
    ULONGLONG                   startVcn=0;
    ULONGLONG                   numClusters=0;
    ULONGLONG                   sLcn=LLINVALID, sLen=0;
    ULONGLONG                   lastLcn=LLINVALID, lastLen=LLINVALID;
    ULONGLONG                   frags=0;
    int                         i;
    BOOL                        compressed = FALSE;
    struct FILEstruct           *firstFrag=NULL;
    char                        buff[512];
    DWORD                       lpBytesReturned;
    DWORD                       err;

	OpenTheFile(fileName, &sourceFile);

    if( sourceFile == INVALID_HANDLE_VALUE ) {
		// could NOT open, make the file locked
		InsDIR(fileName, Bytes, LLINVALID, LLINVALID, 0, 1, 0, FALSE, FALSE, FALSE, FALSE, TRUE, IINVALID);
		cLocked++;
		sprintf(buff, "%s files (%d locked)",
		fmtNumber(cFiles, buff), cLocked);
		UpdateStatusBar(buff, 2, 0);
	    return;
	}

    // setup buffer
    fileMappings = (PGET_RETRIEVAL_DESCRIPTOR) FileMap;

    // dump mapping information of file until EOF
    do {
        if (DeviceIoControl(sourceFile,
                FSCTL_GET_RETRIEVAL_POINTERS,
                &startVcn, sizeof(startVcn),
                fileMappings, sizeof(GET_RETRIEVAL_DESCRIPTOR),
                &lpBytesReturned, NULL) == 0 )
        {
            err = GetLastError();
            if (err != ERROR_MORE_DATA) break;
        }

        // Loop through the buffer of number/cluster pairs
        startVcn = fileMappings->StartVcn;
        for( i = 0; i < (ULONGLONG) fileMappings->NumberOfPairs; i++ ) {
            // On NT 4.0, a compressed virtual run (0-filled) is
            // identified with a cluster offset of -1
            if( fileMappings->Pair[i].Lcn == LLINVALID ) {
                compressed = TRUE;
            } else {
                // number of clusters from file in this pair[i]
                numClusters = fileMappings->Pair[i].Vcn - startVcn;

                // test this cluster if it follows the lastLcn
                // this also happens on the first one because lastLcn is 0
                if ( lastLcn + lastLen != fileMappings->Pair[i].Lcn && !compressed )
                    frags++;

                if (frags==2)
					// make the last fragment fragmented
					InsDIR(fileName, Bytes, lastLcn, lastLen, sLen-lastLen, 0, frags-1, FALSE, TRUE, FALSE, FALSE, FALSE, IINVALID);

                if (frags>=2 && fileMappings->Pair[i].Lcn != LLINVALID && ghAnalyze)
					// the new fragment
					InsDIR(fileName, Bytes, fileMappings->Pair[i].Lcn, numClusters, sLen, 0, frags, FALSE,  TRUE, FALSE, FALSE, FALSE, IINVALID);

                // now remember actual cluster for fragment-recognizing
                lastLcn = fileMappings->Pair[i].Lcn;
                lastLen = numClusters;   // on NTFS?
                //lastLen = numClusters-1; // on FAT?

                if (sLcn == LLINVALID) sLcn = lastLcn; // remember startlcn of the file
                sLen += numClusters;
            }

            // goto next fragment of file
            startVcn = fileMappings->Pair[i].Vcn;

			if (bStopAction) break;
        } // for
    } while ((err == ERROR_MORE_DATA) && (startVcn < cDiskClusters));

    CloseHandle( sourceFile );

    if (ghAnalyze) {
        if (frags > 1 && sLcn!=LLINVALID) {
            // fragments have a zero ->fragments value
            cFragmented++;
            cFragmentsAllTogether+=(unsigned long)frags;

			// make main entry in dirlist for a fragmented file
			// with frag-count, starting LCN of first frag and length of
			// whole file
            InsDIR(fileName, Bytes, sLcn, sLen, 0, frags, 0, FALSE, TRUE, FALSE, FALSE, FALSE, IINVALID);

            sprintf(buff, "%d fragmented", cFragmented);
            UpdateStatusBar(buff, 3, 0);
		} else {
			InsDIR(fileName, Bytes, sLcn, sLen, sLen-lastLen, 1,     1, FALSE, FALSE, FALSE, FALSE, FALSE, IINVALID);
		}
	}
}

// read in whole dirtree
void ReadTreeFAT(char *Path) {
    HANDLE FindHandle;
    WIN32_FIND_DATA FFData;
    char searchPath[512];
    char FileName[512];
    char buff[512];
    __int64 Bytes;

    sprintf(searchPath, "%s*", Path);
    FindHandle = FindFirstFile(searchPath, &FFData);
    if (FindHandle == INVALID_HANDLE_VALUE) return;
    do {
        // wait for draw or break
        if (CheckActivities()) break;

        if (strcmp(FFData.cFileName,".") == 0) continue;
        if (strcmp(FFData.cFileName,"..") == 0) continue;

        if ((FFData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            // it is a dir
            sprintf(FileName, "%s%s\\", Path, FFData.cFileName);

            cDirs++;
            if (subDirLevel == 0) 
				cRootDirs++;
            subDirLevel++;

            if (subDirLevel<12) {
                sprintf(buff, "Analyzing %s", FileName);
                OutLine1(buff);
                //UpdateStatusBar(buff, 4, 0);
            }

            if (cDirs % 64 == 0) {
                // Update the status bar
                sprintf(buff, "%s dirs", fmtNumber(cDirs, buff));
                UpdateStatusBar(buff, 1, 0);
            }

            // get LCN, LEN, fragments
            DumpFile(FileName, IINVALID);

            // recurse Call to ReadTree
            ReadTreeFAT( FileName ); //

        } else {
            // it is a file
			sprintf(FileName,"%s%s", Path, FFData.cFileName);
            Bytes = FFData.nFileSizeHigh *
            ((LONGLONG)MAXDWORD + 1) + FFData.nFileSizeLow;

            cFiles++;
            cBytes+=Bytes;

            if (cFiles % 256 == 0) {
                // Update the status bar
                if (cLocked>0)
                    sprintf(buff, "%s files (%d locked)", fmtNumber(cFiles, buff), cLocked);
                else
                    sprintf(buff, "%s files", fmtNumber(cFiles, buff));
                UpdateStatusBar(buff, 2, 0);
                sprintf(buff, "%s byte", fmtNumber(cBytes, buff));
                UpdateStatusBar(buff, 4, 0);
            }

            // Show progress
            progress = (long int)( ((float)(cBytes)/(float)(cBytesUsed))*imgWidth );
            if (progress > lastprogress) {
                lastprogress = progress;
                DrawProgress(progress);
            }

            DumpFile(FileName, Bytes);
        }
    } while (FindNextFile(FindHandle,&FFData) != 0);

    subDirLevel--;
    FindClose(FindHandle);
}

void DrawSpecialFiles() {
    struct MFTstruct *MHere=NULL;
    struct FILEstruct *Here=NULL;
	unsigned long c=0;

	// draw MFT-stuff
	MHere = topmftlist;
	while (MHere) {
		if (MHere->fragmented) {
			if (MHere->fragid>=1 && MHere->fragments == 0)
			    DrawBlocks(MHere->Lcn, MHere->Lcn + MHere->Len, hbDVio, hpDVio);
		} else if (MHere->fragid>=0 && MHere->fragments<=1)
			DrawBlocks(MHere->Lcn, MHere->Lcn+MHere->Len, hbVio, hpVio);
		MHere = MHere->next;
	}

	Here = topdirlist;
    // draw special files
    while (Here) {
        c++;
        if (Here->fragmented                  && 
            Here->fragid>0                    &&
			((Here->MFTid > 26 && Here->MFTid!=IINVALID &&
			  gPARTINFO.PartitionType == 7) 
			|| gPARTINFO.PartitionType != 7) &&
            Here->fragments!=LLINVALID        &&
            Here->fragments == 0                 ) 
		{
			if (Here->filesize > sizeLimit && !Here->sparse) 
			    DrawBlocks(Here->Lcn, Here->Lcn + Here->Len, hbDRed, hpDRed);
			else if (!Here->sparse)
			    DrawBlocks(Here->Lcn, Here->Lcn + Here->Len, hbRed, hpRed);
			else if (Here->compressed && !Here->sparse) // compressed
				DrawBlocks(Here->Lcn, Here->Lcn + Here->Len, hbOck, hpOck);
		} else {
			if (Here->locked)     // locked files
				DrawBlocks(Here->Lcn, Here->Lcn + Here->Len, hbBrown, hpBrown);
			else if (Here->filesize == IINVALID &&
				!Here->fragmented               && // this is a dir
				!Here->sparse)                    
				DrawBlocks(Here->Lcn, Here->Lcn + Here->Len, hbBlue, hpBlue);
			else if (Here->compressed && Here->fragments != 0 && !Here->sparse) // compressed
				DrawBlocks(Here->Lcn, Here->Lcn + Here->Len, hbOck, hpOck);
			else if (Here->filesize>sizeLimit   && // above sizeLimit
					( (Here->MFTid > 26           && 
					   Here->MFTid!=IINVALID      &&
					   gPARTINFO.PartitionType==7    )
					|| gPARTINFO.PartitionType!= 7     ) &&
					 !Here->locked              &&
					 !Here->fragmented          &&
					 !Here->free                &&
					 !Here->sparse                 )
				DrawBlocks(Here->Lcn, Here->Lcn + Here->Len, hbDGreen, hpDGreen);
		}
        Here = Here->next;
    }

    // do the marks if neccessary
    if (blocksize > 1 && bShowMarks)
        (void*)_beginthread(DrawAllMarks, 0, NULL);

    DrawProgress(-1);
    OutLine1("");
}

void DrawAllMarks(void *d) {
    struct FILEstruct *Here=topdirlist;
    if (bShowMarks)
	while (Here) {
		if (Here->Len > clustersPB*1     &&    // large files
            Here->fragments <= 1         &&    // and not the description-entry of fragmented files
            !Here->free                     )  // and not free space
        DrawMarks(Here->Lcn, Here->Lcn + Here->Len);
        Here = Here->next;
    }
}

// draw layout of volume
void DrawBitMap(void *d) {
    char laststr[512];

    lstrcpy(laststr, LastOutLine);
    OutLine1("Drawing Disk Layout...");

    // draw diskmap
	DumpBitmap();
	DrawSpecialFiles();

    DrawProgress(-1);
    OutLine1(laststr);

    if (IsWindowVisible(ghListWnd) && blocksize > 1)
        DrawBlockBound(xposlast, yposlast);
    ghDraw = NULL;
}

// write the dirlist to file
void DumpLayoutToFile(void *d) {
    FILE *logfile;
    struct FILEstruct *Here;
    char buff[1024], buff2[50];
    unsigned long c=0;

    DrawProgress(-1);

	// wait for analyze
	if ( ghAnalyze )
		WaitForSingleObject(ghAnalyze, INFINITE);

    sprintf(buff2, "%c:\\defragger.txt", cActualDrive+65);
    sprintf(buff, "Exporting Dirlist to file %s", buff2);
    OutLine1(buff);
	buff[0]=0;

    logfile = fopen(buff2, "w");
    
    if (gPARTINFO.PartitionType == 7)
		fprintf(logfile, "MFTID      StartLcn   EndLcn     Len        frags/id   FileSize   dirname/filename\n");
	else
		fprintf(logfile, "StartLcn   EndLcn        Len           frags/id      FileSize      dirname/filename\n");
    
    progress = 0; lastprogress = 0;
	bUpdating = TRUE;
    //Here = topdirlist;
    //Here = topdirlist = SortDict(topdirlist, listlen, byLcnAsc);
    Here = topdirlist = SortDict(topdirlist, listlen, byName);
	while (Here) {
        if (bStopAction) break;

		progress = (long int)( (float)c++/(float)(listlen*imgWidth)*1000000);
        if (progress > lastprogress) {
            lastprogress = progress;
            DrawProgress(progress);
			//fflush(logfile);
        }

        // only fragmented
		//if (Here->fragmented && Here->fragid<=1 && Here->fragments>=1) 
			// only the filename
		    //sprintf(buff, "%10I64u %s\n", Here->MFTid, Here->fileName);

		/**/
		if (Here->Lcn != LLINVALID && !Here->free)
			if (gPARTINFO.PartitionType == 7)
                if (Here->MFTid == IINVALID) //file stored directly in MFT
					sprintf(buff, "         0          0          0          0          0 %10I64d %s\n",
		                    Here->filesize, Here->fileName);
				else if (Here->fragid>0)    // a fragment
						sprintf(buff, "%10I64u %10I64u %10I64u %10I64u %10I64u %10I64d %s\n",
		                        Here->MFTid, Here->Lcn, Here->Lcn+Here->Len, Here->Len,
							    Here->fragid, Here->filesize, Here->fileName);
				else if (Here->free)
						sprintf(buff, "         - %10I64d %10I64d %10I64d         - %10I64d ---free space---\n",
								Here->Lcn, Here->Lcn+Here->Len, Here->Len, Here->filesize);
			    else // a file (fragmented or not)
						sprintf(buff, "%10I64u %10I64u %10I64u %10I64u %10I64u %10I64d %s\n",
								Here->MFTid, Here->Lcn, Here->Lcn+Here->Len, Here->Len,
								Here->fragments, Here->filesize, Here->fileName);
			else
				if (Here->fragid>0)  // a fragment
					sprintf(buff, "%10I64u %10I64u %10I64u %10I64u %12I64d %s\n",
						Here->Lcn, Here->Lcn+Here->Len, Here->Len,
						Here->fragid, Here->filesize, Here->fileName);
				else                 // a file (fragmented or not)
					sprintf(buff, "%11I64u %10I64u %10I64u %10I64u %12I64d %s\n",
						Here->Lcn, Here->Lcn+Here->Len, Here->Len,
						Here->fragments, Here->filesize, Here->fileName);
		else if (Here->free)
            if (gPARTINFO.PartitionType == 7)
				sprintf(buff, "         - %10I64d %10I64d %10I64d          - %10I64d ---free space---\n",
                        Here->Lcn, Here->Lcn+Here->Len, Here->Len, Here->filesize);
			else
				sprintf(buff, "%10I64d %10I64d %10I64d          -   %10I64d ---free space---\n",
                        Here->Lcn, Here->Lcn+Here->Len, Here->Len, Here->filesize);
        else if (Here->filesize == IINVALID)
            if (gPARTINFO.PartitionType == 7)
	            sprintf(buff, "%10I64d %10I64d %10I64d %10I64d %10I64d          0 %s\n",
		            Here->MFTid, Here->Lcn, Here->Lcn+Here->Len, Here->Len,
			        Here->fragments, Here->fileName);
			else
	            sprintf(buff, "%10I64d %10I64d %10I64d %10I64d          0 %s\n",
		            Here->Lcn, Here->Lcn+Here->Len, Here->Len,
			        Here->fragments, Here->fileName);
        else
            if (gPARTINFO.PartitionType == 7)
				sprintf(buff, "%10I64d %10I64d %10I64d %10I64d %10I64d %10I64d %s\n",
					Here->MFTid, Here->Lcn, Here->Lcn+Here->Len, Here->Len,
					Here->fragments, Here->filesize, Here->fileName);
			else
				sprintf(buff, "%10I64u %10I64u %10I64u %10I64u %10I64d %s\n",
					Here->Lcn, Here->Lcn+Here->Len, Here->Len,
					Here->fragments, Here->filesize, Here->fileName);
		/**/

		if (buff[0] != 0 && buff != NULL) {
			int i=0;
			while (buff[i]) fputc(buff[i++], logfile);
			//fprintf(logfile, buff);
			fflush(logfile);
			buff[0]=0;
		}
        Here = Here->next;
    }
    fclose(logfile);
    bUpdating = FALSE;
    DrawProgress(-1);
    OutLine1("");
    progress=0;
}

// ...
void AnalyzeVolume(void *d) {
    char buff[512];
	int i;

    // maybe there has changed something
    GetPartitionInfo();
    GetPartitionInfoDetails();

	sprintf(buff, "%c:\\", cActualDrive+65);
	cRootDirs=0; cDirs=0; cFiles=0; cBytes=0;
	cLocked=0; cCompressed=0; cFragmented=0; cFragmentsAllTogether=0;
	subDirLevel=0;
	progress=0, lastprogress=0;
	lastsort=0;

	bAnalyzed     = FALSE;
	bDumped       = FALSE;

	// draw empty diskmap
	DumpBitmap();
	bDumped       = TRUE;

	DrawSpecialFiles();

	sprintf(buff, "%c:\\", cActualDrive+65);
	// create dirlist - analyze dirs and files
	if (gPARTINFO.PartitionType == 7) {
		// NTFS-file-systems

		/*       //counterinit-start
		LARGE_INTEGER QFreq, QCountStart, QCountEnd;
		QueryPerformanceFrequency(&QFreq);
		QueryPerformanceCounter(&QCountStart);
		*/       //counterinit-end

		/**/       // "MFT"-version start
		// read the MFT recursiv/rekursiv - starting from root-MFT #5
		ReadMFTRecord(5, buff); 
		/**/       // "MFT"-version end

		/*     //counter-start
		QueryPerformanceCounter(&QCountEnd);
		{
			char buff2[30];
			float diff;
			diff = ((float)QCountEnd.QuadPart - (float)QCountStart.QuadPart) / (float)QFreq.QuadPart;
			sprintf(buff2, "%.3f sec.", diff);
			OutLine0(buff2);
		}
		*/     //counter-end

	} else {
		// FAT-file-systems
		DumpFile(buff, IINVALID); // root dir
  		ReadTreeFAT(buff);
	}
	lastsort=0; progress=0, lastprogress=0;
	topdirlist = SortDict(topdirlist, listlen, byLcnDesc);

	if (!bStopAction)
		bAnalyzed = TRUE;

	if ( bStopAction ) {
		bAnalyzed = FALSE;
		// draw diskmap
		DumpBitmap();
		DrawSpecialFiles();

		ResetStatusBar();
		UpdateStatusBar("Analyze stopped", 4, 0);
	} else {
		cDirsFiles = cDirs + cFiles;
		DrawSpecialFiles();

		// Update the status bar
		sprintf(buff, "%s dirs", fmtNumber(cDirs, buff));
		UpdateStatusBar(buff, 1, 0);
		if (cLocked>0)
			sprintf(buff, "%s files (%d locked)", fmtNumber(cFiles, buff), cLocked);
		else
			sprintf(buff, "%s files", fmtNumber(cFiles, buff));
		UpdateStatusBar(buff, 2, 0);
		sprintf(buff, "%d fragmented", cFragmented);
		UpdateStatusBar(buff, 3, 0);
		sprintf(buff, "%s byte", fmtNumber(cBytes, buff));
		UpdateStatusBar(buff, 4, 0);
	}
	DrawProgress(-1);
	OutLine1("");

    // re-enable menu
    EnableMenus(0);
    ghAnalyze = NULL;

    if (IsWindowVisible(ghListWnd)) {
        GetWindowText(ghListWnd, buff, 512);
        if (strstrlwr(buff, "Drive Information") >= 0)
            UpdateDriveInformation();
        else
			(void*)_beginthread(UpdateClusterList, 0, NULL);
            //UpdateClusterList();
    }
}

void MakeSpace(ULONGLONG MinLcn, ULONGLONG MinLen, BOOL gapsAllowed) {
    struct FILEstruct *Here;
	ULONGLONG lenMoved=0;
	ULONGLONG ClusterStart=0, ClusterEnd=0, Clusters=0;

    // wait for possible drawing
    if (!CheckActivities()) {
		Here = topdirlist = SortDict(topdirlist, listlen, byLcnAsc);
		while (Here && lenMoved < MinLen && !CheckActivities()) {
			if (Here->fragid>=1              && 
				Here->filesize != IINVALID   &&
				Here->Lcn < MinLcn           &&
				!Here->locked                &&
                !Here->free
			   )
			{
				Clusters = GetFreeClusters(MinLcn, Here->Len, &ClusterStart, &ClusterEnd, TRUE);
				if (Clusters == 0) break;
				if (MovePartOfFile(Here, ClusterStart) == Here->Len)
					lenMoved+=Here->Len;

				if (CheckActivities()) break;

			} else if (Here->Len != LLINVALID)
				MinLcn+=Here->Len;
			Here = Here->next;
		}
	}
}

void DefragDirectories(void *d) {
    struct FILEstruct *Here;
	ULONGLONG lenToMove=0, lenToMove2=0;
	ULONGLONG ClusterStart=0, ClusterEnd=0, Clusters=0;

    // wait for possible drawing
    if (!CheckActivities()) {

		// count space for all dir-entries
		Here = topdirlist = SortDict(topdirlist, listlen, bySizeAsc);
		while (Here) {
			if (Here->filesize == IINVALID && 
				Here->fragments!=0         &&
				Here->Len != LLINVALID     && 
				Here->Lcn > lenToMove2         ) 
				lenToMove2+=Here->Len;
			Here = Here->next;
		}

		// count space for all dir-entries
		Here = topdirlist;
		while (Here) {
			if (Here->filesize == IINVALID && 
				Here->Len != LLINVALID     && 
				Here->Lcn > lenToMove2        ) 
				lenToMove+=Here->Len;
			Here = Here->next;
		}

		// make room for the dir-entries begin from start of partition
		MakeSpace(lenToMove + (lenToMove/8), lenToMove + (lenToMove/8), TRUE);

		// move the dir-entries to free space
		Here = topdirlist;
		ClusterStart = lenToMove;
		while (Here) {
			if (Here->filesize == IINVALID   &&
				//!Here->compressed            &&
				//!Here->sparse                &&
				Here->Lcn != LLINVALID          ) 
			{
				Clusters = GetFreeClusters(0, Here->Len, &ClusterStart, &ClusterEnd, FALSE);
				if (Clusters == 0) break;

				MoveFileNormal(Here, ClusterStart);
			}
			Here = Here->next;
		}
	}		
}

void DefragFragments(void *d) {
    struct FILEstruct *Here;
    char buff[512];
    unsigned long c=0;
    ULONGLONG ClusterStart=0, ClusterEnd=0;
    ULONGLONG lastLcn=0, MinLcn=0, lenMoved=0;

    // wait for possible drawing
    if (!CheckActivities()) {

        UpdateStatusBar("Defragging...", 4, 0);

        progress=0; lastprogress=0;

		if (gPARTINFO.PartitionType==7)
			DefragDirectories(NULL);
				  
		//Here = topdirlist = SortDict(topdirlist, listlen, byLcnDesc);
        Here = topdirlist = SortDict(topdirlist, listlen, bySizeAsc);
        //Here = topdirlist = SortDict(topdirlist, listlen, bySizeDesc);
        while (Here && cFragmented>0 ) {
            progress = (long int)( (float)c++/(float)(listlen)*imgWidth );
            if (progress > lastprogress) {
                lastprogress = progress;
                DrawProgress(progress);
                Sleep(0);
            }
		    if (Here->fragmented                  && 
				Here->filesize < sizeLimit        &&
                Here->fragid<=1                   && 
			    Here->fragments>=1                &&
				( (Here->MFTid > 26 && Here->MFTid!=IINVALID &&
				   gPARTINFO.PartitionType == 7) 
				 || gPARTINFO.PartitionType != 7) &&
				!Here->locked
				)
			{
                lenMoved += MoveFileFragmented(Here, &Here->Lcn);
            } // if

           	if (CheckActivities()) break;
            Here = Here->next;
		} // while Here

		if (!bStopAction && cFragmented>0) {
			Here = topdirlist = SortDict(topdirlist, listlen, byName);
			while (Here) {
				if (Here->fragid>=1              && 
					Here->filesize != IINVALID   &&
					Here->fragmented             &&
					!Here->sparse                &&
					!Here->compressed            &&
					!Here->locked                &&
					!Here->free
				   )
				{
					if (GetFreeClusters(MinLcn, Here->Len, &ClusterStart, &ClusterEnd, TRUE) < Here->Len) 
						break;
					if (MovePartOfFile(Here, ClusterStart) == Here->Len)
						lenMoved+=Here->Len;
						MinLcn=lenMoved;

					if (CheckActivities()) break;
				} else if (Here->Len != LLINVALID)
					MinLcn+=Here->Len;
				Here = Here->next;
			}
		}
	
	} // if (CheckActivities()) break;

    DrawProgress(-1);
    ghDefrag = NULL;

    // re-enable menu
    EnableMenus(1);

    sprintf(buff, "");
    if (FilesMoved > 0) {
        sprintf(buff, "%s file(s) moved", fmtNumber(FilesMoved, buff));
        ResetStatusBar();

        bAnalyzed = FALSE;
        
		// draw disk-layout
        if (!bStopAction) DrawBitMap(NULL);
        // do an new Analyze
        //SendMessage(ghWnd, WM_COMMAND, IDM_ANALYZE, 0);
    }
    else if (cFragmented!=0)
        sprintf(buff, "dirs or files above limit fragmented.");
    else if (cFragmented==0)
        sprintf(buff, "Nothing to defrag.");

    OutLine1(buff);
    UpdateStatusBar("DefragFragmented finished", 4, 0);
}

void DefragFreeSpace(void *d) {
    struct FILEstruct *Here, *BestFit;
    ULONGLONG ClusterStart=0, ClusterEnd=0, Clusters=1;
    ULONGLONG fsCS=0, fsCE=0; // freeSpace clusterindicators
    ULONGLONG cfCS=0, cfCE=0; // currentfile clusterindicators
    ULONGLONG lokalCS=0, lokalCE=0;
    char buff[512];
    int i;
    unsigned long c=0;

    if (!CheckActivities()) {
        UpdateStatusBar("Preparing Optimization...", 4, 0);
        progress=0, lastprogress=0;

        // normal files starting from end of volume
        lastsort=0;
		//Here = topdirlist = SortDict(topdirlist, listlen, byLcnDesc);
        Here = topdirlist = SortDict(topdirlist, listlen, byLcnAsc);

        // first run
        while (1) {
            // find the next block with free space
            Clusters = GetFreeClusters(fsCS, 1, &ClusterStart, &ClusterEnd, FALSE);
            if (Clusters == 0) break;
            fsCS = ClusterStart;
            cfCS = ClusterStart;

            i = 0;
            while (ClusterStart <= ClusterEnd && Clusters > 0) {
                if (CheckActivities()) break;

                c=0; lastprogress=0;
                BestFit = NULL;
                
  				// Find a file above first free Cluster
                Here = topdirlist;
                while ( Here ) {
                    if (CheckActivities()) break;
					if ((Here->filesize >= 0              ||
			             (Here->filesize == IINVALID && 
					      gPARTINFO.PartitionType == 7  )    )  &&    // skip dirs on FAT
						Here->filesize < sizeLimit              &&    // limit filesize in bytes
						Here->Lcn >= cfCS                       &&    // a file above current ClusterStart
						Here->Len > 0                           &&    // skip zero byte files
						Here->Len <= Clusters                   &&    // skip too large files
						! Here->locked                          &&
						! Here->free                            &&
						! Here->fragmented
					)                    
                    {
                        if (CheckActivities()) break;
                        if (BestFit == NULL)
							BestFit = Here;
                        if (Here->filesize == IINVALID &&
							gPARTINFO.PartitionType == 7) 
						{
                            if (BestFit->filesize != IINVALID ||
							    Here->Len >= BestFit->Len)
								BestFit = Here;
                        } else {
							if (BestFit->filesize != IINVALID &&
							    Here->Len >= BestFit->Len)
								BestFit = Here;
                        }
                    }
                    c++;
                    Here = Here->next;
                }
                Here = BestFit;
                if (Here == NULL) 
					break;

                // set new currentFile clusterStart
                //if (Here->Lcn > 0 && Here->Lcn != LLINVALID)
                //    cfCS = Here->Lcn + Here->Len;

                // skip wrong files
                if (Here->Len > Clusters-1 &&
                    Here->Lcn+Here->Len < ClusterStart)
                    break;

                if (CheckActivities()) break;
                if (MoveFileNormal(Here, ClusterStart) == Here->Len) {
		            if (CheckActivities()) break;
                    FilesMoved++;
                    Here->Lcn = ClusterStart;
                    Clusters -= Here->Len;
                    fsCS += Here->Len;
                    cfCS += Here->Len;
                    ClusterStart = Here->Lcn+Here->Len;
                    //InsDIRFree(Here->Lcn, Here->Len);
                    //Here = topdirlist = SortDict(topdirlist, listlen, byLcnDesc);
                }
                else break;

                //cBound = ClusterStart + Here->Len;
                //cfCS = cfCS + Here->Len;
                i++;
            }
            // while
            if (i > 0) continue;

////////////////////////////////////
			/**/
			i = 0;
            while (1) {
                // Find a File above ClusterEnd
                Here = topdirlist;
                while ( Here ) {
	                if (CheckActivities()) break;
					if (fsCS>=cDiskClusters) break;
                    
					if ((Here->filesize >= 0              ||
			             (Here->filesize == IINVALID && 
					      gPARTINFO.PartitionType == 7  )    )  &&    // skip dirs on FAT
						Here->Len <= Clusters         &&
						Here->filesize < sizeLimit    && // limit filesize in bytes
						Here->Lcn >= cfCS             && // a file behind the free cluster
						Here->Len > 0                 && // skip zero byte files
						!Here->locked                 && // skip locked files
						!Here->free                   //&&
						//!Here->fragmented
				       ) break;
                    Here = Here->next;
                }
                if (Here == NULL) {
                    fsCS = cfCS;
                    //ClusterStart = ClusterEnd+1;
                    break;
                }

                if (Here->Len > 100) {
                    // move the file away
                    if (CheckActivities()) break;

                    if (GetFreeClusters(0, Here->Len,
                        &lokalCS, &lokalCE, TRUE) >= Here->Len)
                    {
                        if (CheckActivities()) break;
						
                        if (MovePartOfFile(Here, lokalCS) == Here->Len)
                        {
							if (CheckActivities()) break;
                            FilesMoved++; i++;
                            Here->Lcn = lokalCS;
                            if (lokalCS == ClusterStart)
                                ClusterStart = lokalCS + Here->Len;
                            //Clusters -= Here->Len;
                            //cfCS = Here->Lcn+Here->Len;
                            //InsDIRFree(Here->Lcn, Here->Len);
                            //Here = topdirlist = SortDict(topdirlist, listlen, byLcnDesc);
                            //lokalCS = Here->Lcn + Here->Len;
                            //ClusterEnd = ClusterEnd + Here->Len;
                            break;
                        }
                        else
                            ;
                        break;
                    }
                }

                if (CheckActivities()) break;

                if (MovePartOfFile(Here, ClusterStart) == Here->Len)
                {
		            if (CheckActivities()) break;
                    FilesMoved++;
                    Here->Lcn = ClusterStart;
                    Clusters -= Here->Len;
                    fsCS = ClusterStart + Here->Len;
                    //ClusterStart += Here->Lcn+Here->Len;
                    //InsDIRFree(Here->Lcn, Here->Len);
                    //Here = topdirlist = SortDict(topdirlist, listlen, byLcnDesc);
                }
                else break;

                //cfCS = cfCS + Here->Len;
                //ClusterStart = ClusterStart + Here->Len;
                //ClusterEnd = ClusterEnd + Here->Len;

                i++;
            } // while find a file above
			/**/
////////////////////////////////////

            if (CheckActivities()) break;

            if (i > 0) continue;
            fsCS = fsCS + 1;
            //ClusterStart = ClusterEnd;
        } // while 1
    }

    ghDefrag = NULL;

    // re-enable menu
    EnableMenus(2);

    sprintf(buff, "");
    if (FilesMoved > 0) {
        sprintf(buff, "%s file(s) moved", fmtNumber(FilesMoved, buff));
        ResetStatusBar();

        bAnalyzed = FALSE;
        // draw disk-layout
		if (!bStopAction) DrawBitMap(NULL);
    }
    OutLine1(buff);
    UpdateStatusBar("DefragFreeSpace finished", 4, 0);
}

// this function is why I made this prog
// complicated ... and not ready
void DefragByDirectories(void *d) {
    struct FILEstruct *Here, *AList, *ALtop, *ALthisdir=NULL;
    ULONGLONG ClusterStart=0, ClusterEnd=0, Clusters=0;
    ULONGLONG al_size, endDirLcn=0, lastDirLcn=0, lastDiskLcn=0;
    ULONGLONG dirsize=0, dirsizecounter=0;
    ULONGLONG cc=0;
    int i, lenDir;
    char buff[512];
    char DirToMove[512], DirToMoveLast[512], DirToMoveFirst[512];
    unsigned long RootDirsCounter=0;

    // wait for possible drawing
    if (!CheckActivities()) {

        UpdateStatusBar("Optimizing...", 4, 0);
        progress=0, lastprogress=0;

        // debug - set only this dir as working
        if (cActualDrive == cWinDir[0]-65) {
            lstrcpy((LPTSTR)DirToMoveFirst, (LPCTSTR)cWinDir);
        } else {
            //InputBox MessageBox
            lstrcpy((LPTSTR)DirToMoveFirst, (LPCTSTR)"C:\\WIN98\\");
        }

        lstrcpy((LPTSTR)DirToMove, (LPCTSTR)DirToMoveFirst);
        lstrcpy((LPTSTR)DirToMoveLast, (LPCTSTR)DirToMoveFirst);
		lenDir = strlen(DirToMove);

        // copy to a new dirlist for better handling
        AList = ALtop = SortDict(CopyDIR(topdirlist), listlen, byName);
        Here = topdirlist = SortDict(topdirlist, listlen, byLcnAsc);
        while (AList) {
            if (CheckActivities()) break;

            if (RootDirsCounter==cRootDirs) break;

            // check if this dir is finished
            if ( strnicmp(DirToMoveFirst, DirToMove, strlen(DirToMove)) != 0) {
                while (AList) {
                    if (//AList->filesize == IINVALID &&
                    strnicmp(DirToMoveLast, AList->fileName,
                    strlen(DirToMoveLast) ) != 0 ) {
                        // dir is finished - build next path
                        for (i=0; ; i++) {
                            DirToMove[i] = AList->fileName[i];
                            if ((DirToMove[i]=='\\' || DirToMove[i]=='\0')
                            && i>3) {
                                DirToMove[i+1] = '\0';
                                break;
                            }
                            ;
                            //strrchr(string, ch) - string + 1;
                        }

                        if (strnicmp(DirToMove, DirToMoveLast, lenDir) != 0) {
                            dirsizecounter = 0;
                            break; // found next dir
                        }
                    }
                    AList = AList->next;
                }
            }
            ALthisdir = AList;

            RootDirsCounter++;
            lstrcpy(DirToMoveLast, DirToMove);
            lenDir = strlen(DirToMove);

            // count size of DirToMove
            dirsize = 0;
            AList = ALtop;
            while (AList) {
                if (//AList->filesize != IINVALID   && // skip dirs
                AList->Len > 0                && // skip zero byte files
                !AList->locked                && // skip locked
                !AList->free                  &&
                !AList->fragmented            && // skip fragmented
                !AList->compressed            && // skip compressed
                strnicmp(AList->fileName, DirToMove, lenDir) == 0 ) {
                    dirsize += AList->Len;
                }
                AList = AList->next;
            }

            Clusters = 0; ClusterStart = ClusterEnd = 0;
            Clusters = GetFreeClusters(ClusterStart, 1,
            &ClusterStart, &ClusterEnd, TRUE);
            lastDirLcn = ClusterStart;

            // the cluster, after which to put moved files
            endDirLcn += dirsize;
            lastDiskLcn = endDirLcn;

            // find files that match DirToMove & lastLcn
            AList = ALtop;
            while (AList) {

                // filter out files for moving
                if (//AList->filesize != IINVALID   && // skip dirs
                    AList->Len > 0               && // skip zero byte files
                    !AList->locked               && // skip locked
                    !AList->free                 &&
                    AList->Lcn > lastDirLcn      &&
                    //AList->Lcn > lastDiskLcn      &&
                    strnicmp(AList->fileName, DirToMove, lenDir) == 0 )
                {

                    al_size = AList->Len;

                    ClusterEnd = lastDirLcn;
                    Clusters = 0;
                    while (Clusters<AList->Len && ClusterStart != LLINVALID) {
                        ClusterStart = ClusterEnd;
                        Clusters = GetFreeClusters(ClusterStart, AList->Len,
                        &ClusterStart, &ClusterEnd, TRUE);
                    }

                    if (AList->filesize == IINVALID && gPARTINFO.PartitionType != 7 ) {
                        AList = AList->next;
                        continue;
                    }

                    if (ClusterStart > AList->Lcn) {
                        AList = AList->next;
                        continue;
                    }

                    if ( Clusters >= AList->Len   &&
                         ClusterStart < endDirLcn     )
                    {
                        // enough space for requested file
                        if (CheckActivities()) break;

                        if (MoveFileNormal(AList, ClusterStart) == AList->Len)
                            FilesMoved++;

						if (CheckActivities()) break;

                        AList->Lcn = ClusterStart;
                        //InsDIRFree(AList->Lcn, AList->Len);
                        //Here = topdirlist = SortDict(topdirlist, listlen, byLcnDesc);
                        if (lastDirLcn == AList->Lcn) {
                            lastDirLcn = AList->Lcn + AList->Len;
                            DrawBlocks(lastDirLcn, lastDirLcn+1, hbBlack, hpBlack);
                        }
                        AList = ALtop;
                    } else {
                        // move files from actual position to somewhere else
                        Here = topdirlist;
                        while (Here) {
                            if (Here->Lcn >= lastDiskLcn     &&
                                //Here->Lcn <  endDirLcn       &&
                                Here->filesize < sizeLimit   && // limit filesize in bytes
                                Here->Len > 0                && // skip zero byte files
                                Here->filesize != IINVALID   && // skip dirs
                                !Here->locked                && // skip locked files
                                ! Here->free                 &&
                                strnicmp(Here->fileName, DirToMove, lenDir) != 0 )
                            {
                                if (CheckActivities()) break;

                                if (Here==NULL) break;
                                if (Here->filesize == IINVALID && gPARTINFO.PartitionType != 7 ) {
                                    Here = Here->next;
                                    continue;
                                }

                                // from end of dirsize
                                if (GetFreeClusters(lastDiskLcn, Here->Len,
                                    &ClusterStart, &ClusterEnd, TRUE) >= Here->Len)
                                {
                                    if (MoveFileNormal(Here, ClusterStart) == Here->Len)
                                    {
							            if (CheckActivities()) break;
                                        FilesMoved++;
                                        cc += Here->Len;
                                        Here->Lcn = ClusterStart;
                                        //InsDIRFree(Here->Lcn, Here->Len);
                                        //Here = topdirlist = SortDict(topdirlist, listlen, byLcnDesc);
                                        if (lastDiskLcn == Here->Lcn) {
                                            lastDiskLcn = Here->Lcn + Here->Len;
                                            DrawBlocks(lastDiskLcn, lastDiskLcn+1, hbBlack, hpBlack);
                                        }
                                    }
                                }
                            }
                            if (cc >= dirsize) {
                                AList = ALtop;
                                break;
                            }
                            Here = Here->next;
                        }
                        // while for actual file in AList
                        if (CheckActivities()) break;
                        if (cc >= al_size) continue;
                    }
                    // elseif

                    dirsizecounter += al_size;
                    if (dirsizecounter >= dirsize) {
                        AList = ALtop;
                        break;
                    }

                }
                // if
                AList = AList->next;
            }
            // while for current DirToMove
            if (CheckActivities()) break;
            AList = ALthisdir;
            AList = AList->next;
        }
        // while for whole dirtree

        DelDIR(ALtop);
    }

    ghDefrag = NULL;

    // re-enable menu
    EnableMenus(5);

    sprintf(buff, "");
    if (FilesMoved>0) {
        sprintf(buff, "%s file(s) moved", fmtNumber(FilesMoved, buff));
        ResetStatusBar();

        bAnalyzed = FALSE;
        // draw disk-layout
		if (!bStopAction) DrawBitMap(NULL);
        // do an new Analyze
        //SendMessage(ghWnd, WM_COMMAND, IDM_ANALYZE, 0);
    }

    OutLine1(buff);
    UpdateStatusBar("DefragBySortingDirs finished", 4, 0);
}