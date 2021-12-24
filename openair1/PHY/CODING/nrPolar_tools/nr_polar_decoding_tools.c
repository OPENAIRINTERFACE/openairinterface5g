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

/*!\file PHY/CODING/nrPolar_tools/nr_polar_decoding_tools.c
 * \brief
 * \author Turker Yilmaz
 * \date 2018
 * \version 0.1
 * \company EURECOM
 * \email turker.yilmaz@eurecom.fr
 * \note
 * \warning
*/

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include "PHY/sse_intrin.h"
#include "PHY/impl_defs_top.h"

//#define DEBUG_NEW_IMPL 1

void updateLLR(double ***llr,
			   uint8_t **llrU,
			   uint8_t ***bit,
			   uint8_t **bitU,
			   uint8_t listSize,
			   uint16_t row,
			   uint16_t col,
			   uint16_t xlen,
			   uint8_t ylen)
{
	uint16_t offset = (xlen/(pow(2,(ylen-col-1))));
	for (uint8_t i=0; i<listSize; i++) {
		if (( (row) % (2*offset) ) >= offset ) {
			if(bitU[row-offset][col]==0) updateBit(bit, bitU, listSize, (row-offset), col, xlen, ylen);
			if(llrU[row-offset][col+1]==0) updateLLR(llr, llrU, bit, bitU, listSize, (row-offset), (col+1), xlen, ylen);
			if(llrU[row][col+1]==0) updateLLR(llr, llrU, bit, bitU, listSize, row, (col+1), xlen, ylen);
			llr[row][col][i] = (pow((-1),bit[row-offset][col][i])*llr[row-offset][col+1][i]) + llr[row][col+1][i];
		} else {
			if(llrU[row][col+1]==0) updateLLR(llr, llrU, bit, bitU, listSize, row, (col+1), xlen, ylen);
			if(llrU[row+offset][col+1]==0) updateLLR(llr, llrU, bit, bitU, listSize, (row+offset), (col+1), xlen, ylen);
			computeLLR(llr, row, col, i, offset);
		}
	}
	llrU[row][col]=1;

	//	printf("LLR (a %f, b %f): llr[%d][%d] %f\n",32*a,32*b,col,row,32*llr[col][row]);
}

void updateBit(uint8_t ***bit,
			   uint8_t **bitU,
			   uint8_t listSize,
			   uint16_t row,
			   uint16_t col,
			   uint16_t xlen,
			   uint8_t ylen)
{
	uint16_t offset = ( xlen/(pow(2,(ylen-col))) );

	for (uint8_t i=0; i<listSize; i++) {
		if (( (row) % (2*offset) ) >= offset ) {
			if (bitU[row][col-1]==0) updateBit(bit, bitU, listSize, row, (col-1), xlen, ylen);
			bit[row][col][i] = bit[row][col-1][i];
		} else {
			if (bitU[row][col-1]==0) updateBit(bit, bitU, listSize, row, (col-1), xlen, ylen);
			if (bitU[row+offset][col-1]==0) updateBit(bit, bitU, listSize, (row+offset), (col-1), xlen, ylen);
			bit[row][col][i] = ( (bit[row][col-1][i]+bit[row+offset][col-1][i]) % 2);
		}
	}

	bitU[row][col]=1;
}

void updatePathMetric(double *pathMetric,
		              double ***llr,
					  uint8_t listSize,
					  uint8_t bitValue,
					  uint16_t row)
{
	int8_t multiplier = (2*bitValue) - 1;
	for (uint8_t i=0; i<listSize; i++)
		pathMetric[i] += log ( 1 + exp(multiplier*llr[row][0][i]) ) ; //eq. (11b)

}

void updatePathMetric2(double *pathMetric,
					   double ***llr,
					   uint8_t listSize,
					   uint16_t row)
{
	double *tempPM = malloc(sizeof(double) * listSize);
	memcpy(tempPM, pathMetric, (sizeof(double) * listSize));

	uint8_t bitValue = 0;
	int8_t multiplier = (2 * bitValue) - 1;
	for (uint8_t i = 0; i < listSize; i++)
		pathMetric[i] += log(1 + exp(multiplier * llr[row][0][i])); //eq. (11b)

	bitValue = 1;
	multiplier = (2 * bitValue) - 1;
	for (uint8_t i = listSize; i < 2*listSize; i++)
		pathMetric[i] = tempPM[(i-listSize)] + log(1 + exp(multiplier * llr[row][0][(i-listSize)])); //eq. (11b)

	free(tempPM);
}

