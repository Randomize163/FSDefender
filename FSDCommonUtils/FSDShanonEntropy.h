#pragma once
#include "FSDCommonInclude.h"

// 0.0 is an absolute order, 8.0 is an absolute chaos.
double CalculateShannonEntropy(PVOID pvBuffer, size_t cbBuffer);