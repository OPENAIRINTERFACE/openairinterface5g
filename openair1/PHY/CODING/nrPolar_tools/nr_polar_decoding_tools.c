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

#include "PHY/impl_defs_top.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include "PHY/sse_intrin.h"

inline void computeLLR(double llr[1+nmax][Nmax], uint16_t row, uint16_t col, 
		       uint16_t offset, uint8_t approximation) __attribute__((always_inline));
inline void computeLLR(double llr[1+nmax][Nmax], uint16_t row, uint16_t col, 
		       uint16_t offset, uint8_t approximation) {

        double a;
        double b;
	double absA,absB;


	a = llr[col + 1][row];   
	b = llr[col+1][row + offset];
	
	if (approximation) { //eq. (9)
	  absA = fabs(a);
	  absB = fabs(b);
	  llr[col][row] = copysign(1.0, a) * copysign(1.0, b) * fmin(absA, absB);
	} else { //eq. (8a)
	  llr[col][row] = log((exp(a + b) + 1) / (exp(a) + exp(b)));
	}
	//	printf("LLR (a %f, b %f): llr[%d][%d] %f\n",32*a,32*b,col,row,32*llr[col][row]);
}

int16_t llrtab[256][256];

void nr_polar_llrtableinit() {
  int16_t absA,absB;
  int16_t minabs;

  for (int a=-128;a<128;a++) {
    for (int b=-128;b<128;b++) {
	absA=abs(a);
	absB=abs(b);
	minabs = absA<absB ? absA:absB;
	if ((a<0 && b<0) || (a>=0 && b>=0)) llrtab[a+128][b+128] = minabs;
	else                                llrtab[a+128][b+128] = -minabs;
      }
  }
}


void updateLLR(decoder_list_t **dlist,uint8_t **llrU, uint8_t **bitU,
	       uint8_t listSize, uint16_t row, uint16_t col, uint16_t xlen, uint8_t ylen, uint8_t approximation) {

  uint16_t offset = (xlen/(1<<(ylen-col-1)));
  if (( (row) % (2*offset) ) >= offset ) {
    if (bitU[row-offset][col]==0) updateBit(dlist, bitU, listSize, (row-offset), col, xlen, ylen);
    if (llrU[row-offset][col+1]==0) updateLLR(dlist, llrU, bitU, listSize, (row-offset), (col+1), xlen, ylen, approximation);
    if (llrU[row][col+1]==0) updateLLR(dlist, llrU, bitU, listSize, row, (col+1), xlen, ylen, approximation);
    for (uint8_t i=0; i<listSize; i++) {
      dlist[i]->llr[col][row] = (pow((-1),dlist[i]->bit[col][row-offset])*dlist[i]->llr[col+1][row-offset]) + dlist[i]->llr[col+1][row];
    }
  } else {
    if (llrU[row][col+1]==0) updateLLR(dlist, llrU, bitU, listSize, row, (col+1), xlen, ylen, approximation);
    if (llrU[row+offset][col+1]==0) updateLLR(dlist, llrU, bitU, listSize, (row+offset), (col+1), xlen, ylen, approximation);
    for (int i=0;i<listSize;i++) computeLLR(dlist[i]->llr, row, col, offset, approximation);
  }
  
  llrU[row][col]=1;
}

void updateLLR_int8(decoder_list_int8_t **dlist,uint8_t **llrU, uint8_t **bitU,
		    uint8_t listSize, uint16_t row, uint16_t col, uint16_t xlen, uint8_t ylen,
		    int generate_optim_code,FILE *fd) {
  uint16_t offset = (xlen/(1<<(ylen-col-1)));
  if (( (row) % (2*offset) ) >= offset ) {
    if (bitU[row-offset][col]==0) updateBit_int8(dlist, bitU, listSize, (row-offset), col, xlen, ylen,generate_optim_code,fd);
    if (llrU[row-offset][col+1]==0) updateLLR_int8(dlist, llrU, bitU, listSize, (row-offset), (col+1), xlen, ylen,generate_optim_code,fd);
    if (llrU[row][col+1]==0) updateLLR_int8(dlist, llrU, bitU, listSize, row, (col+1), xlen, ylen,generate_optim_code,fd);

      
    if (generate_optim_code==1) fprintf(fd,"updateLLR_int8_A(sorted_dlist,%d,%d,%d,%d);\n",listSize,col,row,offset);

    updateLLR_int8_A(dlist,listSize,col,row,offset);
      
  } else {
    if (llrU[row][col+1]==0) updateLLR_int8(dlist, llrU, bitU, listSize, row, (col+1), xlen, ylen,generate_optim_code,fd);
    if (llrU[row+offset][col+1]==0) updateLLR_int8(dlist, llrU, bitU, listSize, (row+offset), (col+1), xlen, ylen,generate_optim_code,fd);
    if (generate_optim_code==1) fprintf(fd,"computeLLR_int8(sorted_dlist,%d,%d,%d,%d);\n",listSize,row,col,offset);
    computeLLR_int8(dlist,listSize, row, col, offset);
  }
  
  llrU[row][col]=1;
}

