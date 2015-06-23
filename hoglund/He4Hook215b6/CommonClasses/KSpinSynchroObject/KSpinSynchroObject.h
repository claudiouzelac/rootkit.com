#ifndef __KSPINSYNCHROOBJECT_H
 #define __KSPINSYNCHROOBJECT_H

#ifndef __TEST_WIN32
 extern "C"
 {
  #include "ntddk.h"
 }
#endif //__TEST_WIN32

#include "../KSynchroObject/KSynchroObject.h"

class KSpinSynchroObject;
//**************************************************************************//

class KSpinSynchroObject : public KSynchroObject
{
  public:

   explicit
   KSpinSynchroObject();
   virtual ~KSpinSynchroObject();

   virtual void        Lock(IN ULONG dwContext);
   virtual void        UnLock(IN ULONG dwContext);

  private:
   KSpinSynchroObject(const KSpinSynchroObject&);
   KSpinSynchroObject& operator=(const KSpinSynchroObject& right);

  private:
   #ifndef __TEST_WIN32
   KSPIN_LOCK          m_Lock;
   PETHREAD            m_CurrentOwner;
   KIRQL               m_OldIrql;
   ULONG               m_dwLockCount;
   #else
   DWORD               m_Lock;
   #endif //__TEST_WIN32
};

#endif //__CSPINSYNCHROOBJECT_H