void computeLLR(double ***llr,
				uint16_t row,
				uint16_t col,
				uint8_t i,
				uint16_t offset)
{
	double a = llr[row][col + 1][i];
	double b = llr[row + offset][col + 1][i];
	llr[row][col][i] = log((exp(a + b) + 1) / (exp(a) + exp(b))); //eq. (8a)
}

void updateCrcChecksum(uint8_t **crcChecksum,
					   uint8_t **crcGen,
					   uint8_t listSize,
					   uint32_t i2,
					   uint8_t len)
{
	for (uint8_t i = 0; i < listSize; i++) {
		for (uint8_t j = 0; j < len; j++) {
			crcChecksum[j][i] = ( (crcChecksum[j][i] + crcGen[i2][j]) % 2 );
		}
	}
}

void updateCrcChecksum2(uint8_t **crcChecksum,
						uint8_t **crcGen,
						uint8_t listSize,
						uint32_t i2,
						uint8_t len)
{
	for (uint8_t i = 0; i < listSize; i++) {
		for (uint8_t j = 0; j < len; j++) {
			crcChecksum[j][i+listSize] = ( (crcChecksum[j][i] + crcGen[i2][j]) % 2 );
		}
	}
}



decoder_node_t *new_decoder_node(int first_leaf_index, int level) {

  decoder_node_t *node=(decoder_node_t *)malloc(sizeof(decoder_node_t));

  node->first_leaf_index=first_leaf_index;
  node->level=level;
  node->Nv = 1<<level;
  node->leaf = 0;
  node->left=(decoder_node_t *)NULL;
  node->right=(decoder_node_t *)NULL;
  node->all_frozen=0;
  node->alpha  = (int16_t*)malloc16(node->Nv*sizeof(int16_t));
  node->beta   = (int16_t*)malloc16(node->Nv*sizeof(int16_t));
  memset((void*)node->beta,-1,node->Nv*sizeof(int16_t));
  
  return(node);
}

decoder_node_t *add_nodes(int level, int first_leaf_index, t_nrPolar_params *polarParams) {

  int all_frozen_below = 1;
  int Nv = 1<<level;
  decoder_node_t *new_node = new_decoder_node(first_leaf_index, level);
#ifdef DEBUG_NEW_IMPL
  printf("New node %d order %d, level %d\n",polarParams->tree.num_nodes,Nv,level);
#endif
  polarParams->tree.num_nodes++;
  if (level==0) {
#ifdef DEBUG_NEW_IMPL
    printf("leaf %d (%s)\n", first_leaf_index, polarParams->information_bit_pattern[first_leaf_index]==1 ? "information or crc" : "frozen");
#endif
    new_node->leaf=1;
    new_node->all_frozen = polarParams->information_bit_pattern[first_leaf_index]==0 ? 1 : 0;
    return new_node; // this is a leaf node
  }

  for (int i=0;i<Nv;i++) {
    if (polarParams->information_bit_pattern[i+first_leaf_index]>0) {
    	  all_frozen_below=0;
        break;
    }
  }

  if (all_frozen_below==0)
	  new_node->left=add_nodes(level-1, first_leaf_index, polarParams);
  else {
#ifdef DEBUG_NEW_IMPL
    printf("aggregating frozen bits %d ... %d at level %d (%s)\n",first_leaf_index,first_leaf_index+Nv-1,level,((first_leaf_index/Nv)&1)==0?"left":"right");
#endif
    new_node->leaf=1;
    new_node->all_frozen=1;
  }
  if (all_frozen_below==0)
	  new_node->right=add_nodes(level-1,first_leaf_index+(Nv/2),polarParams);

#ifdef DEBUG_NEW_IMPL
  printf("new_node (%d): first_leaf_index %d, left %p, right %p\n",Nv,first_leaf_index,new_node->left,new_node->right);
#endif

  return(new_node);
}

void build_decoder_tree(t_nrPolar_params *polarParams)
{
  polarParams->tree.num_nodes=0;
  polarParams->tree.root = add_nodes(polarParams->n,0,polarParams);
#ifdef DEBUG_NEW_IMPL
  printf("root : left %p, right %p\n",polarParams->tree.root->left,polarParams->tree.root->right);
#endif
}

