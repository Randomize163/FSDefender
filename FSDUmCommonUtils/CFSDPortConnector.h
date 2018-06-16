#pragma once

#include <windows.h>
#include <fltuser.h>

#define MAX_RECIEVE_BUFFER_SIZE 1024

struct CFSDPortConnectorMessage
{
    CFSDPortConnectorMessage()
    {
        memset(this, 0, sizeof(*this));
    }

    static CFSDPortConnectorMessage* CastFrom(LPOVERLAPPED pOverlapped)
    {
        return CONTAINING_RECORD(pOverlapped, CFSDPortConnectorMessage, aOverlapped);
    }

    union
    {
        FILTER_MESSAGE_HEADER aRecieveHeader;
        FILTER_REPLY_HEADER   aReplyHeader;
    };

    BYTE pBuffer[MAX_RECIEVE_BUFFER_SIZE];

    OVERLAPPED  aOverlapped;
};

class CFSDPortConnector
{
public:
    CFSDPortConnector();

    ~CFSDPortConnector();

    HRESULT Initialize(LPCWSTR wszPortName);

    void Close();

    HRESULT SendMessage(
        LPVOID    lpInBuffer,
        DWORD    dwInBufferSize,
        LPVOID  lpReplyBuffer,
        LPDWORD lpReplyBufferSize
    );

    HRESULT RecieveMessage(
        CFSDPortConnectorMessage* pMessage
    );

    HRESULT ReplyMessage(
        CFSDPortConnectorMessage* pMessage
    );

    HANDLE GetHandle() const
    {
        return m_hPort;
    }

private:
    HANDLE m_hPort;
};