void updateBit(decoder_list_t **dlist, uint8_t **bitU, uint8_t listSize, uint16_t row,
	       uint16_t col, uint16_t xlen, uint8_t ylen) {
  uint16_t offset = ( xlen/(pow(2,(ylen-col))) );
  
  for (uint8_t i=0; i<listSize; i++) {
    if (( (row) % (2*offset) ) >= offset ) {
      if (bitU[row][col-1]==0) updateBit(dlist, bitU, listSize, row, (col-1), xlen, ylen);
      dlist[i]->bit[col][row] = dlist[i]->bit[col-1][row];
    } else {
      if (bitU[row][col-1]==0) updateBit(dlist, bitU, listSize, row, (col-1), xlen, ylen);
      if (bitU[row+offset][col-1]==0) updateBit(dlist, bitU, listSize, (row+offset), (col-1), xlen, ylen);
      dlist[i]->bit[col][row] = ( (dlist[i]->bit[col-1][row]+dlist[i]->bit[col-1][row+offset]) % 2);
    }
  }
  
  bitU[row][col]=1;
}



void updateBit_int8(decoder_list_int8_t **dlist, uint8_t **bitU, 
		    uint8_t listSize, uint16_t row,
		    uint16_t col, uint16_t xlen, uint8_t ylen,
		    int generate_optim_code,FILE *fd) {

  uint16_t offset = ( xlen/(pow(2,(ylen-col))) );
  
  if (( (row) % (2*offset) ) >= offset ) {

    if (bitU[row][col-1]==0) updateBit_int8(dlist, bitU, listSize, row, (col-1), xlen, ylen,generate_optim_code,fd);
    //      dlist[i]->bit[col][row] = dlist[i]->bit[col-1][row];

    if (generate_optim_code==1) fprintf(fd,"updateBit_int8_A(sorted_dlist,%d,%d,%d);\n",listSize,col,row);

    updateBit_int8_A(dlist,listSize,col,row);

  } else {
    if (bitU[row][col-1]==0) updateBit_int8(dlist, bitU, listSize, row, (col-1), xlen, ylen,generate_optim_code,fd);
    if (bitU[row+offset][col-1]==0) updateBit_int8(dlist, bitU, listSize, (row+offset), (col-1), xlen, ylen,generate_optim_code,fd);
      //      dlist[i]->bit[col][row] = dlist[i]->bit[col-1][row]^dlist[i]->bit[col-1][row+offset];
      //      printf("updating dlist[%d]->bit[%d][%d] => %d\n",i,col,row,dlist[i]->bit[col][row]);

    if (generate_optim_code==1) fprintf(fd,"updateBit_int8_B(sorted_dlist,%d,%d,%d,%d);\n",listSize,col,row,offset);

    updateBit_int8_B(dlist,listSize,col,row,offset);
  }

  
  bitU[row][col]=1;
}
 
void updatePathMetric(decoder_list_t **dlist,uint8_t listSize, uint8_t bitValue,
		       uint16_t row, uint8_t approximation) {
   
  if (approximation) { //eq. (12)
    for (uint8_t i=0; i<listSize; i++) {
      if ((2*bitValue) != ( 1 - copysign(1.0,dlist[i]->llr[0][row]) )) dlist[i]->pathMetric += fabs(dlist[i]->llr[0][row]);
     }
  } else { //eq. (11b)
    int8_t multiplier = (2*bitValue) - 1;
    for (uint8_t i=0; i<listSize; i++) {
      dlist[i]->pathMetric += log ( 1 + exp(multiplier*dlist[i]->llr[0][row]) ) ;
    }  
  }
  
}
 


