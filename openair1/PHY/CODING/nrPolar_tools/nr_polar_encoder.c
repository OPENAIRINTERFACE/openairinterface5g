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

/*!\file PHY/CODING/nrPolar_tools/nr_polar_encoder.c
 * \brief
 * \author Raymond Knopp, Turker Yilmaz
 * \date 2018
 * \version 0.1
 * \company EURECOM
 * \email raymond.knopp@eurecom.fr, turker.yilmaz@eurecom.fr
 * \note
 * \warning
 */

//#define DEBUG_POLAR_ENCODER
//#define DEBUG_POLAR_ENCODER_DCI
//#define DEBUG_POLAR_MATLAB
//#define POLAR_CODING_DEBUG

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include "assertions.h"
#include <stdint.h>

//input  [a_31 a_30 ... a_0]
//output [f_31 f_30 ... f_0] [f_63 f_62 ... f_32] ...

void polar_encoder(uint32_t *in,
                   uint32_t *out,
                   int8_t messageType,
                   uint16_t messageLength,
                   uint8_t aggregation_level) {
  t_nrPolar_params *polarParams=nr_polar_params(messageType, messageLength, aggregation_level, false);
  if (1) {//polarParams->idx == 0 || polarParams->idx == 1) { //PBCH or PDCCH
    /*
    uint64_t B = (((uint64_t)*in)&((((uint64_t)1)<<32)-1)) | (((uint64_t)crc24c((uint8_t*)in,polarParams->payloadBits)>>8)<<polarParams->payloadBits);
    #ifdef DEBUG_POLAR_ENCODER
    printf("polar_B %llx (crc %x)\n",B,crc24c((uint8_t*)in,polarParams->payloadBits)>>8);
    #endif
    nr_bit2byte_uint32_8_t((uint32_t*)&B, polarParams->K, polarParams->nr_polar_B);*/
    nr_bit2byte_uint32_8(in, polarParams->payloadBits, polarParams->nr_polar_A);
    /*
     * Bytewise operations
     */
    //Calculate CRC.
    nr_matrix_multiplication_uint8_1D_uint8_2D(polarParams->nr_polar_A,
                                               polarParams->crc_generator_matrix,
                                               polarParams->nr_polar_crc,
                                               polarParams->payloadBits,
                                               polarParams->crcParityBits);

    for (uint8_t i = 0; i < polarParams->crcParityBits; i++)
      polarParams->nr_polar_crc[i] = (polarParams->nr_polar_crc[i] % 2);

    //Attach CRC to the Transport Block. (a to b)
    for (uint16_t i = 0; i < polarParams->payloadBits; i++)
      polarParams->nr_polar_B[i] = polarParams->nr_polar_A[i];

    for (uint16_t i = polarParams->payloadBits; i < polarParams->K; i++)
      polarParams->nr_polar_B[i]= polarParams->nr_polar_crc[i-(polarParams->payloadBits)];

#ifdef DEBUG_POLAR_ENCODER
    uint64_t B2=0;

    for (int i = 0; i<polarParams->K; i++) B2 = B2 | ((uint64_t)polarParams->nr_polar_B[i] << i);

    printf("polar_B %lx\n",B2);
    for (int i=0; i< polarParams->payloadBits; i++) printf("a[%d]=%d\n", i, polarParams->nr_polar_A[i]);
    for (int i=0; i< polarParams->K; i++) printf("b[%d]=%d\n", i, polarParams->nr_polar_B[i]);
#endif
    /*    for (int j=0;j<polarParams->crcParityBits;j++) {
      for (int i=0;i<polarParams->payloadBits;i++)
    printf("%1d.%1d+",polarParams->crc_generator_matrix[i][j],polarParams->nr_polar_A[i]);
      printf(" => %d\n",polarParams->nr_polar_crc[j]);
      }*/
  } else { //UCI
  }

  //Interleaving (c to c')
  nr_polar_interleaver(polarParams->nr_polar_B,
                       polarParams->nr_polar_CPrime,
                       polarParams->interleaving_pattern,
                       polarParams->K);
#ifdef DEBUG_POLAR_ENCODER
  uint64_t Cprime=0;

  for (int i = 0; i<polarParams->K; i++) {
    Cprime = Cprime | ((uint64_t)polarParams->nr_polar_CPrime[i] << i);

    if (polarParams->nr_polar_CPrime[i] == 1) printf("pos %d : %lx\n",i,Cprime);
  }

  printf("polar_Cprime %lx\n",Cprime);
#endif
  //Bit insertion (c' to u)
  nr_polar_bit_insertion(polarParams->nr_polar_CPrime,
                         polarParams->nr_polar_U,
                         polarParams->N,
                         polarParams->K,
                         polarParams->Q_I_N,
                         polarParams->Q_PC_N,
                         polarParams->n_pc);
  //Encoding (u to d)
  /*  memset(polarParams->nr_polar_U,0,polarParams->N);
  polarParams->nr_polar_U[247]=1;
  polarParams->nr_polar_U[253]=1;*/
  nr_matrix_multiplication_uint8_1D_uint8_2D(polarParams->nr_polar_U,
		  	  	  	  	  	  	  	  	     polarParams->G_N,
											 polarParams->nr_polar_D,
											 polarParams->N,
											 polarParams->N);

  for (uint16_t i = 0; i < polarParams->N; i++)
    polarParams->nr_polar_D[i] = (polarParams->nr_polar_D[i] % 2);

  uint64_t D[8];
  memset((void *)D,0,8*sizeof(int64_t));
#ifdef DEBUG_POLAR_ENCODER

  for (int i=0; i<polarParams->N; i++)  D[i/64] |= ((uint64_t)polarParams->nr_polar_D[i])<<(i&63);

  printf("D %llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx\n",
         D[0],D[1],D[2],D[3],D[4],D[5],D[6],D[7]);
#endif
  //Rate matching
  //Sub-block interleaving (d to y) and Bit selection (y to e)
  nr_polar_interleaver(polarParams->nr_polar_D,
                       polarParams->nr_polar_E,
                       polarParams->rate_matching_pattern,
                       polarParams->encoderLength);
  /*
   * Return bits.
   */
#ifdef DEBUG_POLAR_ENCODER

  for (int i=0; i< polarParams->encoderLength; i++) printf("f[%d]=%d\n", i, polarParams->nr_polar_E[i]);

#endif
  nr_byte2bit_uint8_32(polarParams->nr_polar_E, polarParams->encoderLength, out);

  polarReturn;

}

