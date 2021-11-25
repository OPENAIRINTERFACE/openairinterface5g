static inline void nrLDPC_bnProc_BG1_R23_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z ) {
        uint32_t M, i; 
// Process group with 2 CNs 
 M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[108 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[108 + i ], ((__m256i*) bnProcBuf)[108 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[120 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[108 + i ], ((__m256i*) bnProcBuf)[120 + i]);
}
// Process group with 3 CNs 
       M = (5*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[132 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[120 + i ], ((__m256i*) bnProcBuf)[132 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[192 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[120 + i ], ((__m256i*) bnProcBuf)[192 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[252 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[120 + i ], ((__m256i*) bnProcBuf)[252 + i]);
}
// Process group with 4 CNs 
       M = (3*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[312 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[180 + i ], ((__m256i*) bnProcBuf)[312 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[348 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[180 + i ], ((__m256i*) bnProcBuf)[348 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[384 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[180 + i ], ((__m256i*) bnProcBuf)[384 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[420 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[180 + i ], ((__m256i*) bnProcBuf)[420 + i]);
}
// Process group with 5 CNs 
       M = (7*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[456 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[216 + i ], ((__m256i*) bnProcBuf)[456 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[540 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[216 + i ], ((__m256i*) bnProcBuf)[540 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[624 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[216 + i ], ((__m256i*) bnProcBuf)[624 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[708 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[216 + i ], ((__m256i*) bnProcBuf)[708 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[792 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[216 + i ], ((__m256i*) bnProcBuf)[792 + i]);
}
// Process group with 6 CNs 
       M = (8*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[876 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[300 + i ], ((__m256i*) bnProcBuf)[876 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[972 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[300 + i ], ((__m256i*) bnProcBuf)[972 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1068 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[300 + i ], ((__m256i*) bnProcBuf)[1068 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1164 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[300 + i ], ((__m256i*) bnProcBuf)[1164 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1260 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[300 + i ], ((__m256i*) bnProcBuf)[1260 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1356 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[300 + i ], ((__m256i*) bnProcBuf)[1356 + i]);
}
// Process group with 7 CNs 
// Process group with 8 CNs 
// Process group with 9 CNs 
// Process group with 10 CNs 
// Process group with 11 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1452 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1452 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1464 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1464 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1476 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1476 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1488 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1488 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1500 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1500 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1512 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1512 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1524 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1524 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1536 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1536 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1548 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1548 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1560 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1560 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1572 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[396 + i ], ((__m256i*) bnProcBuf)[1572 + i]);
}
// Process group with 12 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1584 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1584 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1596 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1596 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1608 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1608 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1620 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1620 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1632 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1632 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1644 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1644 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1656 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1656 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1668 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1668 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1680 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1680 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1692 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1692 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1704 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1704 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1716 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[408 + i ], ((__m256i*) bnProcBuf)[1716 + i]);
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
