#ifndef __KMUTEXTSYNCHROOBJECT_H
 #define __KMUTEXTSYNCHROOBJECT_H

extern "C"
{
 #include "ntddk.h"
}

#include "../KSynchroObject/KSynchroObject.h"


class KMutexSynchroObject;
//**************************************************************************//

class KMutexSynchroObject : public KSynchroObject
{
  public:
   explicit
   KMutexSynchroObject();
   virtual ~KMutexSynchroObject();

   virtual void        Lock(IN ULONG dwContext = 0);
   virtual void        UnLock(IN ULONG dwContext = 0);

  private:
   KMutexSynchroObject(const KMutexSynchroObject&);
   KMutexSynchroObject& operator=(const KMutexSynchroObject& right);

  private:
   KMUTEX                m_Lock;
};

#endif //__CMUTEXTSYNCHROOBJECT_H
