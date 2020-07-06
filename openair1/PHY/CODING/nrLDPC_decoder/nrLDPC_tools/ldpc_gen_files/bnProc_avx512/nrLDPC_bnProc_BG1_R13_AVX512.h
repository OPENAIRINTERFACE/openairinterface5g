static inline void nrLDPC_bnProc_BG1_R13_AVX512(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z ) {
        uint32_t M, i; 
// Process group with 2 CNs 
// Process group with 3 CNs 
// Process group with 4 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[252 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[252 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[258 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[258 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[264 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[264 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[270 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[270 + i]);
}
// Process group with 5 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[276 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[276 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[282 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[282 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[288 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[288 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[294 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[294 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[300 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[300 + i]);
}
// Process group with 6 CNs 
       M = (2*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[306 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[264 + i ], ((__m512i*) bnProcBuf)[306 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[318 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[264 + i ], ((__m512i*) bnProcBuf)[318 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[330 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[264 + i ], ((__m512i*) bnProcBuf)[330 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[342 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[264 + i ], ((__m512i*) bnProcBuf)[342 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[354 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[264 + i ], ((__m512i*) bnProcBuf)[354 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[366 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[264 + i ], ((__m512i*) bnProcBuf)[366 + i]);
}
// Process group with 7 CNs 
       M = (4*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[378 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[378 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[402 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[402 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[426 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[426 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[450 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[450 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[474 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[474 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[498 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[498 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[522 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[522 + i]);
}
// Process group with 8 CNs 
       M = (3*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[546 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[546 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[564 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[564 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[582 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[582 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[600 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[600 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[618 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[618 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[636 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[636 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[654 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[654 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[672 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[672 + i]);
}
// Process group with 9 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[690 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[318 + i ], ((__m512i*) bnProcBuf)[690 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[696 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[318 + i ], ((__m512i*) bnProcBuf)[696 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[702 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[318 + i ], ((__m512i*) bnProcBuf)[702 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[708 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[318 + i ], ((__m512i*) bnProcBuf)[708 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[714 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[318 + i ], ((__m512i*) bnProcBuf)[714 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[720 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[318 + i ], ((__m512i*) bnProcBuf)[720 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[726 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[318 + i ], ((__m512i*) bnProcBuf)[726 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[732 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[318 + i ], ((__m512i*) bnProcBuf)[732 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[738 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[318 + i ], ((__m512i*) bnProcBuf)[738 + i]);
}
// Process group with 10 CNs 
       M = (4*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[744 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[324 + i ], ((__m512i*) bnProcBuf)[744 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[768 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[324 + i ], ((__m512i*) bnProcBuf)[768 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[792 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[324 + i ], ((__m512i*) bnProcBuf)[792 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[816 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[324 + i ], ((__m512i*) bnProcBuf)[816 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[840 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[324 + i ], ((__m512i*) bnProcBuf)[840 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[864 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[324 + i ], ((__m512i*) bnProcBuf)[864 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[888 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[324 + i ], ((__m512i*) bnProcBuf)[888 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[912 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[324 + i ], ((__m512i*) bnProcBuf)[912 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[936 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[324 + i ], ((__m512i*) bnProcBuf)[936 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[960 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[324 + i ], ((__m512i*) bnProcBuf)[960 + i]);
}
// Process group with 11 CNs 
       M = (3*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[984 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[984 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1002 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[1002 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1020 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[1020 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1038 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[1038 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1056 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[1056 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1074 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[1074 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1092 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[1092 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1110 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[1110 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1128 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[1128 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1146 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[1146 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1164 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[348 + i ], ((__m512i*) bnProcBuf)[1164 + i]);
}
// Process group with 12 CNs 
       M = (4*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1182 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1182 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1206 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1206 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1230 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1230 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1254 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1254 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1278 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1278 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1302 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1302 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1326 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1326 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1350 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1350 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1374 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1374 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1398 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1398 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1422 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1422 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1446 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[366 + i ], ((__m512i*) bnProcBuf)[1446 + i]);
}
// Process group with 13 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1470 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1470 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1476 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1476 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1482 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1482 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1488 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1488 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1494 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1494 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1500 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1500 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1506 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1506 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1512 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1512 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1518 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1518 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1524 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1524 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1530 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1530 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1536 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1536 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1542 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[390 + i ], ((__m512i*) bnProcBuf)[1542 + i]);
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
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1548 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1548 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1554 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1554 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1560 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1560 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1566 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1566 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1572 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1572 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1578 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1578 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1584 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1584 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1590 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1590 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1596 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1596 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1602 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1602 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1608 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1608 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1614 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1614 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1620 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1620 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1626 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1626 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1632 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1632 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1638 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1638 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1644 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1644 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1650 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1650 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1656 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1656 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1662 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1662 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1668 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1668 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1674 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1674 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1680 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1680 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1686 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1686 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1692 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1692 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1698 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1698 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1704 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1704 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1710 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[396 + i ], ((__m512i*) bnProcBuf)[1710 + i]);
}
// Process group with 29 CNs 
// Process group with 30 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1716 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1716 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1722 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1722 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1728 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1728 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1734 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1734 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1740 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1740 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1746 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1746 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1752 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1752 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1758 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1758 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1764 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1764 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1770 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1770 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1776 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1776 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1782 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1782 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1788 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1788 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1794 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1794 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1800 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1800 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1806 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1806 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1812 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1812 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1818 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1818 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1824 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1824 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1830 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1830 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1836 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1836 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1842 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1842 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1848 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1848 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1854 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1854 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1860 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1860 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1866 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1866 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1872 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1872 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1878 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1878 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1884 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1884 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1890 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[402 + i ], ((__m512i*) bnProcBuf)[1890 + i]);
}
}