#if defined(__arm__) || defined(__aarch64__)
// translate 1-1 SIMD functions from SSE to NEON
#define __m128i int16x8_t
#define __m64 int8x8_t
#define _mm_abs_epi16(a) vabsq_s16(a)
#define _mm_min_epi16(a,b) vminq_s16(a,b)
#define _mm_subs_epi16(a,b) vsubq_s16(a,b)
#define _mm_abs_pi16(a) vabs_s16(a)
#define _mm_min_pi16(a,b) vmin_s16(a,b)
#define _mm_subs_pi16(a,b) vsub_s16(a,b)
#endif

void applyFtoleft(const t_nrPolar_params *pp, decoder_node_t *node) {
  int16_t *alpha_v=node->alpha;
  int16_t *alpha_l=node->left->alpha;
  int16_t *betal = node->left->beta;
  int16_t a,b,absa,absb,maska,maskb,minabs;

#ifdef DEBUG_NEW_IMPL
  printf("applyFtoleft %d, Nv %d (level %d,node->left (leaf %d, AF %d))\n",node->first_leaf_index,node->Nv,node->level,node->left->leaf,node->left->all_frozen);


  for (int i=0;i<node->Nv;i++) printf("i%d (frozen %d): alpha_v[i] = %d\n",i,1-pp->information_bit_pattern[node->first_leaf_index+i],alpha_v[i]);
#endif

 

  if (node->left->all_frozen == 0) {
#if defined(__AVX2__)
    int avx2mod = (node->Nv/2)&15;
    if (avx2mod == 0) {
      __m256i a256,b256,absa256,absb256,minabs256;
      int avx2len = node->Nv/2/16;

      //      printf("avx2len %d\n",avx2len);
      for (int i=0;i<avx2len;i++) {
	a256       =((__m256i*)alpha_v)[i];
	b256       =((__m256i*)alpha_v)[i+avx2len];
	absa256    =_mm256_abs_epi16(a256);
	absb256    =_mm256_abs_epi16(b256);
	minabs256  =_mm256_min_epi16(absa256,absb256);
	((__m256i*)alpha_l)[i] =_mm256_sign_epi16(minabs256,_mm256_sign_epi16(a256,b256));
      }
    }
    else if (avx2mod == 8) {
      __m128i a128,b128,absa128,absb128,minabs128;
      a128       =*((__m128i*)alpha_v);
      b128       =((__m128i*)alpha_v)[1];
      absa128    =_mm_abs_epi16(a128);
      absb128    =_mm_abs_epi16(b128);
      minabs128  =_mm_min_epi16(absa128,absb128);
      *((__m128i*)alpha_l) =_mm_sign_epi16(minabs128,_mm_sign_epi16(a128,b128));
    }
    else if (avx2mod == 4) {
      __m64 a64,b64,absa64,absb64,minabs64;
      a64       =*((__m64*)alpha_v);
      b64       =((__m64*)alpha_v)[1];
      absa64    =_mm_abs_pi16(a64);
      absb64    =_mm_abs_pi16(b64);
      minabs64  =_mm_min_pi16(absa64,absb64);
      *((__m64*)alpha_l) =_mm_sign_pi16(minabs64,_mm_sign_pi16(a64,b64));
    }
    else
#else
    int sse4mod = (node->Nv/2)&7;
    int sse4len = node->Nv/2/8;
#if defined(__arm__) || defined(__aarch64__)
    int16x8_t signatimesb,comp1,comp2,negminabs128;
    int16x8_t zero=vdupq_n_s16(0);
#endif

    if (sse4mod == 0) {
      for (int i=0;i<sse4len;i++) {
	__m128i a128,b128,absa128,absb128,minabs128;
	int sse4len = node->Nv/2/8;
	
	a128       =*((__m128i*)alpha_v);
	b128       =((__m128i*)alpha_v)[1];
	absa128    =_mm_abs_epi16(a128);
	absb128    =_mm_abs_epi16(b128);
	minabs128  =_mm_min_epi16(absa128,absb128);
#if defined(__arm__) || defined(__aarch64__)
	// unfortunately no direct equivalent to _mm_sign_epi16
	signatimesb=vxorrq_s16(a128,b128);
	comp1=vcltq_s16(signatimesb,zero);
	comp2=vcgeq_s16(signatimesb,zero);
	negminabs128=vnegq_s16(minabs128);
	*((__m128i*)alpha_l) =vorrq_s16(vandq_s16(minabs128,comp0),vandq_s16(negminabs128,comp1));
#else
	*((__m128i*)alpha_l) =_mm_sign_epi16(minabs128,_mm_sign_epi16(a128,b128));
#endif
      }
    }
    else if (sse4mod == 4) {
      __m64 a64,b64,absa64,absb64,minabs64;
      a64       =*((__m64*)alpha_v);
      b64       =((__m64*)alpha_v)[1];
      absa64    =_mm_abs_pi16(a64);
      absb64    =_mm_abs_pi16(b64);
      minabs64  =_mm_min_pi16(absa64,absb64);
#if defined(__arm__) || defined(__aarch64__)
	AssertFatal(1==0,"Need to do this still for ARM\n");
#else
      *((__m64*)alpha_l) =_mm_sign_pi16(minabs64,_mm_sign_epi16(a64,b64));
#endif
    }

    else
#endif
    { // equivalent scalar code to above, activated only on non x86/ARM architectures
      for (int i=0;i<node->Nv/2;i++) {
    	  a=alpha_v[i];
    	  b=alpha_v[i+(node->Nv/2)];
    	  maska=a>>15;
    	  maskb=b>>15;
    	  absa=(a+maska)^maska;
    	  absb=(b+maskb)^maskb;
    	  minabs = absa<absb ? absa : absb;
    	  alpha_l[i] = (maska^maskb)==0 ? minabs : -minabs;
    	  //	printf("alphal[%d] %d (%d,%d)\n",i,alpha_l[i],a,b);
    	  }
    }
    if (node->Nv == 2) { // apply hard decision on left node
      betal[0] = (alpha_l[0]>0) ? -1 : 1;
#ifdef DEBUG_NEW_IMPL
      printf("betal[0] %d (%p)\n",betal[0],&betal[0]);
#endif
      pp->nr_polar_U[node->first_leaf_index] = (1+betal[0])>>1; 
#ifdef DEBUG_NEW_IMPL
      printf("Setting bit %d to %d (LLR %d)\n",node->first_leaf_index,(betal[0]+1)>>1,alpha_l[0]);
#endif
    }
  }
}

