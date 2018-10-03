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
 * For more information about the OpenAirInterface (OAI) Software Alliance
 *      contact@openairinterface.org
 */

/*
 * Return values:
 *  0 --> Success
 * -1 --> All list entries have failed the CRC checks
 */

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"
#include "PHY/TOOLS/time_meas.h"

#define SHOWCOMP 1

int8_t polar_decoder(
		double *input,
		uint8_t *output,
		t_nrPolar_params *polarParams,
		uint8_t listSize,
		double *aPrioriPayload,
		uint8_t pathMetricAppr,
		time_stats_t *init,
                time_stats_t *polar_rate_matching,
                time_stats_t *decoding,
                time_stats_t *bit_extraction,
                time_stats_t *deinterleaving,
		time_stats_t *sorting,
		time_stats_t *path_metric,
		time_stats_t *update_LLR)
{

  start_meas(init);
  
  
  uint8_t **bitUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
  uint8_t **llrUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
  decoder_list_t dlist[2*listSize];
  
  for ( int i=0;i<2*listSize;i++) {
    //	  dlist[i].bit         = nr_alloc_uint8_t_2D_array((polarParams->n+1), polarParams->N);
    //	  dlist[i].llr         = nr_alloc_double_2D_array((polarParams->n+1), polarParams->N);
    //dlist[i].crcChecksum = malloc(sizeof(uint8_t)*polarParams->crcParityBits);
    for (int j=0; j< polarParams->n+1; j++) {
      memset((void*)&dlist[i].bit[j][0],0,sizeof(uint8_t)*polarParams->N);
      memset((void*)&dlist[i].llr[j][0],0,sizeof(double)*polarParams->N);
    }
    for (int j=0;j<polarParams->crcParityBits;j++) dlist[i].crcChecksum[j] = 0;
    dlist[i].pathMetric  = 0;
  }
  
  for (int i=0; i<polarParams->N; i++) { 
    memset((void *)&llrUpdated[i][0],0,sizeof(uint8_t)*polarParams->n);
    memset((void *)&bitUpdated[i][0],0,sizeof(uint8_t)*polarParams->n);
    llrUpdated[i][polarParams->n]=1;
    bitUpdated[i][0]=((polarParams->information_bit_pattern[i]+1) % 2);
  }
  
  
  uint8_t **extended_crc_generator_matrix = malloc(polarParams->K * sizeof(uint8_t *)); //G_P3
  uint8_t **tempECGM = malloc(polarParams->K * sizeof(uint8_t *)); //G_P2
  for (int i = 0; i < polarParams->K; i++){
    extended_crc_generator_matrix[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
    tempECGM[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
  }
  
  for (int i=0; i<polarParams->payloadBits; i++) {
    for (int j=0; j<polarParams->crcParityBits; j++) {
      tempECGM[i][j]=polarParams->crc_generator_matrix[i][j];
    }
  }
  for (int i=polarParams->payloadBits; i<polarParams->K; i++) {
    for (int j=0; j<polarParams->crcParityBits; j++) {
      if( (i-polarParams->payloadBits) == j ){
	tempECGM[i][j]=1;
      } else {
	tempECGM[i][j]=0;
      }
    }
  }
  
  for (int i=0; i<polarParams->K; i++) {
    for (int j=0; j<polarParams->crcParityBits; j++) {
      extended_crc_generator_matrix[i][j]=tempECGM[polarParams->interleaving_pattern[i]][j];
    }
  }
  
  stop_meas(init);
  start_meas(polar_rate_matching);
  
  double d_tilde[polarParams->N];// = malloc(sizeof(double) * polarParams->N);
  nr_polar_rate_matching(input, d_tilde, polarParams->rate_matching_pattern, polarParams->K, polarParams->N, polarParams->encoderLength);
  memcpy((void*)&dlist[0].llr[polarParams->n][0],(void*)&d_tilde[0],sizeof(double)*polarParams->N);
  stop_meas(polar_rate_matching);
  
  /*
   * SCL polar decoder.
   */
  start_meas(decoding);
  uint32_t nonFrozenBit=0;
  uint8_t currentListSize=1;
  
  decoder_list_t *sorted_dlist[2*listSize];
  decoder_list_t *temp_dlist[2*listSize];
  int listIndex[2*listSize];
  double pathMetric[2*listSize];
  
  for (uint8_t i = 0; i < 2*listSize; i++) sorted_dlist[i] = &dlist[i];
  
  for (uint16_t currentBit=0; currentBit<polarParams->N; currentBit++){
    //    printf("***************** BIT %d (currentListSize %d, information_bit_pattern %d)\n",
    //	   currentBit,currentListSize,polarParams->information_bit_pattern[currentBit]);
    
    start_meas(update_LLR);
    updateLLR(sorted_dlist, llrUpdated, bitUpdated, currentListSize, currentBit, 0, polarParams->N, (polarParams->n+1), pathMetricAppr);
    stop_meas(update_LLR);
    
    if (polarParams->information_bit_pattern[currentBit]==0) { //Frozen bit.
      updatePathMetric(sorted_dlist,currentListSize, 0, currentBit, pathMetricAppr); //approximation=0 --> 11b, approximation=1 --> 12
    } else { //Information or CRC bit.
      if ( (polarParams->interleaving_pattern[nonFrozenBit] < polarParams->payloadBits) && (aPrioriPayload[polarParams->interleaving_pattern[nonFrozenBit]] == 0) ) {
	//Information bit with known value of "0".
	updatePathMetric(sorted_dlist, currentListSize, 0, currentBit, pathMetricAppr);
	bitUpdated[currentBit][0]=1; //0=False, 1=True
      } else if ( (polarParams->interleaving_pattern[nonFrozenBit] < polarParams->payloadBits) && (aPrioriPayload[polarParams->interleaving_pattern[nonFrozenBit]] == 1) ) {
	//Information bit with known value of "1".
	printf("Information bit with known value of 1\n");
	updatePathMetric(sorted_dlist, currentListSize, 1, currentBit, pathMetricAppr);
	for (uint8_t i=0; i<currentListSize; i++) sorted_dlist[i]->bit[0][currentBit]=1;
	bitUpdated[currentBit][0]=1;
	updateCrcChecksum(sorted_dlist, extended_crc_generator_matrix, currentListSize, nonFrozenBit, polarParams->crcParityBits);
      } else {
	
	start_meas(path_metric);
	updatePathMetric2(sorted_dlist, currentListSize, currentBit, pathMetricAppr);
	stop_meas(path_metric);
	
	start_meas(sorting); 

	if (currentListSize <= listSize/2) {
	  // until listsize is full we need to copy bit and LLR arrays to new entries
	  // below we only copy the ones we need to keep for sure
	  
	  for (int i = 0; i < currentListSize; i++) {
	    for (int k = 0; k < (polarParams->n+1); k++) {
	      
	      memcpy((void*)&sorted_dlist[i+currentListSize]->bit[k][0],(void*)&sorted_dlist[i]->bit[k][0],sizeof(uint8_t)*polarParams->N);
	      memcpy((void*)&sorted_dlist[i+currentListSize]->llr[k][0],(void*)&sorted_dlist[i]->llr[k][0],sizeof(double)*polarParams->N);
	    }
	  }
	}
	
	for (int i = 0; i < currentListSize; i++) {
	  sorted_dlist[i]->bit[0][currentBit]=0;
	  sorted_dlist[i+currentListSize]->bit[0][currentBit]=1;
	}
	bitUpdated[currentBit][0]=1;
	updateCrcChecksum2(sorted_dlist,extended_crc_generator_matrix, currentListSize, nonFrozenBit, polarParams->crcParityBits);
	
	currentListSize*=2;
	
	//Keep only the best "listSize" number of entries.
	if (currentListSize > listSize) {
	  int listIndex2[listSize];
	  
	  for (int i = 0; i < currentListSize; i++) { 
	    listIndex[i]=i;
	    pathMetric[i] = sorted_dlist[i]->pathMetric;
	  }
	  nr_sort_asc_double_1D_array_ind(pathMetric, listIndex, currentListSize);
	  for (int i=0;i<currentListSize;i++) {
	    listIndex2[listIndex[i]] = i;
	  }

	  
	  // copy the llr/bit arrays that are needed
	  for (int i = 0; i < listSize; i++) {
	    if ((listIndex2[i+listSize]<listSize) && (listIndex2[i]<listSize)) { // both '0' and '1' path metrics are to be kept
	      // do memcpy of LLR and Bit arrays
	      
	      for (int k = 0; k < (polarParams->n+1); k++) {
		
		memcpy((void*)&sorted_dlist[i+listSize]->bit[k][0],
		       (void*)&sorted_dlist[i]->bit[k][0],
		       sizeof(uint8_t)*polarParams->N);
		memcpy((void*)&sorted_dlist[i+listSize]->llr[k][0],
		       (void*)&sorted_dlist[i]->llr[k][0],
		       sizeof(double)*polarParams->N);
	      }
	      sorted_dlist[i]->bit[0][currentBit]=0;
	      sorted_dlist[i+listSize]->bit[0][currentBit]=1;
	    }
	    else if (listIndex2[i+listSize]<listSize) { // only '1' path metric is to be kept
	      // just change the current bit from '0' to '1'
	      
	      for (int k = 0; k < (polarParams->n+1); k++) {
		memcpy((void*)&sorted_dlist[i+listSize]->bit[k][0],
		       (void*)&sorted_dlist[i]->bit[k][0],
		       sizeof(uint8_t)*polarParams->N);
		memcpy((void*)&sorted_dlist[i+listSize]->llr[k][0],
		       (void*)&sorted_dlist[i]->llr[k][0],
		       sizeof(double)*polarParams->N); 
	      }
	      sorted_dlist[i+listSize]->bit[0][currentBit]=1;
	      
	      /*
		decoder_list_t *tmp = sorted_dlist[i+listSize];
		sorted_dlist[i+listSize] = sorted_dlist[i];
		sorted_dlist[i+listSize]->pathMetric = tmp->pathMetric;
		sorted_dlist[i+listSize]->bit[0][currentBit]=1;
		memcpy((void*)&sorted_dlist[i+listSize]->crcChecksum[0],
		(void*)&tmp->crcChecksum[0],
		24*sizeof(uint8_t));*/
	      
	    }
	  }
	  
	  currentListSize = listSize;
	  
	  for (int i = 0; i < 2*listSize; i++) {
	    temp_dlist[i] = sorted_dlist[i];
	  }
	  for (int i = 0; i < 2*listSize; i++) {
	    //		  printf("i %d => %d\n",i,listIndex[i]);
	    sorted_dlist[i] = temp_dlist[listIndex[i]];
	  }
	}
	stop_meas(sorting);
	
      }
      nonFrozenBit++;
    }
    
  }
  
  for (uint8_t i = 0; i < fmin(listSize, (pow(2,polarParams->crcCorrectionBits)) ); i++) {
    //	  printf("list index %d :",i);
    //	  for (int j=0;j<polarParams->crcParityBits;j++) printf("%d",sorted_dlist[i]->crcChecksum[j]);
    //	  printf(" => %d (%f)\n",sorted_dlist[i]->crcState,sorted_dlist[i]->pathMetric);
    int crcState = 1;
    for (int j=0;j<polarParams->crcParityBits;j++) if (sorted_dlist[i]->crcChecksum[j]!=0) crcState=0;
    if (crcState == 1) {
      for (int j = 0; j < polarParams->N; j++) polarParams->nr_polar_u[j]=sorted_dlist[i]->bit[0][j];
      
      start_meas(bit_extraction);
      //Extract the information bits (û to ĉ)
      nr_polar_info_bit_extraction(polarParams->nr_polar_u, polarParams->nr_polar_cPrime, polarParams->information_bit_pattern, polarParams->N);
      stop_meas(bit_extraction);
      //Deinterleaving (ĉ to b)
      start_meas(deinterleaving);
      nr_polar_deinterleaver(polarParams->nr_polar_cPrime, polarParams->nr_polar_b, polarParams->interleaving_pattern, polarParams->K);
      stop_meas(deinterleaving);
      //Remove the CRC (â)
      for (int j = 0; j < polarParams->payloadBits; j++) output[j]=polarParams->nr_polar_b[j];
      
      break;
    }
  }
  
  //	free(d_tilde);
  /*
    for (int i=0;i<2*listSize;i++) {
    //	  printf("correct: Freeing dlist[%d].bit %p\n",i,dlist[i].bit);
    nr_free_uint8_t_2D_array(dlist[i].bit, (polarParams->n+1));
    nr_free_double_2D_array(dlist[i].llr, (polarParams->n+1));
    free(dlist[i].crcChecksum);
    }*/
  nr_free_uint8_t_2D_array(extended_crc_generator_matrix, polarParams->K);
  nr_free_uint8_t_2D_array(tempECGM, polarParams->K);
  stop_meas(decoding);
  return(0);
}


#define decoder_int8_A(sorted_dlist,currentListSize,polarParams) {for (int i = 0; i < currentListSize; i++) { \
           for (int k = 0; k < (polarParams->n+1); k++) { \
	    memcpy((void*)&sorted_dlist[i+currentListSize]->bit[k][0],\
		   (void*)&sorted_dlist[i]->bit[k][0],\
		   sizeof(uint8_t)*polarParams->N);\
	    memcpy((void*)&sorted_dlist[i+currentListSize]->llr[k][0],\
		   (void*)&sorted_dlist[i]->llr[k][0],\
		   sizeof(int16_t)*polarParams->N);}}}

