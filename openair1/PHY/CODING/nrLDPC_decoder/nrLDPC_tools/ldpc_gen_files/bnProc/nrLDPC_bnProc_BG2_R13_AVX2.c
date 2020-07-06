void nrLDPC_bnProc_BG2_R13_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z  ) {
        __m256i* p_bnProcBuf; 
        __m256i* p_bnProcBufRes; 
        __m256i* p_llrRes; 
        __m256i* p_res; 
        uint32_t M, i; 
// Process group with 2 CNs 
 M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[216 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[216 + i ], ((__m256i*) bnProcBuf)[216 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[228 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[216 + i ], ((__m256i*) bnProcBuf)[228 + i]);
}
// Process group with 3 CNs 
// Process group with 4 CNs 
       M = (2*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[240 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[228 + i ], ((__m256i*) bnProcBuf)[240 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[264 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[228 + i ], ((__m256i*) bnProcBuf)[264 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[288 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[228 + i ], ((__m256i*) bnProcBuf)[288 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[312 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[228 + i ], ((__m256i*) bnProcBuf)[312 + i]);
}
// Process group with 5 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[336 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[252 + i ], ((__m256i*) bnProcBuf)[336 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[348 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[252 + i ], ((__m256i*) bnProcBuf)[348 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[360 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[252 + i ], ((__m256i*) bnProcBuf)[360 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[372 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[252 + i ], ((__m256i*) bnProcBuf)[372 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[384 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[252 + i ], ((__m256i*) bnProcBuf)[384 + i]);
}
// Process group with 6 CNs 
       M = (5*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[396 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[264 + i ], ((__m256i*) bnProcBuf)[396 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[456 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[264 + i ], ((__m256i*) bnProcBuf)[456 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[516 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[264 + i ], ((__m256i*) bnProcBuf)[516 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[576 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[264 + i ], ((__m256i*) bnProcBuf)[576 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[636 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[264 + i ], ((__m256i*) bnProcBuf)[636 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[696 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[264 + i ], ((__m256i*) bnProcBuf)[696 + i]);
}
// Process group with 7 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[756 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[324 + i ], ((__m256i*) bnProcBuf)[756 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[768 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[324 + i ], ((__m256i*) bnProcBuf)[768 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[780 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[324 + i ], ((__m256i*) bnProcBuf)[780 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[792 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[324 + i ], ((__m256i*) bnProcBuf)[792 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[804 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[324 + i ], ((__m256i*) bnProcBuf)[804 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[816 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[324 + i ], ((__m256i*) bnProcBuf)[816 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[828 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[324 + i ], ((__m256i*) bnProcBuf)[828 + i]);
}
// Process group with 8 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[840 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[336 + i ], ((__m256i*) bnProcBuf)[840 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[852 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[336 + i ], ((__m256i*) bnProcBuf)[852 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[864 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[336 + i ], ((__m256i*) bnProcBuf)[864 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[876 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[336 + i ], ((__m256i*) bnProcBuf)[876 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[888 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[336 + i ], ((__m256i*) bnProcBuf)[888 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[900 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[336 + i ], ((__m256i*) bnProcBuf)[900 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[912 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[336 + i ], ((__m256i*) bnProcBuf)[912 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[924 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[336 + i ], ((__m256i*) bnProcBuf)[924 + i]);
}
// Process group with 9 CNs 
// Process group with 10 CNs 
// Process group with 11 CNs 
// Process group with 12 CNs 
// Process group with 13 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[936 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[936 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[948 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[948 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[960 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[960 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[972 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[972 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[984 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[984 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[996 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[996 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1008 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[1008 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1020 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[1020 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1032 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[1032 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1044 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[1044 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1056 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[1056 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1068 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[1068 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1080 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[348 + i ], ((__m256i*) bnProcBuf)[1080 + i]);
}
// Process group with 14 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1092 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1092 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1104 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1104 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1116 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1116 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1128 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1128 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1140 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1140 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1152 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1152 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1164 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1164 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1176 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1176 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1188 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1188 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1200 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1200 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1212 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1212 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1224 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1224 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1236 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1236 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1248 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[360 + i ], ((__m256i*) bnProcBuf)[1248 + i]);
}
// Process group with 15 CNs 
// Process group with 16 CNs 
       M = (1*Z + 31)>>5;
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1260 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1260 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1272 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1272 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1284 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1284 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1296 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1296 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1308 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1308 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1320 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1320 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1332 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1332 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1344 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1344 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1356 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1356 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1368 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1368 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1380 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1380 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1392 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1392 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1404 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1404 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1416 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1416 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1428 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1428 + i]);
}
            for (i=0;i<M;i++) {
            ((__m256i*)bnProcBufRes)[1440 + i ] = _mm256_subs_epi8(((__m256i*)llrRes)[372 + i ], ((__m256i*) bnProcBuf)[1440 + i]);
}
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
