//#include "stdafx.h"
#include "LogitechDevice.h"
#include <cstdio>
#include <ddk\hidclass.h>

HidDevice **HidDevices = NULL;
unsigned long long NumOfDevices = 0;

BOOLEAN ReadOverlapped (PHID_DEVICE cHidDevice, HANDLE CompletionEvent)
/*++
RoutineDescription:
   Given a struct _HID_DEVICE, obtain a read report and unpack the values
   into the InputData array.
--*/
{
	static OVERLAPPED  overlap;
	DWORD bytesRead;
	BOOL readStatus;

	/*
	// Setup the overlap structure using the completion event passed in to
	//  to use for signalling the completion of the Read
	*/

	memset(&overlap, 0, sizeof(OVERLAPPED));
    
	overlap.hEvent = CompletionEvent;
    
	/*
	// Execute the read call saving the return code to determine how to 
	//  proceed (ie. the read completed synchronously or not).
	*/

	readStatus = ReadFile ( cHidDevice->HidDevice,	cHidDevice-> InputReportBuffer, cHidDevice-> Caps.InputReportByteLength, &bytesRead, &overlap);
                          
	/*
	// If the readStatus is FALSE, then one of two cases occurred.  
	//  1) ReadFile call succeeded but the Read is an overlapped one.  Here,
	//      we should return TRUE to indicate that the Read succeeded.  However,
	//      the calling thread should be blocked on the completion event
	//      which means it won't continue until the read actually completes
	//    
	//  2) The ReadFile call failed for some unknown reason...In this case,
	//      the return code will be FALSE
	*/        

	if (!readStatus) 
		return (ERROR_IO_PENDING == GetLastError());
	

	/*
	// If readStatus is TRUE, then the ReadFile call completed synchronously,
	//   since the calling thread is probably going to wait on the completion
	//   event, signal the event so it knows it can continue.
	*/

	else 
	{
		SetEvent(CompletionEvent);
		return true;
	}
}

BOOLEAN WriteOverlapped (PHID_DEVICE cHidDevice, HANDLE CompletionEvent)
{
	static OVERLAPPED  overlap;
	DWORD bytesWriten;
	BOOL WriteStatus;

	/*
	// Setup the overlap structure using the completion event passed in to
	//  to use for signalling the completion of the Read
	*/

	memset(&overlap, 0, sizeof(OVERLAPPED));
    
	overlap.hEvent = CompletionEvent;
	//overlap.Offset = 0x304;
	//overlap.OffsetHigh = 0x1;
    
	/*
	// Execute the read call saving the return code to determine how to 
	//  proceed (ie. the read completed synchronously or not).
	*/
	unsigned char Buffer[4];
	Buffer[0] = 0x04;
	Buffer[1] = 0x80;
	cHidDevice->OutputReportBuffer = new char[cHidDevice->Caps.OutputReportByteLength];
	cHidDevice->OutputReportBuffer[0] = 0x04;
	cHidDevice->OutputReportBuffer[1] = 0x40;
	WriteStatus = WriteFile( cHidDevice->HidDevice,	cHidDevice->OutputReportBuffer, cHidDevice->Caps.OutputReportByteLength, NULL, &overlap);
                          
	/*
	// If the readStatus is FALSE, then one of two cases occurred.  
	//  1) ReadFile call succeeded but the Read is an overlapped one.  Here,
	//      we should return TRUE to indicate that the Read succeeded.  However,
	//      the calling thread should be blocked on the completion event
	//      which means it won't continue until the read actually completes
	//    
	//  2) The ReadFile call failed for some unknown reason...In this case,
	//      the return code will be FALSE
	*/        

	if (!WriteStatus) 
		return (ERROR_IO_PENDING == GetLastError());
	

	/*
	// If readStatus is TRUE, then the ReadFile call completed synchronously,
	//   since the calling thread is probably going to wait on the completion
	//   event, signal the event so it knows it can continue.
	*/

	else 
	{
		SetEvent(CompletionEvent);
		return true;
	}
}

