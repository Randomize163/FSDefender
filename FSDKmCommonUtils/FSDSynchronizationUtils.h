#pragma once

#include "FSDCommonInclude.h"

class CSpinLock
{
public:
    CSpinLock()
    {
        KeInitializeSpinLock(&m_aLock);
    }

    void Acquire(KIRQL* pOldIrql)
    {
        KeAcquireSpinLock(&m_aLock, pOldIrql);
    }

    void Release(KIRQL aOldIrql)
    {
        KeReleaseSpinLock(&m_aLock, aOldIrql);
    }

    ~CSpinLock() {}

private:
    CSpinLock(CSpinLock&);
    CSpinLock& operator=(CSpinLock&);

private:
    KSPIN_LOCK m_aLock;
};


template<class T>
class CAutoLock
{
public:
    CAutoLock(T* pLock)
        : m_pLock(pLock)
        , m_fAcquired(false)
    {
        Acquire();
    }

    ~CAutoLock()
    {
        if (m_fAcquired)
        {
            Release();
        }

        ASSERT(!m_fAcquired);
    }

    void Acquire()
    {
        ASSERT(!m_fAcquired);

        m_pLock->Acquire(&m_aIrql);
        m_fAcquired = true;
    }

    void Release()
    {
        ASSERT(m_fAcquired);

        m_pLock->Release(m_aIrql);
        m_fAcquired = false;
    }

private:
    T*    m_pLock;
    bool  m_fAcquired;
    KIRQL m_aIrql;
};

typedef CAutoLock<CSpinLock> CAutoSpinLock;

template<class T>
inline T* UtilInterlockedCompareExchangePointer(T* volatile* ppDest, T* pExchange, T* pComperand)
{
    return static_cast<T*>(InterlockedCompareExchangePointer(
        reinterpret_cast<PVOID volatile*>(ppDest), pExchange, pComperand));
}

template<class T>
inline T* UtilInterlockedExchangePointer(T* volatile* ppDest, T* pExchange)
{
    return static_cast<T*>(InterlockedExchangePointer(
        reinterpret_cast<PVOID volatile*>(ppDest), pExchange));
}