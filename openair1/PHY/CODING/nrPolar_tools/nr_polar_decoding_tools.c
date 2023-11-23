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

static inline void updateBit(uint8_t listSize,
			     uint16_t row,
			     uint16_t col,
			     uint16_t xlen,
			     uint8_t ylen,
			     int zlen,
			     uint8_t bit[xlen][ylen][zlen],
			     uint8_t bitU[xlen][ylen])
{
  uint16_t offset = (xlen / (pow(2, (ylen - col))));

  for (uint8_t i = 0; i < listSize; i++) {
    if (((row) % (2 * offset)) >= offset) {
      if (bitU[row][col - 1] == 0)
        updateBit(listSize, row, (col - 1), xlen, ylen, zlen, bit, bitU);
      bit[row][col][i] = bit[row][col - 1][i];
    } else {
      if (bitU[row][col - 1] == 0)
        updateBit(listSize, row, (col - 1), xlen, ylen, zlen, bit, bitU);
      if (bitU[row + offset][col - 1] == 0)
        updateBit(listSize, (row + offset), (col - 1), xlen, ylen, zlen, bit, bitU);
      bit[row][col][i] = ((bit[row][col - 1][i] + bit[row + offset][col - 1][i]) % 2);
    }
  }

  bitU[row][col] = 1;
}

static inline void computeLLR(uint16_t row,
			      uint16_t col,
			      uint8_t i,
			      uint16_t offset,
			      int xlen,
			      int ylen,
			      int zlen,
			      double llr[xlen][ylen][zlen])
{
  double a = llr[row][col + 1][i];
  double b = llr[row + offset][col + 1][i];
  llr[row][col][i] = log((exp(a + b) + 1) / (exp(a) + exp(b))); //eq. (8a)
}

void updateLLR(uint8_t listSize,
               uint16_t row,
               uint16_t col,
               uint16_t xlen,
               uint8_t ylen,
               int zlen,
               double llr[xlen][ylen][zlen],
               uint8_t llrU[xlen][ylen],
               uint8_t bit[xlen][ylen][zlen],
               uint8_t bitU[xlen][ylen])
{
  uint16_t offset = (xlen / (pow(2, (ylen - col - 1))));
  for (uint8_t i = 0; i < listSize; i++) {
    if ((row % (2 * offset)) >= offset) {
      if (bitU[row - offset][col] == 0)
        updateBit(listSize, (row - offset), col, xlen, ylen, zlen, bit, bitU);
      if (llrU[row - offset][col + 1] == 0)
        updateLLR(listSize, (row - offset), (col + 1), xlen, ylen, zlen, llr, llrU, bit, bitU);
      if (llrU[row][col + 1] == 0)
        updateLLR(listSize, row, (col + 1), xlen, ylen, zlen, llr, llrU, bit, bitU);
      llr[row][col][i] = (pow((-1), bit[row - offset][col][i]) * llr[row - offset][col + 1][i]) + llr[row][col + 1][i];
    } else {
      if (llrU[row][col + 1] == 0)
        updateLLR(listSize, row, (col + 1), xlen, ylen, zlen, llr, llrU, bit, bitU);
      if (llrU[row + offset][col + 1] == 0)
        updateLLR(listSize, (row + offset), (col + 1), xlen, ylen, zlen, llr, llrU, bit, bitU);
      computeLLR(row, col, i, offset, xlen, ylen, zlen, llr);
    }
  }
  llrU[row][col] = 1;

  //	printf("LLR (a %f, b %f): llr[%d][%d] %f\n",32*a,32*b,col,row,32*llr[col][row]);
}

void updatePathMetric(double *pathMetric,
		      uint8_t listSize,
		      uint8_t bitValue,
		      uint16_t row,
		      int xlen,
		      int ylen,
		      int zlen,
		      double llr[xlen][ylen][zlen]
		      )
{
	int8_t multiplier = (2*bitValue) - 1;
	for (uint8_t i=0; i<listSize; i++)
		pathMetric[i] += log ( 1 + exp(multiplier*llr[row][0][i]) ) ; //eq. (11b)

}

