#ifndef _MURMURHASH3_H__
#define _MURMURHASH3_H__

#include <cstdint>



#define FORCE_INLINE inline //__attribute__((always_inline))

//See https://stackoverflow.com/questions/24089740/murmurhash3-between-java-and-c-is-not-aligning 
inline uint32_t rotl32(uint32_t x, int8_t r) {
//    return (x << r) | (x >> (32 - r));
    return (x << r) | (int32_t)((uint32_t)x >> (32 - r)); //similar to >>> in Java
}

#define ROTL32(x,y) rotl32(x,y)


//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

FORCE_INLINE int32_t fmix32 ( int32_t h )
{
//  h ^= h >> 16;
//  h *= 0x85ebca6b;
//  h ^= h >> 13;
//  h *= 0xc2b2ae35;
//  h ^= h >> 16;
  h ^= (int32_t)((uint32_t)h >> 16); // similar to >>> in Java
  h *= 0x85ebca6b;
  h ^= (int32_t)((uint32_t)h >> 13);
  h *= 0xc2b2ae35;
  h ^= (int32_t)((uint32_t)h >> 16);
  
  return h;
}

using namespace std;

union DataBlock
{
    std::int32_t i;     // occupies 4 bytes
    std::int8_t c[4];  
};   

class MurmurHash3
{
    /**
     * the length of the current running byte sequence
     */
    uint32_t _len;
    /**
     * the byte history for the last 4 bytes seen
     */
    DataBlock data;
    int32_t _h1;
    int32_t _seed = 0;

    public:
        MurmurHash3() : MurmurHash3(0) {};
        MurmurHash3(int32_t);

        void reset();

        int32_t pushByte(int8_t b);

};

#endif
