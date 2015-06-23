#ifndef __KINTERLOCKEDCOUNTER_H
 #define __KINTERLOCKEDCOUNTER_H

extern "C"
{
 #include "ntddk.h"
}

#include "../Include/KNew.h"

class KInterlockedCounter;
//********************************************************************//

class KInterlockedCounter
{
  public:
   explicit
   KInterlockedCounter(LONG nStartValue = 0);
   virtual ~KInterlockedCounter();

   KInterlockedCounter& operator=(const KInterlockedCounter& right);
   KInterlockedCounter& operator=(const LONG nValue);

   KInterlockedCounter& operator++();
   const KInterlockedCounter operator++(int);
   KInterlockedCounter& operator--();
   const KInterlockedCounter operator--(int);

   BOOLEAN CompareExchange(const LONG nValueComperand, const LONG nValueExchange);

 private:
   KInterlockedCounter(const KInterlockedCounter& right);

 protected:
   LONG     m_nValue;

};

#endif //__KINTERLOCKEDCOUNTER_H