HidDevice::HidDevice()
{
	Connected = false;
	MyMessageHWND = NULL;
	cHidDevice = NULL;
	TerminateThread = false;
	ReadThreadTimeOut = 10;
	OnExtraKeysPressed = NULL;
	OnExtraKeysReleased = NULL;
	Owner = NULL;
	VID = 0;
	PID = 0;
	Usage = 0;
	UsagePage = 0;

	HidDevice **OldDevices = HidDevices;
	HidDevices = new HidDevice*[++NumOfDevices];
	for(int i = 0; i < NumOfDevices-1; i++)
		HidDevices[i] = OldDevices[i];
	if(OldDevices)
		delete[] OldDevices;
	HidDevices[NumOfDevices-1] = this;
}

HidDevice::~HidDevice()
{
	TerminateThread = true;
	if(cHidDevice)
	{
		CloseHidDevice(cHidDevice);
	}

	bool found = false;
	for(int i = 0; i < NumOfDevices; i++)
	{
		if(HidDevices[i] == this)
			found = true;
		if(found)
			HidDevices[i] = HidDevices[i+1];
	}
	HidDevice **OldDevices = HidDevices;
	HidDevices = new HidDevice*[--NumOfDevices];
	for(int i = 0; i < NumOfDevices; i++)
		HidDevices[i] = OldDevices[i];
	if(OldDevices)
		delete[] OldDevices;
}

void HidDevice::SetOwner(void *NewOwner)
{
	Owner = NewOwner;
}

void HidDevice::WriteToDevice(byte *Message, int size)
{
}

