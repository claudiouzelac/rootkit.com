#include "KInterlockedCounter.h"

KInterlockedCounter::KInterlockedCounter(LONG nStartValue)
{
  InterlockedExchange(&m_nValue, nStartValue);
}

KInterlockedCounter::KInterlockedCounter(const KInterlockedCounter& right) 
{
  new (this) KInterlockedCounter(right.m_nValue);
}

KInterlockedCounter::~KInterlockedCounter()
{

}

KInterlockedCounter& KInterlockedCounter::operator=(const KInterlockedCounter& right)
{
  new (this) KInterlockedCounter(right.m_nValue);
  return *this;
}

KInterlockedCounter& KInterlockedCounter::operator=(const LONG nValue)
{
  new (this) KInterlockedCounter(nValue);
  return *this;
}

KInterlockedCounter& KInterlockedCounter::operator++()
{
  InterlockedIncrement(&m_nValue);
  return *this;
}

KInterlockedCounter& KInterlockedCounter::operator--()
{
  InterlockedDecrement(&m_nValue);
  return *this;
}

const KInterlockedCounter KInterlockedCounter::operator++(int)
{
  KInterlockedCounter Old = *this;
  ++(*this);
  return Old;
}

const KInterlockedCounter KInterlockedCounter::operator--(int)
{
  KInterlockedCounter Old = *this;
  --(*this);
  return Old;
}

BOOLEAN KInterlockedCounter::CompareExchange(const LONG nValueComperand, const LONG nValueExchange)
{
  #ifndef __WIN2K
  if (InterlockedCompareExchange((PVOID*)&m_nValue, (PVOID)nValueExchange, (PVOID)nValueComperand) == (PVOID)nValueComperand)
    return TRUE;
  #else
  if (InterlockedCompareExchange(&m_nValue, nValueExchange, nValueComperand) == nValueComperand)
    return TRUE;
  #endif //__WIN2K
  return FALSE;
}