void updatePathMetric0_int8(decoder_list_int8_t **dlist,uint8_t listSize, uint16_t row,int generate_optim_code,FILE *fd) {

  int16_t mask,absllr;
  updatePathMetric0_int8_A(dlist,listSize,row,mask,absllr);
  
  if (generate_optim_code == 1) fprintf(fd,"updatePathMetric0_int8_A(sorted_dlist,%d,%d,mask,absllr);\n",listSize,row);



    /*
      mask = dlist[i]->llr[0][row]>>15;
      
      if (mask != 0) {
        int16_t absllr = (dlist[i]->llr[0][row]+mask)^mask; 
        dlist[i]->pathMetric += absllr;
	}*/



}

void updatePathMetric2(decoder_list_t **dlist, uint8_t listSize, uint16_t row, uint8_t appr) {

  int i;

  for (i=0;i<listSize;i++) dlist[i+listSize]->pathMetric = dlist[i]->pathMetric;
  decoder_list_t **dlist2 = &dlist[listSize];

  if (appr) { //eq. (12)
    for (i = 0; i < listSize; i++) {
      // bitValue=0
      if (dlist[i]->llr[0][row]<0) dlist[i]->pathMetric  -= dlist[i]->llr[0][row];
       // bitValue=1
      else                         dlist2[i]->pathMetric += dlist[i]->llr[0][row];
    }
  } else { //eq. (11b)
    for (i = 0; i < listSize; i++) {
      // bitValue=0
       dlist[i]->pathMetric += log(1 + exp(-dlist[i]->llr[0][row]));
      // bitValue=1
       dlist2[i]->pathMetric += log(1 + exp(dlist[i]->llr[0][row]));

    }
  }
}




void updatePathMetric2_int8(decoder_list_int8_t **dlist, uint8_t listSize, uint16_t row,int generate_optim_code,FILE *fd) {




  if (generate_optim_code == 1) fprintf(fd,"updatePathMetric2_int8_A(sorted_dlist,%d,%d);\n",
					listSize,row);
  
  updatePathMetric2_int8_A(dlist,listSize,row);
  //    dlist[i+listSize]->pathMetric = dlist[i]->pathMetric;
  //if (dlist[i]->llr[0][row]<0) dlist[i]->pathMetric  -= dlist[i]->llr[0][row];
  //else                         dlist[i+listSize]->pathMetric += dlist[i]->llr[0][row];
  
}


void updateCrcChecksum(decoder_list_t **dlist, uint8_t **crcGen,
		       uint8_t listSize, uint32_t i2, uint8_t len) {
  for (uint8_t i = 0; i < listSize; i++) {
    for (uint8_t j = 0; j < len; j++) {
      dlist[i]->crcChecksum[j] = ( (dlist[i]->crcChecksum[j] + crcGen[i2][j]) % 2 );
    }
  }
}

void updateCrcChecksum2(decoder_list_t **dlist, uint8_t **crcGen,
			uint8_t listSize, uint32_t i2, uint8_t len) {
  for (uint8_t i = 0; i < listSize; i++) {
    for (uint8_t j = 0; j < len; j++) {
      dlist[i+listSize]->crcChecksum[j] = ( (dlist[i]->crcChecksum[j] + crcGen[i2][j]) % 2 );
    }
  }
}


    
void updateCrcChecksum_int8(decoder_list_int8_t **dlist, uint8_t **crcGen,
			    uint8_t listSize, uint32_t i2, uint8_t len,int generate_optim_code,FILE *fd) {

  if (generate_optim_code == 1) fprintf(fd,"updateCrcChecksum_int8_A(sorted_dlist,%d,crcGen,%d,%d);\n",listSize,i2,len);
  
  updateCrcChecksum_int8_A(dlist,listSize,crcGen,i2,len);
  //    for (uint8_t j = 0; j < len; j++) {
  //      dlist[i]->crcChecksum[j] = ( (dlist[i]->crcChecksum[j] + crcGen[i2][j]) % 2 );
  //    }

}



