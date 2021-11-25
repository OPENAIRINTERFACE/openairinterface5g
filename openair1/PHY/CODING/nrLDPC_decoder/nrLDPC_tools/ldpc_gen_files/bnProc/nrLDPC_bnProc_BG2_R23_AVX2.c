void nrLDPC_bnProc_BG2_R23_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z  ) {
        __m256i* p_bnProcBuf; 
        __m256i* p_bnProcBufRes; 
        __m256i* p_llrRes; 
        __m256i* p_res; 
        uint32_t M, i; 
// Process group with 2 CNs 
 M = (3*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[36 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[36 + i ], ((__m256i*) bnProcBuf)[36 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[72 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[36 + i ], ((__m256i*) bnProcBuf)[72 + i]);
}
// Process group with 3 CNs 
       M = (5*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[108 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[72 + i ], ((__m256i*) bnProcBuf)[108 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[168 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[72 + i ], ((__m256i*) bnProcBuf)[168 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[228 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[72 + i ], ((__m256i*) bnProcBuf)[228 + i]);
}
// Process group with 4 CNs 
       M = (3*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[288 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[132 + i ], ((__m256i*) bnProcBuf)[288 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[324 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[132 + i ], ((__m256i*) bnProcBuf)[324 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[360 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[132 + i ], ((__m256i*) bnProcBuf)[360 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[396 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[132 + i ], ((__m256i*) bnProcBuf)[396 + i]);
}
// Process group with 5 CNs 
       M = (2*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[432 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[168 + i ], ((__m256i*) bnProcBuf)[432 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[456 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[168 + i ], ((__m256i*) bnProcBuf)[456 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[480 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[168 + i ], ((__m256i*) bnProcBuf)[480 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[504 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[168 + i ], ((__m256i*) bnProcBuf)[504 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[528 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[168 + i ], ((__m256i*) bnProcBuf)[528 + i]);
}
// Process group with 6 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[552 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[192 + i ], ((__m256i*) bnProcBuf)[552 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[564 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[192 + i ], ((__m256i*) bnProcBuf)[564 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[576 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[192 + i ], ((__m256i*) bnProcBuf)[576 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[588 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[192 + i ], ((__m256i*) bnProcBuf)[588 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[600 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[192 + i ], ((__m256i*) bnProcBuf)[600 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[612 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[192 + i ], ((__m256i*) bnProcBuf)[612 + i]);
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
