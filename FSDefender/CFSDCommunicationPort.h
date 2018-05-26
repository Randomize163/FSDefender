#pragma once
#include <fltKernel.h>
#include "FSDUtils.h"

class CFSDCommunicationPort
{
public:
	CFSDCommunicationPort();

	NTSTATUS Initialize(LPCWSTR wszName, PFLT_FILTER pFilter);

	void Close();

	NTSTATUS OnConnect(
		IN PFLT_PORT ClientPort,
		IN PVOID ServerPortCookie,
		IN PVOID ConnectionContext,
		IN ULONG SizeOfContext,
		OUT PVOID *ConnectionPortCookie
	);

	void OnDisconnect();

	NTSTATUS OnNewMessage(
		IN PVOID InputBuffer OPTIONAL,
		IN ULONG InputBufferLength,
		OUT PVOID OutputBuffer OPTIONAL,
		IN ULONG OutputBufferLength,
		OUT PULONG ReturnOutputBufferLength
	);

	static NTSTATUS ConnectCallback(
		IN PFLT_PORT ClientPort, 
		IN PVOID ServerPortCookie, 
		IN PVOID ConnectionContext, 
		IN ULONG SizeOfContext, 
		OUT PVOID * ConnectionPortCookie
	);

	static void DisconnectCallback(IN PVOID ConnectionCookie);

	static NTSTATUS NewMessageCallback(
		IN PVOID PortCookie, 
		IN PVOID InputBuffer OPTIONAL, 
		IN ULONG InputBufferLength, 
		OUT PVOID OutputBuffer OPTIONAL, 
		IN ULONG OutputBufferLength, 
		OUT PULONG ReturnOutputBufferLength
	);

	NTSTATUS SendMessage(
		PVOID   pvSendBuffer,
		ULONG   uSendBufferSize,
		PVOID   pvReplyBuffer,
		PULONG  puReplyBufferSize,
		ULONG	uTimeout
	);

	~CFSDCommunicationPort();

private:
	bool		m_fClosed;
	PFLT_PORT	m_pPort;
	PFLT_PORT   m_pClientPort;
	PFLT_FILTER m_pFilter;
};

NTSTATUS NewFSDCommunicationPort(CFSDCommunicationPort** ppPort, LPCWSTR wszName, PFLT_FILTER pFilter);