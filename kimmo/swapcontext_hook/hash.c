// Functions that implement the separate chaining hashtable algorithms.
// The skeletons for these algorithms was taken from the book
// "Data Structures and Algorithm Analysis in C, Second Edition" by
// Mark Allen Weiss.
//
// Kimmo

#include "ntddk.h"
#include "ntstrsafe.h"
#include "hash.h"

#define DRIVERTAG1 'HSAH'

typedef struct _ELEMENT
{
	unsigned int threadID;
	unsigned int processID;
	BYTE imageName[16];
} ELEMENT, *PELEMENT;

typedef struct _TWOWAY
{
	DWORD key;
	ELEMENT data;
	LIST_ENTRY linkfield;
} TWOWAY, *PTWOWAY;

typedef struct _HASHTABLE
{
	unsigned int tableSize;
	PLIST_ENTRY *pListHeads;
} HASHTABLE, *PHASHTABLE;

PNPAGED_LOOKASIDE_LIST pLookasideList_TWOWAY = NULL;

/* Return next prime; assume N >= 10 */
static unsigned int NextPrime(int N)
{
    int i;

    if( N % 2 == 0 )
        N++;
    for( ; ; N += 2 )
    {
        for( i = 3; i * i <= N; i += 2 )
            if( N % i == 0 )
                goto ContOuter;  /* Sorry about this! */
        return N;
        ContOuter: ;
    }
}

unsigned int Hash(DWORD key, unsigned int tableSize)
{
	return key % tableSize;
}

PHASHTABLE InitializeTable(unsigned int tableSize)
{
	PHASHTABLE pHashTable = NULL;
	PTWOWAY pNode = NULL;
	unsigned int i;

	// Allocate space for the hashtable
	pHashTable = ExAllocatePoolWithTag(NonPagedPool, sizeof(HASHTABLE), DRIVERTAG1);
	if (pHashTable == NULL)
	{
		DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
		return NULL;
	}

	pHashTable->tableSize = NextPrime(tableSize);

	// Allocate array of pointers to linkedlists 
	pHashTable->pListHeads = ExAllocatePoolWithTag(NonPagedPool, sizeof(PLIST_ENTRY) * pHashTable->tableSize, DRIVERTAG1);
	if (pHashTable->pListHeads == NULL)
	{
		DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
		return NULL;
	}

	// Allocate space for the lookaside list for the TWOWAY-structures.
	pLookasideList_TWOWAY = ExAllocatePoolWithTag(NonPagedPool, sizeof(NPAGED_LOOKASIDE_LIST), DRIVERTAG1);
	if (pLookasideList_TWOWAY == NULL)
	{
		DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
		return NULL;
	}

	// Initialize the lookaside list.
	ExInitializeNPagedLookasideList(
		pLookasideList_TWOWAY,
		NULL,
		NULL,
		0,
		sizeof(TWOWAY),
		DRIVERTAG1,
		0);

	// Allocate empty nodes for the each linked list.
	for (i = 0; i < pHashTable->tableSize; i++)
	{
		pNode = ExAllocateFromNPagedLookasideList(pLookasideList_TWOWAY);
		if (pNode == NULL)
		{
			DbgPrint("ExAllocateFromNPagedLookasideList returned NULL!\n");
			return NULL;
		}
		else
		{
			pNode->key = 0x00000000;
			RtlZeroMemory(&pNode->data, sizeof(ELEMENT));
			InitializeListHead(&pNode->linkfield);
		}
		pHashTable->pListHeads[i] = &pNode->linkfield;
	}

	return pHashTable;
}

PTWOWAY Find(DWORD key, PHASHTABLE pHashTable)
{
	PTWOWAY pNode = NULL;
	PLIST_ENTRY pListHead = NULL;
	PLIST_ENTRY pListLink = NULL;

	pListHead = pListLink = pHashTable->pListHeads[Hash(key, pHashTable->tableSize)];
	if (pListHead == NULL)
	{
		DbgPrint("pListHead is NULL!\n");
		return NULL;
	}

	if (!IsListEmpty(pListHead))
	{
		do
		{
			pNode = CONTAINING_RECORD(pListLink, TWOWAY, linkfield);
			if (pNode->key == key)
			{
				return pNode;
			}
			pListLink = pListLink->Flink;
		} while (pListLink != pListHead);
	}

	return NULL;
}

