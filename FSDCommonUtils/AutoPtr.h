#pragma once

#include "FSDCommonInclude.h"

template<class T>
class CAuto
{
public:
    CAuto()
        : m_p(NULL)
    {}

    CAuto(T* p)
        : m_p(p)
    {}

    ~CAuto()
    {
        ASSERT(m_p == NULL);
    }

    virtual void Release() = 0;

    void Detach(T** pp)
    {
        ASSERT(*pp == NULL);
        *pp = m_p;

        m_p = NULL;
    }

    void Swap(T** pp)
    {
        T* tmp = *pp;
        *pp = m_p;
        m_p = tmp;
    }

    void Swap(CAuto<T>& p)
    {
        T* tmp = p.m_p;
        p.m_p = m_p;
        m_p = tmp;
    }

    T* Get() const
    {
        return m_p;
    }

    bool operator!() const
    {
        return m_p == NULL;
    }

    T** operator&()
    {
        Release();

        return &m_p;
    }

    T* LetPtr()
    {
        T* p = m_p;
        m_p = NULL;

        return p;
    }

public:
    T * m_p;
};

template<class T>
class CAutoPtr : public CAuto<T>
{
public:
    CAutoPtr()
        : CAuto<T>()
    {}

    CAutoPtr(T* p)
        : CAuto<T>(p)
    {}

    ~CAutoPtr()
    {
        Release();
    }

    T& operator*() const
    {
        return *m_p; 
    }

    T* operator->() const 
    { 
        return m_p; 
    }

    virtual void Release() override
    {
        if (this->m_p)
        {
            delete this->m_p;
            this->m_p = NULL;
        }
    }

private:
    CAutoPtr(T& t);
    T& operator=(T& t);
};

template<class T>
class CAutoArrayPtr : public CAutoPtr<T>
{
public:
    CAutoArrayPtr()
    {}

    CAutoArrayPtr(T* p)
        : CAutoPtr<T>(p)
    {}

    ~CAutoArrayPtr()
    {
        Release();
    }

    virtual void Release() override
    {
        if (this->m_p)
        {
            delete[] this->m_p;
            this->m_p = NULL;
        }
    }

    T& operator[](ULONG i)
    {
        return this->m_p[i];
    }
};

typedef CAutoArrayPtr<WCHAR> CAutoStringW;
typedef CAutoArrayPtr<CHAR>  CAutoStringA;

class CAutoHandle : public CAuto<void>
{
public:
    CAutoHandle()
        : CAuto<void>()
    {}

    CAutoHandle(HANDLE hHandle)
        : CAuto<void>(hHandle)
    {}

    ~CAutoHandle()
    {
        Release();
    }

    virtual void Release() override
    {
        if (this->m_p)
        {
            CloseHandle(this->m_p);
            this->m_p = NULL;
        }
    }
};

#ifdef _KERNEL_MODE 
class CAutoNameInformation : public CAutoPtr<FLT_FILE_NAME_INFORMATION>
{
public:
    CAutoNameInformation()
        : CAutoPtr<FLT_FILE_NAME_INFORMATION>()
    {}

    CAutoNameInformation(PFLT_FILE_NAME_INFORMATION pNameInformation)
        : CAutoPtr<FLT_FILE_NAME_INFORMATION>(pNameInformation)
    {}

    ~CAutoNameInformation()
    {
        Release();
    }

    virtual void Release()
    {
        if (this->m_p)
        {
            FltReleaseFileNameInformation(this->m_p);
            this->m_p = NULL;
        }
    }
};
#endif

template<typename T, class ... Types>
NTSTATUS NewInstanceOf(T** ppNewInstance, Types ... args)
{
    NTSTATUS hr = S_OK;

    CAutoPtr<T> pInstance = new T();
    RETURN_IF_FAILED_ALLOC(pInstance);

    hr = pInstance->Initialize(args...);
    RETURN_IF_FAILED(hr);

    pInstance.Detach(ppNewInstance);

    return S_OK;
}