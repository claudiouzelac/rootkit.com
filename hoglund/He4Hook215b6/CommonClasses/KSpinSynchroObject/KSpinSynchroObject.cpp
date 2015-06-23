#include "KSpinSynchroObject.h"

KSpinSynchroObject::KSpinSynchroObject()
                  : KSynchroObject()
{
  #ifndef __TEST_WIN32
  m_CurrentOwner = NULL;
  m_dwLockCount = 0;
  KeInitializeSpinLock(&m_Lock);
  #endif //__TEST_WIN32
}

KSpinSynchroObject::~KSpinSynchroObject()
{
}

void KSpinSynchroObject::Lock(IN ULONG dwContext)
{
  #ifndef __TEST_WIN32
  PETHREAD CurrentThread = PsGetCurrentThread();
  KIRQL    OldIrql;

  if (m_CurrentOwner != CurrentThread)
  {
    KeAcquireSpinLock(&m_Lock, &OldIrql);
    m_OldIrql = OldIrql;
    m_CurrentOwner = CurrentThread;
    m_dwLockCount = 1;
  }
  else
  {
    if (m_dwLockCount != 0xffffffff)
      ++m_dwLockCount;
  }
  #endif //__TEST_WIN32
}  

void KSpinSynchroObject::UnLock(IN ULONG dwContext)
{
  #ifndef __TEST_WIN32
  PETHREAD CurrentThread = PsGetCurrentThread();

  if (m_CurrentOwner == CurrentThread)
  {
    if (m_dwLockCount > 1)
    {
      --m_dwLockCount;
    }
    else
    {
      m_CurrentOwner = NULL;
      m_dwLockCount = 0;
      KeReleaseSpinLock(&m_Lock, m_OldIrql);
    }
  }
  #endif //__TEST_WIN32
}

