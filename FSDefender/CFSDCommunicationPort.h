#pragma once
#include <fltKernel.h>
#include "FSDUtils.h"

typedef NTSTATUS(*FSD_CONNECT_CALLBACK)(PVOID pvContext, PFLT_PORT pClientPort);
typedef void(*FSD_DISCONNECT_CALLBACK)(PVOID pvContext, PFLT_PORT pClientPort);
typedef NTSTATUS(*FSD_NEW_MESSAGE_CALLBACK)(
	IN	PVOID pvContext,
	IN  PVOID pvInputBuffer, 
	IN  ULONG uInputBufferLength, 
	OUT PVOID pvOutputBuffer, 
	IN  ULONG uOutputBufferLength, 
	OUT PULONG puReturnOutputBufferLength);

class CFSDCommunicationPort
{
public:
	CFSDCommunicationPort();

	NTSTATUS Initialize(
		LPCWSTR					 wszName, 
		PFLT_FILTER			     pFilter, 
		PVOID				     pvContext,
		FSD_CONNECT_CALLBACK	 pfnOnConnect,
		FSD_DISCONNECT_CALLBACK	 pfnOnDisconnect,
		FSD_NEW_MESSAGE_CALLBACK pfnOnNewMessage
	);

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

	PVOID				     m_pvContext;
	FSD_CONNECT_CALLBACK	 m_pfnOnConnect;
	FSD_DISCONNECT_CALLBACK	 m_pfnOnDisconnect;
	FSD_NEW_MESSAGE_CALLBACK m_pfnOnNewMessage;
};