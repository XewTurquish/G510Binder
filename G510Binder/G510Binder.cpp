#define _CRT_SECURE_NO_DEPRECATE

#include "stdafx.h"
#include <iostream>
#include "FS_Thread.hpp"
#include "RCThread.h"
#include "Utility.h"
#include "G510Binder.h"

#define COMMAND(command) if(handled == false && strcmp(args[0], command) == 0)

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}

G510BinderPlugin::G510BinderPlugin()
{
	strcpy(m_ModuleInfo.PluginName, "G510Binder");
	strcpy(m_ModuleInfo.Author, "Xew");
	strcpy(m_ModuleInfo.Website, "http://www.windower.net/");
	m_ModuleInfo.Version = 0.1;
	strcpy(m_ModuleInfo.Identifier, "G510Binder");
}

G510BinderPlugin::~G510BinderPlugin()
{
}

ModuleInfo __stdcall G510BinderPlugin::GetModuleInfo()
{
	odprintf("GetModuleInfo");
	return m_ModuleInfo;
}


void __stdcall G510BinderPlugin::Initialize(IWindower* windower)
{
	m_Windower = windower;
	m_Console = windower->GetConsole();

	m_RCThread.Setup(m_Windower);
	
	m_RCThread.Start();
	m_ThreadStarted = true;

	MState = "M1";
	XMLHidDevice *CurrentDevice;
	char Buffer[2048];
	char Buffer2[2048];
	for(int i = 0; i < wcslen(m_Windower->GetSettings().swzWindowerPath); i++)
	{
		Buffer[i] = ((char*)m_Windower->GetSettings().swzWindowerPath)[i*2];
		Buffer[i+1] = 0;
	}
	sprintf(Buffer, "%sPlugins\\Gamepads.xml", Buffer);
	CurrentDevice = new XMLHidDevice;
	CurrentDevice->LoadXML(Buffer);
	std::vector<string> Names = CurrentDevice->GetNames();
	for(int i = 0; i < Names.size(); i++)
	{
		if(i)
		{
			CurrentDevice = new XMLHidDevice;
			CurrentDevice->LoadXML(Buffer);
		}
		CurrentDevice->UseDevice(Names[i]);
		if(CurrentDevice->Connected)
		{
			sprintf(Buffer2, "%s found.\n", CurrentDevice->GetName().c_str());
			m_Console->Write(Buffer2);
			CurrentDevice->OnExtraKeysPressed = (fpOnExtraKeys)OnKeyPress;
			//CurrentDevice->OnExtraKeysReleased = (fpOnExtraKeys)OnKeyPress;
			CurrentDevice->OnShiftStateChanged = (fpOnShiftStateChanged)OnShiftStateChanged;
			CurrentDevice->OnAxisChanged = (fpOnAxisChanged)OnAxisChanged;
			CurrentDevice->SetOwner(this);
			HidDevices.push_back(CurrentDevice);
			sprintf(Buffer2, "xmlbinderstate%s", CurrentDevice->GetName().c_str());
			ITextObject *Temp = m_Windower->GetTextHandler()->CreateTextObject(Buffer2);

			States.push_back(Temp);
			States.back()->SetText((char *)HidDevices.back()->CurrentShiftState.c_str());
			States.back()->SetVisibility(true);
			States.back()->SetFont("Times New Roman",9);
			States.back()->SetLocation(0,m_Windower->GetSettings().iYRes - (States.size() * 10) - 7);
		}
		else
			delete CurrentDevice;
	}
	
	debug = false;
	m_Console->Write("G510Binder: Initialized\n");
}

void __stdcall G510BinderPlugin::Dispose()
{
	char Buffer[2048];
	for(int i = 0; i < HidDevices.size(); i++)
	{
		States[i]->SetVisibility(false);
		delete HidDevices[i];
	}
}

__declspec(dllexport) IPlugin* CreateInstance()
{
	return (IPlugin*)new G510BinderPlugin();
}

