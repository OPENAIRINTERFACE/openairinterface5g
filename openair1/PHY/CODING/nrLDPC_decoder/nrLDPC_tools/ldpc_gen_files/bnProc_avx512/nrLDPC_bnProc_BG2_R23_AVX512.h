#include <stdint.h>
#include <immintrin.h>
void nrLDPC_bnProc_BG2_R23_AVX512(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z  ) {
        uint32_t M, i; 
// Process group with 2 CNs 
 M = (3*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[18 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[18 + i ], ((__m512i*) bnProcBuf)[18 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[36 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[18 + i ], ((__m512i*) bnProcBuf)[36 + i]);
}
// Process group with 3 CNs 
       M = (5*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[54 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[36 + i ], ((__m512i*) bnProcBuf)[54 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[84 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[36 + i ], ((__m512i*) bnProcBuf)[84 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[114 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[36 + i ], ((__m512i*) bnProcBuf)[114 + i]);
}
// Process group with 4 CNs 
       M = (3*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[144 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[66 + i ], ((__m512i*) bnProcBuf)[144 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[162 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[66 + i ], ((__m512i*) bnProcBuf)[162 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[180 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[66 + i ], ((__m512i*) bnProcBuf)[180 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[198 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[66 + i ], ((__m512i*) bnProcBuf)[198 + i]);
}
// Process group with 5 CNs 
       M = (2*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[216 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[84 + i ], ((__m512i*) bnProcBuf)[216 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[228 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[84 + i ], ((__m512i*) bnProcBuf)[228 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[240 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[84 + i ], ((__m512i*) bnProcBuf)[240 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[252 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[84 + i ], ((__m512i*) bnProcBuf)[252 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[264 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[84 + i ], ((__m512i*) bnProcBuf)[264 + i]);
}
// Process group with 6 CNs 
       M = (1*Z + 63)>>6;
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[276 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[96 + i ], ((__m512i*) bnProcBuf)[276 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[282 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[96 + i ], ((__m512i*) bnProcBuf)[282 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[288 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[96 + i ], ((__m512i*) bnProcBuf)[288 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[294 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[96 + i ], ((__m512i*) bnProcBuf)[294 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[300 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[96 + i ], ((__m512i*) bnProcBuf)[300 + i]);
}
            for (i=0;i<M;i++) {
            ((__m512i*)bnProcBufRes)[306 + i ] = _mm512_subs_epi8(((__m512i*)llrRes)[96 + i ], ((__m512i*) bnProcBuf)[306 + i]);
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
