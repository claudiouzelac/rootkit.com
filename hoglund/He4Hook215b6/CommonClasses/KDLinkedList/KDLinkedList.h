#ifndef __KDLINKEDLIST_H
 #define __KDLINKEDLIST_H

#ifndef __TEST_WIN32
 extern "C"
 {
  #include "ntddk.h"
 }
#endif //__TEST_WIN32

#include "KDLinkedListItem.h"
#include "../KSpinSynchroObject/KSpinSynchroObject.h"
//#include "../KNativeSynchroObject/KNativeSynchroObject.h"

class KDLinkedList;
//*******************************************************************//

class KDLinkedList : public KDLinkedListItem
{
  public:
   explicit
   KDLinkedList(VOID* pObject, KDLinkedListItem* pPrev = NULL, KDLinkedListItem* pNext = NULL);
   virtual ~KDLinkedList();

   virtual void              Clear();
   virtual KDLinkedListItem* GetHead();
   virtual KDLinkedListItem* GetTail();
   virtual PVOID             GetHeadObject();
   virtual PVOID             GetTailObject();
   virtual BOOLEAN           AddHeadObject(PVOID pObject);
   virtual BOOLEAN           AddTailObject(PVOID pObject);
   virtual BOOLEAN           AddObject(PVOID pObject, int nNumber);
   virtual BOOLEAN           AddObject(PVOID pObject, KDLinkedListItem* pPrevItem);
   virtual BOOLEAN           Add(KDLinkedListItem* pItem, KDLinkedListItem* pItemPrev);
   virtual PVOID             RemoveHead();
   virtual PVOID             RemoveTail();
   virtual PVOID             Remove(KDLinkedListItem* pItem);
   virtual PVOID             FindObject(int nNumber);
   virtual KDLinkedListItem* Find(PVOID pObject);
   virtual BOOLEAN           IsEmpty();
   virtual int               Size();

  protected:
   virtual KDLinkedListItem* FindNext(KDLinkedListItem* pStartItem, PVOID pObject);

  private:
   KDLinkedList(const KDLinkedList&);
   KDLinkedList& operator=(const KDLinkedList& right);

  protected:

   KDLinkedListItem*      m_pHead;
   KDLinkedListItem*      m_pTail;

   KSpinSynchroObject     m_KSynchroObject;
   //KNativeSynchroObject   m_KSynchroObject;
};


#endif //__KDLINKEDLIST_H