void __stdcall G510BinderPlugin::HandleCommand(char *command)
{
	char* args[32];
	int argcount = GetArgs(command, args);
	if(argcount == 0 || strlen(args[0]) == 0)
		return;

	bool handled = false;

	COMMAND("debug")
	{
		debug = !debug;
		handled = true;
	}
	COMMAND("AltNames")
	{
		XMLHidDevice *CurrentDevice = NULL;
		for(int i = 0; i < HidDevices.size(); ++i)
		{
			if(HidDevices[i]->GetName() == args[1])
			{
				CurrentDevice = HidDevices[i];
				break;
			}
		}
		if(CurrentDevice == NULL)
			return;

		char Buffer[1024];
		for(int i = 0; i < CurrentDevice->ExtraKeyNames.size(); ++i)
		{
			sprintf(Buffer, "%s > %s", CurrentDevice->ExtraKeyNames[1 << (i+1)].c_str(), CurrentDevice->AltNames[CurrentDevice->ExtraKeyNames[1 << (i+1)]].c_str());
			m_Windower->GetFFXI()->AddToChat(Buffer, 1024);
		}
		handled = true;
	}
	COMMAND("ShiftStates")
	{
		XMLHidDevice *CurrentDevice = NULL;
		for(int i = 0; i < HidDevices.size(); ++i)
		{
			if(HidDevices[i]->GetName() == args[1])
			{
				CurrentDevice = HidDevices[i];
				break;
			}
		}
		if(CurrentDevice == NULL)
			return;
		char Buffer[11024];
		for(int i = 0; i < CurrentDevice->ShiftStates.size(); ++i)
		{
			sprintf(Buffer, "%s", CurrentDevice->ShiftStates[i].Name.c_str());
			m_Windower->GetFFXI()->AddToChat(Buffer, 1024);
			switch(CurrentDevice->ShiftStates[i].Type)
			{
			case SS_NONE:
				sprintf(Buffer, "None");
				m_Windower->GetFFXI()->AddToChat(Buffer, 1024);
				break;
			case SS_AXIS:
				sprintf(Buffer, "AXIS > %s [%2X - %2X]", CurrentDevice->ShiftStates[i].Axises[0].Name.c_str(), CurrentDevice->ShiftStates[i].Axises[0].MinValue, CurrentDevice->ShiftStates[i].Axises[0].MaxValue);
				m_Windower->GetFFXI()->AddToChat(Buffer, 1024);
				break;
			case SS_MULTIPLE_AXIS:
				sprintf(Buffer, "Multiple Axis");
				m_Windower->GetFFXI()->AddToChat(Buffer, 1024);
				for(int j = 0; j < CurrentDevice->ShiftStates[i].Axises.size(); ++j)
				{
					sprintf(Buffer, "AXIS > %s [%2X - %2X]", CurrentDevice->ShiftStates[i].Axises[j].Name.c_str(), CurrentDevice->ShiftStates[i].Axises[j].MinValue, CurrentDevice->ShiftStates[i].Axises[j].MaxValue);
					m_Windower->GetFFXI()->AddToChat(Buffer, 1024);
				}
				break;
			case SS_BUTTON:
				sprintf(Buffer, "Button %s", CurrentDevice->ShiftStates[i].Button.c_str());
				m_Windower->GetFFXI()->AddToChat(Buffer, 1024);
				break;
			}
		}
	}
}

void OnKeyPress(G510BinderPlugin *Owner, XMLHidDevice *Sender, ULLONG Keys)
{
	char *Buffer;
	for(int i= 0; i < Sender->ExtraKeyNames.size(); i ++)
	{
		if(Sender->AltNames[Sender->ExtraKeyNames[1 << (i+1)]].length())
		{
			Buffer = new char[Sender->CurrentShiftState.length() + Sender->AltNames[Sender->ExtraKeyNames[1 << (i+1)]].length()+1];
			if(Keys & 1 << (i+1))
			{
				if(Sender->AltNames[Sender->ExtraKeyNames[1 << (i+1)]] != Sender->CurrentShiftState)
					sprintf(Buffer, "%s%s", Sender->CurrentShiftState.c_str(), Sender->AltNames[Sender->ExtraKeyNames[1 << (i+1)]].c_str());
				else
					sprintf(Buffer, "");
				if(Owner->debug)
					Owner->m_Windower->GetConsole()->Write(Buffer);
				Owner->m_Windower->GetConsole()->HandleCommand(false,Buffer,false);
			}
			delete Buffer;
		}
	}
}

void OnShiftStateChanged(G510BinderPlugin *Owner, XMLHidDevice *Sender, string NewState)
{
	if(Owner->debug)
		Owner->m_Windower->GetConsole()->Write((char*)NewState.c_str());
	Owner->m_Windower->GetConsole()->HandleCommand(false, (char*)NewState.c_str(), false);
	for(int i = 0; i < Owner->HidDevices.size(); ++i)
	{
		if(Owner->HidDevices[i] == Sender)
		{
			Owner->States[i]->SetText((char*)NewState.c_str());
		}
	}
}

void OnAxisChanged(G510BinderPlugin *Owner, XMLHidDevice *Sender, ULLONG Axis, unsigned char Value)
{
	if(Owner->debug)
	{
		char buffer[64];
		sprintf(buffer, "%s %s %X\n", Sender->GetName().c_str(), Sender->ExtraKeyNames[Axis].c_str(), Value);
		Owner->m_Console->Write(buffer);
	}
}