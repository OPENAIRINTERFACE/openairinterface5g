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
#ifndef RRC_GNB_NSA_C
#define RRC_GNB_NSA_C

#include "nr_rrc_defs.h"
#include "NR_RRCReconfiguration.h"
#include "NR_UE-NR-Capability.h"

void rrc_parse_ue_capabilities(gNB_RRC_INST *rrc,BIT_STRING_t *ueCapabilityRAT_Container_nr,BIT_STRING_t *ueCapabilityRAT_Container_MRDC)  {

  struct rrc_gNB_ue_context_s        *ue_context_p = NULL;
  int rnti = taus()&65535;

  AssertFatal(ueCapabilityRAT_Container_nr!=NULL,"ueCapabilityRAT_Container_nr is NULL\n");
  AssertFatal(ueCapabilityRAT_Container_MRDC!=NULL,"ueCapabilityRAT_Container_MRDC is NULL\n");
  // decode and store capabilities
  ue_context_p = rrc_gNB_get_ue_context(rrc,
					rnti);
  
  asn_dec_rval_t dec_rval = uper_decode(NULL,
					&asn_DEF_NR_UE_NR_Capability,
					(void **)&ue_context_p->ue_context.UE_Capability_nr,
					ueCapabilityRAT_Container_nr->buf,
					ueCapabilityRAT_Container_nr->size, 0, 0);

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
      LOG_E(RRC, "Failed to decode UE NR capabilities (%zu bytes)\n", dec_rval.consumed);
      ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability,
                      ue_context_p->ue_context.UE_Capability_nr);
      ue_context_p->ue_context.UE_Capability_nr = 0;
      AssertFatal(1==0,"exiting\n");
  }

  dec_rval = uper_decode(NULL,
			 &asn_DEF_NR_UE_MRDC_Capability,
			 (void **)&ue_context_p->ue_context.UE_Capability_MRDC,
			 ueCapabilityRAT_Container_MRDC->buf,
			 ueCapabilityRAT_Container_MRDC->size, 0, 0);

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
      LOG_E(RRC, "Failed to decode UE MRDC capabilities (%zu bytes)\n", dec_rval.consumed);
      ASN_STRUCT_FREE(asn_DEF_NR_UE_MRDC_Capability,
                      ue_context_p->ue_context.UE_Capability_MRDC);
      ue_context_p->ue_context.UE_Capability_MRDC = 0;
      AssertFatal(1==0,"exiting\n");
  }

  // dump ue_capabilities

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
     xer_fprint(stdout, &asn_DEF_NR_UE_NR_Capability, ue_context_p->ue_context.UE_Capability_nr);
  }  

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
     xer_fprint(stdout, &asn_DEF_NR_UE_MRDC_Capability, ue_context_p->ue_context.UE_Capability_MRDC);
  }  

}

void rrc_add_nsa_user(gNB_RRC_INST *rrc) {

// generate nr-Config-r15 containers for LTE RRC : inside message for X2 EN-DC (CG-Config Message from 38.331)

  rrc_gNB_carrier_data_t *carrier=&rrc->carrier;

// NR RRCReconfiguration

  AssertFatal(carrier->reconfig[rrc->Nb_ue]==NULL,
	      "carrier->reconfig[%d] isn't null\n",rrc->Nb_ue);
  AssertFatal(rrc->Nb_ue < MAX_NR_RRC_UE_CONTEXTS,"cannot add another UE\n");

  carrier->reconfig[rrc->Nb_ue] = calloc(1,sizeof(NR_RRCReconfiguration_t));
  carrier->secondaryCellGroup[rrc->Nb_ue] = calloc(1,sizeof(NR_CellGroupConfig_t));
  memset((void*)carrier->reconfig[rrc->Nb_ue],0,sizeof(NR_RRCReconfiguration_t));
  carrier->reconfig[rrc->Nb_ue]->rrc_TransactionIdentifier=0;
  carrier->reconfig[rrc->Nb_ue]->criticalExtensions.present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration;
  NR_RRCReconfiguration_IEs_t *reconfig_ies=calloc(1,sizeof(NR_RRCReconfiguration_IEs_t));
  carrier->reconfig[rrc->Nb_ue]->criticalExtensions.choice.rrcReconfiguration = reconfig_ies;
  fill_default_reconfig(carrier->ServingCellConfigCommon,reconfig_ies);

  rrc->Nb_ue++;
}


#endif
