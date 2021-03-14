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

/*! \file PHY/CODING/nrPolar_tools/nr_polar_uci_defs.h
* \brief Defines the constant variables for polar coding of the UCI from 38-212, V15.1.1 2018-04.
* \author
* \date 2018
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#ifndef __NR_POLAR_UCI_DEFS__H__
#define __NR_POLAR_UCI_DEFS__H__

#define NR_POLAR_UCI_PUCCH_MESSAGE_TYPE 2 //int8_t
#define NR_POLAR_PUCCH_CRC_ERROR_CORRECTION_BITS 3
#define NR_POLAR_PUCCH_PAYLOAD_BITS 32
#define NR_POLAR_PUCCH_E 32

//Ref. 38-212, Section 6.3.1.2.1
#define NR_POLAR_PUCCH_CRC_PARITY_BITS_SHORT 6
#define NR_POLAR_PUCCH_CRC_PARITY_BITS_LONG 11

#define NR_POLAR_PUCCH_I_SEG_LONG 1
#define NR_POLAR_PUCCH_I_SEG_SHORT 0

//Ref. 38-212, Section 6.3.1.3.1
#define NR_POLAR_PUCCH_N_MAX 10
#define NR_POLAR_PUCCH_I_IL 0
#define NR_POLAR_PUCCH_N_PC_SHORT 3
#define NR_POLAR_PUCCH_N_PC_LONG 0
#define NR_POLAR_PUCCH_N_PC_WM_LONG 0
#define NR_POLAR_PUCCH_N_PC_WM_SHORT 1

//Ref. 38-212, Section 6.3.1.4.1
#define NR_POLAR_PUCCH_I_BIL 1



#endif
