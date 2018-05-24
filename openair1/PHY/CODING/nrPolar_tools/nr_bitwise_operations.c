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

void nr_byte2bit(uint8_t *array, uint8_t arraySize, uint8_t *bitArray){//First 2 parameters are in bytes.

	for (int i=0; i<arraySize; i++){
		bitArray[(7+(i*8))] = ( array[i]>>0 & 0x01);
		bitArray[(6+(i*8))] = ( array[i]>>1 & 0x01);
		bitArray[(5+(i*8))] = ( array[i]>>2 & 0x01);
		bitArray[(4+(i*8))] = ( array[i]>>3 & 0x01);
		bitArray[(3+(i*8))] = ( array[i]>>4 & 0x01);
		bitArray[(2+(i*8))] = ( array[i]>>5 & 0x01);
		bitArray[(1+(i*8))] = ( array[i]>>6 & 0x01);
		bitArray[  (i*8)  ] = ( array[i]>>7 & 0x01);
	}

}
