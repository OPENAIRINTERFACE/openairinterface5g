/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <execinfo.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef MR_MAIN
#include "PHY/defs_common.h"
#include "PHY/impl_defs_top.h"
#include "tools_defs.h"
#else
#include "time_meas.h"
#include "UTIL/LOG/log.h"
#define debug_msg
#define ONE_OVER_SQRT2_Q15 23170

int oai_exit=0;
#endif

#define ONE_OVER_SQRT3_Q15 18919

#include "PHY/sse_intrin.h"

#include "assertions.h"

#define print_shorts(s,x) printf("%s %d,%d,%d,%d,%d,%d,%d,%d\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7])
#define print_shorts256(s,x) printf("%s %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7],(x)[8],(x)[9],(x)[10],(x)[11],(x)[12],(x)[13],(x)[14],(x)[15])

#define print_ints(s,x) printf("%s %d %d %d %d\n",s,(x)[0],(x)[1],(x)[2],(x)[3])


const static int16_t conjugatedft[32] __attribute__((aligned(32))) = {-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1};


const static int16_t reflip[32]  __attribute__((aligned(32))) = {1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1};



#if defined(__x86_64__) || defined(__i386__)
static inline void cmac(__m128i a,__m128i b, __m128i *re32, __m128i *im32) __attribute__((always_inline));
static inline void cmac(__m128i a,__m128i b, __m128i *re32, __m128i *im32)
{

  __m128i cmac_tmp,cmac_tmp_re32,cmac_tmp_im32;

  cmac_tmp    = _mm_sign_epi16(b,*(__m128i*)reflip);
  cmac_tmp_re32  = _mm_madd_epi16(a,cmac_tmp);


  //  cmac_tmp    = _mm_shufflelo_epi16(b,_MM_SHUFFLE(2,3,0,1));
  //  cmac_tmp    = _mm_shufflehi_epi16(cmac_tmp,_MM_SHUFFLE(2,3,0,1));
  cmac_tmp = _mm_shuffle_epi8(b,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  cmac_tmp_im32  = _mm_madd_epi16(cmac_tmp,a);

  *re32 = _mm_add_epi32(*re32,cmac_tmp_re32);
  *im32 = _mm_add_epi32(*im32,cmac_tmp_im32);
}

static inline void cmacc(__m128i a,__m128i b, __m128i *re32, __m128i *im32) __attribute__((always_inline));
static inline void cmacc(__m128i a,__m128i b, __m128i *re32, __m128i *im32)
{

  __m128i cmac_tmp,cmac_tmp_re32,cmac_tmp_im32;


  cmac_tmp_re32  = _mm_madd_epi16(a,b);


  cmac_tmp    = _mm_sign_epi16(b,*(__m128i*)reflip);
  //  cmac_tmp    = _mm_shufflelo_epi16(b,_MM_SHUFFLE(2,3,0,1));
  //  cmac_tmp    = _mm_shufflehi_epi16(cmac_tmp,_MM_SHUFFLE(2,3,0,1));
  cmac_tmp = _mm_shuffle_epi8(cmac_tmp,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  cmac_tmp_im32  = _mm_madd_epi16(cmac_tmp,a);

  *re32 = _mm_add_epi32(*re32,cmac_tmp_re32);
  *im32 = _mm_add_epi32(*im32,cmac_tmp_im32);
}

#ifdef __AVX2__
static inline void cmac_256(__m256i a,__m256i b, __m256i *re32, __m256i *im32) __attribute__((always_inline));
static inline void cmac_256(__m256i a,__m256i b, __m256i *re32, __m256i *im32)
{

  __m256i cmac_tmp,cmac_tmp_re32,cmac_tmp_im32;
  __m256i imshuffle = _mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,19,18,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  cmac_tmp       = _mm256_sign_epi16(b,*(__m256i*)reflip);
  cmac_tmp_re32  = _mm256_madd_epi16(a,cmac_tmp);

  cmac_tmp       = _mm256_shuffle_epi8(b,imshuffle);
  cmac_tmp_im32  = _mm256_madd_epi16(cmac_tmp,a);

  *re32 = _mm256_add_epi32(*re32,cmac_tmp_re32);
  *im32 = _mm256_add_epi32(*im32,cmac_tmp_im32);
}

static inline void cmacc_256(__m256i a,__m256i b, __m256i *re32, __m256i *im32) __attribute__((always_inline));
static inline void cmacc_256(__m256i a,__m256i b, __m256i *re32, __m256i *im32)
{

  __m256i cmac_tmp,cmac_tmp_re32,cmac_tmp_im32;
  __m256i imshuffle = _mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,19,18,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  cmac_tmp_re32   = _mm256_madd_epi16(a,b);


  cmac_tmp        = _mm256_sign_epi16(b,*(__m256i*)reflip);
  cmac_tmp        = _mm256_shuffle_epi8(b,imshuffle);
  cmac_tmp_im32   = _mm256_madd_epi16(cmac_tmp,a);

  *re32 = _mm256_add_epi32(*re32,cmac_tmp_re32);
  *im32 = _mm256_add_epi32(*im32,cmac_tmp_im32);
}

#endif

static inline void cmult(__m128i a,__m128i b, __m128i *re32, __m128i *im32) __attribute__((always_inline));

static inline void cmult(__m128i a,__m128i b, __m128i *re32, __m128i *im32)
{

  register __m128i mmtmpb;

  mmtmpb    = _mm_sign_epi16(b,*(__m128i*)reflip);
  *re32     = _mm_madd_epi16(a,mmtmpb);
  //  mmtmpb    = _mm_shufflelo_epi16(b,_MM_SHUFFLE(2,3,0,1));
  //  mmtmpb    = _mm_shufflehi_epi16(mmtmpb,_MM_SHUFFLE(2,3,0,1));
  mmtmpb        = _mm_shuffle_epi8(b,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  *im32  = _mm_madd_epi16(a,mmtmpb);

}

#ifdef __AVX2__
static inline void cmult_256(__m256i a,__m256i b, __m256i *re32, __m256i *im32) __attribute__((always_inline));

static inline void cmult_256(__m256i a,__m256i b, __m256i *re32, __m256i *im32)
{

  register __m256i mmtmpb;
  __m256i const perm_mask = _mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  mmtmpb    = _mm256_sign_epi16(b,*(__m256i*)reflip);
  *re32     = _mm256_madd_epi16(a,mmtmpb);
  mmtmpb    = _mm256_shuffle_epi8(b,perm_mask);
  *im32     = _mm256_madd_epi16(a,mmtmpb);

}

#endif

static inline void cmultc(__m128i a,__m128i b, __m128i *re32, __m128i *im32) __attribute__((always_inline));

static inline void cmultc(__m128i a,__m128i b, __m128i *re32, __m128i *im32)
{

  register __m128i mmtmpb;

  *re32     = _mm_madd_epi16(a,b);
  mmtmpb    = _mm_sign_epi16(b,*(__m128i*)reflip);
  mmtmpb    = _mm_shuffle_epi8(mmtmpb,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  *im32  = _mm_madd_epi16(a,mmtmpb);

}

#ifdef __AVX2__
static inline void cmultc_256(__m256i a,__m256i b, __m256i *re32, __m256i *im32) __attribute__((always_inline));

static inline void cmultc_256(__m256i a,__m256i b, __m256i *re32, __m256i *im32)
{

  register __m256i mmtmpb;
  __m256i const perm_mask = _mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  *re32     = _mm256_madd_epi16(a,b);
  mmtmpb    = _mm256_sign_epi16(b,*(__m256i*)reflip);
  mmtmpb    = _mm256_shuffle_epi8(mmtmpb,perm_mask);
  *im32     = _mm256_madd_epi16(a,mmtmpb);

}

#endif

static inline __m128i cpack(__m128i xre,__m128i xim) __attribute__((always_inline));

static inline __m128i cpack(__m128i xre,__m128i xim)
{

  register __m128i cpack_tmp1,cpack_tmp2;

  cpack_tmp1 = _mm_unpacklo_epi32(xre,xim);
  cpack_tmp2 = _mm_unpackhi_epi32(xre,xim);
  return(_mm_packs_epi32(_mm_srai_epi32(cpack_tmp1,15),_mm_srai_epi32(cpack_tmp2,15)));

}

#ifdef __AVX2__
static inline __m256i cpack_256(__m256i xre,__m256i xim) __attribute__((always_inline));

static inline __m256i cpack_256(__m256i xre,__m256i xim)
{

  register __m256i cpack_tmp1,cpack_tmp2;

  cpack_tmp1 = _mm256_unpacklo_epi32(xre,xim);
  cpack_tmp2 = _mm256_unpackhi_epi32(xre,xim);
  return(_mm256_packs_epi32(_mm256_srai_epi32(cpack_tmp1,15),_mm256_srai_epi32(cpack_tmp2,15)));

}

#endif

static inline void packed_cmult(__m128i a,__m128i b, __m128i *c) __attribute__((always_inline));

static inline void packed_cmult(__m128i a,__m128i b, __m128i *c)
{

  __m128i cre,cim;
  cmult(a,b,&cre,&cim);
  *c = cpack(cre,cim);

}

#ifdef __AVX2__
static inline void packed_cmult_256(__m256i a,__m256i b, __m256i *c) __attribute__((always_inline));

static inline void packed_cmult_256(__m256i a,__m256i b, __m256i *c)
{

  __m256i cre,cim;
  cmult_256(a,b,&cre,&cim);
  *c = cpack_256(cre,cim);

}
#endif

static inline void packed_cmultc(__m128i a,__m128i b, __m128i *c) __attribute__((always_inline));

static inline void packed_cmultc(__m128i a,__m128i b, __m128i *c)
{

  __m128i cre,cim;

  cmultc(a,b,&cre,&cim);
  *c = cpack(cre,cim);

}

#ifdef __AVX2__
static inline void packed_cmultc_256(__m256i a,__m256i b, __m256i *c) __attribute__((always_inline));

static inline void packed_cmultc_256(__m256i a,__m256i b, __m256i *c)
{

  __m256i cre,cim;

  cmultc_256(a,b,&cre,&cim);
  *c = cpack_256(cre,cim);

}
#endif

static inline __m128i packed_cmult2(__m128i a,__m128i b,__m128i b2) __attribute__((always_inline));

static inline __m128i packed_cmult2(__m128i a,__m128i b,__m128i b2)
{


  register __m128i cre,cim;

  cre       = _mm_madd_epi16(a,b);
  cim       = _mm_madd_epi16(a,b2);

  return(cpack(cre,cim));

}

#ifdef __AVX2__
static inline __m256i packed_cmult2_256(__m256i a,__m256i b,__m256i b2) __attribute__((always_inline));

static inline __m256i packed_cmult2_256(__m256i a,__m256i b,__m256i b2)
{


  register __m256i cre,cim;

  cre       = _mm256_madd_epi16(a,b);
  cim       = _mm256_madd_epi16(a,b2);

  return(cpack_256(cre,cim));

}
#endif

#elif defined (__arm__)
static inline void cmac(int16x8_t a,int16x8_t b, int32x4_t *re32, int32x4_t *im32) __attribute__((always_inline));
static inline void cmac(int16x8_t a,int16x8_t b, int32x4_t *re32, int32x4_t *im32)
{

  
  int32x4_t ab_re0,ab_re1,ab_im0,ab_im1;
  int16x8_t bflip = vrev32q_s16(b);
  int16x8_t bconj = vmulq_s16(b,*(int16x8_t *)reflip);

  ab_re0 = vmull_s16(((int16x4_t*)&a)[0],((int16x4_t*)&bconj)[0]);
  ab_re1 = vmull_s16(((int16x4_t*)&a)[1],((int16x4_t*)&bconj)[1]);
  ab_im0 = vmull_s16(((int16x4_t*)&a)[0],((int16x4_t*)&bflip)[0]);
  ab_im1 = vmull_s16(((int16x4_t*)&a)[1],((int16x4_t*)&bflip)[1]);
  *re32 = vqaddq_s32(*re32,vcombine_s32(vpadd_s32(((int32x2_t*)&ab_re0)[0],((int32x2_t*)&ab_re0)[1]),
					vpadd_s32(((int32x2_t*)&ab_re1)[0],((int32x2_t*)&ab_re1)[1])));
  *im32 = vqaddq_s32(*im32,vcombine_s32(vpadd_s32(((int32x2_t*)&ab_im0)[0],((int32x2_t*)&ab_im0)[1]),
					vpadd_s32(((int32x2_t*)&ab_im1)[0],((int32x2_t*)&ab_im1)[1])));
}

static inline void cmacc(int16x8_t a,int16x8_t b, int32x4_t *re32, int32x4_t *im32) __attribute__((always_inline));
static inline void cmacc(int16x8_t a,int16x8_t b, int32x4_t *re32, int32x4_t *im32)
{
  int32x4_t ab_re0,ab_re1,ab_im0,ab_im1;
  int16x8_t bconj = vmulq_s16(b,*(int16x8_t *)reflip);
  int16x8_t bflip = vrev32q_s16(bconj);

  ab_re0 = vmull_s16(((int16x4_t*)&a)[0],((int16x4_t*)&b)[0]);
  ab_re1 = vmull_s16(((int16x4_t*)&a)[1],((int16x4_t*)&b)[1]);
  ab_im0 = vmull_s16(((int16x4_t*)&a)[0],((int16x4_t*)&bflip)[0]);
  ab_im1 = vmull_s16(((int16x4_t*)&a)[1],((int16x4_t*)&bflip)[1]);
  *re32 = vqaddq_s32(*re32,vcombine_s32(vpadd_s32(((int32x2_t*)&ab_re0)[0],((int32x2_t*)&ab_re0)[1]),
					vpadd_s32(((int32x2_t*)&ab_re1)[0],((int32x2_t*)&ab_re1)[1])));
  *im32 = vqaddq_s32(*im32,vcombine_s32(vpadd_s32(((int32x2_t*)&ab_im0)[0],((int32x2_t*)&ab_im0)[1]),
					vpadd_s32(((int32x2_t*)&ab_im1)[0],((int32x2_t*)&ab_im1)[1])));

}

static inline void cmult(int16x8_t a,int16x8_t b, int32x4_t *re32, int32x4_t *im32) __attribute__((always_inline));
static inline void cmult(int16x8_t a,int16x8_t b, int32x4_t *re32, int32x4_t *im32)
{
  int32x4_t ab_re0,ab_re1,ab_im0,ab_im1;
  int16x8_t bflip = vrev32q_s16(b);
  int16x8_t bconj = vmulq_s16(b,*(int16x8_t *)reflip);
  int16x4_t al,ah,bcl,bch,bfl,bfh;
  int32x2_t abr0l,abr0h,abr1l,abr1h,abi0l,abi0h,abi1l,abi1h;

  al  = vget_low_s16(a);      ah = vget_high_s16(a);
  bcl = vget_low_s16(bconj);  bch = vget_high_s16(bconj);
  bfl = vget_low_s16(bflip);  bfh = vget_high_s16(bflip);

  ab_re0 = vmull_s16(al,bcl);
  ab_re1 = vmull_s16(ah,bch);
  ab_im0 = vmull_s16(al,bfl);
  ab_im1 = vmull_s16(ah,bfh);
  abr0l = vget_low_s32(ab_re0); abr0h = vget_high_s32(ab_re0);
  abr1l = vget_low_s32(ab_re1); abr1h = vget_high_s32(ab_re1);
  abi0l = vget_low_s32(ab_im0); abi0h = vget_high_s32(ab_im0);
  abi1l = vget_low_s32(ab_im1); abi1h = vget_high_s32(ab_im1);

  *re32 = vcombine_s32(vpadd_s32(abr0l,abr0h),
                       vpadd_s32(abr1l,abr1h));
  *im32 = vcombine_s32(vpadd_s32(abi0l,abi0h),
                       vpadd_s32(abi1l,abi1h));
}

static inline void cmultc(int16x8_t a,int16x8_t b, int32x4_t *re32, int32x4_t *im32) __attribute__((always_inline));

static inline void cmultc(int16x8_t a,int16x8_t b, int32x4_t *re32, int32x4_t *im32)
{
  int32x4_t ab_re0,ab_re1,ab_im0,ab_im1;
  int16x8_t bconj = vmulq_s16(b,*(int16x8_t *)reflip);
  int16x8_t bflip = vrev32q_s16(bconj);
  int16x4_t al,ah,bl,bh,bfl,bfh; 
  int32x2_t abr0l,abr0h,abr1l,abr1h,abi0l,abi0h,abi1l,abi1h;
  al  = vget_low_s16(a);     ah = vget_high_s16(a);
  bl  = vget_low_s16(b);     bh = vget_high_s16(b);
  bfl = vget_low_s16(bflip); bfh = vget_high_s16(bflip);

  ab_re0 = vmull_s16(al,bl);
  ab_re1 = vmull_s16(ah,bh);
  ab_im0 = vmull_s16(al,bfl);
  ab_im1 = vmull_s16(ah,bfh);

  abr0l = vget_low_s32(ab_re0); abr0h = vget_high_s32(ab_re0);
  abr1l = vget_low_s32(ab_re1); abr1h = vget_high_s32(ab_re1);
  abi0l = vget_low_s32(ab_im0); abi0h = vget_high_s32(ab_im0);
  abi1l = vget_low_s32(ab_im1); abi1h = vget_high_s32(ab_im1);

  *re32 = vcombine_s32(vpadd_s32(abr0l,abr0h),
		       vpadd_s32(abr1l,abr1h));
  *im32 = vcombine_s32(vpadd_s32(abi0l,abi0h),
		       vpadd_s32(abi1l,abi1h));

}


static inline int16x8_t cpack(int32x4_t xre,int32x4_t xim) __attribute__((always_inline));

static inline int16x8_t cpack(int32x4_t xre,int32x4_t xim)
{
  int32x4x2_t xtmp;

  xtmp = vzipq_s32(xre,xim);
  return(vcombine_s16(vqshrn_n_s32(xtmp.val[0],15),vqshrn_n_s32(xtmp.val[1],15)));

}


static inline void packed_cmult(int16x8_t a,int16x8_t b, int16x8_t *c) __attribute__((always_inline));

static inline void packed_cmult(int16x8_t a,int16x8_t b, int16x8_t *c)
{

  int32x4_t cre,cim;
  cmult(a,b,&cre,&cim);
  *c = cpack(cre,cim);

}


static inline void packed_cmultc(int16x8_t a,int16x8_t b, int16x8_t *c) __attribute__((always_inline));

static inline void packed_cmultc(int16x8_t a,int16x8_t b, int16x8_t *c)
{

  int32x4_t cre,cim;

  cmultc(a,b,&cre,&cim);
  *c = cpack(cre,cim);

}

static inline int16x8_t packed_cmult2(int16x8_t a,int16x8_t b,  int16x8_t b2) __attribute__((always_inline));

static inline int16x8_t packed_cmult2(int16x8_t a,int16x8_t b,  int16x8_t b2)
{

  

  int32x4_t ab_re0,ab_re1,ab_im0,ab_im1,cre,cim;
  
  ab_re0 = vmull_s16(((int16x4_t*)&a)[0],((int16x4_t*)&b)[0]);
  ab_re1 = vmull_s16(((int16x4_t*)&a)[1],((int16x4_t*)&b)[1]);
  ab_im0 = vmull_s16(((int16x4_t*)&a)[0],((int16x4_t*)&b2)[0]);
  ab_im1 = vmull_s16(((int16x4_t*)&a)[1],((int16x4_t*)&b2)[1]);
  cre = vcombine_s32(vpadd_s32(((int32x2_t*)&ab_re0)[0],((int32x2_t*)&ab_re0)[1]),
		     vpadd_s32(((int32x2_t*)&ab_re1)[0],((int32x2_t*)&ab_re1)[1]));
  cim = vcombine_s32(vpadd_s32(((int32x2_t*)&ab_im0)[0],((int32x2_t*)&ab_im0)[1]),
		     vpadd_s32(((int32x2_t*)&ab_im1)[0],((int32x2_t*)&ab_im1)[1]));
  return(cpack(cre,cim));

}

#endif

const static int16_t W0s[16]__attribute__((aligned(32))) = {32767,0,32767,0,32767,0,32767,0,32767,0,32767,0,32767,0,32767,0};

const static int16_t W13s[16]__attribute__((aligned(32))) = {-16384,-28378,-16384,-28378,-16384,-28378,-16384,-28378,-16384,-28378,-16384,-28378,-16384,-28378,-16384,-28378};
const static int16_t W23s[16]__attribute__((aligned(32))) = {-16384,28378,-16384,28378,-16384,28378,-16384,28378,-16384,28378,-16384,28378,-16384,28378,-16384,28378};

const static int16_t W15s[16]__attribute__((aligned(32))) = {10126,-31163,10126,-31163,10126,-31163,10126,-31163,10126,-31163,10126,-31163,10126,-31163,10126,-31163};
const static int16_t W25s[16]__attribute__((aligned(32))) = {-26509,-19260,-26509,-19260,-26509,-19260,-26509,-19260,-26509,-19260,-26509,-19260,-26509,-19260,-26509,-19260};
const static int16_t W35s[16]__attribute__((aligned(32))) = {-26510,19260,-26510,19260,-26510,19260,-26510,19260,-26510,19260,-26510,19260,-26510,19260,-26510,19260};
const static int16_t W45s[16]__attribute__((aligned(32))) = {10126,31163,10126,31163,10126,31163,10126,31163,10126,31163,10126,31163,10126,31163,10126,31163};

#if defined(__x86_64__) || defined(__i386__)
const __m128i *W0 = (__m128i *)W0s;
const __m128i *W13 = (__m128i *)W13s;
const __m128i *W23 = (__m128i *)W23s;
const __m128i *W15 = (__m128i *)W15s;
const __m128i *W25 = (__m128i *)W25s;
const __m128i *W35 = (__m128i *)W35s;
const __m128i *W45 = (__m128i *)W45s;

#ifdef __AVX2__
const __m256i *W0_256 =  (__m256i *)W0s;
const __m256i *W13_256 = (__m256i *)W13s;
const __m256i *W23_256 = (__m256i *)W23s;
const __m256i *W15_256 = (__m256i *)W15s;
const __m256i *W25_256 = (__m256i *)W25s;
const __m256i *W35_256 = (__m256i *)W35s;
const __m256i *W45_256 = (__m256i *)W45s;
#endif

#elif defined(__arm__)
int16x8_t *W0  = (int16x8_t *)W0s;
int16x8_t *W13 = (int16x8_t *)W13s;
int16x8_t *W23 = (int16x8_t *)W23s;
int16x8_t *W15 = (int16x8_t *)W15s;
int16x8_t *W25 = (int16x8_t *)W25s;
int16x8_t *W35 = (int16x8_t *)W35s;
int16x8_t *W45 = (int16x8_t *)W45s;
#endif
const static int16_t dft_norm_table[16] = {9459,  //12
					   6689,//24
					   5461,//36
					   4729,//482
					   4230,//60
					   23170,//72
					   3344,//96
					   3153,//108
					   2991,//120
					   18918,//sqrt(3),//144
					   18918,//sqrt(3),//180
					   16384,//2, //192
					   18918,//sqrt(3), // 216
					   16384,//2, //240
					   18918,//sqrt(3), // 288
					   14654
}; //sqrt(5) //300


#if defined(__x86_64__) || defined(__i386__)
static inline void bfly2(__m128i *x0, __m128i *x1,__m128i *y0, __m128i *y1,__m128i *tw)__attribute__((always_inline));

static inline void bfly2(__m128i *x0, __m128i *x1,__m128i *y0, __m128i *y1,__m128i *tw)
{

  __m128i x0r_2,x0i_2,x1r_2,x1i_2,dy0r,dy1r,dy0i,dy1i;
  __m128i bfly2_tmp1,bfly2_tmp2;

  cmult(*(x0),*(W0),&x0r_2,&x0i_2);
  cmult(*(x1),*(tw),&x1r_2,&x1i_2);

  dy0r = _mm_srai_epi32(_mm_add_epi32(x0r_2,x1r_2),15);
  dy1r = _mm_srai_epi32(_mm_sub_epi32(x0r_2,x1r_2),15);
  dy0i = _mm_srai_epi32(_mm_add_epi32(x0i_2,x1i_2),15);
  //  printf("y0i %d\n",((int16_t *)y0i)[0]);
  dy1i = _mm_srai_epi32(_mm_sub_epi32(x0i_2,x1i_2),15);

  bfly2_tmp1 = _mm_unpacklo_epi32(dy0r,dy0i);
  bfly2_tmp2 = _mm_unpackhi_epi32(dy0r,dy0i);
  *y0 = _mm_packs_epi32(bfly2_tmp1,bfly2_tmp2);

  bfly2_tmp1 = _mm_unpacklo_epi32(dy1r,dy1i);
  bfly2_tmp2 = _mm_unpackhi_epi32(dy1r,dy1i);
  *y1 = _mm_packs_epi32(bfly2_tmp1,bfly2_tmp2);
}

#ifdef __AVX2__

static inline void bfly2_256(__m256i *x0, __m256i *x1,__m256i *y0, __m256i *y1,__m256i *tw)__attribute__((always_inline));

static inline void bfly2_256(__m256i *x0, __m256i *x1,__m256i *y0, __m256i *y1,__m256i *tw)
{

  __m256i x0r_2,x0i_2,x1r_2,x1i_2,dy0r,dy1r,dy0i,dy1i;
  __m256i bfly2_tmp1,bfly2_tmp2;

  cmult_256(*(x0),*(W0_256),&x0r_2,&x0i_2);
  cmult_256(*(x1),*(tw),&x1r_2,&x1i_2);

  dy0r = _mm256_srai_epi32(_mm256_add_epi32(x0r_2,x1r_2),15);
  dy1r = _mm256_srai_epi32(_mm256_sub_epi32(x0r_2,x1r_2),15);
  dy0i = _mm256_srai_epi32(_mm256_add_epi32(x0i_2,x1i_2),15);
  //  printf("y0i %d\n",((int16_t *)y0i)[0]);
  dy1i = _mm256_srai_epi32(_mm256_sub_epi32(x0i_2,x1i_2),15);

  bfly2_tmp1 = _mm256_unpacklo_epi32(dy0r,dy0i);
  bfly2_tmp2 = _mm256_unpackhi_epi32(dy0r,dy0i);
  *y0 = _mm256_packs_epi32(bfly2_tmp1,bfly2_tmp2);

  bfly2_tmp1 = _mm256_unpacklo_epi32(dy1r,dy1i);
  bfly2_tmp2 = _mm256_unpackhi_epi32(dy1r,dy1i);
  *y1 = _mm256_packs_epi32(bfly2_tmp1,bfly2_tmp2);
}

#endif

#elif defined(__arm__)

static inline void bfly2(int16x8_t *x0, int16x8_t *x1,int16x8_t *y0, int16x8_t *y1,int16x8_t *tw)__attribute__((always_inline));

static inline void bfly2(int16x8_t *x0, int16x8_t *x1,int16x8_t *y0, int16x8_t *y1,int16x8_t *tw)
{

  int32x4_t x0r_2,x0i_2,x1r_2,x1i_2,dy0r,dy1r,dy0i,dy1i;

  cmult(*(x0),*(W0),&x0r_2,&x0i_2);
  cmult(*(x1),*(tw),&x1r_2,&x1i_2);

  dy0r = vqaddq_s32(x0r_2,x1r_2);
  dy1r = vqsubq_s32(x0r_2,x1r_2);
  dy0i = vqaddq_s32(x0i_2,x1i_2);
  dy1i = vqsubq_s32(x0i_2,x1i_2);

  *y0 = cpack(dy0r,dy0i);
  *y1 = cpack(dy1r,dy1i);
}


#endif

#if defined(__x86_64__) || defined(__i386__)
static inline void bfly2_tw1(__m128i *x0, __m128i *x1, __m128i *y0, __m128i *y1)__attribute__((always_inline));

static inline void bfly2_tw1(__m128i *x0, __m128i *x1, __m128i *y0, __m128i *y1)
{

  *y0  = _mm_adds_epi16(*x0,*x1);
  *y1  = _mm_subs_epi16(*x0,*x1);

}

#elif defined(__arm__)

static inline void bfly2_tw1(int16x8_t *x0, int16x8_t *x1, int16x8_t *y0, int16x8_t *y1)__attribute__((always_inline));

static inline void bfly2_tw1(int16x8_t *x0, int16x8_t *x1, int16x8_t *y0, int16x8_t *y1)
{

  *y0  = vqaddq_s16(*x0,*x1);
  *y1  = vqsubq_s16(*x0,*x1);

}
#endif
 
#if defined(__x86_64__) || defined(__i386__)



static inline void bfly2_16(__m128i *x0, __m128i *x1, __m128i *y0, __m128i *y1, __m128i *tw, __m128i *twb)__attribute__((always_inline));

static inline void bfly2_16(__m128i *x0, __m128i *x1, __m128i *y0, __m128i *y1, __m128i *tw, __m128i *twb)
{

  //  register __m128i x1t;
  __m128i x1t;

  x1t = packed_cmult2(*(x1),*(tw),*(twb));
  /*
  print_shorts("x0",(int16_t*)x0);
  print_shorts("x1",(int16_t*)x1);
  print_shorts("tw",(int16_t*)tw);
  print_shorts("twb",(int16_t*)twb);
  print_shorts("x1t",(int16_t*)&x1t);*/
  *y0  = _mm_adds_epi16(*x0,x1t);
  *y1  = _mm_subs_epi16(*x0,x1t);
  /*  print_shorts("y0",(int16_t*)y0);
      print_shorts("y1",(int16_t*)y1);*/
}

#ifdef __AVX2__

static inline void bfly2_16_256(__m256i *x0, __m256i *x1, __m256i *y0, __m256i *y1, __m256i *tw, __m256i *twb)__attribute__((always_inline));

static inline void bfly2_16_256(__m256i *x0, __m256i *x1, __m256i *y0, __m256i *y1, __m256i *tw, __m256i *twb)
{

  //  register __m256i x1t;
  __m256i x1t;

  x1t = packed_cmult2_256(*(x1),*(tw),*(twb));
  /*
  print_shorts256("x0",(int16_t*)x0);
  print_shorts256("x1",(int16_t*)x1);
  print_shorts256("tw",(int16_t*)tw);
  print_shorts256("twb",(int16_t*)twb);
  print_shorts256("x1t",(int16_t*)&x1t);*/
  *y0  = _mm256_adds_epi16(*x0,x1t);
  *y1  = _mm256_subs_epi16(*x0,x1t);
  
  /*print_shorts256("y0",(int16_t*)y0);
    print_shorts256("y1",(int16_t*)y1);*/
}
#endif


#elif defined(__arm__)

static inline void bfly2_16(int16x8_t *x0, int16x8_t *x1, int16x8_t *y0, int16x8_t *y1, int16x8_t *tw, int16x8_t *twb)__attribute__((always_inline));

static inline void bfly2_16(int16x8_t *x0, int16x8_t *x1, int16x8_t *y0, int16x8_t *y1, int16x8_t *tw, int16x8_t *twb)
{

  *y0  = vqaddq_s16(*x0,*x1);
  *y1  = vqsubq_s16(*x0,*x1);

}
#endif

#if defined(__x86_64__) || defined(__i386__)
static inline void ibfly2(__m128i *x0, __m128i *x1,__m128i *y0, __m128i *y1,__m128i *tw)__attribute__((always_inline));

static inline void ibfly2(__m128i *x0, __m128i *x1,__m128i *y0, __m128i *y1,__m128i *tw)
{

  __m128i x0r_2,x0i_2,x1r_2,x1i_2,dy0r,dy1r,dy0i,dy1i;
  __m128i bfly2_tmp1,bfly2_tmp2;

  cmultc(*(x0),*(W0),&x0r_2,&x0i_2);
  cmultc(*(x1),*(tw),&x1r_2,&x1i_2);

  dy0r = _mm_srai_epi32(_mm_add_epi32(x0r_2,x1r_2),15);
  dy1r = _mm_srai_epi32(_mm_sub_epi32(x0r_2,x1r_2),15);
  dy0i = _mm_srai_epi32(_mm_add_epi32(x0i_2,x1i_2),15);
  //  printf("y0i %d\n",((int16_t *)y0i)[0]);
  dy1i = _mm_srai_epi32(_mm_sub_epi32(x0i_2,x1i_2),15);

  bfly2_tmp1 = _mm_unpacklo_epi32(dy0r,dy0i);
  bfly2_tmp2 = _mm_unpackhi_epi32(dy0r,dy0i);
  *y0 = _mm_packs_epi32(bfly2_tmp1,bfly2_tmp2);

  bfly2_tmp1 = _mm_unpacklo_epi32(dy1r,dy1i);
  bfly2_tmp2 = _mm_unpackhi_epi32(dy1r,dy1i);
  *y1 = _mm_packs_epi32(bfly2_tmp1,bfly2_tmp2);
}

#ifdef __AVX2__
static inline void ibfly2_256(__m256i *x0, __m256i *x1,__m256i *y0, __m256i *y1,__m256i *tw)__attribute__((always_inline));

static inline void ibfly2_256(__m256i *x0, __m256i *x1,__m256i *y0, __m256i *y1,__m256i *tw)
{

  __m256i x0r_2,x0i_2,x1r_2,x1i_2,dy0r,dy1r,dy0i,dy1i;
  __m256i bfly2_tmp1,bfly2_tmp2;

  cmultc_256(*(x0),*(W0_256),&x0r_2,&x0i_2);
  cmultc_256(*(x1),*(tw),&x1r_2,&x1i_2);

  dy0r = _mm256_srai_epi32(_mm256_add_epi32(x0r_2,x1r_2),15);
  dy1r = _mm256_srai_epi32(_mm256_sub_epi32(x0r_2,x1r_2),15);
  dy0i = _mm256_srai_epi32(_mm256_add_epi32(x0i_2,x1i_2),15);
  //  printf("y0i %d\n",((int16_t *)y0i)[0]);
  dy1i = _mm256_srai_epi32(_mm256_sub_epi32(x0i_2,x1i_2),15);

  bfly2_tmp1 = _mm256_unpacklo_epi32(dy0r,dy0i);
  bfly2_tmp2 = _mm256_unpackhi_epi32(dy0r,dy0i);
  *y0 = _mm256_packs_epi32(bfly2_tmp1,bfly2_tmp2);

  bfly2_tmp1 = _mm256_unpacklo_epi32(dy1r,dy1i);
  bfly2_tmp2 = _mm256_unpackhi_epi32(dy1r,dy1i);
  *y1 = _mm256_packs_epi32(bfly2_tmp1,bfly2_tmp2);
}
#endif

#elif defined(__arm__)
static inline void ibfly2(int16x8_t *x0, int16x8_t *x1,int16x8_t *y0, int16x8_t *y1,int16x8_t *tw)
{

  int32x4_t x0r_2,x0i_2,x1r_2,x1i_2,dy0r,dy1r,dy0i,dy1i;

  cmultc(*(x0),*(W0),&x0r_2,&x0i_2);
  cmultc(*(x1),*(tw),&x1r_2,&x1i_2);

  dy0r = vqaddq_s32(x0r_2,x1r_2);
  dy1r = vqsubq_s32(x0r_2,x1r_2);
  dy0i = vqaddq_s32(x0i_2,x1i_2);
  dy1i = vqsubq_s32(x0i_2,x1i_2);

  *y0 = cpack(dy0r,dy0i);
  *y1 = cpack(dy1r,dy1i);

}

#endif




// This is the radix-3 butterfly (fft)

#if defined(__x86_64__) || defined(__i386__)

static inline void bfly3(__m128i *x0,__m128i *x1,__m128i *x2,
                         __m128i *y0,__m128i *y1,__m128i *y2,
                         __m128i *tw1,__m128i *tw2) __attribute__((always_inline));

static inline void bfly3(__m128i *x0,__m128i *x1,__m128i *x2,
                         __m128i *y0,__m128i *y1,__m128i *y2,
                         __m128i *tw1,__m128i *tw2)
{

  __m128i tmpre,tmpim,x1_2,x2_2;

  packed_cmult(*(x1),*(tw1),&x1_2);
  packed_cmult(*(x2),*(tw2),&x2_2);
  *(y0)  = _mm_adds_epi16(*(x0),_mm_adds_epi16(x1_2,x2_2));
  cmult(x1_2,*(W13),&tmpre,&tmpim);
  cmac(x2_2,*(W23),&tmpre,&tmpim);
  *(y1) = cpack(tmpre,tmpim);
  *(y1) = _mm_adds_epi16(*(x0),*(y1));
  cmult(x1_2,*(W23),&tmpre,&tmpim);
  cmac(x2_2,*(W13),&tmpre,&tmpim);
  *(y2) = cpack(tmpre,tmpim);
  *(y2) = _mm_adds_epi16(*(x0),*(y2));
}

#ifdef __AVX2__

static inline void bfly3_256(__m256i *x0,__m256i *x1,__m256i *x2,
			     __m256i *y0,__m256i *y1,__m256i *y2,
			     __m256i *tw1,__m256i *tw2) __attribute__((always_inline));

static inline void bfly3_256(__m256i *x0,__m256i *x1,__m256i *x2,
			     __m256i *y0,__m256i *y1,__m256i *y2,
			     __m256i *tw1,__m256i *tw2)
{ 

  __m256i tmpre,tmpim,x1_2,x2_2;

  packed_cmult_256(*(x1),*(tw1),&x1_2);
  packed_cmult_256(*(x2),*(tw2),&x2_2);
  *(y0)  = _mm256_adds_epi16(*(x0),_mm256_adds_epi16(x1_2,x2_2));
  cmult_256(x1_2,*(W13_256),&tmpre,&tmpim);
  cmac_256(x2_2,*(W23_256),&tmpre,&tmpim);
  *(y1) = cpack_256(tmpre,tmpim);
  *(y1) = _mm256_adds_epi16(*(x0),*(y1));
  cmult_256(x1_2,*(W23_256),&tmpre,&tmpim);
  cmac_256(x2_2,*(W13_256),&tmpre,&tmpim);
  *(y2) = cpack_256(tmpre,tmpim);
  *(y2) = _mm256_adds_epi16(*(x0),*(y2));
}
#endif

#elif defined(__arm__)
static inline void bfly3(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,
                         int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,
                         int16x8_t *tw1,int16x8_t *tw2) __attribute__((always_inline));

static inline void bfly3(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,
                         int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,
                         int16x8_t *tw1,int16x8_t *tw2)
{

  int32x4_t tmpre,tmpim;
  int16x8_t x1_2,x2_2;

  packed_cmult(*(x1),*(tw1),&x1_2);
  packed_cmult(*(x2),*(tw2),&x2_2);
  *(y0)  = vqaddq_s16(*(x0),vqaddq_s16(x1_2,x2_2));
  cmult(x1_2,*(W13),&tmpre,&tmpim);
  cmac(x2_2,*(W23),&tmpre,&tmpim);
  *(y1) = cpack(tmpre,tmpim);
  *(y1) = vqaddq_s16(*(x0),*(y1));
  cmult(x1_2,*(W23),&tmpre,&tmpim);
  cmac(x2_2,*(W13),&tmpre,&tmpim);
  *(y2) = cpack(tmpre,tmpim);
  *(y2) = vqaddq_s16(*(x0),*(y2));
}

#endif

#if defined(__x86_64__) || defined(__i386__)
static inline void ibfly3(__m128i *x0,__m128i *x1,__m128i *x2,
			  __m128i *y0,__m128i *y1,__m128i *y2,
			  __m128i *tw1,__m128i *tw2) __attribute__((always_inline));

static inline void ibfly3(__m128i *x0,__m128i *x1,__m128i *x2,
			  __m128i *y0,__m128i *y1,__m128i *y2,
			  __m128i *tw1,__m128i *tw2)
{

  __m128i tmpre,tmpim,x1_2,x2_2;

  packed_cmultc(*(x1),*(tw1),&x1_2);
  packed_cmultc(*(x2),*(tw2),&x2_2);
  *(y0)  = _mm_adds_epi16(*(x0),_mm_adds_epi16(x1_2,x2_2));
  cmultc(x1_2,*(W13),&tmpre,&tmpim);
  cmacc(x2_2,*(W23),&tmpre,&tmpim);
  *(y1) = cpack(tmpre,tmpim);
  *(y1) = _mm_adds_epi16(*(x0),*(y1));
  cmultc(x1_2,*(W23),&tmpre,&tmpim);
  cmacc(x2_2,*(W13),&tmpre,&tmpim);
  *(y2) = cpack(tmpre,tmpim);
  *(y2) = _mm_adds_epi16(*(x0),*(y2));
}

#ifdef __AVX2__

static inline void ibfly3_256(__m256i *x0,__m256i *x1,__m256i *x2,
			      __m256i *y0,__m256i *y1,__m256i *y2,
			      __m256i *tw1,__m256i *tw2) __attribute__((always_inline));

static inline void ibfly3_256(__m256i *x0,__m256i *x1,__m256i *x2,
			      __m256i *y0,__m256i *y1,__m256i *y2,
			      __m256i *tw1,__m256i *tw2)
{ 

  __m256i tmpre,tmpim,x1_2,x2_2;

  packed_cmultc_256(*(x1),*(tw1),&x1_2);
  packed_cmultc_256(*(x2),*(tw2),&x2_2);
  *(y0)  = _mm256_adds_epi16(*(x0),_mm256_adds_epi16(x1_2,x2_2));
  cmultc_256(x1_2,*(W13_256),&tmpre,&tmpim);
  cmacc_256(x2_2,*(W23_256),&tmpre,&tmpim);
  *(y1) = cpack_256(tmpre,tmpim);
  *(y1) = _mm256_adds_epi16(*(x0),*(y1));
  cmultc_256(x1_2,*(W23_256),&tmpre,&tmpim);
  cmacc_256(x2_2,*(W13_256),&tmpre,&tmpim);
  *(y2) = cpack_256(tmpre,tmpim);
  *(y2) = _mm256_adds_epi16(*(x0),*(y2));
}
#endif

#elif defined(__arm__)
static inline void ibfly3(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,
			  int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,
			  int16x8_t *tw1,int16x8_t *tw2) __attribute__((always_inline));

static inline void ibfly3(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,
			  int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,
			  int16x8_t *tw1,int16x8_t *tw2)
{

  int32x4_t tmpre,tmpim;
  int16x8_t x1_2,x2_2;

  packed_cmultc(*(x1),*(tw1),&x1_2);
  packed_cmultc(*(x2),*(tw2),&x2_2);
  *(y0)  = vqaddq_s16(*(x0),vqaddq_s16(x1_2,x2_2));
  cmultc(x1_2,*(W13),&tmpre,&tmpim);
  cmacc(x2_2,*(W23),&tmpre,&tmpim);
  *(y1) = cpack(tmpre,tmpim);
  *(y1) = vqaddq_s16(*(x0),*(y1));
  cmultc(x1_2,*(W23),&tmpre,&tmpim);
  cmacc(x2_2,*(W13),&tmpre,&tmpim);
  *(y2) = cpack(tmpre,tmpim);
  *(y2) = vqaddq_s16(*(x0),*(y2));
}
#endif

#if defined(__x86_64__) || defined(__i386__)
static inline void bfly3_tw1(__m128i *x0,__m128i *x1,__m128i *x2,
                             __m128i *y0,__m128i *y1,__m128i *y2) __attribute__((always_inline));

static inline void bfly3_tw1(__m128i *x0,__m128i *x1,__m128i *x2,
                             __m128i *y0,__m128i *y1,__m128i *y2)
{

  __m128i tmpre,tmpim;

  *(y0) = _mm_adds_epi16(*(x0),_mm_adds_epi16(*(x1),*(x2)));
  cmult(*(x1),*(W13),&tmpre,&tmpim);
  cmac(*(x2),*(W23),&tmpre,&tmpim);
  *(y1) = cpack(tmpre,tmpim);
  *(y1) = _mm_adds_epi16(*(x0),*(y1));
  cmult(*(x1),*(W23),&tmpre,&tmpim);
  cmac(*(x2),*(W13),&tmpre,&tmpim);
  *(y2) = cpack(tmpre,tmpim);
  *(y2) = _mm_adds_epi16(*(x0),*(y2));
}

#ifdef __AVX2__

static inline void bfly3_tw1_256(__m256i *x0,__m256i *x1,__m256i *x2,
				 __m256i *y0,__m256i *y1,__m256i *y2) __attribute__((always_inline));

static inline void bfly3_tw1_256(__m256i *x0,__m256i *x1,__m256i *x2,
				 __m256i *y0,__m256i *y1,__m256i *y2)
{

  __m256i tmpre,tmpim;

  *(y0) = _mm256_adds_epi16(*(x0),_mm256_adds_epi16(*(x1),*(x2)));
  cmult_256(*(x1),*(W13_256),&tmpre,&tmpim);
  cmac_256(*(x2),*(W23_256),&tmpre,&tmpim);
  *(y1) = cpack_256(tmpre,tmpim);
  *(y1) = _mm256_adds_epi16(*(x0),*(y1));
  cmult_256(*(x1),*(W23_256),&tmpre,&tmpim);
  cmac_256(*(x2),*(W13_256),&tmpre,&tmpim);
  *(y2) = cpack_256(tmpre,tmpim);
  *(y2) = _mm256_adds_epi16(*(x0),*(y2));
}
#endif

#elif defined(__arm__)
static inline void bfly3_tw1(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,
                             int16x8_t *y0,int16x8_t *y1,int16x8_t *y2) __attribute__((always_inline));

static inline void bfly3_tw1(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,
                             int16x8_t *y0,int16x8_t *y1,int16x8_t *y2)
{

  int32x4_t tmpre,tmpim;

  *(y0) = vqaddq_s16(*(x0),vqaddq_s16(*(x1),*(x2)));
  cmult(*(x1),*(W13),&tmpre,&tmpim);
  cmac(*(x2),*(W23),&tmpre,&tmpim);
  *(y1) = cpack(tmpre,tmpim);
  *(y1) = vqaddq_s16(*(x0),*(y1));
  cmult(*(x1),*(W23),&tmpre,&tmpim);
  cmac(*(x2),*(W13),&tmpre,&tmpim);
  *(y2) = cpack(tmpre,tmpim);
  *(y2) = vqaddq_s16(*(x0),*(y2));

}

#endif

#if defined(__x86_64__) || defined(__i386__)
static inline void bfly4(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                         __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3,
                         __m128i *tw1,__m128i *tw2,__m128i *tw3)__attribute__((always_inline));

static inline void bfly4(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                         __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3,
                         __m128i *tw1,__m128i *tw2,__m128i *tw3)
{

  __m128i x1r_2,x1i_2,x2r_2,x2i_2,x3r_2,x3i_2,dy0r,dy0i,dy1r,dy1i,dy2r,dy2i,dy3r,dy3i;

  //  cmult(*(x0),*(W0),&x0r_2,&x0i_2);
  cmult(*(x1),*(tw1),&x1r_2,&x1i_2);
  cmult(*(x2),*(tw2),&x2r_2,&x2i_2);
  cmult(*(x3),*(tw3),&x3r_2,&x3i_2);
  //  dy0r = _mm_add_epi32(x0r_2,_mm_add_epi32(x1r_2,_mm_add_epi32(x2r_2,x3r_2)));
  //  dy0i = _mm_add_epi32(x0i_2,_mm_add_epi32(x1i_2,_mm_add_epi32(x2i_2,x3i_2)));
  //  *(y0)  = cpack(dy0r,dy0i);
  dy0r = _mm_add_epi32(x1r_2,_mm_add_epi32(x2r_2,x3r_2));
  dy0i = _mm_add_epi32(x1i_2,_mm_add_epi32(x2i_2,x3i_2));
  *(y0)  = _mm_add_epi16(*(x0),cpack(dy0r,dy0i));
  //  dy1r = _mm_add_epi32(x0r_2,_mm_sub_epi32(x1i_2,_mm_add_epi32(x2r_2,x3i_2)));
  //  dy1i = _mm_sub_epi32(x0i_2,_mm_add_epi32(x1r_2,_mm_sub_epi32(x2i_2,x3r_2)));
  //  *(y1)  = cpack(dy1r,dy1i);
  dy1r = _mm_sub_epi32(x1i_2,_mm_add_epi32(x2r_2,x3i_2));
  dy1i = _mm_sub_epi32(_mm_sub_epi32(x3r_2,x2i_2),x1r_2);
  *(y1)  = _mm_add_epi16(*(x0),cpack(dy1r,dy1i));
  //  dy2r = _mm_sub_epi32(x0r_2,_mm_sub_epi32(x1r_2,_mm_sub_epi32(x2r_2,x3r_2)));
  //  dy2i = _mm_sub_epi32(x0i_2,_mm_sub_epi32(x1i_2,_mm_sub_epi32(x2i_2,x3i_2)));
  //  *(y2)  = cpack(dy2r,dy2i);
  dy2r = _mm_sub_epi32(_mm_sub_epi32(x2r_2,x3r_2),x1r_2);
  dy2i = _mm_sub_epi32(_mm_sub_epi32(x2i_2,x3i_2),x1i_2);
  *(y2)  = _mm_add_epi16(*(x0),cpack(dy2r,dy2i));
  //  dy3r = _mm_sub_epi32(x0r_2,_mm_add_epi32(x1i_2,_mm_sub_epi32(x2r_2,x3i_2)));
  //  dy3i = _mm_add_epi32(x0i_2,_mm_sub_epi32(x1r_2,_mm_add_epi32(x2i_2,x3r_2)));
  //  *(y3) = cpack(dy3r,dy3i);
  dy3r = _mm_sub_epi32(_mm_sub_epi32(x3i_2,x2r_2),x1i_2);
  dy3i = _mm_sub_epi32(x1r_2,_mm_add_epi32(x2i_2,x3r_2));
  *(y3) = _mm_add_epi16(*(x0),cpack(dy3r,dy3i));
}

#ifdef __AVX2__
static inline void bfly4_256(__m256i *x0,__m256i *x1,__m256i *x2,__m256i *x3,
			     __m256i *y0,__m256i *y1,__m256i *y2,__m256i *y3,
			     __m256i *tw1,__m256i *tw2,__m256i *tw3)__attribute__((always_inline));

static inline void bfly4_256(__m256i *x0,__m256i *x1,__m256i *x2,__m256i *x3,
			     __m256i *y0,__m256i *y1,__m256i *y2,__m256i *y3,
			     __m256i *tw1,__m256i *tw2,__m256i *tw3)
{

  __m256i x1r_2,x1i_2,x2r_2,x2i_2,x3r_2,x3i_2,dy0r,dy0i,dy1r,dy1i,dy2r,dy2i,dy3r,dy3i;

  //  cmult(*(x0),*(W0),&x0r_2,&x0i_2);
  cmult_256(*(x1),*(tw1),&x1r_2,&x1i_2);
  cmult_256(*(x2),*(tw2),&x2r_2,&x2i_2);
  cmult_256(*(x3),*(tw3),&x3r_2,&x3i_2);
  //  dy0r = _mm_add_epi32(x0r_2,_mm_add_epi32(x1r_2,_mm_add_epi32(x2r_2,x3r_2)));
  //  dy0i = _mm_add_epi32(x0i_2,_mm_add_epi32(x1i_2,_mm_add_epi32(x2i_2,x3i_2)));
  //  *(y0)  = cpack(dy0r,dy0i);
  dy0r = _mm256_add_epi32(x1r_2,_mm256_add_epi32(x2r_2,x3r_2));
  dy0i = _mm256_add_epi32(x1i_2,_mm256_add_epi32(x2i_2,x3i_2));
  *(y0)  = _mm256_add_epi16(*(x0),cpack_256(dy0r,dy0i));
  //  dy1r = _mm_add_epi32(x0r_2,_mm_sub_epi32(x1i_2,_mm_add_epi32(x2r_2,x3i_2)));
  //  dy1i = _mm_sub_epi32(x0i_2,_mm_add_epi32(x1r_2,_mm_sub_epi32(x2i_2,x3r_2)));
  //  *(y1)  = cpack(dy1r,dy1i);
  dy1r = _mm256_sub_epi32(x1i_2,_mm256_add_epi32(x2r_2,x3i_2));
  dy1i = _mm256_sub_epi32(_mm256_sub_epi32(x3r_2,x2i_2),x1r_2);
  *(y1)  = _mm256_add_epi16(*(x0),cpack_256(dy1r,dy1i));
  //  dy2r = _mm_sub_epi32(x0r_2,_mm_sub_epi32(x1r_2,_mm_sub_epi32(x2r_2,x3r_2)));
  //  dy2i = _mm_sub_epi32(x0i_2,_mm_sub_epi32(x1i_2,_mm_sub_epi32(x2i_2,x3i_2)));
  //  *(y2)  = cpack(dy2r,dy2i);
  dy2r = _mm256_sub_epi32(_mm256_sub_epi32(x2r_2,x3r_2),x1r_2);
  dy2i = _mm256_sub_epi32(_mm256_sub_epi32(x2i_2,x3i_2),x1i_2);
  *(y2)  = _mm256_add_epi16(*(x0),cpack_256(dy2r,dy2i));
  //  dy3r = _mm_sub_epi32(x0r_2,_mm_add_epi32(x1i_2,_mm_sub_epi32(x2r_2,x3i_2)));
  //  dy3i = _mm_add_epi32(x0i_2,_mm_sub_epi32(x1r_2,_mm_add_epi32(x2i_2,x3r_2)));
  //  *(y3) = cpack(dy3r,dy3i);
  dy3r = _mm256_sub_epi32(_mm256_sub_epi32(x3i_2,x2r_2),x1i_2);
  dy3i = _mm256_sub_epi32(x1r_2,_mm256_add_epi32(x2i_2,x3r_2));
  *(y3) = _mm256_add_epi16(*(x0),cpack_256(dy3r,dy3i));
}
#endif
#elif defined(__arm__)
static inline void bfly4(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
                         int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3,
                         int16x8_t *tw1,int16x8_t *tw2,int16x8_t *tw3)__attribute__((always_inline));

static inline void bfly4(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
                         int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3,
                         int16x8_t *tw1,int16x8_t *tw2,int16x8_t *tw3)
{

  int32x4_t x1r_2,x1i_2,x2r_2,x2i_2,x3r_2,x3i_2,dy0r,dy0i,dy1r,dy1i,dy2r,dy2i,dy3r,dy3i;

  //  cmult(*(x0),*(W0),&x0r_2,&x0i_2);
  cmult(*(x1),*(tw1),&x1r_2,&x1i_2);
  cmult(*(x2),*(tw2),&x2r_2,&x2i_2);
  cmult(*(x3),*(tw3),&x3r_2,&x3i_2);
  //  dy0r = _mm_add_epi32(x0r_2,_mm_add_epi32(x1r_2,_mm_add_epi32(x2r_2,x3r_2)));
  //  dy0i = _mm_add_epi32(x0i_2,_mm_add_epi32(x1i_2,_mm_add_epi32(x2i_2,x3i_2)));
  //  *(y0)  = cpack(dy0r,dy0i);
  dy0r = vqaddq_s32(x1r_2,vqaddq_s32(x2r_2,x3r_2));
  dy0i = vqaddq_s32(x1i_2,vqaddq_s32(x2i_2,x3i_2));
  *(y0)  = vqaddq_s16(*(x0),cpack(dy0r,dy0i));
  //  dy1r = _mm_add_epi32(x0r_2,_mm_sub_epi32(x1i_2,_mm_add_epi32(x2r_2,x3i_2)));
  //  dy1i = _mm_sub_epi32(x0i_2,_mm_add_epi32(x1r_2,_mm_sub_epi32(x2i_2,x3r_2)));
  //  *(y1)  = cpack(dy1r,dy1i);
  dy1r = vqsubq_s32(x1i_2,vqaddq_s32(x2r_2,x3i_2));
  dy1i = vqsubq_s32(vqsubq_s32(x3r_2,x2i_2),x1r_2);
  *(y1)  = vqaddq_s16(*(x0),cpack(dy1r,dy1i));
  //  dy2r = _mm_sub_epi32(x0r_2,_mm_sub_epi32(x1r_2,_mm_sub_epi32(x2r_2,x3r_2)));
  //  dy2i = _mm_sub_epi32(x0i_2,_mm_sub_epi32(x1i_2,_mm_sub_epi32(x2i_2,x3i_2)));
  //  *(y2)  = cpack(dy2r,dy2i);
  dy2r = vqsubq_s32(vqsubq_s32(x2r_2,x3r_2),x1r_2);
  dy2i = vqsubq_s32(vqsubq_s32(x2i_2,x3i_2),x1i_2);
  *(y2)  = vqaddq_s16(*(x0),cpack(dy2r,dy2i));
  //  dy3r = _mm_sub_epi32(x0r_2,_mm_add_epi32(x1i_2,_mm_sub_epi32(x2r_2,x3i_2)));
  //  dy3i = _mm_add_epi32(x0i_2,_mm_sub_epi32(x1r_2,_mm_add_epi32(x2i_2,x3r_2)));
  //  *(y3) = cpack(dy3r,dy3i);
  dy3r = vqsubq_s32(vqsubq_s32(x3i_2,x2r_2),x1i_2);
  dy3i = vqsubq_s32(x1r_2,vqaddq_s32(x2i_2,x3r_2));
  *(y3) = vqaddq_s16(*(x0),cpack(dy3r,dy3i));
}

#endif

#if defined(__x86_64__) || defined(__i386__)
static inline void ibfly4(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                          __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3,
                          __m128i *tw1,__m128i *tw2,__m128i *tw3)__attribute__((always_inline));

static inline void ibfly4(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                          __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3,
                          __m128i *tw1,__m128i *tw2,__m128i *tw3)
{

  __m128i x1r_2,x1i_2,x2r_2,x2i_2,x3r_2,x3i_2,dy0r,dy0i,dy1r,dy1i,dy2r,dy2i,dy3r,dy3i;


  cmultc(*(x1),*(tw1),&x1r_2,&x1i_2);
  cmultc(*(x2),*(tw2),&x2r_2,&x2i_2);
  cmultc(*(x3),*(tw3),&x3r_2,&x3i_2);

  dy0r = _mm_add_epi32(x1r_2,_mm_add_epi32(x2r_2,x3r_2));
  dy0i = _mm_add_epi32(x1i_2,_mm_add_epi32(x2i_2,x3i_2));
  *(y0)  = _mm_add_epi16(*(x0),cpack(dy0r,dy0i));
  dy3r = _mm_sub_epi32(x1i_2,_mm_add_epi32(x2r_2,x3i_2));
  dy3i = _mm_sub_epi32(_mm_sub_epi32(x3r_2,x2i_2),x1r_2);
  *(y3)  = _mm_add_epi16(*(x0),cpack(dy3r,dy3i));
  dy2r = _mm_sub_epi32(_mm_sub_epi32(x2r_2,x3r_2),x1r_2);
  dy2i = _mm_sub_epi32(_mm_sub_epi32(x2i_2,x3i_2),x1i_2);
  *(y2)  = _mm_add_epi16(*(x0),cpack(dy2r,dy2i));
  dy1r = _mm_sub_epi32(_mm_sub_epi32(x3i_2,x2r_2),x1i_2);
  dy1i = _mm_sub_epi32(x1r_2,_mm_add_epi32(x2i_2,x3r_2));
  *(y1) = _mm_add_epi16(*(x0),cpack(dy1r,dy1i));
}

#ifdef __AVX2__

static inline void ibfly4_256(__m256i *x0,__m256i *x1,__m256i *x2,__m256i *x3,
			      __m256i *y0,__m256i *y1,__m256i *y2,__m256i *y3,
			      __m256i *tw1,__m256i *tw2,__m256i *tw3)__attribute__((always_inline));

static inline void ibfly4_256(__m256i *x0,__m256i *x1,__m256i *x2,__m256i *x3,
			      __m256i *y0,__m256i *y1,__m256i *y2,__m256i *y3,
			      __m256i *tw1,__m256i *tw2,__m256i *tw3)
{

  __m256i x1r_2,x1i_2,x2r_2,x2i_2,x3r_2,x3i_2,dy0r,dy0i,dy1r,dy1i,dy2r,dy2i,dy3r,dy3i;


  cmultc_256(*(x1),*(tw1),&x1r_2,&x1i_2);
  cmultc_256(*(x2),*(tw2),&x2r_2,&x2i_2);
  cmultc_256(*(x3),*(tw3),&x3r_2,&x3i_2);

  dy0r = _mm256_add_epi32(x1r_2,_mm256_add_epi32(x2r_2,x3r_2));
  dy0i = _mm256_add_epi32(x1i_2,_mm256_add_epi32(x2i_2,x3i_2));
  *(y0)  = _mm256_add_epi16(*(x0),cpack_256(dy0r,dy0i));
  dy3r = _mm256_sub_epi32(x1i_2,_mm256_add_epi32(x2r_2,x3i_2));
  dy3i = _mm256_sub_epi32(_mm256_sub_epi32(x3r_2,x2i_2),x1r_2);
  *(y3)  = _mm256_add_epi16(*(x0),cpack_256(dy3r,dy3i));
  dy2r = _mm256_sub_epi32(_mm256_sub_epi32(x2r_2,x3r_2),x1r_2);
  dy2i = _mm256_sub_epi32(_mm256_sub_epi32(x2i_2,x3i_2),x1i_2);
  *(y2)  = _mm256_add_epi16(*(x0),cpack_256(dy2r,dy2i));
  dy1r = _mm256_sub_epi32(_mm256_sub_epi32(x3i_2,x2r_2),x1i_2);
  dy1i = _mm256_sub_epi32(x1r_2,_mm256_add_epi32(x2i_2,x3r_2));
  *(y1) = _mm256_add_epi16(*(x0),cpack_256(dy1r,dy1i));
}

#endif
#elif defined(__arm__)

static inline void ibfly4(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
                          int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3,
                          int16x8_t *tw1,int16x8_t *tw2,int16x8_t *tw3)__attribute__((always_inline));

static inline void ibfly4(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
                          int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3,
                          int16x8_t *tw1,int16x8_t *tw2,int16x8_t *tw3)
{

  int32x4_t x1r_2,x1i_2,x2r_2,x2i_2,x3r_2,x3i_2,dy0r,dy0i,dy1r,dy1i,dy2r,dy2i,dy3r,dy3i;


  cmultc(*(x1),*(tw1),&x1r_2,&x1i_2);
  cmultc(*(x2),*(tw2),&x2r_2,&x2i_2);
  cmultc(*(x3),*(tw3),&x3r_2,&x3i_2);

  dy0r  = vqaddq_s32(x1r_2,vqaddq_s32(x2r_2,x3r_2));
  dy0i  = vqaddq_s32(x1i_2,vqaddq_s32(x2i_2,x3i_2));
  *(y0) = vqaddq_s16(*(x0),cpack(dy0r,dy0i));
  dy3r  = vqsubq_s32(x1i_2,vqaddq_s32(x2r_2,x3i_2));
  dy3i  = vqsubq_s32(vqsubq_s32(x3r_2,x2i_2),x1r_2);
  *(y3) = vqaddq_s16(*(x0),cpack(dy3r,dy3i));
  dy2r  = vqsubq_s32(vqsubq_s32(x2r_2,x3r_2),x1r_2);
  dy2i  = vqsubq_s32(vqsubq_s32(x2i_2,x3i_2),x1i_2);
  *(y2) = vqaddq_s16(*(x0),cpack(dy2r,dy2i));
  dy1r  = vqsubq_s32(vqsubq_s32(x3i_2,x2r_2),x1i_2);
  dy1i  = vqsubq_s32(x1r_2,vqaddq_s32(x2i_2,x3r_2));
  *(y1) = vqaddq_s16(*(x0),cpack(dy1r,dy1i));
}

#endif

#if defined(__x86_64__) || defined(__i386__)

static inline void bfly4_tw1(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                             __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3)__attribute__((always_inline));

static inline void bfly4_tw1(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                             __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3)
{
  register __m128i x1_flip,x3_flip,x02t,x13t;
  register __m128i complex_shuffle = _mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  x02t    = _mm_adds_epi16(*(x0),*(x2));
  x13t    = _mm_adds_epi16(*(x1),*(x3));
  *(y0)   = _mm_adds_epi16(x02t,x13t);
  *(y2)   = _mm_subs_epi16(x02t,x13t);
  x1_flip = _mm_sign_epi16(*(x1),*(__m128i*)conjugatedft);
  x1_flip = _mm_shuffle_epi8(x1_flip,complex_shuffle);
  x3_flip = _mm_sign_epi16(*(x3),*(__m128i*)conjugatedft);
  x3_flip = _mm_shuffle_epi8(x3_flip,complex_shuffle);
  x02t    = _mm_subs_epi16(*(x0),*(x2));
  x13t    = _mm_subs_epi16(x1_flip,x3_flip);
  *(y1)   = _mm_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  *(y3)   = _mm_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

  /*
  *(y0) = _mm_adds_epi16(*(x0),_mm_adds_epi16(*(x1),_mm_adds_epi16(*(x2),*(x3))));
  x1_flip = _mm_sign_epi16(*(x1),*(__m128i*)conjugatedft);
  x1_flip = _mm_shuffle_epi8(x1_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x3_flip = _mm_sign_epi16(*(x3),*(__m128i*)conjugatedft);
  x3_flip = _mm_shuffle_epi8(x3_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  *(y1)   = _mm_adds_epi16(*(x0),_mm_subs_epi16(x1_flip,_mm_adds_epi16(*(x2),x3_flip)));
  *(y2)   = _mm_subs_epi16(*(x0),_mm_subs_epi16(*(x1),_mm_subs_epi16(*(x2),*(x3))));
  *(y3)   = _mm_subs_epi16(*(x0),_mm_adds_epi16(x1_flip,_mm_subs_epi16(*(x2),x3_flip)));
  */
}

#ifdef __AVX2__

static inline void bfly4_tw1_256(__m256i *x0,__m256i *x1,__m256i *x2,__m256i *x3,
				 __m256i *y0,__m256i *y1,__m256i *y2,__m256i *y3)__attribute__((always_inline));

static inline void bfly4_tw1_256(__m256i *x0,__m256i *x1,__m256i *x2,__m256i *x3,
				 __m256i *y0,__m256i *y1,__m256i *y2,__m256i *y3)
{
  register __m256i x1_flip,x3_flip,x02t,x13t;
  register __m256i complex_shuffle = _mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  x02t    = _mm256_adds_epi16(*(x0),*(x2));
  x13t    = _mm256_adds_epi16(*(x1),*(x3));
  *(y0)   = _mm256_adds_epi16(x02t,x13t);
  *(y2)   = _mm256_subs_epi16(x02t,x13t);
  x1_flip = _mm256_sign_epi16(*(x1),*(__m256i*)conjugatedft);
  x1_flip = _mm256_shuffle_epi8(x1_flip,complex_shuffle);
  x3_flip = _mm256_sign_epi16(*(x3),*(__m256i*)conjugatedft);
  x3_flip = _mm256_shuffle_epi8(x3_flip,complex_shuffle);
  x02t    = _mm256_subs_epi16(*(x0),*(x2));
  x13t    = _mm256_subs_epi16(x1_flip,x3_flip);
  *(y1)   = _mm256_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  *(y3)   = _mm256_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f
}
#endif

#elif defined(__arm__)

static inline void bfly4_tw1(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
                             int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3)__attribute__((always_inline));

static inline void bfly4_tw1(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
                             int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3)
{

  register int16x8_t x1_flip,x3_flip;

  *(y0) = vqaddq_s16(*(x0),vqaddq_s16(*(x1),vqaddq_s16(*(x2),*(x3))));
  x1_flip = vrev32q_s16(vmulq_s16(*(x1),*(int16x8_t*)conjugatedft));
  x3_flip = vrev32q_s16(vmulq_s16(*(x3),*(int16x8_t*)conjugatedft));
  *(y1)   = vqaddq_s16(*(x0),vqsubq_s16(x1_flip,vqaddq_s16(*(x2),x3_flip)));
  *(y2)   = vqsubq_s16(*(x0),vqsubq_s16(*(x1),vqsubq_s16(*(x2),*(x3))));
  *(y3)   = vqsubq_s16(*(x0),vqaddq_s16(x1_flip,vqsubq_s16(*(x2),x3_flip)));
}

#endif

#if defined(__x86_64__) || defined(__i386__)

static inline void ibfly4_tw1(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                              __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3)__attribute__((always_inline));

static inline void ibfly4_tw1(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                              __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3)
{

  register __m128i x1_flip,x3_flip;

  *(y0) = _mm_adds_epi16(*(x0),_mm_adds_epi16(*(x1),_mm_adds_epi16(*(x2),*(x3))));

  x1_flip = _mm_sign_epi16(*(x1),*(__m128i*)conjugatedft);
  //  x1_flip = _mm_shufflelo_epi16(x1_flip,_MM_SHUFFLE(2,3,0,1));
  //  x1_flip = _mm_shufflehi_epi16(x1_flip,_MM_SHUFFLE(2,3,0,1));
  x1_flip = _mm_shuffle_epi8(x1_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x3_flip = _mm_sign_epi16(*(x3),*(__m128i*)conjugatedft);
  //  x3_flip = _mm_shufflelo_epi16(x3_flip,_MM_SHUFFLE(2,3,0,1));
  //  x3_flip = _mm_shufflehi_epi16(x3_flip,_MM_SHUFFLE(2,3,0,1));
  x3_flip = _mm_shuffle_epi8(x3_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  *(y1)   = _mm_subs_epi16(*(x0),_mm_adds_epi16(x1_flip,_mm_subs_epi16(*(x2),x3_flip)));
  *(y2)   = _mm_subs_epi16(*(x0),_mm_subs_epi16(*(x1),_mm_subs_epi16(*(x2),*(x3))));
  *(y3)   = _mm_adds_epi16(*(x0),_mm_subs_epi16(x1_flip,_mm_adds_epi16(*(x2),x3_flip)));
}


#elif defined(__arm__)
static inline void ibfly4_tw1(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
			      int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3)__attribute__((always_inline));

static inline void ibfly4_tw1(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
			      int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3)
{

  register int16x8_t x1_flip,x3_flip;

  *(y0) = vqaddq_s16(*(x0),vqaddq_s16(*(x1),vqaddq_s16(*(x2),*(x3))));
  x1_flip = vrev32q_s16(vmulq_s16(*(x1),*(int16x8_t*)conjugatedft));
  x3_flip = vrev32q_s16(vmulq_s16(*(x3),*(int16x8_t*)conjugatedft));
  *(y1)   = vqsubq_s16(*(x0),vqaddq_s16(x1_flip,vqsubq_s16(*(x2),x3_flip)));
  *(y2)   = vqsubq_s16(*(x0),vqsubq_s16(*(x1),vqsubq_s16(*(x2),*(x3))));
  *(y3)   = vqaddq_s16(*(x0),vqsubq_s16(x1_flip,vqaddq_s16(*(x2),x3_flip)));
}

#endif

#if defined(__x86_64__) || defined(__i386__)
static inline void bfly4_16(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                            __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3,
                            __m128i *tw1,__m128i *tw2,__m128i *tw3,
                            __m128i *tw1b,__m128i *tw2b,__m128i *tw3b)__attribute__((always_inline));

static inline void bfly4_16(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                            __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3,
                            __m128i *tw1,__m128i *tw2,__m128i *tw3,
                            __m128i *tw1b,__m128i *tw2b,__m128i *tw3b)
{

  register __m128i x1t,x2t,x3t,x02t,x13t;
  register __m128i x1_flip,x3_flip;

  x1t = packed_cmult2(*(x1),*(tw1),*(tw1b));
  x2t = packed_cmult2(*(x2),*(tw2),*(tw2b));
  x3t = packed_cmult2(*(x3),*(tw3),*(tw3b));


  //  bfly4_tw1(x0,&x1t,&x2t,&x3t,y0,y1,y2,y3);
  x02t  = _mm_adds_epi16(*(x0),x2t);
  x13t  = _mm_adds_epi16(x1t,x3t);
  /*
  *(y0) = _mm_adds_epi16(*(x0),_mm_adds_epi16(x1t,_mm_adds_epi16(x2t,x3t)));
  *(y2)   = _mm_subs_epi16(*(x0),_mm_subs_epi16(x1t,_mm_subs_epi16(x2t,x3t)));
  */
  *(y0)   = _mm_adds_epi16(x02t,x13t);
  *(y2)   = _mm_subs_epi16(x02t,x13t);

  x1_flip = _mm_sign_epi16(x1t,*(__m128i*)conjugatedft);
  //  x1_flip = _mm_shufflelo_epi16(x1_flip,_MM_SHUFFLE(2,3,0,1));
  //  x1_flip = _mm_shufflehi_epi16(x1_flip,_MM_SHUFFLE(2,3,0,1));
  x1_flip = _mm_shuffle_epi8(x1_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x3_flip = _mm_sign_epi16(x3t,*(__m128i*)conjugatedft);
  //  x3_flip = _mm_shufflelo_epi16(x3_flip,_MM_SHUFFLE(2,3,0,1));
  //  x3_flip = _mm_shufflehi_epi16(x3_flip,_MM_SHUFFLE(2,3,0,1));
  x3_flip = _mm_shuffle_epi8(x3_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x02t  = _mm_subs_epi16(*(x0),x2t);
  x13t  = _mm_subs_epi16(x1_flip,x3_flip);
  /*
  *(y1)   = _mm_adds_epi16(*(x0),_mm_subs_epi16(x1_flip,_mm_adds_epi16(x2t,x3_flip)));  // x0 + x1f - x2 - x3f
  *(y3)   = _mm_subs_epi16(*(x0),_mm_adds_epi16(x1_flip,_mm_subs_epi16(x2t,x3_flip)));  // x0 - x1f - x2 + x3f
  */
  *(y1)   = _mm_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  *(y3)   = _mm_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

}

#ifdef __AVX2__
static inline void bfly4_16_256(__m256i *x0,__m256i *x1,__m256i *x2,__m256i *x3,
				__m256i *y0,__m256i *y1,__m256i *y2,__m256i *y3,
				__m256i *tw1,__m256i *tw2,__m256i *tw3,
				__m256i *tw1b,__m256i *tw2b,__m256i *tw3b)__attribute__((always_inline));

static inline void bfly4_16_256(__m256i *x0,__m256i *x1,__m256i *x2,__m256i *x3,
				__m256i *y0,__m256i *y1,__m256i *y2,__m256i *y3,
				__m256i *tw1,__m256i *tw2,__m256i *tw3,
				__m256i *tw1b,__m256i *tw2b,__m256i *tw3b)
{

  register __m256i x1t,x2t,x3t,x02t,x13t;
  register __m256i x1_flip,x3_flip;
  register __m256i complex_shuffle = _mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  // each input xi is assumed to be to consecutive vectors xi0 xi1 on which to perform the 8 butterflies
  // [xi00 xi01 xi02 xi03 xi10 xi20 xi30 xi40]
  // each output yi is the same

  x1t = packed_cmult2_256(*(x1),*(tw1),*(tw1b));
  x2t = packed_cmult2_256(*(x2),*(tw2),*(tw2b));
  x3t = packed_cmult2_256(*(x3),*(tw3),*(tw3b));

  x02t  = _mm256_adds_epi16(*(x0),x2t);
  x13t  = _mm256_adds_epi16(x1t,x3t);
  *(y0)   = _mm256_adds_epi16(x02t,x13t);
  *(y2)   = _mm256_subs_epi16(x02t,x13t);

  x1_flip = _mm256_sign_epi16(x1t,*(__m256i*)conjugatedft);
  x1_flip = _mm256_shuffle_epi8(x1_flip,complex_shuffle);
  x3_flip = _mm256_sign_epi16(x3t,*(__m256i*)conjugatedft);
  x3_flip = _mm256_shuffle_epi8(x3_flip,complex_shuffle);
  x02t  = _mm256_subs_epi16(*(x0),x2t);
  x13t  = _mm256_subs_epi16(x1_flip,x3_flip);
  *(y1)   = _mm256_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  *(y3)   = _mm256_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

}

#endif

#elif defined(__arm__)

static inline void bfly4_16(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
                            int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3,
                            int16x8_t *tw1,int16x8_t *tw2,int16x8_t *tw3,
                            int16x8_t *tw1b,int16x8_t *tw2b,int16x8_t *tw3b)__attribute__((always_inline));

static inline void bfly4_16(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
                            int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3,
                            int16x8_t *tw1,int16x8_t *tw2,int16x8_t *tw3,
                            int16x8_t *tw1b,int16x8_t *tw2b,int16x8_t *tw3b)
{

  register int16x8_t x1t,x2t,x3t,x02t,x13t;
  register int16x8_t x1_flip,x3_flip;

  x1t = packed_cmult2(*(x1),*(tw1),*(tw1b));
  x2t = packed_cmult2(*(x2),*(tw2),*(tw2b));
  x3t = packed_cmult2(*(x3),*(tw3),*(tw3b));



  x02t  = vqaddq_s16(*(x0),x2t);
  x13t  = vqaddq_s16(x1t,x3t);
  *(y0)   = vqaddq_s16(x02t,x13t);
  *(y2)   = vqsubq_s16(x02t,x13t);
  x1_flip = vrev32q_s16(vmulq_s16(x1t,*(int16x8_t*)conjugatedft));
  x3_flip = vrev32q_s16(vmulq_s16(x3t,*(int16x8_t*)conjugatedft));
  x02t  = vqsubq_s16(*(x0),x2t);
  x13t  = vqsubq_s16(x1_flip,x3_flip);
  *(y1)   = vqaddq_s16(x02t,x13t);  // x0 + x1f - x2 - x3f
  *(y3)   = vqsubq_s16(x02t,x13t);  // x0 - x1f - x2 + x3f
}
#endif

#if defined(__x86_64__) || defined(__i386__)
static inline void ibfly4_16(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                             __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3,
                             __m128i *tw1,__m128i *tw2,__m128i *tw3,
                             __m128i *tw1b,__m128i *tw2b,__m128i *tw3b)__attribute__((always_inline));

static inline void ibfly4_16(__m128i *x0,__m128i *x1,__m128i *x2,__m128i *x3,
                             __m128i *y0,__m128i *y1,__m128i *y2,__m128i *y3,
                             __m128i *tw1,__m128i *tw2,__m128i *tw3,
                             __m128i *tw1b,__m128i *tw2b,__m128i *tw3b)
{

  register __m128i x1t,x2t,x3t,x02t,x13t;
  register __m128i x1_flip,x3_flip;

  x1t = packed_cmult2(*(x1),*(tw1),*(tw1b));
  x2t = packed_cmult2(*(x2),*(tw2),*(tw2b));
  x3t = packed_cmult2(*(x3),*(tw3),*(tw3b));


  //  bfly4_tw1(x0,&x1t,&x2t,&x3t,y0,y1,y2,y3);
  x02t  = _mm_adds_epi16(*(x0),x2t);
  x13t  = _mm_adds_epi16(x1t,x3t);
  /*
  *(y0) = _mm_adds_epi16(*(x0),_mm_adds_epi16(x1t,_mm_adds_epi16(x2t,x3t)));
  *(y2)   = _mm_subs_epi16(*(x0),_mm_subs_epi16(x1t,_mm_subs_epi16(x2t,x3t)));
  */
  *(y0)   = _mm_adds_epi16(x02t,x13t);
  *(y2)   = _mm_subs_epi16(x02t,x13t);

  x1_flip = _mm_sign_epi16(x1t,*(__m128i*)conjugatedft);
  //  x1_flip = _mm_shufflelo_epi16(x1_flip,_MM_SHUFFLE(2,3,0,1));
  //  x1_flip = _mm_shufflehi_epi16(x1_flip,_MM_SHUFFLE(2,3,0,1));
  x1_flip = _mm_shuffle_epi8(x1_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x3_flip = _mm_sign_epi16(x3t,*(__m128i*)conjugatedft);
  //  x3_flip = _mm_shufflelo_epi16(x3_flip,_MM_SHUFFLE(2,3,0,1));
  //  x3_flip = _mm_shufflehi_epi16(x3_flip,_MM_SHUFFLE(2,3,0,1));
  x3_flip = _mm_shuffle_epi8(x3_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x02t  = _mm_subs_epi16(*(x0),x2t);
  x13t  = _mm_subs_epi16(x1_flip,x3_flip);
  /*
  *(y1)   = _mm_adds_epi16(*(x0),_mm_subs_epi16(x1_flip,_mm_adds_epi16(x2t,x3_flip)));  // x0 + x1f - x2 - x3f
  *(y3)   = _mm_subs_epi16(*(x0),_mm_adds_epi16(x1_flip,_mm_subs_epi16(x2t,x3_flip)));  // x0 - x1f - x2 + x3f
  */
  *(y3)   = _mm_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  *(y1)   = _mm_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

}

#ifdef __AVX2__
static inline void ibfly4_16_256(__m256i *x0,__m256i *x1,__m256i *x2,__m256i *x3,
				 __m256i *y0,__m256i *y1,__m256i *y2,__m256i *y3,
				 __m256i *tw1,__m256i *tw2,__m256i *tw3,
				 __m256i *tw1b,__m256i *tw2b,__m256i *tw3b)__attribute__((always_inline));

static inline void ibfly4_16_256(__m256i *x0,__m256i *x1,__m256i *x2,__m256i *x3,
				 __m256i *y0,__m256i *y1,__m256i *y2,__m256i *y3,
				 __m256i *tw1,__m256i *tw2,__m256i *tw3,
				 __m256i *tw1b,__m256i *tw2b,__m256i *tw3b)
{

  register __m256i x1t,x2t,x3t,x02t,x13t;
  register __m256i x1_flip,x3_flip;
  register __m256i complex_shuffle = _mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  // each input xi is assumed to be to consecutive vectors xi0 xi1 on which to perform the 8 butterflies
  // [xi00 xi01 xi02 xi03 xi10 xi20 xi30 xi40]
  // each output yi is the same

  x1t = packed_cmult2_256(*(x1),*(tw1),*(tw1b));
  x2t = packed_cmult2_256(*(x2),*(tw2),*(tw2b));
  x3t = packed_cmult2_256(*(x3),*(tw3),*(tw3b));

  x02t  = _mm256_adds_epi16(*(x0),x2t);
  x13t  = _mm256_adds_epi16(x1t,x3t);
  *(y0)   = _mm256_adds_epi16(x02t,x13t);
  *(y2)   = _mm256_subs_epi16(x02t,x13t);

  x1_flip = _mm256_sign_epi16(x1t,*(__m256i*)conjugatedft);
  x1_flip = _mm256_shuffle_epi8(x1_flip,complex_shuffle);
  x3_flip = _mm256_sign_epi16(x3t,*(__m256i*)conjugatedft);
  x3_flip = _mm256_shuffle_epi8(x3_flip,complex_shuffle);
  x02t  = _mm256_subs_epi16(*(x0),x2t);
  x13t  = _mm256_subs_epi16(x1_flip,x3_flip);
  *(y3)   = _mm256_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  *(y1)   = _mm256_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

}
#endif

#elif defined(__arm__)
static inline void ibfly4_16(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
			     int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3,
			     int16x8_t *tw1,int16x8_t *tw2,int16x8_t *tw3,
			     int16x8_t *tw1b,int16x8_t *tw2b,int16x8_t *tw3b)__attribute__((always_inline));

static inline void ibfly4_16(int16x8_t *x0,int16x8_t *x1,int16x8_t *x2,int16x8_t *x3,
			     int16x8_t *y0,int16x8_t *y1,int16x8_t *y2,int16x8_t *y3,
			     int16x8_t *tw1,int16x8_t *tw2,int16x8_t *tw3,
			     int16x8_t *tw1b,int16x8_t *tw2b,int16x8_t *tw3b)
{

  register int16x8_t x1t,x2t,x3t,x02t,x13t;
  register int16x8_t x1_flip,x3_flip;

  x1t = packed_cmult2(*(x1),*(tw1),*(tw1b));
  x2t = packed_cmult2(*(x2),*(tw2),*(tw2b));
  x3t = packed_cmult2(*(x3),*(tw3),*(tw3b));

  x02t    = vqaddq_s16(*(x0),x2t);
  x13t    = vqaddq_s16(x1t,x3t);
  *(y0)   = vqaddq_s16(x02t,x13t);
  *(y2)   = vqsubq_s16(x02t,x13t);
  x1_flip = vrev32q_s16(vmulq_s16(x1t,*(int16x8_t*)conjugatedft));
  x3_flip = vrev32q_s16(vmulq_s16(x3t,*(int16x8_t*)conjugatedft));
  x02t    = vqsubq_s16(*(x0),x2t);
  x13t    = vqsubq_s16(x1_flip,x3_flip);
  *(y3)   = vqaddq_s16(x02t,x13t);  // x0 - x1f - x2 + x3f
  *(y1)   = vqsubq_s16(x02t,x13t);  // x0 + x1f - x2 - x3f
}

#endif

#if defined(__x86_64__) || defined(__i386__)
static inline void bfly5(__m128i *x0, __m128i *x1, __m128i *x2, __m128i *x3,__m128i *x4,
                         __m128i *y0, __m128i *y1, __m128i *y2, __m128i *y3,__m128i *y4,
                         __m128i *tw1,__m128i *tw2,__m128i *tw3,__m128i *tw4)__attribute__((always_inline));

static inline void bfly5(__m128i *x0, __m128i *x1, __m128i *x2, __m128i *x3,__m128i *x4,
                         __m128i *y0, __m128i *y1, __m128i *y2, __m128i *y3,__m128i *y4,
                         __m128i *tw1,__m128i *tw2,__m128i *tw3,__m128i *tw4)
{



  __m128i x1_2,x2_2,x3_2,x4_2,tmpre,tmpim;

  packed_cmult(*(x1),*(tw1),&x1_2);
  packed_cmult(*(x2),*(tw2),&x2_2);
  packed_cmult(*(x3),*(tw3),&x3_2);
  packed_cmult(*(x4),*(tw4),&x4_2);

  *(y0)  = _mm_adds_epi16(*(x0),_mm_adds_epi16(x1_2,_mm_adds_epi16(x2_2,_mm_adds_epi16(x3_2,x4_2))));
  cmult(x1_2,*(W15),&tmpre,&tmpim);
  cmac(x2_2,*(W25),&tmpre,&tmpim);
  cmac(x3_2,*(W35),&tmpre,&tmpim);
  cmac(x4_2,*(W45),&tmpre,&tmpim);
  *(y1) = cpack(tmpre,tmpim);
  *(y1) = _mm_adds_epi16(*(x0),*(y1));

  cmult(x1_2,*(W25),&tmpre,&tmpim);
  cmac(x2_2,*(W45),&tmpre,&tmpim);
  cmac(x3_2,*(W15),&tmpre,&tmpim);
  cmac(x4_2,*(W35),&tmpre,&tmpim);
  *(y2) = cpack(tmpre,tmpim);
  *(y2) = _mm_adds_epi16(*(x0),*(y2));

  cmult(x1_2,*(W35),&tmpre,&tmpim);
  cmac(x2_2,*(W15),&tmpre,&tmpim);
  cmac(x3_2,*(W45),&tmpre,&tmpim);
  cmac(x4_2,*(W25),&tmpre,&tmpim);
  *(y3) = cpack(tmpre,tmpim);
  *(y3) = _mm_adds_epi16(*(x0),*(y3));

  cmult(x1_2,*(W45),&tmpre,&tmpim);
  cmac(x2_2,*(W35),&tmpre,&tmpim);
  cmac(x3_2,*(W25),&tmpre,&tmpim);
  cmac(x4_2,*(W15),&tmpre,&tmpim);
  *(y4) = cpack(tmpre,tmpim);
  *(y4) = _mm_adds_epi16(*(x0),*(y4));


}

#ifdef __AVX2__

static inline void bfly5_256(__m256i *x0, __m256i *x1, __m256i *x2, __m256i *x3,__m256i *x4,
			     __m256i *y0, __m256i *y1, __m256i *y2, __m256i *y3,__m256i *y4,
			     __m256i *tw1,__m256i *tw2,__m256i *tw3,__m256i *tw4)__attribute__((always_inline));

static inline void bfly5_256(__m256i *x0, __m256i *x1, __m256i *x2, __m256i *x3,__m256i *x4,
			     __m256i *y0, __m256i *y1, __m256i *y2, __m256i *y3,__m256i *y4,
			     __m256i *tw1,__m256i *tw2,__m256i *tw3,__m256i *tw4)
{



  __m256i x1_2,x2_2,x3_2,x4_2,tmpre,tmpim;

  packed_cmult_256(*(x1),*(tw1),&x1_2);
  packed_cmult_256(*(x2),*(tw2),&x2_2);
  packed_cmult_256(*(x3),*(tw3),&x3_2);
  packed_cmult_256(*(x4),*(tw4),&x4_2);

  *(y0)  = _mm256_adds_epi16(*(x0),_mm256_adds_epi16(x1_2,_mm256_adds_epi16(x2_2,_mm256_adds_epi16(x3_2,x4_2))));
  cmult_256(x1_2,*(W15_256),&tmpre,&tmpim);
  cmac_256(x2_2,*(W25_256),&tmpre,&tmpim);
  cmac_256(x3_2,*(W35_256),&tmpre,&tmpim);
  cmac_256(x4_2,*(W45_256),&tmpre,&tmpim);
  *(y1) = cpack_256(tmpre,tmpim);
  *(y1) = _mm256_adds_epi16(*(x0),*(y1));

  cmult_256(x1_2,*(W25_256),&tmpre,&tmpim);
  cmac_256(x2_2,*(W45_256),&tmpre,&tmpim);
  cmac_256(x3_2,*(W15_256),&tmpre,&tmpim);
  cmac_256(x4_2,*(W35_256),&tmpre,&tmpim);
  *(y2) = cpack_256(tmpre,tmpim);
  *(y2) = _mm256_adds_epi16(*(x0),*(y2));

  cmult_256(x1_2,*(W35_256),&tmpre,&tmpim);
  cmac_256(x2_2,*(W15_256),&tmpre,&tmpim);
  cmac_256(x3_2,*(W45_256),&tmpre,&tmpim);
  cmac_256(x4_2,*(W25_256),&tmpre,&tmpim);
  *(y3) = cpack_256(tmpre,tmpim);
  *(y3) = _mm256_adds_epi16(*(x0),*(y3));

  cmult_256(x1_2,*(W45_256),&tmpre,&tmpim);
  cmac_256(x2_2,*(W35_256),&tmpre,&tmpim);
  cmac_256(x3_2,*(W25_256),&tmpre,&tmpim);
  cmac_256(x4_2,*(W15_256),&tmpre,&tmpim);
  *(y4) = cpack_256(tmpre,tmpim);
  *(y4) = _mm256_adds_epi16(*(x0),*(y4));


}
#endif

#elif defined(__arm__)
static inline void bfly5(int16x8_t *x0, int16x8_t *x1, int16x8_t *x2, int16x8_t *x3,int16x8_t *x4,
                         int16x8_t *y0, int16x8_t *y1, int16x8_t *y2, int16x8_t *y3,int16x8_t *y4,
                         int16x8_t *tw1,int16x8_t *tw2,int16x8_t *tw3,int16x8_t *tw4)__attribute__((always_inline));

static inline void bfly5(int16x8_t *x0, int16x8_t *x1, int16x8_t *x2, int16x8_t *x3,int16x8_t *x4,
                         int16x8_t *y0, int16x8_t *y1, int16x8_t *y2, int16x8_t *y3,int16x8_t *y4,
                         int16x8_t *tw1,int16x8_t *tw2,int16x8_t *tw3,int16x8_t *tw4)
{



  int16x8_t x1_2,x2_2,x3_2,x4_2;
  int32x4_t tmpre,tmpim;

  packed_cmult(*(x1),*(tw1),&x1_2);
  packed_cmult(*(x2),*(tw2),&x2_2);
  packed_cmult(*(x3),*(tw3),&x3_2);
  packed_cmult(*(x4),*(tw4),&x4_2);

  *(y0)  = vqaddq_s16(*(x0),vqaddq_s16(x1_2,vqaddq_s16(x2_2,vqaddq_s16(x3_2,x4_2))));
  cmult(x1_2,*(W15),&tmpre,&tmpim);
  cmac(x2_2,*(W25),&tmpre,&tmpim);
  cmac(x3_2,*(W35),&tmpre,&tmpim);
  cmac(x4_2,*(W45),&tmpre,&tmpim);
  *(y1) = cpack(tmpre,tmpim);
  *(y1) = vqaddq_s16(*(x0),*(y1));

  cmult(x1_2,*(W25),&tmpre,&tmpim);
  cmac(x2_2,*(W45),&tmpre,&tmpim);
  cmac(x3_2,*(W15),&tmpre,&tmpim);
  cmac(x4_2,*(W35),&tmpre,&tmpim);
  *(y2) = cpack(tmpre,tmpim);
  *(y2) = vqaddq_s16(*(x0),*(y2));

  cmult(x1_2,*(W35),&tmpre,&tmpim);
  cmac(x2_2,*(W15),&tmpre,&tmpim);
  cmac(x3_2,*(W45),&tmpre,&tmpim);
  cmac(x4_2,*(W25),&tmpre,&tmpim);
  *(y3) = cpack(tmpre,tmpim);
  *(y3) = vqaddq_s16(*(x0),*(y3));

  cmult(x1_2,*(W45),&tmpre,&tmpim);
  cmac(x2_2,*(W35),&tmpre,&tmpim);
  cmac(x3_2,*(W25),&tmpre,&tmpim);
  cmac(x4_2,*(W15),&tmpre,&tmpim);
  *(y4) = cpack(tmpre,tmpim);
  *(y4) = vqaddq_s16(*(x0),*(y4));


}


#endif

#if defined(__x86_64__) || defined(__i386__)
static inline void bfly5_tw1(__m128i *x0, __m128i *x1, __m128i *x2, __m128i *x3,__m128i *x4,
                             __m128i *y0, __m128i *y1, __m128i *y2, __m128i *y3,__m128i *y4) __attribute__((always_inline));

static inline void bfly5_tw1(__m128i *x0, __m128i *x1, __m128i *x2, __m128i *x3,__m128i *x4,
                             __m128i *y0, __m128i *y1, __m128i *y2, __m128i *y3,__m128i *y4)
{

  __m128i tmpre,tmpim;

  *(y0) = _mm_adds_epi16(*(x0),_mm_adds_epi16(*(x1),_mm_adds_epi16(*(x2),_mm_adds_epi16(*(x3),*(x4)))));
  cmult(*(x1),*(W15),&tmpre,&tmpim);
  cmac(*(x2),*(W25),&tmpre,&tmpim);
  cmac(*(x3),*(W35),&tmpre,&tmpim);
  cmac(*(x4),*(W45),&tmpre,&tmpim);
  *(y1) = cpack(tmpre,tmpim);
  *(y1) = _mm_adds_epi16(*(x0),*(y1));
  cmult(*(x1),*(W25),&tmpre,&tmpim);
  cmac(*(x2),*(W45),&tmpre,&tmpim);
  cmac(*(x3),*(W15),&tmpre,&tmpim);
  cmac(*(x4),*(W35),&tmpre,&tmpim);
  *(y2) = cpack(tmpre,tmpim);
  *(y2) = _mm_adds_epi16(*(x0),*(y2));
  cmult(*(x1),*(W35),&tmpre,&tmpim);
  cmac(*(x2),*(W15),&tmpre,&tmpim);
  cmac(*(x3),*(W45),&tmpre,&tmpim);
  cmac(*(x4),*(W25),&tmpre,&tmpim);
  *(y3) = cpack(tmpre,tmpim);
  *(y3) = _mm_adds_epi16(*(x0),*(y3));
  cmult(*(x1),*(W45),&tmpre,&tmpim);
  cmac(*(x2),*(W35),&tmpre,&tmpim);
  cmac(*(x3),*(W25),&tmpre,&tmpim);
  cmac(*(x4),*(W15),&tmpre,&tmpim);
  *(y4) = cpack(tmpre,tmpim);
  *(y4) = _mm_adds_epi16(*(x0),*(y4));
}

#ifdef __AVX2__
static inline void bfly5_tw1_256(__m256i *x0, __m256i *x1, __m256i *x2, __m256i *x3,__m256i *x4,
				 __m256i *y0, __m256i *y1, __m256i *y2, __m256i *y3,__m256i *y4) __attribute__((always_inline));

static inline void bfly5_tw1_256(__m256i *x0, __m256i *x1, __m256i *x2, __m256i *x3,__m256i *x4,
				 __m256i *y0, __m256i *y1, __m256i *y2, __m256i *y3,__m256i *y4)
{

  __m256i tmpre,tmpim;

  *(y0) = _mm256_adds_epi16(*(x0),_mm256_adds_epi16(*(x1),_mm256_adds_epi16(*(x2),_mm256_adds_epi16(*(x3),*(x4)))));
  cmult_256(*(x1),*(W15_256),&tmpre,&tmpim);
  cmac_256(*(x2),*(W25_256),&tmpre,&tmpim);
  cmac_256(*(x3),*(W35_256),&tmpre,&tmpim);
  cmac_256(*(x4),*(W45_256),&tmpre,&tmpim);
  *(y1) = cpack_256(tmpre,tmpim);
  *(y1) = _mm256_adds_epi16(*(x0),*(y1));
  cmult_256(*(x1),*(W25_256),&tmpre,&tmpim);
  cmac_256(*(x2),*(W45_256),&tmpre,&tmpim);
  cmac_256(*(x3),*(W15_256),&tmpre,&tmpim);
  cmac_256(*(x4),*(W35_256),&tmpre,&tmpim);
  *(y2) = cpack_256(tmpre,tmpim);
  *(y2) = _mm256_adds_epi16(*(x0),*(y2));
  cmult_256(*(x1),*(W35_256),&tmpre,&tmpim);
  cmac_256(*(x2),*(W15_256),&tmpre,&tmpim);
  cmac_256(*(x3),*(W45_256),&tmpre,&tmpim);
  cmac_256(*(x4),*(W25_256),&tmpre,&tmpim);
  *(y3) = cpack_256(tmpre,tmpim);
  *(y3) = _mm256_adds_epi16(*(x0),*(y3));
  cmult_256(*(x1),*(W45_256),&tmpre,&tmpim);
  cmac_256(*(x2),*(W35_256),&tmpre,&tmpim);
  cmac_256(*(x3),*(W25_256),&tmpre,&tmpim);
  cmac_256(*(x4),*(W15_256),&tmpre,&tmpim);
  *(y4) = cpack_256(tmpre,tmpim);
  *(y4) = _mm256_adds_epi16(*(x0),*(y4));
}
#endif
#elif defined(__arm__)
static inline void bfly5_tw1(int16x8_t *x0, int16x8_t *x1, int16x8_t *x2, int16x8_t *x3,int16x8_t *x4,
                             int16x8_t *y0, int16x8_t *y1, int16x8_t *y2, int16x8_t *y3,int16x8_t *y4) __attribute__((always_inline));

static inline void bfly5_tw1(int16x8_t *x0, int16x8_t *x1, int16x8_t *x2, int16x8_t *x3,int16x8_t *x4,
                             int16x8_t *y0, int16x8_t *y1, int16x8_t *y2, int16x8_t *y3,int16x8_t *y4)
{

  int32x4_t tmpre,tmpim;

  *(y0) = vqaddq_s16(*(x0),vqaddq_s16(*(x1),vqaddq_s16(*(x2),vqaddq_s16(*(x3),*(x4)))));
  cmult(*(x1),*(W15),&tmpre,&tmpim);
  cmac(*(x2),*(W25),&tmpre,&tmpim);
  cmac(*(x3),*(W35),&tmpre,&tmpim);
  cmac(*(x4),*(W45),&tmpre,&tmpim);
  *(y1) = cpack(tmpre,tmpim);
  *(y1) = vqaddq_s16(*(x0),*(y1));
  cmult(*(x1),*(W25),&tmpre,&tmpim);
  cmac(*(x2),*(W45),&tmpre,&tmpim);
  cmac(*(x3),*(W15),&tmpre,&tmpim);
  cmac(*(x4),*(W35),&tmpre,&tmpim);
  *(y2) = cpack(tmpre,tmpim);
  *(y2) = vqaddq_s16(*(x0),*(y2));
  cmult(*(x1),*(W35),&tmpre,&tmpim);
  cmac(*(x2),*(W15),&tmpre,&tmpim);
  cmac(*(x3),*(W45),&tmpre,&tmpim);
  cmac(*(x4),*(W25),&tmpre,&tmpim);
  *(y3) = cpack(tmpre,tmpim);
  *(y3) = vqaddq_s16(*(x0),*(y3));
  cmult(*(x1),*(W45),&tmpre,&tmpim);
  cmac(*(x2),*(W35),&tmpre,&tmpim);
  cmac(*(x3),*(W25),&tmpre,&tmpim);
  cmac(*(x4),*(W15),&tmpre,&tmpim);
  *(y4) = cpack(tmpre,tmpim);
  *(y4) = vqaddq_s16(*(x0),*(y4));
}

#endif
// performs 4x4 transpose of input x (complex interleaved) using 128bit SIMD intrinsics
// i.e. x = [x0r x0i x1r x1i ... x15r x15i], y = [x0r x0i x4r x4i x8r x8i x12r x12i x1r x1i x5r x5i x9r x9i x13r x13i x2r x2i ... x15r x15i]

#if defined(__x86_64__) || defined(__i386__)
static inline void transpose16(__m128i *x,__m128i *y) __attribute__((always_inline));
static inline void transpose16(__m128i *x,__m128i *y)
{
  register __m128i ytmp0,ytmp1,ytmp2,ytmp3;

  ytmp0 = _mm_unpacklo_epi32(x[0],x[1]);
  ytmp1 = _mm_unpackhi_epi32(x[0],x[1]);
  ytmp2 = _mm_unpacklo_epi32(x[2],x[3]);
  ytmp3 = _mm_unpackhi_epi32(x[2],x[3]);
  y[0]    = _mm_unpacklo_epi64(ytmp0,ytmp2);
  y[1]    = _mm_unpackhi_epi64(ytmp0,ytmp2);
  y[2]    = _mm_unpacklo_epi64(ytmp1,ytmp3);
  y[3]    = _mm_unpackhi_epi64(ytmp1,ytmp3);
}

#elif defined(__arm__)
static inline void transpose16(int16x8_t *x,int16x8_t *y) __attribute__((always_inline));
static inline void transpose16(int16x8_t *x,int16x8_t *y)
{
  register uint32x4x2_t ytmp0,ytmp1;

  ytmp0 = vtrnq_u32((uint32x4_t)(x[0]),(uint32x4_t)(x[1]));
  ytmp1 = vtrnq_u32((uint32x4_t)(x[2]),(uint32x4_t)(x[3]));

  y[0]  = vcombine_s16(vget_low_s16((int16x8_t)ytmp0.val[0]),vget_low_s16((int16x8_t)ytmp1.val[0]));
  y[1]  = vcombine_s16(vget_high_s16((int16x8_t)ytmp0.val[0]),vget_high_s16((int16x8_t)ytmp1.val[0]));
  y[2]  = vcombine_s16(vget_low_s16((int16x8_t)ytmp0.val[1]),vget_low_s16((int16x8_t)ytmp1.val[1]));
  y[3]  = vcombine_s16(vget_high_s16((int16x8_t)ytmp0.val[1]),vget_high_s16((int16x8_t)ytmp1.val[1]));
}

# endif
// same as above but output is offset by off
#if defined(__x86_64__) || defined(__i386__)
static inline void transpose16_ooff(__m128i *x,__m128i *y,int off) __attribute__((always_inline));

static inline void transpose16_ooff(__m128i *x,__m128i *y,int off)
{
  register __m128i ytmp0,ytmp1,ytmp2,ytmp3;
  __m128i *y2=y;

  ytmp0 = _mm_unpacklo_epi32(x[0],x[1]); // x00 x10 x01 x11
  ytmp1 = _mm_unpackhi_epi32(x[0],x[1]); // x02 x12 x03 x13
  ytmp2 = _mm_unpacklo_epi32(x[2],x[3]); // x20 x30 x21 x31
  ytmp3 = _mm_unpackhi_epi32(x[2],x[3]); // x22 x32 x23 x33
  *y2     = _mm_unpacklo_epi64(ytmp0,ytmp2); // x00 x10 x20 x30 
  y2+=off;
  *y2     = _mm_unpackhi_epi64(ytmp0,ytmp2); // x01 x11 x21 x31
  y2+=off;
  *y2     = _mm_unpacklo_epi64(ytmp1,ytmp3); // x02 x12 x22 x32
  y2+=off;
  *y2     = _mm_unpackhi_epi64(ytmp1,ytmp3); // x03 x13 x23 x33
}

#ifdef __AVX2__

static inline void transpose16_ooff_simd256(__m256i *x,__m256i *y,int off) __attribute__((always_inline));
static inline void transpose16_ooff_simd256(__m256i *x,__m256i *y,int off)
{
  register __m256i ytmp0,ytmp1,ytmp2,ytmp3,ytmp4,ytmp5,ytmp6,ytmp7;
  __m256i *y2=y;
  __m256i const perm_mask = _mm256_set_epi32(7, 3, 5, 1, 6, 2, 4, 0);

  ytmp0 = _mm256_permutevar8x32_epi32(x[0],perm_mask);  // x00 x10 x01 x11 x02 x12 x03 x13
  ytmp1 = _mm256_permutevar8x32_epi32(x[1],perm_mask);  // x20 x30 x21 x31 x22 x32 x23 x33
  ytmp2 = _mm256_permutevar8x32_epi32(x[2],perm_mask);  // x40 x50 x41 x51 x42 x52 x43 x53
  ytmp3 = _mm256_permutevar8x32_epi32(x[3],perm_mask);  // x60 x70 x61 x71 x62 x72 x63 x73
  ytmp4 = _mm256_unpacklo_epi64(ytmp0,ytmp1);           // x00 x10 x20 x30 x01 x11 x21 x31
  ytmp5 = _mm256_unpackhi_epi64(ytmp0,ytmp1);           // x02 x12 x22 x32 x03 x13 x23 x33
  ytmp6 = _mm256_unpacklo_epi64(ytmp2,ytmp3);           // x40 x50 x60 x70 x41 x51 x61 x71
  ytmp7 = _mm256_unpackhi_epi64(ytmp2,ytmp3);           // x42 x52 x62 x72 x43 x53 x63 x73

  *y2    = _mm256_insertf128_si256(ytmp4,_mm256_extracti128_si256(ytmp6,0),1);  //x00 x10 x20 x30 x40 x50 x60 x70
  y2+=off;  
  *y2    = _mm256_insertf128_si256(ytmp6,_mm256_extracti128_si256(ytmp4,1),0);  //x01 x11 x21 x31 x41 x51 x61 x71
  y2+=off;  
  *y2    = _mm256_insertf128_si256(ytmp5,_mm256_extracti128_si256(ytmp7,0),1);  //x00 x10 x20 x30 x40 x50 x60 x70
  y2+=off;  
  *y2    = _mm256_insertf128_si256(ytmp7,_mm256_extracti128_si256(ytmp5,1),0);  //x01 x11 x21 x31 x41 x51 x61 x71
}
#endif

#elif defined(__arm__)
static inline void transpose16_ooff(int16x8_t *x,int16x8_t *y,int off) __attribute__((always_inline));

static inline void transpose16_ooff(int16x8_t *x,int16x8_t *y,int off)
{
  int16x8_t *y2=y;
  register uint32x4x2_t ytmp0,ytmp1;

  ytmp0 = vtrnq_u32((uint32x4_t)(x[0]),(uint32x4_t)(x[1]));
  ytmp1 = vtrnq_u32((uint32x4_t)(x[2]),(uint32x4_t)(x[3]));

  *y2   = (int16x8_t)vcombine_s16(vget_low_s16((int16x8_t)ytmp0.val[0]),vget_low_s16((int16x8_t)ytmp1.val[0])); y2+=off;
  *y2   = (int16x8_t)vcombine_s16(vget_low_s16((int16x8_t)ytmp0.val[1]),vget_low_s16((int16x8_t)ytmp1.val[1])); y2+=off;
  *y2   = (int16x8_t)vcombine_s16(vget_high_s16((int16x8_t)ytmp0.val[0]),vget_high_s16((int16x8_t)ytmp1.val[0])); y2+=off;
  *y2   = (int16x8_t)vcombine_s16(vget_high_s16((int16x8_t)ytmp0.val[1]),vget_high_s16((int16x8_t)ytmp1.val[1]));


}

#endif

#if defined(__x86_64__) || defined(__i386__)

static inline void transpose4_ooff(__m64 *x,__m64 *y,int off)__attribute__((always_inline));
static inline void transpose4_ooff(__m64 *x,__m64 *y,int off)
{
  y[0]   = _mm_unpacklo_pi32(x[0],x[1]);
  y[off] = _mm_unpackhi_pi32(x[0],x[1]);

  // x[0] = [x0 x1]
  // x[1] = [x2 x3]
  // y[0] = [x0 x2]
  // y[off] = [x1 x3]
}
#ifdef __AVX2__
static inline void transpose4_ooff_simd256(__m256i *x,__m256i *y,int off)__attribute__((always_inline));
static inline void transpose4_ooff_simd256(__m256i *x,__m256i *y,int off)
{
  __m256i const perm_mask = _mm256_set_epi32(7, 5, 3, 1, 6, 4, 2, 0);
  __m256i perm_tmp0,perm_tmp1;

  // x[0] = [x0 x1 x2 x3 x4 x5 x6 x7]
  // x[1] = [x8 x9 x10 x11 x12 x13 x14]
  // y[0] = [x0 x2 x4 x6 x8 x10 x12 x14]
  // y[off] = [x1 x3 x5 x7 x9 x11 x13 x15]
  perm_tmp0 = _mm256_permutevar8x32_epi32(x[0],perm_mask);
  perm_tmp1 = _mm256_permutevar8x32_epi32(x[1],perm_mask);
  y[0]   = _mm256_insertf128_si256(perm_tmp0,_mm256_extracti128_si256(perm_tmp1,0),1);
  y[off] = _mm256_insertf128_si256(perm_tmp1,_mm256_extracti128_si256(perm_tmp0,1),0);
}
#endif
#elif (__arm__)

static inline void transpose4_ooff(int16x4_t *x,int16x4_t *y,int off)__attribute__((always_inline));
static inline void transpose4_ooff(int16x4_t *x,int16x4_t *y,int off)
{
  uint32x2x2_t ytmp = vtrn_u32((uint32x2_t)x[0],(uint32x2_t)x[1]);

  y[0]   = (int16x4_t)ytmp.val[0];
  y[off] = (int16x4_t)ytmp.val[1];
}

#endif

// 16-point optimized DFT kernel

const static int16_t tw16[24] __attribute__((aligned(32))) = { 32767,0,30272,-12540,23169 ,-23170,12539 ,-30273,
                                                  32767,0,23169,-23170,0     ,-32767,-23170,-23170,
                                                  32767,0,12539,-30273,-23170,-23170,-30273,12539
                                                };

const static int16_t tw16a[24] __attribute__((aligned(32))) = {32767,0,30272,12540,23169 ,23170,12539 ,30273,
                                                  32767,0,23169,23170,0     ,32767,-23170,23170,
                                                  32767,0,12539,30273,-23170,23170,-30273,-12539
                                                 };

const static int16_t tw16b[24] __attribute__((aligned(32))) = { 0,32767,-12540,30272,-23170,23169 ,-30273,12539,
                                                   0,32767,-23170,23169,-32767,0     ,-23170,-23170,
                                                   0,32767,-30273,12539,-23170,-23170,12539 ,-30273
                                                 };

const static int16_t tw16c[24] __attribute__((aligned(32))) = { 0,32767,12540,30272,23170,23169 ,30273 ,12539,
                                                   0,32767,23170,23169,32767,0     ,23170 ,-23170,
                                                   0,32767,30273,12539,23170,-23170,-12539,-30273
                                                 };

#ifdef __AVX2__

const static int16_t tw16rep[48] __attribute__((aligned(32))) = { 32767,0,30272,-12540,23169 ,-23170,12539 ,-30273,32767,0,30272,-12540,23169 ,-23170,12539 ,-30273,
						     32767,0,23169,-23170,0     ,-32767,-23170,-23170,32767,0,23169,-23170,0     ,-32767,-23170,-23170,
						     32767,0,12539,-30273,-23170,-23170,-30273,12539,32767,0,12539,-30273,-23170,-23170,-30273,12539
                                                   };

const static int16_t tw16arep[48] __attribute__((aligned(32))) = {32767,0,30272,12540,23169 ,23170,12539 ,30273,32767,0,30272,12540,23169 ,23170,12539 ,30273,
						     32767,0,23169,23170,0     ,32767,-23170,23170,32767,0,23169,23170,0     ,32767,-23170,23170,
						     32767,0,12539,30273,-23170,23170,-30273,-12539,32767,0,12539,30273,-23170,23170,-30273,-12539
                                                    }; 

const static int16_t tw16brep[48] __attribute__((aligned(32))) = { 0,32767,-12540,30272,-23170,23169 ,-30273,12539,0,32767,-12540,30272,-23170,23169 ,-30273,12539,
                                                      0,32767,-23170,23169,-32767,0     ,-23170,-23170,0,32767,-23170,23169,-32767,0     ,-23170,-23170,
                                                      0,32767,-30273,12539,-23170,-23170,12539 ,-30273,0,32767,-30273,12539,-23170,-23170,12539 ,-30273
                                                    };

const static int16_t tw16crep[48] __attribute__((aligned(32))) = { 0,32767,12540,30272,23170,23169 ,30273 ,12539,0,32767,12540,30272,23170,23169 ,30273 ,12539,
						      0,32767,23170,23169,32767,0     ,23170 ,-23170,0,32767,23170,23169,32767,0     ,23170 ,-23170,
						      0,32767,30273,12539,23170,-23170,-12539,-30273,0,32767,30273,12539,23170,-23170,-12539,-30273
                                                    };

#endif /* __AVX2__ */



static inline void dft16(int16_t *x,int16_t *y) __attribute__((always_inline));

static inline void dft16(int16_t *x,int16_t *y)
{

#if defined(__x86_64__) || defined(__i386__)

  __m128i *tw16a_128=(__m128i *)tw16a,*tw16b_128=(__m128i *)tw16b,*x128=(__m128i *)x,*y128=(__m128i *)y;



  /*  This is the original version before unrolling

  bfly4_tw1(x128,x128+1,x128+2,x128+3,
      y128,y128+1,y128+2,y128+3);

  transpose16(y128,ytmp);

  bfly4_16(ytmp,ytmp+1,ytmp+2,ytmp+3,
     y128,y128+1,y128+2,y128+3,
     tw16_128,tw16_128+1,tw16_128+2);
  */

  register __m128i x1_flip,x3_flip,x02t,x13t;
  register __m128i ytmp0,ytmp1,ytmp2,ytmp3,xtmp0,xtmp1,xtmp2,xtmp3;
  register __m128i complex_shuffle = _mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  // First stage : 4 Radix-4 butterflies without input twiddles

  x02t    = _mm_adds_epi16(x128[0],x128[2]);
  x13t    = _mm_adds_epi16(x128[1],x128[3]);
  xtmp0   = _mm_adds_epi16(x02t,x13t);
  xtmp2   = _mm_subs_epi16(x02t,x13t);
  x1_flip = _mm_sign_epi16(x128[1],*(__m128i*)conjugatedft);
  x1_flip = _mm_shuffle_epi8(x1_flip,complex_shuffle);
  x3_flip = _mm_sign_epi16(x128[3],*(__m128i*)conjugatedft);
  x3_flip = _mm_shuffle_epi8(x3_flip,complex_shuffle);
  x02t    = _mm_subs_epi16(x128[0],x128[2]);
  x13t    = _mm_subs_epi16(x1_flip,x3_flip);
  xtmp1   = _mm_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  xtmp3   = _mm_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

  ytmp0   = _mm_unpacklo_epi32(xtmp0,xtmp1);
  ytmp1   = _mm_unpackhi_epi32(xtmp0,xtmp1);
  ytmp2   = _mm_unpacklo_epi32(xtmp2,xtmp3);
  ytmp3   = _mm_unpackhi_epi32(xtmp2,xtmp3);
  xtmp0   = _mm_unpacklo_epi64(ytmp0,ytmp2);
  xtmp1   = _mm_unpackhi_epi64(ytmp0,ytmp2);
  xtmp2   = _mm_unpacklo_epi64(ytmp1,ytmp3);
  xtmp3   = _mm_unpackhi_epi64(ytmp1,ytmp3);

  // Second stage : 4 Radix-4 butterflies with input twiddles
  xtmp1 = packed_cmult2(xtmp1,tw16a_128[0],tw16b_128[0]);
  xtmp2 = packed_cmult2(xtmp2,tw16a_128[1],tw16b_128[1]);
  xtmp3 = packed_cmult2(xtmp3,tw16a_128[2],tw16b_128[2]);

  x02t    = _mm_adds_epi16(xtmp0,xtmp2);
  x13t    = _mm_adds_epi16(xtmp1,xtmp3);
  y128[0] = _mm_adds_epi16(x02t,x13t);
  y128[2] = _mm_subs_epi16(x02t,x13t);
  x1_flip = _mm_sign_epi16(xtmp1,*(__m128i*)conjugatedft);
  x1_flip = _mm_shuffle_epi8(x1_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x3_flip = _mm_sign_epi16(xtmp3,*(__m128i*)conjugatedft);
  x3_flip = _mm_shuffle_epi8(x3_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x02t    = _mm_subs_epi16(xtmp0,xtmp2);
  x13t    = _mm_subs_epi16(x1_flip,x3_flip);
  y128[1] = _mm_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  y128[3] = _mm_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

#elif defined(__arm__)

  int16x8_t *tw16a_128=(int16x8_t *)tw16a,*tw16b_128=(int16x8_t *)tw16b,*x128=(int16x8_t *)x,*y128=(int16x8_t *)y;

  /*  This is the original version before unrolling

  bfly4_tw1(x128,x128+1,x128+2,x128+3,
      y128,y128+1,y128+2,y128+3);

  transpose16(y128,ytmp);

  bfly4_16(ytmp,ytmp+1,ytmp+2,ytmp+3,
     y128,y128+1,y128+2,y128+3,
     tw16_128,tw16_128+1,tw16_128+2);
  */

  register int16x8_t x1_flip,x3_flip,x02t,x13t;
  register int16x8_t xtmp0,xtmp1,xtmp2,xtmp3;
  register uint32x4x2_t ytmp0,ytmp1;
  register int16x8_t ytmp0b,ytmp1b,ytmp2b,ytmp3b;

  // First stage : 4 Radix-4 butterflies without input twiddles
  
  x02t    = vqaddq_s16(x128[0],x128[2]);
  x13t    = vqaddq_s16(x128[1],x128[3]);
  xtmp0   = vqaddq_s16(x02t,x13t);
  xtmp2   = vqsubq_s16(x02t,x13t);
  x1_flip = vrev32q_s16(vmulq_s16(x128[1],*(int16x8_t*)conjugatedft));
  x3_flip = vrev32q_s16(vmulq_s16(x128[3],*(int16x8_t*)conjugatedft));
  x02t    = vqsubq_s16(x128[0],x128[2]);
  x13t    = vqsubq_s16(x1_flip,x3_flip);
  xtmp1   = vqaddq_s16(x02t,x13t);  // x0 + x1f - x2 - x3f
  xtmp3   = vqsubq_s16(x02t,x13t);  // x0 - x1f - x2 + x3f

  ytmp0  = vtrnq_u32((uint32x4_t)(xtmp0),(uint32x4_t)(xtmp1));
// y0[0] = [x00 x10 x02 x12], y0[1] = [x01 x11 x03 x13]
  ytmp1  = vtrnq_u32((uint32x4_t)(xtmp2),(uint32x4_t)(xtmp3));
// y1[0] = [x20 x30 x22 x32], y1[1] = [x21 x31 x23 x33]


  ytmp0b = vcombine_s16(vget_low_s16((int16x8_t)ytmp0.val[0]),vget_low_s16((int16x8_t)ytmp1.val[0]));
// y0 = [x00 x10 x20 x30] 
  ytmp1b = vcombine_s16(vget_low_s16((int16x8_t)ytmp0.val[1]),vget_low_s16((int16x8_t)ytmp1.val[1]));
// t1 = [x01 x11 x21 x31] 
  ytmp2b = vcombine_s16(vget_high_s16((int16x8_t)ytmp0.val[0]),vget_high_s16((int16x8_t)ytmp1.val[0]));
// t2 = [x02 x12 x22 x32]
  ytmp3b = vcombine_s16(vget_high_s16((int16x8_t)ytmp0.val[1]),vget_high_s16((int16x8_t)ytmp1.val[1]));
// t3 = [x03 x13 x23 x33]


  // Second stage : 4 Radix-4 butterflies with input twiddles
  xtmp1 = packed_cmult2(ytmp1b,tw16a_128[0],tw16b_128[0]);
  xtmp2 = packed_cmult2(ytmp2b,tw16a_128[1],tw16b_128[1]);
  xtmp3 = packed_cmult2(ytmp3b,tw16a_128[2],tw16b_128[2]);

  x02t    = vqaddq_s16(ytmp0b,xtmp2);
  x13t    = vqaddq_s16(xtmp1,xtmp3);
  y128[0] = vqaddq_s16(x02t,x13t);
  y128[2] = vqsubq_s16(x02t,x13t);
  x1_flip = vrev32q_s16(vmulq_s16(xtmp1,*(int16x8_t*)conjugatedft));
  x3_flip = vrev32q_s16(vmulq_s16(xtmp3,*(int16x8_t*)conjugatedft));
  x02t    = vqsubq_s16(ytmp0b,xtmp2);
  x13t    = vqsubq_s16(x1_flip,x3_flip);
  y128[1] = vqaddq_s16(x02t,x13t);  // x0 + x1f - x2 - x3f
  y128[3] = vqsubq_s16(x02t,x13t);  // x0 - x1f - x2 + x3f


#endif
}

#if defined(__x86_64__) || defined(__i386__)
#ifdef __AVX2__
// Does two 16-point DFTS (x[0 .. 15] is 128 LSBs of input vector, x[16..31] is in 128 MSBs) 
static inline void dft16_simd256(int16_t *x,int16_t *y) __attribute__((always_inline));
static inline void dft16_simd256(int16_t *x,int16_t *y)
{

  __m256i *tw16a_256=(__m256i *)tw16arep,*tw16b_256=(__m256i *)tw16brep,*x256=(__m256i *)x,*y256=(__m256i *)y;

  __m256i x1_flip,x3_flip,x02t,x13t;
  __m256i ytmp0,ytmp1,ytmp2,ytmp3,xtmp0,xtmp1,xtmp2,xtmp3;
  register __m256i complex_shuffle = _mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  // First stage : 4 Radix-4 butterflies without input twiddles

  x02t    = _mm256_adds_epi16(x256[0],x256[2]);
  x13t    = _mm256_adds_epi16(x256[1],x256[3]);
  xtmp0   = _mm256_adds_epi16(x02t,x13t);
  xtmp2   = _mm256_subs_epi16(x02t,x13t);
  x1_flip = _mm256_sign_epi16(x256[1],*(__m256i*)conjugatedft);
  x1_flip = _mm256_shuffle_epi8(x1_flip,complex_shuffle);
  x3_flip = _mm256_sign_epi16(x256[3],*(__m256i*)conjugatedft);
  x3_flip = _mm256_shuffle_epi8(x3_flip,complex_shuffle);
  x02t    = _mm256_subs_epi16(x256[0],x256[2]);
  x13t    = _mm256_subs_epi16(x1_flip,x3_flip);
  xtmp1   = _mm256_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  xtmp3   = _mm256_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

  /*  print_shorts256("xtmp0",(int16_t*)&xtmp0);
      print_shorts256("xtmp1",(int16_t*)&xtmp1);
  print_shorts256("xtmp2",(int16_t*)&xtmp2);
  print_shorts256("xtmp3",(int16_t*)&xtmp3);*/

  ytmp0   = _mm256_unpacklo_epi32(xtmp0,xtmp1);  
  ytmp1   = _mm256_unpackhi_epi32(xtmp0,xtmp1);
  ytmp2   = _mm256_unpacklo_epi32(xtmp2,xtmp3);
  ytmp3   = _mm256_unpackhi_epi32(xtmp2,xtmp3);
  xtmp0   = _mm256_unpacklo_epi64(ytmp0,ytmp2);
  xtmp1   = _mm256_unpackhi_epi64(ytmp0,ytmp2);
  xtmp2   = _mm256_unpacklo_epi64(ytmp1,ytmp3);
  xtmp3   = _mm256_unpackhi_epi64(ytmp1,ytmp3);

  // Second stage : 4 Radix-4 butterflies with input twiddles
  xtmp1 = packed_cmult2_256(xtmp1,tw16a_256[0],tw16b_256[0]);
  xtmp2 = packed_cmult2_256(xtmp2,tw16a_256[1],tw16b_256[1]);
  xtmp3 = packed_cmult2_256(xtmp3,tw16a_256[2],tw16b_256[2]);

  /*  print_shorts256("xtmp0",(int16_t*)&xtmp0);
  print_shorts256("xtmp1",(int16_t*)&xtmp1);
  print_shorts256("xtmp2",(int16_t*)&xtmp2);
  print_shorts256("xtmp3",(int16_t*)&xtmp3);*/

  x02t    = _mm256_adds_epi16(xtmp0,xtmp2);
  x13t    = _mm256_adds_epi16(xtmp1,xtmp3);
  ytmp0   = _mm256_adds_epi16(x02t,x13t);
  ytmp2   = _mm256_subs_epi16(x02t,x13t);
  x1_flip = _mm256_sign_epi16(xtmp1,*(__m256i*)conjugatedft);
  x1_flip = _mm256_shuffle_epi8(x1_flip,complex_shuffle);
  x3_flip = _mm256_sign_epi16(xtmp3,*(__m256i*)conjugatedft);
  x3_flip = _mm256_shuffle_epi8(x3_flip,complex_shuffle);
  x02t    = _mm256_subs_epi16(xtmp0,xtmp2);
  x13t    = _mm256_subs_epi16(x1_flip,x3_flip);
  ytmp1   = _mm256_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  ytmp3   = _mm256_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f
 

  // [y0  y1  y2  y3  y16 y17 y18 y19]
  // [y4  y5  y6  y7  y20 y21 y22 y23]
  // [y8  y9  y10 y11 y24 y25 y26 y27]
  // [y12 y13 y14 y15 y28 y29 y30 y31]

  y256[0] = _mm256_insertf128_si256(ytmp0,_mm256_extracti128_si256(ytmp1,0),1);
  y256[1] = _mm256_insertf128_si256(ytmp2,_mm256_extracti128_si256(ytmp3,0),1);
  y256[2] = _mm256_insertf128_si256(ytmp1,_mm256_extracti128_si256(ytmp0,1),0);
  y256[3] = _mm256_insertf128_si256(ytmp3,_mm256_extracti128_si256(ytmp2,1),0);

  // [y0  y1  y2  y3  y4  y5  y6  y7]
  // [y8  y9  y10 y11 y12 y13 y14 y15]
  // [y16 y17 y18 y19 y20 y21 y22 y23]
  // [y24 y25 y26 y27 y28 y29 y30 y31]
}

#endif  
#endif
static inline void idft16(int16_t *x,int16_t *y) __attribute__((always_inline));

static inline void idft16(int16_t *x,int16_t *y)
{

#if defined(__x86_64__) || defined(__i386__)
  __m128i *tw16a_128=(__m128i *)tw16,*tw16b_128=(__m128i *)tw16c,*x128=(__m128i *)x,*y128=(__m128i *)y;

  /*
  bfly4_tw1(x128,x128+1,x128+2,x128+3,
      y128,y128+1,y128+2,y128+3);

  transpose16(y128,ytmp);

  bfly4_16(ytmp,ytmp+1,ytmp+2,ytmp+3,
     y128,y128+1,y128+2,y128+3,
     tw16_128,tw16_128+1,tw16_128+2);
  */

  register __m128i x1_flip,x3_flip,x02t,x13t;
  register __m128i ytmp0,ytmp1,ytmp2,ytmp3,xtmp0,xtmp1,xtmp2,xtmp3;

  // First stage : 4 Radix-4 butterflies without input twiddles

  x02t    = _mm_adds_epi16(x128[0],x128[2]);
  x13t    = _mm_adds_epi16(x128[1],x128[3]);
  xtmp0   = _mm_adds_epi16(x02t,x13t);
  xtmp2   = _mm_subs_epi16(x02t,x13t);
  x1_flip = _mm_sign_epi16(x128[1],*(__m128i*)conjugatedft);
  x1_flip = _mm_shuffle_epi8(x1_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x3_flip = _mm_sign_epi16(x128[3],*(__m128i*)conjugatedft);
  x3_flip = _mm_shuffle_epi8(x3_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x02t    = _mm_subs_epi16(x128[0],x128[2]);
  x13t    = _mm_subs_epi16(x1_flip,x3_flip);
  xtmp3   = _mm_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  xtmp1   = _mm_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

  ytmp0   = _mm_unpacklo_epi32(xtmp0,xtmp1);
  ytmp1   = _mm_unpackhi_epi32(xtmp0,xtmp1);
  ytmp2   = _mm_unpacklo_epi32(xtmp2,xtmp3);
  ytmp3   = _mm_unpackhi_epi32(xtmp2,xtmp3);
  xtmp0   = _mm_unpacklo_epi64(ytmp0,ytmp2);
  xtmp1   = _mm_unpackhi_epi64(ytmp0,ytmp2);
  xtmp2   = _mm_unpacklo_epi64(ytmp1,ytmp3);
  xtmp3   = _mm_unpackhi_epi64(ytmp1,ytmp3);

  // Second stage : 4 Radix-4 butterflies with input twiddles
  xtmp1 = packed_cmult2(xtmp1,tw16a_128[0],tw16b_128[0]);
  xtmp2 = packed_cmult2(xtmp2,tw16a_128[1],tw16b_128[1]);
  xtmp3 = packed_cmult2(xtmp3,tw16a_128[2],tw16b_128[2]);

  x02t    = _mm_adds_epi16(xtmp0,xtmp2);
  x13t    = _mm_adds_epi16(xtmp1,xtmp3);
  y128[0] = _mm_adds_epi16(x02t,x13t);
  y128[2] = _mm_subs_epi16(x02t,x13t);
  x1_flip = _mm_sign_epi16(xtmp1,*(__m128i*)conjugatedft);
  x1_flip = _mm_shuffle_epi8(x1_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x3_flip = _mm_sign_epi16(xtmp3,*(__m128i*)conjugatedft);
  x3_flip = _mm_shuffle_epi8(x3_flip,_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2));
  x02t    = _mm_subs_epi16(xtmp0,xtmp2);
  x13t    = _mm_subs_epi16(x1_flip,x3_flip);
  y128[3] = _mm_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  y128[1] = _mm_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

#elif defined(__arm__)
  int16x8_t *tw16a_128=(int16x8_t *)tw16,*tw16b_128=(int16x8_t *)tw16c,*x128=(int16x8_t *)x,*y128=(int16x8_t *)y;

  /*  This is the original version before unrolling

  bfly4_tw1(x128,x128+1,x128+2,x128+3,
      y128,y128+1,y128+2,y128+3);

  transpose16(y128,ytmp);

  bfly4_16(ytmp,ytmp+1,ytmp+2,ytmp+3,
     y128,y128+1,y128+2,y128+3,
     tw16_128,tw16_128+1,tw16_128+2);
  */

  register int16x8_t x1_flip,x3_flip,x02t,x13t;
  register int16x8_t xtmp0,xtmp1,xtmp2,xtmp3;
  register uint32x4x2_t ytmp0,ytmp1;
  register int16x8_t ytmp0b,ytmp1b,ytmp2b,ytmp3b;

  // First stage : 4 Radix-4 butterflies without input twiddles

  x02t    = vqaddq_s16(x128[0],x128[2]);
  x13t    = vqaddq_s16(x128[1],x128[3]);
  xtmp0   = vqaddq_s16(x02t,x13t);
  xtmp2   = vqsubq_s16(x02t,x13t);
  x1_flip = vrev32q_s16(vmulq_s16(x128[1],*(int16x8_t*)conjugatedft));
  x3_flip = vrev32q_s16(vmulq_s16(x128[3],*(int16x8_t*)conjugatedft));
  x02t    = vqsubq_s16(x128[0],x128[2]);
  x13t    = vqsubq_s16(x1_flip,x3_flip);
  xtmp3   = vqaddq_s16(x02t,x13t);  // x0 + x1f - x2 - x3f
  xtmp1   = vqsubq_s16(x02t,x13t);  // x0 - x1f - x2 + x3f

  ytmp0  = vtrnq_u32((uint32x4_t)(xtmp0),(uint32x4_t)(xtmp1));
// y0[0] = [x00 x10 x02 x12], y0[1] = [x01 x11 x03 x13]
  ytmp1  = vtrnq_u32((uint32x4_t)(xtmp2),(uint32x4_t)(xtmp3));
// y1[0] = [x20 x30 x22 x32], y1[1] = [x21 x31 x23 x33]


  ytmp0b = vcombine_s16(vget_low_s16((int16x8_t)ytmp0.val[0]),vget_low_s16((int16x8_t)ytmp1.val[0]));
// y0 = [x00 x10 x20 x30] 
  ytmp1b = vcombine_s16(vget_low_s16((int16x8_t)ytmp0.val[1]),vget_low_s16((int16x8_t)ytmp1.val[1]));
// t1 = [x01 x11 x21 x31] 
  ytmp2b = vcombine_s16(vget_high_s16((int16x8_t)ytmp0.val[0]),vget_high_s16((int16x8_t)ytmp1.val[0]));
// t2 = [x02 x12 x22 x32]
  ytmp3b = vcombine_s16(vget_high_s16((int16x8_t)ytmp0.val[1]),vget_high_s16((int16x8_t)ytmp1.val[1]));
// t3 = [x03 x13 x23 x33]

  // Second stage : 4 Radix-4 butterflies with input twiddles
  xtmp1 = packed_cmult2(ytmp1b,tw16a_128[0],tw16b_128[0]);
  xtmp2 = packed_cmult2(ytmp2b,tw16a_128[1],tw16b_128[1]);
  xtmp3 = packed_cmult2(ytmp3b,tw16a_128[2],tw16b_128[2]);

  x02t    = vqaddq_s16(ytmp0b,xtmp2);
  x13t    = vqaddq_s16(xtmp1,xtmp3);
  y128[0] = vqaddq_s16(x02t,x13t);
  y128[2] = vqsubq_s16(x02t,x13t);
  x1_flip = vrev32q_s16(vmulq_s16(xtmp1,*(int16x8_t*)conjugatedft));
  x3_flip = vrev32q_s16(vmulq_s16(xtmp3,*(int16x8_t*)conjugatedft));
  x02t    = vqsubq_s16(ytmp0b,xtmp2);
  x13t    = vqsubq_s16(x1_flip,x3_flip);
  y128[3] = vqaddq_s16(x02t,x13t);  // x0 + x1f - x2 - x3f
  y128[1] = vqsubq_s16(x02t,x13t);  // x0 - x1f - x2 + x3f

#endif
}

#if defined(__x86_64__) || defined(__i386__)
#ifdef __AVX2__
// Does two 16-point IDFTS (x[0 .. 15] is 128 LSBs of input vector, x[16..31] is in 128 MSBs) 
static inline void idft16_simd256(int16_t *x,int16_t *y) __attribute__((always_inline));
static inline void idft16_simd256(int16_t *x,int16_t *y)
{

  __m256i *tw16a_256=(__m256i *)tw16rep,*tw16b_256=(__m256i *)tw16crep,*x256=(__m256i *)x,*y256=(__m256i *)y;
  register __m256i x1_flip,x3_flip,x02t,x13t;
  register __m256i ytmp0,ytmp1,ytmp2,ytmp3,xtmp0,xtmp1,xtmp2,xtmp3;
  register __m256i complex_shuffle = _mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);

  // First stage : 4 Radix-4 butterflies without input twiddles

  x02t    = _mm256_adds_epi16(x256[0],x256[2]);
  x13t    = _mm256_adds_epi16(x256[1],x256[3]);
  xtmp0   = _mm256_adds_epi16(x02t,x13t);
  xtmp2   = _mm256_subs_epi16(x02t,x13t);
  x1_flip = _mm256_sign_epi16(x256[1],*(__m256i*)conjugatedft);
  x1_flip = _mm256_shuffle_epi8(x1_flip,complex_shuffle);
  x3_flip = _mm256_sign_epi16(x256[3],*(__m256i*)conjugatedft);
  x3_flip = _mm256_shuffle_epi8(x3_flip,complex_shuffle);
  x02t    = _mm256_subs_epi16(x256[0],x256[2]);
  x13t    = _mm256_subs_epi16(x1_flip,x3_flip);
  xtmp3   = _mm256_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  xtmp1   = _mm256_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

  ytmp0   = _mm256_unpacklo_epi32(xtmp0,xtmp1);  
  ytmp1   = _mm256_unpackhi_epi32(xtmp0,xtmp1);
  ytmp2   = _mm256_unpacklo_epi32(xtmp2,xtmp3);
  ytmp3   = _mm256_unpackhi_epi32(xtmp2,xtmp3);
  xtmp0   = _mm256_unpacklo_epi64(ytmp0,ytmp2);
  xtmp1   = _mm256_unpackhi_epi64(ytmp0,ytmp2);
  xtmp2   = _mm256_unpacklo_epi64(ytmp1,ytmp3);
  xtmp3   = _mm256_unpackhi_epi64(ytmp1,ytmp3);

  // Second stage : 4 Radix-4 butterflies with input twiddles
  xtmp1 = packed_cmult2_256(xtmp1,tw16a_256[0],tw16b_256[0]);
  xtmp2 = packed_cmult2_256(xtmp2,tw16a_256[1],tw16b_256[1]);
  xtmp3 = packed_cmult2_256(xtmp3,tw16a_256[2],tw16b_256[2]);

  x02t    = _mm256_adds_epi16(xtmp0,xtmp2);
  x13t    = _mm256_adds_epi16(xtmp1,xtmp3);
  ytmp0   = _mm256_adds_epi16(x02t,x13t);
  ytmp2   = _mm256_subs_epi16(x02t,x13t);
  x1_flip = _mm256_sign_epi16(xtmp1,*(__m256i*)conjugatedft);
  x1_flip = _mm256_shuffle_epi8(x1_flip,complex_shuffle);
  x3_flip = _mm256_sign_epi16(xtmp3,*(__m256i*)conjugatedft);
  x3_flip = _mm256_shuffle_epi8(x3_flip,complex_shuffle);
  x02t    = _mm256_subs_epi16(xtmp0,xtmp2);
  x13t    = _mm256_subs_epi16(x1_flip,x3_flip);
  ytmp3   = _mm256_adds_epi16(x02t,x13t);  // x0 + x1f - x2 - x3f
  ytmp1   = _mm256_subs_epi16(x02t,x13t);  // x0 - x1f - x2 + x3f

  // [y0  y1  y2  y3  y16 y17 y18 y19]
  // [y4  y5  y6  y7  y20 y21 y22 y23]
  // [y8  y9  y10 y11 y24 y25 y26 y27]
  // [y12 y13 y14 y15 y28 y29 y30 y31]

  y256[0] = _mm256_insertf128_si256(ytmp0,_mm256_extracti128_si256(ytmp1,0),1);
  y256[1] = _mm256_insertf128_si256(ytmp2,_mm256_extracti128_si256(ytmp3,0),1);
  y256[2] = _mm256_insertf128_si256(ytmp1,_mm256_extracti128_si256(ytmp0,1),0);
  y256[3] = _mm256_insertf128_si256(ytmp3,_mm256_extracti128_si256(ytmp2,1),0);

}
#endif  
#endif

// 64-point optimized DFT

const static int16_t tw64[96] __attribute__((aligned(32))) = { 
32767,0,32609,-3212,32137,-6393,31356,-9512,
30272,-12540,28897,-15447,27244,-18205,25329,-20788,
23169,-23170,20787,-25330,18204,-27245,15446,-28898,
12539,-30273,9511,-31357,6392,-32138,3211,-32610,
32767,0,32137,-6393,30272,-12540,27244,-18205,
23169,-23170,18204,-27245,12539,-30273,6392,-32138,
0,-32767,-6393,-32138,-12540,-30273,-18205,-27245,
-23170,-23170,-27245,-18205,-30273,-12540,-32138,-6393,
32767,0,31356,-9512,27244,-18205,20787,-25330,
12539,-30273,3211,-32610,-6393,-32138,-15447,-28898,
-23170,-23170,-28898,-15447,-32138,-6393,-32610,3211,
-30273,12539,-25330,20787,-18205,27244,-9512,31356
                                                };
const static int16_t tw64a[96] __attribute__((aligned(32))) = { 
32767,0,32609,3212,32137,6393,31356,9512,
30272,12540,28897,15447,27244,18205,25329,20788,
23169,23170,20787,25330,18204,27245,15446,28898,
12539,30273,9511,31357,6392,32138,3211,32610,
32767,0,32137,6393,30272,12540,27244,18205,
23169,23170,18204,27245,12539,30273,6392,32138,
0,32767,-6393,32138,-12540,30273,-18205,27245,
-23170,23170,-27245,18205,-30273,12540,-32138,6393,
32767,0,31356,9512,27244,18205,20787,25330,
12539,30273,3211,32610,-6393,32138,-15447,28898,
-23170,23170,-28898,15447,-32138,6393,-32610,-3211,
-30273,-12539,-25330,-20787,-18205,-27244,-9512,-31356
                                                 };
const static int16_t tw64b[96] __attribute__((aligned(32))) = { 
0,32767,-3212,32609,-6393,32137,-9512,31356,
-12540,30272,-15447,28897,-18205,27244,-20788,25329,
-23170,23169,-25330,20787,-27245,18204,-28898,15446,
-30273,12539,-31357,9511,-32138,6392,-32610,3211,
0,32767,-6393,32137,-12540,30272,-18205,27244,
-23170,23169,-27245,18204,-30273,12539,-32138,6392,
-32767,0,-32138,-6393,-30273,-12540,-27245,-18205,
-23170,-23170,-18205,-27245,-12540,-30273,-6393,-32138,
0,32767,-9512,31356,-18205,27244,-25330,20787,
-30273,12539,-32610,3211,-32138,-6393,-28898,-15447,
-23170,-23170,-15447,-28898,-6393,-32138,3211,-32610,
12539,-30273,20787,-25330,27244,-18205,31356,-9512
                                                 };
const static int16_t tw64c[96] __attribute__((aligned(32))) = { 
0,32767,3212,32609,6393,32137,9512,31356,
12540,30272,15447,28897,18205,27244,20788,25329,
23170,23169,25330,20787,27245,18204,28898,15446,
30273,12539,31357,9511,32138,6392,32610,3211,
0,32767,6393,32137,12540,30272,18205,27244,
23170,23169,27245,18204,30273,12539,32138,6392,
32767,0,32138,-6393,30273,-12540,27245,-18205,
23170,-23170,18205,-27245,12540,-30273,6393,-32138,
0,32767,9512,31356,18205,27244,25330,20787,
30273,12539,32610,3211,32138,-6393,28898,-15447,
23170,-23170,15447,-28898,6393,-32138,-3211,-32610,
-12539,-30273,-20787,-25330,-27244,-18205,-31356,-9512
                                                 };
#if defined(__x86_64__) || defined(__i386__)
#define simd_q15_t __m128i
#define simdshort_q15_t __m64
#define shiftright_int16(a,shift) _mm_srai_epi16(a,shift)
#define set1_int16(a) _mm_set1_epi16(a);
#define mulhi_int16(a,b) _mm_mulhrs_epi16 (a,b)
#ifdef __AVX2__
#define simd256_q15_t __m256i
#define shiftright_int16_simd256(a,shift) _mm256_srai_epi16(a,shift)
#define set1_int16_simd256(a) _mm256_set1_epi16(a);
#define mulhi_int16_simd256(a,b) _mm256_slli_epi16(_mm256_mulhi_epi16(a,b),1);
#endif

#elif defined(__arm__)
#define simd_q15_t int16x8_t
#define simdshort_q15_t int16x4_t
#define shiftright_int16(a,shift) vshrq_n_s16(a,shift)
#define set1_int16(a) vdupq_n_s16(a)
#define mulhi_int16(a,b) vqdmulhq_s16(a,b);
#define _mm_empty() 
#define _m_empty()

#endif

#ifndef __AVX2__
void dft64(int16_t *x,int16_t *y,int scale)
{

  simd_q15_t xtmp[16],ytmp[16],*tw64a_128=(simd_q15_t *)tw64a,*tw64b_128=(simd_q15_t *)tw64b,*x128=(simd_q15_t *)x,*y128=(simd_q15_t *)y;


#ifdef D64STATS
  time_stats_t ts_t,ts_d,ts_b;

  reset_meas(&ts_t);
  reset_meas(&ts_d);
  reset_meas(&ts_b);
  start_meas(&ts_t);
#endif


  transpose16_ooff(x128,xtmp,4);
  // xtmp0  = x00 x10 x20 x30
  // xtmp4  = x01 x11 x21 x31
  // xtmp8  = x02 x12 x22 x32
  // xtmp12 = x03 x13 x23 x33
  transpose16_ooff(x128+4,xtmp+1,4);
  // xtmp1  = x40 x50 x60 x70
  // xtmp5  = x41 x51 x61 x71
  // xtmp9  = x42 x52 x62 x72
  // xtmp13 = x43 x53 x63 x73
  transpose16_ooff(x128+8,xtmp+2,4);
  // xtmp2  = x80 x90 xa0 xb0
  // xtmp6  = x41 x51 x61 x71
  // xtmp10 = x82 x92 xa2 xb2
  // xtmp14 = x83 x93 xa3 xb3
  transpose16_ooff(x128+12,xtmp+3,4);
  // xtmp3  = xc0 xd0 xe0 xf0
  // xtmp7  = xc1 xd1 xe1 xf1
  // xtmp11 = xc2 xd2 xe2 xf2
  // xtmp15 = xc3 xd3 xe3 xf3

#ifdef D64STATS
  stop_meas(&ts_t);
  start_meas(&ts_d);
#endif

  // xtmp0  = x00 x10 x20 x30
  // xtmp1  = x40 x50 x60 x70
  // xtmp2  = x80 x90 xa0 xb0
  // xtmp3  = xc0 xd0 xe0 xf0
  dft16((int16_t*)(xtmp),(int16_t*)ytmp);

  // xtmp4  = x01 x11 x21 x31
  // xtmp5  = x41 x51 x61 x71
  // xtmp6  = x81 x91 xa1 xb1
  // xtmp7  = xc1 xd1 xe1 xf1
  dft16((int16_t*)(xtmp+4),(int16_t*)(ytmp+4));
  dft16((int16_t*)(xtmp+8),(int16_t*)(ytmp+8));
  dft16((int16_t*)(xtmp+12),(int16_t*)(ytmp+12));


#ifdef D64STATS
  stop_meas(&ts_d);
  start_meas(&ts_b);
#endif


  bfly4_16(ytmp,ytmp+4,ytmp+8,ytmp+12,
           y128,y128+4,y128+8,y128+12,
           tw64a_128,tw64a_128+4,tw64a_128+8,
           tw64b_128,tw64b_128+4,tw64b_128+8);

  bfly4_16(ytmp+1,ytmp+5,ytmp+9,ytmp+13,
           y128+1,y128+5,y128+9,y128+13,
           tw64a_128+1,tw64a_128+5,tw64a_128+9,
           tw64b_128+1,tw64b_128+5,tw64b_128+9);

  bfly4_16(ytmp+2,ytmp+6,ytmp+10,ytmp+14,
           y128+2,y128+6,y128+10,y128+14,
           tw64a_128+2,tw64a_128+6,tw64a_128+10,
           tw64b_128+2,tw64b_128+6,tw64b_128+10);

  bfly4_16(ytmp+3,ytmp+7,ytmp+11,ytmp+15,
           y128+3,y128+7,y128+11,y128+15,
           tw64a_128+3,tw64a_128+7,tw64a_128+11,
           tw64b_128+3,tw64b_128+7,tw64b_128+11);

#ifdef D64STATS
  stop_meas(&ts_b);
  printf("t: %llu cycles, d: %llu cycles, b: %llu cycles\n",ts_t.diff,ts_d.diff,ts_b.diff);
#endif


  if (scale>0) {
    y128[0]  = shiftright_int16(y128[0],3);
    y128[1]  = shiftright_int16(y128[1],3);
    y128[2]  = shiftright_int16(y128[2],3);
    y128[3]  = shiftright_int16(y128[3],3);
    y128[4]  = shiftright_int16(y128[4],3);
    y128[5]  = shiftright_int16(y128[5],3);
    y128[6]  = shiftright_int16(y128[6],3);
    y128[7]  = shiftright_int16(y128[7],3);
    y128[8]  = shiftright_int16(y128[8],3);
    y128[9]  = shiftright_int16(y128[9],3);
    y128[10] = shiftright_int16(y128[10],3);
    y128[11] = shiftright_int16(y128[11],3);
    y128[12] = shiftright_int16(y128[12],3);
    y128[13] = shiftright_int16(y128[13],3);
    y128[14] = shiftright_int16(y128[14],3);
    y128[15] = shiftright_int16(y128[15],3);
  }

  _mm_empty();
  _m_empty();

}

#else // __AVX2__
void dft64(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[16],ytmp[16],*tw64a_256=(simd256_q15_t *)tw64a,*tw64b_256=(simd256_q15_t *)tw64b,*x256=(simd256_q15_t *)x,*y256=(simd256_q15_t *)y;
  simd256_q15_t xintl0,xintl1,xintl2,xintl3,xintl4,xintl5,xintl6,xintl7;
  simd256_q15_t const perm_mask = _mm256_set_epi32(7, 3, 5, 1, 6, 2, 4, 0);


#ifdef D64STATS
  time_stats_t ts_t,ts_d,ts_b;

  reset_meas(&ts_t);
  reset_meas(&ts_d);
  reset_meas(&ts_b);
  start_meas(&ts_t);
#endif

#ifdef D64STATS
  stop_meas(&ts_t);
  start_meas(&ts_d);
#endif
  /*  
  print_shorts256("x2560",(int16_t*)x256);
  print_shorts256("x2561",(int16_t*)(x256+1));
  print_shorts256("x2562",(int16_t*)(x256+2));
  print_shorts256("x2563",(int16_t*)(x256+3));
  print_shorts256("x2564",(int16_t*)(x256+4));
  print_shorts256("x2565",(int16_t*)(x256+5));
  print_shorts256("x2566",(int16_t*)(x256+6));
  print_shorts256("x2567",(int16_t*)(x256+7));
  */
  xintl0 = _mm256_permutevar8x32_epi32(x256[0],perm_mask);  // x0  x4  x1  x5  x2  x6  x3  x7
  xintl1 = _mm256_permutevar8x32_epi32(x256[1],perm_mask);  // x8  x12 x9  x13 x10 x14 x11 x15
  xintl2 = _mm256_permutevar8x32_epi32(x256[2],perm_mask);  // x16 x20 x17 x21 x18 x22 x19 x23
  xintl3 = _mm256_permutevar8x32_epi32(x256[3],perm_mask);  // x24 x28 x25 x29 x26 x30 x27 x31
  xintl4 = _mm256_permutevar8x32_epi32(x256[4],perm_mask);  // x32 x28 x25 x29 x26 x30 x27 x31
  xintl5 = _mm256_permutevar8x32_epi32(x256[5],perm_mask);  // x40 x28 x25 x29 x26 x30 x27 x31
  xintl6 = _mm256_permutevar8x32_epi32(x256[6],perm_mask);  // x48 x28 x25 x29 x26 x30 x27 x31
  xintl7 = _mm256_permutevar8x32_epi32(x256[7],perm_mask);  // x56 x28 x25 x29 x26 x30 x27 x31
  /*
  print_shorts256("xintl0",(int16_t*)&xintl0);
  print_shorts256("xintl1",(int16_t*)&xintl1);
  print_shorts256("xintl2",(int16_t*)&xintl2);
  print_shorts256("xintl3",(int16_t*)&xintl3);
  print_shorts256("xintl4",(int16_t*)&xintl4);
  print_shorts256("xintl5",(int16_t*)&xintl5);
  print_shorts256("xintl6",(int16_t*)&xintl6);
  print_shorts256("xintl7",(int16_t*)&xintl7);
  */
  xtmp[0] = _mm256_unpacklo_epi64(xintl0,xintl1);        // x0  x4  x8  x12 x1  x5  x9  x13
  xtmp[4] = _mm256_unpackhi_epi64(xintl0,xintl1);        // x2  x6  x10 x14 x3  x7  x11 x15
  xtmp[1] = _mm256_unpacklo_epi64(xintl2,xintl3);        // x16 x20 x24 x28 x17 x21 x25 x29
  xtmp[5] = _mm256_unpackhi_epi64(xintl2,xintl3);        // x18 x22 x26 x30 x19 x23 x27 x31
  xtmp[2] = _mm256_unpacklo_epi64(xintl4,xintl5);        // x32 x36 x40 x44 x33 x37 x41 x45
  xtmp[6] = _mm256_unpackhi_epi64(xintl4,xintl5);        // x34 x38 x42 x46 x35 x39 x43 x47
  xtmp[3] = _mm256_unpacklo_epi64(xintl6,xintl7);        // x48 x52 x56 x60 x49 x53 x57 x61
  xtmp[7] = _mm256_unpackhi_epi64(xintl6,xintl7);        // x50 x54 x58 x62 x51 x55 x59 x63
  /*
  print_shorts256("xtmp0",(int16_t*)xtmp);
  print_shorts256("xtmp1",(int16_t*)(xtmp+1));
  print_shorts256("xtmp2",(int16_t*)(xtmp+2));
  print_shorts256("xtmp3",(int16_t*)(xtmp+3));
  print_shorts256("xtmp4",(int16_t*)(xtmp+4));
  print_shorts256("xtmp5",(int16_t*)(xtmp+5));
  print_shorts256("xtmp6",(int16_t*)(xtmp+6));
  print_shorts256("xtmp7",(int16_t*)(xtmp+7));
  */
  dft16_simd256((int16_t*)(xtmp),(int16_t*)ytmp);
  // [y0  y1  y2  y3  y4  y5  y6  y7]
  // [y8  y9  y10 y11 y12 y13 y14 y15]
  // [y16 y17 y18 y19 y20 y21 y22 y23]
  // [y24 y25 y26 y27 y28 y29 y30 y31]
  /*
  print_shorts256("ytmp0",(int16_t*)ytmp);
  print_shorts256("ytmp1",(int16_t*)(ytmp+1));
  print_shorts256("ytmp2",(int16_t*)(ytmp+2));
  print_shorts256("ytmp3",(int16_t*)(ytmp+3));
  */
  dft16_simd256((int16_t*)(xtmp+4),(int16_t*)(ytmp+4));
  // [y32 y33 y34 y35 y36 y37 y38 y39]
  // [y40 y41 y42 y43 y44 y45 y46 y47]
  // [y48 y49 y50 y51 y52 y53 y54 y55]
  // [y56 y57 y58 y59 y60 y61 y62 y63]
  /*
  print_shorts256("ytmp4",(int16_t*)(ytmp+4));
  print_shorts256("ytmp5",(int16_t*)(ytmp+5));
  print_shorts256("ytmp6",(int16_t*)(ytmp+6));
  print_shorts256("ytmp7",(int16_t*)(ytmp+7));
  */
#ifdef D64STATS
  stop_meas(&ts_d);
  start_meas(&ts_b);
#endif


  bfly4_16_256(ytmp,ytmp+2,ytmp+4,ytmp+6,
	       y256,y256+2,y256+4,y256+6,
	       tw64a_256,tw64a_256+2,tw64a_256+4,
	       tw64b_256,tw64b_256+2,tw64b_256+4);
  // [y0  y1  y2  y3  y4  y5  y6  y7]
  // [y16 y17 y18 y19 y20 y21 y22 y23]
  // [y32 y33 y34 y35 y36 y37 y38 y39]
  // [y48 y49 y50 y51 y52 y53 y54 y55]

  bfly4_16_256(ytmp+1,ytmp+3,ytmp+5,ytmp+7,
	       y256+1,y256+3,y256+5,y256+7,
	       tw64a_256+1,tw64a_256+3,tw64a_256+5,
	       tw64b_256+1,tw64b_256+3,tw64b_256+5);
  // [y8  y9  y10 y11 y12 y13 y14 y15]
  // [y24 y25 y26 y27 y28 y29 y30 y31]
  // [y40 y41 y42 y43 y44 y45 y46 y47]
  // [y56 y57 y58 y59 y60 y61 y62 y63]
  /*  
  print_shorts256("y256_0",(int16_t*)&y256[0]);
  print_shorts256("y256_1",(int16_t*)&y256[1]);
  print_shorts256("y256_2",(int16_t*)&y256[2]);
  print_shorts256("y256_3",(int16_t*)&y256[3]);
  print_shorts256("y256_4",(int16_t*)&y256[4]);
  print_shorts256("y256_5",(int16_t*)&y256[5]);
  print_shorts256("y256_6",(int16_t*)&y256[6]);
  print_shorts256("y256_7",(int16_t*)&y256[7]);
  */

#ifdef D64STATS
  stop_meas(&ts_b);
  printf("t: %llu cycles, d: %llu cycles, b: %llu cycles\n",ts_t.diff,ts_d.diff,ts_b.diff);
#endif


  if (scale>0) {
    y256[0]  = shiftright_int16_simd256(y256[0],3);
    y256[1]  = shiftright_int16_simd256(y256[1],3);
    y256[2]  = shiftright_int16_simd256(y256[2],3);
    y256[3]  = shiftright_int16_simd256(y256[3],3);
    y256[4]  = shiftright_int16_simd256(y256[4],3);
    y256[5]  = shiftright_int16_simd256(y256[5],3);
    y256[6]  = shiftright_int16_simd256(y256[6],3);
    y256[7]  = shiftright_int16_simd256(y256[7],3);
  }

  _mm_empty();
  _m_empty();


}
#endif

#ifndef __AVX2__
void idft64(int16_t *x,int16_t *y,int scale)
{

  simd_q15_t xtmp[16],ytmp[16],*tw64a_128=(simd_q15_t *)tw64,*tw64b_128=(simd_q15_t *)tw64c,*x128=(simd_q15_t *)x,*y128=(simd_q15_t *)y;


#ifdef D64STATS
  time_stats_t ts_t,ts_d,ts_b;

  reset_meas(&ts_t);
  reset_meas(&ts_d);
  reset_meas(&ts_b);
  start_meas(&ts_t);
#endif


  transpose16_ooff(x128,xtmp,4);
  transpose16_ooff(x128+4,xtmp+1,4);
  transpose16_ooff(x128+8,xtmp+2,4);
  transpose16_ooff(x128+12,xtmp+3,4);


#ifdef D64STATS
  stop_meas(&ts_t);
  start_meas(&ts_d);
#endif


  idft16((int16_t*)(xtmp),(int16_t*)ytmp);
  idft16((int16_t*)(xtmp+4),(int16_t*)(ytmp+4));
  idft16((int16_t*)(xtmp+8),(int16_t*)(ytmp+8));
  idft16((int16_t*)(xtmp+12),(int16_t*)(ytmp+12));


#ifdef D64STATS
  stop_meas(&ts_d);
  start_meas(&ts_b);
#endif


  ibfly4_16(ytmp,ytmp+4,ytmp+8,ytmp+12,
            y128,y128+4,y128+8,y128+12,
            tw64a_128,tw64a_128+4,tw64a_128+8,
            tw64b_128,tw64b_128+4,tw64b_128+8);
  ibfly4_16(ytmp+1,ytmp+5,ytmp+9,ytmp+13,
            y128+1,y128+5,y128+9,y128+13,
            tw64a_128+1,tw64a_128+5,tw64a_128+9,
            tw64b_128+1,tw64b_128+5,tw64b_128+9);

  ibfly4_16(ytmp+2,ytmp+6,ytmp+10,ytmp+14,
            y128+2,y128+6,y128+10,y128+14,
            tw64a_128+2,tw64a_128+6,tw64a_128+10,
            tw64b_128+2,tw64b_128+6,tw64b_128+10);

  ibfly4_16(ytmp+3,ytmp+7,ytmp+11,ytmp+15,
            y128+3,y128+7,y128+11,y128+15,
            tw64a_128+3,tw64a_128+7,tw64a_128+11,
            tw64b_128+3,tw64b_128+7,tw64b_128+11);

#ifdef D64STATS
  stop_meas(&ts_b);
  printf("t: %llu cycles, d: %llu cycles, b: %llu cycles\n",ts_t.diff,ts_d.diff,ts_b.diff);
#endif


  if (scale>0) {

    y128[0]  = shiftright_int16(y128[0],3);
    y128[1]  = shiftright_int16(y128[1],3);
    y128[2]  = shiftright_int16(y128[2],3);
    y128[3]  = shiftright_int16(y128[3],3);
    y128[4]  = shiftright_int16(y128[4],3);
    y128[5]  = shiftright_int16(y128[5],3);
    y128[6]  = shiftright_int16(y128[6],3);
    y128[7]  = shiftright_int16(y128[7],3);
    y128[8]  = shiftright_int16(y128[8],3);
    y128[9]  = shiftright_int16(y128[9],3);
    y128[10] = shiftright_int16(y128[10],3);
    y128[11] = shiftright_int16(y128[11],3);
    y128[12] = shiftright_int16(y128[12],3);
    y128[13] = shiftright_int16(y128[13],3);
    y128[14] = shiftright_int16(y128[14],3);
    y128[15] = shiftright_int16(y128[15],3);

  }

  _mm_empty();
  _m_empty();

}

#else // __AVX2__
void idft64(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[16],ytmp[16],*tw64a_256=(simd256_q15_t *)tw64,*tw64b_256=(simd256_q15_t *)tw64c,*x256=(simd256_q15_t *)x,*y256=(simd256_q15_t *)y;
  register simd256_q15_t xintl0,xintl1,xintl2,xintl3,xintl4,xintl5,xintl6,xintl7;
  simd256_q15_t const perm_mask = _mm256_set_epi32(7, 3, 5, 1, 6, 2, 4, 0);


#ifdef D64STATS
  time_stats_t ts_t,ts_d,ts_b;

  reset_meas(&ts_t);
  reset_meas(&ts_d);
  reset_meas(&ts_b);
  start_meas(&ts_t);
#endif

#ifdef D64STATS
  stop_meas(&ts_t);
  start_meas(&ts_d);
#endif

  xintl0 = _mm256_permutevar8x32_epi32(x256[0],perm_mask);  // x0  x4  x1  x5  x2  x6  x3  x7
  xintl1 = _mm256_permutevar8x32_epi32(x256[1],perm_mask);  // x8  x12 x9  x13 x10 x14 x11 x15
  xintl2 = _mm256_permutevar8x32_epi32(x256[2],perm_mask);  // x16 x20 x17 x21 x18 x22 x19 x23
  xintl3 = _mm256_permutevar8x32_epi32(x256[3],perm_mask);  // x24 x28 x25 x29 x26 x30 x27 x31
  xintl4 = _mm256_permutevar8x32_epi32(x256[4],perm_mask);  // x24 x28 x25 x29 x26 x30 x27 x31
  xintl5 = _mm256_permutevar8x32_epi32(x256[5],perm_mask);  // x24 x28 x25 x29 x26 x30 x27 x31
  xintl6 = _mm256_permutevar8x32_epi32(x256[6],perm_mask);  // x24 x28 x25 x29 x26 x30 x27 x31
  xintl7 = _mm256_permutevar8x32_epi32(x256[7],perm_mask);  // x24 x28 x25 x29 x26 x30 x27 x31

  xtmp[0] = _mm256_unpacklo_epi64(xintl0,xintl1);        // x0  x4  x8  x12 x1  x5  x9  x13
  xtmp[4] = _mm256_unpackhi_epi64(xintl0,xintl1);        // x2  x6  x10 x14 x3  x7  x11 x15
  xtmp[1] = _mm256_unpacklo_epi64(xintl2,xintl3);        // x16 x20 x24 x28 x17 x21 x25 x29
  xtmp[5] = _mm256_unpackhi_epi64(xintl2,xintl3);        // x18 x22 x26 x30 x19 x23 x27 x31
  xtmp[2] = _mm256_unpacklo_epi64(xintl4,xintl5);        // x32 x36 x40 x44 x33 x37 x41 x45
  xtmp[6] = _mm256_unpackhi_epi64(xintl4,xintl5);        // x34 x38 x42 x46 x35 x39 x43 x47
  xtmp[3] = _mm256_unpacklo_epi64(xintl6,xintl7);        // x48 x52 x56 x60 x49 x53 x57 x61
  xtmp[7] = _mm256_unpackhi_epi64(xintl6,xintl7);        // x50 x54 x58 x62 x51 x55 x59 x63


  idft16_simd256((int16_t*)(xtmp),(int16_t*)ytmp);
  // [y0  y1  y2  y3  y16 y17 y18 y19]
  // [y4  y5  y6  y7  y20 y21 y22 y23]
  // [y8  y9  y10 y11 y24 y25 y26 y27]
  // [y12 y13 y14 y15 y28 y29 y30 y31]

  idft16_simd256((int16_t*)(xtmp+4),(int16_t*)(ytmp+4));
  // [y32 y33 y34 y35 y48 y49 y50 y51]
  // [y36 y37 y38 y39 y52 y53 y54 y55]
  // [y40 y41 y42 y43 y56 y57 y58 y59]
  // [y44 y45 y46 y47 y60 y61 y62 y63]

#ifdef D64STATS
  stop_meas(&ts_d);
  start_meas(&ts_b);
#endif


  ibfly4_16_256(ytmp,ytmp+2,ytmp+4,ytmp+6,
		y256,y256+2,y256+4,y256+6,
		tw64a_256,tw64a_256+2,tw64a_256+4,
		tw64b_256,tw64b_256+2,tw64b_256+4);
  // [y0  y1  y2  y3  y4  y5  y6  y7]
  // [y16 y17 y18 y19 y20 y21 y22 y23]
  // [y32 y33 y34 y35 y36 y37 y38 y39]
  // [y48 y49 y50 y51 y52 y53 y54 y55]

  ibfly4_16_256(ytmp+1,ytmp+3,ytmp+5,ytmp+7,
		y256+1,y256+3,y256+5,y256+7,
		tw64a_256+1,tw64a_256+3,tw64a_256+5,
		tw64b_256+1,tw64b_256+3,tw64b_256+5);
  // [y8  y9  y10 y11 y12 y13 y14 y15]
  // [y24 y25 y26 y27 y28 y29 y30 y31]
  // [y40 y41 y42 y43 y44 y45 y46 y47]
  // [y56 y57 y58 y59 y60 y61 y62 y63]


#ifdef D64STATS
  stop_meas(&ts_b);
  printf("t: %llu cycles, d: %llu cycles, b: %llu cycles\n",ts_t.diff,ts_d.diff,ts_b.diff);
#endif


  if (scale>0) {
    y256[0]  = shiftright_int16_simd256(y256[0],3);
    y256[1]  = shiftright_int16_simd256(y256[1],3);
    y256[2]  = shiftright_int16_simd256(y256[2],3);
    y256[3]  = shiftright_int16_simd256(y256[3],3);
    y256[4]  = shiftright_int16_simd256(y256[4],3);
    y256[5]  = shiftright_int16_simd256(y256[5],3);
    y256[6]  = shiftright_int16_simd256(y256[6],3);
    y256[7]  = shiftright_int16_simd256(y256[7],3);
  }

  _mm_empty();
  _m_empty();

}
#endif

int16_t tw128[128] __attribute__((aligned(32))) = {  32767,0,32727,-1608,32609,-3212,32412,-4808,32137,-6393,31785,-7962,31356,-9512,30851,-11039,30272,-12540,29621,-14010,28897,-15447,28105,-16846,27244,-18205,26318,-19520,25329,-20788,24278,-22005,23169,-23170,22004,-24279,20787,-25330,19519,-26319,18204,-27245,16845,-28106,15446,-28898,14009,-29622,12539,-30273,11038,-30852,9511,-31357,7961,-31786,6392,-32138,4807,-32413,3211,-32610,1607,-32728,0,-32767,-1608,-32728,-3212,-32610,-4808,-32413,-6393,-32138,-7962,-31786,-9512,-31357,-11039,-30852,-12540,-30273,-14010,-29622,-15447,-28898,-16846,-28106,-18205,-27245,-19520,-26319,-20788,-25330,-22005,-24279,-23170,-23170,-24279,-22005,-25330,-20788,-26319,-19520,-27245,-18205,-28106,-16846,-28898,-15447,-29622,-14010,-30273,-12540,-30852,-11039,-31357,-9512,-31786,-7962,-32138,-6393,-32413,-4808,-32610,-3212,-32728,-1608};

int16_t tw128a[128] __attribute__((aligned(32))) = { 32767,0,32727,1608,32609,3212,32412,4808,32137,6393,31785,7962,31356,9512,30851,11039,30272,12540,29621,14010,28897,15447,28105,16846,27244,18205,26318,19520,25329,20788,24278,22005,23169,23170,22004,24279,20787,25330,19519,26319,18204,27245,16845,28106,15446,28898,14009,29622,12539,30273,11038,30852,9511,31357,7961,31786,6392,32138,4807,32413,3211,32610,1607,32728,0,32767,-1608,32728,-3212,32610,-4808,32413,-6393,32138,-7962,31786,-9512,31357,-11039,30852,-12540,30273,-14010,29622,-15447,28898,-16846,28106,-18205,27245,-19520,26319,-20788,25330,-22005,24279,-23170,23170,-24279,22005,-25330,20788,-26319,19520,-27245,18205,-28106,16846,-28898,15447,-29622,14010,-30273,12540,-30852,11039,-31357,9512,-31786,7962,-32138,6393,-32413,4808,-32610,3212,-32728,1608};

int16_t tw128b[128] __attribute__((aligned(32))) = {0,32767,-1608,32727,-3212,32609,-4808,32412,-6393,32137,-7962,31785,-9512,31356,-11039,30851,-12540,30272,-14010,29621,-15447,28897,-16846,28105,-18205,27244,-19520,26318,-20788,25329,-22005,24278,-23170,23169,-24279,22004,-25330,20787,-26319,19519,-27245,18204,-28106,16845,-28898,15446,-29622,14009,-30273,12539,-30852,11038,-31357,9511,-31786,7961,-32138,6392,-32413,4807,-32610,3211,-32728,1607,-32767,0,-32728,-1608,-32610,-3212,-32413,-4808,-32138,-6393,-31786,-7962,-31357,-9512,-30852,-11039,-30273,-12540,-29622,-14010,-28898,-15447,-28106,-16846,-27245,-18205,-26319,-19520,-25330,-20788,-24279,-22005,-23170,-23170,-22005,-24279,-20788,-25330,-19520,-26319,-18205,-27245,-16846,-28106,-15447,-28898,-14010,-29622,-12540,-30273,-11039,-30852,-9512,-31357,-7962,-31786,-6393,-32138,-4808,-32413,-3212,-32610,-1608,-32728};

int16_t tw128c[128] __attribute__((aligned(32))) = {0,32767,1608,32727,3212,32609,4808,32412,6393,32137,7962,31785,9512,31356,11039,30851,12540,30272,14010,29621,15447,28897,16846,28105,18205,27244,19520,26318,20788,25329,22005,24278,23170,23169,24279,22004,25330,20787,26319,19519,27245,18204,28106,16845,28898,15446,29622,14009,30273,12539,30852,11038,31357,9511,31786,7961,32138,6392,32413,4807,32610,3211,32728,1607,32767,0,32728,-1608,32610,-3212,32413,-4808,32138,-6393,31786,-7962,31357,-9512,30852,-11039,30273,-12540,29622,-14010,28898,-15447,28106,-16846,27245,-18205,26319,-19520,25330,-20788,24279,-22005,23170,-23170,22005,-24279,20788,-25330,19520,-26319,18205,-27245,16846,-28106,15447,-28898,14010,-29622,12540,-30273,11039,-30852,9512,-31357,7962,-31786,6393,-32138,4808,-32413,3212,-32610,1608,-32728};

#ifndef __AVX2__
void dft128(int16_t *x,int16_t *y,int scale)
{

  simdshort_q15_t xtmp[64],*x64 = (simdshort_q15_t *)x;
  simd_q15_t ytmp[32],*tw128a_128p=(simd_q15_t *)tw128a,*tw128b_128p=(simd_q15_t *)tw128b,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i;
  simd_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16(ONE_OVER_SQRT2_Q15);


  transpose4_ooff(x64  ,xtmp,32);
  transpose4_ooff(x64+2,xtmp+1,32);
  transpose4_ooff(x64+4,xtmp+2,32);
  transpose4_ooff(x64+6,xtmp+3,32);
  transpose4_ooff(x64+8,xtmp+4,32);
  transpose4_ooff(x64+10,xtmp+5,32);
  transpose4_ooff(x64+12,xtmp+6,32);
  transpose4_ooff(x64+14,xtmp+7,32);
  transpose4_ooff(x64+16,xtmp+8,32);
  transpose4_ooff(x64+18,xtmp+9,32);
  transpose4_ooff(x64+20,xtmp+10,32);
  transpose4_ooff(x64+22,xtmp+11,32);
  transpose4_ooff(x64+24,xtmp+12,32);
  transpose4_ooff(x64+26,xtmp+13,32);
  transpose4_ooff(x64+28,xtmp+14,32);
  transpose4_ooff(x64+30,xtmp+15,32);
  transpose4_ooff(x64+32,xtmp+16,32);
  transpose4_ooff(x64+34,xtmp+17,32);
  transpose4_ooff(x64+36,xtmp+18,32);
  transpose4_ooff(x64+38,xtmp+19,32);
  transpose4_ooff(x64+40,xtmp+20,32);
  transpose4_ooff(x64+42,xtmp+21,32);
  transpose4_ooff(x64+44,xtmp+22,32);
  transpose4_ooff(x64+46,xtmp+23,32);
  transpose4_ooff(x64+48,xtmp+24,32);
  transpose4_ooff(x64+50,xtmp+25,32);
  transpose4_ooff(x64+52,xtmp+26,32);
  transpose4_ooff(x64+54,xtmp+27,32);
  transpose4_ooff(x64+56,xtmp+28,32);
  transpose4_ooff(x64+58,xtmp+29,32);
  transpose4_ooff(x64+60,xtmp+30,32);
  transpose4_ooff(x64+62,xtmp+31,32);

  dft64((int16_t*)(xtmp),(int16_t*)ytmp,1);
  dft64((int16_t*)(xtmp+32),(int16_t*)(ytmp+16),1);
  if (LOG_DUMPFLAG(DEBUG_DFT)) {
    LOG_M("dft128a.m","dfta",ytmp,64,1,1);
    LOG_M("dft128b.m","dftb",ytmp+16,64,1,1);
  }
  for (i=0; i<16; i++) {
    bfly2_16(ytmpp,ytmpp+16,
             y128p,y128p+16,
             tw128a_128p,
             tw128b_128p);
    tw128a_128p++;
    tw128b_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {

    y128[0] = mulhi_int16(y128[0],ONE_OVER_SQRT2_Q15_128);
    y128[1] = mulhi_int16(y128[1],ONE_OVER_SQRT2_Q15_128);
    y128[2] = mulhi_int16(y128[2],ONE_OVER_SQRT2_Q15_128);
    y128[3] = mulhi_int16(y128[3],ONE_OVER_SQRT2_Q15_128);
    y128[4] = mulhi_int16(y128[4],ONE_OVER_SQRT2_Q15_128);
    y128[5] = mulhi_int16(y128[5],ONE_OVER_SQRT2_Q15_128);
    y128[6] = mulhi_int16(y128[6],ONE_OVER_SQRT2_Q15_128);
    y128[7] = mulhi_int16(y128[7],ONE_OVER_SQRT2_Q15_128);
    y128[8] = mulhi_int16(y128[8],ONE_OVER_SQRT2_Q15_128);
    y128[9] = mulhi_int16(y128[9],ONE_OVER_SQRT2_Q15_128);
    y128[10] = mulhi_int16(y128[10],ONE_OVER_SQRT2_Q15_128);
    y128[11] = mulhi_int16(y128[11],ONE_OVER_SQRT2_Q15_128);
    y128[12] = mulhi_int16(y128[12],ONE_OVER_SQRT2_Q15_128);
    y128[13] = mulhi_int16(y128[13],ONE_OVER_SQRT2_Q15_128);
    y128[14] = mulhi_int16(y128[14],ONE_OVER_SQRT2_Q15_128);
    y128[15] = mulhi_int16(y128[15],ONE_OVER_SQRT2_Q15_128);
    y128[16] = mulhi_int16(y128[16],ONE_OVER_SQRT2_Q15_128);
    y128[17] = mulhi_int16(y128[17],ONE_OVER_SQRT2_Q15_128);
    y128[18] = mulhi_int16(y128[18],ONE_OVER_SQRT2_Q15_128);
    y128[19] = mulhi_int16(y128[19],ONE_OVER_SQRT2_Q15_128);
    y128[20] = mulhi_int16(y128[20],ONE_OVER_SQRT2_Q15_128);
    y128[21] = mulhi_int16(y128[21],ONE_OVER_SQRT2_Q15_128);
    y128[22] = mulhi_int16(y128[22],ONE_OVER_SQRT2_Q15_128);
    y128[23] = mulhi_int16(y128[23],ONE_OVER_SQRT2_Q15_128);
    y128[24] = mulhi_int16(y128[24],ONE_OVER_SQRT2_Q15_128);
    y128[25] = mulhi_int16(y128[25],ONE_OVER_SQRT2_Q15_128);
    y128[26] = mulhi_int16(y128[26],ONE_OVER_SQRT2_Q15_128);
    y128[27] = mulhi_int16(y128[27],ONE_OVER_SQRT2_Q15_128);
    y128[28] = mulhi_int16(y128[28],ONE_OVER_SQRT2_Q15_128);
    y128[29] = mulhi_int16(y128[29],ONE_OVER_SQRT2_Q15_128);
    y128[30] = mulhi_int16(y128[30],ONE_OVER_SQRT2_Q15_128);
    y128[31] = mulhi_int16(y128[31],ONE_OVER_SQRT2_Q15_128);


  }
  if (LOG_DUMPFLAG(DEBUG_DFT)) {
     LOG_M("dft128out.m","dft128",y,128,1,1);
     exit(-1);
  }
  _mm_empty();
  _m_empty();

}

#else // __AVX2__
void dft128(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[16],*x256 = (simd256_q15_t *)x;
  simd256_q15_t ytmp[16],*y256=(simd256_q15_t*)y;
  simd256_q15_t *tw128a_256p=(simd256_q15_t *)tw128a,*tw128b_256p=(simd256_q15_t *)tw128b,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i;
  simd256_q15_t ONE_OVER_SQRT2_Q15_256 = set1_int16_simd256(ONE_OVER_SQRT2_Q15);

  transpose4_ooff_simd256(x256  ,xtmp,8);
  transpose4_ooff_simd256(x256+2,xtmp+1,8);
  transpose4_ooff_simd256(x256+4,xtmp+2,8);
  transpose4_ooff_simd256(x256+6,xtmp+3,8);
  transpose4_ooff_simd256(x256+8,xtmp+4,8);
  transpose4_ooff_simd256(x256+10,xtmp+5,8);
  transpose4_ooff_simd256(x256+12,xtmp+6,8);
  transpose4_ooff_simd256(x256+14,xtmp+7,8);
  if (LOG_DUMPFLAG(DEBUG_DFT)) {  
     LOG_M("dft128ina_256.m","dftina",xtmp,64,1,1);
     LOG_M("dft128inb_256.m","dftinb",xtmp+8,64,1,1);
  }

  dft64((int16_t*)(xtmp),(int16_t*)ytmp,1);
  dft64((int16_t*)(xtmp+8),(int16_t*)(ytmp+8),1);
  if (LOG_DUMPFLAG(DEBUG_DFT)) {  
    LOG_M("dft128outa_256.m","dftouta",ytmp,64,1,1);
    LOG_M("dft128outb_256.m","dftoutb",ytmp+8,64,1,1);
  }

  for (i=0; i<8; i++) {
    bfly2_16_256(ytmpp,ytmpp+8,
		 y256p,y256p+8,
		 tw128a_256p,
		 tw128b_256p);
    tw128a_256p++;
    tw128b_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {

    y256[0] = mulhi_int16_simd256(y256[0],ONE_OVER_SQRT2_Q15_256);
    y256[1] = mulhi_int16_simd256(y256[1],ONE_OVER_SQRT2_Q15_256);
    y256[2] = mulhi_int16_simd256(y256[2],ONE_OVER_SQRT2_Q15_256);
    y256[3] = mulhi_int16_simd256(y256[3],ONE_OVER_SQRT2_Q15_256);
    y256[4] = mulhi_int16_simd256(y256[4],ONE_OVER_SQRT2_Q15_256);
    y256[5] = mulhi_int16_simd256(y256[5],ONE_OVER_SQRT2_Q15_256);
    y256[6] = mulhi_int16_simd256(y256[6],ONE_OVER_SQRT2_Q15_256);
    y256[7] = mulhi_int16_simd256(y256[7],ONE_OVER_SQRT2_Q15_256);
    y256[8] = mulhi_int16_simd256(y256[8],ONE_OVER_SQRT2_Q15_256);
    y256[9] = mulhi_int16_simd256(y256[9],ONE_OVER_SQRT2_Q15_256);
    y256[10] = mulhi_int16_simd256(y256[10],ONE_OVER_SQRT2_Q15_256);
    y256[11] = mulhi_int16_simd256(y256[11],ONE_OVER_SQRT2_Q15_256);
    y256[12] = mulhi_int16_simd256(y256[12],ONE_OVER_SQRT2_Q15_256);
    y256[13] = mulhi_int16_simd256(y256[13],ONE_OVER_SQRT2_Q15_256);
    y256[14] = mulhi_int16_simd256(y256[14],ONE_OVER_SQRT2_Q15_256);
    y256[15] = mulhi_int16_simd256(y256[15],ONE_OVER_SQRT2_Q15_256);

  }
  if (LOG_DUMPFLAG(DEBUG_DFT)) {  
   LOG_M("dft128.m","dft",y256,128,1,1);
   exit(-1);
  }
}

#endif

#ifndef __AVX2__
void idft128(int16_t *x,int16_t *y,int scale)
{

  simdshort_q15_t xtmp[64],*x64 = (simdshort_q15_t *)x;
  simd_q15_t ytmp[32],*tw128_128p=(simd_q15_t *)tw128,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i;
  simd_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16(ONE_OVER_SQRT2_Q15);


  transpose4_ooff(x64  ,xtmp,32);
  transpose4_ooff(x64+2,xtmp+1,32);
  transpose4_ooff(x64+4,xtmp+2,32);
  transpose4_ooff(x64+6,xtmp+3,32);
  transpose4_ooff(x64+8,xtmp+4,32);
  transpose4_ooff(x64+10,xtmp+5,32);
  transpose4_ooff(x64+12,xtmp+6,32);
  transpose4_ooff(x64+14,xtmp+7,32);
  transpose4_ooff(x64+16,xtmp+8,32);
  transpose4_ooff(x64+18,xtmp+9,32);
  transpose4_ooff(x64+20,xtmp+10,32);
  transpose4_ooff(x64+22,xtmp+11,32);
  transpose4_ooff(x64+24,xtmp+12,32);
  transpose4_ooff(x64+26,xtmp+13,32);
  transpose4_ooff(x64+28,xtmp+14,32);
  transpose4_ooff(x64+30,xtmp+15,32);
  transpose4_ooff(x64+32,xtmp+16,32);
  transpose4_ooff(x64+34,xtmp+17,32);
  transpose4_ooff(x64+36,xtmp+18,32);
  transpose4_ooff(x64+38,xtmp+19,32);
  transpose4_ooff(x64+40,xtmp+20,32);
  transpose4_ooff(x64+42,xtmp+21,32);
  transpose4_ooff(x64+44,xtmp+22,32);
  transpose4_ooff(x64+46,xtmp+23,32);
  transpose4_ooff(x64+48,xtmp+24,32);
  transpose4_ooff(x64+50,xtmp+25,32);
  transpose4_ooff(x64+52,xtmp+26,32);
  transpose4_ooff(x64+54,xtmp+27,32);
  transpose4_ooff(x64+56,xtmp+28,32);
  transpose4_ooff(x64+58,xtmp+29,32);
  transpose4_ooff(x64+60,xtmp+30,32);
  transpose4_ooff(x64+62,xtmp+31,32);

  idft64((int16_t*)(xtmp),(int16_t*)ytmp,1);
  idft64((int16_t*)(xtmp+32),(int16_t*)(ytmp+16),1);


  for (i=0; i<16; i++) {
    ibfly2(ytmpp,ytmpp+16,
           y128p,y128p+16,
           tw128_128p);
    tw128_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {

    y128[0]  = mulhi_int16(y128[0],ONE_OVER_SQRT2_Q15_128);
    y128[1]  = mulhi_int16(y128[1],ONE_OVER_SQRT2_Q15_128);
    y128[2]  = mulhi_int16(y128[2],ONE_OVER_SQRT2_Q15_128);
    y128[3]  = mulhi_int16(y128[3],ONE_OVER_SQRT2_Q15_128);
    y128[4]  = mulhi_int16(y128[4],ONE_OVER_SQRT2_Q15_128);
    y128[5]  = mulhi_int16(y128[5],ONE_OVER_SQRT2_Q15_128);
    y128[6]  = mulhi_int16(y128[6],ONE_OVER_SQRT2_Q15_128);
    y128[7]  = mulhi_int16(y128[7],ONE_OVER_SQRT2_Q15_128);
    y128[8]  = mulhi_int16(y128[8],ONE_OVER_SQRT2_Q15_128);
    y128[9]  = mulhi_int16(y128[9],ONE_OVER_SQRT2_Q15_128);
    y128[10] = mulhi_int16(y128[10],ONE_OVER_SQRT2_Q15_128);
    y128[11] = mulhi_int16(y128[11],ONE_OVER_SQRT2_Q15_128);
    y128[12] = mulhi_int16(y128[12],ONE_OVER_SQRT2_Q15_128);
    y128[13] = mulhi_int16(y128[13],ONE_OVER_SQRT2_Q15_128);
    y128[14] = mulhi_int16(y128[14],ONE_OVER_SQRT2_Q15_128);
    y128[15] = mulhi_int16(y128[15],ONE_OVER_SQRT2_Q15_128);
    y128[16] = mulhi_int16(y128[16],ONE_OVER_SQRT2_Q15_128);
    y128[17] = mulhi_int16(y128[17],ONE_OVER_SQRT2_Q15_128);
    y128[18] = mulhi_int16(y128[18],ONE_OVER_SQRT2_Q15_128);
    y128[19] = mulhi_int16(y128[19],ONE_OVER_SQRT2_Q15_128);
    y128[20] = mulhi_int16(y128[20],ONE_OVER_SQRT2_Q15_128);
    y128[21] = mulhi_int16(y128[21],ONE_OVER_SQRT2_Q15_128);
    y128[22] = mulhi_int16(y128[22],ONE_OVER_SQRT2_Q15_128);
    y128[23] = mulhi_int16(y128[23],ONE_OVER_SQRT2_Q15_128);
    y128[24] = mulhi_int16(y128[24],ONE_OVER_SQRT2_Q15_128);
    y128[25] = mulhi_int16(y128[25],ONE_OVER_SQRT2_Q15_128);
    y128[26] = mulhi_int16(y128[26],ONE_OVER_SQRT2_Q15_128);
    y128[27] = mulhi_int16(y128[27],ONE_OVER_SQRT2_Q15_128);
    y128[28] = mulhi_int16(y128[28],ONE_OVER_SQRT2_Q15_128);
    y128[29] = mulhi_int16(y128[29],ONE_OVER_SQRT2_Q15_128);
    y128[30] = mulhi_int16(y128[30],ONE_OVER_SQRT2_Q15_128);
    y128[31] = mulhi_int16(y128[31],ONE_OVER_SQRT2_Q15_128);

  }

  _mm_empty();
  _m_empty();

}

#else // __AVX2__
void idft128(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[16],*x256 = (simd256_q15_t *)x;
  simd256_q15_t ytmp[16],*y256=(simd256_q15_t*)y;
  simd256_q15_t *tw128_256p=(simd256_q15_t *)tw128,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i;
  simd256_q15_t ONE_OVER_SQRT2_Q15_256 = set1_int16_simd256(ONE_OVER_SQRT2_Q15);


  transpose4_ooff_simd256(x256  ,xtmp,8);
  transpose4_ooff_simd256(x256+2,xtmp+1,8);
  transpose4_ooff_simd256(x256+4,xtmp+2,8);
  transpose4_ooff_simd256(x256+6,xtmp+3,8);
  transpose4_ooff_simd256(x256+8,xtmp+4,8);
  transpose4_ooff_simd256(x256+10,xtmp+5,8);
  transpose4_ooff_simd256(x256+12,xtmp+6,8);
  transpose4_ooff_simd256(x256+14,xtmp+7,8);

  idft64((int16_t*)(xtmp),(int16_t*)ytmp,1);
  idft64((int16_t*)(xtmp+8),(int16_t*)(ytmp+8),1);


  for (i=0; i<8; i++) {
    ibfly2_256(ytmpp,ytmpp+8,
	       y256p,y256p+8,
	       tw128_256p);
    tw128_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {

    y256[0] = mulhi_int16_simd256(y256[0],ONE_OVER_SQRT2_Q15_256);
    y256[1] = mulhi_int16_simd256(y256[1],ONE_OVER_SQRT2_Q15_256);
    y256[2] = mulhi_int16_simd256(y256[2],ONE_OVER_SQRT2_Q15_256);
    y256[3] = mulhi_int16_simd256(y256[3],ONE_OVER_SQRT2_Q15_256);
    y256[4] = mulhi_int16_simd256(y256[4],ONE_OVER_SQRT2_Q15_256);
    y256[5] = mulhi_int16_simd256(y256[5],ONE_OVER_SQRT2_Q15_256);
    y256[6] = mulhi_int16_simd256(y256[6],ONE_OVER_SQRT2_Q15_256);
    y256[7] = mulhi_int16_simd256(y256[7],ONE_OVER_SQRT2_Q15_256);
    y256[8] = mulhi_int16_simd256(y256[8],ONE_OVER_SQRT2_Q15_256);
    y256[9] = mulhi_int16_simd256(y256[9],ONE_OVER_SQRT2_Q15_256);
    y256[10] = mulhi_int16_simd256(y256[10],ONE_OVER_SQRT2_Q15_256);
    y256[11] = mulhi_int16_simd256(y256[11],ONE_OVER_SQRT2_Q15_256);
    y256[12] = mulhi_int16_simd256(y256[12],ONE_OVER_SQRT2_Q15_256);
    y256[13] = mulhi_int16_simd256(y256[13],ONE_OVER_SQRT2_Q15_256);
    y256[14] = mulhi_int16_simd256(y256[14],ONE_OVER_SQRT2_Q15_256);
    y256[15] = mulhi_int16_simd256(y256[15],ONE_OVER_SQRT2_Q15_256);

  }

}

#endif

int16_t tw256[384] __attribute__((aligned(32))) = {  32767,0,32757,-805,32727,-1608,32678,-2411,32609,-3212,32520,-4012,32412,-4808,32284,-5602,32137,-6393,31970,-7180,31785,-7962,31580,-8740,31356,-9512,31113,-10279,30851,-11039,30571,-11793,30272,-12540,29955,-13279,29621,-14010,29268,-14733,28897,-15447,28510,-16151,28105,-16846,27683,-17531,27244,-18205,26789,-18868,26318,-19520,25831,-20160,25329,-20788,24811,-21403,24278,-22005,23731,-22595,23169,-23170,22594,-23732,22004,-24279,21402,-24812,20787,-25330,20159,-25832,19519,-26319,18867,-26790,18204,-27245,17530,-27684,16845,-28106,16150,-28511,15446,-28898,14732,-29269,14009,-29622,13278,-29956,12539,-30273,11792,-30572,11038,-30852,10278,-31114,9511,-31357,8739,-31581,7961,-31786,7179,-31971,6392,-32138,5601,-32285,4807,-32413,4011,-32521,3211,-32610,2410,-32679,1607,-32728,804,-32758,
                                                     32767,0,32727,-1608,32609,-3212,32412,-4808,32137,-6393,31785,-7962,31356,-9512,30851,-11039,30272,-12540,29621,-14010,28897,-15447,28105,-16846,27244,-18205,26318,-19520,25329,-20788,24278,-22005,23169,-23170,22004,-24279,20787,-25330,19519,-26319,18204,-27245,16845,-28106,15446,-28898,14009,-29622,12539,-30273,11038,-30852,9511,-31357,7961,-31786,6392,-32138,4807,-32413,3211,-32610,1607,-32728,0,-32767,-1608,-32728,-3212,-32610,-4808,-32413,-6393,-32138,-7962,-31786,-9512,-31357,-11039,-30852,-12540,-30273,-14010,-29622,-15447,-28898,-16846,-28106,-18205,-27245,-19520,-26319,-20788,-25330,-22005,-24279,-23170,-23170,-24279,-22005,-25330,-20788,-26319,-19520,-27245,-18205,-28106,-16846,-28898,-15447,-29622,-14010,-30273,-12540,-30852,-11039,-31357,-9512,-31786,-7962,-32138,-6393,-32413,-4808,-32610,-3212,-32728,-1608,
                                                     32767,0,32678,-2411,32412,-4808,31970,-7180,31356,-9512,30571,-11793,29621,-14010,28510,-16151,27244,-18205,25831,-20160,24278,-22005,22594,-23732,20787,-25330,18867,-26790,16845,-28106,14732,-29269,12539,-30273,10278,-31114,7961,-31786,5601,-32285,3211,-32610,804,-32758,-1608,-32728,-4012,-32521,-6393,-32138,-8740,-31581,-11039,-30852,-13279,-29956,-15447,-28898,-17531,-27684,-19520,-26319,-21403,-24812,-23170,-23170,-24812,-21403,-26319,-19520,-27684,-17531,-28898,-15447,-29956,-13279,-30852,-11039,-31581,-8740,-32138,-6393,-32521,-4012,-32728,-1608,-32758,804,-32610,3211,-32285,5601,-31786,7961,-31114,10278,-30273,12539,-29269,14732,-28106,16845,-26790,18867,-25330,20787,-23732,22594,-22005,24278,-20160,25831,-18205,27244,-16151,28510,-14010,29621,-11793,30571,-9512,31356,-7180,31970,-4808,32412,-2411,32678
                                                  };

int16_t tw256a[384] __attribute__((aligned(32))) = { 32767,0,32757,804,32727,1607,32678,2410,32609,3211,32520,4011,32412,4807,32284,5601,32137,6392,31970,7179,31785,7961,31580,8739,31356,9511,31113,10278,30851,11038,30571,11792,30272,12539,29955,13278,29621,14009,29268,14732,28897,15446,28510,16150,28105,16845,27683,17530,27244,18204,26789,18867,26318,19519,25831,20159,25329,20787,24811,21402,24278,22004,23731,22594,23169,23169,22594,23731,22004,24278,21402,24811,20787,25329,20159,25831,19519,26318,18867,26789,18204,27244,17530,27683,16845,28105,16150,28510,15446,28897,14732,29268,14009,29621,13278,29955,12539,30272,11792,30571,11038,30851,10278,31113,9511,31356,8739,31580,7961,31785,7179,31970,6392,32137,5601,32284,4807,32412,4011,32520,3211,32609,2410,32678,1607,32727,804,32757,
                                                     32767,0,32727,1607,32609,3211,32412,4807,32137,6392,31785,7961,31356,9511,30851,11038,30272,12539,29621,14009,28897,15446,28105,16845,27244,18204,26318,19519,25329,20787,24278,22004,23169,23169,22004,24278,20787,25329,19519,26318,18204,27244,16845,28105,15446,28897,14009,29621,12539,30272,11038,30851,9511,31356,7961,31785,6392,32137,4807,32412,3211,32609,1607,32727,0,32767,-1608,32727,-3212,32609,-4808,32412,-6393,32137,-7962,31785,-9512,31356,-11039,30851,-12540,30272,-14010,29621,-15447,28897,-16846,28105,-18205,27244,-19520,26318,-20788,25329,-22005,24278,-23170,23169,-24279,22004,-25330,20787,-26319,19519,-27245,18204,-28106,16845,-28898,15446,-29622,14009,-30273,12539,-30852,11038,-31357,9511,-31786,7961,-32138,6392,-32413,4807,-32610,3211,-32728,1607,
                                                     32767,0,32678,2410,32412,4807,31970,7179,31356,9511,30571,11792,29621,14009,28510,16150,27244,18204,25831,20159,24278,22004,22594,23731,20787,25329,18867,26789,16845,28105,14732,29268,12539,30272,10278,31113,7961,31785,5601,32284,3211,32609,804,32757,-1608,32727,-4012,32520,-6393,32137,-8740,31580,-11039,30851,-13279,29955,-15447,28897,-17531,27683,-19520,26318,-21403,24811,-23170,23169,-24812,21402,-26319,19519,-27684,17530,-28898,15446,-29956,13278,-30852,11038,-31581,8739,-32138,6392,-32521,4011,-32728,1607,-32758,-805,-32610,-3212,-32285,-5602,-31786,-7962,-31114,-10279,-30273,-12540,-29269,-14733,-28106,-16846,-26790,-18868,-25330,-20788,-23732,-22595,-22005,-24279,-20160,-25832,-18205,-27245,-16151,-28511,-14010,-29622,-11793,-30572,-9512,-31357,-7180,-31971,-4808,-32413,-2411,-32679
                                                   };

int16_t tw256b[384] __attribute__((aligned(32))) = {0,32767,-805,32757,-1608,32727,-2411,32678,-3212,32609,-4012,32520,-4808,32412,-5602,32284,-6393,32137,-7180,31970,-7962,31785,-8740,31580,-9512,31356,-10279,31113,-11039,30851,-11793,30571,-12540,30272,-13279,29955,-14010,29621,-14733,29268,-15447,28897,-16151,28510,-16846,28105,-17531,27683,-18205,27244,-18868,26789,-19520,26318,-20160,25831,-20788,25329,-21403,24811,-22005,24278,-22595,23731,-23170,23169,-23732,22594,-24279,22004,-24812,21402,-25330,20787,-25832,20159,-26319,19519,-26790,18867,-27245,18204,-27684,17530,-28106,16845,-28511,16150,-28898,15446,-29269,14732,-29622,14009,-29956,13278,-30273,12539,-30572,11792,-30852,11038,-31114,10278,-31357,9511,-31581,8739,-31786,7961,-31971,7179,-32138,6392,-32285,5601,-32413,4807,-32521,4011,-32610,3211,-32679,2410,-32728,1607,-32758,804,
                                                    0,32767,-1608,32727,-3212,32609,-4808,32412,-6393,32137,-7962,31785,-9512,31356,-11039,30851,-12540,30272,-14010,29621,-15447,28897,-16846,28105,-18205,27244,-19520,26318,-20788,25329,-22005,24278,-23170,23169,-24279,22004,-25330,20787,-26319,19519,-27245,18204,-28106,16845,-28898,15446,-29622,14009,-30273,12539,-30852,11038,-31357,9511,-31786,7961,-32138,6392,-32413,4807,-32610,3211,-32728,1607,-32767,0,-32728,-1608,-32610,-3212,-32413,-4808,-32138,-6393,-31786,-7962,-31357,-9512,-30852,-11039,-30273,-12540,-29622,-14010,-28898,-15447,-28106,-16846,-27245,-18205,-26319,-19520,-25330,-20788,-24279,-22005,-23170,-23170,-22005,-24279,-20788,-25330,-19520,-26319,-18205,-27245,-16846,-28106,-15447,-28898,-14010,-29622,-12540,-30273,-11039,-30852,-9512,-31357,-7962,-31786,-6393,-32138,-4808,-32413,-3212,-32610,-1608,-32728,
                                                    0,32767,-2411,32678,-4808,32412,-7180,31970,-9512,31356,-11793,30571,-14010,29621,-16151,28510,-18205,27244,-20160,25831,-22005,24278,-23732,22594,-25330,20787,-26790,18867,-28106,16845,-29269,14732,-30273,12539,-31114,10278,-31786,7961,-32285,5601,-32610,3211,-32758,804,-32728,-1608,-32521,-4012,-32138,-6393,-31581,-8740,-30852,-11039,-29956,-13279,-28898,-15447,-27684,-17531,-26319,-19520,-24812,-21403,-23170,-23170,-21403,-24812,-19520,-26319,-17531,-27684,-15447,-28898,-13279,-29956,-11039,-30852,-8740,-31581,-6393,-32138,-4012,-32521,-1608,-32728,804,-32758,3211,-32610,5601,-32285,7961,-31786,10278,-31114,12539,-30273,14732,-29269,16845,-28106,18867,-26790,20787,-25330,22594,-23732,24278,-22005,25831,-20160,27244,-18205,28510,-16151,29621,-14010,30571,-11793,31356,-9512,31970,-7180,32412,-4808,32678,-2411
                                                   };
#ifndef __AVX2__
void dft256(int16_t *x,int16_t *y,int scale)
{

  simd_q15_t xtmp[64],ytmp[64],*tw256a_128p=(simd_q15_t *)tw256a,*tw256b_128p=(simd_q15_t *)tw256b,*x128=(simd_q15_t *)x,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i;

#ifdef D256STATS
  time_stats_t ts_t,ts_d,ts_b;

  reset_meas(&ts_t);
  reset_meas(&ts_d);
  reset_meas(&ts_b);
  start_meas(&ts_t);
#endif
  /*
  for (i=0,j=0;i<64;i+=4,j++) {
    transpose16_ooff(x128+i,xtmp+j,16);
  }
  */
  transpose16_ooff(x128+0,xtmp+0,16);
  transpose16_ooff(x128+4,xtmp+1,16);
  transpose16_ooff(x128+8,xtmp+2,16);
  transpose16_ooff(x128+12,xtmp+3,16);
  transpose16_ooff(x128+16,xtmp+4,16);
  transpose16_ooff(x128+20,xtmp+5,16);
  transpose16_ooff(x128+24,xtmp+6,16);
  transpose16_ooff(x128+28,xtmp+7,16);
  transpose16_ooff(x128+32,xtmp+8,16);
  transpose16_ooff(x128+36,xtmp+9,16);
  transpose16_ooff(x128+40,xtmp+10,16);
  transpose16_ooff(x128+44,xtmp+11,16);
  transpose16_ooff(x128+48,xtmp+12,16);
  transpose16_ooff(x128+52,xtmp+13,16);
  transpose16_ooff(x128+56,xtmp+14,16);
  transpose16_ooff(x128+60,xtmp+15,16);

#ifdef D256STATS
  stop_meas(&ts_t);
  start_meas(&ts_d);
#endif

  dft64((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  dft64((int16_t*)(xtmp+16),(int16_t*)(ytmp+16),1);
  dft64((int16_t*)(xtmp+32),(int16_t*)(ytmp+32),1);
  dft64((int16_t*)(xtmp+48),(int16_t*)(ytmp+48),1);

#ifdef D256STATS
  stop_meas(&ts_d);
  start_meas(&ts_b);
#endif

  for (i=0; i<16; i+=4) {
    bfly4_16(ytmpp,ytmpp+16,ytmpp+32,ytmpp+48,
             y128p,y128p+16,y128p+32,y128p+48,
             tw256a_128p,tw256a_128p+16,tw256a_128p+32,
             tw256b_128p,tw256b_128p+16,tw256b_128p+32);
    bfly4_16(ytmpp+1,ytmpp+17,ytmpp+33,ytmpp+49,
             y128p+1,y128p+17,y128p+33,y128p+49,
             tw256a_128p+1,tw256a_128p+17,tw256a_128p+33,
             tw256b_128p+1,tw256b_128p+17,tw256b_128p+33);
    bfly4_16(ytmpp+2,ytmpp+18,ytmpp+34,ytmpp+50,
             y128p+2,y128p+18,y128p+34,y128p+50,
             tw256a_128p+2,tw256a_128p+18,tw256a_128p+34,
             tw256b_128p+2,tw256b_128p+18,tw256b_128p+34);
    bfly4_16(ytmpp+3,ytmpp+19,ytmpp+35,ytmpp+51,
             y128p+3,y128p+19,y128p+35,y128p+51,
             tw256a_128p+3,tw256a_128p+19,tw256a_128p+35,
             tw256b_128p+3,tw256b_128p+19,tw256b_128p+35);
    tw256a_128p+=4;
    tw256b_128p+=4;
    y128p+=4;
    ytmpp+=4;
  }

#ifdef D256STATS
  stop_meas(&ts_b);
  printf("t: %llu cycles, d: %llu cycles, b: %llu cycles\n",ts_t.diff,ts_d.diff,ts_b.diff);
#endif

  if (scale>0) {

    for (i=0; i<4; i++) {
      y128[0]  = shiftright_int16(y128[0],1);
      y128[1]  = shiftright_int16(y128[1],1);
      y128[2]  = shiftright_int16(y128[2],1);
      y128[3]  = shiftright_int16(y128[3],1);
      y128[4]  = shiftright_int16(y128[4],1);
      y128[5]  = shiftright_int16(y128[5],1);
      y128[6]  = shiftright_int16(y128[6],1);
      y128[7]  = shiftright_int16(y128[7],1);
      y128[8]  = shiftright_int16(y128[8],1);
      y128[9]  = shiftright_int16(y128[9],1);
      y128[10] = shiftright_int16(y128[10],1);
      y128[11] = shiftright_int16(y128[11],1);
      y128[12] = shiftright_int16(y128[12],1);
      y128[13] = shiftright_int16(y128[13],1);
      y128[14] = shiftright_int16(y128[14],1);
      y128[15] = shiftright_int16(y128[15],1);

      y128+=16;
    }

  }

  _mm_empty();
  _m_empty();

}



void idft256(int16_t *x,int16_t *y,int scale)
{

  simd_q15_t xtmp[64],ytmp[64],*tw256_128p=(simd_q15_t *)tw256,*x128=(simd_q15_t *)x,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i,j;

  for (i=0,j=0; i<64; i+=4,j++) {
    transpose16_ooff(x128+i,xtmp+j,16);
  }


  idft64((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  idft64((int16_t*)(xtmp+16),(int16_t*)(ytmp+16),1);
  idft64((int16_t*)(xtmp+32),(int16_t*)(ytmp+32),1);
  idft64((int16_t*)(xtmp+48),(int16_t*)(ytmp+48),1);

  for (i=0; i<16; i++) {
    ibfly4(ytmpp,ytmpp+16,ytmpp+32,ytmpp+48,
           y128p,y128p+16,y128p+32,y128p+48,
           tw256_128p,tw256_128p+16,tw256_128p+32);
    tw256_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0; i<4; i++) {
      y128[0]  = shiftright_int16(y128[0],1);
      y128[1]  = shiftright_int16(y128[1],1);
      y128[2]  = shiftright_int16(y128[2],1);
      y128[3]  = shiftright_int16(y128[3],1);
      y128[4]  = shiftright_int16(y128[4],1);
      y128[5]  = shiftright_int16(y128[5],1);
      y128[6]  = shiftright_int16(y128[6],1);
      y128[7]  = shiftright_int16(y128[7],1);
      y128[8]  = shiftright_int16(y128[8],1);
      y128[9]  = shiftright_int16(y128[9],1);
      y128[10] = shiftright_int16(y128[10],1);
      y128[11] = shiftright_int16(y128[11],1);
      y128[12] = shiftright_int16(y128[12],1);
      y128[13] = shiftright_int16(y128[13],1);
      y128[14] = shiftright_int16(y128[14],1);
      y128[15] = shiftright_int16(y128[15],1);

      y128+=16;
    }

  }

  _mm_empty();
  _m_empty();

}

#else //__AVX2__

void dft256(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[32],ytmp[32],*tw256a_256p=(simd256_q15_t *)tw256a,*tw256b_256p=(simd256_q15_t *)tw256b,*x256=(simd256_q15_t *)x,*y256=(simd256_q15_t *)y,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i;

  transpose16_ooff_simd256(x256+0,xtmp+0,8);
  transpose16_ooff_simd256(x256+4,xtmp+1,8);
  transpose16_ooff_simd256(x256+8,xtmp+2,8);
  transpose16_ooff_simd256(x256+12,xtmp+3,8);
  transpose16_ooff_simd256(x256+16,xtmp+4,8);
  transpose16_ooff_simd256(x256+20,xtmp+5,8);
  transpose16_ooff_simd256(x256+24,xtmp+6,8);
  transpose16_ooff_simd256(x256+28,xtmp+7,8);
  /*
  char vname[10];
  for (i=0;i<32;i++) {
    sprintf(vname,"xtmp%d",i);
    print_shorts256(vname,(int16_t*)(xtmp+i));
  }
  exit(-1);*/

  dft64((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  dft64((int16_t*)(xtmp+8),(int16_t*)(ytmp+8),1);
  dft64((int16_t*)(xtmp+16),(int16_t*)(ytmp+16),1);
  dft64((int16_t*)(xtmp+24),(int16_t*)(ytmp+24),1);


  bfly4_16_256(ytmpp,ytmpp+8,ytmpp+16,ytmpp+24,
	       y256p,y256p+8,y256p+16,y256p+24,
	       tw256a_256p,tw256a_256p+8,tw256a_256p+16,
	       tw256b_256p,tw256b_256p+8,tw256b_256p+16);
  bfly4_16_256(ytmpp+1,ytmpp+9,ytmpp+17,ytmpp+25,
	       y256p+1,y256p+9,y256p+17,y256p+25,
	       tw256a_256p+1,tw256a_256p+9,tw256a_256p+17,
	       tw256b_256p+1,tw256b_256p+9,tw256b_256p+17);
  bfly4_16_256(ytmpp+2,ytmpp+10,ytmpp+18,ytmpp+26,
	       y256p+2,y256p+10,y256p+18,y256p+26,
	       tw256a_256p+2,tw256a_256p+10,tw256a_256p+18,
	       tw256b_256p+2,tw256b_256p+10,tw256b_256p+18);
  bfly4_16_256(ytmpp+3,ytmpp+11,ytmpp+19,ytmpp+27,
	       y256p+3,y256p+11,y256p+19,y256p+27,
	       tw256a_256p+3,tw256a_256p+11,tw256a_256p+19,
	       tw256b_256p+3,tw256b_256p+11,tw256b_256p+19);
  bfly4_16_256(ytmpp+4,ytmpp+12,ytmpp+20,ytmpp+28,
	       y256p+4,y256p+12,y256p+20,y256p+28,
	       tw256a_256p+4,tw256a_256p+12,tw256a_256p+20,
	       tw256b_256p+4,tw256b_256p+12,tw256b_256p+20);
  bfly4_16_256(ytmpp+5,ytmpp+13,ytmpp+21,ytmpp+29,
	       y256p+5,y256p+13,y256p+21,y256p+29,
	       tw256a_256p+5,tw256a_256p+13,tw256a_256p+21,
	       tw256b_256p+5,tw256b_256p+13,tw256b_256p+21);
  bfly4_16_256(ytmpp+6,ytmpp+14,ytmpp+22,ytmpp+30,
	       y256p+6,y256p+14,y256p+22,y256p+30,
	       tw256a_256p+6,tw256a_256p+14,tw256a_256p+22,
	       tw256b_256p+6,tw256b_256p+14,tw256b_256p+22);
  bfly4_16_256(ytmpp+7,ytmpp+15,ytmpp+23,ytmpp+31,
	       y256p+7,y256p+15,y256p+23,y256p+31,
	       tw256a_256p+7,tw256a_256p+15,tw256a_256p+23,
	       tw256b_256p+7,tw256b_256p+15,tw256b_256p+23);

  if (scale>0) {

    for (i=0; i<2; i++) {
      y256[0]  = shiftright_int16_simd256(y256[0],1);
      y256[1]  = shiftright_int16_simd256(y256[1],1);
      y256[2]  = shiftright_int16_simd256(y256[2],1);
      y256[3]  = shiftright_int16_simd256(y256[3],1);
      y256[4]  = shiftright_int16_simd256(y256[4],1);
      y256[5]  = shiftright_int16_simd256(y256[5],1);
      y256[6]  = shiftright_int16_simd256(y256[6],1);
      y256[7]  = shiftright_int16_simd256(y256[7],1);
      y256[8]  = shiftright_int16_simd256(y256[8],1);
      y256[9]  = shiftright_int16_simd256(y256[9],1);
      y256[10] = shiftright_int16_simd256(y256[10],1);
      y256[11] = shiftright_int16_simd256(y256[11],1);
      y256[12] = shiftright_int16_simd256(y256[12],1);
      y256[13] = shiftright_int16_simd256(y256[13],1);
      y256[14] = shiftright_int16_simd256(y256[14],1);
      y256[15] = shiftright_int16_simd256(y256[15],1);

      y256+=16;
    }

  }

  _mm_empty();
  _m_empty();

}

void idft256(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[32],ytmp[32],*tw256_256p=(simd256_q15_t *)tw256,*x256=(simd256_q15_t *)x,*y256=(simd256_q15_t *)y,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i;

  transpose16_ooff_simd256(x256+0,xtmp+0,8);
  transpose16_ooff_simd256(x256+4,xtmp+1,8);
  transpose16_ooff_simd256(x256+8,xtmp+2,8);
  transpose16_ooff_simd256(x256+12,xtmp+3,8);
  transpose16_ooff_simd256(x256+16,xtmp+4,8);
  transpose16_ooff_simd256(x256+20,xtmp+5,8);
  transpose16_ooff_simd256(x256+24,xtmp+6,8);
  transpose16_ooff_simd256(x256+28,xtmp+7,8);
  
  idft64((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  idft64((int16_t*)(xtmp+8),(int16_t*)(ytmp+8),1);
  idft64((int16_t*)(xtmp+16),(int16_t*)(ytmp+16),1);
  idft64((int16_t*)(xtmp+24),(int16_t*)(ytmp+24),1);
  
  
  ibfly4_256(ytmpp,ytmpp+8,ytmpp+16,ytmpp+24,
	     y256p,y256p+8,y256p+16,y256p+24,
	     tw256_256p,tw256_256p+8,tw256_256p+16);

  ibfly4_256(ytmpp+1,ytmpp+9,ytmpp+17,ytmpp+25,
	     y256p+1,y256p+9,y256p+17,y256p+25,
	     tw256_256p+1,tw256_256p+9,tw256_256p+17);

  ibfly4_256(ytmpp+2,ytmpp+10,ytmpp+18,ytmpp+26,
	     y256p+2,y256p+10,y256p+18,y256p+26,
	     tw256_256p+2,tw256_256p+10,tw256_256p+18);

  ibfly4_256(ytmpp+3,ytmpp+11,ytmpp+19,ytmpp+27,
	     y256p+3,y256p+11,y256p+19,y256p+27,
	     tw256_256p+3,tw256_256p+11,tw256_256p+19);

  ibfly4_256(ytmpp+4,ytmpp+12,ytmpp+20,ytmpp+28,
	     y256p+4,y256p+12,y256p+20,y256p+28,
	     tw256_256p+4,tw256_256p+12,tw256_256p+20);

  ibfly4_256(ytmpp+5,ytmpp+13,ytmpp+21,ytmpp+29,
	     y256p+5,y256p+13,y256p+21,y256p+29,
	     tw256_256p+5,tw256_256p+13,tw256_256p+21);

  ibfly4_256(ytmpp+6,ytmpp+14,ytmpp+22,ytmpp+30,
	     y256p+6,y256p+14,y256p+22,y256p+30,
	     tw256_256p+6,tw256_256p+14,tw256_256p+22);

  ibfly4_256(ytmpp+7,ytmpp+15,ytmpp+23,ytmpp+31,
	     y256p+7,y256p+15,y256p+23,y256p+31,
	     tw256_256p+7,tw256_256p+15,tw256_256p+23);

  
  if (scale>0) {

    for (i=0; i<2; i++) {
      y256[0]  = shiftright_int16_simd256(y256[0],1);
      y256[1]  = shiftright_int16_simd256(y256[1],1);
      y256[2]  = shiftright_int16_simd256(y256[2],1);
      y256[3]  = shiftright_int16_simd256(y256[3],1);
      y256[4]  = shiftright_int16_simd256(y256[4],1);
      y256[5]  = shiftright_int16_simd256(y256[5],1);
      y256[6]  = shiftright_int16_simd256(y256[6],1);
      y256[7]  = shiftright_int16_simd256(y256[7],1);
      y256[8]  = shiftright_int16_simd256(y256[8],1);
      y256[9]  = shiftright_int16_simd256(y256[9],1);
      y256[10] = shiftright_int16_simd256(y256[10],1);
      y256[11] = shiftright_int16_simd256(y256[11],1);
      y256[12] = shiftright_int16_simd256(y256[12],1);
      y256[13] = shiftright_int16_simd256(y256[13],1);
      y256[14] = shiftright_int16_simd256(y256[14],1);
      y256[15] = shiftright_int16_simd256(y256[15],1);

      y256+=16;
    }

  }

  _mm_empty();
  _m_empty();

}

#endif
int16_t tw512[512] __attribute__((aligned(32))) = {
  32767,0,32764,-403,32757,-805,32744,-1207,32727,-1608,32705,-2010,32678,-2411,32646,-2812,32609,-3212,32567,-3612,32520,-4012,32468,-4410,32412,-4808,32350,-5206,32284,-5602,32213,-5998,32137,-6393,32056,-6787,31970,-7180,31880,-7572,31785,-7962,31684,-8352,31580,-8740,31470,-9127,31356,-9512,31236,-9896,31113,-10279,30984,-10660,30851,-11039,30713,-11417,30571,-11793,30424,-12167,30272,-12540,30116,-12910,29955,-13279,29790,-13646,29621,-14010,29446,-14373,29268,-14733,29085,-15091,28897,-15447,28706,-15800,28510,-16151,28309,-16500,28105,-16846,27896,-17190,27683,-17531,27466,-17869,27244,-18205,27019,-18538,26789,-18868,26556,-19195,26318,-19520,26077,-19841,25831,-20160,25582,-20475,25329,-20788,25072,-21097,24811,-21403,24546,-21706,24278,-22005,24006,-22302,23731,-22595,23452,-22884,23169,-23170,22883,-23453,22594,-23732,22301,-24007,22004,-24279,21705,-24547,21402,-24812,21096,-25073,20787,-25330,20474,-25583,20159,-25832,19840,-26078,19519,-26319,19194,-26557,18867,-26790,18537,-27020,18204,-27245,17868,-27467,17530,-27684,17189,-27897,16845,-28106,16499,-28310,16150,-28511,15799,-28707,15446,-28898,15090,-29086,14732,-29269,14372,-29447,14009,-29622,13645,-29791,13278,-29956,12909,-30117,12539,-30273,12166,-30425,11792,-30572,11416,-30714,11038,-30852,10659,-30985,10278,-31114,9895,-31237,9511,-31357,9126,-31471,8739,-31581,8351,-31685,7961,-31786,7571,-31881,7179,-31971,6786,-32057,6392,-32138,5997,-32214,5601,-32285,5205,-32351,4807,-32413,4409,-32469,4011,-32521,3611,-32568,3211,-32610,2811,-32647,2410,-32679,2009,-32706,1607,-32728,1206,-32745,804,-32758,402,-32765,0,-32767,-403,-32765,-805,-32758,-1207,-32745,-1608,-32728,-2010,-32706,-2411,-32679,-2812,-32647,-3212,-32610,-3612,-32568,-4012,-32521,-4410,-32469,-4808,-32413,-5206,-32351,-5602,-32285,-5998,-32214,-6393,-32138,-6787,-32057,-7180,-31971,-7572,-31881,-7962,-31786,-8352,-31685,-8740,-31581,-9127,-31471,-9512,-31357,-9896,-31237,-10279,-31114,-10660,-30985,-11039,-30852,-11417,-30714,-11793,-30572,-12167,-30425,-12540,-30273,-12910,-30117,-13279,-29956,-13646,-29791,-14010,-29622,-14373,-29447,-14733,-29269,-15091,-29086,-15447,-28898,-15800,-28707,-16151,-28511,-16500,-28310,-16846,-28106,-17190,-27897,-17531,-27684,-17869,-27467,-18205,-27245,-18538,-27020,-18868,-26790,-19195,-26557,-19520,-26319,-19841,-26078,-20160,-25832,-20475,-25583,-20788,-25330,-21097,-25073,-21403,-24812,-21706,-24547,-22005,-24279,-22302,-24007,-22595,-23732,-22884,-23453,-23170,-23170,-23453,-22884,-23732,-22595,-24007,-22302,-24279,-22005,-24547,-21706,-24812,-21403,-25073,-21097,-25330,-20788,-25583,-20475,-25832,-20160,-26078,-19841,-26319,-19520,-26557,-19195,-26790,-18868,-27020,-18538,-27245,-18205,-27467,-17869,-27684,-17531,-27897,-17190,-28106,-16846,-28310,-16500,-28511,-16151,-28707,-15800,-28898,-15447,-29086,-15091,-29269,-14733,-29447,-14373,-29622,-14010,-29791,-13646,-29956,-13279,-30117,-12910,-30273,-12540,-30425,-12167,-30572,-11793,-30714,-11417,-30852,-11039,-30985,-10660,-31114,-10279,-31237,-9896,-31357,-9512,-31471,-9127,-31581,-8740,-31685,-8352,-31786,-7962,-31881,-7572,-31971,-7180,-32057,-6787,-32138,-6393,-32214,-5998,-32285,-5602,-32351,-5206,-32413,-4808,-32469,-4410,-32521,-4012,-32568,-3612,-32610,-3212,-32647,-2812,-32679,-2411,-32706,-2010,-32728,-1608,-32745,-1207,-32758,-805,-32765,-403
};

int16_t tw512a[512] __attribute__((aligned(32))) = {
  32767,0,32764,403,32757,805,32744,1207,32727,1608,32705,2010,32678,2411,32646,2812,32609,3212,32567,3612,32520,4012,32468,4410,32412,4808,32350,5206,32284,5602,32213,5998,32137,6393,32056,6787,31970,7180,31880,7572,31785,7962,31684,8352,31580,8740,31470,9127,31356,9512,31236,9896,31113,10279,30984,10660,30851,11039,30713,11417,30571,11793,30424,12167,30272,12540,30116,12910,29955,13279,29790,13646,29621,14010,29446,14373,29268,14733,29085,15091,28897,15447,28706,15800,28510,16151,28309,16500,28105,16846,27896,17190,27683,17531,27466,17869,27244,18205,27019,18538,26789,18868,26556,19195,26318,19520,26077,19841,25831,20160,25582,20475,25329,20788,25072,21097,24811,21403,24546,21706,24278,22005,24006,22302,23731,22595,23452,22884,23169,23170,22883,23453,22594,23732,22301,24007,22004,24279,21705,24547,21402,24812,21096,25073,20787,25330,20474,25583,20159,25832,19840,26078,19519,26319,19194,26557,18867,26790,18537,27020,18204,27245,17868,27467,17530,27684,17189,27897,16845,28106,16499,28310,16150,28511,15799,28707,15446,28898,15090,29086,14732,29269,14372,29447,14009,29622,13645,29791,13278,29956,12909,30117,12539,30273,12166,30425,11792,30572,11416,30714,11038,30852,10659,30985,10278,31114,9895,31237,9511,31357,9126,31471,8739,31581,8351,31685,7961,31786,7571,31881,7179,31971,6786,32057,6392,32138,5997,32214,5601,32285,5205,32351,4807,32413,4409,32469,4011,32521,3611,32568,3211,32610,2811,32647,2410,32679,2009,32706,1607,32728,1206,32745,804,32758,402,32765,0,32767,-403,32765,-805,32758,-1207,32745,-1608,32728,-2010,32706,-2411,32679,-2812,32647,-3212,32610,-3612,32568,-4012,32521,-4410,32469,-4808,32413,-5206,32351,-5602,32285,-5998,32214,-6393,32138,-6787,32057,-7180,31971,-7572,31881,-7962,31786,-8352,31685,-8740,31581,-9127,31471,-9512,31357,-9896,31237,-10279,31114,-10660,30985,-11039,30852,-11417,30714,-11793,30572,-12167,30425,-12540,30273,-12910,30117,-13279,29956,-13646,29791,-14010,29622,-14373,29447,-14733,29269,-15091,29086,-15447,28898,-15800,28707,-16151,28511,-16500,28310,-16846,28106,-17190,27897,-17531,27684,-17869,27467,-18205,27245,-18538,27020,-18868,26790,-19195,26557,-19520,26319,-19841,26078,-20160,25832,-20475,25583,-20788,25330,-21097,25073,-21403,24812,-21706,24547,-22005,24279,-22302,24007,-22595,23732,-22884,23453,-23170,23170,-23453,22884,-23732,22595,-24007,22302,-24279,22005,-24547,21706,-24812,21403,-25073,21097,-25330,20788,-25583,20475,-25832,20160,-26078,19841,-26319,19520,-26557,19195,-26790,18868,-27020,18538,-27245,18205,-27467,17869,-27684,17531,-27897,17190,-28106,16846,-28310,16500,-28511,16151,-28707,15800,-28898,15447,-29086,15091,-29269,14733,-29447,14373,-29622,14010,-29791,13646,-29956,13279,-30117,12910,-30273,12540,-30425,12167,-30572,11793,-30714,11417,-30852,11039,-30985,10660,-31114,10279,-31237,9896,-31357,9512,-31471,9127,-31581,8740,-31685,8352,-31786,7962,-31881,7572,-31971,7180,-32057,6787,-32138,6393,-32214,5998,-32285,5602,-32351,5206,-32413,4808,-32469,4410,-32521,4012,-32568,3612,-32610,3212,-32647,2812,-32679,2411,-32706,2010,-32728,1608,-32745,1207,-32758,805,-32765,403
};



int16_t tw512b[512] __attribute__((aligned(32))) = {
  0,32767,-403,32764,-805,32757,-1207,32744,-1608,32727,-2010,32705,-2411,32678,-2812,32646,-3212,32609,-3612,32567,-4012,32520,-4410,32468,-4808,32412,-5206,32350,-5602,32284,-5998,32213,-6393,32137,-6787,32056,-7180,31970,-7572,31880,-7962,31785,-8352,31684,-8740,31580,-9127,31470,-9512,31356,-9896,31236,-10279,31113,-10660,30984,-11039,30851,-11417,30713,-11793,30571,-12167,30424,-12540,30272,-12910,30116,-13279,29955,-13646,29790,-14010,29621,-14373,29446,-14733,29268,-15091,29085,-15447,28897,-15800,28706,-16151,28510,-16500,28309,-16846,28105,-17190,27896,-17531,27683,-17869,27466,-18205,27244,-18538,27019,-18868,26789,-19195,26556,-19520,26318,-19841,26077,-20160,25831,-20475,25582,-20788,25329,-21097,25072,-21403,24811,-21706,24546,-22005,24278,-22302,24006,-22595,23731,-22884,23452,-23170,23169,-23453,22883,-23732,22594,-24007,22301,-24279,22004,-24547,21705,-24812,21402,-25073,21096,-25330,20787,-25583,20474,-25832,20159,-26078,19840,-26319,19519,-26557,19194,-26790,18867,-27020,18537,-27245,18204,-27467,17868,-27684,17530,-27897,17189,-28106,16845,-28310,16499,-28511,16150,-28707,15799,-28898,15446,-29086,15090,-29269,14732,-29447,14372,-29622,14009,-29791,13645,-29956,13278,-30117,12909,-30273,12539,-30425,12166,-30572,11792,-30714,11416,-30852,11038,-30985,10659,-31114,10278,-31237,9895,-31357,9511,-31471,9126,-31581,8739,-31685,8351,-31786,7961,-31881,7571,-31971,7179,-32057,6786,-32138,6392,-32214,5997,-32285,5601,-32351,5205,-32413,4807,-32469,4409,-32521,4011,-32568,3611,-32610,3211,-32647,2811,-32679,2410,-32706,2009,-32728,1607,-32745,1206,-32758,804,-32765,402,-32767,0,-32765,-403,-32758,-805,-32745,-1207,-32728,-1608,-32706,-2010,-32679,-2411,-32647,-2812,-32610,-3212,-32568,-3612,-32521,-4012,-32469,-4410,-32413,-4808,-32351,-5206,-32285,-5602,-32214,-5998,-32138,-6393,-32057,-6787,-31971,-7180,-31881,-7572,-31786,-7962,-31685,-8352,-31581,-8740,-31471,-9127,-31357,-9512,-31237,-9896,-31114,-10279,-30985,-10660,-30852,-11039,-30714,-11417,-30572,-11793,-30425,-12167,-30273,-12540,-30117,-12910,-29956,-13279,-29791,-13646,-29622,-14010,-29447,-14373,-29269,-14733,-29086,-15091,-28898,-15447,-28707,-15800,-28511,-16151,-28310,-16500,-28106,-16846,-27897,-17190,-27684,-17531,-27467,-17869,-27245,-18205,-27020,-18538,-26790,-18868,-26557,-19195,-26319,-19520,-26078,-19841,-25832,-20160,-25583,-20475,-25330,-20788,-25073,-21097,-24812,-21403,-24547,-21706,-24279,-22005,-24007,-22302,-23732,-22595,-23453,-22884,-23170,-23170,-22884,-23453,-22595,-23732,-22302,-24007,-22005,-24279,-21706,-24547,-21403,-24812,-21097,-25073,-20788,-25330,-20475,-25583,-20160,-25832,-19841,-26078,-19520,-26319,-19195,-26557,-18868,-26790,-18538,-27020,-18205,-27245,-17869,-27467,-17531,-27684,-17190,-27897,-16846,-28106,-16500,-28310,-16151,-28511,-15800,-28707,-15447,-28898,-15091,-29086,-14733,-29269,-14373,-29447,-14010,-29622,-13646,-29791,-13279,-29956,-12910,-30117,-12540,-30273,-12167,-30425,-11793,-30572,-11417,-30714,-11039,-30852,-10660,-30985,-10279,-31114,-9896,-31237,-9512,-31357,-9127,-31471,-8740,-31581,-8352,-31685,-7962,-31786,-7572,-31881,-7180,-31971,-6787,-32057,-6393,-32138,-5998,-32214,-5602,-32285,-5206,-32351,-4808,-32413,-4410,-32469,-4012,-32521,-3612,-32568,-3212,-32610,-2812,-32647,-2411,-32679,-2010,-32706,-1608,-32728,-1207,-32745,-805,-32758,-403,-32765
};

int16_t tw512c[512] __attribute__((aligned(32))) = {
  0,32767,403,32764,805,32757,1207,32744,1608,32727,2010,32705,2411,32678,2812,32646,3212,32609,3612,32567,4012,32520,4410,32468,4808,32412,5206,32350,5602,32284,5998,32213,6393,32137,6787,32056,7180,31970,7572,31880,7962,31785,8352,31684,8740,31580,9127,31470,9512,31356,9896,31236,10279,31113,10660,30984,11039,30851,11417,30713,11793,30571,12167,30424,12540,30272,12910,30116,13279,29955,13646,29790,14010,29621,14373,29446,14733,29268,15091,29085,15447,28897,15800,28706,16151,28510,16500,28309,16846,28105,17190,27896,17531,27683,17869,27466,18205,27244,18538,27019,18868,26789,19195,26556,19520,26318,19841,26077,20160,25831,20475,25582,20788,25329,21097,25072,21403,24811,21706,24546,22005,24278,22302,24006,22595,23731,22884,23452,23170,23169,23453,22883,23732,22594,24007,22301,24279,22004,24547,21705,24812,21402,25073,21096,25330,20787,25583,20474,25832,20159,26078,19840,26319,19519,26557,19194,26790,18867,27020,18537,27245,18204,27467,17868,27684,17530,27897,17189,28106,16845,28310,16499,28511,16150,28707,15799,28898,15446,29086,15090,29269,14732,29447,14372,29622,14009,29791,13645,29956,13278,30117,12909,30273,12539,30425,12166,30572,11792,30714,11416,30852,11038,30985,10659,31114,10278,31237,9895,31357,9511,31471,9126,31581,8739,31685,8351,31786,7961,31881,7571,31971,7179,32057,6786,32138,6392,32214,5997,32285,5601,32351,5205,32413,4807,32469,4409,32521,4011,32568,3611,32610,3211,32647,2811,32679,2410,32706,2009,32728,1607,32745,1206,32758,804,32765,402,32767,0,32765,-403,32758,-805,32745,-1207,32728,-1608,32706,-2010,32679,-2411,32647,-2812,32610,-3212,32568,-3612,32521,-4012,32469,-4410,32413,-4808,32351,-5206,32285,-5602,32214,-5998,32138,-6393,32057,-6787,31971,-7180,31881,-7572,31786,-7962,31685,-8352,31581,-8740,31471,-9127,31357,-9512,31237,-9896,31114,-10279,30985,-10660,30852,-11039,30714,-11417,30572,-11793,30425,-12167,30273,-12540,30117,-12910,29956,-13279,29791,-13646,29622,-14010,29447,-14373,29269,-14733,29086,-15091,28898,-15447,28707,-15800,28511,-16151,28310,-16500,28106,-16846,27897,-17190,27684,-17531,27467,-17869,27245,-18205,27020,-18538,26790,-18868,26557,-19195,26319,-19520,26078,-19841,25832,-20160,25583,-20475,25330,-20788,25073,-21097,24812,-21403,24547,-21706,24279,-22005,24007,-22302,23732,-22595,23453,-22884,23170,-23170,22884,-23453,22595,-23732,22302,-24007,22005,-24279,21706,-24547,21403,-24812,21097,-25073,20788,-25330,20475,-25583,20160,-25832,19841,-26078,19520,-26319,19195,-26557,18868,-26790,18538,-27020,18205,-27245,17869,-27467,17531,-27684,17190,-27897,16846,-28106,16500,-28310,16151,-28511,15800,-28707,15447,-28898,15091,-29086,14733,-29269,14373,-29447,14010,-29622,13646,-29791,13279,-29956,12910,-30117,12540,-30273,12167,-30425,11793,-30572,11417,-30714,11039,-30852,10660,-30985,10279,-31114,9896,-31237,9512,-31357,9127,-31471,8740,-31581,8352,-31685,7962,-31786,7572,-31881,7180,-31971,6787,-32057,6393,-32138,5998,-32214,5602,-32285,5206,-32351,4808,-32413,4410,-32469,4012,-32521,3612,-32568,3212,-32610,2812,-32647,2411,-32679,2010,-32706,1608,-32728,1207,-32745,805,-32758,403,-32765
};

#ifndef __AVX2__
void dft512(int16_t *x,int16_t *y,int scale)
{

  simdshort_q15_t xtmp[256],*xtmpp,*x64 = (simdshort_q15_t *)x;
  simd_q15_t ytmp[128],*tw512a_128p=(simd_q15_t *)tw512a,*tw512b_128p=(simd_q15_t *)tw512b,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i;
  simd_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16(ONE_OVER_SQRT2_Q15);

  xtmpp = xtmp;

  for (i=0; i<4; i++) {
    transpose4_ooff(x64  ,xtmpp,128);
    transpose4_ooff(x64+2,xtmpp+1,128);
    transpose4_ooff(x64+4,xtmpp+2,128);
    transpose4_ooff(x64+6,xtmpp+3,128);
    transpose4_ooff(x64+8,xtmpp+4,128);
    transpose4_ooff(x64+10,xtmpp+5,128);
    transpose4_ooff(x64+12,xtmpp+6,128);
    transpose4_ooff(x64+14,xtmpp+7,128);
    transpose4_ooff(x64+16,xtmpp+8,128);
    transpose4_ooff(x64+18,xtmpp+9,128);
    transpose4_ooff(x64+20,xtmpp+10,128);
    transpose4_ooff(x64+22,xtmpp+11,128);
    transpose4_ooff(x64+24,xtmpp+12,128);
    transpose4_ooff(x64+26,xtmpp+13,128);
    transpose4_ooff(x64+28,xtmpp+14,128);
    transpose4_ooff(x64+30,xtmpp+15,128);
    transpose4_ooff(x64+32,xtmpp+16,128);
    transpose4_ooff(x64+34,xtmpp+17,128);
    transpose4_ooff(x64+36,xtmpp+18,128);
    transpose4_ooff(x64+38,xtmpp+19,128);
    transpose4_ooff(x64+40,xtmpp+20,128);
    transpose4_ooff(x64+42,xtmpp+21,128);
    transpose4_ooff(x64+44,xtmpp+22,128);
    transpose4_ooff(x64+46,xtmpp+23,128);
    transpose4_ooff(x64+48,xtmpp+24,128);
    transpose4_ooff(x64+50,xtmpp+25,128);
    transpose4_ooff(x64+52,xtmpp+26,128);
    transpose4_ooff(x64+54,xtmpp+27,128);
    transpose4_ooff(x64+56,xtmpp+28,128);
    transpose4_ooff(x64+58,xtmpp+29,128);
    transpose4_ooff(x64+60,xtmpp+30,128);
    transpose4_ooff(x64+62,xtmpp+31,128);
    x64+=64;
    xtmpp+=32;
  }

  dft256((int16_t*)(xtmp),(int16_t*)ytmp,1);
  dft256((int16_t*)(xtmp+128),(int16_t*)(ytmp+64),1);


  for (i=0; i<64; i+=8) {
    bfly2_16(ytmpp,ytmpp+64,
             y128p,y128p+64,
             tw512a_128p,
             tw512b_128p);
    bfly2_16(ytmpp+1,ytmpp+65,
             y128p+1,y128p+65,
             tw512a_128p+1,
             tw512b_128p+1);
    bfly2_16(ytmpp+2,ytmpp+66,
             y128p+2,y128p+66,
             tw512a_128p+2,
             tw512b_128p+2);
    bfly2_16(ytmpp+3,ytmpp+67,
             y128p+3,y128p+67,
             tw512a_128p+3,
             tw512b_128p+3);
    bfly2_16(ytmpp+4,ytmpp+68,
             y128p+4,y128p+68,
             tw512a_128p+4,
             tw512b_128p+4);
    bfly2_16(ytmpp+5,ytmpp+69,
             y128p+5,y128p+69,
             tw512a_128p+5,
             tw512b_128p+5);
    bfly2_16(ytmpp+6,ytmpp+70,
             y128p+6,y128p+70,
             tw512a_128p+6,
             tw512b_128p+6);
    bfly2_16(ytmpp+7,ytmpp+71,
             y128p+7,y128p+71,
             tw512a_128p+7,
             tw512b_128p+7);
    tw512a_128p+=8;
    tw512b_128p+=8;
    y128p+=8;
    ytmpp+=8;
  }

  if (scale>0) {
    y128p = y128;

    for (i=0; i<8; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT2_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT2_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT2_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT2_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT2_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT2_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT2_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT2_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT2_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT2_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT2_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT2_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT2_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT2_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT2_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT2_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

void idft512(int16_t *x,int16_t *y,int scale)
{

  simdshort_q15_t xtmp[256],*xtmpp,*x64 = (simdshort_q15_t *)x;
  simd_q15_t ytmp[128],*tw512_128p=(simd_q15_t *)tw512,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i;
  simd_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16(ONE_OVER_SQRT2_Q15);

  xtmpp = xtmp;

  for (i=0; i<4; i++) {
    transpose4_ooff(x64  ,xtmpp,128);
    transpose4_ooff(x64+2,xtmpp+1,128);
    transpose4_ooff(x64+4,xtmpp+2,128);
    transpose4_ooff(x64+6,xtmpp+3,128);
    transpose4_ooff(x64+8,xtmpp+4,128);
    transpose4_ooff(x64+10,xtmpp+5,128);
    transpose4_ooff(x64+12,xtmpp+6,128);
    transpose4_ooff(x64+14,xtmpp+7,128);
    transpose4_ooff(x64+16,xtmpp+8,128);
    transpose4_ooff(x64+18,xtmpp+9,128);
    transpose4_ooff(x64+20,xtmpp+10,128);
    transpose4_ooff(x64+22,xtmpp+11,128);
    transpose4_ooff(x64+24,xtmpp+12,128);
    transpose4_ooff(x64+26,xtmpp+13,128);
    transpose4_ooff(x64+28,xtmpp+14,128);
    transpose4_ooff(x64+30,xtmpp+15,128);
    transpose4_ooff(x64+32,xtmpp+16,128);
    transpose4_ooff(x64+34,xtmpp+17,128);
    transpose4_ooff(x64+36,xtmpp+18,128);
    transpose4_ooff(x64+38,xtmpp+19,128);
    transpose4_ooff(x64+40,xtmpp+20,128);
    transpose4_ooff(x64+42,xtmpp+21,128);
    transpose4_ooff(x64+44,xtmpp+22,128);
    transpose4_ooff(x64+46,xtmpp+23,128);
    transpose4_ooff(x64+48,xtmpp+24,128);
    transpose4_ooff(x64+50,xtmpp+25,128);
    transpose4_ooff(x64+52,xtmpp+26,128);
    transpose4_ooff(x64+54,xtmpp+27,128);
    transpose4_ooff(x64+56,xtmpp+28,128);
    transpose4_ooff(x64+58,xtmpp+29,128);
    transpose4_ooff(x64+60,xtmpp+30,128);
    transpose4_ooff(x64+62,xtmpp+31,128);
    x64+=64;
    xtmpp+=32;
  }

  idft256((int16_t*)(xtmp),(int16_t*)ytmp,1);
  idft256((int16_t*)(xtmp+128),(int16_t*)(ytmp+64),1);


  for (i=0; i<64; i++) {
    ibfly2(ytmpp,ytmpp+64,
           y128p,y128p+64,
           tw512_128p);
    tw512_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {
    y128p = y128;

    for (i=0; i<8; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT2_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT2_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT2_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT2_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT2_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT2_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT2_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT2_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT2_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT2_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT2_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT2_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT2_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT2_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT2_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT2_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

#else //__AVX2__

void dft512(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[64],*x256 = (simd256_q15_t *)x;
  simd256_q15_t ytmp[64],*y256=(simd256_q15_t*)y;
  simd256_q15_t *tw512_256p=(simd256_q15_t*)tw512,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i;
  simd256_q15_t ONE_OVER_SQRT2_Q15_256 = set1_int16_simd256(ONE_OVER_SQRT2_Q15);


  transpose4_ooff_simd256(x256  ,xtmp,32);
  transpose4_ooff_simd256(x256+2,xtmp+1,32);
  transpose4_ooff_simd256(x256+4,xtmp+2,32);
  transpose4_ooff_simd256(x256+6,xtmp+3,32);
  transpose4_ooff_simd256(x256+8,xtmp+4,32);
  transpose4_ooff_simd256(x256+10,xtmp+5,32);
  transpose4_ooff_simd256(x256+12,xtmp+6,32);
  transpose4_ooff_simd256(x256+14,xtmp+7,32);
  transpose4_ooff_simd256(x256+16,xtmp+8,32);
  transpose4_ooff_simd256(x256+18,xtmp+9,32);
  transpose4_ooff_simd256(x256+20,xtmp+10,32);
  transpose4_ooff_simd256(x256+22,xtmp+11,32);
  transpose4_ooff_simd256(x256+24,xtmp+12,32);
  transpose4_ooff_simd256(x256+26,xtmp+13,32);
  transpose4_ooff_simd256(x256+28,xtmp+14,32);
  transpose4_ooff_simd256(x256+30,xtmp+15,32);
  transpose4_ooff_simd256(x256+32,xtmp+16,32);
  transpose4_ooff_simd256(x256+34,xtmp+17,32);
  transpose4_ooff_simd256(x256+36,xtmp+18,32);
  transpose4_ooff_simd256(x256+38,xtmp+19,32);
  transpose4_ooff_simd256(x256+40,xtmp+20,32);
  transpose4_ooff_simd256(x256+42,xtmp+21,32);
  transpose4_ooff_simd256(x256+44,xtmp+22,32);
  transpose4_ooff_simd256(x256+46,xtmp+23,32);
  transpose4_ooff_simd256(x256+48,xtmp+24,32);
  transpose4_ooff_simd256(x256+50,xtmp+25,32);
  transpose4_ooff_simd256(x256+52,xtmp+26,32);
  transpose4_ooff_simd256(x256+54,xtmp+27,32);
  transpose4_ooff_simd256(x256+56,xtmp+28,32);
  transpose4_ooff_simd256(x256+58,xtmp+29,32);
  transpose4_ooff_simd256(x256+60,xtmp+30,32);
  transpose4_ooff_simd256(x256+62,xtmp+31,32);

  dft256((int16_t*)(xtmp),(int16_t*)ytmp,1);
  dft256((int16_t*)(xtmp+32),(int16_t*)(ytmp+32),1);


  for (i=0; i<32; i++) {
    bfly2_256(ytmpp,ytmpp+32,
	      y256p,y256p+32,
	      tw512_256p);
    tw512_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0;i<4;i++) {
      y256[0] = mulhi_int16_simd256(y256[0],ONE_OVER_SQRT2_Q15_256);
      y256[1] = mulhi_int16_simd256(y256[1],ONE_OVER_SQRT2_Q15_256);
      y256[2] = mulhi_int16_simd256(y256[2],ONE_OVER_SQRT2_Q15_256);
      y256[3] = mulhi_int16_simd256(y256[3],ONE_OVER_SQRT2_Q15_256);
      y256[4] = mulhi_int16_simd256(y256[4],ONE_OVER_SQRT2_Q15_256);
      y256[5] = mulhi_int16_simd256(y256[5],ONE_OVER_SQRT2_Q15_256);
      y256[6] = mulhi_int16_simd256(y256[6],ONE_OVER_SQRT2_Q15_256);
      y256[7] = mulhi_int16_simd256(y256[7],ONE_OVER_SQRT2_Q15_256);
      y256[8] = mulhi_int16_simd256(y256[8],ONE_OVER_SQRT2_Q15_256);
      y256[9] = mulhi_int16_simd256(y256[9],ONE_OVER_SQRT2_Q15_256);
      y256[10] = mulhi_int16_simd256(y256[10],ONE_OVER_SQRT2_Q15_256);
      y256[11] = mulhi_int16_simd256(y256[11],ONE_OVER_SQRT2_Q15_256);
      y256[12] = mulhi_int16_simd256(y256[12],ONE_OVER_SQRT2_Q15_256);
      y256[13] = mulhi_int16_simd256(y256[13],ONE_OVER_SQRT2_Q15_256);
      y256[14] = mulhi_int16_simd256(y256[14],ONE_OVER_SQRT2_Q15_256);
      y256[15] = mulhi_int16_simd256(y256[15],ONE_OVER_SQRT2_Q15_256);
      y256+=16;
    }
  }

}

void idft512(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[64],*x256 = (simd256_q15_t *)x;
  simd256_q15_t ytmp[64],*y256=(simd256_q15_t*)y;
  simd256_q15_t *tw512_256p=(simd256_q15_t *)tw512,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i;
  simd256_q15_t ONE_OVER_SQRT2_Q15_256 = set1_int16_simd256(ONE_OVER_SQRT2_Q15);


  transpose4_ooff_simd256(x256  ,xtmp,32);
  transpose4_ooff_simd256(x256+2,xtmp+1,32);
  transpose4_ooff_simd256(x256+4,xtmp+2,32);
  transpose4_ooff_simd256(x256+6,xtmp+3,32);
  transpose4_ooff_simd256(x256+8,xtmp+4,32);
  transpose4_ooff_simd256(x256+10,xtmp+5,32);
  transpose4_ooff_simd256(x256+12,xtmp+6,32);
  transpose4_ooff_simd256(x256+14,xtmp+7,32);
  transpose4_ooff_simd256(x256+16,xtmp+8,32);
  transpose4_ooff_simd256(x256+18,xtmp+9,32);
  transpose4_ooff_simd256(x256+20,xtmp+10,32);
  transpose4_ooff_simd256(x256+22,xtmp+11,32);
  transpose4_ooff_simd256(x256+24,xtmp+12,32);
  transpose4_ooff_simd256(x256+26,xtmp+13,32);
  transpose4_ooff_simd256(x256+28,xtmp+14,32);
  transpose4_ooff_simd256(x256+30,xtmp+15,32);
  transpose4_ooff_simd256(x256+32,xtmp+16,32);
  transpose4_ooff_simd256(x256+34,xtmp+17,32);
  transpose4_ooff_simd256(x256+36,xtmp+18,32);
  transpose4_ooff_simd256(x256+38,xtmp+19,32);
  transpose4_ooff_simd256(x256+40,xtmp+20,32);
  transpose4_ooff_simd256(x256+42,xtmp+21,32);
  transpose4_ooff_simd256(x256+44,xtmp+22,32);
  transpose4_ooff_simd256(x256+46,xtmp+23,32);
  transpose4_ooff_simd256(x256+48,xtmp+24,32);
  transpose4_ooff_simd256(x256+50,xtmp+25,32);
  transpose4_ooff_simd256(x256+52,xtmp+26,32);
  transpose4_ooff_simd256(x256+54,xtmp+27,32);
  transpose4_ooff_simd256(x256+56,xtmp+28,32);
  transpose4_ooff_simd256(x256+58,xtmp+29,32);
  transpose4_ooff_simd256(x256+60,xtmp+30,32);
  transpose4_ooff_simd256(x256+62,xtmp+31,32);

  idft256((int16_t*)(xtmp),(int16_t*)ytmp,1);
  idft256((int16_t*)(xtmp+32),(int16_t*)(ytmp+32),1);


  for (i=0; i<32; i++) {
    ibfly2_256(ytmpp,ytmpp+32,
	       y256p,y256p+32,
	       tw512_256p);
    tw512_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0;i<4;i++) {
      y256[0] = mulhi_int16_simd256(y256[0],ONE_OVER_SQRT2_Q15_256);
      y256[1] = mulhi_int16_simd256(y256[1],ONE_OVER_SQRT2_Q15_256);
      y256[2] = mulhi_int16_simd256(y256[2],ONE_OVER_SQRT2_Q15_256);
      y256[3] = mulhi_int16_simd256(y256[3],ONE_OVER_SQRT2_Q15_256);
      y256[4] = mulhi_int16_simd256(y256[4],ONE_OVER_SQRT2_Q15_256);
      y256[5] = mulhi_int16_simd256(y256[5],ONE_OVER_SQRT2_Q15_256);
      y256[6] = mulhi_int16_simd256(y256[6],ONE_OVER_SQRT2_Q15_256);
      y256[7] = mulhi_int16_simd256(y256[7],ONE_OVER_SQRT2_Q15_256);
      y256[8] = mulhi_int16_simd256(y256[8],ONE_OVER_SQRT2_Q15_256);
      y256[9] = mulhi_int16_simd256(y256[9],ONE_OVER_SQRT2_Q15_256);
      y256[10] = mulhi_int16_simd256(y256[10],ONE_OVER_SQRT2_Q15_256);
      y256[11] = mulhi_int16_simd256(y256[11],ONE_OVER_SQRT2_Q15_256);
      y256[12] = mulhi_int16_simd256(y256[12],ONE_OVER_SQRT2_Q15_256);
      y256[13] = mulhi_int16_simd256(y256[13],ONE_OVER_SQRT2_Q15_256);
      y256[14] = mulhi_int16_simd256(y256[14],ONE_OVER_SQRT2_Q15_256);
      y256[15] = mulhi_int16_simd256(y256[15],ONE_OVER_SQRT2_Q15_256);
      y256+=16;
    }
  }

}

#endif

int16_t tw1024[1536] __attribute__((aligned(32)));

#ifndef __AVX2__
void dft1024(int16_t *x,int16_t *y,int scale)
{

  simd_q15_t xtmp[256],ytmp[256],*tw1024_128p=(simd_q15_t *)tw1024,*x128=(simd_q15_t *)x,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i,j;

  for (i=0,j=0; i<256; i+=4,j++) {
    transpose16_ooff(x128+i,xtmp+j,64);
  }


  dft256((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  dft256((int16_t*)(xtmp+64),(int16_t*)(ytmp+64),1);
  dft256((int16_t*)(xtmp+128),(int16_t*)(ytmp+128),1);
  dft256((int16_t*)(xtmp+192),(int16_t*)(ytmp+192),1);

  for (i=0; i<64; i++) {
    bfly4(ytmpp,ytmpp+64,ytmpp+128,ytmpp+192,
          y128p,y128p+64,y128p+128,y128p+192,
          tw1024_128p,tw1024_128p+64,tw1024_128p+128);
    tw1024_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0; i<16; i++) {
      y128[0]  = shiftright_int16(y128[0],1);
      y128[1]  = shiftright_int16(y128[1],1);
      y128[2]  = shiftright_int16(y128[2],1);
      y128[3]  = shiftright_int16(y128[3],1);
      y128[4]  = shiftright_int16(y128[4],1);
      y128[5]  = shiftright_int16(y128[5],1);
      y128[6]  = shiftright_int16(y128[6],1);
      y128[7]  = shiftright_int16(y128[7],1);
      y128[8]  = shiftright_int16(y128[8],1);
      y128[9]  = shiftright_int16(y128[9],1);
      y128[10] = shiftright_int16(y128[10],1);
      y128[11] = shiftright_int16(y128[11],1);
      y128[12] = shiftright_int16(y128[12],1);
      y128[13] = shiftright_int16(y128[13],1);
      y128[14] = shiftright_int16(y128[14],1);
      y128[15] = shiftright_int16(y128[15],1);

      y128+=16;
    }

  }

  _mm_empty();
  _m_empty();

}

void idft1024(int16_t *x,int16_t *y,int scale)
{

  simd_q15_t xtmp[256],ytmp[256],*tw1024_128p=(simd_q15_t *)tw1024,*x128=(simd_q15_t *)x,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i,j;

  for (i=0,j=0; i<256; i+=4,j++) {
    transpose16_ooff(x128+i,xtmp+j,64);
  }


  idft256((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  idft256((int16_t*)(xtmp+64),(int16_t*)(ytmp+64),1);
  idft256((int16_t*)(xtmp+128),(int16_t*)(ytmp+128),1);
  idft256((int16_t*)(xtmp+192),(int16_t*)(ytmp+192),1);

  for (i=0; i<64; i++) {
    ibfly4(ytmpp,ytmpp+64,ytmpp+128,ytmpp+192,
           y128p,y128p+64,y128p+128,y128p+192,
           tw1024_128p,tw1024_128p+64,tw1024_128p+128);
    tw1024_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0; i<16; i++) {
      y128[0]  = shiftright_int16(y128[0],1);
      y128[1]  = shiftright_int16(y128[1],1);
      y128[2]  = shiftright_int16(y128[2],1);
      y128[3]  = shiftright_int16(y128[3],1);
      y128[4]  = shiftright_int16(y128[4],1);
      y128[5]  = shiftright_int16(y128[5],1);
      y128[6]  = shiftright_int16(y128[6],1);
      y128[7]  = shiftright_int16(y128[7],1);
      y128[8]  = shiftright_int16(y128[8],1);
      y128[9]  = shiftright_int16(y128[9],1);
      y128[10] = shiftright_int16(y128[10],1);
      y128[11] = shiftright_int16(y128[11],1);
      y128[12] = shiftright_int16(y128[12],1);
      y128[13] = shiftright_int16(y128[13],1);
      y128[14] = shiftright_int16(y128[14],1);
      y128[15] = shiftright_int16(y128[15],1);

      y128+=16;
    }

  }

  _mm_empty();
  _m_empty();

}

#else //__AVX2__
void dft1024(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[128],ytmp[128],*tw1024_256p=(simd256_q15_t *)tw1024,*x256=(simd256_q15_t *)x,*y256=(simd256_q15_t *)y,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i,j;

  for (i=0,j=0; i<128; i+=4,j++) {
    transpose16_ooff_simd256(x256+i,xtmp+j,32);
  }


  dft256((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  dft256((int16_t*)(xtmp+32),(int16_t*)(ytmp+32),1);
  dft256((int16_t*)(xtmp+64),(int16_t*)(ytmp+64),1);
  dft256((int16_t*)(xtmp+96),(int16_t*)(ytmp+96),1);

  for (i=0; i<32; i++) {
    bfly4_256(ytmpp,ytmpp+32,ytmpp+64,ytmpp+96,
	      y256p,y256p+32,y256p+64,y256p+96,
	      tw1024_256p,tw1024_256p+32,tw1024_256p+64);
    tw1024_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0; i<8; i++) {
      y256[0]  = shiftright_int16_simd256(y256[0],1);
      y256[1]  = shiftright_int16_simd256(y256[1],1);
      y256[2]  = shiftright_int16_simd256(y256[2],1);
      y256[3]  = shiftright_int16_simd256(y256[3],1);
      y256[4]  = shiftright_int16_simd256(y256[4],1);
      y256[5]  = shiftright_int16_simd256(y256[5],1);
      y256[6]  = shiftright_int16_simd256(y256[6],1);
      y256[7]  = shiftright_int16_simd256(y256[7],1);
      y256[8]  = shiftright_int16_simd256(y256[8],1);
      y256[9]  = shiftright_int16_simd256(y256[9],1);
      y256[10] = shiftright_int16_simd256(y256[10],1);
      y256[11] = shiftright_int16_simd256(y256[11],1);
      y256[12] = shiftright_int16_simd256(y256[12],1);
      y256[13] = shiftright_int16_simd256(y256[13],1);
      y256[14] = shiftright_int16_simd256(y256[14],1);
      y256[15] = shiftright_int16_simd256(y256[15],1);

      y256+=16;
    }

  }

  _mm_empty();
  _m_empty();

}

void idft1024(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[128],ytmp[128],*tw1024_256p=(simd256_q15_t *)tw1024,*x256=(simd256_q15_t *)x,*y256=(simd256_q15_t *)y,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i,j;

  for (i=0,j=0; i<128; i+=4,j++) {
    transpose16_ooff_simd256(x256+i,xtmp+j,32);
  }


  idft256((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  idft256((int16_t*)(xtmp+32),(int16_t*)(ytmp+32),1);
  idft256((int16_t*)(xtmp+64),(int16_t*)(ytmp+64),1);
  idft256((int16_t*)(xtmp+96),(int16_t*)(ytmp+96),1);

  for (i=0; i<32; i++) {
    ibfly4_256(ytmpp,ytmpp+32,ytmpp+64,ytmpp+96,
	       y256p,y256p+32,y256p+64,y256p+96,
	       tw1024_256p,tw1024_256p+32,tw1024_256p+64);
    tw1024_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0; i<8; i++) {
      y256[0]  = shiftright_int16_simd256(y256[0],1);
      y256[1]  = shiftright_int16_simd256(y256[1],1);
      y256[2]  = shiftright_int16_simd256(y256[2],1);
      y256[3]  = shiftright_int16_simd256(y256[3],1);
      y256[4]  = shiftright_int16_simd256(y256[4],1);
      y256[5]  = shiftright_int16_simd256(y256[5],1);
      y256[6]  = shiftright_int16_simd256(y256[6],1);
      y256[7]  = shiftright_int16_simd256(y256[7],1);
      y256[8]  = shiftright_int16_simd256(y256[8],1);
      y256[9]  = shiftright_int16_simd256(y256[9],1);
      y256[10] = shiftright_int16_simd256(y256[10],1);
      y256[11] = shiftright_int16_simd256(y256[11],1);
      y256[12] = shiftright_int16_simd256(y256[12],1);
      y256[13] = shiftright_int16_simd256(y256[13],1);
      y256[14] = shiftright_int16_simd256(y256[14],1);
      y256[15] = shiftright_int16_simd256(y256[15],1);

      y256+=16;
    }

  }

  _mm_empty();
  _m_empty();

}
#endif

int16_t tw2048[2048] __attribute__((aligned(32)));

#ifndef __AVX2__
void dft2048(int16_t *x,int16_t *y,int scale)
{

  simdshort_q15_t xtmp[1024],*xtmpp,*x64 = (simdshort_q15_t *)x;
  simd_q15_t ytmp[512],*tw2048_128p=(simd_q15_t *)tw2048,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i;
  simd_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16(ONE_OVER_SQRT2_Q15);

  xtmpp = xtmp;

  for (i=0; i<16; i++) {
    transpose4_ooff(x64  ,xtmpp,512);
    transpose4_ooff(x64+2,xtmpp+1,512);
    transpose4_ooff(x64+4,xtmpp+2,512);
    transpose4_ooff(x64+6,xtmpp+3,512);
    transpose4_ooff(x64+8,xtmpp+4,512);
    transpose4_ooff(x64+10,xtmpp+5,512);
    transpose4_ooff(x64+12,xtmpp+6,512);
    transpose4_ooff(x64+14,xtmpp+7,512);
    transpose4_ooff(x64+16,xtmpp+8,512);
    transpose4_ooff(x64+18,xtmpp+9,512);
    transpose4_ooff(x64+20,xtmpp+10,512);
    transpose4_ooff(x64+22,xtmpp+11,512);
    transpose4_ooff(x64+24,xtmpp+12,512);
    transpose4_ooff(x64+26,xtmpp+13,512);
    transpose4_ooff(x64+28,xtmpp+14,512);
    transpose4_ooff(x64+30,xtmpp+15,512);
    transpose4_ooff(x64+32,xtmpp+16,512);
    transpose4_ooff(x64+34,xtmpp+17,512);
    transpose4_ooff(x64+36,xtmpp+18,512);
    transpose4_ooff(x64+38,xtmpp+19,512);
    transpose4_ooff(x64+40,xtmpp+20,512);
    transpose4_ooff(x64+42,xtmpp+21,512);
    transpose4_ooff(x64+44,xtmpp+22,512);
    transpose4_ooff(x64+46,xtmpp+23,512);
    transpose4_ooff(x64+48,xtmpp+24,512);
    transpose4_ooff(x64+50,xtmpp+25,512);
    transpose4_ooff(x64+52,xtmpp+26,512);
    transpose4_ooff(x64+54,xtmpp+27,512);
    transpose4_ooff(x64+56,xtmpp+28,512);
    transpose4_ooff(x64+58,xtmpp+29,512);
    transpose4_ooff(x64+60,xtmpp+30,512);
    transpose4_ooff(x64+62,xtmpp+31,512);
    x64+=64;
    xtmpp+=32;
  }

  dft1024((int16_t*)(xtmp),(int16_t*)ytmp,1);
  dft1024((int16_t*)(xtmp+512),(int16_t*)(ytmp+256),1);


  for (i=0; i<256; i++) {
    bfly2(ytmpp,ytmpp+256,
          y128p,y128p+256,
          tw2048_128p);
    tw2048_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {
    y128p = y128;

    for (i=0; i<32; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT2_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT2_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT2_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT2_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT2_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT2_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT2_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT2_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT2_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT2_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT2_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT2_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT2_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT2_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT2_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT2_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

void idft2048(int16_t *x,int16_t *y,int scale)
{

  simdshort_q15_t xtmp[1024],*xtmpp,*x64 = (simdshort_q15_t *)x;
  simd_q15_t ytmp[512],*tw2048_128p=(simd_q15_t *)tw2048,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i;
  simd_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16(ONE_OVER_SQRT2_Q15);

  xtmpp = xtmp;

  for (i=0; i<16; i++) {
    transpose4_ooff(x64  ,xtmpp,512);
    transpose4_ooff(x64+2,xtmpp+1,512);
    transpose4_ooff(x64+4,xtmpp+2,512);
    transpose4_ooff(x64+6,xtmpp+3,512);
    transpose4_ooff(x64+8,xtmpp+4,512);
    transpose4_ooff(x64+10,xtmpp+5,512);
    transpose4_ooff(x64+12,xtmpp+6,512);
    transpose4_ooff(x64+14,xtmpp+7,512);
    transpose4_ooff(x64+16,xtmpp+8,512);
    transpose4_ooff(x64+18,xtmpp+9,512);
    transpose4_ooff(x64+20,xtmpp+10,512);
    transpose4_ooff(x64+22,xtmpp+11,512);
    transpose4_ooff(x64+24,xtmpp+12,512);
    transpose4_ooff(x64+26,xtmpp+13,512);
    transpose4_ooff(x64+28,xtmpp+14,512);
    transpose4_ooff(x64+30,xtmpp+15,512);
    transpose4_ooff(x64+32,xtmpp+16,512);
    transpose4_ooff(x64+34,xtmpp+17,512);
    transpose4_ooff(x64+36,xtmpp+18,512);
    transpose4_ooff(x64+38,xtmpp+19,512);
    transpose4_ooff(x64+40,xtmpp+20,512);
    transpose4_ooff(x64+42,xtmpp+21,512);
    transpose4_ooff(x64+44,xtmpp+22,512);
    transpose4_ooff(x64+46,xtmpp+23,512);
    transpose4_ooff(x64+48,xtmpp+24,512);
    transpose4_ooff(x64+50,xtmpp+25,512);
    transpose4_ooff(x64+52,xtmpp+26,512);
    transpose4_ooff(x64+54,xtmpp+27,512);
    transpose4_ooff(x64+56,xtmpp+28,512);
    transpose4_ooff(x64+58,xtmpp+29,512);
    transpose4_ooff(x64+60,xtmpp+30,512);
    transpose4_ooff(x64+62,xtmpp+31,512);
    x64+=64;
    xtmpp+=32;
  }

  idft1024((int16_t*)(xtmp),(int16_t*)ytmp,1);
  idft1024((int16_t*)(xtmp+512),(int16_t*)(ytmp+256),1);


  for (i=0; i<256; i++) {
    ibfly2(ytmpp,ytmpp+256,
           y128p,y128p+256,
           tw2048_128p);
    tw2048_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {
    y128p = y128;

    for (i=0; i<32; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT2_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT2_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT2_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT2_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT2_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT2_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT2_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT2_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT2_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT2_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT2_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT2_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT2_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT2_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT2_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT2_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

#else // __AVX2__

void dft2048(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[256],*xtmpp,*x256 = (simd256_q15_t *)x;
  simd256_q15_t ytmp[256],*tw2048_256p=(simd256_q15_t *)tw2048,*y256=(simd256_q15_t *)y,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i;
  simd256_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16_simd256(ONE_OVER_SQRT2_Q15);

  xtmpp = xtmp;

  for (i=0; i<4; i++) {
    transpose4_ooff_simd256(x256  ,xtmpp,128);
    transpose4_ooff_simd256(x256+2,xtmpp+1,128);
    transpose4_ooff_simd256(x256+4,xtmpp+2,128);
    transpose4_ooff_simd256(x256+6,xtmpp+3,128);
    transpose4_ooff_simd256(x256+8,xtmpp+4,128);
    transpose4_ooff_simd256(x256+10,xtmpp+5,128);
    transpose4_ooff_simd256(x256+12,xtmpp+6,128);
    transpose4_ooff_simd256(x256+14,xtmpp+7,128);
    transpose4_ooff_simd256(x256+16,xtmpp+8,128);
    transpose4_ooff_simd256(x256+18,xtmpp+9,128);
    transpose4_ooff_simd256(x256+20,xtmpp+10,128);
    transpose4_ooff_simd256(x256+22,xtmpp+11,128);
    transpose4_ooff_simd256(x256+24,xtmpp+12,128);
    transpose4_ooff_simd256(x256+26,xtmpp+13,128);
    transpose4_ooff_simd256(x256+28,xtmpp+14,128);
    transpose4_ooff_simd256(x256+30,xtmpp+15,128);
    transpose4_ooff_simd256(x256+32,xtmpp+16,128);
    transpose4_ooff_simd256(x256+34,xtmpp+17,128);
    transpose4_ooff_simd256(x256+36,xtmpp+18,128);
    transpose4_ooff_simd256(x256+38,xtmpp+19,128);
    transpose4_ooff_simd256(x256+40,xtmpp+20,128);
    transpose4_ooff_simd256(x256+42,xtmpp+21,128);
    transpose4_ooff_simd256(x256+44,xtmpp+22,128);
    transpose4_ooff_simd256(x256+46,xtmpp+23,128);
    transpose4_ooff_simd256(x256+48,xtmpp+24,128);
    transpose4_ooff_simd256(x256+50,xtmpp+25,128);
    transpose4_ooff_simd256(x256+52,xtmpp+26,128);
    transpose4_ooff_simd256(x256+54,xtmpp+27,128);
    transpose4_ooff_simd256(x256+56,xtmpp+28,128);
    transpose4_ooff_simd256(x256+58,xtmpp+29,128);
    transpose4_ooff_simd256(x256+60,xtmpp+30,128);
    transpose4_ooff_simd256(x256+62,xtmpp+31,128);
    x256+=64;
    xtmpp+=32;
  }

  dft1024((int16_t*)(xtmp),(int16_t*)ytmp,1);
  dft1024((int16_t*)(xtmp+128),(int16_t*)(ytmp+128),1);


  for (i=0; i<128; i++) {
    bfly2_256(ytmpp,ytmpp+128,
	      y256p,y256p+128,
	      tw2048_256p);
    tw2048_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {
    y256p = y256;

    for (i=0; i<16; i++) {
      y256p[0]  = mulhi_int16_simd256(y256p[0],ONE_OVER_SQRT2_Q15_128);
      y256p[1]  = mulhi_int16_simd256(y256p[1],ONE_OVER_SQRT2_Q15_128);
      y256p[2]  = mulhi_int16_simd256(y256p[2],ONE_OVER_SQRT2_Q15_128);
      y256p[3]  = mulhi_int16_simd256(y256p[3],ONE_OVER_SQRT2_Q15_128);
      y256p[4]  = mulhi_int16_simd256(y256p[4],ONE_OVER_SQRT2_Q15_128);
      y256p[5]  = mulhi_int16_simd256(y256p[5],ONE_OVER_SQRT2_Q15_128);
      y256p[6]  = mulhi_int16_simd256(y256p[6],ONE_OVER_SQRT2_Q15_128);
      y256p[7]  = mulhi_int16_simd256(y256p[7],ONE_OVER_SQRT2_Q15_128);
      y256p[8]  = mulhi_int16_simd256(y256p[8],ONE_OVER_SQRT2_Q15_128);
      y256p[9]  = mulhi_int16_simd256(y256p[9],ONE_OVER_SQRT2_Q15_128);
      y256p[10] = mulhi_int16_simd256(y256p[10],ONE_OVER_SQRT2_Q15_128);
      y256p[11] = mulhi_int16_simd256(y256p[11],ONE_OVER_SQRT2_Q15_128);
      y256p[12] = mulhi_int16_simd256(y256p[12],ONE_OVER_SQRT2_Q15_128);
      y256p[13] = mulhi_int16_simd256(y256p[13],ONE_OVER_SQRT2_Q15_128);
      y256p[14] = mulhi_int16_simd256(y256p[14],ONE_OVER_SQRT2_Q15_128);
      y256p[15] = mulhi_int16_simd256(y256p[15],ONE_OVER_SQRT2_Q15_128);
      y256p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

void idft2048(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[256],*xtmpp,*x256 = (simd256_q15_t *)x;
  simd256_q15_t ytmp[256],*tw2048_256p=(simd256_q15_t *)tw2048,*y256=(simd256_q15_t *)y,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i;
  simd256_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16_simd256(ONE_OVER_SQRT2_Q15);

  xtmpp = xtmp;

  for (i=0; i<4; i++) {
    transpose4_ooff_simd256(x256  ,xtmpp,128);
    transpose4_ooff_simd256(x256+2,xtmpp+1,128);
    transpose4_ooff_simd256(x256+4,xtmpp+2,128);
    transpose4_ooff_simd256(x256+6,xtmpp+3,128);
    transpose4_ooff_simd256(x256+8,xtmpp+4,128);
    transpose4_ooff_simd256(x256+10,xtmpp+5,128);
    transpose4_ooff_simd256(x256+12,xtmpp+6,128);
    transpose4_ooff_simd256(x256+14,xtmpp+7,128);
    transpose4_ooff_simd256(x256+16,xtmpp+8,128);
    transpose4_ooff_simd256(x256+18,xtmpp+9,128);
    transpose4_ooff_simd256(x256+20,xtmpp+10,128);
    transpose4_ooff_simd256(x256+22,xtmpp+11,128);
    transpose4_ooff_simd256(x256+24,xtmpp+12,128);
    transpose4_ooff_simd256(x256+26,xtmpp+13,128);
    transpose4_ooff_simd256(x256+28,xtmpp+14,128);
    transpose4_ooff_simd256(x256+30,xtmpp+15,128);
    transpose4_ooff_simd256(x256+32,xtmpp+16,128);
    transpose4_ooff_simd256(x256+34,xtmpp+17,128);
    transpose4_ooff_simd256(x256+36,xtmpp+18,128);
    transpose4_ooff_simd256(x256+38,xtmpp+19,128);
    transpose4_ooff_simd256(x256+40,xtmpp+20,128);
    transpose4_ooff_simd256(x256+42,xtmpp+21,128);
    transpose4_ooff_simd256(x256+44,xtmpp+22,128);
    transpose4_ooff_simd256(x256+46,xtmpp+23,128);
    transpose4_ooff_simd256(x256+48,xtmpp+24,128);
    transpose4_ooff_simd256(x256+50,xtmpp+25,128);
    transpose4_ooff_simd256(x256+52,xtmpp+26,128);
    transpose4_ooff_simd256(x256+54,xtmpp+27,128);
    transpose4_ooff_simd256(x256+56,xtmpp+28,128);
    transpose4_ooff_simd256(x256+58,xtmpp+29,128);
    transpose4_ooff_simd256(x256+60,xtmpp+30,128);
    transpose4_ooff_simd256(x256+62,xtmpp+31,128);
    x256+=64;
    xtmpp+=32;
  }

  idft1024((int16_t*)(xtmp),(int16_t*)ytmp,1);
  idft1024((int16_t*)(xtmp+128),(int16_t*)(ytmp+128),1);


  for (i=0; i<128; i++) {
    ibfly2_256(ytmpp,ytmpp+128,
	       y256p,y256p+128,
	       tw2048_256p);
    tw2048_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {
    y256p = y256;

    for (i=0; i<16; i++) {
      y256p[0]  = mulhi_int16_simd256(y256p[0],ONE_OVER_SQRT2_Q15_128);
      y256p[1]  = mulhi_int16_simd256(y256p[1],ONE_OVER_SQRT2_Q15_128);
      y256p[2]  = mulhi_int16_simd256(y256p[2],ONE_OVER_SQRT2_Q15_128);
      y256p[3]  = mulhi_int16_simd256(y256p[3],ONE_OVER_SQRT2_Q15_128);
      y256p[4]  = mulhi_int16_simd256(y256p[4],ONE_OVER_SQRT2_Q15_128);
      y256p[5]  = mulhi_int16_simd256(y256p[5],ONE_OVER_SQRT2_Q15_128);
      y256p[6]  = mulhi_int16_simd256(y256p[6],ONE_OVER_SQRT2_Q15_128);
      y256p[7]  = mulhi_int16_simd256(y256p[7],ONE_OVER_SQRT2_Q15_128);
      y256p[8]  = mulhi_int16_simd256(y256p[8],ONE_OVER_SQRT2_Q15_128);
      y256p[9]  = mulhi_int16_simd256(y256p[9],ONE_OVER_SQRT2_Q15_128);
      y256p[10] = mulhi_int16_simd256(y256p[10],ONE_OVER_SQRT2_Q15_128);
      y256p[11] = mulhi_int16_simd256(y256p[11],ONE_OVER_SQRT2_Q15_128);
      y256p[12] = mulhi_int16_simd256(y256p[12],ONE_OVER_SQRT2_Q15_128);
      y256p[13] = mulhi_int16_simd256(y256p[13],ONE_OVER_SQRT2_Q15_128);
      y256p[14] = mulhi_int16_simd256(y256p[14],ONE_OVER_SQRT2_Q15_128);
      y256p[15] = mulhi_int16_simd256(y256p[15],ONE_OVER_SQRT2_Q15_128);
      y256p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

#endif



int16_t tw4096[3*2*1024];

#ifndef __AVX2__
void dft4096(int16_t *x,int16_t *y,int scale)
{

  simd_q15_t xtmp[1024],ytmp[1024],*tw4096_128p=(simd_q15_t *)tw4096,*x128=(simd_q15_t *)x,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i,j;

  for (i=0,j=0; i<1024; i+=4,j++) {
    transpose16_ooff(x128+i,xtmp+j,256);
  }


  dft1024((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  dft1024((int16_t*)(xtmp+256),(int16_t*)(ytmp+256),1);
  dft1024((int16_t*)(xtmp+512),(int16_t*)(ytmp+512),1);
  dft1024((int16_t*)(xtmp+768),(int16_t*)(ytmp+768),1);

  for (i=0; i<256; i++) {
    bfly4(ytmpp,ytmpp+256,ytmpp+512,ytmpp+768,
          y128p,y128p+256,y128p+512,y128p+768,
          tw4096_128p,tw4096_128p+256,tw4096_128p+512);
    tw4096_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0; i<64; i++) {
      y128[0]  = shiftright_int16(y128[0],1);
      y128[1]  = shiftright_int16(y128[1],1);
      y128[2]  = shiftright_int16(y128[2],1);
      y128[3]  = shiftright_int16(y128[3],1);
      y128[4]  = shiftright_int16(y128[4],1);
      y128[5]  = shiftright_int16(y128[5],1);
      y128[6]  = shiftright_int16(y128[6],1);
      y128[7]  = shiftright_int16(y128[7],1);
      y128[8]  = shiftright_int16(y128[8],1);
      y128[9]  = shiftright_int16(y128[9],1);
      y128[10] = shiftright_int16(y128[10],1);
      y128[11] = shiftright_int16(y128[11],1);
      y128[12] = shiftright_int16(y128[12],1);
      y128[13] = shiftright_int16(y128[13],1);
      y128[14] = shiftright_int16(y128[14],1);
      y128[15] = shiftright_int16(y128[15],1);

      y128+=16;
    }

  }

  _mm_empty();
  _m_empty();

}



void idft4096(int16_t *x,int16_t *y,int scale)
{

  simd_q15_t xtmp[1024],ytmp[1024],*tw4096_128p=(simd_q15_t *)tw4096,*x128=(simd_q15_t *)x,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i,j;

  for (i=0,j=0; i<1024; i+=4,j++) {
    transpose16_ooff(x128+i,xtmp+j,256);
  }


  idft1024((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  idft1024((int16_t*)(xtmp+256),(int16_t*)(ytmp+256),1);
  idft1024((int16_t*)(xtmp+512),(int16_t*)(ytmp+512),1);
  idft1024((int16_t*)(xtmp+768),(int16_t*)(ytmp+768),1);

  for (i=0; i<256; i++) {
    ibfly4(ytmpp,ytmpp+256,ytmpp+512,ytmpp+768,
           y128p,y128p+256,y128p+512,y128p+768,
           tw4096_128p,tw4096_128p+256,tw4096_128p+512);
    tw4096_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0; i<64; i++) {
      y128[0]  = shiftright_int16(y128[0],1);
      y128[1]  = shiftright_int16(y128[1],1);
      y128[2]  = shiftright_int16(y128[2],1);
      y128[3]  = shiftright_int16(y128[3],1);
      y128[4]  = shiftright_int16(y128[4],1);
      y128[5]  = shiftright_int16(y128[5],1);
      y128[6]  = shiftright_int16(y128[6],1);
      y128[7]  = shiftright_int16(y128[7],1);
      y128[8]  = shiftright_int16(y128[8],1);
      y128[9]  = shiftright_int16(y128[9],1);
      y128[10] = shiftright_int16(y128[10],1);
      y128[11] = shiftright_int16(y128[11],1);
      y128[12] = shiftright_int16(y128[12],1);
      y128[13] = shiftright_int16(y128[13],1);
      y128[14] = shiftright_int16(y128[14],1);
      y128[15] = shiftright_int16(y128[15],1);

      y128+=16;
    }

  }

  _mm_empty();
  _m_empty();

}

#else //__AVX2__
void dft4096(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[512],ytmp[512],*tw4096_256p=(simd256_q15_t *)tw4096,*x256=(simd256_q15_t *)x,*y256=(simd256_q15_t *)y,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i,j;

  for (i=0,j=0; i<512; i+=4,j++) {
    transpose16_ooff_simd256(x256+i,xtmp+j,128);
  }


  dft1024((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  dft1024((int16_t*)(xtmp+128),(int16_t*)(ytmp+128),1);
  dft1024((int16_t*)(xtmp+256),(int16_t*)(ytmp+256),1);
  dft1024((int16_t*)(xtmp+384),(int16_t*)(ytmp+384),1);

  for (i=0; i<128; i++) {
    bfly4_256(ytmpp,ytmpp+128,ytmpp+256,ytmpp+384,
	      y256p,y256p+128,y256p+256,y256p+384,
	      tw4096_256p,tw4096_256p+128,tw4096_256p+256);
    tw4096_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0; i<32; i++) {
      y256[0]  = shiftright_int16_simd256(y256[0],1);
      y256[1]  = shiftright_int16_simd256(y256[1],1);
      y256[2]  = shiftright_int16_simd256(y256[2],1);
      y256[3]  = shiftright_int16_simd256(y256[3],1);
      y256[4]  = shiftright_int16_simd256(y256[4],1);
      y256[5]  = shiftright_int16_simd256(y256[5],1);
      y256[6]  = shiftright_int16_simd256(y256[6],1);
      y256[7]  = shiftright_int16_simd256(y256[7],1);
      y256[8]  = shiftright_int16_simd256(y256[8],1);
      y256[9]  = shiftright_int16_simd256(y256[9],1);
      y256[10] = shiftright_int16_simd256(y256[10],1);
      y256[11] = shiftright_int16_simd256(y256[11],1);
      y256[12] = shiftright_int16_simd256(y256[12],1);
      y256[13] = shiftright_int16_simd256(y256[13],1);
      y256[14] = shiftright_int16_simd256(y256[14],1);
      y256[15] = shiftright_int16_simd256(y256[15],1);

      y256+=16;
    }

  }

  _mm_empty();
  _m_empty();

}

void idft4096(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[512],ytmp[512],*tw4096_256p=(simd256_q15_t *)tw4096,*x256=(simd256_q15_t *)x,*y256=(simd256_q15_t *)y,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i,j;

  for (i=0,j=0; i<512; i+=4,j++) {
    transpose16_ooff_simd256(x256+i,xtmp+j,128);
  }


  idft1024((int16_t*)(xtmp),(int16_t*)(ytmp),1);
  idft1024((int16_t*)(xtmp+128),(int16_t*)(ytmp+128),1);
  idft1024((int16_t*)(xtmp+256),(int16_t*)(ytmp+256),1);
  idft1024((int16_t*)(xtmp+384),(int16_t*)(ytmp+384),1);

  for (i=0; i<128; i++) {
    ibfly4_256(ytmpp,ytmpp+128,ytmpp+256,ytmpp+384,
	       y256p,y256p+128,y256p+256,y256p+384,
	       tw4096_256p,tw4096_256p+128,tw4096_256p+256);
    tw4096_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {

    for (i=0; i<32; i++) {
      y256[0]  = shiftright_int16_simd256(y256[0],1);
      y256[1]  = shiftright_int16_simd256(y256[1],1);
      y256[2]  = shiftright_int16_simd256(y256[2],1);
      y256[3]  = shiftright_int16_simd256(y256[3],1);
      y256[4]  = shiftright_int16_simd256(y256[4],1);
      y256[5]  = shiftright_int16_simd256(y256[5],1);
      y256[6]  = shiftright_int16_simd256(y256[6],1);
      y256[7]  = shiftright_int16_simd256(y256[7],1);
      y256[8]  = shiftright_int16_simd256(y256[8],1);
      y256[9]  = shiftright_int16_simd256(y256[9],1);
      y256[10] = shiftright_int16_simd256(y256[10],1);
      y256[11] = shiftright_int16_simd256(y256[11],1);
      y256[12] = shiftright_int16_simd256(y256[12],1);
      y256[13] = shiftright_int16_simd256(y256[13],1);
      y256[14] = shiftright_int16_simd256(y256[14],1);
      y256[15] = shiftright_int16_simd256(y256[15],1);

      y256+=16;
    }

  }

  _mm_empty();
  _m_empty();

}

#endif //__AVX2__


int16_t tw8192[2*4096] __attribute__((aligned(32)));

#ifndef __AVX2__
void dft8192(int16_t *x,int16_t *y,int scale)
{

  simdshort_q15_t xtmp[4096],*xtmpp,*x64 = (simdshort_q15_t *)x;
  simd_q15_t ytmp[1024],*tw8192_128p=(simd_q15_t *)tw8192,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i;
  simd_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16(ONE_OVER_SQRT2_Q15);
  
  xtmpp = xtmp;

  for (i=0; i<64; i++) {
    transpose4_ooff(x64  ,xtmpp,2048);
    transpose4_ooff(x64+2,xtmpp+1,2048);
    transpose4_ooff(x64+4,xtmpp+2,2048);
    transpose4_ooff(x64+6,xtmpp+3,2048);
    transpose4_ooff(x64+8,xtmpp+4,2048);
    transpose4_ooff(x64+10,xtmpp+5,2048);
    transpose4_ooff(x64+12,xtmpp+6,2048);
    transpose4_ooff(x64+14,xtmpp+7,2048);
    transpose4_ooff(x64+16,xtmpp+8,2048);
    transpose4_ooff(x64+18,xtmpp+9,2048);
    transpose4_ooff(x64+20,xtmpp+10,2048);
    transpose4_ooff(x64+22,xtmpp+11,2048);
    transpose4_ooff(x64+24,xtmpp+12,2048);
    transpose4_ooff(x64+26,xtmpp+13,2048);
    transpose4_ooff(x64+28,xtmpp+14,2048);
    transpose4_ooff(x64+30,xtmpp+15,2048);
    transpose4_ooff(x64+32,xtmpp+16,2048);
    transpose4_ooff(x64+34,xtmpp+17,2048);
    transpose4_ooff(x64+36,xtmpp+18,2048);
    transpose4_ooff(x64+38,xtmpp+19,2048);
    transpose4_ooff(x64+40,xtmpp+20,2048);
    transpose4_ooff(x64+42,xtmpp+21,2048);
    transpose4_ooff(x64+44,xtmpp+22,2048);
    transpose4_ooff(x64+46,xtmpp+23,2048);
    transpose4_ooff(x64+48,xtmpp+24,2048);
    transpose4_ooff(x64+50,xtmpp+25,2048);
    transpose4_ooff(x64+52,xtmpp+26,2048);
    transpose4_ooff(x64+54,xtmpp+27,2048);
    transpose4_ooff(x64+56,xtmpp+28,2048);
    transpose4_ooff(x64+58,xtmpp+29,2048);
    transpose4_ooff(x64+60,xtmpp+30,2048);
    transpose4_ooff(x64+62,xtmpp+31,2048);
    x64+=64;
    xtmpp+=32;
  }

  dft4096((int16_t*)(xtmp),(int16_t*)ytmp,1);
  dft4096((int16_t*)(xtmp+2048),(int16_t*)(ytmp+1024),1);


  for (i=0; i<1024; i++) {
    bfly2(ytmpp,ytmpp+1024,
          y128p,y128p+1024,
          tw8192_128p);
    tw8192_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {
    y128p = y128;

    for (i=0; i<128; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT2_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT2_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT2_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT2_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT2_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT2_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT2_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT2_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT2_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT2_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT2_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT2_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT2_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT2_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT2_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT2_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

void idft8192(int16_t *x,int16_t *y,int scale)
{

  simdshort_q15_t xtmp[4096],*xtmpp,*x64 = (simdshort_q15_t *)x;
  simd_q15_t ytmp[2048],*tw8192_128p=(simd_q15_t *)tw8192,*y128=(simd_q15_t *)y,*y128p=(simd_q15_t *)y;
  simd_q15_t *ytmpp = &ytmp[0];
  int i;
  simd_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16(ONE_OVER_SQRT2_Q15);
  
  xtmpp = xtmp;

  for (i=0; i<64; i++) {
    transpose4_ooff(x64  ,xtmpp,2048);
    transpose4_ooff(x64+2,xtmpp+1,2048);
    transpose4_ooff(x64+4,xtmpp+2,2048);
    transpose4_ooff(x64+6,xtmpp+3,2048);
    transpose4_ooff(x64+8,xtmpp+4,2048);
    transpose4_ooff(x64+10,xtmpp+5,2048);
    transpose4_ooff(x64+12,xtmpp+6,2048);
    transpose4_ooff(x64+14,xtmpp+7,2048);
    transpose4_ooff(x64+16,xtmpp+8,2048);
    transpose4_ooff(x64+18,xtmpp+9,2048);
    transpose4_ooff(x64+20,xtmpp+10,2048);
    transpose4_ooff(x64+22,xtmpp+11,2048);
    transpose4_ooff(x64+24,xtmpp+12,2048);
    transpose4_ooff(x64+26,xtmpp+13,2048);
    transpose4_ooff(x64+28,xtmpp+14,2048);
    transpose4_ooff(x64+30,xtmpp+15,2048);
    transpose4_ooff(x64+32,xtmpp+16,2048);
    transpose4_ooff(x64+34,xtmpp+17,2048);
    transpose4_ooff(x64+36,xtmpp+18,2048);
    transpose4_ooff(x64+38,xtmpp+19,2048);
    transpose4_ooff(x64+40,xtmpp+20,2048);
    transpose4_ooff(x64+42,xtmpp+21,2048);
    transpose4_ooff(x64+44,xtmpp+22,2048);
    transpose4_ooff(x64+46,xtmpp+23,2048);
    transpose4_ooff(x64+48,xtmpp+24,2048);
    transpose4_ooff(x64+50,xtmpp+25,2048);
    transpose4_ooff(x64+52,xtmpp+26,2048);
    transpose4_ooff(x64+54,xtmpp+27,2048);
    transpose4_ooff(x64+56,xtmpp+28,2048);
    transpose4_ooff(x64+58,xtmpp+29,2048);
    transpose4_ooff(x64+60,xtmpp+30,2048);
    transpose4_ooff(x64+62,xtmpp+31,2048);
    x64+=64;
    xtmpp+=32;
  }

  idft4096((int16_t*)(xtmp),(int16_t*)ytmp,1);
  idft4096((int16_t*)(xtmp+2048),(int16_t*)(ytmp+1024),1);


  for (i=0; i<1024; i++) {
    ibfly2(ytmpp,ytmpp+1024,
           y128p,y128p+1024,
           tw8192_128p);
    tw8192_128p++;
    y128p++;
    ytmpp++;
  }

  if (scale>0) {
    y128p = y128;

    for (i=0; i<128; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT2_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT2_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT2_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT2_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT2_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT2_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT2_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT2_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT2_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT2_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT2_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT2_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT2_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT2_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT2_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT2_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

#else // __AVX2__
void dft8192(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[1024],*xtmpp,*x256 = (simd256_q15_t *)x;
  simd256_q15_t ytmp[1024],*tw8192_256p=(simd256_q15_t *)tw8192,*y256=(simd256_q15_t *)y,*y256p=(simd256_q15_t *)y;

  simd256_q15_t *ytmpp = &ytmp[0];
  int i;
  simd256_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16_simd256(ONE_OVER_SQRT2_Q15);
  
  xtmpp = xtmp;

  for (i=0; i<16; i++) {
    transpose4_ooff_simd256(x256  ,xtmpp,512);
    transpose4_ooff_simd256(x256+2,xtmpp+1,512);
    transpose4_ooff_simd256(x256+4,xtmpp+2,512);
    transpose4_ooff_simd256(x256+6,xtmpp+3,512);
    transpose4_ooff_simd256(x256+8,xtmpp+4,512);
    transpose4_ooff_simd256(x256+10,xtmpp+5,512);
    transpose4_ooff_simd256(x256+12,xtmpp+6,512);
    transpose4_ooff_simd256(x256+14,xtmpp+7,512);
    transpose4_ooff_simd256(x256+16,xtmpp+8,512);
    transpose4_ooff_simd256(x256+18,xtmpp+9,512);
    transpose4_ooff_simd256(x256+20,xtmpp+10,512);
    transpose4_ooff_simd256(x256+22,xtmpp+11,512);
    transpose4_ooff_simd256(x256+24,xtmpp+12,512);
    transpose4_ooff_simd256(x256+26,xtmpp+13,512);
    transpose4_ooff_simd256(x256+28,xtmpp+14,512);
    transpose4_ooff_simd256(x256+30,xtmpp+15,512);
    transpose4_ooff_simd256(x256+32,xtmpp+16,512);
    transpose4_ooff_simd256(x256+34,xtmpp+17,512);
    transpose4_ooff_simd256(x256+36,xtmpp+18,512);
    transpose4_ooff_simd256(x256+38,xtmpp+19,512);
    transpose4_ooff_simd256(x256+40,xtmpp+20,512);
    transpose4_ooff_simd256(x256+42,xtmpp+21,512);
    transpose4_ooff_simd256(x256+44,xtmpp+22,512);
    transpose4_ooff_simd256(x256+46,xtmpp+23,512);
    transpose4_ooff_simd256(x256+48,xtmpp+24,512);
    transpose4_ooff_simd256(x256+50,xtmpp+25,512);
    transpose4_ooff_simd256(x256+52,xtmpp+26,512);
    transpose4_ooff_simd256(x256+54,xtmpp+27,512);
    transpose4_ooff_simd256(x256+56,xtmpp+28,512);
    transpose4_ooff_simd256(x256+58,xtmpp+29,512);
    transpose4_ooff_simd256(x256+60,xtmpp+30,512);
    transpose4_ooff_simd256(x256+62,xtmpp+31,512);
    x256+=64;
    xtmpp+=32;
  }

  dft4096((int16_t*)(xtmp),(int16_t*)ytmp,1);
  dft4096((int16_t*)(xtmp+512),(int16_t*)(ytmp+512),1);


  for (i=0; i<512; i++) {
    bfly2_256(ytmpp,ytmpp+512,
	      y256p,y256p+512,
	      tw8192_256p);
    tw8192_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {
    y256p = y256;

    for (i=0; i<64; i++) {
      y256p[0]  = mulhi_int16_simd256(y256p[0],ONE_OVER_SQRT2_Q15_128);
      y256p[1]  = mulhi_int16_simd256(y256p[1],ONE_OVER_SQRT2_Q15_128);
      y256p[2]  = mulhi_int16_simd256(y256p[2],ONE_OVER_SQRT2_Q15_128);
      y256p[3]  = mulhi_int16_simd256(y256p[3],ONE_OVER_SQRT2_Q15_128);
      y256p[4]  = mulhi_int16_simd256(y256p[4],ONE_OVER_SQRT2_Q15_128);
      y256p[5]  = mulhi_int16_simd256(y256p[5],ONE_OVER_SQRT2_Q15_128);
      y256p[6]  = mulhi_int16_simd256(y256p[6],ONE_OVER_SQRT2_Q15_128);
      y256p[7]  = mulhi_int16_simd256(y256p[7],ONE_OVER_SQRT2_Q15_128);
      y256p[8]  = mulhi_int16_simd256(y256p[8],ONE_OVER_SQRT2_Q15_128);
      y256p[9]  = mulhi_int16_simd256(y256p[9],ONE_OVER_SQRT2_Q15_128);
      y256p[10] = mulhi_int16_simd256(y256p[10],ONE_OVER_SQRT2_Q15_128);
      y256p[11] = mulhi_int16_simd256(y256p[11],ONE_OVER_SQRT2_Q15_128);
      y256p[12] = mulhi_int16_simd256(y256p[12],ONE_OVER_SQRT2_Q15_128);
      y256p[13] = mulhi_int16_simd256(y256p[13],ONE_OVER_SQRT2_Q15_128);
      y256p[14] = mulhi_int16_simd256(y256p[14],ONE_OVER_SQRT2_Q15_128);
      y256p[15] = mulhi_int16_simd256(y256p[15],ONE_OVER_SQRT2_Q15_128);
      y256p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

void idft8192(int16_t *x,int16_t *y,int scale)
{

  simd256_q15_t xtmp[1024],*xtmpp,*x256 = (simd256_q15_t *)x;
  simd256_q15_t ytmp[1024],*tw8192_256p=(simd256_q15_t *)tw8192,*y256=(simd256_q15_t *)y,*y256p=(simd256_q15_t *)y;
  simd256_q15_t *ytmpp = &ytmp[0];
  int i;
  simd256_q15_t ONE_OVER_SQRT2_Q15_128 = set1_int16_simd256(ONE_OVER_SQRT2_Q15);
  
  xtmpp = xtmp;

  for (i=0; i<16; i++) {
    transpose4_ooff_simd256(x256  ,xtmpp,512);
    transpose4_ooff_simd256(x256+2,xtmpp+1,512);
    transpose4_ooff_simd256(x256+4,xtmpp+2,512);
    transpose4_ooff_simd256(x256+6,xtmpp+3,512);
    transpose4_ooff_simd256(x256+8,xtmpp+4,512);
    transpose4_ooff_simd256(x256+10,xtmpp+5,512);
    transpose4_ooff_simd256(x256+12,xtmpp+6,512);
    transpose4_ooff_simd256(x256+14,xtmpp+7,512);
    transpose4_ooff_simd256(x256+16,xtmpp+8,512);
    transpose4_ooff_simd256(x256+18,xtmpp+9,512);
    transpose4_ooff_simd256(x256+20,xtmpp+10,512);
    transpose4_ooff_simd256(x256+22,xtmpp+11,512);
    transpose4_ooff_simd256(x256+24,xtmpp+12,512);
    transpose4_ooff_simd256(x256+26,xtmpp+13,512);
    transpose4_ooff_simd256(x256+28,xtmpp+14,512);
    transpose4_ooff_simd256(x256+30,xtmpp+15,512);
    transpose4_ooff_simd256(x256+32,xtmpp+16,512);
    transpose4_ooff_simd256(x256+34,xtmpp+17,512);
    transpose4_ooff_simd256(x256+36,xtmpp+18,512);
    transpose4_ooff_simd256(x256+38,xtmpp+19,512);
    transpose4_ooff_simd256(x256+40,xtmpp+20,512);
    transpose4_ooff_simd256(x256+42,xtmpp+21,512);
    transpose4_ooff_simd256(x256+44,xtmpp+22,512);
    transpose4_ooff_simd256(x256+46,xtmpp+23,512);
    transpose4_ooff_simd256(x256+48,xtmpp+24,512);
    transpose4_ooff_simd256(x256+50,xtmpp+25,512);
    transpose4_ooff_simd256(x256+52,xtmpp+26,512);
    transpose4_ooff_simd256(x256+54,xtmpp+27,512);
    transpose4_ooff_simd256(x256+56,xtmpp+28,512);
    transpose4_ooff_simd256(x256+58,xtmpp+29,512);
    transpose4_ooff_simd256(x256+60,xtmpp+30,512);
    transpose4_ooff_simd256(x256+62,xtmpp+31,512);
    x256+=64;
    xtmpp+=32;
  }

  idft4096((int16_t*)(xtmp),(int16_t*)ytmp,1);
  idft4096((int16_t*)(xtmp+512),(int16_t*)(ytmp+512),1);


  for (i=0; i<512; i++) {
    ibfly2_256(ytmpp,ytmpp+512,
	       y256p,y256p+512,
	       tw8192_256p);
    tw8192_256p++;
    y256p++;
    ytmpp++;
  }

  if (scale>0) {
    y256p = y256;

    for (i=0; i<64; i++) {
      y256p[0]  = mulhi_int16_simd256(y256p[0],ONE_OVER_SQRT2_Q15_128);
      y256p[1]  = mulhi_int16_simd256(y256p[1],ONE_OVER_SQRT2_Q15_128);
      y256p[2]  = mulhi_int16_simd256(y256p[2],ONE_OVER_SQRT2_Q15_128);
      y256p[3]  = mulhi_int16_simd256(y256p[3],ONE_OVER_SQRT2_Q15_128);
      y256p[4]  = mulhi_int16_simd256(y256p[4],ONE_OVER_SQRT2_Q15_128);
      y256p[5]  = mulhi_int16_simd256(y256p[5],ONE_OVER_SQRT2_Q15_128);
      y256p[6]  = mulhi_int16_simd256(y256p[6],ONE_OVER_SQRT2_Q15_128);
      y256p[7]  = mulhi_int16_simd256(y256p[7],ONE_OVER_SQRT2_Q15_128);
      y256p[8]  = mulhi_int16_simd256(y256p[8],ONE_OVER_SQRT2_Q15_128);
      y256p[9]  = mulhi_int16_simd256(y256p[9],ONE_OVER_SQRT2_Q15_128);
      y256p[10] = mulhi_int16_simd256(y256p[10],ONE_OVER_SQRT2_Q15_128);
      y256p[11] = mulhi_int16_simd256(y256p[11],ONE_OVER_SQRT2_Q15_128);
      y256p[12] = mulhi_int16_simd256(y256p[12],ONE_OVER_SQRT2_Q15_128);
      y256p[13] = mulhi_int16_simd256(y256p[13],ONE_OVER_SQRT2_Q15_128);
      y256p[14] = mulhi_int16_simd256(y256p[14],ONE_OVER_SQRT2_Q15_128);
      y256p[15] = mulhi_int16_simd256(y256p[15],ONE_OVER_SQRT2_Q15_128);
      y256p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}


#endif

int16_t twa1536[1024],twb1536[1024];

// 512 x 3
void idft1536(int16_t *input, int16_t *output, int scale)
{
  int i,i2,j;
  uint32_t tmp[3][512 ]__attribute__((aligned(32)));
  uint32_t tmpo[3][512] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<512; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  idft512((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),1);
  idft512((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),1);
  idft512((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),1);

  for (i=0,i2=0; i<1024; i+=8,i2+=4)  {
    ibfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),((simd_q15_t*)&tmpo[2][i2]),
          (simd_q15_t*)(output+i),(simd_q15_t*)(output+1024+i),(simd_q15_t*)(output+2048+i),
          (simd_q15_t*)(twa1536+i),(simd_q15_t*)(twb1536+i));
  }


  if (scale==1) {
    for (i=0; i<24; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

void dft1536(int16_t *input, int16_t *output, int scale)
{
  int i,i2,j;
  uint32_t tmp[3][512] __attribute__((aligned(32)));
  uint32_t tmpo[3][512] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<512; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  dft512((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),1);
  dft512((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),1);
  dft512((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),1);

  /*
  for (i=1; i<512; i++) {
    tmpo[0][i] = tmpo[0][i<<1];
    tmpo[1][i] = tmpo[1][i<<1];
    tmpo[2][i] = tmpo[2][i<<1];
    }*/
  if (LOG_DUMPFLAG(DEBUG_DFT)) {
    LOG_M("dft1536out0.m","o0",tmpo[0],2048,1,1);
    LOG_M("dft1536out1.m","o1",tmpo[1],2048,1,1);
    LOG_M("dft1536out2.m","o2",tmpo[2],2048,1,1);
  }
  for (i=0,i2=0; i<1024; i+=8,i2+=4)  {
    bfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),(simd_q15_t*)(&tmpo[2][i2]),
          (simd_q15_t*)(output+i),(simd_q15_t*)(output+1024+i),(simd_q15_t*)(output+2048+i),
          (simd_q15_t*)(twa1536+i),(simd_q15_t*)(twb1536+i));
  }

  if (scale==1) {
    for (i=0; i<24; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}

int16_t twa3072[2048] __attribute__((aligned(32)));
int16_t twb3072[2048] __attribute__((aligned(32)));
// 1024 x 3
void dft3072(int16_t *input, int16_t *output,int scale)
{
  int i,i2,j;
  uint32_t tmp[3][1024] __attribute__((aligned(32)));
  uint32_t tmpo[3][1024] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<1024; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  dft1024((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),1);
  dft1024((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),1);
  dft1024((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),1);

  for (i=0,i2=0; i<2048; i+=8,i2+=4)  {
    bfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),(simd_q15_t*)(&tmpo[2][i2]),
          (simd_q15_t*)(output+i),(simd_q15_t*)(output+2048+i),(simd_q15_t*)(output+4096+i),
          (simd_q15_t*)(twa3072+i),(simd_q15_t*)(twb3072+i));
  }

  if (scale==1) {
    for (i=0; i<48; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();
}

void idft3072(int16_t *input, int16_t *output,int scale)
{
  int i,i2,j;
  uint32_t tmp[3][1024]__attribute__((aligned(32)));
  uint32_t tmpo[3][1024] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<1024; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  idft1024((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),1);
  idft1024((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),1);
  idft1024((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),1);

  for (i=0,i2=0; i<2048; i+=8,i2+=4)  {
    ibfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),((simd_q15_t*)&tmpo[2][i2]),
          (simd_q15_t*)(output+i),(simd_q15_t*)(output+2048+i),(simd_q15_t*)(output+4096+i),
          (simd_q15_t*)(twa3072+i),(simd_q15_t*)(twb3072+i));
  }


  if (scale==1) {
    for (i=0; i<48; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();
}


int16_t twa6144[4096] __attribute__((aligned(32)));
int16_t twb6144[4096] __attribute__((aligned(32)));

void idft6144(int16_t *input, int16_t *output,int scale)
{
  int i,i2,j;
  uint32_t tmp[3][2048] __attribute__((aligned(32)));
  uint32_t tmpo[3][2048] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<2048; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  idft2048((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),1);
  idft2048((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),1);
  idft2048((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),1);

  if (LOG_DUMPFLAG(DEBUG_DFT)) {
    LOG_M("idft6144in.m","in",input,6144,1,1);
    LOG_M("idft6144out0.m","o0",tmpo[0],2048,1,1);
    LOG_M("idft6144out1.m","o1",tmpo[1],2048,1,1);
    LOG_M("idft6144out2.m","o2",tmpo[2],2048,1,1);
  }

  for (i=0,i2=0; i<4096; i+=8,i2+=4)  {
    ibfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),((simd_q15_t*)&tmpo[2][i2]),
	   (simd_q15_t*)(output+i),(simd_q15_t*)(output+4096+i),(simd_q15_t*)(output+8192+i),
	   (simd_q15_t*)(twa6144+i),(simd_q15_t*)(twb6144+i));
  }


  if (scale==1) {
    for (i=0; i<96; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }

  _mm_empty();
  _m_empty();

}


void dft6144(int16_t *input, int16_t *output,int scale)
{
  int i,i2,j;
  uint32_t tmp[3][2048] __attribute__((aligned(32)));
  uint32_t tmpo[3][2048] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<2048; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  dft2048((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),1);
  dft2048((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),1);
  dft2048((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),1);

  /*
  for (i=1; i<2048; i++) {
    tmpo[0][i] = tmpo[0][i<<1];
    tmpo[1][i] = tmpo[1][i<<1];
    tmpo[2][i] = tmpo[2][i<<1];
    }*/
  if (LOG_DUMPFLAG(DEBUG_DFT)) {
    LOG_M("ft6144out0.m","o0",tmpo[0],2048,1,1);
    LOG_M("ft6144out1.m","o1",tmpo[1],2048,1,1);
    LOG_M("ft6144out2.m","o2",tmpo[2],2048,1,1);
  }
  for (i=0,i2=0; i<4096; i+=8,i2+=4)  {
    bfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),(simd_q15_t*)(&tmpo[2][i2]),
          (simd_q15_t*)(output+i),(simd_q15_t*)(output+4096+i),(simd_q15_t*)(output+8192+i),
          (simd_q15_t*)(twa6144+i),(simd_q15_t*)(twb6144+i));
  }

  if (scale==1) {
    for (i=0; i<96; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }
  
  _mm_empty();
  _m_empty();

}


int16_t twa12288[8192] __attribute__((aligned(32)));
int16_t twb12288[8192] __attribute__((aligned(32)));
// 4096 x 3
void dft12288(int16_t *input, int16_t *output,int scale)
{
  int i,i2,j;
  uint32_t tmp[3][4096] __attribute__((aligned(32)));
  uint32_t tmpo[3][4096] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<4096; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  dft4096((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),scale);
  dft4096((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),scale);
  dft4096((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),scale);
  /*
  for (i=1; i<4096; i++) {
    tmpo[0][i] = tmpo[0][i<<1];
    tmpo[1][i] = tmpo[1][i<<1];
    tmpo[2][i] = tmpo[2][i<<1];
    }*/
  if (LOG_DUMPFLAG(DEBUG_DFT)) {
    LOG_M("dft12288out0.m","o0",tmpo[0],4096,1,1);
    LOG_M("dft12288out1.m","o1",tmpo[1],4096,1,1);
    LOG_M("dft12288out2.m","o2",tmpo[2],4096,1,1);
  }
  for (i=0,i2=0; i<8192; i+=8,i2+=4)  {
    bfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),(simd_q15_t*)(&tmpo[2][i2]),
          (simd_q15_t*)(output+i),(simd_q15_t*)(output+8192+i),(simd_q15_t*)(output+16384+i),
          (simd_q15_t*)(twa12288+i),(simd_q15_t*)(twb12288+i));
  }

  if (scale==1) {
    for (i=0; i<192; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }
  _mm_empty();
  _m_empty();

}

void idft12288(int16_t *input, int16_t *output,int scale)
{
  int i,i2,j;
  uint32_t tmp[3][4096] __attribute__((aligned(32)));
  uint32_t tmpo[3][4096] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<4096; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }



  idft4096((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),scale);
  idft4096((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),scale);
  idft4096((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),scale);

  if (LOG_DUMPFLAG(DEBUG_DFT)) {
    LOG_M("idft12288in.m","in",input,12288,1,1);
    LOG_M("idft12288out0.m","o0",tmpo[0],4096,1,1);
    LOG_M("idft12288out1.m","o1",tmpo[1],4096,1,1);
    LOG_M("idft12288out2.m","o2",tmpo[2],4096,1,1);
  }

  for (i=0,i2=0; i<8192; i+=8,i2+=4)  {
    ibfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),((simd_q15_t*)&tmpo[2][i2]),
          (simd_q15_t*)(output+i),(simd_q15_t*)(output+8192+i),(simd_q15_t*)(output+16384+i),
          (simd_q15_t*)(twa12288+i),(simd_q15_t*)(twb12288+i));
  }

  if (scale==1) {
    for (i=0; i<192; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }
  _mm_empty();
  _m_empty();
  if (LOG_DUMPFLAG(DEBUG_DFT)) {
     LOG_M("idft12288out.m","out",output,6144,1,1);
  }
}

int16_t twa18432[12288] __attribute__((aligned(32)));
int16_t twb18432[12288] __attribute__((aligned(32)));
// 6144 x 3
void dft18432(int16_t *input, int16_t *output,int scale) {

  int i,i2,j;
  uint32_t tmp[3][6144] __attribute__((aligned(32)));
  uint32_t tmpo[3][6144] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<6144; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  dft6144((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),scale);
  dft6144((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),scale);
  dft6144((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),scale);

  for (i=0,i2=0; i<12288; i+=8,i2+=4)  {
    bfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),(simd_q15_t*)(&tmpo[2][i2]),
          (simd_q15_t*)(output+i),(simd_q15_t*)(output+12288+i),(simd_q15_t*)(output+24576+i),
          (simd_q15_t*)(twa18432+i),(simd_q15_t*)(twb18432+i));
  }
  if (scale==1) {
    for (i=0; i<288; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }
  _mm_empty();
  _m_empty();
}

void idft18432(int16_t *input, int16_t *output,int scale) {

  int i,i2,j;
  uint32_t tmp[3][6144] __attribute__((aligned(32)));
  uint32_t tmpo[3][6144] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<6144; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  idft6144((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),scale);
  idft6144((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),scale);
  idft6144((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),scale);

  for (i=0,i2=0; i<12288; i+=8,i2+=4)  {
    ibfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),(simd_q15_t*)(&tmpo[2][i2]),
	   (simd_q15_t*)(output+i),(simd_q15_t*)(output+12288+i),(simd_q15_t*)(output+24576+i),
	   (simd_q15_t*)(twa18432+i),(simd_q15_t*)(twb18432+i));
  }
  if (scale==1) {
    for (i=0; i<288; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }
  _mm_empty();
  _m_empty();
}


int16_t twa24576[16384] __attribute__((aligned(32)));
int16_t twb24576[16384] __attribute__((aligned(32)));
// 8192 x 3
void dft24576(int16_t *input, int16_t *output,int scale)
{
  int i,i2,j;
  uint32_t tmp[3][8192] __attribute__((aligned(32)));
  uint32_t tmpo[3][8192] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<8192; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  dft8192((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),1);
  dft8192((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),1);
  dft8192((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),1);
  /*
  for (i=1; i<8192; i++) {
    tmpo[0][i] = tmpo[0][i<<1];
    tmpo[1][i] = tmpo[1][i<<1];
    tmpo[2][i] = tmpo[2][i<<1];
    }*/
  if (LOG_DUMPFLAG(DEBUG_DFT)) {
    LOG_M("dft24576out0.m","o0",tmpo[0],8192,1,1);
    LOG_M("dft24576out1.m","o1",tmpo[1],8192,1,1);
    LOG_M("dft24576out2.m","o2",tmpo[2],8192,1,1);
  }
  for (i=0,i2=0; i<16384; i+=8,i2+=4)  {
    bfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),(simd_q15_t*)(&tmpo[2][i2]),
          (simd_q15_t*)(output+i),(simd_q15_t*)(output+16384+i),(simd_q15_t*)(output+32768+i),
          (simd_q15_t*)(twa24576+i),(simd_q15_t*)(twb24576+i));
  }


  if (scale==1) {
    for (i=0; i<384; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }
  _mm_empty();
  _m_empty();
  if (LOG_DUMPFLAG(DEBUG_DFT)) {
     LOG_M("out.m","out",output,24576,1,1);
  }
}

void idft24576(int16_t *input, int16_t *output,int scale)
{
  int i,i2,j;
  uint32_t tmp[3][8192] __attribute__((aligned(32)));
  uint32_t tmpo[3][8192] __attribute__((aligned(32)));
  simd_q15_t *y128p=(simd_q15_t*)output;
  simd_q15_t ONE_OVER_SQRT3_Q15_128 = set1_int16(ONE_OVER_SQRT3_Q15);

  for (i=0,j=0; i<8192; i++) {
    tmp[0][i] = ((uint32_t *)input)[j++];
    tmp[1][i] = ((uint32_t *)input)[j++];
    tmp[2][i] = ((uint32_t *)input)[j++];
  }

  idft8192((int16_t*)(tmp[0]),(int16_t*)(tmpo[0]),1);
  idft8192((int16_t*)(tmp[1]),(int16_t*)(tmpo[1]),1);
  idft8192((int16_t*)(tmp[2]),(int16_t*)(tmpo[2]),1);
  
  if (LOG_DUMPFLAG(DEBUG_DFT)) {
    LOG_M("idft24576in.m","in",input,24576,1,1);
    LOG_M("idft24576out0.m","o0",tmpo[0],8192,1,1);
    LOG_M("idft24576out1.m","o1",tmpo[1],8192,1,1);
    LOG_M("idft24576out2.m","o2",tmpo[2],8192,1,1);
  }

  for (i=0,i2=0; i<16384; i+=8,i2+=4)  {
    ibfly3((simd_q15_t*)(&tmpo[0][i2]),(simd_q15_t*)(&tmpo[1][i2]),((simd_q15_t*)&tmpo[2][i2]),
          (simd_q15_t*)(output+i),(simd_q15_t*)(output+16384+i),(simd_q15_t*)(output+32768+i),
          (simd_q15_t*)(twa24576+i),(simd_q15_t*)(twb24576+i));
  }
  if (scale==1) {
    for (i=0; i<384; i++) {
      y128p[0]  = mulhi_int16(y128p[0],ONE_OVER_SQRT3_Q15_128);
      y128p[1]  = mulhi_int16(y128p[1],ONE_OVER_SQRT3_Q15_128);
      y128p[2]  = mulhi_int16(y128p[2],ONE_OVER_SQRT3_Q15_128);
      y128p[3]  = mulhi_int16(y128p[3],ONE_OVER_SQRT3_Q15_128);
      y128p[4]  = mulhi_int16(y128p[4],ONE_OVER_SQRT3_Q15_128);
      y128p[5]  = mulhi_int16(y128p[5],ONE_OVER_SQRT3_Q15_128);
      y128p[6]  = mulhi_int16(y128p[6],ONE_OVER_SQRT3_Q15_128);
      y128p[7]  = mulhi_int16(y128p[7],ONE_OVER_SQRT3_Q15_128);
      y128p[8]  = mulhi_int16(y128p[8],ONE_OVER_SQRT3_Q15_128);
      y128p[9]  = mulhi_int16(y128p[9],ONE_OVER_SQRT3_Q15_128);
      y128p[10] = mulhi_int16(y128p[10],ONE_OVER_SQRT3_Q15_128);
      y128p[11] = mulhi_int16(y128p[11],ONE_OVER_SQRT3_Q15_128);
      y128p[12] = mulhi_int16(y128p[12],ONE_OVER_SQRT3_Q15_128);
      y128p[13] = mulhi_int16(y128p[13],ONE_OVER_SQRT3_Q15_128);
      y128p[14] = mulhi_int16(y128p[14],ONE_OVER_SQRT3_Q15_128);
      y128p[15] = mulhi_int16(y128p[15],ONE_OVER_SQRT3_Q15_128);
      y128p+=16;
    }
  }
  _mm_empty();
  _m_empty();

  if (LOG_DUMPFLAG(DEBUG_DFT)) {
    LOG_M("idft24576out.m","out",output,24576,1,1);
  }
}

///  THIS SECTION IS FOR ALL PUSCH DFTS (i.e. radix 2^a * 3^b * 4^c * 5^d)
///  They use twiddles for 4-way parallel DFTS (i.e. 4 DFTS with interleaved input/output)

static int16_t W1_12s[8]__attribute__((aligned(32))) = {28377,-16383,28377,-16383,28377,-16383,28377,-16383};
static int16_t W2_12s[8]__attribute__((aligned(32))) = {16383,-28377,16383,-28377,16383,-28377,16383,-28377};
static int16_t W3_12s[8]__attribute__((aligned(32))) = {0,-32767,0,-32767,0,-32767,0,-32767};
static int16_t W4_12s[8]__attribute__((aligned(32))) = {-16383,-28377,-16383,-28377,-16383,-28377,-16383,-28377};
static int16_t W6_12s[8]__attribute__((aligned(32))) = {-32767,0,-32767,0,-32767,0,-32767,0};

simd_q15_t *W1_12=(simd_q15_t *)W1_12s;
simd_q15_t *W2_12=(simd_q15_t *)W2_12s;
simd_q15_t *W3_12=(simd_q15_t *)W3_12s;
simd_q15_t *W4_12=(simd_q15_t *)W4_12s;
simd_q15_t *W6_12=(simd_q15_t *)W6_12s;


static simd_q15_t norm128;

static inline void dft12f(simd_q15_t *x0,
                          simd_q15_t *x1,
                          simd_q15_t *x2,
                          simd_q15_t *x3,
                          simd_q15_t *x4,
                          simd_q15_t *x5,
                          simd_q15_t *x6,
                          simd_q15_t *x7,
                          simd_q15_t *x8,
                          simd_q15_t *x9,
                          simd_q15_t *x10,
                          simd_q15_t *x11,
                          simd_q15_t *y0,
                          simd_q15_t *y1,
                          simd_q15_t *y2,
                          simd_q15_t *y3,
                          simd_q15_t *y4,
                          simd_q15_t *y5,
                          simd_q15_t *y6,
                          simd_q15_t *y7,
                          simd_q15_t *y8,
                          simd_q15_t *y9,
                          simd_q15_t *y10,
                          simd_q15_t *y11) __attribute__((always_inline));

static inline void dft12f(simd_q15_t *x0,
                          simd_q15_t *x1,
                          simd_q15_t *x2,
                          simd_q15_t *x3,
                          simd_q15_t *x4,
                          simd_q15_t *x5,
                          simd_q15_t *x6,
                          simd_q15_t *x7,
                          simd_q15_t *x8,
                          simd_q15_t *x9,
                          simd_q15_t *x10,
                          simd_q15_t *x11,
                          simd_q15_t *y0,
                          simd_q15_t *y1,
                          simd_q15_t *y2,
                          simd_q15_t *y3,
                          simd_q15_t *y4,
                          simd_q15_t *y5,
                          simd_q15_t *y6,
                          simd_q15_t *y7,
                          simd_q15_t *y8,
                          simd_q15_t *y9,
                          simd_q15_t *y10,
                          simd_q15_t *y11)
{


  simd_q15_t tmp_dft12[12];

  simd_q15_t *tmp_dft12_ptr = &tmp_dft12[0];

  // msg("dft12\n");

  bfly4_tw1(x0,
            x3,
            x6,
            x9,
            tmp_dft12_ptr,
            tmp_dft12_ptr+3,
            tmp_dft12_ptr+6,
            tmp_dft12_ptr+9);


  bfly4_tw1(x1,
            x4,
            x7,
            x10,
            tmp_dft12_ptr+1,
            tmp_dft12_ptr+4,
            tmp_dft12_ptr+7,
            tmp_dft12_ptr+10);


  bfly4_tw1(x2,
            x5,
            x8,
            x11,
            tmp_dft12_ptr+2,
            tmp_dft12_ptr+5,
            tmp_dft12_ptr+8,
            tmp_dft12_ptr+11);

  //  k2=0;
  bfly3_tw1(tmp_dft12_ptr,
            tmp_dft12_ptr+1,
            tmp_dft12_ptr+2,
            y0,
            y4,
            y8);



  //  k2=1;
  bfly3(tmp_dft12_ptr+3,
        tmp_dft12_ptr+4,
        tmp_dft12_ptr+5,
        y1,
        y5,
        y9,
        W1_12,
        W2_12);



  //  k2=2;
  bfly3(tmp_dft12_ptr+6,
        tmp_dft12_ptr+7,
        tmp_dft12_ptr+8,
        y2,
        y6,
        y10,
        W2_12,
        W4_12);

  //  k2=3;
  bfly3(tmp_dft12_ptr+9,
        tmp_dft12_ptr+10,
        tmp_dft12_ptr+11,
        y3,
        y7,
        y11,
        W3_12,
        W6_12);

}




void dft12(int16_t *x,int16_t *y)
{

  simd_q15_t *x128 = (simd_q15_t *)x,*y128 = (simd_q15_t *)y;
  dft12f(&x128[0],
         &x128[1],
         &x128[2],
         &x128[3],
         &x128[4],
         &x128[5],
         &x128[6],
         &x128[7],
         &x128[8],
         &x128[9],
         &x128[10],
         &x128[11],
         &y128[0],
         &y128[1],
         &y128[2],
         &y128[3],
         &y128[4],
         &y128[5],
         &y128[6],
         &y128[7],
         &y128[8],
         &y128[9],
         &y128[10],
         &y128[11]);

  _mm_empty();
  _m_empty();

}

#ifdef __AVX2__

static int16_t W1_12s_256[16]__attribute__((aligned(32))) = {28377,-16383,28377,-16383,28377,-16383,28377,-16383,28377,-16383,28377,-16383,28377,-16383,28377,-16383};
static int16_t W2_12s_256[16]__attribute__((aligned(32))) = {16383,-28377,16383,-28377,16383,-28377,16383,-28377,16383,-28377,16383,-28377,16383,-28377,16383,-28377};
static int16_t W3_12s_256[16]__attribute__((aligned(32))) = {0,-32767,0,-32767,0,-32767,0,-32767,0,-32767,0,-32767,0,-32767,0,-32767};
static int16_t W4_12s_256[16]__attribute__((aligned(32))) = {-16383,-28377,-16383,-28377,-16383,-28377,-16383,-28377,-16383,-28377,-16383,-28377,-16383,-28377,-16383,-28377};
static int16_t W6_12s_256[16]__attribute__((aligned(32))) = {-32767,0,-32767,0,-32767,0,-32767,0,-32767,0,-32767,0,-32767,0,-32767,0};

simd256_q15_t *W1_12_256=(simd256_q15_t *)W1_12s_256;
simd256_q15_t *W2_12_256=(simd256_q15_t *)W2_12s_256;
simd256_q15_t *W3_12_256=(simd256_q15_t *)W3_12s_256;
simd256_q15_t *W4_12_256=(simd256_q15_t *)W4_12s_256;
simd256_q15_t *W6_12_256=(simd256_q15_t *)W6_12s_256;



static inline void dft12f_simd256(simd256_q15_t *x0,
				  simd256_q15_t *x1,
				  simd256_q15_t *x2,
				  simd256_q15_t *x3,
				  simd256_q15_t *x4,
				  simd256_q15_t *x5,
				  simd256_q15_t *x6,
				  simd256_q15_t *x7,
				  simd256_q15_t *x8,
				  simd256_q15_t *x9,
				  simd256_q15_t *x10,
				  simd256_q15_t *x11,
				  simd256_q15_t *y0,
				  simd256_q15_t *y1,
				  simd256_q15_t *y2,
				  simd256_q15_t *y3,
				  simd256_q15_t *y4,
				  simd256_q15_t *y5,
				  simd256_q15_t *y6,
				  simd256_q15_t *y7,
				  simd256_q15_t *y8,
				  simd256_q15_t *y9,
				  simd256_q15_t *y10,
				  simd256_q15_t *y11) __attribute__((always_inline));

static inline void dft12f_simd256(simd256_q15_t *x0,
				  simd256_q15_t *x1,
				  simd256_q15_t *x2,
				  simd256_q15_t *x3,
				  simd256_q15_t *x4,
				  simd256_q15_t *x5,
				  simd256_q15_t *x6,
				  simd256_q15_t *x7,
				  simd256_q15_t *x8,
				  simd256_q15_t *x9,
				  simd256_q15_t *x10,
				  simd256_q15_t *x11,
				  simd256_q15_t *y0,
				  simd256_q15_t *y1,
				  simd256_q15_t *y2,
				  simd256_q15_t *y3,
				  simd256_q15_t *y4,
				  simd256_q15_t *y5,
				  simd256_q15_t *y6,
				  simd256_q15_t *y7,
				  simd256_q15_t *y8,
				  simd256_q15_t *y9,
				  simd256_q15_t *y10,
				  simd256_q15_t *y11)
{


  simd256_q15_t tmp_dft12[12];

  simd256_q15_t *tmp_dft12_ptr = &tmp_dft12[0];

  // msg("dft12\n");

  bfly4_tw1_256(x0,
		x3,
		x6,
		x9,
		tmp_dft12_ptr,
		tmp_dft12_ptr+3,
		tmp_dft12_ptr+6,
		tmp_dft12_ptr+9);


  bfly4_tw1_256(x1,
		x4,
		x7,
		x10,
		tmp_dft12_ptr+1,
		tmp_dft12_ptr+4,
		tmp_dft12_ptr+7,
		tmp_dft12_ptr+10);
  

  bfly4_tw1_256(x2,
		x5,
		x8,
		x11,
		tmp_dft12_ptr+2,
		tmp_dft12_ptr+5,
		tmp_dft12_ptr+8,
		tmp_dft12_ptr+11);
  
  //  k2=0;
  bfly3_tw1_256(tmp_dft12_ptr,
		tmp_dft12_ptr+1,
		tmp_dft12_ptr+2,
		y0,
		y4,
		y8);
  
  
  
  //  k2=1;
  bfly3_256(tmp_dft12_ptr+3,
	    tmp_dft12_ptr+4,
	    tmp_dft12_ptr+5,
	    y1,
	    y5,
	    y9,
	    W1_12_256,
	    W2_12_256);
  
  
  
  //  k2=2;
  bfly3_256(tmp_dft12_ptr+6,
	    tmp_dft12_ptr+7,
	    tmp_dft12_ptr+8,
	    y2,
	    y6,
	    y10,
	    W2_12_256,
	    W4_12_256);
  
  //  k2=3;
  bfly3_256(tmp_dft12_ptr+9,
	    tmp_dft12_ptr+10,
	    tmp_dft12_ptr+11,
	    y3,
	    y7,
	    y11,
	    W3_12_256,
	    W6_12_256);
  
}




void dft12_simd256(int16_t *x,int16_t *y)
{

  simd256_q15_t *x256 = (simd256_q15_t *)x,*y256 = (simd256_q15_t *)y;
  dft12f_simd256(&x256[0],
		 &x256[1],
		 &x256[2],
		 &x256[3],
		 &x256[4],
		 &x256[5],
		 &x256[6],
		 &x256[7],
		 &x256[8],
		 &x256[9],
		 &x256[10],
		 &x256[11],
		 &y256[0],
		 &y256[1],
		 &y256[2],
		 &y256[3],
		 &y256[4],
		 &y256[5],
		 &y256[6],
		 &y256[7],
		 &y256[8],
		 &y256[9],
		 &y256[10],
		 &y256[11]);
  
  _mm_empty();
  _m_empty();

}

#endif

static int16_t tw24[88]__attribute__((aligned(32)));

void dft24(int16_t *x,int16_t *y,unsigned char scale_flag)
{

  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *tw128=(simd_q15_t *)&tw24[0];
  simd_q15_t ytmp128[24];//=&ytmp128array[0];
  int i,j,k;

  //  msg("dft24\n");
  dft12f(x128,
         x128+2,
         x128+4,
         x128+6,
         x128+8,
         x128+10,
         x128+12,
         x128+14,
         x128+16,
         x128+18,
         x128+20,
         x128+22,
         ytmp128,
         ytmp128+2,
         ytmp128+4,
         ytmp128+6,
         ytmp128+8,
         ytmp128+10,
         ytmp128+12,
         ytmp128+14,
         ytmp128+16,
         ytmp128+18,
         ytmp128+20,
         ytmp128+22);
  //  msg("dft24b\n");

  dft12f(x128+1,
         x128+3,
         x128+5,
         x128+7,
         x128+9,
         x128+11,
         x128+13,
         x128+15,
         x128+17,
         x128+19,
         x128+21,
         x128+23,
         ytmp128+1,
         ytmp128+3,
         ytmp128+5,
         ytmp128+7,
         ytmp128+9,
         ytmp128+11,
         ytmp128+13,
         ytmp128+15,
         ytmp128+17,
         ytmp128+19,
         ytmp128+21,
         ytmp128+23);

  //  msg("dft24c\n");

  bfly2_tw1(ytmp128,
            ytmp128+1,
            y128,
            y128+12);

  //  msg("dft24d\n");

  for (i=2,j=1,k=0; i<24; i+=2,j++,k++) {

    bfly2(ytmp128+i,
          ytmp128+i+1,
          y128+j,
          y128+j+12,
          tw128+k);
    //    msg("dft24e\n");
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[1]);

    for (i=0; i<24; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa36[88]__attribute__((aligned(32)));
static int16_t twb36[88]__attribute__((aligned(32)));

void dft36(int16_t *x,int16_t *y,unsigned char scale_flag)
{

  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa36[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb36[0];
  simd_q15_t ytmp128[36];//&ytmp128array[0];


  int i,j,k;

  dft12f(x128,
         x128+3,
         x128+6,
         x128+9,
         x128+12,
         x128+15,
         x128+18,
         x128+21,
         x128+24,
         x128+27,
         x128+30,
         x128+33,
         ytmp128,
         ytmp128+3,
         ytmp128+6,
         ytmp128+9,
         ytmp128+12,
         ytmp128+15,
         ytmp128+18,
         ytmp128+21,
         ytmp128+24,
         ytmp128+27,
         ytmp128+30,
         ytmp128+33);

  dft12f(x128+1,
         x128+4,
         x128+7,
         x128+10,
         x128+13,
         x128+16,
         x128+19,
         x128+22,
         x128+25,
         x128+28,
         x128+31,
         x128+34,
         ytmp128+1,
         ytmp128+4,
         ytmp128+7,
         ytmp128+10,
         ytmp128+13,
         ytmp128+16,
         ytmp128+19,
         ytmp128+22,
         ytmp128+25,
         ytmp128+28,
         ytmp128+31,
         ytmp128+34);

  dft12f(x128+2,
         x128+5,
         x128+8,
         x128+11,
         x128+14,
         x128+17,
         x128+20,
         x128+23,
         x128+26,
         x128+29,
         x128+32,
         x128+35,
         ytmp128+2,
         ytmp128+5,
         ytmp128+8,
         ytmp128+11,
         ytmp128+14,
         ytmp128+17,
         ytmp128+20,
         ytmp128+23,
         ytmp128+26,
         ytmp128+29,
         ytmp128+32,
         ytmp128+35);


  bfly3_tw1(ytmp128,
            ytmp128+1,
            ytmp128+2,
            y128,
            y128+12,
            y128+24);

  for (i=3,j=1,k=0; i<36; i+=3,j++,k++) {

    bfly3(ytmp128+i,
          ytmp128+i+1,
          ytmp128+i+2,
          y128+j,
          y128+j+12,
          y128+j+24,
          twa128+k,
          twb128+k);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[2]);

    for (i=0; i<36; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa48[88]__attribute__((aligned(32)));
static int16_t twb48[88]__attribute__((aligned(32)));
static int16_t twc48[88]__attribute__((aligned(32)));

void dft48(int16_t *x, int16_t *y,unsigned char scale_flag)
{

  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa48[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb48[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc48[0];
  simd_q15_t ytmp128[48];//=&ytmp128array[0];
  int i,j,k;


  dft12f(x128,
         x128+4,
         x128+8,
         x128+12,
         x128+16,
         x128+20,
         x128+24,
         x128+28,
         x128+32,
         x128+36,
         x128+40,
         x128+44,
         ytmp128,
         ytmp128+4,
         ytmp128+8,
         ytmp128+12,
         ytmp128+16,
         ytmp128+20,
         ytmp128+24,
         ytmp128+28,
         ytmp128+32,
         ytmp128+36,
         ytmp128+40,
         ytmp128+44);


  dft12f(x128+1,
         x128+5,
         x128+9,
         x128+13,
         x128+17,
         x128+21,
         x128+25,
         x128+29,
         x128+33,
         x128+37,
         x128+41,
         x128+45,
         ytmp128+1,
         ytmp128+5,
         ytmp128+9,
         ytmp128+13,
         ytmp128+17,
         ytmp128+21,
         ytmp128+25,
         ytmp128+29,
         ytmp128+33,
         ytmp128+37,
         ytmp128+41,
         ytmp128+45);


  dft12f(x128+2,
         x128+6,
         x128+10,
         x128+14,
         x128+18,
         x128+22,
         x128+26,
         x128+30,
         x128+34,
         x128+38,
         x128+42,
         x128+46,
         ytmp128+2,
         ytmp128+6,
         ytmp128+10,
         ytmp128+14,
         ytmp128+18,
         ytmp128+22,
         ytmp128+26,
         ytmp128+30,
         ytmp128+34,
         ytmp128+38,
         ytmp128+42,
         ytmp128+46);


  dft12f(x128+3,
         x128+7,
         x128+11,
         x128+15,
         x128+19,
         x128+23,
         x128+27,
         x128+31,
         x128+35,
         x128+39,
         x128+43,
         x128+47,
         ytmp128+3,
         ytmp128+7,
         ytmp128+11,
         ytmp128+15,
         ytmp128+19,
         ytmp128+23,
         ytmp128+27,
         ytmp128+31,
         ytmp128+35,
         ytmp128+39,
         ytmp128+43,
         ytmp128+47);



  bfly4_tw1(ytmp128,
            ytmp128+1,
            ytmp128+2,
            ytmp128+3,
            y128,
            y128+12,
            y128+24,
            y128+36);



  for (i=4,j=1,k=0; i<48; i+=4,j++,k++) {

    bfly4(ytmp128+i,
          ytmp128+i+1,
          ytmp128+i+2,
          ytmp128+i+3,
          y128+j,
          y128+j+12,
          y128+j+24,
          y128+j+36,
          twa128+k,
          twb128+k,
          twc128+k);

  }

  if (scale_flag == 1) {
    norm128 = set1_int16(dft_norm_table[3]);

    for (i=0; i<48; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa60[88]__attribute__((aligned(32)));
static int16_t twb60[88]__attribute__((aligned(32)));
static int16_t twc60[88]__attribute__((aligned(32)));
static int16_t twd60[88]__attribute__((aligned(32)));

void dft60(int16_t *x,int16_t *y,unsigned char scale)
{

  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa60[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb60[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc60[0];
  simd_q15_t *twd128=(simd_q15_t *)&twd60[0];
  simd_q15_t ytmp128[60];//=&ytmp128array[0];
  int i,j,k;

  dft12f(x128,
         x128+5,
         x128+10,
         x128+15,
         x128+20,
         x128+25,
         x128+30,
         x128+35,
         x128+40,
         x128+45,
         x128+50,
         x128+55,
         ytmp128,
         ytmp128+5,
         ytmp128+10,
         ytmp128+15,
         ytmp128+20,
         ytmp128+25,
         ytmp128+30,
         ytmp128+35,
         ytmp128+40,
         ytmp128+45,
         ytmp128+50,
         ytmp128+55);

  dft12f(x128+1,
         x128+6,
         x128+11,
         x128+16,
         x128+21,
         x128+26,
         x128+31,
         x128+36,
         x128+41,
         x128+46,
         x128+51,
         x128+56,
         ytmp128+1,
         ytmp128+6,
         ytmp128+11,
         ytmp128+16,
         ytmp128+21,
         ytmp128+26,
         ytmp128+31,
         ytmp128+36,
         ytmp128+41,
         ytmp128+46,
         ytmp128+51,
         ytmp128+56);

  dft12f(x128+2,
         x128+7,
         x128+12,
         x128+17,
         x128+22,
         x128+27,
         x128+32,
         x128+37,
         x128+42,
         x128+47,
         x128+52,
         x128+57,
         ytmp128+2,
         ytmp128+7,
         ytmp128+12,
         ytmp128+17,
         ytmp128+22,
         ytmp128+27,
         ytmp128+32,
         ytmp128+37,
         ytmp128+42,
         ytmp128+47,
         ytmp128+52,
         ytmp128+57);

  dft12f(x128+3,
         x128+8,
         x128+13,
         x128+18,
         x128+23,
         x128+28,
         x128+33,
         x128+38,
         x128+43,
         x128+48,
         x128+53,
         x128+58,
         ytmp128+3,
         ytmp128+8,
         ytmp128+13,
         ytmp128+18,
         ytmp128+23,
         ytmp128+28,
         ytmp128+33,
         ytmp128+38,
         ytmp128+43,
         ytmp128+48,
         ytmp128+53,
         ytmp128+58);

  dft12f(x128+4,
         x128+9,
         x128+14,
         x128+19,
         x128+24,
         x128+29,
         x128+34,
         x128+39,
         x128+44,
         x128+49,
         x128+54,
         x128+59,
         ytmp128+4,
         ytmp128+9,
         ytmp128+14,
         ytmp128+19,
         ytmp128+24,
         ytmp128+29,
         ytmp128+34,
         ytmp128+39,
         ytmp128+44,
         ytmp128+49,
         ytmp128+54,
         ytmp128+59);

  bfly5_tw1(ytmp128,
            ytmp128+1,
            ytmp128+2,
            ytmp128+3,
            ytmp128+4,
            y128,
            y128+12,
            y128+24,
            y128+36,
            y128+48);

  for (i=5,j=1,k=0; i<60; i+=5,j++,k++) {

    bfly5(ytmp128+i,
          ytmp128+i+1,
          ytmp128+i+2,
          ytmp128+i+3,
          ytmp128+i+4,
          y128+j,
          y128+j+12,
          y128+j+24,
          y128+j+36,
          y128+j+48,
          twa128+k,
          twb128+k,
          twc128+k,
          twd128+k);
  }

  if (scale == 1) {
    norm128 = set1_int16(dft_norm_table[4]);

    for (i=0; i<60; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
//      printf("y[%d] = (%d,%d)\n",i,((int16_t*)&y128[i])[0],((int16_t*)&y128[i])[1]);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t tw72[280]__attribute__((aligned(32)));

void dft72(int16_t *x,int16_t *y,unsigned char scale_flag)
{

  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *tw128=(simd_q15_t *)&tw72[0];
  simd_q15_t x2128[72];// = (simd_q15_t *)&x2128array[0];

  simd_q15_t ytmp128[72];//=&ytmp128array2[0];

  for (i=0,j=0; i<36; i++,j+=2) {
    x2128[i]    = x128[j];    // even inputs
    x2128[i+36] = x128[j+1];  // odd inputs
  }

  dft36((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft36((int16_t *)(x2128+36),(int16_t *)(ytmp128+36),1);

  bfly2_tw1(ytmp128,ytmp128+36,y128,y128+36);

  for (i=1,j=0; i<36; i++,j++) {
    bfly2(ytmp128+i,
          ytmp128+36+i,
          y128+i,
          y128+36+i,
          tw128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[5]);

    for (i=0; i<72; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t tw96[376]__attribute__((aligned(32)));

void dft96(int16_t *x,int16_t *y,unsigned char scale_flag)
{


  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *tw128=(simd_q15_t *)&tw96[0];
  simd_q15_t x2128[96];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[96];//=&ytmp128array2[0];


  for (i=0,j=0; i<48; i++,j+=2) {
    x2128[i]    = x128[j];
    x2128[i+48] = x128[j+1];
  }

  dft48((int16_t *)x2128,(int16_t *)ytmp128,0);
  dft48((int16_t *)(x2128+48),(int16_t *)(ytmp128+48),0);


  bfly2_tw1(ytmp128,ytmp128+48,y128,y128+48);

  for (i=1,j=0; i<48; i++,j++) {
    bfly2(ytmp128+i,
          ytmp128+48+i,
          y128+i,
          y128+48+i,
          tw128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[6]);

    for (i=0; i<96; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa108[280]__attribute__((aligned(32)));
static int16_t twb108[280]__attribute__((aligned(32)));

void dft108(int16_t *x,int16_t *y,unsigned char scale_flag)
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa108[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb108[0];
  simd_q15_t x2128[108];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[108];//=&ytmp128array2[0];


  for (i=0,j=0; i<36; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+36] = x128[j+1];
    x2128[i+72] = x128[j+2];
  }

  dft36((int16_t *)x2128,(int16_t *)ytmp128,0);
  dft36((int16_t *)(x2128+36),(int16_t *)(ytmp128+36),0);
  dft36((int16_t *)(x2128+72),(int16_t *)(ytmp128+72),0);

  bfly3_tw1(ytmp128,ytmp128+36,ytmp128+72,y128,y128+36,y128+72);

  for (i=1,j=0; i<36; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+36+i,
          ytmp128+72+i,
          y128+i,
          y128+36+i,
          y128+72+i,
          twa128+j,
          twb128+j);

  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[7]);

    for (i=0; i<108; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t tw120[472]__attribute__((aligned(32)));
void dft120(int16_t *x,int16_t *y, unsigned char scale_flag)
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *tw128=(simd_q15_t *)&tw120[0];
  simd_q15_t x2128[120];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[120];//=&ytmp128array2[0];

  for (i=0,j=0; i<60; i++,j+=2) {
    x2128[i]    = x128[j];
    x2128[i+60] = x128[j+1];
  }

  dft60((int16_t *)x2128,(int16_t *)ytmp128,0);
  dft60((int16_t *)(x2128+60),(int16_t *)(ytmp128+60),0);


  bfly2_tw1(ytmp128,ytmp128+60,y128,y128+60);

  for (i=1,j=0; i<60; i++,j++) {
    bfly2(ytmp128+i,
          ytmp128+60+i,
          y128+i,
          y128+60+i,
          tw128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[8]);

    for (i=0; i<120; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa144[376]__attribute__((aligned(32)));
static int16_t twb144[376]__attribute__((aligned(32)));

void dft144(int16_t *x,int16_t *y,unsigned char scale_flag)
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa144[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb144[0];
  simd_q15_t x2128[144];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[144];//=&ytmp128array2[0];



  for (i=0,j=0; i<48; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+48] = x128[j+1];
    x2128[i+96] = x128[j+2];
  }

  dft48((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft48((int16_t *)(x2128+48),(int16_t *)(ytmp128+48),1);
  dft48((int16_t *)(x2128+96),(int16_t *)(ytmp128+96),1);

  bfly3_tw1(ytmp128,ytmp128+48,ytmp128+96,y128,y128+48,y128+96);

  for (i=1,j=0; i<48; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+48+i,
          ytmp128+96+i,
          y128+i,
          y128+48+i,
          y128+96+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[9]);

    for (i=0; i<144; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa180[472]__attribute__((aligned(32)));
static int16_t twb180[472]__attribute__((aligned(32)));

void dft180(int16_t *x,int16_t *y,unsigned char scale_flag)
{

  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa180[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb180[0];
  simd_q15_t x2128[180];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[180];//=&ytmp128array2[0];



  for (i=0,j=0; i<60; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+60] = x128[j+1];
    x2128[i+120] = x128[j+2];
  }

  dft60((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft60((int16_t *)(x2128+60),(int16_t *)(ytmp128+60),1);
  dft60((int16_t *)(x2128+120),(int16_t *)(ytmp128+120),1);

  bfly3_tw1(ytmp128,ytmp128+60,ytmp128+120,y128,y128+60,y128+120);

  for (i=1,j=0; i<60; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+60+i,
          ytmp128+120+i,
          y128+i,
          y128+60+i,
          y128+120+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[10]);

    for (i=0; i<180; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa192[376]__attribute__((aligned(32)));
static int16_t twb192[376]__attribute__((aligned(32)));
static int16_t twc192[376]__attribute__((aligned(32)));

void dft192(int16_t *x,int16_t *y,unsigned char scale_flag)
{

  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa192[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb192[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc192[0];
  simd_q15_t x2128[192];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[192];//=&ytmp128array2[0];



  for (i=0,j=0; i<48; i++,j+=4) {
    x2128[i]    = x128[j];
    x2128[i+48] = x128[j+1];
    x2128[i+96] = x128[j+2];
    x2128[i+144] = x128[j+3];
  }

  dft48((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft48((int16_t *)(x2128+48),(int16_t *)(ytmp128+48),1);
  dft48((int16_t *)(x2128+96),(int16_t *)(ytmp128+96),1);
  dft48((int16_t *)(x2128+144),(int16_t *)(ytmp128+144),1);

  bfly4_tw1(ytmp128,ytmp128+48,ytmp128+96,ytmp128+144,y128,y128+48,y128+96,y128+144);

  for (i=1,j=0; i<48; i++,j++) {
    bfly4(ytmp128+i,
          ytmp128+48+i,
          ytmp128+96+i,
          ytmp128+144+i,
          y128+i,
          y128+48+i,
          y128+96+i,
          y128+144+i,
          twa128+j,
          twb128+j,
          twc128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[11]);

    for (i=0; i<192; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa216[568]__attribute__((aligned(32)));
static int16_t twb216[568]__attribute__((aligned(32)));

void dft216(int16_t *x,int16_t *y,unsigned char scale_flag)
{

  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa216[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb216[0];
  simd_q15_t x2128[216];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[216];//=&ytmp128array3[0];



  for (i=0,j=0; i<72; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+72] = x128[j+1];
    x2128[i+144] = x128[j+2];
  }

  dft72((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft72((int16_t *)(x2128+72),(int16_t *)(ytmp128+72),1);
  dft72((int16_t *)(x2128+144),(int16_t *)(ytmp128+144),1);

  bfly3_tw1(ytmp128,ytmp128+72,ytmp128+144,y128,y128+72,y128+144);

  for (i=1,j=0; i<72; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+72+i,
          ytmp128+144+i,
          y128+i,
          y128+72+i,
          y128+144+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[12]);

    for (i=0; i<216; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa240[472]__attribute__((aligned(32)));
static int16_t twb240[472]__attribute__((aligned(32)));
static int16_t twc240[472]__attribute__((aligned(32)));

void dft240(int16_t *x,int16_t *y,unsigned char scale_flag)
{

  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa240[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb240[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc240[0];
  simd_q15_t x2128[240];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[240];//=&ytmp128array2[0];



  for (i=0,j=0; i<60; i++,j+=4) {
    x2128[i]    = x128[j];
    x2128[i+60] = x128[j+1];
    x2128[i+120] = x128[j+2];
    x2128[i+180] = x128[j+3];
  }

  dft60((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft60((int16_t *)(x2128+60),(int16_t *)(ytmp128+60),1);
  dft60((int16_t *)(x2128+120),(int16_t *)(ytmp128+120),1);
  dft60((int16_t *)(x2128+180),(int16_t *)(ytmp128+180),1);

  bfly4_tw1(ytmp128,ytmp128+60,ytmp128+120,ytmp128+180,y128,y128+60,y128+120,y128+180);

  for (i=1,j=0; i<60; i++,j++) {
    bfly4(ytmp128+i,
          ytmp128+60+i,
          ytmp128+120+i,
          ytmp128+180+i,
          y128+i,
          y128+60+i,
          y128+120+i,
          y128+180+i,
          twa128+j,
          twb128+j,
          twc128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[13]);

    for (i=0; i<240; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa288[760]__attribute__((aligned(32)));
static int16_t twb288[760]__attribute__((aligned(32)));

void dft288(int16_t *x,int16_t *y,unsigned char scale_flag)
{

  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa288[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb288[0];
  simd_q15_t x2128[288];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[288];//=&ytmp128array3[0];



  for (i=0,j=0; i<96; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+96] = x128[j+1];
    x2128[i+192] = x128[j+2];
  }

  dft96((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft96((int16_t *)(x2128+96),(int16_t *)(ytmp128+96),1);
  dft96((int16_t *)(x2128+192),(int16_t *)(ytmp128+192),1);

  bfly3_tw1(ytmp128,ytmp128+96,ytmp128+192,y128,y128+96,y128+192);

  for (i=1,j=0; i<96; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+96+i,
          ytmp128+192+i,
          y128+i,
          y128+96+i,
          y128+192+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[14]);

    for (i=0; i<288; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa300[472]__attribute__((aligned(32)));
static int16_t twb300[472]__attribute__((aligned(32)));
static int16_t twc300[472]__attribute__((aligned(32)));
static int16_t twd300[472]__attribute__((aligned(32)));

void dft300(int16_t *x,int16_t *y,unsigned char scale_flag)
{

  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa300[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb300[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc300[0];
  simd_q15_t *twd128=(simd_q15_t *)&twd300[0];
  simd_q15_t x2128[300];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[300];//=&ytmp128array2[0];



  for (i=0,j=0; i<60; i++,j+=5) {
    x2128[i]    = x128[j];
    x2128[i+60] = x128[j+1];
    x2128[i+120] = x128[j+2];
    x2128[i+180] = x128[j+3];
    x2128[i+240] = x128[j+4];
  }

  dft60((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft60((int16_t *)(x2128+60),(int16_t *)(ytmp128+60),1);
  dft60((int16_t *)(x2128+120),(int16_t *)(ytmp128+120),1);
  dft60((int16_t *)(x2128+180),(int16_t *)(ytmp128+180),1);
  dft60((int16_t *)(x2128+240),(int16_t *)(ytmp128+240),1);

  bfly5_tw1(ytmp128,ytmp128+60,ytmp128+120,ytmp128+180,ytmp128+240,y128,y128+60,y128+120,y128+180,y128+240);

  for (i=1,j=0; i<60; i++,j++) {
    bfly5(ytmp128+i,
          ytmp128+60+i,
          ytmp128+120+i,
          ytmp128+180+i,
          ytmp128+240+i,
          y128+i,
          y128+60+i,
          y128+120+i,
          y128+180+i,
          y128+240+i,
          twa128+j,
          twb128+j,
          twc128+j,
          twd128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[15]);

    for (i=0; i<300; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

static int16_t twa324[107*2*4];
static int16_t twb324[107*2*4];

void dft324(int16_t *x,int16_t *y,unsigned char scale_flag)  // 108 x 3
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa324[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb324[0];
  simd_q15_t x2128[324];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[324];//=&ytmp128array3[0];



  for (i=0,j=0; i<108; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+108] = x128[j+1];
    x2128[i+216] = x128[j+2];
  }

  dft108((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft108((int16_t *)(x2128+108),(int16_t *)(ytmp128+108),1);
  dft108((int16_t *)(x2128+216),(int16_t *)(ytmp128+216),1);

  bfly3_tw1(ytmp128,ytmp128+108,ytmp128+216,y128,y128+108,y128+216);

  for (i=1,j=0; i<108; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+108+i,
          ytmp128+216+i,
          y128+i,
          y128+108+i,
          y128+216+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[14]);

    for (i=0; i<324; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};

static int16_t twa360[119*2*4];
static int16_t twb360[119*2*4];

void dft360(int16_t *x,int16_t *y,unsigned char scale_flag)  // 120 x 3
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa360[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb360[0];
  simd_q15_t x2128[360];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[360];//=&ytmp128array3[0];



  for (i=0,j=0; i<120; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+120] = x128[j+1];
    x2128[i+240] = x128[j+2];
  }

  dft120((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft120((int16_t *)(x2128+120),(int16_t *)(ytmp128+120),1);
  dft120((int16_t *)(x2128+240),(int16_t *)(ytmp128+240),1);

  bfly3_tw1(ytmp128,ytmp128+120,ytmp128+240,y128,y128+120,y128+240);

  for (i=1,j=0; i<120; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+120+i,
          ytmp128+240+i,
          y128+i,
          y128+120+i,
          y128+240+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[14]);

    for (i=0; i<360; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};

static int16_t twa384[95*2*4];
static int16_t twb384[95*2*4];
static int16_t twc384[95*2*4];

void dft384(int16_t *x,int16_t *y,unsigned char scale_flag)  // 96 x 4
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa384[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb384[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc384[0];
  simd_q15_t x2128[384];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[384];//=&ytmp128array2[0];



  for (i=0,j=0; i<96; i++,j+=4) {
    x2128[i]    = x128[j];
    x2128[i+96] = x128[j+1];
    x2128[i+192] = x128[j+2];
    x2128[i+288] = x128[j+3];
  }

  dft96((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft96((int16_t *)(x2128+96),(int16_t *)(ytmp128+96),1);
  dft96((int16_t *)(x2128+192),(int16_t *)(ytmp128+192),1);
  dft96((int16_t *)(x2128+288),(int16_t *)(ytmp128+288),1);

  bfly4_tw1(ytmp128,ytmp128+96,ytmp128+192,ytmp128+288,y128,y128+96,y128+192,y128+288);

  for (i=1,j=0; i<96; i++,j++) {
    bfly4(ytmp128+i,
          ytmp128+96+i,
          ytmp128+192+i,
          ytmp128+288+i,
          y128+i,
          y128+96+i,
          y128+192+i,
          y128+288+i,
          twa128+j,
          twb128+j,
          twc128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(16384);//dft_norm_table[13]);

    for (i=0; i<384; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};

static int16_t twa432[107*2*4];
static int16_t twb432[107*2*4];
static int16_t twc432[107*2*4];

void dft432(int16_t *x,int16_t *y,unsigned char scale_flag)  // 108 x 4
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa432[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb432[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc432[0];
  simd_q15_t x2128[432];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[432];//=&ytmp128array2[0];


  for (i=0,j=0; i<108; i++,j+=4) {
    x2128[i]    = x128[j];
    x2128[i+108] = x128[j+1];
    x2128[i+216] = x128[j+2];
    x2128[i+324] = x128[j+3];
  }

  dft108((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft108((int16_t *)(x2128+108),(int16_t *)(ytmp128+108),1);
  dft108((int16_t *)(x2128+216),(int16_t *)(ytmp128+216),1);
  dft108((int16_t *)(x2128+324),(int16_t *)(ytmp128+324),1);

  bfly4_tw1(ytmp128,ytmp128+108,ytmp128+216,ytmp128+324,y128,y128+108,y128+216,y128+324);

  for (i=1,j=0; i<108; i++,j++) {
    bfly4(ytmp128+i,
          ytmp128+108+i,
          ytmp128+216+i,
          ytmp128+324+i,
          y128+i,
          y128+108+i,
          y128+216+i,
          y128+324+i,
          twa128+j,
          twb128+j,
          twc128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(16384);//dft_norm_table[13]);

    for (i=0; i<432; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};
static int16_t twa480[119*2*4];
static int16_t twb480[119*2*4];
static int16_t twc480[119*2*4];

void dft480(int16_t *x,int16_t *y,unsigned char scale_flag)  // 120 x 4
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa480[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb480[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc480[0];
  simd_q15_t x2128[480];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[480];//=&ytmp128array2[0];



  for (i=0,j=0; i<120; i++,j+=4) {
    x2128[i]    = x128[j];
    x2128[i+120] = x128[j+1];
    x2128[i+240] = x128[j+2];
    x2128[i+360] = x128[j+3];
  }

  dft120((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft120((int16_t *)(x2128+120),(int16_t *)(ytmp128+120),1);
  dft120((int16_t *)(x2128+240),(int16_t *)(ytmp128+240),1);
  dft120((int16_t *)(x2128+360),(int16_t *)(ytmp128+360),1);

  bfly4_tw1(ytmp128,ytmp128+120,ytmp128+240,ytmp128+360,y128,y128+120,y128+240,y128+360);

  for (i=1,j=0; i<120; i++,j++) {
    bfly4(ytmp128+i,
          ytmp128+120+i,
          ytmp128+240+i,
          ytmp128+360+i,
          y128+i,
          y128+120+i,
          y128+240+i,
          y128+360+i,
          twa128+j,
          twb128+j,
          twc128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(16384);//dft_norm_table[13]);

    for (i=0; i<480; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};


static int16_t twa540[179*2*4];
static int16_t twb540[179*2*4];

void dft540(int16_t *x,int16_t *y,unsigned char scale_flag)  // 180 x 3
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa540[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb540[0];
  simd_q15_t x2128[540];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[540];//=&ytmp128array3[0];



  for (i=0,j=0; i<180; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+180] = x128[j+1];
    x2128[i+360] = x128[j+2];
  }

  dft180((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft180((int16_t *)(x2128+180),(int16_t *)(ytmp128+180),1);
  dft180((int16_t *)(x2128+360),(int16_t *)(ytmp128+360),1);

  bfly3_tw1(ytmp128,ytmp128+180,ytmp128+360,y128,y128+180,y128+360);

  for (i=1,j=0; i<180; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+180+i,
          ytmp128+360+i,
          y128+i,
          y128+180+i,
          y128+360+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[14]);

    for (i=0; i<540; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};

static int16_t twa576[191*2*4];
static int16_t twb576[191*2*4];

void dft576(int16_t *x,int16_t *y,unsigned char scale_flag)  // 192 x 3
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa576[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb576[0];
  simd_q15_t x2128[576];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[576];//=&ytmp128array3[0];



  for (i=0,j=0; i<192; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+192] = x128[j+1];
    x2128[i+384] = x128[j+2];
  }


  dft192((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft192((int16_t *)(x2128+192),(int16_t *)(ytmp128+192),1);
  dft192((int16_t *)(x2128+384),(int16_t *)(ytmp128+384),1);

  bfly3_tw1(ytmp128,ytmp128+192,ytmp128+384,y128,y128+192,y128+384);

  for (i=1,j=0; i<192; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+192+i,
          ytmp128+384+i,
          y128+i,
          y128+192+i,
          y128+384+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[14]);

    for (i=0; i<576; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();
};


static int16_t twa600[299*2*4];

void dft600(int16_t *x,int16_t *y,unsigned char scale_flag)  // 300 x 2
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *tw128=(simd_q15_t *)&twa600[0];
  simd_q15_t x2128[600];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[600];//=&ytmp128array2[0];


  for (i=0,j=0; i<300; i++,j+=2) {
    x2128[i]    = x128[j];
    x2128[i+300] = x128[j+1];
  }

  dft300((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft300((int16_t *)(x2128+300),(int16_t *)(ytmp128+300),1);


  bfly2_tw1(ytmp128,ytmp128+300,y128,y128+300);

  for (i=1,j=0; i<300; i++,j++) {
    bfly2(ytmp128+i,
          ytmp128+300+i,
          y128+i,
          y128+300+i,
          tw128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(ONE_OVER_SQRT2_Q15);

    for (i=0; i<600; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();
};


static int16_t twa648[215*2*4];
static int16_t twb648[215*2*4];

void dft648(int16_t *x,int16_t *y,unsigned char scale_flag)  // 216 x 3
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa648[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb648[0];
  simd_q15_t x2128[648];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[648];//=&ytmp128array3[0];



  for (i=0,j=0; i<216; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+216] = x128[j+1];
    x2128[i+432] = x128[j+2];
  }

  dft216((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft216((int16_t *)(x2128+216),(int16_t *)(ytmp128+216),1);
  dft216((int16_t *)(x2128+432),(int16_t *)(ytmp128+432),1);

  bfly3_tw1(ytmp128,ytmp128+216,ytmp128+432,y128,y128+216,y128+432);

  for (i=1,j=0; i<216; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+216+i,
          ytmp128+432+i,
          y128+i,
          y128+216+i,
          y128+432+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[14]);

    for (i=0; i<648; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};


static int16_t twa720[179*2*4];
static int16_t twb720[179*2*4];
static int16_t twc720[179*2*4];


void dft720(int16_t *x,int16_t *y,unsigned char scale_flag)  // 180 x 4
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa720[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb720[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc720[0];
  simd_q15_t x2128[720];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[720];//=&ytmp128array2[0];



  for (i=0,j=0; i<180; i++,j+=4) {
    x2128[i]    = x128[j];
    x2128[i+180] = x128[j+1];
    x2128[i+360] = x128[j+2];
    x2128[i+540] = x128[j+3];
  }

  dft180((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft180((int16_t *)(x2128+180),(int16_t *)(ytmp128+180),1);
  dft180((int16_t *)(x2128+360),(int16_t *)(ytmp128+360),1);
  dft180((int16_t *)(x2128+540),(int16_t *)(ytmp128+540),1);

  bfly4_tw1(ytmp128,ytmp128+180,ytmp128+360,ytmp128+540,y128,y128+180,y128+360,y128+540);

  for (i=1,j=0; i<180; i++,j++) {
    bfly4(ytmp128+i,
          ytmp128+180+i,
          ytmp128+360+i,
          ytmp128+540+i,
          y128+i,
          y128+180+i,
          y128+360+i,
          y128+540+i,
          twa128+j,
          twb128+j,
          twc128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(16384);//dft_norm_table[13]);

    for (i=0; i<720; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};

static int16_t twa768[191*2*4];
static int16_t twb768[191*2*4];
static int16_t twc768[191*2*4];

void dft768(int16_t *x,int16_t *y,unsigned char scale_flag) { // 192x 4;

  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa768[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb768[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc768[0];
  simd_q15_t x2128[768];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[768];//=&ytmp128array2[0];



  for (i=0,j=0; i<192; i++,j+=4) {
    x2128[i]     = x128[j];
    x2128[i+192] = x128[j+1];
    x2128[i+384] = x128[j+2];
    x2128[i+576] = x128[j+3];
  }

  dft192((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft192((int16_t *)(x2128+192),(int16_t *)(ytmp128+192),1);
  dft192((int16_t *)(x2128+384),(int16_t *)(ytmp128+384),1);
  dft192((int16_t *)(x2128+576),(int16_t *)(ytmp128+576),1);

  bfly4_tw1(ytmp128,ytmp128+192,ytmp128+384,ytmp128+576,y128,y128+192,y128+384,y128+576);

  for (i=1,j=0; i<192; i++,j++) {
    bfly4(ytmp128+i,
          ytmp128+192+i,
          ytmp128+384+i,
          ytmp128+576+i,
          y128+i,
          y128+192+i,
          y128+384+i,
          y128+576+i,
          twa128+j,
          twb128+j,
          twc128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(16384);//dft_norm_table[13]);

    for (i=0; i<768; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();


}


static int16_t twa864[287*2*4];
static int16_t twb864[287*2*4];

void dft864(int16_t *x,int16_t *y,unsigned char scale_flag)  // 288 x 3
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa864[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb864[0];
  simd_q15_t x2128[864];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[864];//=&ytmp128array3[0];



  for (i=0,j=0; i<288; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+288] = x128[j+1];
    x2128[i+576] = x128[j+2];
  }

  dft288((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft288((int16_t *)(x2128+288),(int16_t *)(ytmp128+288),1);
  dft288((int16_t *)(x2128+576),(int16_t *)(ytmp128+576),1);

  bfly3_tw1(ytmp128,ytmp128+288,ytmp128+576,y128,y128+288,y128+576);

  for (i=1,j=0; i<288; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+288+i,
          ytmp128+576+i,
          y128+i,
          y128+288+i,
          y128+576+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[14]);

    for (i=0; i<864; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};

static int16_t twa900[299*2*4];
static int16_t twb900[299*2*4];

void dft900(int16_t *x,int16_t *y,unsigned char scale_flag)  // 300 x 3
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa900[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb900[0];
  simd_q15_t x2128[900];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[900];//=&ytmp128array3[0];



  for (i=0,j=0; i<300; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+300] = x128[j+1];
    x2128[i+600] = x128[j+2];
  }

  dft300((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft300((int16_t *)(x2128+300),(int16_t *)(ytmp128+300),1);
  dft300((int16_t *)(x2128+600),(int16_t *)(ytmp128+600),1);

  bfly3_tw1(ytmp128,ytmp128+300,ytmp128+600,y128,y128+300,y128+600);

  for (i=1,j=0; i<300; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+300+i,
          ytmp128+600+i,
          y128+i,
          y128+300+i,
          y128+600+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[14]);

    for (i=0; i<900; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};


static int16_t twa960[239*2*4];
static int16_t twb960[239*2*4];
static int16_t twc960[239*2*4];


void dft960(int16_t *x,int16_t *y,unsigned char scale_flag)  // 240 x 4
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa960[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb960[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc960[0];
  simd_q15_t x2128[960];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[960];//=&ytmp128array2[0];



  for (i=0,j=0; i<240; i++,j+=4) {
    x2128[i]    = x128[j];
    x2128[i+240] = x128[j+1];
    x2128[i+480] = x128[j+2];
    x2128[i+720] = x128[j+3];
  }

  dft240((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft240((int16_t *)(x2128+240),(int16_t *)(ytmp128+240),1);
  dft240((int16_t *)(x2128+480),(int16_t *)(ytmp128+480),1);
  dft240((int16_t *)(x2128+720),(int16_t *)(ytmp128+720),1);

  bfly4_tw1(ytmp128,ytmp128+240,ytmp128+480,ytmp128+720,y128,y128+240,y128+480,y128+720);

  for (i=1,j=0; i<240; i++,j++) {
    bfly4(ytmp128+i,
          ytmp128+240+i,
          ytmp128+480+i,
          ytmp128+720+i,
          y128+i,
          y128+240+i,
          y128+480+i,
          y128+720+i,
          twa128+j,
          twb128+j,
          twc128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(16384);//dft_norm_table[13]);

    for (i=0; i<960; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};


static int16_t twa972[323*2*4];
static int16_t twb972[323*2*4];

void dft972(int16_t *x,int16_t *y,unsigned char scale_flag)  // 324 x 3
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa972[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb972[0];
  simd_q15_t x2128[972];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[972];//=&ytmp128array3[0];



  for (i=0,j=0; i<324; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+324] = x128[j+1];
    x2128[i+648] = x128[j+2];
  }

  dft324((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft324((int16_t *)(x2128+324),(int16_t *)(ytmp128+324),1);
  dft324((int16_t *)(x2128+648),(int16_t *)(ytmp128+648),1);

  bfly3_tw1(ytmp128,ytmp128+324,ytmp128+648,y128,y128+324,y128+648);

  for (i=1,j=0; i<324; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+324+i,
          ytmp128+648+i,
          y128+i,
          y128+324+i,
          y128+648+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[14]);

    for (i=0; i<972; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};

static int16_t twa1080[359*2*4];
static int16_t twb1080[359*2*4];

void dft1080(int16_t *x,int16_t *y,unsigned char scale_flag)  // 360 x 3
{
  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa1080[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb1080[0];
  simd_q15_t x2128[1080];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[1080];//=&ytmp128array3[0];



  for (i=0,j=0; i<360; i++,j+=3) {
    x2128[i]    = x128[j];
    x2128[i+360] = x128[j+1];
    x2128[i+720] = x128[j+2];
  }

  dft360((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft360((int16_t *)(x2128+360),(int16_t *)(ytmp128+360),1);
  dft360((int16_t *)(x2128+720),(int16_t *)(ytmp128+720),1);

  bfly3_tw1(ytmp128,ytmp128+360,ytmp128+720,y128,y128+360,y128+720);

  for (i=1,j=0; i<360; i++,j++) {
    bfly3(ytmp128+i,
          ytmp128+360+i,
          ytmp128+720+i,
          y128+i,
          y128+360+i,
          y128+720+i,
          twa128+j,
          twb128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(dft_norm_table[14]);

    for (i=0; i<1080; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

};

static int16_t twa1152[287*2*4];
static int16_t twb1152[287*2*4];
static int16_t twc1152[287*2*4];

void dft1152(int16_t *x,int16_t *y,unsigned char scale_flag)  // 288 x 4
{

  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa1152[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb1152[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc1152[0];
  simd_q15_t x2128[1152];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[1152];//=&ytmp128array2[0];



  for (i=0,j=0; i<288; i++,j+=4) {
    x2128[i]    = x128[j];
    x2128[i+288] = x128[j+1];
    x2128[i+576] = x128[j+2];
    x2128[i+864] = x128[j+3];
  }

  dft288((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft288((int16_t *)(x2128+288),(int16_t *)(ytmp128+288),1);
  dft288((int16_t *)(x2128+576),(int16_t *)(ytmp128+576),1);
  dft288((int16_t *)(x2128+864),(int16_t *)(ytmp128+864),1);

  bfly4_tw1(ytmp128,ytmp128+288,ytmp128+576,ytmp128+864,y128,y128+288,y128+576,y128+864);

  for (i=1,j=0; i<288; i++,j++) {
    bfly4(ytmp128+i,
          ytmp128+288+i,
          ytmp128+576+i,
          ytmp128+864+i,
          y128+i,
          y128+288+i,
          y128+576+i,
          y128+864+i,
          twa128+j,
          twb128+j,
          twc128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(16384);//dft_norm_table[13]);

    for (i=0; i<1152; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();
};

int16_t twa1200[4784];
int16_t twb1200[4784];
int16_t twc1200[4784];

void dft1200(int16_t *x,int16_t *y,unsigned char scale_flag)
{

  int i,j;
  simd_q15_t *x128=(simd_q15_t *)x;
  simd_q15_t *y128=(simd_q15_t *)y;
  simd_q15_t *twa128=(simd_q15_t *)&twa1200[0];
  simd_q15_t *twb128=(simd_q15_t *)&twb1200[0];
  simd_q15_t *twc128=(simd_q15_t *)&twc1200[0];
  simd_q15_t x2128[1200];// = (simd_q15_t *)&x2128array[0];
  simd_q15_t ytmp128[1200];//=&ytmp128array2[0];



  for (i=0,j=0; i<300; i++,j+=4) {
    x2128[i]    = x128[j];
    x2128[i+300] = x128[j+1];
    x2128[i+600] = x128[j+2];
    x2128[i+900] = x128[j+3];
  }

  dft300((int16_t *)x2128,(int16_t *)ytmp128,1);
  dft300((int16_t *)(x2128+300),(int16_t *)(ytmp128+300),1);
  dft300((int16_t *)(x2128+600),(int16_t *)(ytmp128+600),1);
  dft300((int16_t *)(x2128+900),(int16_t *)(ytmp128+900),1);

  bfly4_tw1(ytmp128,ytmp128+300,ytmp128+600,ytmp128+900,y128,y128+300,y128+600,y128+900);

  for (i=1,j=0; i<300; i++,j++) {
    bfly4(ytmp128+i,
          ytmp128+300+i,
          ytmp128+600+i,
          ytmp128+900+i,
          y128+i,
          y128+300+i,
          y128+600+i,
          y128+900+i,
          twa128+j,
          twb128+j,
          twc128+j);
  }

  if (scale_flag==1) {
    norm128 = set1_int16(16384);//dft_norm_table[13]);
    for (i=0; i<1200; i++) {
      y128[i] = mulhi_int16(y128[i],norm128);
    }
  }

  _mm_empty();
  _m_empty();

}

void init_rad4(int N,int16_t *tw) {

  int16_t *twa = tw;
  int16_t *twb = twa+(N/2);
  int16_t *twc = twb+(N/2);
  int i;

  for (i=0;i<(N/4);i++) {
    *twa = (int16_t)round(32767.0*cos(2*M_PI*i/N)); twa++;
    *twa = -(int16_t)round(32767.0*sin(2*M_PI*i/N)); twa++;
    *twb = (int16_t)round(32767.0*cos(2*M_PI*2*i/N)); twb++;
    *twb = -(int16_t)round(32767.0*sin(2*M_PI*2*i/N)); twb++;
    *twc = (int16_t)round(32767.0*cos(2*M_PI*3*i/N)); twc++;
    *twc = -(int16_t)round(32767.0*sin(2*M_PI*3*i/N)); twc++;
  }
}
void init_rad4_rep(int N,int16_t *twa,int16_t *twb,int16_t *twc) {

  int i,j;

  for (i=1;i<(N/4);i++) {
    twa[0] = (int16_t)round(32767.0*cos(2*M_PI*i/N)); 
    twa[1] = -(int16_t)round(32767.0*sin(2*M_PI*i/N));
    twb[0] = (int16_t)round(32767.0*cos(2*M_PI*2*i/N));
    twb[1] = -(int16_t)round(32767.0*sin(2*M_PI*2*i/N));
    twc[0] = (int16_t)round(32767.0*cos(2*M_PI*3*i/N));
    twc[1] = -(int16_t)round(32767.0*sin(2*M_PI*3*i/N));
    for (j=1;j<4;j++) {
      ((int32_t*)twa)[j]=((int32_t*)twa)[0];
      ((int32_t*)twb)[j]=((int32_t*)twb)[0];
      ((int32_t*)twc)[j]=((int32_t*)twc)[0];
    }
    twa+=8;
    twb+=8;
    twc+=8;
  }
}

void init_rad2(int N,int16_t *tw) {

  int16_t *twa = tw;
  int i;

  for (i=0;i<(N>>1);i++) {
    *twa = (int16_t)round(32767.0*cos(2*M_PI*i/N)); twa++;
    *twa = -(int16_t)round(32767.0*sin(2*M_PI*i/N)); twa++;
  }
}

void init_rad2_rep(int N,int16_t *twa) {

  int i,j;

  for (i=1;i<(N/2);i++) {
    twa[0] = (int16_t)round(32767.0*cos(2*M_PI*i/N)); 
    twa[1] = -(int16_t)round(32767.0*sin(2*M_PI*i/N));
    for (j=1;j<4;j++) {
      ((int32_t*)twa)[j]=((int32_t*)twa)[0];
    }
    twa+=8;
  }
}

void init_rad3(int N,int16_t *twa,int16_t *twb) {

  int i;

  for (i=0;i<(N/3);i++) {
    *twa = (int16_t)round(32767.0*cos(2*M_PI*i/N)); twa++;
    *twa = -(int16_t)round(32767.0*sin(2*M_PI*i/N)); twa++;
    *twb = (int16_t)round(32767.0*cos(2*M_PI*2*i/N)); twb++;
    *twb = -(int16_t)round(32767.0*sin(2*M_PI*2*i/N)); twb++;
  }
}

void init_rad3_rep(int N,int16_t *twa,int16_t *twb) {

  int i,j;

  for (i=1;i<(N/3);i++) {
    twa[0] = (int16_t)round(32767.0*cos(2*M_PI*i/N)); 
    twa[1] = -(int16_t)round(32767.0*sin(2*M_PI*i/N));
    twb[0] = (int16_t)round(32767.0*cos(2*M_PI*2*i/N));
    twb[1] = -(int16_t)round(32767.0*sin(2*M_PI*2*i/N));
    for (j=1;j<4;j++) {
      ((int32_t*)twa)[j]=((int32_t*)twa)[0];
      ((int32_t*)twb)[j]=((int32_t*)twb)[0];
    }
    twa+=8;
    twb+=8;
  }
}

void init_rad5_rep(int N,int16_t *twa,int16_t *twb,int16_t *twc,int16_t *twd) {

  int i,j;

  for (i=1;i<(N/5);i++) {
    twa[0] = (int16_t)round(32767.0*cos(2*M_PI*i/N)); 
    twa[1] = -(int16_t)round(32767.0*sin(2*M_PI*i/N));
    twb[0] = (int16_t)round(32767.0*cos(2*M_PI*2*i/N));
    twb[1] = -(int16_t)round(32767.0*sin(2*M_PI*2*i/N));
    twc[0] = (int16_t)round(32767.0*cos(2*M_PI*3*i/N));
    twc[1] = -(int16_t)round(32767.0*sin(2*M_PI*3*i/N));
    twd[0] = (int16_t)round(32767.0*cos(2*M_PI*4*i/N));
    twd[1] = -(int16_t)round(32767.0*sin(2*M_PI*4*i/N));
    for (j=1;j<4;j++) {
      ((int32_t*)twa)[j]=((int32_t*)twa)[0];
      ((int32_t*)twb)[j]=((int32_t*)twb)[0];
      ((int32_t*)twc)[j]=((int32_t*)twc)[0];
      ((int32_t*)twd)[j]=((int32_t*)twd)[0];
    }
    twa+=8;
    twb+=8;
    twc+=8;
    twd+=8;
  }
}


void init_dfts(void)
{
  init_rad4(1024,tw1024);
  init_rad2(2048,tw2048);
  init_rad4(4096,tw4096);
  init_rad2(8192,tw8192);
  
  init_rad3(1536,twa1536,twb1536);
  init_rad3(3072,twa3072,twb3072);
  init_rad3(6144,twa6144,twb6144);
  init_rad3(12288,twa12288,twb12288);
  init_rad3(18432,twa18432,twb18432);
  init_rad3(24576,twa24576,twb24576);

  init_rad2_rep(24,tw24);
  init_rad3_rep(36,twa36,twb36);
  init_rad4_rep(48,twa48,twb48,twc48);
  init_rad5_rep(60,twa60,twb60,twc60,twd60);
  init_rad2_rep(72,tw72);
  init_rad2_rep(96,tw96);
  init_rad3_rep(108,twa108,twb108);
  init_rad2_rep(120,tw120);
  init_rad3_rep(144,twa144,twb144);
  init_rad3_rep(180,twa180,twb180);
  init_rad4_rep(192,twa192,twb192,twc192);
  init_rad3_rep(216,twa216,twb216);
  init_rad4_rep(240,twa240,twb240,twc240);
  init_rad3_rep(288,twa288,twb288);
  init_rad5_rep(300,twa300,twb300,twc300,twd300);
  init_rad3_rep(324,twa324,twb324);
  init_rad3_rep(360,twa360,twb360);
  init_rad4_rep(384,twa384,twb384,twc384);
  init_rad4_rep(432,twa432,twb432,twc432);
  init_rad4_rep(480,twa480,twb480,twc480);
  init_rad3_rep(540,twa540,twb540);
  init_rad3_rep(576,twa576,twb576);
  init_rad2_rep(600,twa600);
  init_rad3_rep(648,twa648,twb648);
  init_rad4_rep(720,twa720,twb720,twc720);
  init_rad4_rep(768,twa768,twb768,twc768);
  init_rad3_rep(864,twa864,twb864);
  init_rad3_rep(900,twa900,twb900);
  init_rad4_rep(960,twa960,twb960,twc960);
  init_rad3_rep(972,twa972,twb972);
  init_rad3_rep(1080,twa1080,twb1080);
  init_rad4_rep(1152,twa1152,twb1152,twc1152);
  init_rad4_rep(1200,twa1200,twb1200,twc1200);
}

#ifdef MR_MAIN
#include <string.h>
#include <stdio.h>

#define LOG_M write_output
int write_output(const char *fname,const char *vname,void *data,int length,int dec,char format)
{

  FILE *fp=NULL;
  int i;


  printf("Writing %d elements of type %d to %s\n",length,format,fname);


  if (format == 10 || format ==11 || format == 12 || format == 13 || format == 14) {
    fp = fopen(fname,"a+");
  } else if (format != 10 && format !=11  && format != 12 && format != 13 && format != 14) {
    fp = fopen(fname,"w+");
  }



  if (fp== NULL) {
    printf("[OPENAIR][FILE OUTPUT] Cannot open file %s\n",fname);
    return(-1);
  }

  if (format != 10 && format !=11  && format != 12 && format != 13 && format != 14)
    fprintf(fp,"%s = [",vname);


  switch (format) {
  case 0:   // real 16-bit

    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%d\n",((short *)data)[i]);
    }

    break;

  case 1:  // complex 16-bit
  case 13:
  case 14:
  case 15:

    for (i=0; i<length<<1; i+=(2*dec)) {
      fprintf(fp,"%d + j*(%d)\n",((short *)data)[i],((short *)data)[i+1]);

    }


    break;

  case 2:  // real 32-bit
    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%d\n",((int *)data)[i]);
    }

    break;

  case 3: // complex 32-bit
    for (i=0; i<length<<1; i+=(2*dec)) {
      fprintf(fp,"%d + j*(%d)\n",((int *)data)[i],((int *)data)[i+1]);
    }

    break;

  case 4: // real 8-bit
    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%d\n",((char *)data)[i]);
    }

    break;

  case 5: // complex 8-bit
    for (i=0; i<length<<1; i+=(2*dec)) {
      fprintf(fp,"%d + j*(%d)\n",((char *)data)[i],((char *)data)[i+1]);
    }

    break;

  case 6:  // real 64-bit
    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%lld\n",((long long*)data)[i]);
    }

    break;

  case 7: // real double
    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%g\n",((double *)data)[i]);
    }

    break;

  case 8: // complex double
    for (i=0; i<length<<1; i+=2*dec) {
      fprintf(fp,"%g + j*(%g)\n",((double *)data)[i], ((double *)data)[i+1]);
    }

    break;

  case 9: // real unsigned 8-bit
    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%d\n",((unsigned char *)data)[i]);
    }

    break;


  case 10 : // case eren 16 bit complex :

    for (i=0; i<length<<1; i+=(2*dec)) {

      if((i < 2*(length-1)) && (i > 0))
        fprintf(fp,"%d + j*(%d),",((short *)data)[i],((short *)data)[i+1]);
      else if (i == 2*(length-1))
        fprintf(fp,"%d + j*(%d);",((short *)data)[i],((short *)data)[i+1]);
      else if (i == 0)
        fprintf(fp,"\n%d + j*(%d),",((short *)data)[i],((short *)data)[i+1]);



    }

    break;

  case 11 : //case eren 16 bit real for channel magnitudes:
    for (i=0; i<length; i+=dec) {

      if((i <(length-1))&& (i > 0))
        fprintf(fp,"%d,",((short *)data)[i]);
      else if (i == (length-1))
        fprintf(fp,"%d;",((short *)data)[i]);
      else if (i == 0)
        fprintf(fp,"\n%d,",((short *)data)[i]);
    }

    printf("\n erennnnnnnnnnnnnnn: length :%d",length);
    break;

  case 12 : // case eren for log2_maxh real unsigned 8 bit
    fprintf(fp,"%d \n",((unsigned char *)&data)[0]);
    break;

  }

  if (format != 10 && format !=11 && format !=12 && format != 13 && format != 15) {
    fprintf(fp,"];\n");
    fclose(fp);
    return(0);
  } else if (format == 10 || format ==11 || format == 12 || format == 13 || format == 15) {
    fclose(fp);
    return(0);
  }

  return 0;
}


int main(int argc, char**argv)
{


  time_stats_t ts;
#ifdef __AVX2__
  simd256_q15_t x[4096],y[4096],tw0,tw1,tw2,tw3;
#else
  simd_q15_t x[8192],y[8192],tw0,tw1,tw2,tw3;
#endif
  int i;
  simd_q15_t *x128=(simd_q15_t*)x,*y128=(simd_q15_t*)y;

  init_dfts();

  set_taus_seed(0);
  opp_enabled = 1;
 /* 
    ((int16_t *)&tw0)[0] = 32767;
    ((int16_t *)&tw0)[1] = 0;
    ((int16_t *)&tw0)[2] = 32767;
    ((int16_t *)&tw0)[3] = 0;
    ((int16_t *)&tw0)[4] = 32767;
    ((int16_t *)&tw0)[5] = 0;
    ((int16_t *)&tw0)[6] = 32767;
    ((int16_t *)&tw0)[7] = 0;

    ((int16_t *)&tw1)[0] = 32767;
    ((int16_t *)&tw1)[1] = 0;
    ((int16_t *)&tw1)[2] = 32767;
    ((int16_t *)&tw1)[3] = 0;
    ((int16_t *)&tw1)[4] = 32767;
    ((int16_t *)&tw1)[5] = 0;
    ((int16_t *)&tw1)[6] = 32767;
    ((int16_t *)&tw1)[7] = 0;

    ((int16_t *)&tw2)[0] = 32767;
    ((int16_t *)&tw2)[1] = 0;
    ((int16_t *)&tw2)[2] = 32767;
    ((int16_t *)&tw2)[3] = 0;
    ((int16_t *)&tw2)[4] = 32767;
    ((int16_t *)&tw2)[5] = 0;
    ((int16_t *)&tw2)[6] = 32767;
    ((int16_t *)&tw2)[7] = 0;

    ((int16_t *)&tw3)[0] = 32767;
    ((int16_t *)&tw3)[1] = 0;
    ((int16_t *)&tw3)[2] = 32767;
    ((int16_t *)&tw3)[3] = 0;
    ((int16_t *)&tw3)[4] = 32767;
    ((int16_t *)&tw3)[5] = 0;
    ((int16_t *)&tw3)[6] = 32767;
    ((int16_t *)&tw3)[7] = 0;
 */
    for (i=0;i<300;i++) {
#if defined(__x86_64__) || defined(__i386__)
#ifndef __AVX2__
      x[i] = _mm_set1_epi32(taus());
      x[i] = _mm_srai_epi16(x[i],4);
#else
      x[i] = _mm256_set1_epi32(taus());
      x[i] = _mm256_srai_epi16(x[i],4);
#endif
#elif defined(__arm__)
      x[i] = (int16x8_t)vdupq_n_s32(taus());
      x[i] = vshrq_n_s16(x[i],4);
#endif
    }
      /*
    bfly2_tw1(x,x+1,y,y+1);
    printf("(%d,%d) (%d,%d) => (%d,%d) (%d,%d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&y[0])[0],((int16_t*)&y[0])[1],((int16_t*)&y[1])[0],((int16_t*)&y[1])[1]);
    printf("(%d,%d) (%d,%d) => (%d,%d) (%d,%d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&y[0])[2],((int16_t*)&y[0])[3],((int16_t*)&y[1])[2],((int16_t*)&y[1])[3]);
    printf("(%d,%d) (%d,%d) => (%d,%d) (%d,%d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&y[0])[4],((int16_t*)&y[0])[5],((int16_t*)&y[1])[4],((int16_t*)&y[1])[5]);
    printf("(%d,%d) (%d,%d) => (%d,%d) (%d,%d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&y[0])[6],((int16_t*)&y[0])[7],((int16_t*)&y[1])[6],((int16_t*)&y[1])[7]);
    bfly2(x,x+1,y,y+1, &tw0);
    printf("0(%d,%d) (%d,%d) => (%d,%d) (%d,%d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&y[0])[0],((int16_t*)&y[0])[1],((int16_t*)&y[1])[0],((int16_t*)&y[1])[1]);
    printf("1(%d,%d) (%d,%d) => (%d,%d) (%d,%d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&y[0])[2],((int16_t*)&y[0])[3],((int16_t*)&y[1])[2],((int16_t*)&y[1])[3]);
    printf("2(%d,%d) (%d,%d) => (%d,%d) (%d,%d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&y[0])[4],((int16_t*)&y[0])[5],((int16_t*)&y[1])[4],((int16_t*)&y[1])[5]);
    printf("3(%d,%d) (%d,%d) => (%d,%d) (%d,%d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&y[0])[6],((int16_t*)&y[0])[7],((int16_t*)&y[1])[6],((int16_t*)&y[1])[7]);
    bfly2(x,x+1,y,y+1, &tw0);

    bfly3_tw1(x,x+1,x+2,y, y+1,y+2);
    printf("0(%d,%d) (%d,%d) (%d %d) => (%d,%d) (%d,%d) (%d %d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&y[0])[0],((int16_t*)&y[0])[1],((int16_t*)&y[1])[0],((int16_t*)&y[1])[1],((int16_t*)&y[2])[0],((int16_t*)&y[2])[1]);
    printf("1(%d,%d) (%d,%d) (%d %d) => (%d,%d) (%d,%d) (%d %d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&y[0])[2],((int16_t*)&y[0])[3],((int16_t*)&y[1])[2],((int16_t*)&y[1])[3],((int16_t*)&y[2])[2],((int16_t*)&y[2])[3]);
    printf("2(%d,%d) (%d,%d) (%d %d) => (%d,%d) (%d,%d) (%d %d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&y[0])[4],((int16_t*)&y[0])[5],((int16_t*)&y[1])[4],((int16_t*)&y[1])[5],((int16_t*)&y[2])[4],((int16_t*)&y[2])[5]);
    printf("3(%d,%d) (%d,%d) (%d %d) => (%d,%d) (%d,%d) (%d %d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&y[0])[6],((int16_t*)&y[0])[7],((int16_t*)&y[1])[6],((int16_t*)&y[1])[7],((int16_t*)&y[2])[6],((int16_t*)&y[2])[7]);
    bfly3(x,x+1,x+2,y, y+1,y+2,&tw0,&tw1);

    printf("0(%d,%d) (%d,%d) (%d %d) => (%d,%d) (%d,%d) (%d %d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&y[0])[0],((int16_t*)&y[0])[1],((int16_t*)&y[1])[0],((int16_t*)&y[1])[1],((int16_t*)&y[2])[0],((int16_t*)&y[2])[1]);
    printf("1(%d,%d) (%d,%d) (%d %d) => (%d,%d) (%d,%d) (%d %d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&y[0])[2],((int16_t*)&y[0])[3],((int16_t*)&y[1])[2],((int16_t*)&y[1])[3],((int16_t*)&y[2])[2],((int16_t*)&y[2])[3]);
    printf("2(%d,%d) (%d,%d) (%d %d) => (%d,%d) (%d,%d) (%d %d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&y[0])[4],((int16_t*)&y[0])[5],((int16_t*)&y[1])[4],((int16_t*)&y[1])[5],((int16_t*)&y[2])[4],((int16_t*)&y[2])[5]);
    printf("3(%d,%d) (%d,%d) (%d %d) => (%d,%d) (%d,%d) (%d %d)\n",((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&y[0])[6],((int16_t*)&y[0])[7],((int16_t*)&y[1])[6],((int16_t*)&y[1])[7],((int16_t*)&y[2])[6],((int16_t*)&y[2])[7]);


    bfly4_tw1(x,x+1,x+2,x+3,y, y+1,y+2,y+3);
    printf("(%d,%d) (%d,%d) (%d %d) (%d,%d) => (%d,%d) (%d,%d) (%d %d) (%d,%d)\n",
     ((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],
     ((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&x[3])[0],((int16_t*)&x[3])[1],
     ((int16_t*)&y[0])[0],((int16_t*)&y[0])[1],((int16_t*)&y[1])[0],((int16_t*)&y[1])[1],
     ((int16_t*)&y[2])[0],((int16_t*)&y[2])[1],((int16_t*)&y[3])[0],((int16_t*)&y[3])[1]);

    bfly4(x,x+1,x+2,x+3,y, y+1,y+2,y+3,&tw0,&tw1,&tw2);
    printf("0(%d,%d) (%d,%d) (%d %d) (%d,%d) => (%d,%d) (%d,%d) (%d %d) (%d,%d)\n",
     ((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],
     ((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&x[3])[0],((int16_t*)&x[3])[1],
     ((int16_t*)&y[0])[0],((int16_t*)&y[0])[1],((int16_t*)&y[1])[0],((int16_t*)&y[1])[1],
     ((int16_t*)&y[2])[0],((int16_t*)&y[2])[1],((int16_t*)&y[3])[0],((int16_t*)&y[3])[1]);
    printf("1(%d,%d) (%d,%d) (%d %d) (%d,%d) => (%d,%d) (%d,%d) (%d %d) (%d,%d)\n",
     ((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],
     ((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&x[3])[0],((int16_t*)&x[3])[1],
     ((int16_t*)&y[0])[2],((int16_t*)&y[0])[3],((int16_t*)&y[1])[2],((int16_t*)&y[1])[3],
     ((int16_t*)&y[2])[2],((int16_t*)&y[2])[3],((int16_t*)&y[3])[2],((int16_t*)&y[3])[3]);
    printf("2(%d,%d) (%d,%d) (%d %d) (%d,%d) => (%d,%d) (%d,%d) (%d %d) (%d,%d)\n",
     ((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],
     ((int16_t*)&x[2])[0],((int16_t*)&x[2])[1],((int16_t*)&x[3])[0],((int16_t*)&x[3])[1],
     ((int16_t*)&y[0])[4],((int16_t*)&y[0])[5],((int16_t*)&y[1])[4],((int16_t*)&y[1])[5],
     ((int16_t*)&y[2])[4],((int16_t*)&y[2])[5],((int16_t*)&y[3])[4],((int16_t*)&y[3])[5]);
    printf("3(%d,%d) (%d,%d) (%d %d) (%d,%d) => (%d,%d) (%d,%d) (%d %d) (%d,%d)\n",
     ((int16_t*)&x[0])[0],((int16_t*)&x[0])[1],((int16_t*)&x[1])[0],((int16_t*)&x[1])[1],
     ((int16_t*)&x[2])[6],((int16_t*)&x[2])[7],((int16_t*)&x[3])[6],((int16_t*)&x[3])[7],
     ((int16_t*)&y[0])[6],((int16_t*)&y[0])[7],((int16_t*)&y[1])[6],((int16_t*)&y[1])[7],
     ((int16_t*)&y[2])[0],((int16_t*)&y[2])[1],((int16_t*)&y[3])[0],((int16_t*)&y[3])[1]);

    bfly5_tw1(x,x+1,x+2,x+3,x+4,y,y+1,y+2,y+3,y+4);

    for (i=0;i<5;i++)
      printf("%d,%d,",
       ((int16_t*)&x[i])[0],((int16_t*)&x[i])[1]);
    printf("\n");
    for (i=0;i<5;i++)
      printf("%d,%d,",
       ((int16_t*)&y[i])[0],((int16_t*)&y[i])[1]);
    printf("\n");

    bfly5(x,x+1,x+2,x+3,x+4,y, y+1,y+2,y+3,y+4,&tw0,&tw1,&tw2,&tw3);
    for (i=0;i<5;i++)
      printf("%d,%d,",
       ((int16_t*)&x[i])[0],((int16_t*)&x[i])[1]);
    printf("\n");
    for (i=0;i<5;i++)
      printf("%d,%d,",
       ((int16_t*)&y[i])[0],((int16_t*)&y[i])[1]);
    printf("\n");


    printf("\n\n12-point\n");
    dft12f(x,
     x+1,
     x+2,
     x+3,
     x+4,
     x+5,
     x+6,
     x+7,
     x+8,
     x+9,
     x+10,
     x+11,
     y,
     y+1,
     y+2,
     y+3,
     y+4,
     y+5,
     y+6,
     y+7,
     y+8,
     y+9,
     y+10,
     y+11);


    printf("X: ");
    for (i=0;i<12;i++)
      printf("%d,%d,",((int16_t*)(&x[i]))[0],((int16_t *)(&x[i]))[1]);
    printf("\nY:");
    for (i=0;i<12;i++)
      printf("%d,%d,",((int16_t*)(&y[i]))[0],((int16_t *)(&y[i]))[1]);
    printf("\n");

 */

    for (i=0;i<32;i++) {
      ((int16_t*)x)[i] = (int16_t)((taus()&0xffff))>>5;
    }
    memset((void*)&y[0],0,16*4);
    idft16((int16_t *)x,(int16_t *)y);
    printf("\n\n16-point\n");
    printf("X: ");
    for (i=0;i<4;i++)
      printf("%d,%d,%d,%d,%d,%d,%d,%d,",((int16_t*)&x[i])[0],((int16_t *)&x[i])[1],((int16_t*)&x[i])[2],((int16_t *)&x[i])[3],((int16_t*)&x[i])[4],((int16_t*)&x[i])[5],((int16_t*)&x[i])[6],((int16_t*)&x[i])[7]);
    printf("\nY:");

    for (i=0;i<4;i++)
      printf("%d,%d,%d,%d,%d,%d,%d,%d,",((int16_t*)&y[i])[0],((int16_t *)&y[i])[1],((int16_t*)&y[i])[2],((int16_t *)&y[i])[3],((int16_t*)&y[i])[4],((int16_t *)&y[i])[5],((int16_t*)&y[i])[6],((int16_t *)&y[i])[7]);
    printf("\n");
 
  memset((void*)&x[0],0,2048*4);
      
  for (i=0; i<2048; i+=4) {
     ((int16_t*)x)[i<<1] = 1024;
     ((int16_t*)x)[1+(i<<1)] = 0;
     ((int16_t*)x)[2+(i<<1)] = 0;
     ((int16_t*)x)[3+(i<<1)] = 1024;
     ((int16_t*)x)[4+(i<<1)] = -1024;
     ((int16_t*)x)[5+(i<<1)] = 0;
     ((int16_t*)x)[6+(i<<1)] = 0;
     ((int16_t*)x)[7+(i<<1)] = -1024;
     }
  /*
  for (i=0; i<2048; i+=2) {
     ((int16_t*)x)[i<<1] = 1024;
     ((int16_t*)x)[1+(i<<1)] = 0;
     ((int16_t*)x)[2+(i<<1)] = -1024;
     ((int16_t*)x)[3+(i<<1)] = 0;
     }
       
  for (i=0;i<2048*2;i++) {
    ((int16_t*)x)[i] = i/2;//(int16_t)((taus()&0xffff))>>5;
  }
     */
  memset((void*)&x[0],0,64*sizeof(int32_t));
  for (i=2;i<36;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=(128-36);i<128;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  idft64((int16_t *)x,(int16_t *)y,1);
  

  printf("64-point\n");
  printf("X: ");
  for (i=0;i<8;i++)
    print_shorts256("",((int16_t *)x)+(i*16));

  printf("\nY:");

  for (i=0;i<8;i++)
    print_shorts256("",((int16_t *)y)+(i*16));
  printf("\n");

  


  idft64((int16_t *)x,(int16_t *)y,1);
  idft64((int16_t *)x,(int16_t *)y,1);
  idft64((int16_t *)x,(int16_t *)y,1);
  reset_meas(&ts);

  for (i=0; i<10000000; i++) {
    start_meas(&ts);
    idft64((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);

  }
  /*
  printf("\n\n64-point (%f cycles, #trials %d)\n",(double)ts.diff/(double)ts.trials,ts.trials);
  //  LOG_M("x64.m","x64",x,64,1,1);
  LOG_M("y64.m","y64",y,64,1,1);
  LOG_M("x64.m","x64",x,64,1,1);
  */
/*
  printf("X: ");
  for (i=0;i<16;i++)
    printf("%d,%d,%d,%d,%d,%d,%d,%d,",((int16_t*)&x[i])[0],((int16_t *)&x[i])[1],((int16_t*)&x[i])[2],((int16_t *)&x[i])[3],((int16_t*)&x[i])[4],((int16_t*)&x[i])[5],((int16_t*)&x[i])[6],((int16_t*)&x[i])[7]);
  printf("\nY:");

  for (i=0;i<16;i++)
    printf("%d,%d,%d,%d,%d,%d,%d,%d,",((int16_t*)&y[i])[0],((int16_t *)&y[i])[1],((int16_t*)&y[i])[2],((int16_t *)&y[i])[3],((int16_t*)&y[i])[4],((int16_t *)&y[i])[5],((int16_t*)&y[i])[6],((int16_t *)&y[i])[7]);
  printf("\n");

  idft64((int16_t*)y,(int16_t*)x,1);
  printf("X: ");
  for (i=0;i<16;i++)
    printf("%d,%d,%d,%d,%d,%d,%d,%d,",((int16_t*)&x[i])[0],((int16_t *)&x[i])[1],((int16_t*)&x[i])[2],((int16_t *)&x[i])[3],((int16_t*)&x[i])[4],((int16_t*)&x[i])[5],((int16_t*)&x[i])[6],((int16_t*)&x[i])[7]);
 
  for (i=0; i<256; i++) {
    ((int16_t*)x)[i] = (int16_t)((taus()&0xffff))>>5;
  }
*/
  
  memset((void*)&x[0],0,128*4);
  for (i=2;i<72;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=(256-72);i<256;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);

  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft128((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n128-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y128.m","y128",y,128,1,1);
  LOG_M("x128.m","x128",x,128,1,1);
/*
  printf("X: ");
   for (i=0;i<32;i++)
     printf("%d,%d,%d,%d,%d,%d,%d,%d,",((int16_t*)&x[i])[0],((int16_t *)&x[i])[1],((int16_t*)&x[i])[2],((int16_t *)&x[i])[3],((int16_t*)&x[i])[4],((int16_t*)&x[i])[5],((int16_t*)&x[i])[6],((int16_t*)&x[i])[7]);
   printf("\nY:");

   for (i=0;i<32;i++)
     printf("%d,%d,%d,%d,%d,%d,%d,%d,",((int16_t*)&y[i])[0],((int16_t *)&y[i])[1],((int16_t*)&y[i])[2],((int16_t *)&y[i])[3],((int16_t*)&y[i])[4],((int16_t *)&y[i])[5],((int16_t*)&y[i])[6],((int16_t *)&y[i])[7]);
   printf("\n");
*/

  /*
  for (i=0; i<512; i++) {
    ((int16_t*)x)[i] = (int16_t)((taus()&0xffff))>>5;
  }
  
  memset((void*)&y[0],0,256*4);
  */
  memset((void*)&x[0],0,256*sizeof(int32_t));
  for (i=2;i<144;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=(512-144);i<512;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);

  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft256((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n256-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y256.m","y256",y,256,1,1);
  LOG_M("x256.m","x256",x,256,1,1);

  memset((void*)&x[0],0,512*sizeof(int32_t));
  for (i=2;i<302;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=(1024-300);i<1024;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }

  reset_meas(&ts);
  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft512((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n512-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y512.m","y512",y,512,1,1);
  LOG_M("x512.m","x512",x,512,1,1);
  /*
  printf("X: ");
  for (i=0;i<64;i++)
    printf("%d,%d,%d,%d,%d,%d,%d,%d,",((int16_t*)&x[i])[0],((int16_t *)&x[i])[1],((int16_t*)&x[i])[2],((int16_t *)&x[i])[3],((int16_t*)&x[i])[4],((int16_t*)&x[i])[5],((int16_t*)&x[i])[6],((int16_t*)&x[i])[7]);
  printf("\nY:");

  for (i=0;i<64;i++)
    printf("%d,%d,%d,%d,%d,%d,%d,%d,",((int16_t*)&y[i])[0],((int16_t *)&y[i])[1],((int16_t*)&y[i])[2],((int16_t *)&y[i])[3],((int16_t*)&y[i])[4],((int16_t *)&y[i])[5],((int16_t*)&y[i])[6],((int16_t *)&y[i])[7]);
  printf("\n");
  */

  memset((void*)x,0,1024*sizeof(int32_t));
  for (i=2;i<602;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=2*724;i<2048;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);

  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft1024((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n1024-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y1024.m","y1024",y,1024,1,1);
  LOG_M("x1024.m","x1024",x,1024,1,1);

  memset((void*)x,0,2048*sizeof(int32_t));
  for (i=2;i<1202;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=2*(2048-600);i<4096;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);

  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft2048((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n2048-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y2048.m","y2048",y,2048,1,1);
  LOG_M("x2048.m","x2048",x,2048,1,1);

  memset((void*)x,0,2048*sizeof(int32_t));
  for (i=2;i<2402;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=2*(4096-1200);i<8192;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);

  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft4096((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n4096-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y4096.m","y4096",y,4096,1,1);
  LOG_M("x4096.m","x4096",x,4096,1,1);

  memset((void*)x,0,8192*sizeof(int32_t));
  for (i=2;i<4802;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=2*(8192-2400);i<16384;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);
  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft8192((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n1536-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y8192.m","y8192",y,8192,1,1);
  LOG_M("x8192.m","x8192",x,8192,1,1);

  memset((void*)x,0,1536*sizeof(int32_t));
  for (i=2;i<1202;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=2*(1536-600);i<3072;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);
  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft1536((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n1536-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y1536.m","y1536",y,1536,1,1);
  LOG_M("x1536.m","x1536",x,1536,1,1);

  printf("\n\n1536-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y8192.m","y8192",y,8192,1,1);
  LOG_M("x8192.m","x8192",x,8192,1,1);

  memset((void*)x,0,3072*sizeof(int32_t));
  for (i=2;i<1202;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=2*(3072-600);i<3072;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);
  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft3072((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n3072-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y3072.m","y3072",y,3072,1,1);
  LOG_M("x3072.m","x3072",x,3072,1,1);

  memset((void*)x,0,6144*sizeof(int32_t));
  for (i=2;i<4802;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=2*(6144-2400);i<12288;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);
  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft6144((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n6144-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y6144.m","y6144",y,6144,1,1);
  LOG_M("x6144.m","x6144",x,6144,1,1);

  memset((void*)x,0,12288*sizeof(int32_t));
  for (i=2;i<9602;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=2*(12288-4800);i<24576;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);
  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft12288((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n12288-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y12288.m","y12288",y,12288,1,1);
  LOG_M("x12288.m","x12288",x,12288,1,1);

  memset((void*)x,0,18432*sizeof(int32_t));
  for (i=2;i<14402;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=2*(18432-7200);i<36864;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);
  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft18432((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n18432-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y18432.m","y18432",y,18432,1,1);
  LOG_M("x18432.m","x18432",x,18432,1,1);

  memset((void*)x,0,24576*sizeof(int32_t));
  for (i=2;i<19202;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  for (i=2*(24576-19200);i<49152;i++) {
    if ((taus() & 1)==0)
      ((int16_t*)x)[i] = 364;
    else
      ((int16_t*)x)[i] = -364;
  }
  reset_meas(&ts);
  for (i=0; i<10000; i++) {
    start_meas(&ts);
    idft24576((int16_t *)x,(int16_t *)y,1);
    stop_meas(&ts);
  }

  printf("\n\n24576-point(%f cycles)\n",(double)ts.diff/(double)ts.trials);
  LOG_M("y24576.m","y24576",y,24576,1,1);
  LOG_M("x24576.m","x24576",x,24576,1,1);

  int dftsizes[33]={24,36,48,60,72,96,108,120,144,180,192,216,240,288,300,324,360,384,432,480,540,576,600,648,720,768,864,900,960,972,1080,1152,1200};
  void (*dft)(int16_t *x,int16_t *y,uint8_t scale)[33] = {dft24,dft36,dft48,dft60,dft72,dft96,dft108,dft120,dft144,dft180,dft192,dft216,dft240,dft288,dft300,dft324,dft360,dft384,dft432,dft480,dft540,dft576,dft600,dft648,dft720,dft768,dft864,dft900,dft960,dft972,dft1080,dft1152,dft1200};
  for (int n=0;n<33;n++) {
    // 4xN-point DFT
    memset((void*)x,0,dftsizes[n]*8*sizeof(int16_t));
    for (i=0;i<dftsizes[n]*8;i+=8) {
      if ((taus() & 1)==0)
	((int16_t*)x)[i]   = 364;
      else
	((int16_t*)x)[i]   = -364;
      if ((taus() & 1)==0)
	((int16_t*)x)[i+1] = 364;
      else
	((int16_t*)x)[i+1] = -364;
    }
    
    reset_meas(&ts);
    for (i=0; i<10000; i++) {
      start_meas(&ts);
      (dft[n])((int16_t *)x,(int16_t *)y,1);
      stop_meas(&ts);
    }
    
    printf("\n\n4x%d-point(%f cycles)\n",dftsizes[n],(double)ts.diff/(double)ts.trials);
    char ystr[5],xstr[5],ystr2[5],xstr2[5];
    sprintf(ystr,"y%d.m",dftsizes[n]);
    sprintf(xstr,"x%d.m",dftsizes[n]);
    sprintf(ystr2,"y%d",dftsizes[n]);
    sprintf(xstr2,"x%d",dftsizes[n]);
    LOG_M(ystr,ystr2,y,dftsizes[n]*4,1,1);
    LOG_M(xstr,xstr2,x,dftsizes[n]*4,1,1);
  }


  return(0);
}


#endif