void polar_encoder_dci(uint32_t *in,
                       uint32_t *out,
                       uint16_t n_RNTI,
                       int8_t messageType,
                       uint16_t messageLength,
                       uint8_t aggregation_level) {
  t_nrPolar_params *polarParams=nr_polar_params(messageType, messageLength, aggregation_level, false);

#ifdef DEBUG_POLAR_ENCODER_DCI
  printf("[polar_encoder_dci] in: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n", in[0], in[1], in[2], in[3]);
#endif
  /*
   * Bytewise operations
   */
  //(a to a')
  nr_bit2byte_uint32_8(in, polarParams->payloadBits, polarParams->nr_polar_A);

  for (int i=0; i<polarParams->crcParityBits; i++) polarParams->nr_polar_APrime[i]=1;

  for (int i=0; i<polarParams->payloadBits; i++) polarParams->nr_polar_APrime[i+(polarParams->crcParityBits)]=polarParams->nr_polar_A[i];

#ifdef DEBUG_POLAR_ENCODER_DCI
  printf("[polar_encoder_dci] A: ");
  for (int i=0; i<polarParams->payloadBits; i++) printf("%d-", polarParams->nr_polar_A[i]);
  printf("\n");

  printf("[polar_encoder_dci] APrime: ");
  for (int i=0; i<polarParams->K; i++) printf("%d-", polarParams->nr_polar_APrime[i]);
  printf("\n");

  printf("[polar_encoder_dci] GP: ");
  for (int i=0; i<polarParams->crcParityBits; i++) printf("%d-", polarParams->crc_generator_matrix[0][i]);
  printf("\n");
#endif
  //Calculate CRC.
  nr_matrix_multiplication_uint8_1D_uint8_2D(polarParams->nr_polar_APrime,
		  	  	  	  	  	  	  	  	  	 polarParams->crc_generator_matrix,
											 polarParams->nr_polar_crc,
											 polarParams->K,
											 polarParams->crcParityBits);

  for (uint8_t i = 0; i < polarParams->crcParityBits; i++) polarParams->nr_polar_crc[i] = (polarParams->nr_polar_crc[i] % 2);

#ifdef DEBUG_POLAR_ENCODER_DCI
  printf("[polar_encoder_dci] CRC: ");
  for (int i=0; i<polarParams->crcParityBits; i++) printf("%d-", polarParams->nr_polar_crc[i]);
  printf("\n");
#endif

  //Attach CRC to the Transport Block. (a to b)
  for (uint16_t i = 0; i < polarParams->payloadBits; i++)
    polarParams->nr_polar_B[i] = polarParams->nr_polar_A[i];

  for (uint16_t i = polarParams->payloadBits; i < polarParams->K; i++)
    polarParams->nr_polar_B[i]= polarParams->nr_polar_crc[i-(polarParams->payloadBits)];

  //Scrambling (b to c)
  for (int i=0; i<16; i++)
	polarParams->nr_polar_B[polarParams->payloadBits+8+i]=( polarParams->nr_polar_B[polarParams->payloadBits+8+i] + ((n_RNTI>>(15-i))&1) ) % 2;

#ifdef DEBUG_POLAR_ENCODER_DCI
  printf("[polar_encoder_dci] B: ");
  for (int i = 0; i < polarParams->K; i++) printf("%d-", polarParams->nr_polar_B[i]);
  printf("\n");
#endif
  //Interleaving (c to c')
  nr_polar_interleaver(polarParams->nr_polar_B, polarParams->nr_polar_CPrime, polarParams->interleaving_pattern, polarParams->K);
  //Bit insertion (c' to u)
  nr_polar_bit_insertion(polarParams->nr_polar_CPrime,
                         polarParams->nr_polar_U,
                         polarParams->N,
                         polarParams->K,
                         polarParams->Q_I_N,
                         polarParams->Q_PC_N,
                         polarParams->n_pc);
  //Encoding (u to d)
  nr_matrix_multiplication_uint8_1D_uint8_2D(polarParams->nr_polar_U,
		  	  	  	  	  	  	  	  	  	 polarParams->G_N,
											 polarParams->nr_polar_D,
											 polarParams->N,
											 polarParams->N);
  for (uint16_t i = 0; i < polarParams->N; i++) polarParams->nr_polar_D[i] = (polarParams->nr_polar_D[i] % 2);

  //Rate matching
  //Sub-block interleaving (d to y) and Bit selection (y to e)
  nr_polar_interleaver(polarParams->nr_polar_D,
                       polarParams->nr_polar_E,
                       polarParams->rate_matching_pattern,
                       polarParams->encoderLength);
  /*
   * Return bits.
   */
  nr_byte2bit_uint8_32(polarParams->nr_polar_E, polarParams->encoderLength, out);
#ifdef DEBUG_POLAR_ENCODER_DCI
  printf("[polar_encoder_dci] E: ");
  for (int i = 0; i < polarParams->encoderLength; i++) printf("%d-", polarParams->nr_polar_E[i]);

  uint8_t outputInd = ceil(polarParams->encoderLength / 32.0);
  printf("\n[polar_encoder_dci] out: ");
  for (int i = 0; i < outputInd; i++) printf("[%d]->0x%08x\t", i, out[i]);
#endif
  polarReturn;
}