#define decoder_int8_B(sorted_dlist,currentListSize) {for (int i = 0; i < currentListSize; i++) {sorted_dlist[i]->bit[0][currentBit]=0;sorted_dlist[i+currentListSize]->bit[0][currentBit]=1;}}

void inline decoder_int8_C(decoder_list_int8_t *sorted_dlist[],
			   t_nrPolar_params *polarParams,
			   int currentBit,
			   int currentListSize,
			   int listSize) {

  int32_t pathMetric[2*listSize];
  decoder_list_int8_t *temp_dlist[2*listSize];
  
  int listIndex[2*listSize];
  int listIndex2[2*listSize];
 
  for (int i = 0; i < currentListSize; i++) { 
    listIndex[i]=i;
    pathMetric[i] = sorted_dlist[i]->pathMetric;
  }
  nr_sort_asc_int16_1D_array_ind(pathMetric, listIndex, currentListSize);
  for (int i=0;i<currentListSize;i++) {
    listIndex2[listIndex[i]] = i;
  }
  
  // copy the llr/bit arrays that are needed
  for (int i = 0; i < listSize; i++) {
    //	  printf("listIndex[%d] %d\n",i,listIndex[i]);
    if ((listIndex2[i+listSize]<listSize) && (listIndex2[i]<listSize)) { // both '0' and '1' path metrics are to be kept
      // do memcpy of LLR and Bit arrays
      
      for (int k = 0; k < (polarParams->n+1); k++) {
	
	memcpy((void*)&sorted_dlist[i+listSize]->bit[k][0],
	       (void*)&sorted_dlist[i]->bit[k][0],
	       sizeof(uint8_t)*polarParams->N);
	memcpy((void*)&sorted_dlist[i+listSize]->llr[k][0],
	       (void*)&sorted_dlist[i]->llr[k][0],
	       sizeof(int16_t)*polarParams->N);
      }
      sorted_dlist[i]->bit[0][currentBit]=0;
      sorted_dlist[i+listSize]->bit[0][currentBit]=1;
    }
    else if (listIndex2[i+listSize]<listSize) { // only '1' path metric is to be kept
      // just change the current bit from '0' to '1'
      
      for (int k = 0; k < (polarParams->n+1); k++) {
	memcpy((void*)&sorted_dlist[i+listSize]->bit[k][0],
	       (void*)&sorted_dlist[i]->bit[k][0],
	       sizeof(uint8_t)*polarParams->N);
	memcpy((void*)&sorted_dlist[i+listSize]->llr[k][0],
	       (void*)&sorted_dlist[i]->llr[k][0],
	       sizeof(int16_t)*polarParams->N); 
      }
      sorted_dlist[i+listSize]->bit[0][currentBit]=1;
      
      /*
	decoder_list_t *tmp = sorted_dlist[i+listSize];
	sorted_dlist[i+listSize] = sorted_dlist[i];
	sorted_dlist[i+listSize]->pathMetric = tmp->pathMetric;
	sorted_dlist[i+listSize]->bit[0][currentBit]=1;
	memcpy((void*)&sorted_dlist[i+listSize]->crcChecksum[0],
	(void*)&tmp->crcChecksum[0],
	24*sizeof(uint8_t));*/
      
    }
  }
  
 
  
  for (int i = 0; i < 2*listSize; i++) {
    temp_dlist[i] = sorted_dlist[i];
  }
  for (int i = 0; i < 2*listSize; i++) {
    //		  printf("i %d => %d\n",i,listIndex[i]);
    sorted_dlist[i] = temp_dlist[listIndex[i]];
  }
}

