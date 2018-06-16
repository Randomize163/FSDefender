#pragma once

struct ListItem
{
    ListItem()
        : m_pNext(NULL)
        , m_pPrev(NULL)
    {}

    ListItem* m_pNext;
    ListItem* m_pPrev;
};

template<class T>
class CFSDList
{
public:
    CFSDList()
        : m_cSize(0)
    {
        m_HeadItem.m_pNext = &m_TailItem;
        m_TailItem.m_pPrev = &m_HeadItem;
    }

    ~CFSDList()
    {
        ASSERT(m_cSize == 0);
    }

    T* CastFrom(ListItem* pListItem)
    {
        return CONTAINING_RECORD(pListItem, T, m_pNext);
    }

    T* Front()
    {
        if (Size() == 0)
        {
            return NULL;
        }

        return CastFrom(m_HeadItem.m_pNext);
    }

    void PushFront(ListItem* pItem)
    {
        pItem->m_pNext = m_HeadItem.m_pNext;
        pItem->m_pPrev = &m_HeadItem;

        m_HeadItem.m_pNext->m_pPrev = pItem;
        m_HeadItem.m_pNext = pItem;
       
        m_cSize++;
    }

    T* PopFront()
    {
        if (Size() == 0)
        {
            return NULL;
        }

        ListItem* pItem = m_HeadItem.m_pNext;

        pItem->m_pNext->m_pPrev = &m_HeadItem;
        m_HeadItem.m_pNext      = pItem->m_pNext;

        pItem->m_pNext = NULL;
        pItem->m_pPrev  = NULL;

        m_cSize--;

        return CastFrom(pItem);
    }

    size_t Size() const
    {
        return m_cSize;
    }

    ListItem* Back()
    {
        if (Size() == 0)
        {
            return NULL;
        }

        return m_TailItem.m_pPrev;
    }

    void PushBack(ListItem* pItem)
    {
        pItem->m_pPrev = m_TailItem.m_pPrev;
        pItem->m_pNext = &m_TailItem;

        m_TailItem.m_pPrev->m_pNext = pItem;
        m_TailItem.m_pPrev          = pItem;
    }

    T* PopBack()
    {
        if (Size() == 0)
        {
            return NULL;
        }

        ListItem* pItem = m_TailItem.m_pPrev;

        pItem->m_pPrev->m_pNext = &m_TailItem;
        m_TailItem.m_pPrev = pItem->m_pPrev;

        pItem->m_pNext = NULL;
        pItem->m_Prev = NULL;

        m_cSize--;

        return CastFrom(pItem);
    }

private:
    ListItem m_HeadItem;
    ListItem m_TailItem;

    size_t   m_cSize;
};