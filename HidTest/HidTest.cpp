// HidTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\G510Binder\LogitechDevice.h"
#include <conio.h>

void OnKeyPress(void *Owner, XMLHidDevice *Sender, ULLONG Keys);

int _tmain(int argc, _TCHAR* argv[])
{
	XMLHidDevice MyDevice;
	MyDevice.LoadXML("..\\G510Binder\\Gamepads.xml");
	MyDevice.UseDevice(MyDevice.GetNames()[3]);

	MyDevice.OnExtraKeysPressed = (fpOnExtraKeys)OnKeyPress;

	char temp;
	temp = getch();
	while(temp != 13)
	{
		temp = getch();
	}
	return 0;
}

void OnKeyPress(void *Owner, XMLHidDevice *Sender, ULLONG Keys)
{
	char *Buffer;
	for(ULLONG i= 0; i < Sender->ExtraKeyNames.size(); i ++)
	{
		if(Sender->AltNames[Sender->ExtraKeyNames[(ULLONG)1 << (i+1)]].length())
		{
			
			if(Keys & (ULLONG)1 << (i+1))
			{
				Buffer = new char[Sender->CurrentShiftState.length() + Sender->AltNames[Sender->ExtraKeyNames[(ULLONG)1 << (i+1)]].length()+2];
				if(Sender->AltNames[Sender->ExtraKeyNames[(ULLONG)1 << (i+1)]] != Sender->CurrentShiftState)
					sprintf(Buffer, "%s%s\n", Sender->CurrentShiftState.c_str(), Sender->AltNames[Sender->ExtraKeyNames[(ULLONG)1 << (i+1)]].c_str());
				else
					sprintf(Buffer, "");
				printf(Buffer);
				delete Buffer;
			}
		}
	}
}