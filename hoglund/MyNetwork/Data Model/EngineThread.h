///////////////////////////////////////////////////////////
//  EngineThread.h
//  Implementation of the Class EngineThread
//  Created on:      24-Dec-2003 11:58:23 AM
///////////////////////////////////////////////////////////

#if !defined(EngineThread_E252F1AA_9C93_4af6_88DC_2797AD858539__INCLUDED_)
#define EngineThread_E252F1AA_9C93_4af6_88DC_2797AD858539__INCLUDED_

#include "ThreadsafeObject.h"

class EngineThread : public ThreadsafeObject
{
protected:
	bool	m_stop_flag;
	HANDLE  m_thread_handle;
public:
	EngineThread();
	virtual ~EngineThread();
	virtual void Start();
	virtual void Stop();

	// don't call Run directly, instead call Start
	virtual void Run();

};
#endif // !defined(EngineThread_E252F1AA_9C93_4af6_88DC_2797AD858539__INCLUDED_)