int8_t polar_decoder_int8(int16_t *input,
			  uint8_t *output,
			  t_nrPolar_params *polarParams,
			  uint8_t listSize,
			  time_stats_t *init,
			  time_stats_t *polar_rate_matching,
			  time_stats_t *decoding,
			  time_stats_t *bit_extraction,
			  time_stats_t *deinterleaving,
			  time_stats_t *sorting,
			  time_stats_t *path_metric,
			  time_stats_t *update_LLR)
{

  start_meas(init);
  
  
  uint8_t **bitUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
  uint8_t **llrUpdated = nr_alloc_uint8_t_2D_array(polarParams->N, (polarParams->n+1)); //0=False, 1=True
  decoder_list_int8_t dlist[2*listSize];

  
  for ( int i=0;i<2*listSize;i++) {
    //	  dlist[i].bit         = nr_alloc_uint8_t_2D_array((polarParams->n+1), polarParams->N);
    //	  dlist[i].llr         = nr_alloc_double_2D_array((polarParams->n+1), polarParams->N);
    //dlist[i].crcChecksum = malloc(sizeof(uint8_t)*polarParams->crcParityBits);
    for (int j=0; j< polarParams->n+1; j++) {
      memset((void*)&dlist[i].bit[j][0],0,sizeof(uint8_t)*polarParams->N);
      memset((void*)&dlist[i].llr[j][0],0,sizeof(int16_t)*polarParams->N);
    }
    for (int j=0;j<polarParams->crcParityBits;j++) dlist[i].crcChecksum[j] = 0;
    dlist[i].pathMetric  = 0;
  }
  
  for (int i=0; i<polarParams->N; i++) { 
    memset((void *)&llrUpdated[i][0],0,sizeof(uint8_t)*polarParams->n);
    memset((void *)&bitUpdated[i][0],0,sizeof(uint8_t)*polarParams->n);
    llrUpdated[i][polarParams->n]=1;
    bitUpdated[i][0]=((polarParams->information_bit_pattern[i]+1) % 2);
  }
  
  
  uint8_t **extended_crc_generator_matrix = malloc(polarParams->K * sizeof(uint8_t *)); //G_P3
  uint8_t **tempECGM = malloc(polarParams->K * sizeof(uint8_t *)); //G_P2
  for (int i = 0; i < polarParams->K; i++){
    extended_crc_generator_matrix[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
    tempECGM[i] = malloc(polarParams->crcParityBits * sizeof(uint8_t));
  }
  
  for (int i=0; i<polarParams->payloadBits; i++) {
    for (int j=0; j<polarParams->crcParityBits; j++) {
      tempECGM[i][j]=polarParams->crc_generator_matrix[i][j];
    }
  }
  for (int i=polarParams->payloadBits; i<polarParams->K; i++) {
    for (int j=0; j<polarParams->crcParityBits; j++) {
      if( (i-polarParams->payloadBits) == j ){
	tempECGM[i][j]=1;
      } else {
	tempECGM[i][j]=0;
      }
    }
  }
  
  for (int i=0; i<polarParams->K; i++) {
    for (int j=0; j<polarParams->crcParityBits; j++) {
      extended_crc_generator_matrix[i][j]=tempECGM[polarParams->interleaving_pattern[i]][j];
    }
  }
  
  stop_meas(init);
  start_meas(polar_rate_matching);
  
  int16_t d_tilde[polarParams->N];// = malloc(sizeof(double) * polarParams->N);
  nr_polar_rate_matching_int8(input, d_tilde, polarParams->rate_matching_pattern, polarParams->K, polarParams->N, polarParams->encoderLength);
  memcpy((void*)&dlist[0].llr[polarParams->n][0],(void*)&d_tilde[0],sizeof(int16_t)*polarParams->N);
  stop_meas(polar_rate_matching); 
  
  /*
   * SCL polar decoder.
   */
  start_meas(decoding);
  uint32_t nonFrozenBit=0;
  uint8_t currentListSize=1;
  
  decoder_list_int8_t *sorted_dlist[2*listSize];

  for (uint8_t i = 0; i < 2*listSize; i++) sorted_dlist[i] = &dlist[i];

  for (uint16_t currentBit=0; currentBit<polarParams->N; currentBit++){
    //    printf("***************** BIT %d (currentListSize %d, information_bit_pattern %d)\n",
    //	   currentBit,currentListSize,polarParams->information_bit_pattern[currentBit]);
    
    start_meas(update_LLR);
    updateLLR_int8(sorted_dlist, llrUpdated, bitUpdated, currentListSize, currentBit, 0, polarParams->N, (polarParams->n+1));
    stop_meas(update_LLR);
    
    if (polarParams->information_bit_pattern[currentBit]==0) { //Frozen bit.
      updatePathMetric0_int8(sorted_dlist,currentListSize, currentBit); //approximation=0 --> 11b, approximation=1 --> 12
    } else { //Information or CRC bit.
      start_meas(path_metric);
      updatePathMetric2_int8(sorted_dlist, currentListSize, currentBit);
      stop_meas(path_metric);
      
      start_meas(sorting); 
      
      if (currentListSize <= listSize/2) {
	// until listsize is full we need to copy bit and LLR arrays to new entries
	// below we only copy the ones we need to keep for sure
	decoder_int8_A(sorted_dlist,currentListSize,polarParams);
#ifdef SHOWCOMP
	printf("decoder_int8_A(sorted_dlist,%d,polarParams);\n",currentListSize);
#endif
      }	
	decoder_int8_B(sorted_dlist,currentListSize);
#ifdef SHOWCOMP
	printf("decoder_int8_B(sorted_dlist,%d);\n",currentListSize);
#endif
	
      bitUpdated[currentBit][0]=1;

      updateCrcChecksum2_int8(sorted_dlist,extended_crc_generator_matrix, currentListSize, nonFrozenBit, polarParams->crcParityBits);
      
      currentListSize*=2;
      
      //Keep only the best "listSize" number of entries.
      if (currentListSize > listSize) {

	decoder_int8_C(sorted_dlist,
		       polarParams,
		       currentBit,
		       currentListSize,
		       listSize);
#ifdef SHOWCOMP
	printf("decoder_int8_C(sorted_dlist,polarParams,%d,%d,%d);\n",currentBit,currentListSize,listSize);
#endif
	currentListSize = listSize;
      }

      stop_meas(sorting);
      
      nonFrozenBit++;      
      }

    }
  
  

  for (uint8_t i = 0; i < fmin(listSize, (pow(2,polarParams->crcCorrectionBits)) ); i++) {
    //	  printf("list index %d :",i);
    //	  for (int j=0;j<polarParams->crcParityBits;j++) printf("%d",sorted_dlist[i]->crcChecksum[j]);
    //	  printf(" => %d (%f)\n",sorted_dlist[i]->crcState,sorted_dlist[i]->pathMetric);
    int crcState = 1;
    for (int j=0;j<polarParams->crcParityBits;j++) if (sorted_dlist[i]->crcChecksum[j]!=0) crcState=0;
    if (crcState == 1) {
      for (int j = 0; j < polarParams->N; j++) polarParams->nr_polar_u[j]=sorted_dlist[i]->bit[0][j];
      
      start_meas(bit_extraction);
      //Extract the information bits (û to ĉ)
      nr_polar_info_bit_extraction(polarParams->nr_polar_u, polarParams->nr_polar_cPrime, polarParams->information_bit_pattern, polarParams->N);
      stop_meas(bit_extraction);
      //Deinterleaving (ĉ to b)
      start_meas(deinterleaving);
      nr_polar_deinterleaver(polarParams->nr_polar_cPrime, polarParams->nr_polar_b, polarParams->interleaving_pattern, polarParams->K);
      stop_meas(deinterleaving);
      //Remove the CRC (â)
      for (int j = 0; j < polarParams->payloadBits; j++) output[j]=polarParams->nr_polar_b[j];
      
      break;
    }
  }
  
  //	free(d_tilde);
  /*
    for (int i=0;i<2*listSize;i++) {
    //	  printf("correct: Freeing dlist[%d].bit %p\n",i,dlist[i].bit);
    nr_free_uint8_t_2D_array(dlist[i].bit, (polarParams->n+1));
    nr_free_double_2D_array(dlist[i].llr, (polarParams->n+1));
    free(dlist[i].crcChecksum);
    }*/
  nr_free_uint8_t_2D_array(extended_crc_generator_matrix, polarParams->K);
  nr_free_uint8_t_2D_array(tempECGM, polarParams->K);
  stop_meas(decoding);
  return(0);
}
