static inline void nrLDPC_bnProc_BG1_R89_AVX512(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z ) {
        uint32_t M, i; 
// Process group with 2 CNs 
 M = (3*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[6 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[6 + i ], ((__m512i*) bnProcBuf)[6 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[24 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[6 + i ], ((__m512i*) bnProcBuf)[24 + i]);
}
// Process group with 3 CNs 
       M = (21*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[42 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[24 + i ], ((__m512i*) bnProcBuf)[42 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[168 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[24 + i ], ((__m512i*) bnProcBuf)[168 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[294 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[24 + i ], ((__m512i*) bnProcBuf)[294 + i]);
}
// Process group with 4 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[420 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[150 + i ], ((__m512i*) bnProcBuf)[420 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[426 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[150 + i ], ((__m512i*) bnProcBuf)[426 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[432 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[150 + i ], ((__m512i*) bnProcBuf)[432 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[438 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[150 + i ], ((__m512i*) bnProcBuf)[438 + i]);
}
// Process group with 5 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[444 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[156 + i ], ((__m512i*) bnProcBuf)[444 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[450 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[156 + i ], ((__m512i*) bnProcBuf)[450 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[456 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[156 + i ], ((__m512i*) bnProcBuf)[456 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[462 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[156 + i ], ((__m512i*) bnProcBuf)[462 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[468 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[156 + i ], ((__m512i*) bnProcBuf)[468 + i]);
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
