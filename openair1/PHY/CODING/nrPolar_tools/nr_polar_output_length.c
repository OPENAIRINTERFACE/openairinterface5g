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

uint32_t nr_polar_output_length(uint16_t K, uint16_t E, uint8_t n_max){
	uint8_t n_1, n_2, n_min=5, n;
	uint32_t polar_code_output_length;
	double R_min=1.0/8;
	
	if ( (E <= (9.0/8)*pow(2,ceil(log2(E))-1)) && (K/E < 9.0/16) ) {
		n_1 = ceil(log2(E))-1;		
	} else {
		n_1 = ceil(log2(E));
	}
	
	n_2 = ceil(log2(K/R_min));
	
	n=n_max;
	if (n>n_1) n=n_1;
	if (n>n_2) n=n_2;
	if (n<n_min) n=n_min;
	
	polar_code_output_length = (uint32_t) pow(2.0,n);
	
	return polar_code_output_length;
}