void updateCrcChecksum2_int8(decoder_list_int8_t **dlist, uint8_t **crcGen,
			     uint8_t listSize, uint32_t i2, uint8_t len,int generate_optim_code,FILE *fd) {

  if (generate_optim_code == 1) fprintf(fd,"updateCrcChecksum2_int8_A(sorted_dlist,%d,polarParams->extended_crc_generator_matrix,%d,%d);\n",listSize,i2,len);
  
  updateCrcChecksum2_int8_A(dlist,listSize,crcGen,i2,len);
  //    for (uint8_t j = 0; j < len; j++) {
    //      dlist[i+listSize]->crcChecksum[j] = ( (dlist[i]->crcChecksum[j] + crcGen[i2][j]) % 2 );
    //    }

}


decoder_node_t *new_decoder_node(int first_leaf_index,int level) {

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

decoder_node_t *add_nodes(int level,int first_leaf_index,t_nrPolar_params *pp) {

  int all_frozen_below=1;
  int Nv = 1<<level;
  decoder_node_t *new_node = new_decoder_node(first_leaf_index,level);
#ifdef DEBUG_NEW_IMPL
  printf("New node %d order %d, level %d\n",pp->tree.num_nodes,Nv,level);
  pp->tree.num_nodes++;
#endif
  if (level==0) {
#ifdef DEBUG_NEW_IMPL
    printf("leaf %d (%s)\n",first_leaf_index,pp->information_bit_pattern[first_leaf_index]==1 ? "information or crc" : "frozen");
#endif
    new_node->leaf=1;
    new_node->all_frozen = pp->information_bit_pattern[first_leaf_index]==0 ? 1 : 0;
    return new_node; // this is a leaf node
  }

  for (int i=0;i<Nv;i++) {
    if (pp->information_bit_pattern[i+first_leaf_index]>0) all_frozen_below=0; 
  }
if (all_frozen_below==0) new_node->left=add_nodes(level-1,first_leaf_index,pp);
 else {
#ifdef DEBUG_NEW_IMPL
   printf("aggregating frozen bits %d ... %d at level %d (%s)\n",first_leaf_index,first_leaf_index+Nv-1,level,((first_leaf_index/Nv)&1)==0?"left":"right");
#endif
    new_node->leaf=1;
    new_node->all_frozen=1;
  }
  if (all_frozen_below==0) new_node->right=add_nodes(level-1,first_leaf_index+(Nv/2),pp);

  return(new_node);
}

void build_decoder_tree(t_nrPolar_params *pp) {

  pp->tree.num_nodes=0;
  pp->tree.root = add_nodes(pp->n,0,pp);
  			       
}

void applyFtoleft(t_nrPolar_params *pp,decoder_node_t *node) {
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
      
      for (int i=0;i<avx2len;i++) {
	a256       =((__m256i*)alpha_v)[i];
	b256       =((__m256i*)alpha_v)[i+avx2len];
	absa256    =_mm256_abs_epi16(a256);
	absb256    =_mm256_abs_epi16(b256);
	minabs256  =_mm256_min_epi16(absa256,absb256);
	((__m256i*)alpha_l)[i] =_mm256_sign_epi16(minabs256,_mm256_xor_si256(a256,b256));
      }
    }
    else if (avx2mod == 8) {
      __m128i a128,b128,absa128,absb128,minabs128;
      a128       =*((__m128i*)alpha_v);
      b128       =((__m128i*)alpha_v)[1];
      absa128    =_mm_abs_epi16(a128);
      absb128    =_mm_abs_epi16(b128);
      minabs128  =_mm_min_epi16(absa128,absb128);
      *((__m128i*)alpha_l) =_mm_sign_epi16(minabs128,_mm_xor_si128(a128,b128));
    }
    else if (avx2mod == 4) {
      __m64 a64,b64,absa64,absb64,minabs64;
      a64       =*((__m64*)alpha_v);
      b64       =((__m64*)alpha_v)[1];
      absa64    =_mm_abs_pi16(a64);
      absb64    =_mm_abs_pi16(b64);
      minabs64  =_mm_min_pi16(absa64,absb64);
      *((__m64*)alpha_l) =_mm_sign_pi16(minabs64,_mm_xor_si64(a64,b64));
    }
    else
#endif
    {
      for (int i=0;i<node->Nv/2;i++) {
	a=alpha_v[i];
	b=alpha_v[i+(node->Nv/2)];
	maska=a>>15;
	maskb=b>>15;
	absa=(a+maska)^maska;
	absb=(b+maskb)^maskb;
	minabs = absa<absb ? absa : absb;
	alpha_l[i] = (maska^maskb)==0 ? minabs : -minabs;
      }
    }
    if (node->Nv == 2) { // apply hard decision on left node
      betal[0] = (alpha_l[0]>0) ? -1 : 1;
#ifdef DEBUG_NEW_IMPL
      printf("betal[0] %d (%p)\n",betal[0],&betal[0]);
#endif
      pp->nr_polar_u[node->first_leaf_index] = (1+betal[0])>>1; 
#ifdef DEBUG_NEW_IMPL
      printf("Setting bit %d to %d (LLR %d)\n",node->first_leaf_index,(betal[0]+1)>>1,alpha_l[0]);
#endif
    }
  }
}

