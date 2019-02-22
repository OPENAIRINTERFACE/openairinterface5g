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
//#define DEBUG_POLAR_ENCODER_TIMING

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include "assertions.h"

//input  [a_31 a_30 ... a_0]
//output [f_31 f_30 ... f_0] [f_63 f_62 ... f_32] ...

void polar_encoder(uint32_t *in,
                   uint32_t *out,
                   t_nrPolar_params *polarParams) {
  if (polarParams->idx == 0) { //PBCH
    /*
    uint64_t B = (((uint64_t)*in)&((((uint64_t)1)<<32)-1)) | (((uint64_t)crc24c((uint8_t*)in,polarParams->payloadBits)>>8)<<polarParams->payloadBits);
    #ifdef DEBUG_POLAR_ENCODER
    printf("polar_B %llx (crc %x)\n",B,crc24c((uint8_t*)in,polarParams->payloadBits)>>8);
    #endif
    nr_bit2byte_uint32_8_t((uint32_t*)&B, polarParams->K, polarParams->nr_polar_B);*/
    nr_bit2byte_uint32_8_t((uint32_t *)in, polarParams->payloadBits, polarParams->nr_polar_A);
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

    printf("polar_B %llx\n",B2);
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

    if (polarParams->nr_polar_CPrime[i] == 1) printf("pos %d : %llx\n",i,Cprime);
  }

  printf("polar_Cprime %llx\n",Cprime);
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
  nr_byte2bit_uint8_32_t(polarParams->nr_polar_E, polarParams->encoderLength, out);
}

void polar_encoder_dci(uint32_t *in,
                       uint32_t *out,
                       t_nrPolar_params *polarParams,
                       uint16_t n_RNTI) {
#ifdef DEBUG_POLAR_ENCODER_DCI
  printf("[polar_encoder_dci] in: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n", in[0], in[1], in[2], in[3]);
#endif
  /*
   * Bytewise operations
   */
  //(a to a')
  nr_bit2byte_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_A);

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
  for (int i=0; i<16; i++) {
    polarParams->nr_polar_B[polarParams->payloadBits+8+i] =
      ( polarParams->nr_polar_B[polarParams->payloadBits+8+i] + ((n_RNTI>>(15-i))&1) ) % 2;
  }

  /*  //(a to a')
  nr_crc_bit2bit_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_aPrime);
  //Parity bits computation (p)
  polarParams->crcBit = crc24c(polarParams->nr_polar_aPrime, (polarParams->payloadBits+polarParams->crcParityBits));
  #ifdef DEBUG_POLAR_ENCODER_DCI
  printf("[polar_encoder_dci] crc: 0x%08x\n", polarParams->crcBit);
  for (int i=0; i<32; i++)
  {
  printf("%d\n",((polarParams->crcBit)>>i)&1);
  }
  #endif
  //(a to b)
  //
  // Bytewise operations
  //
  uint8_t arrayInd = ceil(polarParams->payloadBits / 8.0);
  for (int i=0; i<arrayInd-1; i++){
  for (int j=0; j<8; j++) {
  polarParams->nr_polar_B[j+(i*8)] = ((polarParams->nr_polar_aPrime[3+i]>>(7-j)) & 1);
  }
  }
  for (int i=0; i<((polarParams->payloadBits)%8); i++) {
  polarParams->nr_polar_B[i+(arrayInd-1)*8] = ((polarParams->nr_polar_aPrime[3+(arrayInd-1)]>>(7-i)) & 1);
  }
  for (int i=0; i<8; i++) {
  polarParams->nr_polar_B[polarParams->payloadBits+i] = ((polarParams->crcBit)>>(31-i))&1;
  }
  //Scrambling (b to c)
  for (int i=0; i<16; i++) {
  polarParams->nr_polar_B[polarParams->payloadBits+8+i] =
  ( (((polarParams->crcBit)>>(23-i))&1) + ((n_RNTI>>(15-i))&1) ) % 2;
  }*/
