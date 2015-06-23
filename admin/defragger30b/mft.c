// mft.c - mft reading stuff
// defragger 3.0beta for nt/2k/xp free.pages.at/blumetools/ blume1975@web.de

#include "globals.h"
#include "defrag.h"
#include "list.h"
#include "mft.h"
#include "statbar.h"

int           MakeMask           (unsigned char size);
int           ToSigned           (int val, unsigned char size);
__int64       mySetFilePointer   (HANDLE hf, __int64 distance, DWORD MoveMethod);
void          MFTAttrGetName     (BYTE *MFTRecord, USHORT offset, UCHAR NameLength, char *fname);
LONGLONG      ReadRunList        (BYTE *MFTRecord, __int64 MFTid, LONGLONG parent, int recstart, int recpos, int recsize, USHORT flags, char *dname, char *fname, __int64 size, BOOL bAttrIndex, ULONGLONG AttrSize);
void          ReadIndexBlock     (LONGLONG parent, LONGLONG offset, LONGLONG lenght, char *dir, ULONGLONG AttrSize);
void          ReadIndizes        (BYTE * data, LONGLONG parent, LONGLONG size, char *dname, BOOLEAN Nonresident);
struct        FILEstruct         *anker=NULL;


//makes bitmask for xor...
int MakeMask(unsigned char size) {
    int i, r=0;
    for (i=0; i<(size * 8); i++)
        r = r | (1 << i);
    return r;
}

int ToSigned(int val, unsigned char size) {
    if ((val >> ((size * 8) - 1)) & 1)
        return(-1 *  (  (val ^ MakeMask(size) )+1  )   );
    else
        return val;
}

__int64 mySetFilePointer(HANDLE hf, __int64 distance, DWORD MoveMethod) {
   LARGE_INTEGER li;
   li.QuadPart = distance;
   li.LowPart = SetFilePointer (hf, li.LowPart, &li.HighPart, MoveMethod);

   if (li.LowPart==0xFFFFFFFF && GetLastError()!=NO_ERROR)
      return -1;

   return li.QuadPart;
}    
	
void MFTAttrGetName(BYTE *MFTRecord, USHORT offset, UCHAR NameLength, char *fname)
{
    unsigned char i=0, j=0;
    char c, lastc;

    if (NameLength==0) {
        fname[0]='\0';
    } else {
        lastc = c = MFTRecord[offset+(i++)];
        do {
            if (c>31 || c<0)
                fname[j++] = c;
            lastc = c;
            c = MFTRecord[offset+(i++)];
        } while (((lastc!=0 && c==0) ||
                  (lastc==0 && c!=0)    ) &&
                 j<NameLength);
        fname[j]=0;
    }
}

