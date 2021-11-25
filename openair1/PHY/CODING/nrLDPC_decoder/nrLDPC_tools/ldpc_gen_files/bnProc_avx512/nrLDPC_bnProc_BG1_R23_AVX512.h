static inline void nrLDPC_bnProc_BG1_R23_AVX512(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z ) {
        uint32_t M, i; 
// Process group with 2 CNs 
 M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[54 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[54 + i ], ((__m512i*) bnProcBuf)[54 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[60 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[54 + i ], ((__m512i*) bnProcBuf)[60 + i]);
}
// Process group with 3 CNs 
       M = (5*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[66 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[60 + i ], ((__m512i*) bnProcBuf)[66 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[96 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[60 + i ], ((__m512i*) bnProcBuf)[96 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[126 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[60 + i ], ((__m512i*) bnProcBuf)[126 + i]);
}
// Process group with 4 CNs 
       M = (3*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[156 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[90 + i ], ((__m512i*) bnProcBuf)[156 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[174 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[90 + i ], ((__m512i*) bnProcBuf)[174 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[192 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[90 + i ], ((__m512i*) bnProcBuf)[192 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[210 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[90 + i ], ((__m512i*) bnProcBuf)[210 + i]);
}
// Process group with 5 CNs 
       M = (7*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[228 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[108 + i ], ((__m512i*) bnProcBuf)[228 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[270 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[108 + i ], ((__m512i*) bnProcBuf)[270 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[312 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[108 + i ], ((__m512i*) bnProcBuf)[312 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[354 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[108 + i ], ((__m512i*) bnProcBuf)[354 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[396 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[108 + i ], ((__m512i*) bnProcBuf)[396 + i]);
}
// Process group with 6 CNs 
       M = (8*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[438 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[150 + i ], ((__m512i*) bnProcBuf)[438 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[486 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[150 + i ], ((__m512i*) bnProcBuf)[486 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[534 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[150 + i ], ((__m512i*) bnProcBuf)[534 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[582 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[150 + i ], ((__m512i*) bnProcBuf)[582 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[630 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[150 + i ], ((__m512i*) bnProcBuf)[630 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[678 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[150 + i ], ((__m512i*) bnProcBuf)[678 + i]);
}
// Process group with 7 CNs 
// Process group with 8 CNs 
// Process group with 9 CNs 
// Process group with 10 CNs 
// Process group with 11 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[726 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[726 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[732 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[732 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[738 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[738 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[744 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[744 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[750 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[750 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[756 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[756 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[762 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[762 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[768 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[768 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[774 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[774 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[780 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[780 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[786 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[198 + i ], ((__m512i*) bnProcBuf)[786 + i]);
}
// Process group with 12 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[792 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[792 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[798 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[798 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[804 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[804 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[810 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[810 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[816 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[816 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[822 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[822 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[828 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[828 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[834 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[834 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[840 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[840 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[846 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[846 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[852 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[852 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[858 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[204 + i ], ((__m512i*) bnProcBuf)[858 + i]);
}
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
