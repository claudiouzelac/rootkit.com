// list.c - list functions for insert, delete, sort
// defragger 3.0beta for nt/2k/xp free.pages.at/blumetools/ blume1975@web.de

#include "globals.h"
#include "list.h"
#include "defrag.h"

struct FILEstruct *InsDIR(char *name, __int64 size, ULONGLONG Lcn, ULONGLONG Len, ULONGLONG Vcn, ULONGLONG fragments, ULONGLONG fragid,
						  BOOL free, BOOL fragmented, BOOL compressed, BOOL sparse, BOOL locked, __int64 MFTid) {
    struct FILEstruct *newel;

    // vorspulen
	while (dirlist && dirlist->next) dirlist = dirlist->next;

    // make mem for struct
    newel = (struct FILEstruct*) malloc (sizeof(struct FILEstruct));
    // make mem for filename
    newel->fileName = (char *)malloc(strlen(name) + 1);

    lstrcpy(newel->fileName, name);
    newel->filesize = size;
    newel->fragments  = fragments;
    newel->fragid     = fragid;
    newel->Lcn        = Lcn;
    newel->Len        = Len;
    newel->Vcn        = Vcn;
    newel->free       = free;
    newel->fragmented = fragmented;
    newel->compressed = compressed;
	newel->sparse     = sparse;
	newel->locked     = locked;
    newel->MFTid      = MFTid;
	
    if (dirlist==NULL) {                    // list existing?
        newel->next      = dirlist;         // create if not
		newel->prev      = NULL;            // previous is nothing
        topdirlist       = dirlist = newel; // set start of list
    } else {
        newel->prev      = dirlist;         // previous is the current one
        dirlist->next    = newel;           // ansonsten an ende anfuegen
        newel->next      = NULL;            // next is nothing
        dirlist          = dirlist->next;   // set list to end
    }
    listlen++;

	if (fragments<=1 && fragmented && (MFTid>1 || MFTid==IINVALID)  && !sparse) {
		if  (size > sizeLimit)
			DrawBlocks(Lcn, Lcn+Len, hbDRed, hpDRed);
		else
			DrawBlocks(Lcn, Lcn+Len, hbRed, hpRed);
	} else {
		if  (compressed && !sparse)
			DrawBlocks(Lcn, Lcn+Len, hbOck, hpOck);
		else if (locked) 
			DrawBlocks(Lcn, Lcn+Len, hbBrown, hpBrown);
	}

	if (dirlist->filesize>sizeLimit && !fragmented && !free && !sparse && !locked && (MFTid>1 || MFTid==IINVALID) )
		DrawBlocks(Lcn, Lcn+Len, hbDGreen, hpDGreen);
	if (dirlist->filesize==IINVALID)                // dirs in blue
		DrawBlocks(Lcn, Lcn+Len, hbBlue, hpBlue);
	if (bShowMarks && Len >= clustersPB*1 && !free && !sparse && fragments<=1 && fragid>=1 ) // large files
        DrawMarks(Lcn, Lcn+Len);

    return newel;
}

struct MFTstruct *InsMFT(char *name, __int64 size, ULONGLONG Lcn, ULONGLONG Len, ULONGLONG Vcn, ULONGLONG fragments, ULONGLONG fragid,
						  BOOL fragmented, BOOL locked, __int64 MFTid) {
    struct MFTstruct *newel;

    // vorspulen
	while (mftlist && mftlist->next) mftlist = mftlist->next;

    // make mem for struct
    newel = (struct MFTstruct*) malloc (sizeof(struct MFTstruct));
    // make mem for filename
    newel->fileName = (char *)malloc(strlen(name) + 1);

    lstrcpy(newel->fileName, name);
    newel->filesize   = size;
    newel->fragments  = fragments;
    newel->fragmented = fragmented;
    newel->fragid     = fragid;
    newel->Lcn        = Lcn;
    newel->Len        = Len;
    newel->MFTid      = MFTid;
	newel->Vcn        = Vcn;

    if (mftlist==NULL) {            // liste schon angelegt?
        newel->next   = mftlist;    // wenn nein, dann anlegen
        topmftlist    = mftlist = newel;
    } else {
        mftlist->next = newel;      // ansonsten an ende anfuegen
        newel->next   = NULL;
        mftlist       = mftlist->next;
    }
    mftlistlen++;
    return newel;
}

struct FILEstruct *CopyDIR(struct FILEstruct *top) {
    struct FILEstruct *work, *newel, *newlist=NULL, *topnewlist;
    char laststr[512];
    lstrcpy(laststr, LastOutLine);
    OutLine1("caching");

