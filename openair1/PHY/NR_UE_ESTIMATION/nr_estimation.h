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

#ifndef __NR_ESTIMATION_DEFS__H__
#define __NR_ESTIMATION_DEFS__H__


#include "PHY/defs_nr_UE.h"
//#include "PHY/defs_gNB.h"
/** @addtogroup _PHY_PARAMETER_ESTIMATION_BLOCKS_
 * @{
 */

/*!\brief Timing drift hysterisis in samples*/
#define SYNCH_HYST 2

/*!
\brief This function performs channel estimation including frequency and temporal interpolation
\param phy_vars_ue Pointer to UE PHY variables
\param eNB_id Index of target eNB
\param eNB_offset Offset for interfering eNB (in terms cell ID mod 3)
\param Ns slot number (0..19)
\param p antenna port
\param l symbol within slot
\param symbol symbol within frame
*/
int nr_pdcch_channel_estimation(PHY_VARS_NR_UE *ue,
				uint8_t eNB_id,
				uint8_t eNB_offset,
				unsigned char Ns,
				unsigned char p,
				unsigned char l,
				unsigned char symbol,
				unsigned short coreset_start_subcarrier,
				unsigned short nb_rb_coreset);

int nr_pbch_channel_estimation(PHY_VARS_NR_UE *ue,
                              uint8_t eNB_id,
                              uint8_t eNB_offset,
                              unsigned char Ns,
                              unsigned char p,
                              unsigned char l,
                              unsigned char symbol);

int nr_pdsch_channel_estimation(PHY_VARS_NR_UE *ue,
                              uint8_t eNB_id,
                              uint8_t eNB_offset,
                              unsigned char Ns,
                              unsigned char p,
                              unsigned char l,
                              unsigned char symbol,
							  unsigned short bwp_start_subcarrier,
							  unsigned short nb_rb_pdsch);
#endif
