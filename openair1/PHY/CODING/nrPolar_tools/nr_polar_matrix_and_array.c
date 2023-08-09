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

/*!\file PHY/CODING/nrPolar_tools/nr_polar_matrix_and_array.c
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

void nr_matrix_multiplication_uint8_1D_uint8_2D(const uint8_t *matrix1,
                                                const uint8_t **matrix2,
                                                uint8_t *output,
                                                uint16_t row,
                                                uint16_t col)
{
  for (uint16_t i = 0; i < col; i++) {
    output[i] = 0;
    for (uint16_t j = 0; j < row; j++) {
			output[i] += matrix1[j] * matrix2[j][i];
		}
  }
}

// Modified Bubble Sort.
void nr_sort_asc_double_1D_array_ind(double *matrix, uint8_t *ind, uint8_t len) {
	int swaps;
	double temp;
	int tempInd;

	for (int i = 0; i < len; i++) {
		swaps = 0;
		for (int j = 0; j < (len - i) - 1; j++) {
			if (matrix[j] > matrix[j + 1]) {
				temp = matrix[j];
				matrix[j] = matrix[j + 1];
				matrix[j + 1] = temp;

				tempInd = ind[j];
				ind[j] = ind[j + 1];
				ind[j + 1] = tempInd;

				swaps++;
			}
		}
		if (swaps == 0)
			break;
	}
}

void nr_sort_asc_int16_1D_array_ind(int32_t *matrix, int *ind, int len) {
  int swaps;
  int16_t temp;
  int tempInd;
  
  for (int i = 0; i < len; i++) {
    swaps = 0;
    for (int j = 0; j < (len - i) - 1; j++) {
      if (matrix[j] > matrix[j + 1]) {
	temp = matrix[j];
	matrix[j] = matrix[j + 1];
	matrix[j + 1] = temp;
	
	tempInd = ind[j];
	ind[j] = ind[j + 1];
	ind[j + 1] = tempInd;
	
	swaps++;
      }
    }
    if (swaps == 0)
      break;
  }
}
