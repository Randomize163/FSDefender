#include "CFilter.h"
#include "FSDCommonInclude.h"
#include "FSDCommonDefs.h"

CFilter::CFilter()
    : m_hFilter(NULL)
{}

CFilter::~CFilter()
{
    if (m_hFilter)
    {
        Close();
    }
}

NTSTATUS CFilter::Initialize(PDRIVER_OBJECT pDriverObject, const FLT_REGISTRATION* pFilterRegistration)
{
    NTSTATUS hr = S_OK;

    hr = FltRegisterFilter(pDriverObject, pFilterRegistration, &m_hFilter);
    RETURN_IF_FAILED(hr);

    return S_OK;
}

void CFilter::Close()
{
    ASSERT(m_hFilter);

    FltUnregisterFilter(m_hFilter);
}

NTSTATUS CFilter::StartFiltering()
{
    NTSTATUS hr = S_OK;

    hr = FltStartFiltering(m_hFilter);
    RETURN_IF_FAILED(hr);

    return S_OK;
}
