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

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

void nr_polar_bit_insertion(uint8_t *input, uint8_t *output, uint16_t N, uint16_t K,
		int16_t *Q_I_N, int16_t *Q_PC_N, uint8_t n_PC){

	uint16_t k=0;
	uint8_t flag;

	if (n_PC>0) {
		/*
		 *
		 */
	} else {
		for (int n=0; n<=N-1; n++) {
			flag=0;
			for (int m=0; m<=(K+n_PC)-1; m++) {
				if ( n == Q_I_N[m]) {
					flag=1;
					break;
				}
			}
			if (flag) { // n ϵ Q_I_N
				output[n]=input[k];
				k++;
			} else {
				output[n] = 0;
			}
		}
	}

}
