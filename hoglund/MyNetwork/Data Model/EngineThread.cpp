///////////////////////////////////////////////////////////
//  EngineThread.cpp
//  Implementation of the Class EngineThread
//  Created on:      24-Dec-2003 11:58:23 AM
///////////////////////////////////////////////////////////

#include "stdafx.h"
#include <process.h>
#include "EngineThread.h"
#include "UserConsole.h"

DWORD WINAPI _enginefunction(LPVOID *p);

EngineThread::EngineThread() :
m_stop_flag(TRUE),
m_thread_handle(0)
{

}

EngineThread::~EngineThread()
{

}

void EngineThread::Start()
{
	DWORD id;
	m_stop_flag = FALSE;
	m_thread_handle = CreateThread( 
							NULL,
							0,
							(LPTHREAD_START_ROUTINE)_enginefunction, 
							this,
							0,
							&id);
}

void EngineThread::Stop()
{
	if(m_thread_handle)
	{
		m_stop_flag = TRUE;
		WaitForSingleObject(m_thread_handle, INFINITE);
	}
	else
	{
		TRACE0("Error, no thread handle");
	}
}

void EngineThread::Run()
{
	// do something useful here
	while(1)
	{
		if(m_stop_flag) break;
		Sleep(1);
	}
}

//generic starter for all thread objects
DWORD WINAPI _enginefunction(LPVOID *p)
{
	EngineThread *theThread = (EngineThread *)p;
	theThread->Run();
	return(0);
}