void applyGtoright(t_nrPolar_params *pp,decoder_node_t *node) {

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
    else 
#endif
      {
	for (int i=0;i<node->Nv/2;i++) {
	  alpha_r[i] = alpha_v[i+(node->Nv/2)] - (betal[i]*alpha_v[i]);
	}
      }
    if (node->Nv == 2) { // apply hard decision on right node
      betar[0] = (alpha_r[0]>0) ? -1 : 1;
      pp->nr_polar_u[node->first_leaf_index+1] = (1+betar[0])>>1;
#ifdef DEBUG_NEW_IMPL
      printf("Setting bit %d to %d (LLR %d frozen_mask %d)\n",node->first_leaf_index+1,(betar[0]+1)>>1,alpha_r[0],frozen_mask);
#endif
    } 
  }
}

int16_t minus1[16] = {-1,-1,-1,-1,
		      -1,-1,-1,-1,
		      -1,-1,-1,-1,
		      -1,-1,-1,-1};

void computeBeta(t_nrPolar_params *pp,decoder_node_t *node) {

  int16_t *betav = node->beta;
  int16_t *betal = node->left->beta;
  int16_t *betar = node->right->beta;
#ifdef DEBUG_NEW_IMPL
  printf("Computing beta @ level %d first_leaf_index %d (all_frozen %d)\n",node->level,node->first_leaf_index,node->left->all_frozen);
#endif
  if (node->left->all_frozen==0) { // if left node is not aggregation of frozen bits
#if defined(__AVX2__) 
    int avx2mod = (node->Nv/2)&15;
    if (avx2mod == 0) {
      int avx2len = node->Nv/2/16;
      
      for (int i=0;i<avx2len;i++) {
	((__m256i*)betav)[i] = _mm256_sign_epi16(((__m256i*)betar)[i],
						  ((__m256i*)betal)[i]);
	((__m256i*)betav)[i] = _mm256_sign_epi16(((__m256i*)betav)[i],
						  ((__m256i*)minus1)[0]);
      }
    }
    else if (avx2mod == 8) {
      ((__m128i*)betav)[0] = _mm_sign_epi16(((__m128i*)betar)[0],
					    ((__m128i*)betal)[0]);
      ((__m128i*)betav)[0] = _mm_sign_epi16(((__m128i*)betav)[0],
					    ((__m128i*)minus1)[0]);
    }
    else if (avx2mod == 4) {
      ((__m64*)betav)[0] = _mm_sign_pi16(((__m64*)betar)[0],
					 ((__m64*)betal)[0]);
      ((__m64*)betav)[0] = _mm_sign_pi16(((__m64*)betav)[0],
					 ((__m64*)minus1)[0]);
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

void generic_polar_decoder(t_nrPolar_params *pp,decoder_node_t *node) {


  // Apply F to left
  applyFtoleft(pp,node);
  // if left is not a leaf recurse down to the left
  if (node->left->leaf==0) generic_polar_decoder(pp,node->left);

  applyGtoright(pp,node);
  if (node->right->leaf==0) generic_polar_decoder(pp,node->right);	

  computeBeta(pp,node);

} 
