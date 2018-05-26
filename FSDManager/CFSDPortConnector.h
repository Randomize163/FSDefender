#pragma once

#include <windows.h>
#include <fltuser.h>

struct CFSDPortConnectorMessage
{
	union
	{
		FILTER_MESSAGE_HEADER	aRecieveHeader;
		FILTER_REPLY_HEADER     aReplyHeader;
	};

	BYTE pBuffer[256];
};

static_assert(sizeof(CFSDPortConnectorMessage) == sizeof(FILTER_MESSAGE_HEADER) + 256, "Invalid size");
static_assert(sizeof(CFSDPortConnectorMessage) == sizeof(FILTER_REPLY_HEADER) + 256, "Invalid size");

class CFSDPortConnector
{
public:
	CFSDPortConnector();

	~CFSDPortConnector();

	HRESULT Initialize(LPCWSTR wszPortName);

	void Close();

	HRESULT SendMessage(
		LPVOID	lpInBuffer,
		DWORD	dwInBufferSize,
		LPVOID  lpReplyBuffer,
		LPDWORD lpReplyBufferSize
	);

	HRESULT RecieveMessage(
		CFSDPortConnectorMessage* pMessage
	);

	HRESULT ReplyMessage(
		CFSDPortConnectorMessage* pMessage
	);
	
private:
	HANDLE m_hPort;
};

