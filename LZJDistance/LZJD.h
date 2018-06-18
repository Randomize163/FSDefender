#ifndef _LZJD_H__
#define _LZJD_H__

#include <set>
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_set>
#include "MurmurHash3.h"

using namespace std;

class LZJD 
{
    public:
        LZJD();
        ~LZJD();
};

std::vector<int32_t> getAllHashes(std::vector<char>& bytes);

std::vector<int32_t> digest(uint64_t k, std::vector<char>& bytes);

int32_t similarity(const std::vector<int32_t>& x_minset, const std::vector<int32_t>& y_minset);

#endif