/*
 * Interleaving of coded bits implementation
 * TS 138.212: Section 5.4.1.3 - Interleaving of coded bits
 */
void nr_polar_rm_interleaving_cb(void *in, void *out, uint16_t E)
{
  int T = ceil((sqrt(8 * E + 1) - 1) / 2);
  int v[T][T];
  int k = 0;
  uint64_t *in64 = (uint64_t *)in;
  for (int i = 0; i < T; i++) {
    for (int j = 0; j < T - i; j++) {
      if (k < E) {
        int k1 = k >> 6;
        int k2 = k - (k1 << 6);
        v[i][j] = (in64[k1] >> k2) & 1;
      } else {
        v[i][j] = -1;
      }
      k++;
    }
  }

  k = 0;
  uint64_t *out64 = (uint64_t *)out;
  memset(out64, 0, E >> 3);
  for (int j = 0; j < T; j++) {
    for (int i = 0; i < T - j; i++) {
      if (v[i][j] != -1) {
        int k1 = k >> 6;
        int k2 = k - (k1 << 6);
        out64[k1] |= (uint64_t)v[i][j] << k2;
        k++;
      }
    }
  }
}

static inline void polar_rate_matching(const t_nrPolar_params *polarParams,void *in,void *out) __attribute__((always_inline));

