#include "KDLinkedListItem.h"

KDLinkedListItem::KDLinkedListItem(VOID* pObject, KDLinkedListItem* pPrev, KDLinkedListItem* pNext)
{
  m_pPrev = pPrev;
  m_pNext = pNext; 
  m_pObject = pObject;
}

KDLinkedListItem::~KDLinkedListItem()
{

}

KDLinkedListItem* KDLinkedListItem::GetPrev()
{
  return m_pPrev;
}

VOID KDLinkedListItem::SetPrev(KDLinkedListItem* pPrev)
{
  m_pPrev = pPrev;
}

KDLinkedListItem* KDLinkedListItem::GetNext()
{
  return m_pNext;
}

VOID KDLinkedListItem::SetNext(KDLinkedListItem* pNext)
{
  m_pNext = pNext;
}

VOID* KDLinkedListItem::GetObject()
{
  return m_pObject;
}

VOID KDLinkedListItem::SetObject(VOID* pObject)
{
  m_pObject = pObject;
}
