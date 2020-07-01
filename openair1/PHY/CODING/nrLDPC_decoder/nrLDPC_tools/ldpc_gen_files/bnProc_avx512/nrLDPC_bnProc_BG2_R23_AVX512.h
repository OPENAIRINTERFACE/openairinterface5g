#include <stdint.h>
#include <immintrin.h>
void nrLDPC_bnProc_BG2_R23_AVX512(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z  ) {
        __m512i* p_bnProcBuf; 
        __m512i* p_bnProcBufRes; 
        __m512i* p_llrRes; 
        __m512i* p_res; 
        uint32_t M, i; 
// Process group with 2 CNs 
 M = (3*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [1152];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [1152];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [1152];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [1152];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
// Process group with 3 CNs 
       M = (5*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [3456];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [3456];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [2304];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [2304];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m512i*) &llrRes  [2304];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[60 + i]);
}
// Process group with 4 CNs 
       M = (3*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [9216];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [9216];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [4224];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [4224];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [4224];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[54];
            p_llrRes = (__m512i*) &llrRes  [4224];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[54 + i]);
}
// Process group with 5 CNs 
       M = (2*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [13824];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [13824];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [5376];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [5376];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [5376];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [5376];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m512i*) &llrRes  [5376];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[48 + i]);
}
// Process group with 6 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [17664];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [17664];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [6144];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [6144];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [6144];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [6144];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [6144];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [6144];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
// Process group with 7 CNs 
// Process group with 8 CNs 
// Process group with 9 CNs 
// Process group with 10 CNs 
// Process group with 11 CNs 
// Process group with 12 CNs 
// Process group with 13 CNs 
// Process group with 14 CNs 
// Process group with 15 CNs 
// Process group with 16 CNs 
// Process group with 17 CNs 
// Process group with 18 CNs 
// Process group with 19 CNs 
// Process group with 20 CNs 
// Process group with 21 CNs 
// Process group with 22 CNs 
// Process group with <23 CNs 
// Process group with 24 CNs 
// Process group with 25 CNs 
// Process group with 26 CNs 
// Process group with 27 CNs 
// Process group with 28 CNs 
// Process group with 29 CNs 
// Process group with 30 CNs 
}
