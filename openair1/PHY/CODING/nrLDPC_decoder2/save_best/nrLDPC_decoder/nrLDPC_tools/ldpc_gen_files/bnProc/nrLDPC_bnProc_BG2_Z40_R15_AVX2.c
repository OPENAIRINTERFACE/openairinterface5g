#include <stdint.h>
#include <immintrin.h>
void nrLDPC_bnProc_BG2_Z40_R15_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes  ) {
        __m256i* p_bnProcBuf; 
        __m256i* p_bnProcBufRes; 
        __m256i* p_llrRes; 
        __m256i* p_res; 
// Process group with 2 CNs 
// Process group with 3 CNs 
// Process group with 4 CNs 
// Process group with 5 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [14592];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [14592];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [14592];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [14592];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [14592];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [14592];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [14592];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 6 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [18432];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [18432];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [15360];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [15360];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [15360];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [15360];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [15360];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [15360];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 7 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [20736];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [20736];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [15744];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [15744];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [15744];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [15744];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [15744];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [15744];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [15744];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 8 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [23424];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [23424];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [16128];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 9 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [26496];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [26496];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[168];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[168 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[192];
            p_llrRes = (__m256i*) &llrRes  [16512];
            for (int i=0;i<3;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[192 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 10 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [33408];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [33408];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [17280];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [17280];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [17280];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [17280];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [17280];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [17280];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [17280];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [17280];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [17280];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [17280];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 11 CNs 
// Process group with 12 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [37248];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [37248];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m256i*) &llrRes  [17664];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[132 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 13 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [41856];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [41856];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [18048];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 14 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [46848];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [46848];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[132 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[156];
            p_llrRes = (__m256i*) &llrRes  [18432];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[156 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 15 CNs 
// Process group with 16 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [52224];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [52224];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[132 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[156];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[156 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[168];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[168 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[180];
            p_llrRes = (__m256i*) &llrRes  [18816];
            for (int i=0;i<2;i++) {
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
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [58368];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [58368];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[132 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[156];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[156 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[168];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[168 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[180];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[180 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[192];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[192 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[204];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[204 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[216];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[216 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[228];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[228 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[240];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[240 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[252];
            p_llrRes = (__m256i*) &llrRes  [19200];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[252 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 23 CNs 
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [66816];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [66816];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[132 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[144];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[144 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[156];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[156 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[168];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[168 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[180];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[180 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[192];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[192 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[204];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[204 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[216];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[216 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[228];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[228 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[240];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[240 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[252];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[252 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[264];
            p_llrRes = (__m256i*) &llrRes  [19584];
            for (int i=0;i<2;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[264 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 24 CNs 
// Process group with 25 CNs 
// Process group with 26 CNs 
// Process group with 27 CNs 
// Process group with 28 CNs 
// Process group with 29 CNs 
// Process group with 30 CNs 
}
