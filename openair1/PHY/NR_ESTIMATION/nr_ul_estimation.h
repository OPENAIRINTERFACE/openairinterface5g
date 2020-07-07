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

#ifndef __NR_UL_ESTIMATION_DEFS__H__
#define __NR_UL_ESTIMATION_DEFS__H__


#include "PHY/defs_gNB.h"
/** @addtogroup _PHY_PARAMETER_ESTIMATION_BLOCKS_
 * @{
 */


/*!
\brief This function performs channel estimation including frequency interpolation
\param gNB Pointer to gNB PHY variables
\param Ns slot number (0..19)
\param p
\param symbol symbol within slot
\param bwp_start_subcarrier, first allocated subcarrier
\param nb_rb_pusch, number of allocated RBs for this UE
*/

int nr_pusch_channel_estimation(PHY_VARS_gNB *gNB,
                                unsigned char Ns,
                                unsigned short p,
                                unsigned char symbol,
                                unsigned short bwp_start_subcarrier,
                                nfapi_nr_pusch_pdu_t *pusch_pdu);

int nr_est_timing_advance_pusch(PHY_VARS_gNB* phy_vars_gNB, int UE_id);

#endif
