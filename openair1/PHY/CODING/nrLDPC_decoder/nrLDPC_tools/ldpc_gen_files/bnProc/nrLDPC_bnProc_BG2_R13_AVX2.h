#include <stdint.h>
#include <immintrin.h>
void nrLDPC_bnProc_BG2_R13_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z  ) {
        __m256i* p_bnProcBuf; 
        __m256i* p_bnProcBufRes; 
        __m256i* p_llrRes; 
        __m256i* p_res; 
          uint32_t M, i;
// Process group with 2 CNs 
 M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [6912];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [6912];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [6912];
            for (i=0;i<M;i++) {
 M = (1*Z + 31)>>5;
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
             p_res++;
             p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [6912];
            for (i=0;i<M;i++) {
 M = (1*Z + 31)>>5;
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
             p_res++;
             p_llrRes++;
}
// Process group with 3 CNs 
// Process group with 4 CNs 
 M = (2*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [7680];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [7680];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [7296];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [7296];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [7296];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [7296];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 5 CNs 
 M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [10752];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [10752];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [8064];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [8064];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [8064];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [8064];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [8064];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 6 CNs 
 M = (5*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [12672];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [12672];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [8448];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [8448];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [8448];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[180];
            p_llrRes = (__m256i*) &llrRes  [8448];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[180 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[240];
            p_llrRes = (__m256i*) &llrRes  [8448];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[240 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[300];
            p_llrRes = (__m256i*) &llrRes  [8448];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[300 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 7 CNs 
 M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [24192];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [24192];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [10368];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [10368];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [10368];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [10368];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [10368];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [10368];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [10368];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 8 CNs 
 M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [26880];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [26880];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [10752];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [10752];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [10752];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [10752];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [10752];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [10752];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [10752];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [10752];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 9 CNs 
// Process group with 10 CNs 
// Process group with 11 CNs 
// Process group with 12 CNs 
// Process group with 13 CNs 
 M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [29952];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [29952];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [11136];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 14 CNs 
 M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [34944];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [34944];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[132 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[156];
            p_llrRes = (__m256i*) &llrRes  [11520];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[156 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 15 CNs 
// Process group with 16 CNs 
 M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [40320];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [40320];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[132 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[156];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[156 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[168];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[168 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[180];
            p_llrRes = (__m256i*) &llrRes  [11904];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[180 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 17 CNs 
// Process group with 18 CNs 
// Process group with 19 CNs 
// Process group with 20 CNs 
// Process group with 21 CNs 
// Process group with 22 CNs 
// Process group with 23 CNs 
// Process group with 24 CNs 
// Process group with 25 CNs 
// Process group with 26 CNs 
// Process group with 27 CNs 
// Process group with 28 CNs 
// Process group with 29 CNs 
// Process group with 30 CNs 
}
