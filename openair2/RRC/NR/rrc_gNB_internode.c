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

/*! \file rrc_gNB_internode.c
 * \brief rrc internode procedures for gNB
 * \author Raymond Knopp
 * \date 2019
 * \version 1.0
 * \company Eurecom
 * \email: raymond.knopp@eurecom.fr
 */
#ifndef RRC_GNB_INTERNODE_C
#define RRC_GNB_INTERNODE_C

#include "nr_rrc_defs.h"
#include "NR_RRCReconfiguration.h"
#include "NR_UE-NR-Capability.h"
#include "NR_CG-ConfigInfo.h"
#include "NR_UE-CapabilityRAT-ContainerList.h"
#include "LTE_UE-CapabilityRAT-ContainerList.h"
#include "NR_CG-Config.h"
#include "uper_encoder.h"
#include "uper_decoder.h"
#include "executables/softmodem-common.h"

int parse_CG_ConfigInfo(gNB_RRC_INST *rrc, NR_CG_ConfigInfo_t *CG_ConfigInfo, x2ap_ENDC_sgnb_addition_req_t *m) {
  if (CG_ConfigInfo->criticalExtensions.present == NR_CG_ConfigInfo__criticalExtensions_PR_c1) {
    if (CG_ConfigInfo->criticalExtensions.choice.c1) {
      if (CG_ConfigInfo->criticalExtensions.choice.c1->present == NR_CG_ConfigInfo__criticalExtensions__c1_PR_cg_ConfigInfo) {
        NR_CG_ConfigInfo_IEs_t *cg_ConfigInfo = CG_ConfigInfo->criticalExtensions.choice.c1->choice.cg_ConfigInfo;

        if (cg_ConfigInfo->ue_CapabilityInfo) {
          // Decode UE-CapabilityRAT-ContainerList
          NR_UE_CapabilityRAT_ContainerList_t *UE_CapabilityRAT_ContainerList=NULL;
          UE_CapabilityRAT_ContainerList = calloc(1,sizeof(NR_UE_CapabilityRAT_ContainerList_t));
          asn_dec_rval_t dec_rval = uper_decode(NULL,
                                                &asn_DEF_NR_UE_CapabilityRAT_ContainerList,
                                                (void **)&UE_CapabilityRAT_ContainerList,
                                                cg_ConfigInfo->ue_CapabilityInfo->buf,
                                                cg_ConfigInfo->ue_CapabilityInfo->size, 0, 0);

          if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
            AssertFatal(1==0,"[InterNode] Failed to decode NR_UE_CapabilityRAT_ContainerList (%zu bits), size of OCTET_STRING %lu\n",
                        dec_rval.consumed, cg_ConfigInfo->ue_CapabilityInfo->size);
          }

          rrc_parse_ue_capabilities(rrc,UE_CapabilityRAT_ContainerList, m,cg_ConfigInfo);
        }

        if (cg_ConfigInfo->candidateCellInfoListMN) AssertFatal(1==0,"Can't handle candidateCellInfoListMN yet\n");
      } else AssertFatal(1==0,"c1 extension is not cg_ConfigInfo, returning\n");
    } else {
      LOG_E(RRC,"c1 extension not found, returning\n");
      return(-1);
    }
  } else {
    LOG_E(RRC,"Ignoring unknown CG_ConfigInfo extensions\n");
    return(-1);
  }

  return(0);
}


int generate_CG_Config(gNB_RRC_INST *rrc,
                       NR_CG_Config_t *cg_Config,
                       NR_RRCReconfiguration_t *reconfig,
                       NR_RadioBearerConfig_t *rbconfig) {
  cg_Config->criticalExtensions.present = NR_CG_Config__criticalExtensions_PR_c1;
  cg_Config->criticalExtensions.choice.c1 = calloc(1,sizeof(*cg_Config->criticalExtensions.choice.c1));
  cg_Config->criticalExtensions.choice.c1->present = NR_CG_Config__criticalExtensions__c1_PR_cg_Config;
  cg_Config->criticalExtensions.choice.c1->choice.cg_Config = calloc(1,sizeof(NR_CG_Config_IEs_t));
  char buffer[1024];
  int total_size;
  asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_RRCReconfiguration, NULL, (void *)reconfig, buffer, 1024);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  cg_Config->criticalExtensions.choice.c1->choice.cg_Config->scg_CellGroupConfig = calloc(1,sizeof(OCTET_STRING_t));
  OCTET_STRING_fromBuf(cg_Config->criticalExtensions.choice.c1->choice.cg_Config->scg_CellGroupConfig,
                       (const char *)buffer,
                       (enc_rval.encoded+7)>>3);
  total_size = (enc_rval.encoded+7)>>3;

  FILE *fd; // file to be generated for nr-ue
  if (get_softmodem_params()->phy_test==1 || get_softmodem_params()->do_ra > 0 || get_softmodem_params()->sa == 1) {
    // This is for phytest only, emulate first X2 message if uecap.raw file is present
    LOG_I(RRC,"Dumping NR_RRCReconfiguration message (%jd bytes)\n",(enc_rval.encoded+7)>>3);
    for (int i=0; i<(enc_rval.encoded+7)>>3; i++) {
      printf("%02x",((uint8_t *)buffer)[i]);
    }
    printf("\n");
    fd = fopen("reconfig.raw","w");
    if (fd != NULL) {
      fwrite((void *)buffer,1,(size_t)((enc_rval.encoded+7)>>3),fd);
      fclose(fd);
    }
  }
  
  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_RadioBearerConfig, NULL, (void *)rbconfig, buffer, 1024);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
	       enc_rval.failed_type->name, enc_rval.encoded);
  cg_Config->criticalExtensions.choice.c1->choice.cg_Config->scg_RB_Config = calloc(1,sizeof(OCTET_STRING_t));
  
  OCTET_STRING_fromBuf(cg_Config->criticalExtensions.choice.c1->choice.cg_Config->scg_RB_Config,
		       (const char *)buffer,
		       (enc_rval.encoded+7)>>3);


  
  if (get_softmodem_params()->phy_test==1 || get_softmodem_params()->do_ra > 0 || get_softmodem_params()->sa == 1) {

    LOG_I(RRC,"Dumping scg_RB_Config message (%jd bytes)\n",(enc_rval.encoded+7)>>3);
    for (int i=0; i<(enc_rval.encoded+7)>>3; i++) {
      printf("%02x",((uint8_t *)buffer)[i]);
    }
    
    printf("\n");
    fd = fopen("rbconfig.raw","w");
    if (fd != NULL) {
      fwrite((void *)buffer,1,(size_t)((enc_rval.encoded+7)>>3),fd);
      fclose(fd);
    }
  }
  
  total_size = total_size + ((enc_rval.encoded+7)>>3);
  return(total_size);
}

#endif