void applyGtoright(const t_nrPolar_params *pp,decoder_node_t *node) {

  int16_t *alpha_v=node->alpha;
  int16_t *alpha_r=node->right->alpha;
  int16_t *betal = node->left->beta;
  int16_t *betar = node->right->beta;

#ifdef DEBUG_NEW_IMPL
  printf("applyGtoright %d, Nv %d (level %d), (leaf %d, AF %d)\n",node->first_leaf_index,node->Nv,node->level,node->right->leaf,node->right->all_frozen);
#endif
  
  if (node->right->all_frozen == 0) {  
#if defined(__AVX2__) 
    int avx2mod = (node->Nv/2)&15;
    if (avx2mod == 0) {
      int avx2len = node->Nv/2/16;
      
      for (int i=0;i<avx2len;i++) {
	((__m256i *)alpha_r)[i] = 
	  _mm256_subs_epi16(((__m256i *)alpha_v)[i+avx2len],
			    _mm256_sign_epi16(((__m256i *)alpha_v)[i],
					      ((__m256i *)betal)[i]));	
      }
    }
    else if (avx2mod == 8) {
      ((__m128i *)alpha_r)[0] = _mm_subs_epi16(((__m128i *)alpha_v)[1],_mm_sign_epi16(((__m128i *)alpha_v)[0],((__m128i *)betal)[0]));	
    }
    else if (avx2mod == 4) {
      ((__m64 *)alpha_r)[0] = _mm_subs_pi16(((__m64 *)alpha_v)[1],_mm_sign_pi16(((__m64 *)alpha_v)[0],((__m64 *)betal)[0]));	
    }
    else
#else
    int sse4mod = (node->Nv/2)&7;

    if (sse4mod == 0) {
      int sse4len = node->Nv/2/8;
      
      for (int i=0;i<sse4len;i++) {
#if defined(__arm__) || defined(__aarch64__)
	((int16x8_t *)alpha_r)[0] = vsubq_s16(((int16x8_t *)alpha_v)[1],vmulq_epi16(((int16x8_t *)alpha_v)[0],((int16x8_t *)betal)[0]));
#else
	((__m128i *)alpha_r)[0] = _mm_subs_epi16(((__m128i *)alpha_v)[1],_mm_sign_epi16(((__m128i *)alpha_v)[0],((__m128i *)betal)[0]));
#endif	
      }
    }
    else if (sse4mod == 4) {
#if defined(__arm__) || defined(__aarch64__)
      ((int16x4_t *)alpha_r)[0] = vsub_s16(((int16x4_t *)alpha_v)[1],vmul_epi16(((int16x4_t *)alpha_v)[0],((int16x4_t *)betal)[0]));
#else
      ((__m64 *)alpha_r)[0] = _mm_subs_pi16(((__m64 *)alpha_v)[1],_mm_sign_pi16(((__64 *)alpha_v)[0],((__m64 *)betal)[0]));	
#endif
    }
    else 
#endif
      {// equivalent scalar code to above, activated only on non x86/ARM architectures or Nv=1,2
	for (int i=0;i<node->Nv/2;i++) {
	  alpha_r[i] = alpha_v[i+(node->Nv/2)] - (betal[i]*alpha_v[i]);
	}
      }
    if (node->Nv == 2) { // apply hard decision on right node
      betar[0] = (alpha_r[0]>0) ? -1 : 1;
      pp->nr_polar_U[node->first_leaf_index+1] = (1+betar[0])>>1;
#ifdef DEBUG_NEW_IMPL
      printf("Setting bit %d to %d (LLR %d)\n",node->first_leaf_index+1,(betar[0]+1)>>1,alpha_r[0]);
#endif
    } 
  }
}

