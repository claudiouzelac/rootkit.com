#include "../KSynchroObject/KSynchroObject.h"
#include "KLocker.h"

KLocker::KLocker(KSynchroObject* pSynchroObject)
{
  m_pSynchroObject = pSynchroObject;
  #ifndef __TEST_WIN32
  m_pSynchroObject->Lock((ULONG)&m_OldIrql);
  #endif //__TEST_WIN32
}

KLocker::~KLocker()
{
  #ifndef __TEST_WIN32
  m_pSynchroObject->UnLock((ULONG)m_OldIrql);
  #endif //__TEST_WIN32
}

