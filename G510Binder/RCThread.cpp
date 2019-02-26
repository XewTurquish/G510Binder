#include "stdafx.h"
#include "FS_Thread.hpp"
#include "RCThread.h"
#include "FFXiBase.h"
#include "Utility.h"



RCThread::RCThread()
{
	m_Windower = NULL;
	m_Console = NULL;
}

RCThread::~RCThread()
{
}

void RCThread::Setup(IWindower* windower)
{
	m_Windower = windower;
	m_Console = windower->GetConsole();
}

unsigned int RCThread::EntryPoint(void)
{
	while (!bIsTerminationSignaled())
	{
		
		Sleep(100);
	}
	return true;
}

