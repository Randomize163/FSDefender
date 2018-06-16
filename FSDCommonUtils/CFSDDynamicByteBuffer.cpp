#include "CFSDDynamicByteBuffer.h"

CFSDDynamicByteBuffer::CFSDDynamicByteBuffer()
    : m_cbEffectiveSize(0)
{}

CFSDDynamicByteBuffer::~CFSDDynamicByteBuffer()
{}

NTSTATUS CFSDDynamicByteBuffer::Initialize(size_t cbReservation)
{
    NTSTATUS hr = S_OK;

    hr = Reserve(cbReservation);
    RETURN_IF_FAILED(hr);

    return S_OK;
}

NTSTATUS CFSDDynamicByteBuffer::Reserve(size_t cbReservation)
{
    CAutoArrayPtr<BYTE> pNewBuffer = new BYTE[m_cbReservedSize + cbReservation];
    RETURN_IF_FAILED_ALLOC(pNewBuffer);

    memcpy(pNewBuffer.LetPtr(), m_pBuffer.LetPtr(), m_cbEffectiveSize);

    pNewBuffer.Swap(m_pBuffer);

    return S_OK;
}

NTSTATUS CFSDDynamicByteBuffer::Append(BYTE* pbData, size_t cbData)
{
    HRESULT hr = S_OK;

    if (cbData > GetSpareSize())
    {
        hr = Reserve(max(m_cbReservedSize/2, cbData));
        RETURN_IF_FAILED(hr);
    }
    
    // TODO: return this assert
    //ASSERT(cbData <= GetSpareSize());

    memcpy(m_pBuffer.LetPtr() + m_cbEffectiveSize, pbData, cbData);

    m_cbEffectiveSize += cbData;

    return S_OK;
}