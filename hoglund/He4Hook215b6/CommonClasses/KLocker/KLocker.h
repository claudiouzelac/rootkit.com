#ifndef __KLOCKER_H
 #define __KLOCKER_H

#ifndef __TEST_WIN32
 extern "C"
 {
  #include "ntddk.h"
 }
#endif //__TEST_WIN32


class KLocker;
class KSynchroObject;
//**************************************************************************//

class KLocker
{
  public:
   KLocker(KSynchroObject* pSynchroObject);
   virtual ~KLocker();

  protected:

  private:
   KLocker(const KLocker&);
   KLocker& operator=(const KLocker& right);

  private:
   KSynchroObject* m_pSynchroObject;
   #ifndef __TEST_WIN32
   KIRQL           m_OldIrql;
   #endif //__TEST_WIN32
};

#endif //__KLOCKER_H
