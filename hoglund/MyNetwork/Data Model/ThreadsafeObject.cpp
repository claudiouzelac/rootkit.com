///////////////////////////////////////////////////////////
//  ThreadsafeObject.cpp
//  Implementation of the Class ThreadsafeObject
//  Created on:      24-Dec-2003 11:58:27 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ThreadsafeObject.h"


ThreadsafeObject::ThreadsafeObject() :
m_current_lock_thread(0),
m_refcount(0)
{
	InitializeCriticalSection(&m_tech);
}



ThreadsafeObject::~ThreadsafeObject()
{
	DeleteCriticalSection(&m_tech);
}

void ThreadsafeObject::LockMutex()
{
#if 0
	HANDLE ct = GetCurrentThread();
	if(ct == m_current_lock_thread) 
	{
		//recursive locking attempt, no-op it
		m_refcount++;
		TRACE1("lock refup %d", m_refcount);
		return; 
	}
#endif
	
	EnterCriticalSection(&m_tech);
	
#if 0
	m_current_lock_thread = ct;
	m_refcount++;
	TRACE1("lock start ref %d", m_refcount);
	assert(1 == m_refcount); 
#endif
}

void ThreadsafeObject::UnlockMutex()
{
#if 0
	HANDLE ct = GetCurrentThread();
	if(ct == m_current_lock_thread) 
	{
		//recursive un-locking attempt
		m_refcount--;
		TRACE1("lock refdown %d", m_refcount);
	}
	if(m_refcount) return; //not at zero yet, don't truly unlock 	
	m_current_lock_thread = 0;
#endif

	LeaveCriticalSection(&m_tech);
}