void Insert(DWORD key, PDATA pData, PHASHTABLE pHashTable)
{
	PTWOWAY pNode = NULL; 
	PTWOWAY pNewNode = NULL;
	PLIST_ENTRY pListHead = NULL;

	pNode = Find(key, pHashTable);
	// The node with the given key was not found.
	if (pNode == NULL)
	{
		pNewNode = ExAllocateFromNPagedLookasideList(pLookasideList_TWOWAY);
		if (pNewNode == NULL)
		{
			DbgPrint("ExAllocateFromNPagedLookasideList returned NULL!\n");
			return;
		}
		
		// Insert the data to the node.
		pNewNode->key = key;
		pNewNode->data.threadID = pData->threadID;
		pNewNode->data.processID = pData->processID;
		if (STATUS_SUCCESS != RtlStringCbCopyA(pNewNode->data.imageName, 16, pData->imageName))
		{
			DbgPrint("RtlStringCbCopyA failed!\n");
		}

		// Insert the node to the doubly-linked list.
		pListHead = pHashTable->pListHeads[Hash(key, pHashTable->tableSize)];
		InsertTailList(pListHead, &pNewNode->linkfield);
#ifdef MY_DEBUG
		DbgPrint("INSERT: thread ID = 0x%x process ID = 0x%x image = %s\n", pData->threadID, pData->processID, pData->imageName);
#endif
	}
#ifdef MY_DEBUG
	else
	{
		if (pNode->data.processID != pData->processID)
		{
			DbgPrint("Node with key = 0x%x already in list\n", key);
			DbgPrint("OLD: thread ID = 0x%x process ID = 0x%x image = %s\n", pNode->data.threadID, pNode->data.processID, pNode->data.imageName);
			DbgPrint("NEW: thread ID = 0x%x process ID = 0x%x image = %s\n", pData->threadID, pData->processID, pData->imageName);
		}
	}
#endif
}

void Remove(DWORD key, PHASHTABLE pHashTable)
{
	PTWOWAY pNode = NULL; 
	PLIST_ENTRY pListHead = NULL;

	pNode = Find(key, pHashTable);

	// The node with the given key was found.
	if (pNode != NULL)
	{
#ifdef MY_DEBUG
		DbgPrint("REMOVE: thread ID = 0x%x process ID = 0x%x image = %s\n", pNode->data.threadID, pNode->data.processID, pNode->data.imageName);
#endif
		RemoveEntryList(&pNode->linkfield);
		ExFreeToNPagedLookasideList(pLookasideList_TWOWAY, pNode);
	}
}

void DestroyTable(PHASHTABLE pHashTable)
{
	PTWOWAY pNode = NULL; 
	PTWOWAY pTmpNode = NULL;
	PLIST_ENTRY pListHead = NULL;
	PLIST_ENTRY pListLink = NULL;
    unsigned int i;

	for (i = 0; i < pHashTable->tableSize; i++)
	{
		pListHead = pListLink = pHashTable->pListHeads[i];
		if (pListHead == NULL)
		{
			DbgPrint("pListHead is NULL!\n");
			continue;
		}
		if (!IsListEmpty(pListHead))
		{
			do
			{
				pNode = CONTAINING_RECORD(pListLink, TWOWAY, linkfield);
				pListLink = pListLink->Flink;
				ExFreeToNPagedLookasideList(pLookasideList_TWOWAY, pNode);
			} while (pListLink != pListHead);
		}
		else
		{
			pNode = CONTAINING_RECORD(pListHead, TWOWAY, linkfield);
			ExFreeToNPagedLookasideList(pLookasideList_TWOWAY, pNode);
		}
	}

	ExDeleteNPagedLookasideList(pLookasideList_TWOWAY);
	ExFreePoolWithTag(pLookasideList_TWOWAY, DRIVERTAG1);
	ExFreePoolWithTag(pHashTable->pListHeads, DRIVERTAG1);
	ExFreePoolWithTag(pHashTable, DRIVERTAG1);
}

void DumpTable(PHASHTABLE pHashTable)
{
	PTWOWAY pNode = NULL;
	PLIST_ENTRY pListHead = NULL;
	PLIST_ENTRY pListLink = NULL;
    unsigned int i;

	for (i = 0; i < pHashTable->tableSize; i++)
	{
		pListHead = pListLink = pHashTable->pListHeads[i];
		if (pListHead == NULL)
		{
			DbgPrint("pListHead is NULL!\n");
			continue;
		}
		if (!IsListEmpty(pListHead))
		{
			do
			{
				pNode = CONTAINING_RECORD(pListLink, TWOWAY, linkfield);
				pListLink = pListLink->Flink;
				if (pNode->key != 0)
				{
					DbgPrint("key = 0x%8x  threadID = %4d  processID = %4ld  image = %s\n",
						pNode->key, pNode->data.threadID, pNode->data.processID, pNode->data.imageName);
				}
			} while (pListLink != pListHead);
		}
	}
}