    if (top==NULL) return NULL;
    topnewlist=newel=newlist=NULL;
    work=top;                 // alles an den Listenanfang
    while (work != NULL) {
        // neues DirTree-Element anlegen
        newel             = (struct FILEstruct*) malloc (sizeof(struct FILEstruct));
        newel->fileName   = (char *)malloc(strlen(work->fileName) + 1);

        lstrcpy(newel->fileName, work->fileName);
        newel->filesize   = work->filesize;
        newel->fragments  = work->fragments;
        newel->fragid     = work->fragid;
        newel->Lcn        = work->Lcn;
        newel->Len        = work->Len;
        newel->Vcn        = work->Vcn;
        newel->free       = work->free;
        newel->compressed = work->compressed;
        newel->fragmented = work->fragmented;
        newel->MFTid      = work->MFTid;
	    newel->next       = work->next;
	    newel->prev       = work->prev;

        if (newlist == NULL) {          // liste schon angelegt?
            newel->next   = newlist;    // wenn nein, dann anlegen
            newlist       = newel;
            topnewlist    = newlist;
		    topnewlist->prev = NULL;    // previous is nothing
        } else {
            newel->prev   = dirlist;    // previous is the current one
            newlist->next = newel;      // ansonsten an ende anfuegen
            newel->next   = NULL;
            newlist       = newlist->next;
        }
        work = work->next;
    }
    OutLine1(laststr);
    return topnewlist;
}

void DelDIR(struct FILEstruct *top) {
    struct FILEstruct *last, *work;
    char laststr[512];
    lstrcpy(laststr, LastOutLine);
    OutLine1("uncaching");

    if (top == NULL) return;

    last=work=top;                 /* alles an den Listenanfang */
    while (work!=NULL) {
        if (work->next==NULL) {    /* am Ende der Liste */
            top  = NULL;           /* Element */
            free(work->fileName);
            free(work);
            work = NULL;           /* abbruchbedingung setzen */
        } else {
            //if (work==top) {     /* am Anfang? */
                top  = work->next; /* top neu setzen */
                free(work->fileName);
                free(work);
                work = top;
            //}
        }
    } // while
    //free(last);
	OutLine1(laststr);
}

void DelDIRmft(struct MFTstruct *top) {
    struct MFTstruct *lastmft, *workmft;
    lastmft=workmft=top;                  /* alles an den Listenanfang */
    while (workmft!=NULL) {
        if (workmft->next==NULL) {        /* am Ende der Liste */
            top=NULL;                     /* Element */
            free(workmft->fileName);
            free(workmft);
            workmft=NULL;
        } else {
            //if (workmft==top) {         /* am Anfang? */
                top  = workmft->next;     /* top neu setzen */
                free(workmft->fileName);
                free(workmft);
                workmft = top;
            //}
        }
    } // while 
}

#define Compare(a,b) ( strcmp(a,b) )  // used by SortDict
#define numcmp(x, y) ( ((x)>(y)) ? (1) : ((x)<(y)) ? (-1) : (0) )

////////////////////////////////////////////////////////////////////////////////////
// this fast mergesort routine is not from me - thanks to the author
// fastest sorting algorithm I've seen
// sorts a list of 15000 entries in 0.1 sec with no memory use
////////////////////////////////////////////////////////////////////////////////////

/* 
 * from Crack v4.1, the "Sensible" Unix Password Cracker
 * Alec David Edward Muffett, Unix Programmer and Unemployed Coffee Drinker.
 * aem@aber.ac.uk aem@uk.ac.aber aem%aber@ukacrl.bitnet mcsun!ukc!aber!aem
 * "I didn't invent the Unix Password Security problem. I just optimised it."
 *
 * Sort a list of struct FILEstruct by using an iterative bottom-up merge sort.
 * This particular piece of code took me ages to do (well, 2 days + 3 weeks
 * research) and provides a FAST way of sorting a linked list without the
 * overhead of increasing memory usage via malloc() or brk(). Why ? Because I
 * have to assume that there is no more memory, thats why. It's all Brian
 * Thompsett's fault! Really! Filling the swapspace on a SparcStation2 and
 * expecting Crack to survive! Argh! 8-) 
 * Since this code is so nice, I'll comment it fairly thoroughly
 */
