static inline void nrLDPC_bnProcPc_BG1_R89_AVX512(int8_t* bnProcBuf,int8_t* llrRes ,  int8_t* llrProcBuf, uint16_t Z ) {
   __m512i zmm0, zmm1, zmmRes0, zmmRes1;  
        __m256i* p_bnProcBuf; 
        __m256i* p_llrProcBuf;
        __m512i* p_llrRes; 
         uint32_t M ;
// Process group with 1 CNs 
 M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [0];
    p_llrProcBuf    = (__m256i*) &llrProcBuf   [0];
    p_llrRes        = (__m512i*) &llrRes       [0];
            for (int i=0,j=0;i<M;i++,j+=2) {
            zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);
            zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[j + 1]);
            zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);
            zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
            zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);
            zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1);
            zmm0 = _mm512_packs_epi16(zmmRes0, zmmRes1);
            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);
}
// Process group with 2 CNs 
 M = (3*Z + 63)>>6;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [384];
    p_llrProcBuf    = (__m256i*) &llrProcBuf   [384];
    p_llrRes        = (__m512i*) &llrRes       [384];
            for (int i=0,j=0;i<M;i++,j+=2) {
            zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);
            zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf[j + 1]);
            zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[36 + j]);
            zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
            zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[36 + j +1]);
           zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1); 
            zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);
            zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
            zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);
            zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1);
            zmm0 = _mm512_packs_epi16(zmmRes0, zmmRes1);
            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);
}
// Process group with 3 CNs 
 M = (21*Z + 63)>>6;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [2688];
    p_llrProcBuf    = (__m256i*) &llrProcBuf   [1536];
    p_llrRes        = (__m512i*) &llrRes       [1536];
        for (int i=0,j=0;i<M;i++,j+=2) {
        zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);
        zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);
        zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[252 + j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[252 + j +1]);
        zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1); 
        zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[504 + j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[504 + j +1]);
        zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1); 
        zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);
        zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1);
        zmm0 = _mm512_packs_epi16(zmmRes0, zmmRes1);
            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);
}
// Process group with 4 CNs 
 M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [26880];
    p_llrProcBuf    = (__m256i*) &llrProcBuf   [9600];
    p_llrRes        = (__m512i*) &llrRes       [9600];
        for (int i=0,j=0;i<M;i++,j+=2) {
        zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);
        zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);
        zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[12 + j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[12 + j +1]);
       zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1); 
        zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[24 + j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[24 + j +1]);
       zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1); 
        zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[36 + j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[36 + j +1]);
       zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1); 
        zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);
        zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1);
        zmm0 = _mm512_packs_epi16(zmmRes0, zmmRes1);
            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);
}
// Process group with 5 CNs 
 M = (1*Z + 63)>>6;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [28416];
    p_llrProcBuf    = (__m256i*) &llrProcBuf   [9984];
    p_llrRes        = (__m512i*) &llrRes       [9984];
        for (int i=0,j=0;i<M;i++,j+=2) {
        zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);
        zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);
        zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[12 + j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[12 + j +1]);
       zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1); 
        zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[24 + j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[24 + j +1]);
       zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1); 
        zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[36 + j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[36 + j +1]);
       zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1); 
        zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[48 + j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[48 + j +1]);
       zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1); 
        zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);
        zmmRes0 = _mm512_adds_epi16(zmmRes0, zmm0);
        zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);
        zmmRes1 = _mm512_adds_epi16(zmmRes1, zmm1);
        zmm0 = _mm512_packs_epi16(zmmRes0, zmmRes1);
            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);
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