// fuckin runlist-DATA with LCNs and LENs
LONGLONG ReadRunList(BYTE *MFTRecord, __int64 MFTid, LONGLONG parent, int recstart, int local_recpos, int recsize, USHORT flags,
					 char *dname, char *fname, __int64 size, BOOL bAttrIndex, ULONGLONG AttrSize) 
{
    //       RunOffset,   RunLength,   OldOffset
    ULONGLONG RunOffset=0, RunLength=0, OldOffset=0, lastLcn=0, lastLen=LLINVALID;
    ULONGLONG R=0, V=0, sLcn=LLINVALID, sLen=0, lastnonSparseLen=0;
    BYTE B, L, O;
    ULONGLONG frags=0;
    struct FILEstruct *Last=NULL, *ContinuedEntry=NULL;
    struct FILEstruct *Here=NULL;
	char thename[512];
	int runNO=1;
	BOOL compressed   = FALSE;
	BOOL encrypted    = FALSE;
	BOOL sparse       = FALSE, lastSparse = FALSE;
	BOOL globalsparse = FALSE;
	BOOL fragmented   = FALSE;
	BOOL locked       = FALSE;
	BOOL specialflag  = FALSE;

	sprintf(thename, "%s%s", dname, fname);

	if ((MFTid==2)               || // mark $LogFile as locked
		(size!=IINVALID       && 
		 IsFileInUse(thename) &&
		 MFTid!=-1)
	   )
	{
		locked=TRUE;
		cLocked++;
	}

	if (flags & 0x0001) compressed = TRUE;
	if (flags== 0x4000) encrypted  = TRUE;

	if (size!=IINVALID && MFTid > 24 &&  MFTid != parent)
		cBytes += size;

	// if this is a continued fragment from more than two indizees
	// then remember the describing fragment from last RunList
	if (dirlist && dirlist->MFTid == MFTid)
	if ( lstrcmp(thename, dirlist->fileName)==0 )
	{
		ContinuedEntry = Last = dirlist; //->prev;
		if (Last->Len != LLINVALID)
			lastnonSparseLen = Last->Len;

		if (dirlist->sparse || ContinuedEntry->sparse) {
			frags        = dirlist->fragments;
			fragmented   = dirlist->fragmented;
			if (ContinuedEntry->sparse || dirlist->sparse)
				globalsparse = TRUE;
			compressed   = dirlist->compressed;
			sLcn         = dirlist->Lcn;
			sLen         = dirlist->Len;
		}
	}

	if (dirlist) anker = dirlist;

	do { // while (B > 0  && recsize - local_recpos > 2) {
	  // get first byte for this runlist/datarun
	  memcpy(&B, &MFTRecord[recstart + local_recpos++], 1);

	  if (B!=0 && (recsize - local_recpos) > 2) {

		if (specialflag) {
			specialflag = FALSE;
			RunOffset = 0;
		}

		OldOffset = RunOffset;
        L  =  B & 0x0F;       // len in bytes
        O  = (B & 0xF0) >> 4; // offset in bytes
		if (recsize < local_recpos + L + 2)
			break;

		RunLength = 0;
        RunOffset = 0;

        // get Len for this run
        memcpy(&RunLength,&MFTRecord[recstart + local_recpos],L);
        local_recpos += L;
        V += L;

        // get offset for this run
        memcpy(&RunOffset, &MFTRecord[recstart + local_recpos], O);
        local_recpos += O;

		RunOffset = OldOffset + ToSigned((int)RunOffset, O);

		if (RunOffset > cDiskClusters)
		    RunOffset = (RunOffset & 0xFFFFFF) << 4;

		if (RunLength > cDiskClusters)
		    RunLength = (RunLength & 0xFFFFFF) << 8;

		//if (ContinuedEntry && RunOffset == 0)
		//	RunOffset = ContinuedEntry->Lcn;
	
		if (RunOffset == 0 &&
			Last           && Last->sparse && 
			ContinuedEntry && ContinuedEntry->sparse
			) 
		{
			specialflag = TRUE;
			RunOffset = Last->Lcn;
		} // else lastLcn = RunOffset;

		// nother check!? yes!
		if (RunOffset >=0 && RunOffset < cDiskClusters && 
			RunOffset != LLINVALID &&
			RunLength > 0 && RunLength < cDiskClusters    ) 
		{
			R = 16 - (V%16);
            frags++;

			/*
			if (frags>1 && 
				!sparse &&
				lastSparse &&
				)
				fragmented = TRUE;
			*/

			// sparse-files
			if (O==0)
				sparse = globalsparse = TRUE;
			else {
				if (lastLen!=LLINVALID && lastLcn+lastnonSparseLen != RunOffset)
					fragmented = TRUE;

				// test this cluster if it follows the Last->Lcn
				// this also happens on the first one because Last->Lcn is 0
				if (Last && Last->Lcn+lastnonSparseLen != (ULONGLONG)(RunOffset)) 
				{
					Here = dirlist;
					while (Here && Here->prev && Here->prev->MFTid == Here->MFTid) {
						Here->fragmented = fragmented;
						Here = Here->prev;
					}
				}
				sparse = FALSE;
				lastnonSparseLen = RunLength;
			}

            if (frags==2) {
				// make the first fragment fragmented
				InsDIR(thename, size, lastLcn, lastLen, sLen-lastLen, 0, frags-1, FALSE, fragmented, compressed, lastSparse, locked, MFTid);
				if (MFTid<2)
				InsMFT(thename, size, lastLcn, lastLen, sLen-lastLen, 0, frags-1, fragmented, locked, MFTid);
			}

			if (frags>=2) {
				// second or more fragment
				InsDIR(thename, size, RunOffset, RunLength, sLen, 0, frags, FALSE, fragmented, compressed, sparse, locked, MFTid);
				if (MFTid<2)
				InsMFT(thename, size, RunOffset, RunLength, sLen, 0, frags, fragmented, locked, MFTid);
			}

            // if its an InexAllocation (INDX) then go deeper
			if (bAttrIndex)
                ReadIndexBlock(parent, RunOffset, RunLength, thename, AttrSize);

			lastLcn = RunOffset;
			lastLen = RunLength;
			lastSparse = sparse;
			Last = dirlist;

			if (sLcn==LLINVALID) 
				sLcn = RunOffset; // remember startlcn of the file
			sLen += RunLength;
        }
        runNO++;
	  }
	} while (B > 0 && recsize - local_recpos > 2);

    if (sLcn==LLINVALID && sLen==0) // this is a small file stored in MFT directly
        sLen = LLINVALID;

    if (compressed) cCompressed++;
	if (frags > 1 && sLen!=0) {
		if (ContinuedEntry) {
			ContinuedEntry->fragments = frags;
			ContinuedEntry->Len       = sLen;

			// cut out
			Here                       = ContinuedEntry->next;
			ContinuedEntry->prev->next = Here;
			Here->prev                 = ContinuedEntry->prev;

			// paste at end
			dirlist->next              = ContinuedEntry;
			ContinuedEntry->prev       = dirlist;
			dirlist                    = dirlist->next;
			dirlist->next              = NULL;
		} else {
			if (fragmented && frags>1) {
				cFragmented++;
				cFragmentsAllTogether+=(unsigned long)frags;
			}
			// fragments have a zero ->fragments value
			// make main entry in dirlist for a fragmented file
			// with frag-count, starting LCN of first frag,
			// length of whole file and fragid=0
			InsDIR(thename, size, sLcn, sLen, 0, frags, 0, FALSE, fragmented, compressed, globalsparse, locked, MFTid);
			if (MFTid < 2)
			InsMFT(thename, size, sLcn, sLen, 0, frags, 0, fragmented, locked, MFTid);
		}
	} else if (!fragmented) {
		InsDIR(thename, size, sLcn, sLen, 0, 1,     1, FALSE, fragmented, compressed, sparse, locked, MFTid);
		if (MFTid < 2)
		InsMFT(thename, size, sLcn, sLen, 0, frags, 1, fragmented, locked, MFTid);
	}

    return sLen;
}

