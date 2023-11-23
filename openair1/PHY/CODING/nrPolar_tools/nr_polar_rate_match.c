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

#include <math.h>
#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

void nr_polar_rate_matching_pattern(uint16_t *rmp, uint16_t *J, const uint8_t *P_i_, uint16_t K, uint16_t N, uint16_t E){
  int d[N];
  int y[N];

  for (int m = 0; m <= N - 1; m++)
    d[m] = m;

  for (int m = 0; m <= N - 1; m++) {
    i = floor((32 * m) / N);
    J[m] = (P_i_[i] * (N / 32)) + (m % (N / 32));
    y[m] = d[J[m]];
  }

  if (E >= N) { // repetition
    for (int k = 0; k <= E - 1; k++) {
      ind = (k % N);
      rmp[k] = y[ind];
    }
  } else {
    if ((K / (double)E) <= (7.0 / 16)) { // puncturing
      for (int k = 0; k <= E - 1; k++) {
        rmp[k] = y[k + N - E];
      }
    } else { // shortening
      for (int k = 0; k <= E - 1; k++) {
        rmp[k] = y[k];
      }
    }
  }
}


void nr_polar_rate_matching(double *input, double *output, uint16_t *rmp, uint16_t K, uint16_t N, uint16_t E){

	if (E>=N) { //repetition
		for (int i=0; i<=N-1; i++) output[i]=0;
		for (int i=0; i<=E-1; i++){
			output[rmp[i]]+=input[i];
		}
	} else {
		if ( (K/(double)E) <= (7.0/16) ) { //puncturing
			for (int i=0; i<=N-1; i++) output[i]=0;
		} else { //shortening
			for (int i=0; i<=N-1; i++) output[i]=INFINITY;
		}

		for (int i=0; i<=E-1; i++){
			output[rmp[i]]=input[i];
		}
	}

}

void nr_polar_rate_matching_int8(int16_t *input, int16_t *output, uint16_t *rmp, uint16_t K, uint16_t N, uint16_t E){

	if (E>=N) { //repetition
		for (int i=0; i<=N-1; i++) output[i]=0;
		for (int i=0; i<=E-1; i++){
			output[rmp[i]]+=input[i];
		}
	} else {
		if ( (K/(double)E) <= (7.0/16) ) { //puncturing
			for (int i=0; i<=N-1; i++) output[i]=0;
		} else { //shortening
			for (int i=0; i<=N-1; i++) output[i]=INFINITY;
		}

		for (int i=0; i<=E-1; i++){
			output[rmp[i]]=input[i];
		}
	}

}
