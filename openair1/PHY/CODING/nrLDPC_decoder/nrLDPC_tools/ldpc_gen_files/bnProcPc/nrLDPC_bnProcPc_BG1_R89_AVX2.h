static inline void nrLDPC_bnProcPc_BG1_R89_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,int8_t* llrRes ,  int8_t* llrProcBuf, uint16_t Z ) {
   __m256i ymm0, ymm1, ymmRes0, ymmRes1;  
        __m128i* p_bnProcBuf; 
        __m128i* p_llrProcBuf;
        __m256i* p_llrRes; 
         uint32_t M ;
// Process group with 1 CNs 
// Process group with 2 CNs 
 M = (3*Z + 31)>>5;
    p_bnProcBuf     = (__m128i*) &bnProcBuf    [384];
    p_llrProcBuf    = (__m128i*) &llrProcBuf   [384];
    p_llrRes        = (__m256i*) &llrRes       [384];
            for (int i=0,j=0;i<M;i++,j+=2) {
            ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);
            ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf[j + 1]);
            ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[72 + j]);
            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
            ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[72 + j +1]);
           ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); 
            ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);
            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
            ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);
            ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);
            ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);
            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);
}
// Process group with 3 CNs 
 M = (21*Z + 31)>>5;
    p_bnProcBuf     = (__m128i*) &bnProcBuf    [2688];
    p_llrProcBuf    = (__m128i*) &llrProcBuf   [1536];
    p_llrRes        = (__m256i*) &llrRes       [1536];
        for (int i=0,j=0;i<M;i++,j+=2) {
        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);
        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);
        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[504 + j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[504 + j +1]);
        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); 
        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[1008 + j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[1008 + j +1]);
        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); 
        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);
        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);
        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);
            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);
}
// Process group with 4 CNs 
 M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m128i*) &bnProcBuf    [26880];
    p_llrProcBuf    = (__m128i*) &llrProcBuf   [9600];
    p_llrRes        = (__m256i*) &llrRes       [9600];
        for (int i=0,j=0;i<M;i++,j+=2) {
        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);
        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);
        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[24 + j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[24 + j +1]);
       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); 
        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[48 + j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[48 + j +1]);
       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); 
        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[72 + j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[72 + j +1]);
       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); 
        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);
        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);
        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);
            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);
}
// Process group with 5 CNs 
 M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m128i*) &bnProcBuf    [28416];
    p_llrProcBuf    = (__m128i*) &llrProcBuf   [9984];
    p_llrRes        = (__m256i*) &llrRes       [9984];
        for (int i=0,j=0;i<M;i++,j+=2) {
        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);
        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);
        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[24 + j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[24 + j +1]);
       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); 
        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[48 + j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[48 + j +1]);
       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); 
        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[72 + j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[72 + j +1]);
       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); 
        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[96 + j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[96 + j +1]);
       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); 
        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);
        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);
        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);
        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);
        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);
            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);
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
