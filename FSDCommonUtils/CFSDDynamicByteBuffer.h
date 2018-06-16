#pragma once

#include "FSDCommonInclude.h"
#include "AutoPtr.h"

class CFSDDynamicByteBuffer
{
public:
    CFSDDynamicByteBuffer();
    ~CFSDDynamicByteBuffer();

    NTSTATUS Initialize(size_t cbReservation);
    
    NTSTATUS Reserve(size_t cbReservation);

    NTSTATUS Append(BYTE* pbData, size_t cbData);

    void Clear()
    {
        m_cbEffectiveSize = 0;
    }

    BYTE* Get()
    {
        return m_pBuffer.LetPtr();
    }

    size_t GetSpareSize() const
    {
        return m_cbReservedSize - m_cbEffectiveSize;
    }

    size_t ReservedSize() const
    {
        return m_cbReservedSize;
    }

private:
    CAutoArrayPtr<BYTE> m_pBuffer;
    size_t              m_cbReservedSize;
    size_t              m_cbEffectiveSize;
};

