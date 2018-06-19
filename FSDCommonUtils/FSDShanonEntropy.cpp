#include "FSDShanonEntropy.h"
#include <math.h>

#define M_LOG2E 1.4426950408889634074 

static inline double log2(double n)
{
    return log(n) * M_LOG2E;
}

double CalculateShannonEntropy(PVOID pvBuffer, size_t cbBuffer)
{
    ULONG pAlphabet[256] = {};

    size_t cbData = 0;
    for (;;)
    {
        if (cbData == cbBuffer)
        {
            break;
        }

        ASSERT(((BYTE*)pvBuffer)[cbData] < 256);
        pAlphabet[((BYTE*)pvBuffer)[cbData]]++;

        cbData++;
    }

    double dEntropy = 0.0;
    for (int i = 0; i < 256; i++)
    {
        if (pAlphabet[i] != 0)
        {

            double dTemp = (double)pAlphabet[i] / (double)cbData;
            dEntropy += (-1) * dTemp * log2(dTemp);
        }
    }

    return dEntropy;
}