template<typename T>
class CAutoPtr
{
public:
	CAutoPtr()
		: m_p(NULL)
	{}

	CAutoPtr(T* p)
		: m_p(p)
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

	T** operator&()
	{
		Release();

		return &m_p;
	}

	void Release()
	{
		if (m_p)
		{
			delete m_p;
		}

		m_p = NULL;
	}

	void Detach(T** pp)
	{
		ASSERT(*pp == NULL);
		*pp = m_p;

		Release();
	}

	bool operator!() const
	{
		return m_p != NULL;
	}

private:
	CAutoPtr(T& t);
	operator=(T& t);

private:
	T* m_p;
};

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