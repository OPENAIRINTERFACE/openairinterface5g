#include <stdint.h>
#include <immintrin.h>
void nrLDPC_bnProc_BG2_R15_AVX512(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z  ) {
        __m512i* p_bnProcBuf; 
        __m512i* p_bnProcBufRes; 
        __m512i* p_llrRes; 
        __m512i* p_res; 
        uint32_t M, i; 
// Process group with 2 CNs 
// Process group with 3 CNs 
// Process group with 4 CNs 
// Process group with 5 CNs 
       M = (2*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [14592];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [14592];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [14592];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [14592];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [14592];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [14592];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m512i*) &llrRes  [14592];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[48 + i]);
}
// Process group with 6 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [18432];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [18432];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [15360];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [15360];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [15360];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [15360];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [15360];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [15360];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
// Process group with 7 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [20736];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [20736];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [15744];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [15744];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [15744];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [15744];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [15744];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [15744];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [15744];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
// Process group with 8 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [23424];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [23424];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [16128];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [16128];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [16128];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [16128];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [16128];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [16128];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [16128];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[42];
            p_llrRes = (__m512i*) &llrRes  [16128];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[42 + i]);
}
// Process group with 9 CNs 
       M = (2*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [26496];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [26496];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [16512];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [16512];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [16512];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [16512];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m512i*) &llrRes  [16512];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[48 + i]);
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m512i*) &llrRes  [16512];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[60 + i]);
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m512i*) &llrRes  [16512];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[72 + i]);
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m512i*) &llrRes  [16512];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[84 + i]);
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m512i*) &llrRes  [16512];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[96 + i]);
}
// Process group with 10 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [33408];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [33408];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [17280];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [17280];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [17280];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [17280];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [17280];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [17280];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [17280];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[42];
            p_llrRes = (__m512i*) &llrRes  [17280];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[42 + i]);
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m512i*) &llrRes  [17280];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[48 + i]);
}
            p_res = &p_bnProcBufRes[54];
            p_llrRes = (__m512i*) &llrRes  [17280];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[54 + i]);
}
// Process group with 11 CNs 
// Process group with 12 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [37248];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [37248];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[42];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[42 + i]);
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[48 + i]);
}
            p_res = &p_bnProcBufRes[54];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[54 + i]);
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[60 + i]);
}
            p_res = &p_bnProcBufRes[66];
            p_llrRes = (__m512i*) &llrRes  [17664];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[66 + i]);
}
// Process group with 13 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [41856];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [41856];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[42];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[42 + i]);
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[48 + i]);
}
            p_res = &p_bnProcBufRes[54];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[54 + i]);
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[60 + i]);
}
            p_res = &p_bnProcBufRes[66];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[66 + i]);
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m512i*) &llrRes  [18048];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[72 + i]);
}
// Process group with 14 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [46848];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [46848];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[42];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[42 + i]);
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[48 + i]);
}
            p_res = &p_bnProcBufRes[54];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[54 + i]);
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[60 + i]);
}
            p_res = &p_bnProcBufRes[66];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[66 + i]);
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[72 + i]);
}
            p_res = &p_bnProcBufRes[78];
            p_llrRes = (__m512i*) &llrRes  [18432];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[78 + i]);
}
// Process group with 15 CNs 
// Process group with 16 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [52224];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [52224];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[42];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[42 + i]);
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[48 + i]);
}
            p_res = &p_bnProcBufRes[54];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[54 + i]);
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[60 + i]);
}
            p_res = &p_bnProcBufRes[66];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[66 + i]);
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[72 + i]);
}
            p_res = &p_bnProcBufRes[78];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[78 + i]);
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[84 + i]);
}
            p_res = &p_bnProcBufRes[90];
            p_llrRes = (__m512i*) &llrRes  [18816];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[90 + i]);
}
// Process group with 17 CNs 
// Process group with 18 CNs 
// Process group with 19 CNs 
// Process group with 20 CNs 
// Process group with 21 CNs 
// Process group with 22 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [58368];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [58368];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[42];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[42 + i]);
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[48 + i]);
}
            p_res = &p_bnProcBufRes[54];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[54 + i]);
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[60 + i]);
}
            p_res = &p_bnProcBufRes[66];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[66 + i]);
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[72 + i]);
}
            p_res = &p_bnProcBufRes[78];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[78 + i]);
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[84 + i]);
}
            p_res = &p_bnProcBufRes[90];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[90 + i]);
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[96 + i]);
}
            p_res = &p_bnProcBufRes[102];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[102 + i]);
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[108 + i]);
}
            p_res = &p_bnProcBufRes[114];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[114 + i]);
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[120 + i]);
}
            p_res = &p_bnProcBufRes[126];
            p_llrRes = (__m512i*) &llrRes  [19200];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[126 + i]);
}
// Process group with <23 CNs 
       M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m512i*) &bnProcBuf    [66816];
   p_bnProcBufRes    = (__m512i*) &bnProcBufRes   [66816];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[0 + i]);
}
            p_res = &p_bnProcBufRes[6];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[6 + i]);
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[12 + i]);
}
            p_res = &p_bnProcBufRes[18];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[18 + i]);
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[24 + i]);
}
            p_res = &p_bnProcBufRes[30];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[30 + i]);
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[36 + i]);
}
            p_res = &p_bnProcBufRes[42];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[42 + i]);
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[48 + i]);
}
            p_res = &p_bnProcBufRes[54];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[54 + i]);
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[60 + i]);
}
            p_res = &p_bnProcBufRes[66];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[66 + i]);
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[72 + i]);
}
            p_res = &p_bnProcBufRes[78];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[78 + i]);
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[84 + i]);
}
            p_res = &p_bnProcBufRes[90];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[90 + i]);
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[96 + i]);
}
            p_res = &p_bnProcBufRes[102];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[102 + i]);
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[108 + i]);
}
            p_res = &p_bnProcBufRes[114];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[114 + i]);
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[120 + i]);
}
            p_res = &p_bnProcBufRes[126];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[126 + i]);
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m512i*) &llrRes  [19584];
            for (i=0;i<M;i++) {
            p_res[i] = _mm512_subs_epi8(p_llrRes[i], p_bnProcBuf[132 + i]);
}
// Process group with 24 CNs 
// Process group with 25 CNs 
// Process group with 26 CNs 
// Process group with 27 CNs 
// Process group with 28 CNs 
// Process group with 29 CNs 
// Process group with 30 CNs 
}
