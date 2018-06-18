#pragma once

#include "FSDCommonInclude.h"
#include "FSDList.h"
#include "FSDSynchronizationUtils.h"

template<class T>
class CAtomicQueue
{
public:
    CAtomicQueue() 
    {
        ASSERT(m_aList.m_pNext == NULL);
    }

    CAtomicQueue(SingleListItem* pHead)
        : m_aList.m_pNext(pHead)
    {}

    ~CAtomicQueue() 
    {
        ASSERT(m_aList.m_pNext == NULL);
    }

    void Push(T* pItem)
    {
        PushList(pItem, pItem);
    }

    void PushList(T* pStart, T* pEnd)
    {
        for (;;)
        {
            SingleListItem* const pLast = m_aList.m_pNext;
            pEnd->m_pNext = pLast;

            if (UtilInterlockedCompareExchangePointer(
                    &m_aList.m_pNext, static_cast<SingleListItem*>(pStart), static_cast<SingleListItem*>(pLast)) == pLast)
            {
                break;
            }
        }
    }

    T* CastFrom(SingleListItem* pListItem)
    {
        if (!pListItem)
        {
            return NULL;
        }

        return CONTAINING_RECORD(pListItem, T, m_pNext);
    }

    T* PopAll()
    {
        SingleListItem* pList = UtilInterlockedExchangePointer(&m_aList.m_pNext, (SingleListItem*)0);
        SingleListItem* pPrev = NULL;

        // Need to reverse list for FIFO iteration
        for (;;)
        {
            if (!pList)
            {
                break;
            }

            SingleListItem* pNext = pList->m_pNext;
            pList->m_pNext = pPrev;

            pPrev = pList;
            pList = pNext;
        }

        return CastFrom(pPrev);
    }

    bool IsEmpty() const
    {
        return m_aList.m_pNext == NULL;
    }

    T* Next(T* pItem)
    {
        return CastFrom(pItem->m_pNext);
    }

private:
    SingleListItem m_aList;
};