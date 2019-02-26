#pragma once
#include <Windows.h>
extern "C"
{
	#include <api/hidsdi.h>
	#include <SetupAPI.h>
	#include <api/dbt.h>
	#include "hid.h"
}

#include "XML.h"
#include <vector>
#include <map>
#include <time.h>
#include <string>

typedef unsigned long long ULLONG;

typedef void (*fpOnExtraKeys)(void *, void *, ULLONG);
typedef void (*fpOnShiftStateChanged)(void *, void *, string);
typedef void (*fpOnAxisChanged)(void *, void *, ULLONG, unsigned char);

class HidDevice
{
public:
	HidDevice();
	~HidDevice();
	void SetOwner(void *NewOwner);
	void (*OnExtraKeysPressed)(void *Owner, void *Sender, ULLONG xKeys);
	void (*OnExtraKeysReleased)(void *Owner, void *Sender, ULLONG xKeys);
	void (*OnAxisChanged)(void *, void *Sender, ULLONG Axis, unsigned char Value);
	friend HidDevice *GetHidDeviceFromHWND(HWND Source);
	bool Connected;
	void WriteToDevice(byte *Message, int size);
protected:
	HWND MyMessageHWND;
	PHID_DEVICE cHidDevice;
	bool TerminateThread;
	unsigned long ReadThreadTimeOut;
	unsigned short VID;
	unsigned short PID;
	unsigned short Usage;
	unsigned short UsagePage;
	void *Owner;

	static DWORD WINAPI AsynchReadThreadProc(HidDevice *CurrentDevice);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool Connect();	
	virtual void GetKeysChanged(unsigned char *Data, ULLONG &Pressed, ULLONG &Released) = 0;
	virtual void ReceivedInput() = 0;
};

enum ShiftStateType { SS_NONE = 0, SS_AXIS, SS_MULTIPLE_AXIS, SS_BUTTON };

struct Axis
{
	string Name;
	unsigned char MinValue;
	unsigned char MaxValue;
};

struct ShiftState
{
	string Name;
	ShiftStateType Type;
	string Button;
	std::vector<Axis> Axises;
	unsigned int Delay;
};

class XMLHidDevice: public HidDevice
{
public:
	XMLHidDevice();
	~XMLHidDevice();
	void LoadXML(char *FileName);
	std::map<ULLONG, string> ExtraKeyNames;
	std::map<string, string> AltNames;
	string GetName();
	std::vector<string> GetNames();
	void UseDevice(string Name);
	void (*OnShiftStateChanged)(void *Owner, void *Sender, string State);
	string CurrentShiftState;
	std::vector<ShiftState> ShiftStates;
private:
	void ReceivedInput();
	void GetKeysChanged(unsigned char *Data, ULLONG &Pressed, ULLONG &Released);
	void CheckShiftStateChanged(ULLONG KeyChanged);
	bool CheckShiftState(const ShiftState &State);
	void InitShiftState();
	XML XmlFile;
	XMLNode *CurrentNode;
	bool Initizialed;
	std::vector<bool> ButtonsPressed;
	std::vector<unsigned char> AxisValues;
	std::map<string, ULLONG> Tokens;
	clock_t TimeToReach;
};