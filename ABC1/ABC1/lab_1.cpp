#include <iostream>
#include <cstdint>
#include "stdio.h"
#include <malloc.h>
#include <mmintrin.h>

using namespace std;

void getResultCPP(_int8* a, _int8* b, _int8* c, _int16* d)
{
    int16_t res[8];

    for (int i = 0; i < 8; i++) {
        res[i] = (a[i] * b[i]) + (c[i] - d[i]);
    }

    printf("Easy way: %i %i %i %i %i %i %i %i \n",
        res[0], res[1], res[2], res[3], res[4], res[5],
        res[6], res[7]);
}

__m64 putArrayInMM(_int8* arr) {
    uint64_t bytes = 18446744073709551615;
    __m64 mmreg;

    //movq mm0, arr
    memcpy(&bytes, arr, 8);
    mmreg = _m_por(_m_from_int(bytes), _mm_slli_si64(_m_from_int(bytes >> 32), 32));

    return mmreg;
}


int main() //15. F[i]=(A[i] *B[i]) +(C[i] -D[i]) , i=1...8
{
    
    int8_t A[8] = { 120, 56, 31, -53, 12, -46, -99, 31 };
    int8_t B[8] = { 113, -23, 71, 65, -76, -33, -14, 12 };
    int8_t C[8] = { -111, 11, 23, 65, 34, 23, 19, -55 };
    int16_t D[8] = { 17179, -7446, 11151, -8391, 14240, 17901, -28751, -30917 };
    /*
    int8_t A[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int8_t B[8] = { 3, 4, 5, 6, 7, 8, 9, 10 };
    int8_t C[8] = { 6, 7, 8, 9, 10, 11, 12, 13 };
    int16_t D[8] = { 4, 5, 6, 7, 8, 9, 10, 11 };
    */

    getResultCPP((_int8*)A, (_int8*)B, (_int8*)C, (_int16*)D);
    //промежуточное long long число для последующей конвертации
    uint64_t bytes, pt1, pt2;
    __m64 temp_reg;
    __m64 zero_vector = _mm_setzero_si64(); // 0 вектор
    __m64 zero2_vector = _mm_setzero_si64(); // 0 вектор

    __m64 mm0, mm1, mm2, mm3;
    __m64 d_high, d_low;

    
    //movq mm0, A
    mm0 = putArrayInMM((_int8*)&A);
    // punpcklbw .. __m64 _m_punpcklbw
    temp_reg = _m_pcmpgtb(zero_vector, mm0); //mask of higher bits
    __m64 a_low = _m_punpcklbw(mm0, temp_reg);
    __m64 a_high = _m_punpckhbw(mm0, temp_reg);

    
    //movq mm1, B
    mm1 = putArrayInMM((_int8*)&B);
    // punpcklbw .. __m64 _m_punpcklbw
    temp_reg = _m_pcmpgtb(zero_vector, mm1); //mask of higher bits
    __m64 b_low = _m_punpcklbw(mm1, temp_reg);
    __m64 b_high = _m_punpckhbw(mm1, temp_reg);


    //multiplying A AND B
    //__m64 ab_low = _m_pmaddwd(a_low, b_low);    // 2 двойных слова произведени¤ [0..3]
    //__m64 ab_high = _m_pmaddwd(a_high, b_high); // 2 двойных слова произведени¤ [4..7]
    __m64 ab_low = _m_pmullw(a_low, b_low);
    __m64 ab_high = _m_pmullw(a_high, b_high); // <ultiplyLowerWords сработало


    mm2 = putArrayInMM((_int8*)&C);
    // punpcklbw .. __m64 _m_punpcklbw
    temp_reg = _m_pcmpgtb(zero_vector, mm2); //mask of higher bits
    __m64 c_low = _m_punpcklbw(mm2, temp_reg);  // 4 слова 
    __m64 c_high = _m_punpckhbw(mm2, temp_reg); // 4 слова



    //movq mm0, D
    memcpy(&pt1, D, sizeof(bytes)); // [0..3]
    d_low = _m_por(_m_from_int(pt1), _mm_slli_si64(_m_from_int(pt1 >> 32), 32));
    memcpy(&pt2, &D[4], sizeof(bytes)); // [4..7]
    d_high = _m_por(_m_from_int(pt2), _mm_slli_si64(_m_from_int(pt2 >> 32), 32));


    // C[i] - D[i]
    c_low = _m_psubsw(c_low, d_low);
    c_high = _m_psubsw(c_high, d_high);

    //—умма
    a_low = _m_paddsw(ab_low, c_low);
    a_high = _m_paddsw(ab_high, c_high);

    int16_t val[8];
    memcpy(val, &a_low, sizeof(a_low));
    memcpy(&val[4], &a_high, sizeof(a_high));

    printf("Hard  way: %i %i %i %i %i %i %i %i \n\n",
        val[0], val[1], val[2], val[3], val[4], val[5],
        val[6], val[7]);

    return 0;
}