static inline void polar_rate_matching(const t_nrPolar_params *polarParams,void *in,void *out) {

  // handle rate matching with a single 128 bit word using bit shuffling
  // can be done with SIMD intrisics if needed
  if (polarParams->groupsize < 8)  {
    AssertFatal(polarParams->encoderLength<=512,"Need to handle groupsize(%d)<8 and N(%d)>512\n",polarParams->groupsize,polarParams->encoderLength);
    uint128_t *out128=(uint128_t*)out;
    uint128_t *in128=(uint128_t*)in;
    for (int i=0;i<polarParams->encoderLength>>7;i++)
      out128[i]=0;
    uint128_t tmp0;
#ifdef DEBUG_POLAR_ENCODER
    uint128_t tmp1;
#endif
    for (int i=0; i<polarParams->encoderLength; i++) {
#ifdef DEBUG_POLAR_ENCODER
      printf("%d<-%u : %llx.%llx =>",i,polarParams->rate_matching_pattern[i],((uint64_t *)out)[1],((uint64_t *)out)[0]);
#endif
      uint8_t pi=polarParams->rate_matching_pattern[i];
      uint8_t pi7=pi>>7;
      uint8_t pimod128=pi&127;
      uint8_t imod128=i&127;
      uint8_t i7=i>>7;
      
      tmp0 = (in128[pi7]&(((uint128_t)1)<<(pimod128)));
      
      if (tmp0!=0) {
        out128[i7] = out128[i7] | ((uint128_t)1)<<imod128;
#ifdef DEBUG_POLAR_ENCODER
        printf("%llx.%llx<->%llx.%llx => %llx.%llx\n",
               ((uint64_t *)&tmp0)[1],((uint64_t *)&tmp0)[0],
               ((uint64_t *)&tmp1)[1],((uint64_t *)&tmp1)[0],
               ((uint64_t *)out)[1],((uint64_t *)out)[0]);
#endif
      }
    }

  }					     
  // These are based on LUTs for byte and short word groups
  else if (polarParams->groupsize == 8)
    for (int i=0; i<polarParams->encoderLength>>3; i++) ((uint8_t *)out)[i] = ((uint8_t *)in)[polarParams->rm_tab[i]];
  else // groupsize==16
    for (int i=0; i<polarParams->encoderLength>>4; i++) {
      ((uint16_t *)out)[i] = ((uint16_t *)in)[polarParams->rm_tab[i]];
    }

  if (polarParams->i_bil == 1) {
    nr_polar_rm_interleaving_cb(out, out, polarParams->encoderLength);
  }
}