int16_t all1[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

void computeBeta(const t_nrPolar_params *pp,decoder_node_t *node) {

  int16_t *betav = node->beta;
  int16_t *betal = node->left->beta;
  int16_t *betar = node->right->beta;
#ifdef DEBUG_NEW_IMPL
  printf("Computing beta @ level %d first_leaf_index %d (all_frozen %d)\n",node->level,node->first_leaf_index,node->left->all_frozen);
#endif
  if (node->left->all_frozen==0) { // if left node is not aggregation of frozen bits
#if defined(__AVX2__) 
    int avx2mod = (node->Nv/2)&15;
    register __m256i allones=*((__m256i*)all1);
    if (avx2mod == 0) {
      int avx2len = node->Nv/2/16;
      for (int i=0;i<avx2len;i++) {
	((__m256i*)betav)[i] = _mm256_or_si256(_mm256_cmpeq_epi16(((__m256i*)betar)[i],
								  ((__m256i*)betal)[i]),allones);
      }
    }
    else if (avx2mod == 8) {
      ((__m128i*)betav)[0] = _mm_or_si128(_mm_cmpeq_epi16(((__m128i*)betar)[0],
							  ((__m128i*)betal)[0]),*((__m128i*)all1));
    }
    else if (avx2mod == 4) {
      ((__m64*)betav)[0] = _mm_or_si64(_mm_cmpeq_pi16(((__m64*)betar)[0],
						      ((__m64*)betal)[0]),*((__m64*)all1));
    }
    else
#else
    int avx2mod = (node->Nv/2)&15;

    if (ssr4mod == 0) {
      int ssr4len = node->Nv/2/8;
      register __m128i allones=*((__m128i*)all1);
      for (int i=0;i<sse4len;i++) {
      ((__m256i*)betav)[i] = _mm_or_si128(_mm_cmpeq_epi16(((__m128i*)betar)[i], ((__m128i*)betal)[i]),allones);
      }
    }
    else if (sse4mod == 4) {
      ((__m64*)betav)[0] = _mm_or_si64(_mm_cmpeq_pi16(((__m64*)betar)[0], ((__m64*)betal)[0]),*((__m64*)all1));
    }
    else
#endif
      {
	for (int i=0;i<node->Nv/2;i++) {
		betav[i] = (betal[i] != betar[i]) ? 1 : -1;
	}
      }
  }
  else memcpy((void*)&betav[0],betar,(node->Nv/2)*sizeof(int16_t));
  memcpy((void*)&betav[node->Nv/2],betar,(node->Nv/2)*sizeof(int16_t));
}

void generic_polar_decoder(const t_nrPolar_params *pp,decoder_node_t *node) {


  // Apply F to left
  applyFtoleft(pp, node);
  // if left is not a leaf recurse down to the left
  if (node->left->leaf==0)
    generic_polar_decoder(pp, node->left);

  applyGtoright(pp, node);
  if (node->right->leaf==0) generic_polar_decoder(pp, node->right);

  computeBeta(pp, node);

} 