void updatePathMetric2(double *pathMetric,
		       uint8_t listSize,
		       uint16_t row,
		       int xlen,
		       int ylen,
		       int zlen,
		       double llr[xlen][ylen][zlen])
{
	double tempPM[listSize];
	memcpy(tempPM, pathMetric, (sizeof(double) * listSize));

	uint8_t bitValue = 0;
	int8_t multiplier = (2 * bitValue) - 1;
	for (uint8_t i = 0; i < listSize; i++)
		pathMetric[i] += log(1 + exp(multiplier * llr[row][0][i])); //eq. (11b)

	bitValue = 1;
	multiplier = (2 * bitValue) - 1;
	for (uint8_t i = listSize; i < 2*listSize; i++)
		pathMetric[i] = tempPM[(i-listSize)] + log(1 + exp(multiplier * llr[row][0][(i-listSize)])); //eq. (11b)

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

void delete_nodes(decoder_node_t * n) {
  if (n) {
    if(n->left)
      delete_nodes(n->left);
    if(n->right)
      delete_nodes(n->right);
    free(n->alpha);
    free(n->beta);
    free(n);
  }
}

void delete_decoder_tree(t_nrPolar_params *polarParams) {
  if (polarParams->tree.root)
    delete_nodes(polarParams->tree.root);
}

void build_decoder_tree(t_nrPolar_params *polarParams)
{
  polarParams->tree.num_nodes=0;
  polarParams->tree.root = add_nodes(polarParams->n,0,polarParams);
#ifdef DEBUG_NEW_IMPL
  printf("root : left %p, right %p\n",polarParams->tree.root->left,polarParams->tree.root->right);
#endif
}

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
    int avx2mod = (node->Nv/2)&15;
    if (avx2mod == 0) {
      simde__m256i a256,b256,absa256,absb256,minabs256;
      int avx2len = node->Nv/2/16;

      //      printf("avx2len %d\n",avx2len);
      for (int i=0;i<avx2len;i++) {
	a256       =((simde__m256i*)alpha_v)[i];
	b256       =((simde__m256i*)alpha_v)[i+avx2len];
	absa256    =simde_mm256_abs_epi16(a256);
	absb256    =simde_mm256_abs_epi16(b256);
	minabs256  =simde_mm256_min_epi16(absa256,absb256);
	((simde__m256i*)alpha_l)[i] =simde_mm256_sign_epi16(minabs256,simde_mm256_sign_epi16(a256,b256));
      }
    }
    else if (avx2mod == 8) {
      simde__m128i a128,b128,absa128,absb128,minabs128;
      a128       =*((simde__m128i*)alpha_v);
      b128       =((simde__m128i*)alpha_v)[1];
      absa128    =simde_mm_abs_epi16(a128);
      absb128    =simde_mm_abs_epi16(b128);
      minabs128  =simde_mm_min_epi16(absa128,absb128);
      *((simde__m128i*)alpha_l) =simde_mm_sign_epi16(minabs128,simde_mm_sign_epi16(a128,b128));
    }
    else if (avx2mod == 4) {
      simde__m64 a64,b64,absa64,absb64,minabs64;
      a64       =*((simde__m64*)alpha_v);
      b64       =((simde__m64*)alpha_v)[1];
      absa64    =simde_mm_abs_pi16(a64);
      absb64    =simde_mm_abs_pi16(b64);
      minabs64  =simde_mm_min_pi16(absa64,absb64);
      *((simde__m64*)alpha_l) =simde_mm_sign_pi16(minabs64,simde_mm_sign_pi16(a64,b64));
    }
    else
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
    int avx2mod = (node->Nv/2)&15;
    if (avx2mod == 0) {
      int avx2len = node->Nv/2/16;
      
      for (int i=0;i<avx2len;i++) {
	((simde__m256i *)alpha_r)[i] = 
	  simde_mm256_subs_epi16(((simde__m256i *)alpha_v)[i+avx2len],
			    simde_mm256_sign_epi16(((simde__m256i *)alpha_v)[i],
					      ((simde__m256i *)betal)[i]));	
      }
    }
    else if (avx2mod == 8) {
      ((simde__m128i *)alpha_r)[0] = simde_mm_subs_epi16(((simde__m128i *)alpha_v)[1],simde_mm_sign_epi16(((simde__m128i *)alpha_v)[0],((simde__m128i *)betal)[0]));	
    }
    else if (avx2mod == 4) {
      ((simde__m64 *)alpha_r)[0] = simde_mm_subs_pi16(((simde__m64 *)alpha_v)[1],simde_mm_sign_pi16(((simde__m64 *)alpha_v)[0],((simde__m64 *)betal)[0]));	
    }
    else
      {
        int temp_alpha_r;
	for (int i = 0; i < node->Nv / 2; i++) {
	  temp_alpha_r = alpha_v[i + (node->Nv / 2)] - (betal[i] * alpha_v[i]);
          if (temp_alpha_r > SHRT_MAX) {
            alpha_r[i] = SHRT_MAX;
          } else if (temp_alpha_r < -SHRT_MAX) {
            alpha_r[i] = -SHRT_MAX;
          } else {
            alpha_r[i] = temp_alpha_r;
          }
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

static const int16_t all1[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

void computeBeta(const t_nrPolar_params *pp,decoder_node_t *node) {

  int16_t *betav = node->beta;
  int16_t *betal = node->left->beta;
  int16_t *betar = node->right->beta;
#ifdef DEBUG_NEW_IMPL
  printf("Computing beta @ level %d first_leaf_index %d (all_frozen %d)\n",node->level,node->first_leaf_index,node->left->all_frozen);
#endif
  if (node->left->all_frozen==0) { // if left node is not aggregation of frozen bits
    int avx2mod = (node->Nv/2)&15;
    register simde__m256i allones=*((simde__m256i*)all1);
    if (avx2mod == 0) {
      int avx2len = node->Nv/2/16;
      for (int i=0;i<avx2len;i++) {
	((simde__m256i*)betav)[i] = simde_mm256_or_si256(simde_mm256_cmpeq_epi16(((simde__m256i*)betar)[i],
								  ((simde__m256i*)betal)[i]),allones);
      }
    }
    else if (avx2mod == 8) {
      ((simde__m128i*)betav)[0] = simde_mm_or_si128(simde_mm_cmpeq_epi16(((simde__m128i*)betar)[0],
							  ((simde__m128i*)betal)[0]),*((simde__m128i*)all1));
    }
    else if (avx2mod == 4) {
      ((simde__m64*)betav)[0] = simde_mm_or_si64(simde_mm_cmpeq_pi16(((simde__m64*)betar)[0],
						      ((simde__m64*)betal)[0]),*((simde__m64*)all1));
    }
    else
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
