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

/*! \file PHY/CODING/nrPolar_tools/nr_polar_dci_defs.h
* \brief Defines the constant variables for polar coding of the DCI from 38-212, V15.1.1 2018-04.
* \author
* \date 2018
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#ifndef __NR_POLAR_DCI_DEFS__H__
#define __NR_POLAR_DCI_DEFS__H__

#define NR_POLAR_DCI_MESSAGE_TYPE 1 //int8_t
#define NR_POLAR_DCI_CRC_PARITY_BITS 24
#define NR_POLAR_DCI_CRC_ERROR_CORRECTION_BITS 3

//Sec. 7.3.3: Channel Coding
#define NR_POLAR_DCI_N_MAX 9   //uint8_t
#define NR_POLAR_DCI_I_IL 1    //uint8_t
#define NR_POLAR_DCI_I_SEG 0   //uint8_t
#define NR_POLAR_DCI_N_PC 0    //uint8_t
#define NR_POLAR_DCI_N_PC_WM 0 //uint8_t

//Sec. 7.3.4: Rate Matching
#define NR_POLAR_DCI_I_BIL 0 //uint8_t

#endif
