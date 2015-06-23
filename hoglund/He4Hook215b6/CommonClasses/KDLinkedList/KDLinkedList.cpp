#include "KDLinkedList.h"
#include "../KLocker/KLocker.h"

KDLinkedList::KDLinkedList(VOID* pObject, KDLinkedListItem* pPrev, KDLinkedListItem* pNext)
            : KDLinkedListItem(pObject, pPrev, pNext)
{
//  m_pStartMatchDirItem = NULL;
  m_pHead = NULL;
  m_pTail = NULL;
}

KDLinkedList::~KDLinkedList()
{
  Clear();
}

void KDLinkedList::Clear()
{
  KLocker locker(&m_KSynchroObject);

  KDLinkedListItem* pItem;
  KDLinkedListItem* pItemNext;

  pItem = m_pHead;
  while (pItem != NULL)
  {
    pItemNext = pItem->GetNext();
    delete pItem;
    pItem = pItemNext;
  }
}

PVOID KDLinkedList::GetHeadObject()
{
  KLocker locker(&m_KSynchroObject);

  PVOID pObject = NULL;

  if (m_pHead != NULL)
    pObject = m_pHead->GetObject();

  return pObject;
}

PVOID KDLinkedList::GetTailObject()
{
  KLocker locker(&m_KSynchroObject);

  PVOID pObject = NULL;

  if (m_pTail != NULL)
    pObject = m_pTail->GetObject();

  return pObject;
}

BOOLEAN KDLinkedList::AddHeadObject(PVOID pObject)
{
  KLocker locker(&m_KSynchroObject);
  return Add(new KDLinkedListItem(pObject, NULL, NULL), (KDLinkedListItem*)NULL);
}

BOOLEAN KDLinkedList::AddTailObject(PVOID pObject)
{
  KLocker locker(&m_KSynchroObject);
  return Add(new KDLinkedListItem(pObject, NULL, NULL), m_pTail);
}

BOOLEAN KDLinkedList::AddObject(PVOID pObject, int nNumber)
{
  KLocker locker(&m_KSynchroObject);

  KDLinkedListItem* pItem = m_pHead;
  int               nCount = 0;
  BOOLEAN           bRes = FALSE;

  while(pItem != NULL)
  {
    if (nCount == nNumber)
    {
      bRes = Add(new KDLinkedListItem(pObject, NULL, NULL), pItem);//Add(pObject, pItem);
      break;
    }
    pItem = pItem->GetNext();
    ++nCount;
  }

  return bRes;
}

BOOLEAN KDLinkedList::AddObject(PVOID pObject, KDLinkedListItem* pItemPrev)
{
  KLocker locker(&m_KSynchroObject);
  return Add(new KDLinkedListItem(pObject, NULL, NULL), pItemPrev);
}

BOOLEAN KDLinkedList::Add(KDLinkedListItem* pItem, KDLinkedListItem* pItemPrev)
{
  KLocker locker(&m_KSynchroObject);

  KDLinkedListItem* pItemNext;

  if (pItem == NULL)
    return FALSE;

  pItem->SetPrev(pItemPrev);
  if (pItemPrev != NULL)
  {
    pItemNext = pItemPrev->GetNext();
    pItem->SetNext(pItemNext);
    pItemPrev->SetNext(pItem);
    if (pItemNext != NULL)
      pItemNext->SetPrev(pItem);
    else
      m_pTail = pItem;
  }
  else
  {
    if (m_pHead == NULL)
    {
      m_pHead = m_pTail = pItem;
      pItem->SetNext(NULL);
    }
    else
    {
      m_pHead->SetPrev(pItem);
      pItem->SetNext(m_pHead);
      m_pHead = pItem;
    }
  }

  return TRUE;
}

PVOID KDLinkedList::RemoveHead()
{
  KLocker locker(&m_KSynchroObject);

  return Remove(m_pHead);
}

PVOID KDLinkedList::RemoveTail()
{
  KLocker locker(&m_KSynchroObject);

  return Remove(m_pTail);
}

PVOID KDLinkedList::Remove(KDLinkedListItem* pItem)
{
  KLocker locker(&m_KSynchroObject);

  KDLinkedListItem*     pItemNext;
  KDLinkedListItem*     pItemPrev;
  PVOID                 pObject = NULL;

  if (pItem != NULL)
  {
    pItemNext = pItem->GetNext();
    pItemPrev = pItem->GetPrev();
    pObject = pItem->GetObject();

    if (pItemPrev != NULL)
    {
      pItemPrev->SetNext(pItemNext);
      if (pItemNext != NULL)
        pItemNext->SetPrev(pItemPrev);
      else
        m_pTail = pItemPrev;
    }
    else
    {
      if (pItemNext != NULL)
      {
        pItemNext->SetPrev(NULL);
        m_pHead = pItemNext;
      }
      else
      {
        m_pHead = m_pTail = NULL;
      }
    }

    delete pItem;
  }

  return pObject;
}

PVOID KDLinkedList::FindObject(int nNumber)
{
  KLocker locker(&m_KSynchroObject);

  KDLinkedListItem* pItem = m_pHead;
  int               nCount = 0;
  PVOID             pObject = NULL;

  while(pItem != NULL)
  {
    if (nCount == nNumber)
    {
      pObject = pItem->GetObject();
      break;
    }
    pItem = pItem->GetNext();
    ++nCount;
  }

  return pObject;
}

BOOLEAN KDLinkedList::IsEmpty()
{
  KLocker locker(&m_KSynchroObject);

  return ((m_pHead == NULL) && (m_pTail == NULL));
}

int KDLinkedList::Size()
{
  KLocker locker(&m_KSynchroObject);

  KDLinkedListItem* pItem = m_pHead;
  int               nCount = 0;

  while(pItem != NULL)
  {
    pItem = pItem->GetNext();
    ++nCount;
  }

  return nCount;
}

KDLinkedListItem* KDLinkedList::GetHead()
{
  KLocker locker(&m_KSynchroObject);
  return m_pHead;
}

KDLinkedListItem* KDLinkedList::GetTail()
{
  KLocker locker(&m_KSynchroObject);
  return m_pTail;
}

KDLinkedListItem* KDLinkedList::Find(PVOID pObject)
{
  KLocker locker(&m_KSynchroObject);

  return FindNext(m_pHead, pObject);
}

KDLinkedListItem* KDLinkedList::FindNext(KDLinkedListItem* pStartItem, PVOID pObject)
{
  KLocker locker(&m_KSynchroObject);

  KDLinkedListItem* pItem;

  pItem = pStartItem;
  while (pItem != NULL)
  {
    if (pItem->GetObject() == pObject)
      return pItem;
    pItem = pItem->GetNext();
  }

  return NULL;
}
