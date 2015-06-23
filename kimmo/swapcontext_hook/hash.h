#ifndef _Hash_H
#define _Hash_H

typedef unsigned char BYTE;
typedef unsigned long DWORD;

typedef struct _DATA
{
	unsigned int threadID;
	unsigned int processID;
	BYTE *imageName;
} DATA, *PDATA;

typedef struct _HASHTABLE HASHTABLE, *PHASHTABLE;

PHASHTABLE InitializeTable(unsigned int tableSize);
void Insert(DWORD key, PDATA pData, PHASHTABLE pHashTable);
void Remove(DWORD key, PHASHTABLE pHashTable);
void DestroyTable(PHASHTABLE pHashTable);
void DumpTable(PHASHTABLE pHashTable);

#endif // _Hash_H
