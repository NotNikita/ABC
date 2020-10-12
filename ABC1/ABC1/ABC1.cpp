#include <iostream>
#include <cstdint>
#include "stdio.h"
#include <malloc.h>

using namespace std;

__declspec(naked) int* SSESolve(_int8* a, _int8* b, _int8* c, _int16* d)
{
    _asm { 
        pushad
        mov       eax, [esp + 32 + 4]
        mov       ebx, [esp + 32 + 8]
        mov       ecx, [esp + 32 + 12]
        mov       edx, [esp + 32 + 16]

        movq      xmm0, qword ptr[eax]
        movq      xmm1, qword ptr[ebx]
        movq      xmm2, qword ptr[ecx]
        movupd    xmm3, [edx]

        punpcklbw xmm0, xmm0
        punpcklbw xmm1, xmm1
        punpcklbw xmm2, xmm2
        psraw     xmm0, 8
        psraw     xmm1, 8
        psraw     xmm2, 8

        pmaddwd   xmm0, xmm1
        psubsw    xmm2, xmm3
        paddsw    xmm0, xmm2
        popad
        push      16
        call      dword ptr[malloc]
        add       esp, 4
        movupd[eax], xmm0
        ret
    }
}

int main() //15. F[i]=(A[i] *B[i]) +(C[i] -D[i]) , i=1...8
{
    int16_t arr_RESULT[8];
    int8_t A[8] = { 120, 56, 31, -53, 12, -46, -99, 31 };
    int8_t B[8] = { 113, -23, 71, 65, -76, -33, -14, 12 };
    int8_t C[8] = { -111, 11, 23, 65, 34, 23, 19, -55 };
    int16_t D[8] = { 17179, -7446, 11151, -8391, 14240, 17901, -28751, -30917 };

    SSESolve((_int8*)&A, (_int8*)&B, (_int8*)&C, (_int16*)&D);


    return 0;

}