struct FILEstruct * MergeSortDict(register struct FILEstruct *chain3, 
                        long int listlength, 
                        int sortItem) {
    // misc counters //
    register int i;
    long int n;      // 2^n for n = 0..x //

    register struct FILEstruct *chain1;  // head of the first extracted subchain //
    register struct FILEstruct *chain2;  // head of second subchain //
    register struct FILEstruct *scratch; // useful temp pointer //
    struct FILEstruct *lead_in;          // PTR TO ELEMENT containing TAIL of unsorted list pre-merging //
    struct FILEstruct *lead_out;         // PTR TO HEAD of unsorted list after extracting chains //
    struct FILEstruct dummy1;            // dummy structures used as fenceposts //
    struct FILEstruct dummy2;
    
    if (lastsort == sortItem)
        return chain3;
    lastsort=sortItem;

    // Put the incoming list into 'dummy1' posthole //
    dummy1.next = chain3;

    // For values of n = 2^(0..30) limited by listlength //
    for (n = 1L; n < listlength; n *= 2) {
        // Store place to get/put head of list in 'lead_in' //
        lead_in = &dummy1;

        // Set chain1 to the head of unsorted list //
        for (chain1 = lead_in->next; chain1; chain1 = lead_in->next) {
             if (bStopAction) return(NULL);

             // Break connection head and chain1 //
             lead_in->next = (struct FILEstruct *) 0;

             // Extract up to length 'n', park on last element before chain2 //
             for (i = n - 1, scratch = chain1;
                 i && scratch->next;
                 scratch = scratch->next) {
                 i--;
             }

             // If chain1 is undersized/exact, there is no chain2 //
             if (i || !scratch->next) {
                // put chain1 back where you got it and break //
                lead_in->next = chain1;
                break;
             }

             // Get pointer to head of chain2 //
             chain2        = scratch->next;

             // Break connection between chain1 & chain2 //
             scratch->next = (struct FILEstruct *) 0;

             // Extract up to length 'n', park on last element of chain2 //
             for (i = n - 1, scratch = chain2;
                 i && scratch->next;
                 scratch = scratch->next) {
                 i--;
             }

             // Even if it's NULL, store rest of list in 'lead_out' //
             lead_out      = scratch->next;

             // Break connection between chain2 & tail of unsorted list //
             scratch->next = (struct FILEstruct *) 0;

             // Now, mergesort chain1 & chain2 to chain3 //

             // Set up dummy list fencepost //
             chain3       = &dummy2;
             chain3->next = (struct FILEstruct *) 0;

             // While there is something in each list //
             while (chain1 && chain2) {
                 // build search criteria
                 switch (sortItem) {
                     case byName :
                        i = Compare (chain1->fileName, chain2->fileName);
                        break;
                     case byLcnAsc :
                        i = numcmp(chain2->Lcn, chain1->Lcn); // note reverse notation!!!
                        break;
                     case byLcnDesc :
                        i = numcmp(chain1->Lcn, chain2->Lcn);
                        break;
                     case byFragments :
                        i = numcmp(chain2->fragments, chain1->fragments);
                        break;
                     case bySizeAsc :
                        i = numcmp(chain2->Len, chain1->Len); // note reverse notation!!!
                        break;
                     case bySizeDesc :
                        i = numcmp(chain1->Len, chain2->Len);
                        break;
                 } // switch

                 //normal: if (chain1->blabla > chain2->blabla)
                 if (i > 0) {  
					 // a > b
                     chain3->next = chain1;
                     chain3       = chain1;
                     chain1       = chain1->next;
                 } else if (i < 0) { 
					 // a < b
                     chain3->next = chain2;
                     chain3       = chain2;
                     chain2       = chain2->next;
                 } else {
                     // a == b. Link them both in. Don't try to get rid of the
                     // multiple copies here, because if any elements is freed up 
                     // at this point the listsize changes and the algorithm runs amok.
                     chain3->next = chain2;
                     chain3       = chain2;
                     chain2       = chain2->next;
                     chain3->next = chain1;
                     chain3       = chain1;
                     chain1       = chain1->next;
                 }
             } // while

             // Whatever is left is sorted and therefore linkable straight
             // onto the end of the current list
             if (chain1) chain3->next = chain1;
                    else chain3->next = chain2;

             // Skip to the end of the sorted list //
             while (chain3->next) chain3 = chain3->next;

             // Append this lot to where you got chain1 from ('lead_in') //
             lead_in->next = dummy2.next;

             // Append rest of unsorted list to chain3 //
             chain3->next = lead_out;

             // Set 'lead_in' for next time to last element of 'chain3' //
             lead_in = chain3;
        } // for 2
    } // for 1

    return (dummy1.next);
}

struct FILEstruct * SortDict (struct FILEstruct *chain3, 
                        long int listlength, 
                        int sortItem)
{
    char laststr[512];
    struct FILEstruct *x;

    //LARGE_INTEGER QFreq, QCountStart, QCountEnd;
    //QueryPerformanceFrequency(&QFreq);
    //QueryPerformanceCounter(&QCountStart);

    lstrcpy(laststr, LastOutLine);
    OutLine1("sorting");

    x = MergeSortDict(chain3, listlength, sortItem);

    /*
	QueryPerformanceCounter(&QCountEnd);
    {
        char buff[10];
        float diff;
        diff = ((float)QCountEnd.QuadPart - (float)QCountStart.QuadPart) / (float)QFreq.QuadPart;
        sprintf(buff, "%.4f sec.", diff);
        OutLine1(buff);
    }
	*/
    OutLine1(laststr);
    return x;
}
