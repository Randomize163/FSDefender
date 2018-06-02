#include "CFSDCommunicationPort.h"

#include <fltKernel.h>
#include "FSDCommonInclude.h"
#include <stdio.h>


CFSDCommunicationPort::CFSDCommunicationPort()
    : m_fClosed(false)
    , m_pPort(NULL)
    , m_pFilter(NULL)
    , m_pClientPort(NULL)
{
}
NTSTATUS CFSDCommunicationPort::Initialize(
    LPCWSTR                     wszName, 
    PFLT_FILTER                 pFilter, 
    PVOID                     pvContext,
    FSD_CONNECT_CALLBACK     pfnOnConnect, 
    FSD_DISCONNECT_CALLBACK  pfnOnDisconnect, 
    FSD_NEW_MESSAGE_CALLBACK pfnOnNewMessage
){
    NTSTATUS hr = S_OK;

    m_pFilter         = pFilter;
    m_pvContext          = pvContext;
    m_pfnOnConnect      = pfnOnConnect;
    m_pfnOnDisconnect = pfnOnDisconnect;
    m_pfnOnNewMessage = pfnOnNewMessage;

    UNICODE_STRING ustrName;
    RtlInitUnicodeString(&ustrName, wszName);

    PSECURITY_DESCRIPTOR sd;

    hr = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
    if (NT_SUCCESS(hr)) 
    {
        OBJECT_ATTRIBUTES oa;
        InitializeObjectAttributes(&oa,
            &ustrName,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            sd);

        hr = FltCreateCommunicationPort(pFilter,
            &m_pPort,
            &oa,
            (PVOID)this,
            ConnectCallback,
            DisconnectCallback,
            NewMessageCallback,
            1);
        //
        //  Free the security descriptor in all cases. It is not needed once
        //  the call to FltCreateCommunicationPort() is made.
        //

        FltFreeSecurityDescriptor(sd);
    }

    return hr;
}

void CFSDCommunicationPort::Close()
{
    ASSERT(!m_fClosed);

    if (m_pClientPort)
    {
        OnDisconnect();
    }

    if (m_pPort)
    {
        FltCloseCommunicationPort(m_pPort);
    }

    m_fClosed = true;
}

NTSTATUS CFSDCommunicationPort::OnConnect(IN PFLT_PORT ClientPort, IN PVOID ServerPortCookie, IN PVOID ConnectionContext, IN ULONG SizeOfContext, OUT PVOID * ConnectionPortCookie)
{
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);

    ASSERT(m_pClientPort == NULL);
    m_pClientPort = ClientPort;

    *ConnectionPortCookie = ServerPortCookie;

    if (m_pfnOnConnect)
    {
        return m_pfnOnConnect(m_pvContext, ClientPort);
    }

    return S_OK;
}

void CFSDCommunicationPort::OnDisconnect()
{
    FltCloseClientPort(m_pFilter, &m_pClientPort);

    if (m_pfnOnDisconnect)
    {
        m_pfnOnDisconnect(m_pvContext, m_pClientPort);
    }

    m_pClientPort = NULL;
}

NTSTATUS CFSDCommunicationPort::OnNewMessage(
    IN PVOID InputBuffer OPTIONAL, 
    IN ULONG InputBufferLength, 
    OUT PVOID OutputBuffer OPTIONAL, 
    IN ULONG OutputBufferLength, 
    OUT PULONG ReturnOutputBufferLength
){
    if (m_pfnOnNewMessage)
    {
        return m_pfnOnNewMessage(m_pvContext, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnOutputBufferLength);
    }

    return S_OK;
}

NTSTATUS CFSDCommunicationPort::ConnectCallback(
    IN PFLT_PORT ClientPort, 
    IN PVOID ServerPortCookie, 
    IN PVOID ConnectionContext, 
    IN ULONG SizeOfContext, 
    OUT PVOID * ConnectionPortCookie
){
    NTSTATUS hr = S_OK;

    CFSDCommunicationPort* pPort = static_cast<CFSDCommunicationPort*>(ServerPortCookie);
    hr = pPort->OnConnect(ClientPort, ServerPortCookie, ConnectionContext, SizeOfContext, ConnectionPortCookie);
    RETURN_IF_FAILED(hr);

    return S_OK;
}

void CFSDCommunicationPort::DisconnectCallback(IN PVOID ConnectionCookie)
{
    CFSDCommunicationPort* pPort = static_cast<CFSDCommunicationPort*>(ConnectionCookie);
    pPort->OnDisconnect();
}

NTSTATUS CFSDCommunicationPort::NewMessageCallback(IN PVOID PortCookie, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PULONG ReturnOutputBufferLength)
{
    NTSTATUS hr = S_OK;

    CFSDCommunicationPort* pPort = static_cast<CFSDCommunicationPort*>(PortCookie);
    hr = pPort->OnNewMessage(InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnOutputBufferLength);
    RETURN_IF_FAILED(hr);

    return S_OK;
}

NTSTATUS CFSDCommunicationPort::SendMessage(PVOID pvSendBuffer, ULONG uSendBufferSize, PVOID pvReplyBuffer, PULONG puReplyBufferSize, ULONG uTimeout)
{
    NTSTATUS hr = S_OK;

    LARGE_INTEGER aTimeout;
    aTimeout.QuadPart = uTimeout;

    ULONG uReplyBufferSize = puReplyBufferSize ? *puReplyBufferSize : 0;
    hr = FltSendMessage(m_pFilter, &m_pClientPort, pvSendBuffer, uSendBufferSize, pvReplyBuffer, (puReplyBufferSize ? &uReplyBufferSize : NULL) , &aTimeout);
    if (hr == STATUS_TIMEOUT)
    {
        return hr;
    }
    RETURN_IF_FAILED(hr);

    if (puReplyBufferSize)
    {
        ASSERT(uReplyBufferSize > *puReplyBufferSize);
        *puReplyBufferSize = uReplyBufferSize;
    }

    return S_OK;
}

CFSDCommunicationPort::~CFSDCommunicationPort()
{
    if (!m_fClosed)
    {
        Close();
    }
}