bool HidDevice::Connect()
{
	PHID_DEVICE HidDevices;
	unsigned long NumOfDevices = 0;
	GUID hidGuid;
	static const char* class_name = "Message";
	static HDEVNOTIFY diNotifyHandle;
	WNDCLASSEX wx = {};

	HidD_GetHidGuid(&hidGuid);

	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = WndProc;
	wx.hInstance = NULL;
	wx.lpszClassName = class_name;
	if(RegisterClassEx(&wx))
		MyMessageHWND = CreateWindowEx(0, class_name, "Message", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	else if(GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
		MyMessageHWND = CreateWindowEx(0, class_name, "Message", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	else
	{
		DWORD Error = GetLastError();
		return false;
	}
	
	if(MyMessageHWND == INVALID_HANDLE_VALUE)
		return false;

	PHID_DEVICE CurrentDevice;

	FindKnownHidDevices(&HidDevices, &NumOfDevices);
	CurrentDevice = HidDevices;

	for(int i = 0; i < NumOfDevices; i++, CurrentDevice++)
	{
		unsigned int cVID = 0;
		unsigned int cPID = 0;
		char *Buffer = new char[strlen(CurrentDevice->DevicePath)+1];
		sscanf(CurrentDevice->DevicePath, "\\\\?\\hid#vid_%4X&pid_%4X&%s", &cVID, &cPID, Buffer);
		delete[] Buffer;
		if(CurrentDevice->Caps.Usage == Usage && CurrentDevice->Caps.UsagePage == UsagePage && cVID == VID && cPID == PID)
		{
			cHidDevice = new HID_DEVICE;
			OpenHidDevice(CurrentDevice->DevicePath, true, true, true, false, cHidDevice);
			break;
		}
	}
	if(!cHidDevice)
		return false;
	Connected = true;

	DEV_BROADCAST_DEVICEINTERFACE broadcastInterface;

	broadcastInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	broadcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	broadcastInterface.dbcc_classguid = hidGuid;

	diNotifyHandle = RegisterDeviceNotification(MyMessageHWND, &broadcastInterface, DEVICE_NOTIFY_WINDOW_HANDLE);
	if(diNotifyHandle == NULL)
	{
		DWORD Error = GetLastError();
		Error = Error;
	}

	free(HidDevices);

	DWORD threadID;
	HANDLE readThread;

	readThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AsynchReadThreadProc, this, 0, &threadID);
	return true;
}

#ifndef CheckKeyPressed
#define CheckKeyPressed(Data, DataIndex, Value, KeysPressed, KeysPressedIndex, GKEY, PressedKeys, ReleasedKeys) \
	if(Data[DataIndex] & Value && !KeysPressed[KeysPressedIndex])\
	{\
		PressedKeys |= GKEY;\
		KeysPressed[KeysPressedIndex] = true;\
	}\
	else if(!(Data[DataIndex] & Value) && KeysPressed[KeysPressedIndex])\
	{\
		ReleasedKeys |= GKEY;\
		KeysPressed[KeysPressedIndex] = false;\
	}
#endif

DWORD WINAPI HidDevice::AsynchReadThreadProc(HidDevice *CurrentDevice)
{
	HANDLE  completionEvent;
	HANDLE  WriteCompletionEvent;
	BOOLEAN    readStatus;
	BOOLEAN    writeStatus;
	DWORD   waitStatus;
	completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	WriteCompletionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);


	if (NULL == completionEvent || NULL == WriteCompletionEvent) 
	{
		goto AsyncRead_End;
	}

	int retVal;
	MSG msg;
	do 
	{
		readStatus = ReadOverlapped(CurrentDevice->cHidDevice, completionEvent );
		//writeStatus = WriteOverlapped(CurrentDevice->cHidDevice, WriteCompletionEvent );
		DWORD Error = GetLastError();

		if (!readStatus) 
			break;


		while (!CurrentDevice->TerminateThread) 
		{
			waitStatus = WaitForSingleObject (completionEvent, CurrentDevice->ReadThreadTimeOut);

			if ( WAIT_OBJECT_0 == waitStatus)
				break;
		}
		if(CurrentDevice->TerminateThread)
			break;

		unsigned char *Data = (unsigned char*)CurrentDevice->cHidDevice->InputReportBuffer;
		if(Data)
		{
			char *Buffer = new char[(CurrentDevice->cHidDevice->Caps.InputReportByteLength*3)+4];
			memset(Buffer, 0, (CurrentDevice->cHidDevice->Caps.InputReportByteLength*3)+4);
			sprintf(Buffer, "%d>", CurrentDevice->cHidDevice->Caps.InputReportByteLength);
			for(int i = 0; i < CurrentDevice->cHidDevice->Caps.InputReportByteLength; i++)
				sprintf(Buffer,"%s%2X ", Buffer, Data[i]);
			sprintf(Buffer,"%s\n", Buffer);
			//printf(Buffer);
			OutputDebugStringA(Buffer);
			delete[] Buffer;
			CurrentDevice->ReceivedInput();
		}
	} while ( !CurrentDevice->TerminateThread);

AsyncRead_End:
	ExitThread(0);
	return (0);
}

LRESULT CALLBACK HidDevice::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_DEVICECHANGE:
		PDEV_BROADCAST_DEVICEINTERFACE b = (PDEV_BROADCAST_DEVICEINTERFACE) lParam;
		TCHAR strBuff[256];

		switch(wParam)
		{
		case DBT_DEVICEARRIVAL:
			//printf("Message DBT_DEVICEARRIVAL %s\n", b->dbcc_name);
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			//printf("Message DBT_DEVICEREMOVECOMPLETE %s\n", b->dbcc_name);
			break;
		case DBT_DEVNODES_CHANGED:
			//printf("Message DBT_DEVNODES_CHANGED %s\n", b->dbcc_name);
			break;
		}
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HidDevice *GetHidDeviceFromHWND(HWND Source)
{
	HidDevice *Return = NULL;

	for(int i= 0; i < NumOfDevices; i++)
		if(Source == HidDevices[i]->MyMessageHWND)
		{
			Return = HidDevices[i];
			break;
		}

	return Return;
}

XMLHidDevice::XMLHidDevice()
{
	OnShiftStateChanged = NULL;
	OnAxisChanged = NULL;
	Initizialed = false;
	TimeToReach = 0;
}

XMLHidDevice::~XMLHidDevice()
{
	TerminateThread = true;
	CurrentNode = NULL;
	//HidDevice::~HidDevice();
}

void XMLHidDevice::LoadXML(char *FileName)
{
	XmlFile.LoadXMLFile(FileName);
}

string XMLHidDevice::GetName()
{
	string Return = "No name";

	if(CurrentNode)
		Return = CurrentNode->GetAttributeValue("Name");

	return Return;
}

std::vector<string> XMLHidDevice::GetNames()
{
	std::vector<string> Return;

	std::vector<XMLNode *> Gamepads = XmlFile.GetXMLNodes("Gamepad");
	for(int i = 0; i < Gamepads.size(); i++)
		Return.push_back(Gamepads[i]->GetAttributeValue("Name"));

	return Return;
}

void XMLHidDevice::UseDevice(string Name)
{
	XMLNode * Gamepad = XmlFile.GetXMLNode("Gamepad" , "Name", Name);
	if(Gamepad)
	{
		CurrentNode = Gamepad;
		XMLNode *xnConfig = Gamepad->GetXMLNode("Config");
		if(xnConfig)
		{
			XMLNode *xnVID = xnConfig->GetXMLNode("VID");
			XMLNode *xnPID = xnConfig->GetXMLNode("PID");
			XMLNode *xnUsage = xnConfig->GetXMLNode("Usage");
			XMLNode *xnUsagePage = xnConfig->GetXMLNode("UsagePage");
			if(xnVID)
				VID = ::_httoi(xnVID->Value.c_str());
			if(xnPID)
				PID = _httoi(xnPID->Value.c_str());
			if(xnUsage)
				Usage = _httoi(xnUsage->Value.c_str());
			if(xnUsagePage)
				UsagePage = _httoi(xnUsagePage->Value.c_str());
		}
		XMLNode *xnInput = Gamepad->GetXMLNode("Input");
		if(xnInput)
		{
			std::vector<XMLNode *> ByteList = xnInput->GetXMLNodes("Byte");
			for(int j = 0; j < ByteList.size(); j++)
			{
				XMLAttribute *xaUnused = ByteList[j]->GetAttribute("Unused");
				if(xaUnused && ToLowerString(xaUnused->Value) == "false")
				{
					XMLAttribute *xaType = ByteList[j]->GetAttribute("Type");
					if(xaType)
					{
						if(ToLowerString(xaType->Value) == "axis")
						{
							AxisValues.push_back(0x00);
							Tokens[ByteList[j]->Value] = (ULLONG)1 << (Tokens.size());
							ExtraKeyNames[Tokens[ByteList[j]->Value]] = ByteList[j]->Value;
						}
						else if(ToLowerString(xaType->Value) == "buttonmask")
						{
							std::vector<XMLNode *> xnMasks = ByteList[j]->GetXMLNodes("Mask");
							for(int k = 0; k < xnMasks.size(); k++)
							{
								ButtonsPressed.push_back(false);
								Tokens[xnMasks[k]->Value] = (ULLONG)1 << (Tokens.size());
								ExtraKeyNames[Tokens[xnMasks[k]->Value]] = xnMasks[k]->Value;
							}
						}							
					}
				}
			}
		}
		XMLNode *xnShiftStates = Gamepad->GetXMLNode("ShiftStates");
		ShiftStates.clear();
		if(xnShiftStates)
		{
			std::vector<XMLNode *> xnStates = xnShiftStates->GetXMLNodes("State");
			ShiftState newState;
			Axis NewAxis;
			std::vector<XMLNode *> Axises;
			for(int j = 0; j < xnStates.size(); j++)
			{
				newState.Axises.clear();
				newState.Button = "";
				newState.Delay = 0;
				newState.Name = xnStates[j]->GetAttributeValue("Name");
				string strType = ToLowerString(xnStates[j]->GetAttributeValue("Type"));
				if(strType == "axis")
					newState.Type = SS_AXIS;
				else if(strType == "multiple axis")
					newState.Type = SS_MULTIPLE_AXIS;
				else if(strType == "button")
					newState.Type = SS_BUTTON;
				else
					newState.Type = SS_NONE;
				if(xnStates[j]->GetAttribute("Delay"))
				{
					newState.Delay = atoi(xnStates[j]->GetAttributeValue("Delay").c_str());
				}
				switch(newState.Type)
				{
				case SS_AXIS:
					NewAxis.Name = xnStates[j]->GetAttributeValue("AxisName");
					NewAxis.MinValue = _httoi(xnStates[j]->GetAttributeValue("MinValue").c_str());
					NewAxis.MaxValue = _httoi(xnStates[j]->GetAttributeValue("MaxValue").c_str());
					newState.Axises.push_back(NewAxis);
					break;
				case SS_MULTIPLE_AXIS:
					Axises = xnStates[j]->GetXMLNodes("Axis");
					for(int k = 0; k < Axises.size(); k++)
					{
						NewAxis.Name = Axises[k]->GetAttributeValue("Name");
						NewAxis.MinValue = _httoi(Axises[k]->GetAttributeValue("MinValue").c_str());
						NewAxis.MaxValue = _httoi(Axises[k]->GetAttributeValue("MaxValue").c_str());
						newState.Axises.push_back(NewAxis);
					}
					break;
				case SS_BUTTON:
					newState.Button = xnStates[j]->GetAttributeValue("ButtonName");
					break;
				}
				ShiftStates.push_back(newState);
			}
		}
		XMLNode *xnAltNames = CurrentNode->GetXMLNode("AltNames");
		if(xnAltNames)
		{
			std::vector<XMLNode *> xnAltNames2 = xnAltNames->GetXMLNodes("AltName");
			XMLNode* CurrentNode1;
			XMLAttribute* CurrentAttribute;
			for(ULLONG j = 0; j < xnAltNames2.size(); j++)
			{
				CurrentNode1 = xnAltNames2[j];
				CurrentAttribute = CurrentNode->GetAttribute("Name");
				AltNames[xnAltNames2[j]->GetAttributeValue("Name")] = xnAltNames2[j]->Value;
			}
		}
		if(!Connect())
		{
			std::vector<XMLNode *> AltPIDs = xnConfig->GetXMLNodes("AltPID");
			for(int j = 0; j < AltPIDs.size(); j++)
			{
				PID = _httoi(AltPIDs[j]->Value.c_str());
				if(Connect())
					break;
			}
		}
		InitShiftState();
		Initizialed = false;
	}
}

void XMLHidDevice::ReceivedInput()
{
	if(TerminateThread || CurrentNode == NULL)
		return;
	XMLNode *UseMask = CurrentNode->GetXMLNode("Byte", "Type", "UseMask");
	std::vector<XMLNode *> ByteList = CurrentNode->GetXMLNode("Input", "", "")->GetXMLNodes("Byte", "", "");
	int Index;
	for(Index = 0; Index < ByteList.size(); Index++)
		if(UseMask == ByteList[Index])
			break;
	if(Index < ByteList.size())
		if(((unsigned char*)cHidDevice->InputReportBuffer)[Index] != (unsigned char)_httoi(UseMask->GetAttributeValue("Value").c_str()))
			return;
	unsigned long long PressedKeys = 0;
	unsigned long long ReleasedKeys = 0;
	
	GetKeysChanged((unsigned char*)cHidDevice->InputReportBuffer, PressedKeys, ReleasedKeys);

	if(OnExtraKeysPressed && PressedKeys)
		(*OnExtraKeysPressed)(Owner, this, PressedKeys);
	if(OnExtraKeysReleased && ReleasedKeys)
		(*OnExtraKeysReleased)(Owner, this, ReleasedKeys);
}

void XMLHidDevice::GetKeysChanged(unsigned char *Data, ULLONG &Pressed, ULLONG &Released)
{
	XMLNode *xnInput = CurrentNode->GetXMLNode("Input");
	std::vector<XMLNode *> ByteList = xnInput->GetXMLNodes("Byte");
	unsigned int AxisIndex = 0;
	unsigned int ButtonIndex = 0;
	char DebugOutput[2048];
	for(int i = 0; i < ByteList.size(); i++)
	{
		sprintf(DebugOutput, "Byte %d equals %2X\n", i, Data[i]);
		//OutputDebugStringA(DebugOutput);
		if(ToLowerString(ByteList[i]->GetAttributeValue("Unused")) == "true")
		{
			//OutputDebugStringA("Byte is unused\n");
			continue;
		}
		if(ToLowerString(ByteList[i]->GetAttributeValue("Type")) == "axis")
		{
			sprintf(DebugOutput, "Byte %d is an axis\n", i);
			//OutputDebugStringA(DebugOutput);
			if(Data[i] != AxisValues[AxisIndex])
			{
				sprintf(DebugOutput, "Byte changed.\n");
				//OutputDebugStringA(DebugOutput);
				AxisValues[AxisIndex] = Data[i];
				CheckShiftStateChanged(Tokens[ByteList[i]->Value]);
				sprintf(DebugOutput, "%s\n", Initizialed ? "Initizialed" : "Not Initizialed");
				if(Initizialed)
				{
					if(OnAxisChanged)
						(*OnAxisChanged)(Owner, this, Tokens[ByteList[i]->Value], Data[i]);
				}
			}
			else
			{
				sprintf(DebugOutput, "Byte stayed the same.\n");
				//OutputDebugStringA(DebugOutput);
			}
			AxisIndex++;
		}
		else if(ToLowerString(ByteList[i]->GetAttributeValue("Type")) == "buttonmask")
		{
			sprintf(DebugOutput, "Byte %d is an buttonmask\n", i);
			//OutputDebugStringA(DebugOutput);
			std::vector<XMLNode *> xnMasks = ByteList[i]->GetXMLNodes("Mask");
			for(int j = 0; j < xnMasks.size(); j++)
			{
				 CheckKeyPressed(Data, i, _httoi(xnMasks[j]->GetAttributeValue("Value").c_str()), ButtonsPressed, ButtonIndex, Tokens[xnMasks[j]->Value], Pressed, Released); 
				 ButtonIndex++;
				 CheckShiftStateChanged(Tokens[xnMasks[j]->Value]);
			}
		}
	}
	if(!Initizialed)
		Initizialed = true;
}

void XMLHidDevice::CheckShiftStateChanged(ULLONG KeyChanged)
{
	char DebugOutput[2048];
	time_t temp = clock();
	sprintf(DebugOutput, "In CheckShiftStateChanged with Key pressed %s\n", ExtraKeyNames[KeyChanged].c_str());
	//OutputDebugStringA(DebugOutput);
	sprintf(DebugOutput, "%d < %d TimeToReach\n", temp, TimeToReach);
	//OutputDebugStringA(DebugOutput);
	if(temp < TimeToReach)
		return;
	bool Changed = false;
	sprintf(DebugOutput, "There are %d shiftstates. \n", ShiftStates.size());
	//OutputDebugStringA(DebugOutput);
	for(int i = 0; i < ShiftStates.size(); i++)
	{
		if(CurrentShiftState == ShiftStates[i].Name)
		{
			//OutputDebugStringA("The Shift State we are checking is the same as the current Shift State. So skipping\n");
			continue;
		}
		switch(ShiftStates[i].Type)
		{
		case SS_AXIS:
			//OutputDebugStringA("The shift state is an axis triggered state.\n");
			if(ShiftStates[i].Axises[0].Name == ExtraKeyNames[KeyChanged])
			{
				Changed = CheckShiftState(ShiftStates[i]);
			}
			else
			{
				sprintf(DebugOutput, "%s doesn't equal %s\n", ShiftStates[i].Axises[0].Name.c_str(), ExtraKeyNames[KeyChanged].c_str());
				//OutputDebugStringA(DebugOutput);
			}
			break;
		case SS_MULTIPLE_AXIS:
			//OutputDebugStringA("The shift state is a multiple axis triggered state.\n");
			for(int j = 0; j < ShiftStates[i].Axises.size(); j++)
			{
				if(ShiftStates[i].Axises[j].Name == ExtraKeyNames[KeyChanged])
				{
					Changed = CheckShiftState(ShiftStates[i]);
					break;
				}
				else
				{
					sprintf(DebugOutput, "%s doesn't equal %s\n", ShiftStates[i].Axises[0].Name.c_str(), ExtraKeyNames[KeyChanged].c_str());
					//OutputDebugStringA(DebugOutput);
				}
			}
			break;
		case  SS_BUTTON:
			//OutputDebugStringA("The shift state is a button triggered state.\n");
			if(ShiftStates[i].Button == ExtraKeyNames[KeyChanged])
				Changed = CheckShiftState(ShiftStates[i]);
			else
			{
				sprintf(DebugOutput, "%s doesn't equal %s\n", ShiftStates[i].Button.c_str(), ExtraKeyNames[KeyChanged].c_str());
				//OutputDebugStringA(DebugOutput);
			}
			break;
		}
		if(Changed)
		{
			sprintf(DebugOutput, "Changing shift state from %s to %s", CurrentShiftState.c_str(), ShiftStates[i].Name.c_str());
			//OutputDebugStringA(DebugOutput);
			TimeToReach = clock() + ShiftStates[i].Delay;
			CurrentShiftState = ShiftStates[i].Name;
			if(OnShiftStateChanged)
				(*OnShiftStateChanged)(Owner, this, CurrentShiftState);
			break;
		}
	}
}

bool XMLHidDevice::CheckShiftState(const ShiftState &State)
{
	std::vector<XMLNode *> ByteList = CurrentNode->GetXMLNode("Input")->GetXMLNodes("Byte", "Unused", "False");
	std::vector<unsigned int> AxisIndices;
	unsigned int AxisIndex = 0;
	unsigned int ButtonIndex = 0;
	for(int i = 0; i < ByteList.size(); i++)
	{
		if(ToLowerString(ByteList[i]->GetAttributeValue("Type")) == "axis")
		{
			if(State.Type == SS_AXIS || State.Type == SS_MULTIPLE_AXIS)
			{
				for(int j = 0; j < State.Axises.size(); j++)
					if(ByteList[i]->Value == State.Axises[j].Name)
						AxisIndices.push_back(AxisIndex);
			}
			AxisIndex++;
		}
		else if(ToLowerString(ByteList[i]->GetAttributeValue("Type")) == "buttonmask")
		{
			bool Found = false;
			for(int j = 0; j < ByteList[i]->Children.size(); j++)
			{
				if(State.Type == SS_BUTTON && ByteList[i]->Children[j]->Value == State.Button)
				{
					Found = true;
					break;
				}
				ButtonIndex++;
			}
			if(Found)
				break;
		}
	}
	switch(State.Type)
	{
	case SS_AXIS:
	case SS_MULTIPLE_AXIS:
		for(int j = 0; j < State.Axises.size(); j++)
		{
			if(State.Axises[j].MinValue > AxisValues[AxisIndices[j]] || AxisValues[AxisIndices[j]] > State.Axises[j].MaxValue)
				return false;
		}
		return true;
		break;
	case SS_BUTTON:
		if(ButtonsPressed.size())
			return ButtonsPressed[ButtonIndex];
		break;
	}
	return false;
}

void XMLHidDevice::InitShiftState()
{
	for(int i = 0; i < ShiftStates.size(); i++)
	{
		if(CheckShiftState(ShiftStates[i]))
		{
			CurrentShiftState = ShiftStates[i].Name;
			break;
		}
	}
	if(CurrentShiftState == "" && ShiftStates.size())
		CurrentShiftState = ShiftStates[0].Name;
}