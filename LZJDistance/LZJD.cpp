#include <algorithm>
#include <fstream>
#include <string>
#include <cstdint>
#include <algorithm>  
#include <mutex>          // std::call_once, std::once_flag

//#include <boost/function_output_iterator.hpp>


#include <intrin.h>
#include <stdint.h>


#include "LZJD.h"
#include "MurmurHash3.h"

using namespace std;

LZJD::LZJD() {
}

LZJD::~LZJD() {
}


std::vector<int32_t> getAllHashes(std::vector<char>& bytes)
{
    std::vector<int32_t> ints;

    std::unordered_set<int32_t> x_set;
    MurmurHash3 running_hash = MurmurHash3();

    for(char b : bytes) 
    {
        int8_t some_byte = b; //wherever you want to get this
        int32_t hash = running_hash.pushByte(some_byte);

        if (x_set.insert(hash).second)
        {
            //was successfully added, so never seen it before. Put it in! 
            ints.push_back(hash);
            running_hash.reset();
        }

    }

    return ints;
}

std::vector<int32_t> digest(uint64_t k, std::vector<char>& bytes)
{
    std::vector<int32_t> ints = getAllHashes(bytes);

    if(ints.size() > k)
    {
        std::nth_element (ints.begin(), ints.begin()+k, ints.end());
        ints.resize(k);
        std::sort(ints.begin(), ints.end());
    }
    else
    {
        std::sort(ints.begin(), ints.end());
        ints.resize(k);
    }
    
    return ints;
}


//Faster vecotrized list intersection taken from https://highlyscalable.wordpress.com/2012/06/05/fast-intersection-sorted-lists-sse/ 
//Need b/c C++ compiler output was over 5x slower than Java at this simple task!
//Surprisingly, sill more than 2x slower than the Java version! Go JIT go! 
size_t intersect_vector(int32_t *A, int32_t *B, size_t s_a, size_t s_b) 
{
    size_t count = 0;
    size_t i_a = 0, i_b = 0;
 
    // trim lengths to be a multiple of 4
    size_t st_a = (s_a / 4) * 4;
    size_t st_b = (s_b / 4) * 4;
 
    while(i_a < st_a && i_b < st_b) 
    {
        //[ load segments of four 32-bit elements
        __m128i v_a = _mm_load_si128((__m128i*)&A[i_a]);
        __m128i v_b = _mm_load_si128((__m128i*)&B[i_b]);
        //]
 
        //[ move pointers
        int32_t a_max = _mm_extract_epi32(v_a, 3);
        int32_t b_max = _mm_extract_epi32(v_b, 3);
        i_a += (a_max <= b_max) * 4;
        i_b += (a_max >= b_max) * 4;
        //]
 
        //[ compute mask of common elements
        const int32_t cyclic_shift = _MM_SHUFFLE(0,3,2,1);
        __m128i cmp_mask1 = _mm_cmpeq_epi32(v_a, v_b);    // pairwise comparison
        v_b = _mm_shuffle_epi32(v_b, cyclic_shift);       // shuffling
        __m128i cmp_mask2 = _mm_cmpeq_epi32(v_a, v_b);    // again...
        v_b = _mm_shuffle_epi32(v_b, cyclic_shift);
        __m128i cmp_mask3 = _mm_cmpeq_epi32(v_a, v_b);    // and again...
        v_b = _mm_shuffle_epi32(v_b, cyclic_shift);
        __m128i cmp_mask4 = _mm_cmpeq_epi32(v_a, v_b);    // and again.
        __m128i cmp_mask = _mm_or_si128(
                _mm_or_si128(cmp_mask1, cmp_mask2),
                _mm_or_si128(cmp_mask3, cmp_mask4)
        ); // OR-ing of comparison masks
        // convert the 128-bit mask to the 4-bit mask
        int32_t mask = _mm_movemask_ps(_mm_castsi128_ps(cmp_mask));
        //]
 
        //[ copy out common elements
//        __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask[mask]);
//        _mm_storeu_si128((__m128i*)&C[count], p);
        count += _mm_popcnt_u32(mask); // a number of elements is a weight of the mask
        //]
    }
 
    // intersect the tail using scalar intersection
 
    return count;
}

int32_t similarity(const std::vector<int32_t>& x_minset, const std::vector<int32_t>& y_minset)
{
    int32_t same = 0;
    
    same = (int32_t)intersect_vector((int32_t*)x_minset.data(), (int32_t*)y_minset.data(), x_minset.size(), y_minset.size());
    double sim = same / (double) (x_minset.size() + y_minset.size() - same);
    return (int) (round(100*sim));
}
