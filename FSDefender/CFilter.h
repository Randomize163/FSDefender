#pragma once
#include <fltKernel.h>

class CFilter
{
public:
	CFilter();

	~CFilter();

	NTSTATUS Initialize(
		_In_ PDRIVER_OBJECT		     pDriverObject,
		_In_ const FLT_REGISTRATION* pFilterRegistration
	);

	void Close();

	NTSTATUS StartFiltering();
	
	PFLT_FILTER Handle() const
	{
		return m_hFilter;
	}

private:
	PFLT_FILTER m_hFilter;
};

