#include <stdint.h>
#include <immintrin.h>
void nrLDPC_bnProc_BG1_Z224_R13_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes ) {
        __m256i* p_bnProcBuf; 
        __m256i* p_bnProcBufRes; 
        __m256i* p_llrRes; 
        __m256i* p_res; 
// Process group with 2 CNs 
// Process group with 3 CNs 
// Process group with 4 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [16128];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [16128];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 5 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [17664];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [17664];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 6 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [19584];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [19584];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [16896];
            for (int i=0;i<14;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [16896];
            for (int i=0;i<14;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [16896];
            for (int i=0;i<14;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [16896];
            for (int i=0;i<14;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [16896];
            for (int i=0;i<14;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [16896];
            for (int i=0;i<14;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 7 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [24192];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [24192];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[192];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[192 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[240];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[240 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[288];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[288 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 8 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [34944];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [34944];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[180];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[180 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[216];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[216 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[252];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[252 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 9 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [44160];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [44160];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [20352];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [20352];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [20352];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [20352];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [20352];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [20352];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [20352];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [20352];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [20352];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 10 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [47616];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [47616];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [20736];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [20736];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [20736];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [20736];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[192];
            p_llrRes = (__m256i*) &llrRes  [20736];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[192 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[240];
            p_llrRes = (__m256i*) &llrRes  [20736];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[240 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[288];
            p_llrRes = (__m256i*) &llrRes  [20736];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[288 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[336];
            p_llrRes = (__m256i*) &llrRes  [20736];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[336 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[384];
            p_llrRes = (__m256i*) &llrRes  [20736];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[384 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[432];
            p_llrRes = (__m256i*) &llrRes  [20736];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[432 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 11 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [62976];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [62976];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[180];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[180 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[216];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[216 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[252];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[252 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[288];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[288 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[324];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[324 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[360];
            p_llrRes = (__m256i*) &llrRes  [22272];
            for (int i=0;i<21;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[360 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 12 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [75648];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [75648];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[192];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[192 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[240];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[240 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[288];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[288 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[336];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[336 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[384];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[384 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[432];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[432 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[480];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[480 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[528];
            p_llrRes = (__m256i*) &llrRes  [23424];
            for (int i=0;i<28;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[528 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 13 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [94080];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [94080];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [24960];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
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
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [99072];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [99072];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[132 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[156];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[156 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[168];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[168 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[180];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[180 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[192];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[192 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[204];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[204 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[216];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[216 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[228];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[228 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[240];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[240 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[252];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[252 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[264];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[264 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[276];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[276 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[288];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[288 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[300];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[300 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[312];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[312 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[324];
            p_llrRes = (__m256i*) &llrRes  [25344];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[324 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 29 CNs 
// Process group with 30 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [109824];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [109824];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[132 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[156];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[156 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[168];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[168 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[180];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[180 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[192];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[192 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[204];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[204 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[216];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[216 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[228];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[228 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[240];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[240 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[252];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[252 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[264];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[264 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[276];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[276 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[288];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[288 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[300];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[300 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[312];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[312 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[324];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[324 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[336];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[336 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[348];
            p_llrRes = (__m256i*) &llrRes  [25728];
            for (int i=0;i<7;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[348 + i]);
            p_res++;
            p_llrRes++;
}
}
