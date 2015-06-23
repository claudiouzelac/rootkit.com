#ifndef __gdt_h__
#define __gdt_h__

VOID DumpGDT();
int GetFirstAvailableGDTSlot();
void TestCallgate(int GDT_Selector);
int InstallCallgate();
VOID RemoveCallgate(int GDT_Selector);


#endif