#ifndef __KSYNCHROOBJECT_H
 #define __KSYNCHROOBJECT_H

#ifndef __TEST_WIN32
 extern "C"
 {
  #include "ntddk.h"
 }
 
 #include "../Include/KNew.h"
#else
 #include <windows.h>
#endif //__TEST_WIN32

class KSynchroObject;
//**************************************************************************//

class KSynchroObject
{
  public:
   explicit
   KSynchroObject();
   virtual ~KSynchroObject();

   virtual void        Lock(IN ULONG dwContext) = 0;
   virtual void        UnLock(IN ULONG dwContext) = 0;

  private:
   KSynchroObject(const KSynchroObject&);
   KSynchroObject& operator=(const KSynchroObject& right);

  private:
};

#endif //__CSYNCHROOBJECT_H