#ifdef DEBUG_POLAR_ENCODER_DCI
  printf("[polar_encoder_dci] B: ");

  for (int i = 0; i < polarParams->K; i++) printf("%d-", polarParams->nr_polar_B[i]);

  printf("\n");
#endif
  //Interleaving (c to c')
  nr_polar_interleaver(polarParams->nr_polar_B,
                       polarParams->nr_polar_CPrime,
                       polarParams->interleaving_pattern,
                       polarParams->K);
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

  for (uint16_t i = 0; i < polarParams->N; i++)
    polarParams->nr_polar_D[i] = (polarParams->nr_polar_D[i] % 2);

  //Rate matching
  //Sub-block interleaving (d to y) and Bit selection (y to e)
  nr_polar_interleaver(polarParams->nr_polar_D,
                       polarParams->nr_polar_E,
                       polarParams->rate_matching_pattern,
                       polarParams->encoderLength);
  /*
   * Return bits.
   */
  nr_byte2bit_uint8_32_t(polarParams->nr_polar_E, polarParams->encoderLength, out);
#ifdef DEBUG_POLAR_ENCODER_DCI
  printf("[polar_encoder_dci] E: ");

  for (int i = 0; i < polarParams->encoderLength; i++) printf("%d-", polarParams->nr_polar_E[i]);

  uint8_t outputInd = ceil(polarParams->encoderLength / 32.0);
  printf("\n[polar_encoder_dci] out: ");

  for (int i = 0; i < outputInd; i++) {
    printf("[%d]->0x%08x\t", i, out[i]);
  }

#endif
}

static inline void polar_rate_matching(t_nrPolar_params *polarParams,void *in,void *out) __attribute__((always_inline));

static inline void polar_rate_matching(t_nrPolar_params *polarParams,void *in,void *out) {
  if (polarParams->groupsize == 8)
    for (int i=0; i<polarParams->encoderLength>>3; i++) ((uint8_t *)out)[i] = ((uint8_t *)in)[polarParams->rm_tab[i]];
  else
    for (int i=0; i<polarParams->encoderLength>>4; i++) {
      ((uint16_t *)out)[i] = ((uint16_t *)in)[polarParams->rm_tab[i]];
    }
}