void build_polar_tables(t_nrPolar_params *polarParams) {
  // build table b -> c'
  AssertFatal(polarParams->K > 17, "K = %d < 18, is not possible\n",polarParams->K);
  AssertFatal(polarParams->K < 129, "K = %d > 128, is not supported yet\n",polarParams->K);
  int bit_i,ip;
  const int numbytes = (polarParams->K+7)/8;
  const int residue = polarParams->K&7;

  for (int byte=0; byte<numbytes; byte++) {
    int numbits = byte<(polarParams->K>>3) ? 8 : residue;

    for (int val=0; val<256; val++) {
      polarParams->cprime_tab0[byte][val] = 0;
      polarParams->cprime_tab1[byte][val] = 0;

      for (int i=0; i<numbits; i++) {
        // flip bit endian of B bitstring
        ip=polarParams->deinterleaving_pattern[polarParams->K-1-((8*byte)+i)];
        AssertFatal(ip<128,"ip = %d\n",ip);
        bit_i=(val>>i)&1;

        if (ip<64) polarParams->cprime_tab0[byte][val] |= (((uint64_t)bit_i)<<ip);
        else       polarParams->cprime_tab1[byte][val] |= (((uint64_t)bit_i)<<(ip&63));
      }
    }
  }

  AssertFatal(polarParams->N == 512 || polarParams->N == 256 || polarParams->N == 128 || polarParams->N == 64, "N = %d, not done yet\n", polarParams->N);

  // build G bit vectors for information bit positions and convert the bit as bytes tables in nr_polar_kronecker_power_matrices.c to
  // 64 bit packed vectors. 
  // Truncates id N%64 != 0
  allocCast2D(pp, uint64_t, polarParams->G_N_tab, polarParams->N, polarParams->N / 64, false);
  simde__m256i zeros = simde_mm256_setzero_si256();
  // this code packs the one bit per byte of G_N into a packed bits G_N_tab
  for (int i = 0; i < polarParams->N; i++) {
    for (int j = 0; j < polarParams->N; j += 64) {
      const simde__m256i tmp1 = simde_mm256_cmpgt_epi8(*(simde__m256i *)&polarParams->G_N[i][j], zeros);
      const simde__m256i tmp2 = simde_mm256_cmpgt_epi8(*(simde__m256i *)&polarParams->G_N[i][j + 32], zeros);
      // cast directly to uint64_t from int32_t propagates the sign bit (in gcc)
      const uint32_t part1 = simde_mm256_movemask_epi8(tmp1);
      const uint32_t part2 = simde_mm256_movemask_epi8(tmp2);
      pp[i][j / 64] = ((uint64_t)part2 << 32) | part1;
    }
#ifdef DEBUG_POLAR_ENCODER
    printf("Bit %d Selecting row %d of G : ", i, i);

    for (int j = 0; j < polarParams->N; j += 4)
      printf("%1x",
             polarParams->G_N[i][j] + (polarParams->G_N[i][j + 1] * 2) + (polarParams->G_N[i][j + 2] * 4)
                 + (polarParams->G_N[i][j + 3] * 8));

    printf("\n");
#endif
  }

  // rate matching table
  int iplast=polarParams->rate_matching_pattern[0];
  int ccnt=0;
#ifdef DEBUG_POLAR_ENCODER
  int groupcnt=0;
  int firstingroup_out=0;
  int firstingroup_in=iplast;
#endif
  int mingroupsize = 1024;

  // compute minimum group size of rate-matching pattern
  for (int outpos=1; outpos<polarParams->encoderLength; outpos++) {
    ip=polarParams->rate_matching_pattern[outpos];
#ifdef DEBUG_POLAR_ENCODER
    printf("rm: outpos %d, inpos %d\n",outpos,ip);
#endif
    if ((ip - iplast) == 1) ccnt++;
    else {
#ifdef DEBUG_POLAR_ENCODER
      groupcnt++;
      printf("group %d (size %d): (%d:%d) => (%d:%d)\n",groupcnt,ccnt+1,
             firstingroup_in,firstingroup_in+ccnt,
             firstingroup_out,firstingroup_out+ccnt);
#endif

      if ((ccnt+1)<mingroupsize) mingroupsize=ccnt+1;

      ccnt=0;
#ifdef DEBUG_POLAR_ENCODER
      firstingroup_out=outpos;
      firstingroup_in=ip;
#endif
    }

    iplast=ip;
  }
#ifdef DEBUG_POLAR_ENCODER
  groupcnt++;
  #endif
  if ((ccnt+1)<mingroupsize) mingroupsize=ccnt+1;
#ifdef DEBUG_POLAR_ENCODER
  printf("group %d (size %d): (%d:%d) => (%d:%d)\n",groupcnt,ccnt+1,
             firstingroup_in,firstingroup_in+ccnt,
             firstingroup_out,firstingroup_out+ccnt);
#endif

  polarParams->groupsize = mingroupsize;

  int shift = 0;
  switch (mingroupsize) {
    case 2:
      shift = 1;
      break;
    case 4:
      shift = 2;
      break;
    case 8:
      shift = 3;
      break;
    case 16:
      shift = 4;
      break;
    default:
      AssertFatal(1 == 0, "mingroupsize = %i is not supported\n", mingroupsize);
      break;
  }

  polarParams->rm_tab = (int *)malloc(sizeof(int) * (polarParams->encoderLength >> shift));

  // rerun again to create groups
  int tcnt = 0;
  for (int outpos = 0; outpos < polarParams->encoderLength; outpos += mingroupsize, tcnt++)
    polarParams->rm_tab[tcnt] = polarParams->rate_matching_pattern[outpos] >> shift;
}

