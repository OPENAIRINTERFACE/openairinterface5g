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

/*! \file rrc_gNB_nsa.c
 * \brief rrc NSA procedures for gNB
 * \author Raymond Knopp
 * \date 2019
 * \version 1.0
 * \company Eurecom
 * \email: raymond.knopp@eurecom.fr
 */
#define RRC_GNB_NSA_C
#define RRC_GNB_NSA_C

#include "NR_RRCReconfiguration.h"

void rrc_parse_ue_capabilities() {

}

void rrc_add_nsa_user(NR_RRC_VARS *rrc) {

// generate nr-Config-r15 containers for LTE RRC : inside message for X2 EN-DC (CG-Config Message from 38.331)



// NR RRCReconfiguration

  AssertFatal(rrc->reconfig[rrc->num_UEs]==NULL,
	      "rrc->reconfig[%d] isn't null\n",rrc->num_UEs);
  AssertFatal(rrc->num_UEs < MAX_NR_RRC_UE_CONTEXTS);

  rrc->reconfig[rrc->num_UEs] = calloc(1,sizeof(NR_RRCReconfiguration_t));
  rrc->secondaryCellGroup[rrc->num_UEs] = calloc(1,sizeof(NR_CellGroupConfig_t));
  memset((void*)rrc->reconfig[rrc->num_UEs],0,sizeof(NR_RRCReconfiguration_t));
  rrc->reconfig[rrc->num_UEs].present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration;
  NR_RRCReconfiguration_IEs_t *reconfig_ies=calloc(1,sizeof(NR_RRCReconfiguration_IEs_t));
  rrc->reconfig[rrc->num_UEs].choice.rrcReconiguration = reconfig_ies;
  fill_default_reconfig(rrc->scc,reconfig_ies);

  rrc->num_UEs++;
}


#endif
