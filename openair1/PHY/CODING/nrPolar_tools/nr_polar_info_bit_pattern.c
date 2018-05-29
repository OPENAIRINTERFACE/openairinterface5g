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

void nr_polar_info_bit_pattern(uint8_t *ibp, int16_t *Q_I_N, int16_t *Q_F_N,
		uint16_t *J, const uint16_t *Q_0_Nminus1, uint16_t K, uint16_t N, uint16_t E,
		uint8_t n_PC) {

	int16_t *Q_Ftmp_N = malloc(sizeof(int16_t) * (N + 1)); // Last element shows the final
	int16_t *Q_Itmp_N = malloc(sizeof(int16_t) * (N + 1)); // array index assigned a value.

	for (int i = 0; i <= N; i++) {
		Q_Ftmp_N[i] = -1; // Empty array.
		Q_Itmp_N[i] = -1;
	}

	uint8_t flag;
	uint16_t limit, ind;

	if (E < N) {
		if ((K / (double) E) <= (7.0 / 16)) { //puncturing
			for (int n = 0; n <= N - E - 1; n++) {
				ind = Q_Ftmp_N[N] + 1;
				Q_Ftmp_N[ind] = J[n];
				Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
			}

			if ((E / (double) N) >= (3.0 / 4)) {
				limit = ceil((double) (3 * N - 2 * E) / 4);
				for (int n = 0; n <= limit - 1; n++) {
					ind = Q_Ftmp_N[N] + 1;
					Q_Ftmp_N[ind] = n;
					Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
				}
			} else {
				limit = ceil((double) (9 * N - 4 * E) / 16);
				for (int n = 0; n <= limit - 1; n++) {
					ind = Q_Ftmp_N[N] + 1;
					Q_Ftmp_N[ind] = n;
					Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
				}
			}
		} else { //shortening
			for (int n = E; n <= N - 1; n++) {
				ind = Q_Ftmp_N[N] + 1;
				Q_Ftmp_N[ind] = J[n];
				Q_Ftmp_N[N] = Q_Ftmp_N[N] + 1;
			}
		}
	}

	//Q_I,tmp_N = Q_0_N-1 \ Q_F,tmp_N
	for (int n = 0; n <= N - 1; n++) {
		flag = 1;
		for (int m = 0; m <= Q_Ftmp_N[N]; m++) {
			if (Q_0_Nminus1[n] == Q_Ftmp_N[m]) {
				flag = 0;
				break;
			}
		}
		if (flag) {
			Q_Itmp_N[Q_Itmp_N[N] + 1] = Q_0_Nminus1[n];
			Q_Itmp_N[N]++;
		}
	}

	//Q_I_N comprises (K+n_PC) most reliable bit indices in Q_I,tmp_N
	for (int n = 0; n <= (K + n_PC) - 1; n++) {
		ind = Q_Itmp_N[N] + n - ((K + n_PC) - 1);
		Q_I_N[n] = Q_Itmp_N[ind];
	}

	//Q_F_N = Q_0_N-1 \ Q_I_N
	for (int n = 0; n <= N - 1; n++) {
		flag = 1;
		for (int m = 0; m <= (K + n_PC) - 1; m++) {
			if (Q_0_Nminus1[n] == Q_I_N[m]) {
				flag = 0;
				break;
			}
		}
		if (flag) {
			Q_F_N[Q_F_N[N] + 1] = Q_0_Nminus1[n];
			Q_F_N[N]++;
		}
	}

	//Information Bit Pattern
	for (int n = 0; n <= N - 1; n++) {
		ibp[n] = 0;

		for (int m = 0; m <= (K + n_PC) - 1; m++) {
			if (n == Q_I_N[m]) {
				ibp[n] = 1;
				break;
			}
		}
	}

	free(Q_Ftmp_N);
	free(Q_Itmp_N);
}

void nr_polar_info_bit_extraction(uint8_t *input, uint8_t *output, uint8_t *pattern, uint16_t size) {
	uint16_t j = 0;
	for (int i = 0; i < size; i++) {
		if (pattern[i] > 0) {
			output[j] = input[i];
			j++;
		}
	}
}
