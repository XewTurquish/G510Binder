#pragma once

#include "plugin.h"
#include "d3d8.h"

class RCThread : public FS_Thread
{
	public:
		RCThread();
		~RCThread();
		void Setup(IWindower* windower);
		unsigned int EntryPoint();

	private:
		bool m_Visible;
		IConsole* m_Console;
		IWindower* m_Windower;

};