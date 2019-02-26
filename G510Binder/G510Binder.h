#pragma once

#include "plugin.h"
#include "guiddef.h"
#include "ffxibase.h"
#include "LogitechDevice.h"

class G510BinderPlugin : public PluginBase, public FFXiBase
{
public:
	G510BinderPlugin();
	virtual ~G510BinderPlugin();
	ModuleInfo __stdcall GetModuleInfo();

	void __stdcall Initialize(IWindower* windower);
	void __stdcall Dispose();

	void __stdcall HandleCommand(char * command);

	string MState;
private:
	ModuleInfo m_ModuleInfo;
	RCThread m_RCThread;
	bool m_ThreadStarted;
	IWindower* m_Windower;
	IConsole* m_Console;
	std::vector<XMLHidDevice *> HidDevices;
	bool debug;
	std::vector<ITextObject *> States;
	
	friend void OnKeyPress(G510BinderPlugin *Owner, XMLHidDevice* Sender, ULLONG Keys);
	friend void OnShiftStateChanged(G510BinderPlugin *Owner, XMLHidDevice *Sender, string NewState);
	friend void OnAxisChanged(G510BinderPlugin *Owner, XMLHidDevice *Sender, ULLONG Axis, unsigned char Value);
};

__declspec(dllexport) IPlugin *CreateInstance();

__declspec(dllexport) double __stdcall GetInterfaceVersion()
{
	return PluginInterfaceVersion;
};

__declspec(dllexport) GUID __stdcall GetGUID()
{
	GUID G510BinderGUID = { 0xEED1C65B, 0x38F5, 0x466B, { 0x86, 0xA6, 0x46, 0x53, 0x04, 0x1E, 0x10, 0x06} };
	return G510BinderGUID;
}