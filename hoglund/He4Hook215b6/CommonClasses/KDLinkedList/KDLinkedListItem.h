#ifndef __KDLINKEDLIST_ITEM_H
 #define __KDLINKEDLIST_ITEM_H

#ifndef __TEST_WIN32
 extern "C"
 {
  #include "ntddk.h"
 }
 
 #include "../Include/KNew.h"
 #include "../Include/KTypes.h"
#else
 #include <windows.h>
#endif //__TEST_WIN32


class KDLinkedListItem;
//*******************************************************************//

class KDLinkedListItem
{
  public:
   explicit
   KDLinkedListItem(VOID* pObject, KDLinkedListItem* pPrev = NULL, KDLinkedListItem* pNext = NULL);
   virtual ~KDLinkedListItem();

   KDLinkedListItem* GetPrev();
   VOID              SetPrev(KDLinkedListItem* pPrev);
   KDLinkedListItem* GetNext();
   VOID              SetNext(KDLinkedListItem* pNext);
   VOID*             GetObject();
   VOID              SetObject(VOID* pObject);

  private:
   KDLinkedListItem(const KDLinkedListItem&);
   KDLinkedListItem& operator=(const KDLinkedListItem& right);

  protected:
   KDLinkedListItem*  m_pPrev;
   KDLinkedListItem*  m_pNext;
   VOID*              m_pObject;
};


#endif //__KDLINKEDLIST_ITEM_H