void ReadIndizes(BYTE * data, LONGLONG parent, LONGLONG size, char *dname, BOOLEAN Nonresident) 
{
	struct FILEstruct *Here=NULL;
    INDEX_HEADER IdxHdr;
    INDEX_RECORD IdxRec;
    LONGLONG nextMFTid, lastMFTid=0;
	LONGLONG pos=0, subpos=0;
	LONG i;
	
	for (i=0; i <= (size / (512 * 8) ); i++) {
        pos =  i * (512 * 8);
		if (pos+sizeof(INDEX_HEADER) >= size) break;

		// read INDEX_HEADER 'INDX' if any
		// there may be one or more of them...
		// overread until new 
		memcpy(&IdxHdr, &data[pos], sizeof(INDEX_HEADER));
		if (IdxHdr.IndxID!=1480871497) //0x58444e49)
	        continue;

	    // set pos to IdxHdr.BytesToIndexEntries + 24 (0x18) !!!
		pos =  i * (512 * 8) + IdxHdr.BytesToIndexEntries + 0x18;
		subpos = pos;
        while (pos - i * (512 * 8) < IdxHdr.SizeOfIndexEntries) // + 0x18
		{
			// read INDEX_RECORD
			memcpy(&IdxRec, &data[pos], sizeof(INDEX_RECORD));
			nextMFTid = IdxRec.MFTReference & 0xFFFFFFFFFF;

			// set next entry
			pos = subpos + IdxRec.Size;
			subpos = pos;
			
			if ( nextMFTid!=5            &&
				 nextMFTid!=0            &&
				 nextMFTid <  MFTentries &&
				 IdxRec.NameType!=0x02   && // no DOS-names!
				 nextMFTid!=lastMFTid    &&
				 nextMFTid!=parent          ) 
			{
				ReadMFTRecord(nextMFTid, dname);
				lastMFTid = nextMFTid;
			}
		}
    }

	lastMFTid=0;
	if (!Nonresident) {
		// read INDEX_HEADER 'INDX' to get size of attr 
		memcpy(&IdxHdr, &data[pos], sizeof(INDEX_HEADER));

        while (pos + 32 < size) 
		{
			// read INDEX_RECORD
			memcpy(&IdxRec, &data[pos], sizeof(INDEX_RECORD));
			nextMFTid = IdxRec.MFTReference & 0xFFFFFFFFFF;

			// set next entry
			pos = subpos + IdxRec.Size;
			subpos = pos;

			if ( nextMFTid!=5            &&
				 nextMFTid!=0            &&
				 nextMFTid <  MFTentries &&
				 IdxRec.NameType!=0x02   && // no DOS-names!
				 nextMFTid!=lastMFTid    &&
				 nextMFTid!=parent          ) 
			{
				ReadMFTRecord(nextMFTid, dname);
				lastMFTid = nextMFTid;
			}
		}
	}
}

