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

/*! \file PHY/NR_TRANSPORT/nr_mcs.c
* \brief Some support routines for NR MCS computations
* \author
* \date 2018
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"

//get_Qm under PHY/LTE_TRANSPORT/lte_mcs.c is the same for NR.

uint8_t get_nr_Qm(uint8_t I_MCS)
{
  if (I_MCS < 5)
    return(2);
  else if (I_MCS < 11)
    return(4);
  else if (I_MCS < 20)
    return(6);
  else
    return(8);
}

uint8_t get_nr_Qm_ul(uint8_t I_MCS) {

  if (I_MCS < 2)
	  return(2);  //This should be 1 if UE has reported to support pi/2 BPSK, and 2 otherwise.
  else if (I_MCS < 10)
    return(2);
  else if (I_MCS < 17)
    return(4);
  else
    return(6);
}
