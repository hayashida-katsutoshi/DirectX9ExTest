#include "DebugMessageHandler.h"
#include <iostream>
#include <windows.h>

#define USE_DEBUG_MESSAGE
#define SERIAL_OUTPUT
#define COM_PORT_NUMBER 7

static int gVerboseLevel = 1;

#ifdef SERIAL_OUTPUT
//Debug Serial Port Class Definition
class CSerialPort {
public:
	//Default Constructor
	CSerialPort() : mHandle(INVALID_HANDLE_VALUE)
	{
		SetupPort();
	}
	//Default Destructor
	~CSerialPort()
	{
		if( mHandle!=INVALID_HANDLE_VALUE )
		{
			CloseHandle( mHandle);
		}
	}

	//Function to output the Debug Messages to the Port
	BOOL Write( LPCVOID lpBuffer,
				DWORD nNumberOfBytesToWrite,
				LPDWORD lpNumberOfBytesWritten)
	{
		SetupPort();		// Patch

		if( mHandle!=INVALID_HANDLE_VALUE )
		{
			return WriteFile( mHandle, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL);
		}
		
		return FALSE;
	}

private:
	HANDLE mHandle;

	void SetupPort()
	{
		if( mHandle == INVALID_HANDLE_VALUE )
		{
			mHandle = OpenDebugPort(COM_PORT_NUMBER);

			if(mHandle != INVALID_HANDLE_VALUE)
			{
				//Setting Debug Port as 115200,8,N,1
				ConfigureDebugPort(mHandle,115200,8,NOPARITY,ONESTOPBIT);
			}
		}
	}

	//Function to Open the Debug Port
	HANDLE OpenDebugPort(ULONG ulPortNumber)
	{
		HANDLE hPortHandle = INVALID_HANDLE_VALUE;
		ULONG ulErrorCode = 0;
		WCHAR wcPortName[MAX_PATH];
		memset(wcPortName,0x00,sizeof(wcPortName));

		swprintf(wcPortName,MAX_PATH,L"%s%d",L"\\\\.\\COM",ulPortNumber);

		hPortHandle = CreateFile(wcPortName,
								GENERIC_READ | GENERIC_WRITE,
								0,		// share mode none
								NULL,	// no security
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL );		// no template

		return hPortHandle;
	}
	//Function which configures the Debug Port
	BOOL ConfigureDebugPort(HANDLE hPortHandle,
							DWORD Baud,
							BYTE Size,
							BYTE Parity,
							BYTE Stop)
	{
		DCB DeviceControlBlock;
		ZeroMemory(&DeviceControlBlock, sizeof(DCB));

		DeviceControlBlock.DCBlength = sizeof(DCB);
		DeviceControlBlock.BaudRate	 = Baud;
		DeviceControlBlock.ByteSize	 = Size;
		DeviceControlBlock.Parity	 = Parity;
		DeviceControlBlock.StopBits	 = Stop;

		return SetCommState( hPortHandle, &DeviceControlBlock);
	}
};

static CSerialPort DebugPort;
#endif

//////////////////////////////////////////////////////////////////////////////
//! @brief			Constructor
//////////////////////////////////////////////////////////////////////////////
DebugMessageHandler::DebugMessageHandler( int level)
: mMyVerboseLevel( level)
{
}

//////////////////////////////////////////////////////////////////////////////
//! @brief			Destructor
//////////////////////////////////////////////////////////////////////////////
DebugMessageHandler::~DebugMessageHandler()
{
	try
	{
#ifdef USE_DEBUG_MESSAGE
		if( mMyVerboseLevel<gVerboseLevel )		// Lower level is higher priority.
		{
			*this << "\n";
#ifdef SERIAL_OUTPUT
			DWORD dummy;
#pragma warning (push)
#pragma warning (disable:4267)
			DebugPort.Write(str().c_str(), str().length(), &dummy);
#pragma warning (pop)
#else
			OutputDebugStringA(str().c_str());
#endif
		}
#endif
	}
	catch( ... )
	{
	}
}

void DebugMessageHandler::SetVerboseLevel( int level)
{
	gVerboseLevel = level;
}

