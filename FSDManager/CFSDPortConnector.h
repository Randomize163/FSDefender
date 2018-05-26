#pragma once

#include <windows.h>
#include <fltuser.h>

#define MAX_RECIEVE_BUFFER_SIZE 256

struct CFSDPortConnectorMessage
{
	union
	{
		FILTER_MESSAGE_HEADER	aRecieveHeader;
		FILTER_REPLY_HEADER     aReplyHeader;
	};

	BYTE pBuffer[MAX_RECIEVE_BUFFER_SIZE];
};

static_assert(sizeof(CFSDPortConnectorMessage) == sizeof(FILTER_MESSAGE_HEADER) + MAX_RECIEVE_BUFFER_SIZE, "Invalid size");
static_assert(sizeof(CFSDPortConnectorMessage) == sizeof(FILTER_REPLY_HEADER) + MAX_RECIEVE_BUFFER_SIZE, "Invalid size");

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