void ReadIndexBlock(LONGLONG parent, LONGLONG offset, LONGLONG lenght, char *currdir, ULONGLONG AttrSize) {
    BYTE *data;
    DWORD bytesret=0;
    ULONGLONG SecPos;
    __int64 BytePos, ByteOffset=offset;
	char dir[512];
	unsigned long bytestoread;
	int progress=0;

	lstrcpy(dir, currdir);

	bytestoread = (unsigned long)AttrSize;
	data = (BYTE *)malloc(bytestoread);

	// calc cluster-position relative to start of disk
	SecPos = ByteOffset * gNTFSDATA.BytesPerCluster;
			 //gPARTINFO.StartingOffset.QuadPart + // start cluster of partition
			 //cSectorsPerCluster +                // skip the first cluster
	BytePos = SecPos;

	// position the file-pointer to the needed cluster
	mySetFilePointer(ghVolume, BytePos, FILE_BEGIN);
	ReadFile(ghVolume, data, (DWORD)bytestoread, &bytesret, NULL);
	ReadIndizes(data, parent, AttrSize, dir, TRUE);
	free(data);
}

void ReadMFTRecord(__int64 MFTid, char *currdir) {
	// some very important varis for MFT-reading
	NTFS_FILE_RECORD_INPUT_BUFFER   gNTFSRECORDi;
	PNTFS_FILE_RECORD_OUTPUT_BUFFER gNTFSRECORDo;

    // the several internal MFT-record structures
	FILE_RECORD_HEADER FileRecHdr;   // 0
    STANDARD_INFORMATION StdInfo;    // 1
    ATTRIBUTE_HDR AttrHdr;           // 2..
    RESIDENT_ATTRIBUTE DataRes;
    NONRESIDENT_ATTRIBUTE DataNonRes;
    FILENAME_ATTRIBUTE FileNameAttr;
    INDEX_ROOT IdxRoot;
    INDEX_RECORD IdxRec;
	ATTRIBUTE_LIST AttrList;

	LONGLONG nextMFTid=0, lastMFTid=0, newparent=0;
	LONGLONG parent=0;
    DWORD recpos, lpBytesReturned;   // the actual position in the current MFT-Record
    USHORT nxtpos, nxtpos2;          // used to find the attributes

    __int64 BytePos=0;
	__int64 fSize=0;
    ULONGLONG SecPos=0;

    char buff[512];
    char dname[512], fname[512], fnamedos[512], attrname[512];     // store the name of the current MFT-Record

	if (MFTid==8) return; // skip ".\$BadClus"

	// setup outputbuffer - FSCTL_GET_NTFS_FILE_RECORD depends on this
	gNTFSRECORDo = (PNTFS_FILE_RECORD_OUTPUT_BUFFER) malloc
					(sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER)+
					gNTFSDATA.BytesPerFileRecordSegment-1);

    // --- which record do I need?
    gNTFSRECORDi.FileReferenceNumber.QuadPart = MFTid;

    // read MFT-record via DeviceIoControl - returns always a used record
    // MUST not be the same MFTid as requested
    if (DeviceIoControl(ghVolume,
        FSCTL_GET_NTFS_FILE_RECORD,
        &gNTFSRECORDi, sizeof(NTFS_FILE_RECORD_INPUT_BUFFER),
        gNTFSRECORDo, sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER)+
                      gNTFSDATA.BytesPerFileRecordSegment-1,
					  &lpBytesReturned, NULL)==0) 
	{
		sprintf(buff, "mftrecord %I64u | %I64u", (MFTid &0xFFFFFF));
		HandleWin32Error(GetLastError(), buff);
	}
    // --- which record did I get?
	if (MFTid!=gNTFSRECORDo->FileReferenceNumber.QuadPart) {
        free(gNTFSRECORDo);
		return;
	}

	if (cMFTentry > MFTentries) 
		MFTentries=cMFTentry;

    // wait for draw or break
    if ( bStopAction ) {
		free(gNTFSRECORDo);
		return;
	}

    // read the record header from start of MFT-record
    memcpy(&FileRecHdr, &gNTFSRECORDo->FileRecordBuffer[0], sizeof(FILE_RECORD_HEADER));

    if ( FileRecHdr.RecHdr.Type==0x454c4946 &&    // magic FILE-number
        (FileRecHdr.Flags==0x0001 ||              // this is a file in use
         FileRecHdr.Flags==0x0003 ||
         FileRecHdr.Flags==0x0005 ||
		 FileRecHdr.Flags==0x0007 ||              // this is a dir
		 FileRecHdr.Flags==0x0009 ||
		 FileRecHdr.Flags==0x000d    
		)         
	   ) 
    {
		// Show progress
		//progress = (long int)( ((float)(cMFTentry++)/(float)(MFTentries))*imgWidth );
		progress = (long int)( ((float)(cBytes     )/(float)(cBytesUsed))*imgWidth );
		if (progress > lastprogress) {
			lastprogress = progress;
			DrawProgress(progress);
		}

		if (FileRecHdr.Flags==0x0001) {
			cFiles++;
			/**/
			if (cFiles > 0 && cFiles % 1024==0) {
				// Update the status bar
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
			/**/
		}

		if (FileRecHdr.Flags==0x0003) {
			cDirs++;
			wsprintf(buff, "Analyzing %s", currdir);
			OutLine1(buff);

			/**/ 
			if (cDirs % 64==0) {
				sprintf(buff, "%s dirs", fmtNumber(cDirs, buff));
				UpdateStatusBar(buff, 1, 0);
			}
			/**/
		}

		//sprintf(buff, "%10I64u - [%.0f%%] %s ", MFTid, ((float)(cMFTentry++)/(float)(MFTentries))*100, currdir);
		//sprintf(buff, "[%.0f%%] %s ", ((float)(cMFTentry++)/(float)(MFTentries))*100, currdir);
		//OutLine1(buff);

		lstrcpy(dname, currdir);
        buff[0]=0;
		fname[0]=0;
		fnamedos[0]=0;

        // set position of first attribute
        recpos = FileRecHdr.AttributeOffset;
        nxtpos = 0;

		// first get name of the file we're working with
        do { // run until attribut-list is empty
			memcpy(&AttrHdr, &gNTFSRECORDo->FileRecordBuffer[recpos], sizeof(ATTRIBUTE_HDR)); // read header of the next attribute

			if (AttrHdr.AttributeType==0xFFFFFFFF) break; // exit at end 
			if (AttrHdr.AttributeType==AttributeFileName) {
			    nxtpos = sizeof(ATTRIBUTE_HDR); // set position of next sub AttrHdr
				memcpy(&DataRes,&gNTFSRECORDo->FileRecordBuffer[recpos+nxtpos], sizeof(RESIDENT_ATTRIBUTE)); // read DataRes-Attr (header)

	            nxtpos2 = DataRes.AttrOffset; // set next AttrHdr position
                memcpy(&FileNameAttr,&gNTFSRECORDo->FileRecordBuffer[recpos+nxtpos2], sizeof(FILENAME_ATTRIBUTE)); // read Name-attribute

				nxtpos2 += sizeof(FILENAME_ATTRIBUTE)-6; // set next AttrHdr position
                if (FileNameAttr.NameType!=0x02) { // no DOS-names!
					fSize = FileNameAttr.RealSize;
					MFTAttrGetName(gNTFSRECORDo->FileRecordBuffer, (USHORT)(recpos+nxtpos2), FileNameAttr.NameLength, fname);
					// time to save parent directory
					newparent = FileNameAttr.DirectoryFileReferenceNumber & 0xFFFFFFFF;
				} else {
					fSize = FileNameAttr.RealSize;
					MFTAttrGetName(gNTFSRECORDo->FileRecordBuffer, (USHORT)(recpos+nxtpos2), FileNameAttr.NameLength, fnamedos);
					// time to save parent directory
					newparent = FileNameAttr.DirectoryFileReferenceNumber & 0xFFFFFFFF;
				}
			}
            recpos += AttrHdr.Length; // goto next AttrHdr
        } while (AttrHdr.AttributeType!=0xFFFFFFFF && 
			     recpos < gNTFSDATA.BytesPerFileRecordSegment);

		if (newparent!=0) parent = newparent;

		if (fname[0]==0 && fnamedos[0]!=0)
			lstrcpy(fname, fnamedos);

        if (lstrcmp(fname, ".")==0)
			fname[0]=0;

		recpos = FileRecHdr.AttributeOffset;
        nxtpos = 0;
        buff[0]=0;

		// run until attribut-list is empty
        do {
            // read attribute-header
            memcpy(&AttrHdr, &gNTFSRECORDo->FileRecordBuffer[recpos], sizeof(ATTRIBUTE_HDR));

			if (AttrHdr.AttributeType==0xFFFFFFFF) break; // exit at end

            // set position of next sub AttrHdr
            nxtpos = sizeof(ATTRIBUTE_HDR);

			if (! AttrHdr.Nonresident) {
                // read DataRes-Attr (header)
                memcpy(&DataRes,&gNTFSRECORDo->FileRecordBuffer[recpos+nxtpos], sizeof(RESIDENT_ATTRIBUTE));
				
                // set next AttrHdr position
                nxtpos2 = DataRes.AttrOffset;

				if (AttrHdr.NameLength>0 && AttrHdr.AttributeType!=AttributeFileName) {
					MFTAttrGetName(gNTFSRECORDo->FileRecordBuffer, (USHORT)(recpos+AttrHdr.NameOffset), AttrHdr.NameLength, buff);

					if (buff[0]!=0) {
						lstrcpy(attrname, ":");
						lstrcat(attrname, buff);
					}
				} else
					attrname[0]=0;

                switch (AttrHdr.AttributeType) {
                case AttributeStandardInformation:
                    // StdInfo - not needed
                    //memcpy(&StdInfo,&gNTFSRECORDo->FileRecordBuffer[recpos+nxtpos2], sizeof(STANDARD_INFORMATION));
                    break;
                case AttributeData:
					// a file that stored directly in MFT
					//if (dname[lstrlen(dname)-1]!='\\') lstrcat(dname, "\\");
					//lstrcat(dname, fname);
					//InsDIR(dname, (__int64)(DataRes.AttrLenth), LLINVALID, LLINVALID, 1, 1, FALSE, FALSE, FALSE, FALSE, FALSE, MFTid);
					break;
                case AttributeObjectId:
					//if (fname[0]!=0) lstrcat(fname, ":");
                    //lstrcat(fname, "$OBJECT_ID");
                    break;
                case AttributeVolumeName:
					//if (fname[0]!=0) lstrcat(fname, ":");
                    //lstrcat(fname, "$VOLUME_ID");
                    break;
                case AttributeVolumeInformation:
					//if (fname[0]!=0) lstrcat(fname, ":");
                    //lstrcat(fname, "$VOLUME_INFORMATION");
                    break;
                case AttributeEAInformation:
					//if (fname[0]!=0) lstrcat(fname, ":");
                    //lstrcat(fname, "$EAINFORMATION");
                    break;
                case AttributeBitmap:
					if (fname[0]==0) {
						lstrcat(fname, "::");
					    lstrcat(fname, "$BITMAP");
					}
                    break;
                case AttributeAttributeList:
					lstrcpy(buff, dname);
					if (buff[lstrlen(buff)-1]!='\\') lstrcat(buff, "\\");
					lstrcat(buff, fname);
					lastMFTid=0;

					while (nxtpos2 < recpos + DataRes.AttrLenth && nextMFTid < MFTentries) {
						memcpy(&AttrList, &gNTFSRECORDo->FileRecordBuffer[recpos + nxtpos2], sizeof(ATTRIBUTE_LIST));

						nextMFTid = AttrList.FileReferenceNumber & 0xFFFFFFFFFF;
						if (nextMFTid!=MFTid       && 
							nextMFTid < MFTentries &&
							nextMFTid > 0          &&
							nextMFTid!=lastMFTid   &&
							nextMFTid!=parent         ) 
						{
							ReadMFTRecord(nextMFTid, buff);
							lastMFTid = nextMFTid;
						}
						nxtpos2 += (USHORT)AttrList.Length;
					}

                    break;
                case AttributeIndexRoot:
					memcpy(&IdxRoot,&gNTFSRECORDo->FileRecordBuffer[recpos + nxtpos2], sizeof(INDEX_ROOT) );

					// start at IdxRoot.FirstEntryOffset to skip header of INDEX_ROOT
					nxtpos2 += ((USHORT)IdxRoot.FirstEntryOffset * 2);  // + 0x18;

					lstrcpy(buff, dname);
					if (buff[lstrlen(buff)-1]!='\\') lstrcat(buff, "\\");
					lstrcat(buff, fname);
					if (attrname[0]!=0 && fname[0]!=0 && FileRecHdr.Flags!=0x0003) 
					{
						lstrcat(buff, attrname);
						lstrcat(buff, ":$INDEX_ALLOCATION");
					} else {
						attrname[0]=0;
					}

					if (FileRecHdr.Flags==0x0003) 
					if (buff[lstrlen(buff)-1]!='\\') lstrcat(buff, "\\");

					// no Index_Allocation used, entries stored in MFT
					ReadIndizes(&gNTFSRECORDo->FileRecordBuffer[recpos + nxtpos2], 
						        parent, IdxRoot.AllocatedSize, 
						        buff, AttrHdr.Nonresident);

					if (IdxRoot.Flags==0x00) {
						// directory entries stored resident in this MFTRecord
						//InsDIR(buff, IINVALID, LLINVALID, LLINVALID, 1, 1, FALSE, FALSE, FALSE, FALSE, FALSE, MFTid);
					} else {
						// index allocation used
					}

					break;
                default: break;
                }
            } else if (AttrHdr.Nonresident) {
                // read DataNonRes-Attr (header)
                memcpy(&DataNonRes, &gNTFSRECORDo->FileRecordBuffer[recpos+nxtpos], sizeof(NONRESIDENT_ATTRIBUTE));

				// set position to start of runlist
                nxtpos2 = DataNonRes.RunArrayOffset; //sizeof(NONRESIDENT_ATTRIBUTE);

				if (AttrHdr.NameLength>0                                     &&
					(FileRecHdr.Flags!=0x0003 ||
					  (AttrHdr.AttributeType!=AttributeIndexRoot       &&
					   AttrHdr.AttributeType!=AttributeIndexAllocation    )) &&
					AttrHdr.AttributeType!=AttributeFileName                    ) 
				{
					buff[0]='\0';
					MFTAttrGetName(gNTFSRECORDo->FileRecordBuffer, (USHORT)(recpos+AttrHdr.NameOffset), AttrHdr.NameLength, buff);

					lstrcpy(attrname, ":");
					lstrcat(attrname, buff);
				} else
					attrname[0]=0;

				switch (AttrHdr.AttributeType) {
                case AttributeIndexAllocation:

					memcpy(&IdxRec,&gNTFSRECORDo->FileRecordBuffer[recpos + nxtpos2], sizeof(INDEX_RECORD));

					lstrcpy(buff, dname);
					if (fname[0]!=0)
						lstrcat(buff, fname);
					
					if (attrname[0]!=0) {
						lstrcat(buff, attrname);
						lstrcat(buff, ":$INDEX_ALLOCATION");
					} else
						attrname[0]=0;

					if (FileRecHdr.Flags==0x0003) 
					if (buff[lstrlen(buff)-1]!='\\') lstrcat(buff, "\\");

					ReadRunList(gNTFSRECORDo->FileRecordBuffer, MFTid, parent, recpos, DataNonRes.RunArrayOffset, (int)AttrHdr.Length, AttrHdr.Flags, 
					            buff, "", IINVALID, TRUE, DataNonRes.RealSize);
					
                    break;
                case AttributeIndexRoot:
					//buff[0]='\0';
					lstrcpy(buff, fname);
					if (attrname[0]!=0 && fname[0]!=0) {
						lstrcat(buff, attrname);
						lstrcat(buff, ":$INDEX_ROOT");
					}
					
					if (FileRecHdr.Flags==0x0003) 
					if (dname[lstrlen(dname)-1]!='\\') lstrcat(dname, "\\");

					ReadRunList(gNTFSRECORDo->FileRecordBuffer, MFTid, parent, recpos, DataNonRes.RunArrayOffset, (int)AttrHdr.Length, AttrHdr.Flags, 
						         dname, buff, IINVALID, TRUE, DataNonRes.RealSize);
					
                    break;
                case AttributeAttributeList:

					//s.o.

                    break;
                case AttributeData:
                    // read the real number of MFT-entries
					if (MFTid==0) 
						MFTentries = (DataNonRes.RealSize / gNTFSDATA.BytesPerSector) >> 1;

					lstrcpy(buff, dname);
					if (fname[0]!=0)
						lstrcat(buff, fname);

					if (attrname[0]!=0) {
						lstrcat(buff, attrname);
						lstrcat(buff, ":$DATA");
					}

					if (DataNonRes.RealSize>cDiskClusters*cBytesPerCluster)
						DataNonRes.RealSize = DataNonRes.RealSize & 0xFFFFFFFF;

					nextMFTid = FileRecHdr.BaseFileRecord & 0xFFFFFFFFFF;
					if (nextMFTid!=0)
						ReadRunList(gNTFSRECORDo->FileRecordBuffer, nextMFTid, MFTid, recpos, nxtpos2, (int)AttrHdr.Length, AttrHdr.Flags, 
									buff, "", DataNonRes.RealSize, FALSE, 0);
					else
						ReadRunList(gNTFSRECORDo->FileRecordBuffer, MFTid, parent, recpos, nxtpos2, (int)AttrHdr.Length, AttrHdr.Flags, 
									buff, "", DataNonRes.RealSize, FALSE, 0);

                    break;
                case AttributeSecurityDescriptor:
					lstrcpy(buff, fname);
					if (MFTid==5) 
						lstrcat(buff, "$Root");
					lstrcat(buff, "::$SECURITY_DESCRIPTOR");
					ReadRunList(gNTFSRECORDo->FileRecordBuffer, MFTid, parent, recpos, nxtpos2, (int)AttrHdr.Length, AttrHdr.Flags, 
						         dname, buff, DataNonRes.RealSize, FALSE, 0);

                    break;
                case AttributeEA:
					lstrcpy(buff, fname);
					lstrcat(buff, attrname);
					lstrcat(buff, "::$EA");
					ReadRunList(gNTFSRECORDo->FileRecordBuffer, MFTid, parent, recpos, nxtpos2, (int)AttrHdr.Length, AttrHdr.Flags, 
						         dname, buff, DataNonRes.RealSize, FALSE, 0);

                    break;
                case AttributePropertySet:
					lstrcpy(buff, fname);
					lstrcat(buff, attrname);
					lstrcat(buff, "::$PropertySet");
					ReadRunList(gNTFSRECORDo->FileRecordBuffer, MFTid, parent, recpos, nxtpos2, (int)AttrHdr.Length, AttrHdr.Flags, 
						         dname, buff, DataNonRes.RealSize, FALSE, 0);

                    break;
                case AttributeReparsePoint:
					lstrcpy(buff, fname);
					lstrcat(buff, attrname);
					lstrcat(buff, "::$Reparse");
					ReadRunList(gNTFSRECORDo->FileRecordBuffer, MFTid, parent, recpos, nxtpos2, (int)AttrHdr.Length, AttrHdr.Flags, 
						         dname, buff, DataNonRes.RealSize, FALSE, 0);

                    break;
                case AttributeBitmap:
					lstrcpy(buff, fname);
					lstrcat(buff, attrname);
					lstrcat(buff, "::$BITMAP");
					ReadRunList(gNTFSRECORDo->FileRecordBuffer, MFTid, parent, recpos, nxtpos2, (int)AttrHdr.Length, AttrHdr.Flags, 
						         dname, buff, DataNonRes.RealSize, FALSE, 0);
                    break;
                case AttributeLoggedUtilityStream:
					lstrcpy(buff, fname);
					lstrcat(buff, attrname);
					lstrcat(buff, ":$LOGGED_UTILITY_STREAM");
					ReadRunList(gNTFSRECORDo->FileRecordBuffer, MFTid, parent, recpos, nxtpos2, (int)AttrHdr.Length, AttrHdr.Flags, 
						         dname, buff, DataNonRes.RealSize, FALSE, 0);
                    break;
                default: break;
                } // switch
            } // if resident or nonresident

            // goto next AttrHdr
            recpos += AttrHdr.Length;

        } while (AttrHdr.AttributeType!=0xFFFFFFFF &&
                 recpos < gNTFSDATA.BytesPerFileRecordSegment);
        //finished current MFT-record

    } // if FILE*

	free(gNTFSRECORDo);
}