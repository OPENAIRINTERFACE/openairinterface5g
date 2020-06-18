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

/*!\file PHY/CODING/nr_polar_init.h
 * \brief
 * \author Turker Yilmaz, Raymond Knopp
 * \date 2018
 * \version 0.1
 * \company EURECOM
 * \email turker.yilmaz@eurecom.fr, raymond.knopp@eurecom.fr
 * \note
 * \warning
*/

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include "PHY/NR_TRANSPORT/nr_dci.h"

static int intcmp(const void *p1,const void *p2) {
  return(*(int16_t *)p1 > *(int16_t *)p2);
}

static void nr_polar_init(t_nrPolar_params * *polarParams,
                          int8_t messageType,
                          uint16_t messageLength,
                          uint8_t aggregation_level,
			  int decoder_flag) {
  t_nrPolar_params *currentPtr = *polarParams;
  uint16_t aggregation_prime = (messageType >= 2) ? aggregation_level : nr_polar_aggregation_prime(aggregation_level);

  //Parse the list. If the node is already created, return without initialization.
  while (currentPtr != NULL) {
    //printf("currentPtr->idx %d, (%d,%d)\n",currentPtr->idx,currentPtr->payloadBits,currentPtr->encoderLength);
    //LOG_D(PHY,"Looking for index %d\n",(messageType * messageLength * aggregation_prime));
    if (currentPtr->idx == (messageType * messageLength * aggregation_prime)) return;
    else currentPtr = currentPtr->nextPtr;
  }

  //  printf("currentPtr %p (polarParams %p)\n",currentPtr,polarParams);
  //Else, initialize and add node to the end of the linked list.
  t_nrPolar_params *newPolarInitNode = calloc(sizeof(t_nrPolar_params),1);
 
  if (newPolarInitNode != NULL) {
    //   LOG_D(PHY,"Setting new polarParams index %d, messageType %d, messageLength %d, aggregation_prime %d\n",(messageType * messageLength * aggregation_prime),messageType,messageLength,aggregation_prime);
    newPolarInitNode->idx = (messageType * messageLength * aggregation_prime);
    newPolarInitNode->nextPtr = NULL;
    //printf("newPolarInitNode->idx %d, (%d,%d,%d:%d)\n",newPolarInitNode->idx,messageType,messageLength,aggregation_prime,aggregation_level);

    if (messageType == 0) { //PBCH
      newPolarInitNode->n_max = NR_POLAR_PBCH_N_MAX;
      newPolarInitNode->i_il = NR_POLAR_PBCH_I_IL;
      newPolarInitNode->i_seg = NR_POLAR_PBCH_I_SEG;
      newPolarInitNode->n_pc = NR_POLAR_PBCH_N_PC;
      newPolarInitNode->n_pc_wm = NR_POLAR_PBCH_N_PC_WM;
      newPolarInitNode->i_bil = NR_POLAR_PBCH_I_BIL;
      newPolarInitNode->crcParityBits = NR_POLAR_PBCH_CRC_PARITY_BITS;
      newPolarInitNode->payloadBits = NR_POLAR_PBCH_PAYLOAD_BITS;
      newPolarInitNode->encoderLength = NR_POLAR_PBCH_E;
      newPolarInitNode->crcCorrectionBits = NR_POLAR_PBCH_CRC_ERROR_CORRECTION_BITS;
      newPolarInitNode->crc_generator_matrix = crc24c_generator_matrix(newPolarInitNode->payloadBits);//G_P
      //printf("Initializing polar parameters for PBCH (K %d, E %d)\n",newPolarInitNode->payloadBits,newPolarInitNode->encoderLength);
    } else if (messageType == 1) { //DCI
      newPolarInitNode->n_max = NR_POLAR_DCI_N_MAX;
      newPolarInitNode->i_il = NR_POLAR_DCI_I_IL;
      newPolarInitNode->i_seg = NR_POLAR_DCI_I_SEG;
      newPolarInitNode->n_pc = NR_POLAR_DCI_N_PC;
      newPolarInitNode->n_pc_wm = NR_POLAR_DCI_N_PC_WM;
      newPolarInitNode->i_bil = NR_POLAR_DCI_I_BIL;
      newPolarInitNode->crcParityBits = NR_POLAR_DCI_CRC_PARITY_BITS;
      newPolarInitNode->payloadBits = messageLength;
      newPolarInitNode->encoderLength = aggregation_level*108;
      newPolarInitNode->crcCorrectionBits = NR_POLAR_DCI_CRC_ERROR_CORRECTION_BITS;
      newPolarInitNode->crc_generator_matrix=crc24c_generator_matrix(newPolarInitNode->payloadBits+newPolarInitNode->crcParityBits);//G_P
      //printf("Initializing polar parameters for DCI (K %d, E %d, L %d)\n",newPolarInitNode->payloadBits,newPolarInitNode->encoderLength,aggregation_level);
    } else if (messageType == 2) { //UCI PUCCH2
      AssertFatal(aggregation_level>2,"Aggregation level (%d) for PUCCH 2 encoding is NPRB and should be > 2\n",aggregation_level);
      AssertFatal(messageLength>11,"Message length %d is too short for polar encoding of UCI\n",messageLength);
      newPolarInitNode->n_max = NR_POLAR_PUCCH_N_MAX;
      newPolarInitNode->i_il = NR_POLAR_PUCCH_I_IL;
      newPolarInitNode->encoderLength = aggregation_level * 16;

      newPolarInitNode->i_seg = 0;
      
      if ((messageLength >= 360 && newPolarInitNode->encoderLength >= 1088)||
	  (messageLength >= 1013)) newPolarInitNode->i_seg = 1;

      newPolarInitNode->crcParityBits = 11;
      newPolarInitNode->n_pc = 0;
      newPolarInitNode->n_pc_wm = 0;

      if (messageLength < 20) {
	newPolarInitNode->crcParityBits = 6;
	newPolarInitNode->n_pc = 3;
	if ((newPolarInitNode->encoderLength - messageLength - 6 + 3) < 193) newPolarInitNode->n_pc_wm = 1; 
      }



      newPolarInitNode->i_bil = NR_POLAR_PUCCH_I_BIL;

      newPolarInitNode->payloadBits = messageLength;
      newPolarInitNode->crcCorrectionBits = NR_POLAR_PUCCH_CRC_ERROR_CORRECTION_BITS;
      //newPolarInitNode->crc_generator_matrix=crc24c_generator_matrix(newPolarInitNode->payloadBits+newPolarInitNode->crcParityBits);//G_P
      //LOG_D(PHY,"New polar node, encoderLength %d, aggregation_level %d\n",newPolarInitNode->encoderLength,aggregation_level);
    } else {
      AssertFatal(1 == 0, "[nr_polar_init] Incorrect Message Type(%d)", messageType);
    }

    newPolarInitNode->K = newPolarInitNode->payloadBits + newPolarInitNode->crcParityBits; // Number of bits to encode.
    newPolarInitNode->N = nr_polar_output_length(newPolarInitNode->K,
						 newPolarInitNode->encoderLength,
						 newPolarInitNode->n_max);
    newPolarInitNode->n = log2(newPolarInitNode->N);
    newPolarInitNode->G_N = nr_polar_kronecker_power_matrices(newPolarInitNode->n);
    //polar_encoder vectors:
    newPolarInitNode->nr_polar_crc = malloc(sizeof(uint8_t) * newPolarInitNode->crcParityBits);
    newPolarInitNode->nr_polar_aPrime = malloc(sizeof(uint8_t) * ((ceil((newPolarInitNode->payloadBits)/32.0)*4)+3));
    newPolarInitNode->nr_polar_APrime = malloc(sizeof(uint8_t) * newPolarInitNode->K);
    newPolarInitNode->nr_polar_D = malloc(sizeof(uint8_t) * newPolarInitNode->N);
    newPolarInitNode->nr_polar_E = malloc(sizeof(uint8_t) * newPolarInitNode->encoderLength);
    //Polar Coding vectors
    newPolarInitNode->nr_polar_U = malloc(sizeof(uint8_t) * newPolarInitNode->N); //Decoder: nr_polar_uHat
    newPolarInitNode->nr_polar_CPrime = malloc(sizeof(uint8_t) * newPolarInitNode->K); //Decoder: nr_polar_cHat
    newPolarInitNode->nr_polar_B = malloc(sizeof(uint8_t) * newPolarInitNode->K); //Decoder: nr_polar_bHat
    newPolarInitNode->nr_polar_A = malloc(sizeof(uint8_t) * newPolarInitNode->payloadBits); //Decoder: nr_polar_aHat
    newPolarInitNode->Q_0_Nminus1 = nr_polar_sequence_pattern(newPolarInitNode->n);
    newPolarInitNode->interleaving_pattern = malloc(sizeof(uint16_t) * newPolarInitNode->K);
    nr_polar_interleaving_pattern(newPolarInitNode->K,
                                  newPolarInitNode->i_il,
                                  newPolarInitNode->interleaving_pattern);
    newPolarInitNode->deinterleaving_pattern = malloc(sizeof(uint16_t) * newPolarInitNode->K);

    for (int i=0; i<newPolarInitNode->K; i++)
      newPolarInitNode->deinterleaving_pattern[newPolarInitNode->interleaving_pattern[i]] = i;

    newPolarInitNode->rate_matching_pattern = malloc(sizeof(uint16_t) * newPolarInitNode->encoderLength);
    uint16_t *J = malloc(sizeof(uint16_t) * newPolarInitNode->N);
    nr_polar_rate_matching_pattern(newPolarInitNode->rate_matching_pattern,
                                   J,
                                   nr_polar_subblock_interleaver_pattern,
                                   newPolarInitNode->K,
                                   newPolarInitNode->N,
                                   newPolarInitNode->encoderLength);
    newPolarInitNode->information_bit_pattern = malloc(sizeof(uint8_t) * newPolarInitNode->N);
    newPolarInitNode->Q_I_N = malloc(sizeof(int16_t) * (newPolarInitNode->K + newPolarInitNode->n_pc));
    newPolarInitNode->Q_F_N = malloc( sizeof(int16_t) * (newPolarInitNode->N + 1)); // Last element shows the final array index assigned a value.
    newPolarInitNode->Q_PC_N = malloc( sizeof(int16_t) * (newPolarInitNode->n_pc));

    for (int i = 0; i <= newPolarInitNode->N; i++)
      newPolarInitNode->Q_F_N[i] = -1; // Empty array.

    nr_polar_info_bit_pattern(newPolarInitNode->information_bit_pattern,
                              newPolarInitNode->Q_I_N,
                              newPolarInitNode->Q_F_N,
                              J,
                              newPolarInitNode->Q_0_Nminus1,
                              newPolarInitNode->K,
                              newPolarInitNode->N,
                              newPolarInitNode->encoderLength,
                              newPolarInitNode->n_pc);
    // sort the Q_I_N array in ascending order (first K positions)
    qsort((void *)newPolarInitNode->Q_I_N,newPolarInitNode->K,sizeof(int16_t),intcmp);
    newPolarInitNode->channel_interleaver_pattern = malloc(sizeof(uint16_t) * newPolarInitNode->encoderLength);
    nr_polar_channel_interleaver_pattern(newPolarInitNode->channel_interleaver_pattern,
                                         newPolarInitNode->i_bil,
                                         newPolarInitNode->encoderLength);
    free(J);
    if (decoder_flag == 1) build_decoder_tree(newPolarInitNode);
    build_polar_tables(newPolarInitNode);
    init_polar_deinterleaver_table(newPolarInitNode);
    //printf("decoder tree nodes %d\n",newPolarInitNode->tree.num_nodes);
  } else {
    AssertFatal(1 == 0, "[nr_polar_init] New t_nrPolar_params * could not be created");
  }

  //Fixme: the list is not thread safe
  //The defect is not critical: we always append (never delete items) and adding two times the same is fine
  newPolarInitNode->nextPtr=*polarParams;
  *polarParams=newPolarInitNode;
  return;
}

