#include <stdint.h>
#include <immintrin.h>
void nrLDPC_bnProc_BG2_R15_AVX512(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z  ) {
        uint32_t M, i; 
// Process group with 2 CNs 
// Process group with 3 CNs 
// Process group with 4 CNs 
// Process group with 5 CNs 
       M = (2*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[228 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[228 + i ], ((__m512i*) bnProcBuf)[228 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[240 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[228 + i ], ((__m512i*) bnProcBuf)[240 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[252 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[228 + i ], ((__m512i*) bnProcBuf)[252 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[264 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[228 + i ], ((__m512i*) bnProcBuf)[264 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[276 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[228 + i ], ((__m512i*) bnProcBuf)[276 + i]);
}
// Process group with 6 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[288 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[240 + i ], ((__m512i*) bnProcBuf)[288 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[294 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[240 + i ], ((__m512i*) bnProcBuf)[294 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[300 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[240 + i ], ((__m512i*) bnProcBuf)[300 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[306 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[240 + i ], ((__m512i*) bnProcBuf)[306 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[312 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[240 + i ], ((__m512i*) bnProcBuf)[312 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[318 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[240 + i ], ((__m512i*) bnProcBuf)[318 + i]);
}
// Process group with 7 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[324 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[246 + i ], ((__m512i*) bnProcBuf)[324 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[330 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[246 + i ], ((__m512i*) bnProcBuf)[330 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[336 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[246 + i ], ((__m512i*) bnProcBuf)[336 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[342 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[246 + i ], ((__m512i*) bnProcBuf)[342 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[348 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[246 + i ], ((__m512i*) bnProcBuf)[348 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[354 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[246 + i ], ((__m512i*) bnProcBuf)[354 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[360 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[246 + i ], ((__m512i*) bnProcBuf)[360 + i]);
}
// Process group with 8 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[366 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[366 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[372 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[372 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[378 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[378 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[384 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[384 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[390 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[390 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[396 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[396 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[402 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[402 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[408 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[252 + i ], ((__m512i*) bnProcBuf)[408 + i]);
}
// Process group with 9 CNs 
       M = (2*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[414 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[414 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[426 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[426 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[438 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[438 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[450 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[450 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[462 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[462 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[474 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[474 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[486 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[486 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[498 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[498 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[510 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[258 + i ], ((__m512i*) bnProcBuf)[510 + i]);
}
// Process group with 10 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[522 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[270 + i ], ((__m512i*) bnProcBuf)[522 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[528 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[270 + i ], ((__m512i*) bnProcBuf)[528 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[534 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[270 + i ], ((__m512i*) bnProcBuf)[534 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[540 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[270 + i ], ((__m512i*) bnProcBuf)[540 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[546 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[270 + i ], ((__m512i*) bnProcBuf)[546 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[552 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[270 + i ], ((__m512i*) bnProcBuf)[552 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[558 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[270 + i ], ((__m512i*) bnProcBuf)[558 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[564 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[270 + i ], ((__m512i*) bnProcBuf)[564 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[570 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[270 + i ], ((__m512i*) bnProcBuf)[570 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[576 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[270 + i ], ((__m512i*) bnProcBuf)[576 + i]);
}
// Process group with 11 CNs 
// Process group with 12 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[582 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[582 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[588 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[588 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[594 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[594 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[600 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[600 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[606 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[606 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[612 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[612 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[618 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[618 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[624 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[624 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[630 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[630 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[636 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[636 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[642 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[642 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[648 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[276 + i ], ((__m512i*) bnProcBuf)[648 + i]);
}
// Process group with 13 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[654 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[654 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[660 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[660 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[666 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[666 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[672 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[672 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[678 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[678 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[684 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[684 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[690 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[690 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[696 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[696 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[702 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[702 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[708 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[708 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[714 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[714 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[720 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[720 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[726 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[282 + i ], ((__m512i*) bnProcBuf)[726 + i]);
}
// Process group with 14 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[732 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[732 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[738 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[738 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[744 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[744 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[750 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[750 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[756 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[756 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[762 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[762 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[768 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[768 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[774 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[774 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[780 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[780 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[786 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[786 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[792 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[792 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[798 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[798 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[804 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[804 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[810 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[288 + i ], ((__m512i*) bnProcBuf)[810 + i]);
}
// Process group with 15 CNs 
// Process group with 16 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[816 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[816 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[822 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[822 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[828 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[828 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[834 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[834 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[840 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[840 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[846 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[846 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[852 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[852 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[858 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[858 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[864 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[864 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[870 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[870 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[876 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[876 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[882 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[882 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[888 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[888 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[894 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[894 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[900 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[900 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[906 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[294 + i ], ((__m512i*) bnProcBuf)[906 + i]);
}
// Process group with 17 CNs 
// Process group with 18 CNs 
// Process group with 19 CNs 
// Process group with 20 CNs 
// Process group with 21 CNs 
// Process group with 22 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[912 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[912 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[918 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[918 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[924 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[924 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[930 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[930 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[936 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[936 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[942 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[942 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[948 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[948 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[954 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[954 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[960 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[960 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[966 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[966 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[972 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[972 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[978 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[978 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[984 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[984 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[990 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[990 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[996 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[996 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1002 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[1002 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1008 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[1008 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1014 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[1014 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1020 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[1020 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1026 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[1026 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1032 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[1032 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1038 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[300 + i ], ((__m512i*) bnProcBuf)[1038 + i]);
}
// Process group with <23 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1044 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1044 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1050 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1050 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1056 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1056 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1062 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1062 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1068 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1068 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1074 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1074 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1080 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1080 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1086 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1086 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1092 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1092 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1098 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1098 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1104 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1104 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1110 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1110 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1116 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1116 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1122 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1122 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1128 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1128 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1134 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1134 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1140 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1140 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1146 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1146 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1152 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1152 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1158 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1158 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1164 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1164 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1170 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1170 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[1176 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[306 + i ], ((__m512i*) bnProcBuf)[1176 + i]);
}
// Process group with 24 CNs 
// Process group with 25 CNs 
// Process group with 26 CNs 
// Process group with 27 CNs 
// Process group with 28 CNs 
// Process group with 29 CNs 
// Process group with 30 CNs 
}
