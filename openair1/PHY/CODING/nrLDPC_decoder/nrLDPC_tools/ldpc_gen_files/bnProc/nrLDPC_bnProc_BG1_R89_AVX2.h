static inline void nrLDPC_bnProc_BG1_R89_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z ) {
        uint32_t M, i; 
// Process group with 2 CNs 
 M = (3*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[12 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[12 + i ], ((__m256i*) bnProcBuf)[12 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[48 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[12 + i ], ((__m256i*) bnProcBuf)[48 + i]);
}
// Process group with 3 CNs 
       M = (21*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[84 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[48 + i ], ((__m256i*) bnProcBuf)[84 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[336 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[48 + i ], ((__m256i*) bnProcBuf)[336 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[588 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[48 + i ], ((__m256i*) bnProcBuf)[588 + i]);
}
// Process group with 4 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[840 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[300 + i ], ((__m256i*) bnProcBuf)[840 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[852 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[300 + i ], ((__m256i*) bnProcBuf)[852 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[864 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[300 + i ], ((__m256i*) bnProcBuf)[864 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[876 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[300 + i ], ((__m256i*) bnProcBuf)[876 + i]);
}
// Process group with 5 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[888 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[312 + i ], ((__m256i*) bnProcBuf)[888 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[900 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[312 + i ], ((__m256i*) bnProcBuf)[900 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[912 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[312 + i ], ((__m256i*) bnProcBuf)[912 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[924 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[312 + i ], ((__m256i*) bnProcBuf)[924 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[936 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[312 + i ], ((__m256i*) bnProcBuf)[936 + i]);
}
// Process group with 6 CNs 
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
