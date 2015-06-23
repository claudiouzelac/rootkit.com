///////////////////////////////////////////////////////////
//  ThreadsafeObject.h
//  Implementation of the Class ThreadsafeObject
//  Created on:      24-Dec-2003 11:58:27 AM
///////////////////////////////////////////////////////////

#if !defined(ThreadsafeObject_655AA69C_51E6_40ea_97B5_51D8F9AAC14F__INCLUDED_)
#define ThreadsafeObject_655AA69C_51E6_40ea_97B5_51D8F9AAC14F__INCLUDED_

class ThreadsafeObject
{
protected:
	CRITICAL_SECTION m_tech;
	HANDLE	m_current_lock_thread;
	int		m_refcount;

public:
	ThreadsafeObject();
	virtual ~ThreadsafeObject();

	void LockMutex();
	void UnlockMutex();

};
#endif // !defined(ThreadsafeObject_655AA69C_51E6_40ea_97B5_51D8F9AAC14F__INCLUDED_)