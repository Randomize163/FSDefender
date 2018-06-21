#include <iostream>
#include <string>
#include <cstdint>
#include <stdint.h>
#include "MurmurHash3.h"
using namespace std;

void MurmurHash3::reset() {
    _len = 0;
    _h1 = _seed;
    data.i = 0;
}

int32_t MurmurHash3::pushByte(int8_t b) 
{
    //store the current byte of input
    data.c[_len % 4] = b;
    _len++;

    static const int32_t c1 = 0xcc9e2d51;
    static const int32_t c2 = 0x1b873593;

    /**
     * We will use this as the value of _h1 to dirty for returning to the caller
     */
    int32_t h1_as_if_done;
    if (_len > 0 && _len % 4 == 0)    //we have a valid history of 4 items!
    {
        // little endian load order
        int32_t k1 = data.i;
        k1 *= c1;
        k1 = ROTL32(k1,15);
        k1 *= c2;

        _h1 ^= k1;
        _h1 = ROTL32(_h1,13); 
        _h1 = _h1*5+0xe6546b64;
        
        h1_as_if_done = _h1;
        data.i = 0;//data is out the window now
    } 
    else  //tail case
    {
        // tail
        int32_t k1 = data.i;
        h1_as_if_done = _h1;

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;
        h1_as_if_done ^= k1;
    }

    // finalization
    h1_as_if_done ^= (int32_t)_len;

    h1_as_if_done = fmix32(h1_as_if_done);

    //  *(uint32_t*)out = h1;
    return h1_as_if_done;
}

MurmurHash3::MurmurHash3(int32_t _seed) {
    this->_seed = _seed;
    this->reset();
}

