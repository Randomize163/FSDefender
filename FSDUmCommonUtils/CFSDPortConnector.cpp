#include "CFSDPortConnector.h"
#include "FSDCommonInclude.h"

CFSDPortConnector::CFSDPortConnector()
	: m_hPort(NULL)
{}

CFSDPortConnector::~CFSDPortConnector()
{
	if (m_hPort)
	{
		Close();
	}
}

HRESULT CFSDPortConnector::Initialize(LPCWSTR wszPortName)
{
	HRESULT hr = S_OK;

	hr = FilterConnectCommunicationPort(wszPortName, 0, NULL, 0, NULL, &m_hPort);
	RETURN_IF_FAILED(hr);

	return S_OK;
}

void CFSDPortConnector::Close()
{
	CloseHandle(m_hPort);
}

HRESULT CFSDPortConnector::SendMessage(
	LPVOID lpInBuffer, 
	DWORD dwInBufferSize, 
	LPVOID lpReplyBuffer, 
	LPDWORD lpReplyBufferSize
){
	HRESULT hr = S_OK;

	DWORD dwBytesReturned;
	hr = FilterSendMessage(m_hPort, lpInBuffer, dwInBufferSize, lpReplyBuffer, *lpReplyBufferSize, &dwBytesReturned);
	RETURN_IF_FAILED(hr);

	if (lpReplyBufferSize)
	{
		*lpReplyBufferSize = dwBytesReturned;
	}

	return S_OK;
}

HRESULT CFSDPortConnector::RecieveMessage(CFSDPortConnectorMessage * pMessage)
{
	HRESULT hr = S_OK;
		
	hr = FilterGetMessage(m_hPort, &pMessage->aRecieveHeader, sizeof(CFSDPortConnectorMessage), NULL);
	RETURN_IF_FAILED(hr);

	return S_OK;
}

HRESULT CFSDPortConnector::ReplyMessage(CFSDPortConnectorMessage * pMessage)
{
	HRESULT hr = S_OK;

	hr = FilterReplyMessage(m_hPort, &pMessage->aReplyHeader, sizeof(CFSDPortConnectorMessage));
	RETURN_IF_FAILED(hr);

	return S_OK;
}