void nr_polar_print_polarParams(t_nrPolar_params *polarParams) {
  uint8_t i = 0;

  if (polarParams == NULL) {
    printf("polarParams is empty.\n");
  } else {
    while (polarParams != NULL) {
      printf("polarParams[%d] = %d\n", i, polarParams->idx);
      polarParams = polarParams->nextPtr;
      i++;
    }
  }

  return;
}

t_nrPolar_params *nr_polar_params (int8_t messageType,
                                   uint16_t messageLength,
                                   uint8_t aggregation_level,
	 		           int decoding_flag,
				   t_nrPolar_params **polarList_ext) {
  static t_nrPolar_params *polarList = NULL;
  nr_polar_init(polarList_ext != NULL ? polarList_ext : &polarList, 
		messageType,messageLength,aggregation_level,decoding_flag);
  t_nrPolar_params *polarParams=polarList_ext != NULL ? *polarList_ext : polarList;
  const int tag=messageType * messageLength * (messageType>=2 ? aggregation_level : nr_polar_aggregation_prime(aggregation_level));


	
  while (polarParams != NULL) {
    //    LOG_D(PHY,"nr_polar_params : tag %d (from nr_polar_init %d)\n",tag,polarParams->idx);
    if (polarParams->idx == tag)
      return polarParams;

    polarParams = polarParams->nextPtr;
  }

  AssertFatal(false,"Polar Init tables internal failure, no polarParams found\n");
  return NULL;
}

uint16_t nr_polar_aggregation_prime (uint8_t aggregation_level) {
  if (aggregation_level == 0) return 0;
  else if (aggregation_level == 1) return NR_POLAR_AGGREGATION_LEVEL_1_PRIME;
  else if (aggregation_level == 2) return NR_POLAR_AGGREGATION_LEVEL_2_PRIME;
  else if (aggregation_level == 4) return NR_POLAR_AGGREGATION_LEVEL_4_PRIME;
  else if (aggregation_level == 8) return NR_POLAR_AGGREGATION_LEVEL_8_PRIME;
  else return NR_POLAR_AGGREGATION_LEVEL_16_PRIME; //aggregation_level == 16
}