void build_polar_tables(t_nrPolar_params *polarParams) {
  // build table b -> c'
  AssertFatal(polarParams->K > 32, "K = %d < 33, is not supported yet\n",polarParams->K);
  AssertFatal(polarParams->K < 129, "K = %d > 64, is not supported yet\n",polarParams->K);
  int bit_i,ip;
  int numbytes = polarParams->K>>3;
  int residue = polarParams->K&7;
  int numbits;

  if (residue>0) numbytes++;

  for (int byte=0; byte<numbytes; byte++) {
    if (byte<(polarParams->K>>3)) numbits=8;
    else numbits=residue;

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

  AssertFatal(polarParams->N==512,"N = %d, not done yet\n",polarParams->N);
  // build G bit vectors for information bit positions and convert the bit as bytes tables in nr_polar_kronecker_power_matrices.c to 64 bit packed vectors.
  // keep only rows of G which correspond to information/crc bits
  polarParams->G_N_tab = (uint64_t **)malloc(polarParams->K * sizeof(int64_t *));
  int k=0;

  for (int i=0; i<polarParams->N; i++) {
    if (polarParams->information_bit_pattern[i] > 0) {
      polarParams->G_N_tab[k] = (uint64_t *)memalign(32,(polarParams->N/64)*sizeof(uint64_t));
      memset((void *)polarParams->G_N_tab[k],0,(polarParams->N/64)*sizeof(uint64_t));

      for (int j=0; j<polarParams->N; j++)
        polarParams->G_N_tab[k][j/64] |= ((uint64_t)polarParams->G_N[i][j])<<(j&63);

#ifdef DEBUG_POLAR_ENCODER
      printf("Bit %d Selecting row %d of G : ",k,i);

      for (int j=0; j<polarParams->N; j+=4) printf("%1x",polarParams->G_N[i][j]+(polarParams->G_N[i][j+1]*2)+(polarParams->G_N[i][j+2]*4)+(polarParams->G_N[i][j+3]*8));

      printf("\n");
#endif
      k++;
    }
  }

  // rate matching table
  int iplast=polarParams->rate_matching_pattern[0];
  int ccnt=0;
  int groupcnt=0;
  int firstingroup_out=0;
  int firstingroup_in=iplast;
  int mingroupsize = 1024;

  // compute minimum group size of rate-matching pattern
  for (int outpos=1; outpos<polarParams->encoderLength; outpos++) {
    ip=polarParams->rate_matching_pattern[outpos];

    if ((ip - iplast) == 1) ccnt++;
    else {
      groupcnt++;
#ifdef DEBUG_POLAR_ENCODER
      printf("group %d (size %d): (%d:%d) => (%d:%d)\n",groupcnt,ccnt+1,
             firstingroup_in,firstingroup_in+ccnt,
             firstingroup_out,firstingroup_out+ccnt);
#endif

      if ((ccnt+1)<mingroupsize) mingroupsize=ccnt+1;

      ccnt=0;
      firstingroup_out=outpos;
      firstingroup_in=ip;
    }

    iplast=ip;
  }

  AssertFatal(mingroupsize==8 || mingroupsize==16,"mingroupsize %d, needs to be handled\n",mingroupsize);
  polarParams->groupsize=mingroupsize;
  int shift=3;

  if (mingroupsize == 16) shift=4;

  polarParams->rm_tab=(int *)malloc(sizeof(int)*polarParams->encoderLength/mingroupsize);
  // rerun again to create groups
  int tcnt=0;

  for (int outpos=0; outpos<polarParams->encoderLength; outpos+=mingroupsize,tcnt++)
    polarParams->rm_tab[tcnt] = polarParams->rate_matching_pattern[outpos]>>shift;
}

void polar_encoder_fast(uint64_t *A,
                        uint32_t *out,
                        int32_t crcmask,
                        t_nrPolar_params *polarParams) {
  AssertFatal(polarParams->K > 32, "K = %d < 33, is not supported yet\n",polarParams->K);
  AssertFatal(polarParams->K < 129, "K = %d > 128, is not supported yet\n",polarParams->K);
  AssertFatal(polarParams->payloadBits < 65, "payload bits = %d > 64, is not supported yet\n",polarParams->payloadBits);
  uint64_t B[4]= {0,0,0,0},Cprime[4]= {0,0,0,0};
  int bitlen = polarParams->payloadBits;
  // append crc
  AssertFatal(bitlen<129,"support for payloads <= 128 bits\n");
  AssertFatal(polarParams->crcParityBits == 24,"support for 24-bit crc only for now\n");
  //int bitlen0=bitlen;
  uint64_t tcrc=0;

  // A bitstring should be stored as a_{N-1} a_{N-2} ... a_{N-A} 0 .... 0, where N=64,128,192,..., N is smallest multiple of 64 greater than or equal to A

  // First flip A bitstring byte endian for CRC routines (optimized for DLSCH/ULSCH, not PBCH/PDCCH)
  // CRC reads in each byte in bit positions 7 downto 0, for PBCH/PDCCH we need to read in a_{A-1} downto a_{0}, A = length of bit string (e.g. 32 for PBCH)
  if (bitlen<=32) {
    uint8_t A32_flip[4];
    uint32_t Aprime= (uint32_t)(((uint32_t)*A)<<(32-bitlen));
    A32_flip[0]=((uint8_t *)&Aprime)[3];
    A32_flip[1]=((uint8_t *)&Aprime)[2];
    A32_flip[2]=((uint8_t *)&Aprime)[1];
    A32_flip[3]=((uint8_t *)&Aprime)[0];
    tcrc = (uint64_t)((crcmask^(crc24c(A32_flip,bitlen)>>8)));
  } else if (bitlen<=64) {
    uint8_t A64_flip[8];
    uint64_t Aprime= (uint32_t)(((uint64_t)*A)<<(64-bitlen));
    A64_flip[0]=((uint8_t *)&Aprime)[7];
    A64_flip[1]=((uint8_t *)&Aprime)[6];
    A64_flip[2]=((uint8_t *)&Aprime)[5];
    A64_flip[3]=((uint8_t *)&Aprime)[4];
    A64_flip[4]=((uint8_t *)&Aprime)[3];
    A64_flip[5]=((uint8_t *)&Aprime)[2];
    A64_flip[6]=((uint8_t *)&Aprime)[1];
    A64_flip[7]=((uint8_t *)&Aprime)[0];
    tcrc = (uint64_t)((crcmask^(crc24c(A64_flip,bitlen)>>8)));
  }

  int n;
  // this is number of quadwords in the bit string
  int quadwlen = (polarParams->K>>6);

  if ((polarParams->K&63) > 0) quadwlen++;

  // Create the B bitstring as
  // b_{N'-1} b_{N'-2} ... b_{N'-A} b_{N'-A-1} ... b_{N'-A-Nparity} = a_{N-1} a_{N-2} ... a_{N-A} p_{N_parity-1} ... p_0

  for (n=0; n<quadwlen; n++) if (n==0) B[n] = (A[n] << polarParams->crcParityBits) | tcrc;
    else      B[n] = (A[n] << polarParams->crcParityBits) | (A[n-1]>>(64-polarParams->crcParityBits));

  uint8_t *Bbyte = (uint8_t *)B;

  // for each byte of B, lookup in corresponding table for 64-bit word corresponding to that byte and its position
  if (polarParams->K<65)
    Cprime[0] = polarParams->cprime_tab0[0][Bbyte[0]] |
                polarParams->cprime_tab0[1][Bbyte[1]] |
                polarParams->cprime_tab0[2][Bbyte[2]] |
                polarParams->cprime_tab0[3][Bbyte[3]] |
                polarParams->cprime_tab0[4][Bbyte[4]] |
                polarParams->cprime_tab0[5][Bbyte[5]] |
                polarParams->cprime_tab0[6][Bbyte[6]] |
                polarParams->cprime_tab0[7][Bbyte[7]];
  else if (polarParams->K < 129) {
    for (int i=0; i<1+(polarParams->K/8); i++) {
      Cprime[0] |= polarParams->cprime_tab0[i][Bbyte[i]];
      Cprime[1] |= polarParams->cprime_tab1[i][Bbyte[i]];
    }
  }

#ifdef DEBUG_POLAR_ENCODER

  if (polarParams->K<65)
    printf("A %llx B %llx Cprime %llx (payload bits %d,crc %x)\n",
           (unsigned long long)(A[0]&(((uint64_t)1<<bitlen)-1)),
           (unsigned long long)(B[0]),
           (unsigned long long)(Cprime[0]),
           polarParams->payloadBits,
           tcrc);
  else if (polarParams->K<129) {
    if (bitlen<64)
      printf("A %llx B %llx|%llx Cprime %llx|%llx (payload bits %d,crc %x)\n",
             (unsigned long long)(A[0]&(((uint64_t)1<<bitlen)-1)),
             (unsigned long long)(B[1]),(unsigned long long)(B[0]),
             (unsigned long long)(Cprime[1]),(unsigned long long)(Cprime[0]),
             polarParams->payloadBits,
             tcrc);
    else
      printf("A %llx|%llx B %llx|%llx Cprime %llx|%llx (payload bits %d,crc %x)\n",
             (unsigned long long)(A[1]&(((uint64_t)1<<(bitlen-64))-1)),(unsigned long long)(A[0]),
             (unsigned long long)(B[1]),(unsigned long long)(B[0]),
             (unsigned long long)(Cprime[1]),(unsigned long long)(Cprime[0]),
             polarParams->payloadBits,
             crc24c((uint8_t *)A,bitlen)>>8);
  }

#endif
  /*  printf("Bbytes : %x.%x.%x.%x.%x.%x.%x.%x\n",Bbyte[0],Bbyte[1],Bbyte[2],Bbyte[3],Bbyte[4],Bbyte[5],Bbyte[6],Bbyte[7]);
  printf("%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx\n",polarParams->cprime_tab[0][Bbyte[0]] ,
      polarParams->cprime_tab[1][Bbyte[1]] ,
      polarParams->cprime_tab[2][Bbyte[2]] ,
      polarParams->cprime_tab[3][Bbyte[3]] ,
      polarParams->cprime_tab[4][Bbyte[4]] ,
      polarParams->cprime_tab[5][Bbyte[5]] ,
      polarParams->cprime_tab[6][Bbyte[6]] ,
      polarParams->cprime_tab[7][Bbyte[7]]);*/
  // now do Gu product (here using 64-bit XORs, we can also do with SIMD after)
  // here we're reading out the bits LSB -> MSB, is this correct w.r.t. 3GPP ?
  uint64_t Cprime_i;
  /*  printf("%llx Cprime_0 (%llx) G %llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx\n",Cprime_i,Cprime &1,
   polarParams->G_N_tab[0][0],
   polarParams->G_N_tab[0][1],
   polarParams->G_N_tab[0][2],
   polarParams->G_N_tab[0][3],
   polarParams->G_N_tab[0][4],
   polarParams->G_N_tab[0][5],
   polarParams->G_N_tab[0][6],
   polarParams->G_N_tab[0][7]);*/
  uint64_t D[8]= {0,0,0,0,0,0,0,0};
  int off=0;
  int len=polarParams->K;

  for (int j=0; j<(1+(polarParams->K>>6)); j++,off+=64,len-=64) {
    for (int i=0; i<((len>63) ? 64 : len); i++) {
      Cprime_i = -((Cprime[j]>>i)&1); // this converts bit 0 as, 0 => 0000x00, 1 => 1111x11
      /*
      #ifdef DEBUG_POLAR_ENCODER
      printf("%llx Cprime_%d (%llx) G %llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx\n",
       Cprime_i,off+i,(Cprime[j]>>i) &1,
       polarParams->G_N_tab[off+i][0],
       polarParams->G_N_tab[off+i][1],
       polarParams->G_N_tab[off+i][2],
       polarParams->G_N_tab[off+i][3],
       polarParams->G_N_tab[off+i][4],
       polarParams->G_N_tab[off+i][5],
       polarParams->G_N_tab[off+i][6],
       polarParams->G_N_tab[off+i][7]);
      #endif
      */
      uint64_t *Gi=polarParams->G_N_tab[off+i];
      D[0] ^= (Cprime_i & Gi[0]);
      D[1] ^= (Cprime_i & Gi[1]);
      D[2] ^= (Cprime_i & Gi[2]);
      D[3] ^= (Cprime_i & Gi[3]);
      D[4] ^= (Cprime_i & Gi[4]);
      D[5] ^= (Cprime_i & Gi[5]);
      D[6] ^= (Cprime_i & Gi[6]);
      D[7] ^= (Cprime_i & Gi[7]);
    }
  }

#ifdef DEBUG_POLAR_ENCODER
  printf("D %llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx\n",
         D[0],
         D[1],
         D[2],
         D[3],
         D[4],
         D[5],
         D[6],
         D[7]);
#endif
  polar_rate_matching(polarParams,(void *)D,(void *)out);
}