void polar_encoder_fast(uint64_t *A,
                        void *out,
                        int32_t crcmask,
                        uint8_t ones_flag,
                        int8_t messageType,
                        uint16_t messageLength,
                        uint8_t aggregation_level) {
                        
  t_nrPolar_params *polarParams=nr_polar_params(messageType, messageLength, aggregation_level, false);

#ifdef POLAR_CODING_DEBUG
  printf("polarParams->payloadBits = %i\n", polarParams->payloadBits);
  printf("polarParams->crcParityBits = %i\n", polarParams->crcParityBits);
  printf("polarParams->K = %i\n", polarParams->K);
  printf("polarParams->N = %i\n", polarParams->N);
  printf("polarParams->encoderLength = %i\n", polarParams->encoderLength);
  printf("polarParams->n_pc = %i\n", polarParams->n_pc);
  printf("polarParams->n_pc_wm = %i\n", polarParams->n_pc_wm);
  printf("polarParams->groupsize = %i\n", polarParams->groupsize);
  printf("polarParams->i_il = %i\n", polarParams->i_il);
  printf("polarParams->i_bil = %i\n", polarParams->i_bil);
  printf("polarParams->n_max = %i\n", polarParams->n_max);
#endif

  //  AssertFatal(polarParams->K > 32, "K = %d < 33, is not supported yet\n",polarParams->K);
  AssertFatal(polarParams->K < 129, "K = %d > 128, is not supported yet\n",polarParams->K);
  AssertFatal(polarParams->payloadBits < 65, "payload bits = %d > 64, is not supported yet\n",polarParams->payloadBits);
  int bitlen = polarParams->payloadBits;
  // append crc
  AssertFatal(bitlen<129,"support for payloads <= 128 bits\n");
  //  AssertFatal(polarParams->crcParityBits == 24,"support for 24-bit crc only for now\n");
  //int bitlen0=bitlen;

#ifdef POLAR_CODING_DEBUG
  printf("\nTX\n");
  printf("a: ");
  for (int n = (bitlen + 63)/64 ; n >=0; n--) {
    if (n % 4 == 0) 
      printf(" "); 
    if (n < bitlen) 
      printf("%lu", (A[n/64] >> (n%64)) & 1);
  }
  printf("\n");
#endif

  uint64_t tcrc=0;
  uint8_t offset = 0;

  // appending 24 ones before a0 for DCI as stated in 38.212 7.3.2
  if (ones_flag) offset = 3;

  // A bit string should be stored as 0, 0, ..., 0, a'_0, a'_1, ..., a'_A-1,
  //???a'_{N-1} a'_{N-2} ... a'_{N-A} 0 .... 0, where N=64,128,192,..., N is smallest multiple of 64 greater than or equal to A

  // First flip A bitstring byte endian for CRC routines (optimized for DLSCH/ULSCH, not PBCH/PDCCH)
  // CRC reads in each byte in bit positions 7 down to 0, for PBCH/PDCCH we need to read in a_{A-1} down to a_{0}, A = length of bit string (e.g. 32 for PBCH)
  if (bitlen<=32) {
    uint8_t A32_flip[4+offset];
    if (ones_flag) {
      A32_flip[0] = 0xff;
      A32_flip[1] = 0xff;
      A32_flip[2] = 0xff;
    }
    uint32_t Aprime= (uint32_t)(((uint32_t)*A)<<(32-bitlen));
    A32_flip[0+offset]=((uint8_t *)&Aprime)[3];
    A32_flip[1+offset]=((uint8_t *)&Aprime)[2];
    A32_flip[2+offset]=((uint8_t *)&Aprime)[1];
    A32_flip[3+offset]=((uint8_t *)&Aprime)[0];
    if      (polarParams->crcParityBits == 24)
      tcrc = (uint64_t)(((crcmask^(crc24c(A32_flip,8*offset+bitlen)>>8)))&0xffffff);
    else if (polarParams->crcParityBits == 11)
      tcrc = (uint64_t)(((crcmask^(crc11(A32_flip,bitlen)>>21)))&0x7ff);
    else if (polarParams->crcParityBits == 6)
      tcrc = (uint64_t)(((crcmask^(crc6(A32_flip,bitlen)>>26)))&0x3f);
  } else if (bitlen<=64) {
    uint8_t A64_flip[8+offset];
    if (ones_flag) {
      A64_flip[0] = 0xff;
      A64_flip[1] = 0xff;
      A64_flip[2] = 0xff;
    }
    uint64_t Aprime= (uint64_t)(((uint64_t)*A)<<(64-bitlen));
    A64_flip[0+offset]=((uint8_t *)&Aprime)[7];
    A64_flip[1+offset]=((uint8_t *)&Aprime)[6];
    A64_flip[2+offset]=((uint8_t *)&Aprime)[5];
    A64_flip[3+offset]=((uint8_t *)&Aprime)[4];
    A64_flip[4+offset]=((uint8_t *)&Aprime)[3];
    A64_flip[5+offset]=((uint8_t *)&Aprime)[2];
    A64_flip[6+offset]=((uint8_t *)&Aprime)[1];
    A64_flip[7+offset]=((uint8_t *)&Aprime)[0];
    if (polarParams->crcParityBits == 24)
      tcrc = (uint64_t)((crcmask^(crc24c(A64_flip,8*offset+bitlen)>>8)))&0xffffff;
    else if (polarParams->crcParityBits == 11)
      tcrc = (uint64_t)((crcmask^(crc11(A64_flip,bitlen)>>21)))&0x7ff;
  }
  else if (bitlen<=128) {
    uint8_t A128_flip[16+offset];
    if (ones_flag) {
      A128_flip[0] = 0xff;
      A128_flip[1] = 0xff;
      A128_flip[2] = 0xff;
    }
    uint128_t Aprime= (uint128_t)(((uint128_t)*A)<<(128-bitlen));
    for (int i=0; i<16 ; i++)
      A128_flip[i+offset]=((uint8_t*)&Aprime)[15-i];
    if (polarParams->crcParityBits == 24)
      tcrc = (uint64_t)((crcmask^(crc24c(A128_flip,8*offset+bitlen)>>8)))&0xffffff;
    else if (polarParams->crcParityBits == 11)
      tcrc = (uint64_t)((crcmask^(crc11(A128_flip,bitlen)>>21)))&0x7ff;
  }

  // this is number of quadwords in the bit string
  int quadwlen = (polarParams->K+63)/64;

  // Create the B bit string as
  // 0, 0, ..., 0, a'_0, a'_1, ..., a'_A-1, p_0, p_1, ..., p_{N_parity-1}

  //??? b_{N'-1} b_{N'-2} ... b_{N'-A} b_{N'-A-1} ... b_{N'-A-Nparity} = a_{N-1} a_{N-2} ... a_{N-A} p_{N_parity-1} ... p_0
  uint64_t B[4]= {0};
  B[0] = (A[0] << polarParams->crcParityBits) | tcrc;
  for (int n = 1; n < quadwlen; n++)
    if ((bitlen+63)/64 > n)
      B[n] = (A[n] << polarParams->crcParityBits) | (A[n-1]>>(64-polarParams->crcParityBits));
    else
      B[n] = (A[n-1]>>(64-polarParams->crcParityBits));

#ifdef POLAR_CODING_DEBUG
  int bitlen_B = bitlen + polarParams->crcParityBits;
  int B_array = (bitlen_B + 63) >> 6;
  int n_start = (B_array << 6) - bitlen_B;
  printf("b: ");
  for (int n = 0; n < bitlen_B; n++) {
    if (n % 4 == 0) {
      printf(" ");
    }
    int n1 = (n + n_start) >> 6;
    int n2 = (n + n_start) - (n1 << 6);
    printf("%lu", (B[B_array - 1 - n1] >> (63 - n2)) & 1);
  }
  printf("\n");
#endif

  // TS 38.212 - Section 5.3.1.1 Interleaving
  // For each byte of B, lookup in corresponding table for 64-bit word corresponding to that byte and its position
  uint64_t Cprime[4]= {0};
  uint8_t *Bbyte = (uint8_t *)B;
  if (polarParams->K < 65) {
    Cprime[0] = polarParams->cprime_tab0[0][Bbyte[0]] |
                polarParams->cprime_tab0[1][Bbyte[1]] |
                polarParams->cprime_tab0[2][Bbyte[2]] |
                polarParams->cprime_tab0[3][Bbyte[3]] |
                polarParams->cprime_tab0[4][Bbyte[4]] |
                polarParams->cprime_tab0[5][Bbyte[5]] |
                polarParams->cprime_tab0[6][Bbyte[6]] |
                polarParams->cprime_tab0[7][Bbyte[7]];
  } else if (polarParams->K < 129) {
    for (int i = 0; i < 1 + (polarParams->K / 8); i++) {
      Cprime[0] |= polarParams->cprime_tab0[i][Bbyte[i]];
      Cprime[1] |= polarParams->cprime_tab1[i][Bbyte[i]];
    }
  }

#ifdef DEBUG_POLAR_MATLAB
  // Cprime = pbchCprime
  for (int i = 0; i < quadwlen; i++) printf("[polar_encoder_fast]C'[%d]= 0x%llx\n", i, (unsigned long long)(Cprime[i]));
#endif

#ifdef POLAR_CODING_DEBUG
  printf("c: ");
  for (int n = 0; n < bitlen_B; n++) {
    if (n % 4 == 0) {
      printf(" ");
    }
    int n1 = n >> 6;
    int n2 = n - (n1 << 6);
    printf("%lu", (Cprime[n1] >> n2) & 1);
  }
  printf("\n");
#endif

  uint64_t u[16] = {0};
  nr_polar_generate_u(u,
                      Cprime,
                      polarParams->information_bit_pattern,
                      polarParams->parity_check_bit_pattern,
                      polarParams->N,
                      polarParams->n_pc);

#ifdef POLAR_CODING_DEBUG
  int N_array = polarParams->N >> 6;
  printf("u: ");
  for (int n = 0; n < polarParams->N; n++) {
    if (n % 4 == 0) {
      printf(" ");
    }
    int n1 = n >> 6;
    int n2 = n - (n1 << 6);
    printf("%lu", (u[N_array - 1 - n1] >> (63 - n2)) & 1);
  }
  printf("\n");
#endif

  uint64_t D[8] = {0};
  nr_polar_uxG(D, u, polarParams->G_N_tab, polarParams->N);

#ifdef POLAR_CODING_DEBUG
  printf("d: ");
  for (int n = 0; n < polarParams->N; n++) {
    if (n % 4 == 0) {
      printf(" ");
    }
    int n1 = n >> 6;
    int n2 = n - (n1 << 6);
    printf("%lu", (D[n1] >> n2) & 1);
  }
  printf("\n");
#endif

  memset((void*)out,0,polarParams->encoderLength>>3);
  polar_rate_matching(polarParams,(void *)D, out);

#ifdef POLAR_CODING_DEBUG
  uint64_t *out64 = (uint64_t *)out;
  printf("rm:");
  for (int n = 0; n < polarParams->encoderLength; n++) {
    if (n % 4 == 0) {
      printf(" ");
    }
    int n1 = n >> 6;
    int n2 = n - (n1 << 6);
    printf("%lu", (out64[n1] >> n2) & 1);
  }
  printf("\n");
#endif

  polarReturn;
}
