#include "KMutexSynchroObject.h"

KMutexSynchroObject::KMutexSynchroObject()
                   : KSynchroObject()
{
  KeInitializeMutex(&m_Lock, 0);
}

KMutexSynchroObject::~KMutexSynchroObject()
{

}

void KMutexSynchroObject::Lock(IN ULONG dwContext)
{
  KeWaitForMutexObject(&m_Lock, Executive, KernelMode, FALSE, NULL);
}  

void KMutexSynchroObject::UnLock(IN ULONG dwContext)
{
  KeReleaseMutex(&m_Lock, FALSE);
}

