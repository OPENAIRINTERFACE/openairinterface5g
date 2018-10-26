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
 * \author Turker Yilmaz
 * \date 2018
 * \version 0.1
 * \company EURECOM
 * \email turker.yilmaz@eurecom.fr
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
		   t_nrPolar_paramsPtr polarParams)
{
  if (polarParams->idx == 0){//PBCH
    nr_bit2byte_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_A);
    /*
     * Bytewise operations
     */
    //Calculate CRC.
    nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_A,
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
  } else { //UCI

  }

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
  nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_U,
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
#ifdef DEBUG_POLAR_ENCODER
  for (int i=0; i< polarParams->encoderLength;i++) printf("f[%d]=%d\n", i, polarParams->nr_polar_E[i]);
#endif

  nr_byte2bit_uint8_32_t(polarParams->nr_polar_E, polarParams->encoderLength, out);
}

void polar_encoder_dci(uint32_t *in,
		       uint32_t *out,
		       t_nrPolar_paramsPtr polarParams,
		       uint16_t n_RNTI)
{
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
  nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_APrime,
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

  /*	//(a to a')
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
  nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_U,
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

void polar_encoder_timing(uint32_t *in,
			  uint32_t *out,
			  t_nrPolar_paramsPtr polarParams,
			  double cpuFreqGHz,
			  FILE* logFile)
{
  //Initiate timing.
  time_stats_t timeEncoderCRCByte, timeEncoderCRCBit, timeEncoderInterleaver, timeEncoderBitInsertion, timeEncoder1, timeEncoder2, timeEncoderRateMatching, timeEncoderByte2Bit;
  reset_meas(&timeEncoderCRCByte); reset_meas(&timeEncoderCRCBit); reset_meas(&timeEncoderInterleaver); reset_meas(&timeEncoderBitInsertion); reset_meas(&timeEncoder1); reset_meas(&timeEncoder2); reset_meas(&timeEncoderRateMatching); reset_meas(&timeEncoderByte2Bit);
  uint16_t n_RNTI=0x0000;

  start_meas(&timeEncoderCRCByte);
  nr_crc_bit2bit_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_aPrime); //(a to a')
  polarParams->crcBit = crc24c(polarParams->nr_polar_aPrime, (polarParams->payloadBits+polarParams->crcParityBits)); //Parity bits computation (p)
  uint8_t arrayInd = ceil(polarParams->payloadBits / 8.0); //(a to b)
  for (int i=0; i<arrayInd-1; i++)
    for (int j=0; j<8; j++)
      polarParams->nr_polar_B[j+(i*8)] = ((polarParams->nr_polar_aPrime[3+i]>>(7-j)) & 1);
  for (int i=0; i<((polarParams->payloadBits)%8); i++) polarParams->nr_polar_B[i+(arrayInd-1)*8] = ((polarParams->nr_polar_aPrime[3+(arrayInd-1)]>>(7-i)) & 1);
  for (int i=0; i<8; i++) polarParams->nr_polar_B[polarParams->payloadBits+i] = ((polarParams->crcBit)>>(31-i))&1;
  for (int i=0; i<16; i++) polarParams->nr_polar_B[polarParams->payloadBits+8+i] =	( (((polarParams->crcBit)>>(23-i))&1) + ((n_RNTI>>(15-i))&1) ) % 2; //Scrambling (b to c)
  stop_meas(&timeEncoderCRCByte);


  start_meas(&timeEncoderCRCBit);
  nr_bit2byte_uint32_8_t(in, polarParams->payloadBits, polarParams->nr_polar_A);
  nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_A, polarParams->crc_generator_matrix, polarParams->nr_polar_crc, polarParams->payloadBits, polarParams->crcParityBits); //Calculate CRC.
  for (uint8_t i = 0; i < polarParams->crcParityBits; i++) polarParams->nr_polar_crc[i] = (polarParams->nr_polar_crc[i] % 2);
  for (uint16_t i = 0; i < polarParams->payloadBits; i++) polarParams->nr_polar_B[i] = polarParams->nr_polar_A[i]; //Attach CRC to the Transport Block. (a to b)
  for (uint16_t i = polarParams->payloadBits; i < polarParams->K; i++) polarParams->nr_polar_B[i]= polarParams->nr_polar_crc[i-(polarParams->payloadBits)];
  stop_meas(&timeEncoderCRCBit);


  start_meas(&timeEncoderInterleaver); //Interleaving (c to c')
  nr_polar_interleaver(polarParams->nr_polar_B, polarParams->nr_polar_CPrime, polarParams->interleaving_pattern, polarParams->K);
  stop_meas(&timeEncoderInterleaver);


  start_meas(&timeEncoderBitInsertion); //Bit insertion (c' to u)
  nr_polar_bit_insertion(polarParams->nr_polar_CPrime, polarParams->nr_polar_U, polarParams->N, polarParams->K, polarParams->Q_I_N, polarParams->Q_PC_N, polarParams->n_pc);
  stop_meas(&timeEncoderBitInsertion);


  start_meas(&timeEncoder1); //Encoding (u to d)
  nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(polarParams->nr_polar_U, polarParams->G_N, polarParams->nr_polar_D, polarParams->N, polarParams->N);
  stop_meas(&timeEncoder1);


  start_meas(&timeEncoder2);
  for (uint16_t i = 0; i < polarParams->N; i++) polarParams->nr_polar_D[i] = (polarParams->nr_polar_D[i] % 2);
  stop_meas(&timeEncoder2);


  start_meas(&timeEncoderRateMatching);//Rate matching //Sub-block interleaving (d to y) and Bit selection (y to e)
  nr_polar_interleaver(polarParams->nr_polar_D, polarParams->nr_polar_E, polarParams->rate_matching_pattern, polarParams->encoderLength);
  stop_meas(&timeEncoderRateMatching);


  start_meas(&timeEncoderByte2Bit); //Return bits.
  nr_byte2bit_uint8_32_t(polarParams->nr_polar_E, polarParams->encoderLength, out);
  stop_meas(&timeEncoderByte2Bit);

  fprintf(logFile,",%f,%f,%f,%f,%f,%f,%f,%f\n",
	  (timeEncoderCRCByte.diff_now/(cpuFreqGHz*1000.0)),
	  (timeEncoderCRCBit.diff_now/(cpuFreqGHz*1000.0)),
	  (timeEncoderInterleaver.diff_now/(cpuFreqGHz*1000.0)),
	  (timeEncoderBitInsertion.diff_now/(cpuFreqGHz*1000.0)),
	  (timeEncoder1.diff_now/(cpuFreqGHz*1000.0)),
	  (timeEncoder2.diff_now/(cpuFreqGHz*1000.0)),
	  (timeEncoderRateMatching.diff_now/(cpuFreqGHz*1000.0)),
	  (timeEncoderByte2Bit.diff_now/(cpuFreqGHz*1000.0)));
}


void build_polar_tables(t_nrPolar_paramsPtr polarParams) {

  
  // build table b -> c'

  AssertFatal(polarParams->K > 32, "K = %d < 33, is not supported yet\n",polarParams->K);
  AssertFatal(polarParams->K < 65, "K = %d > 64, is not supported yet\n",polarParams->K);
  
  int bit_i,ip;
  int numbytes = polarParams->K>>3;
  int residue = polarParams->K&7;
  int numbits;
  if (residue>0) numbytes++;
  for (int byte=0;byte<numbytes;byte++) {
    if (byte<(polarParams->K>>3)) numbits=8;
    else numbits=residue;
    for (int i=0;i<numbits;i++) {
      ip=polarParams->interleaving_pattern[(8*byte)+i];
      printf("input (%d,%d) %d ip %d\n",byte,i,(8*byte)+i,ip);
      AssertFatal(ip<64,"ip = %d\n",ip);
      for (int val=0;val<256;val++) {
	bit_i=(val>>i)&1;
	polarParams->cprime_tab[byte][val] |= (bit_i<<polarParams->interleaving_pattern[(8*byte)+i]);				
      }
    }
    for (int val=0;val<255;val++)
      printf("cprime_tab[%d][%d] = %llx\n",byte,val,polarParams->cprime_tab[byte][val]);

  }
    
  AssertFatal(polarParams->N==512,"N = %d, not done yet\n",polarParams->N);

  // build G bit vectors for information bit positions and convert the bit as bytes tables in nr_polar_kronecker_power_matrices.c to 64 bit packed vectors.
  // keep only rows of G which correspond to information/crc bits
  polarParams->G_N_tab = (uint64_t**)malloc(polarParams->K * sizeof(int64_t*));
  for (int i=0;i<polarParams->K;i++) {
    polarParams->G_N_tab[i] = (uint64_t*)memalign(32,(polarParams->N/64)*sizeof(uint64_t));
    memset((void*)polarParams->G_N_tab[i],0,(polarParams->N/64)*sizeof(uint64_t));
    for (int j=0;j<polarParams->N;j++) 
      polarParams->G_N_tab[i][j/64] |= polarParams->G_N[polarParams->Q_I_N[i]][j]<<(j&63);
  }
}

void polar_encoder_fast(int64_t *A,
			int64_t *D,
			int bitlen,
			int32_t crcmask,
			t_nrPolar_paramsPtr polarParams) {

  AssertFatal(polarParams->K > 32, "K = %d < 33, is not supported yet\n",polarParams->K);
  AssertFatal(polarParams->K < 65, "K = %d > 64, is not supported yet\n",polarParams->K);

  uint64_t B,Cprime;
  
  // append crc
  B = *A | ((crcmask^crc24c(A,bitlen))<<bitlen);
  uint8_t *Bbyte = (uint8_t*)&B;
  // for each byte of B, lookup in corresponding table for 64-bit word corresponding to that byte and its position
  Cprime = polarParams->cprime_tab[0][Bbyte[0]] | 
           polarParams->cprime_tab[1][Bbyte[1]] | 
           polarParams->cprime_tab[2][Bbyte[2]] | 
           polarParams->cprime_tab[3][Bbyte[3]] | 
           polarParams->cprime_tab[4][Bbyte[4]] | 
           polarParams->cprime_tab[5][Bbyte[5]] | 
           polarParams->cprime_tab[6][Bbyte[6]] | 
           polarParams->cprime_tab[7][Bbyte[7]];

  
  // now do Gu product (here using 64-bit XORs, we can also do with SIMD after)
  // here we're reading out the bits LSB -> MSB, is this correct w.r.t. 3GPP ?

  uint64_t Cprime_i = -(Cprime & 1); // this converts bit 0 as, 0 => 0000x00, 1 => 1111x11
  D[0] = Cprime_i & polarParams->G_N_tab[0][0];
  D[1] = Cprime_i & polarParams->G_N_tab[0][1];
  D[2] = Cprime_i & polarParams->G_N_tab[0][2];
  D[3] = Cprime_i & polarParams->G_N_tab[0][3];
  D[4] = Cprime_i & polarParams->G_N_tab[0][4];
  D[5] = Cprime_i & polarParams->G_N_tab[0][5];
  D[6] = Cprime_i & polarParams->G_N_tab[0][6];
  D[7] = Cprime_i & polarParams->G_N_tab[0][7];
  for (int i=1;i<bitlen;i++) {

    Cprime_i = -((Cprime>>i)&1);
    D[0] ^= (Cprime_i & polarParams->G_N_tab[i][0]);
    D[1] ^= (Cprime_i & polarParams->G_N_tab[i][1]);
    D[2] ^= (Cprime_i & polarParams->G_N_tab[i][2]);
    D[3] ^= (Cprime_i & polarParams->G_N_tab[i][3]);
    D[4] ^= (Cprime_i & polarParams->G_N_tab[i][4]);
    D[5] ^= (Cprime_i & polarParams->G_N_tab[i][5]);
    D[6] ^= (Cprime_i & polarParams->G_N_tab[i][6]);
    D[7] ^= (Cprime_i & polarParams->G_N_tab[i][7]);
  }

  // Rate matching on the 8 64-bit D bit-strings should be performed more or less like
  // The interleaving on the single 64-bit input in the first step. We just need 64 lookup tables I guess, and they will have large entries

}
