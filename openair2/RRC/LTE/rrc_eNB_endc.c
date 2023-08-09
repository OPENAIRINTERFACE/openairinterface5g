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

#include "rrc_defs.h"
#include "rrc_extern.h"
#include "rrc_eNB_UE_context.h"
#include "common/ran_context.h"
#include "oai_asn1.h"
#include "LTE_DL-DCCH-Message.h"
#include "uper_encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

extern RAN_CONTEXT_t RC;
extern mui_t rrc_eNB_mui;

int rrc_eNB_generate_RRCConnectionReconfiguration_endc(protocol_ctxt_t *ctxt,
                                                       rrc_eNB_ue_context_t *ue_context,
                                                       unsigned char *buffer,
                                                       int buffer_size,
                                                       OCTET_STRING_t *scg_group_config,
                                                       OCTET_STRING_t *scg_RB_config)
{
  asn_enc_rval_t                                          enc_rval;
  LTE_DL_DCCH_Message_t                                   dl_dcch_msg;
  LTE_RRCConnectionReconfiguration_t                      *r;
  int                                                     trans_id;
  LTE_RadioResourceConfigDedicated_t                      rrcd;
#if 0
  LTE_DRB_ToAddModList_t                                  drb_list;
  struct LTE_DRB_ToAddMod                                 drb;
  long                                                    eps_bearer_id;
  struct LTE_RLC_Config                                   rlc;
  long                                                    lcid;
  struct LTE_LogicalChannelConfig                         lc;
#endif
  LTE_DRB_ToReleaseList_t                                 drb_list;
  LTE_DRB_Identity_t                                      drb;
  struct LTE_LogicalChannelConfig__ul_SpecificParameters  ul_params;
  long                                                    lcg;
  struct LTE_RadioResourceConfigDedicated__mac_MainConfig mac;

  memset(&rrcd, 0, sizeof(rrcd));
  memset(&drb_list, 0, sizeof(drb_list));
  memset(&drb, 0, sizeof(drb));
#if 0
  memset(&rlc, 0, sizeof(rlc));
  memset(&lc, 0, sizeof(lc));
#endif
  memset(&ul_params, 0, sizeof(ul_params));
  memset(&mac, 0, sizeof(mac));
  trans_id = rrc_eNB_get_next_transaction_identifier(ctxt->module_id);

  memset(&dl_dcch_msg,0,sizeof(LTE_DL_DCCH_Message_t));

  dl_dcch_msg.message.present           = LTE_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1.present = LTE_DL_DCCH_MessageType__c1_PR_rrcConnectionReconfiguration;

  r = &dl_dcch_msg.message.choice.c1.choice.rrcConnectionReconfiguration;

  r->rrc_TransactionIdentifier = trans_id;
  r->criticalExtensions.present = LTE_RRCConnectionReconfiguration__criticalExtensions_PR_c1;
  r->criticalExtensions.choice.c1.present = LTE_RRCConnectionReconfiguration__criticalExtensions__c1_PR_rrcConnectionReconfiguration_r8;
  r->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated = &rrcd;

#if 0
  rrcd.drb_ToAddModList = &drb_list;

  eps_bearer_id = 5;
  drb.eps_BearerIdentity = &eps_bearer_id;
  drb.drb_Identity = 5;
  drb.rlc_Config = &rlc;
  lcid = 4;
  drb.logicalChannelIdentity = &lcid;
  drb.logicalChannelConfig = &lc;

  asn1cSeqAdd(&drb_list.list, &drb);

  rlc.present = LTE_RLC_Config_PR_am;
  rlc.choice.am.ul_AM_RLC.t_PollRetransmit = LTE_T_PollRetransmit_ms50;
  rlc.choice.am.ul_AM_RLC.pollPDU = LTE_PollPDU_p16;
  rlc.choice.am.ul_AM_RLC.pollByte = LTE_PollByte_kBinfinity;
  rlc.choice.am.ul_AM_RLC.maxRetxThreshold = LTE_UL_AM_RLC__maxRetxThreshold_t8;
  rlc.choice.am.dl_AM_RLC.t_Reordering = LTE_T_Reordering_ms35;
  rlc.choice.am.dl_AM_RLC.t_StatusProhibit = LTE_T_StatusProhibit_ms25;

  lc.ul_SpecificParameters = &ul_params;
#endif

  /* release drb 1 */
  drb = 1;
  asn1cSeqAdd(&drb_list.list, &drb);
  rrcd.drb_ToReleaseList = &drb_list;

  ul_params.priority = 12;
  ul_params.prioritisedBitRate = LTE_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8;
  ul_params.bucketSizeDuration = LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms300;
  lcg = 3;
  ul_params.logicalChannelGroup = &lcg;

  rrcd.mac_MainConfig = &mac;

  mac.present = LTE_RadioResourceConfigDedicated__mac_MainConfig_PR_explicitValue;
  mac.choice.explicitValue.timeAlignmentTimerDedicated = LTE_TimeAlignmentTimer_sf10240;

  /* NR config */
  struct LTE_RRCConnectionReconfiguration_v890_IEs cr_890;
  struct LTE_RRCConnectionReconfiguration_v920_IEs cr_920;
  struct LTE_RRCConnectionReconfiguration_v1020_IEs cr_1020;
  struct LTE_RRCConnectionReconfiguration_v1130_IEs cr_1130;
  struct LTE_RRCConnectionReconfiguration_v1250_IEs cr_1250;
  struct LTE_RRCConnectionReconfiguration_v1310_IEs cr_1310;
  struct LTE_RRCConnectionReconfiguration_v1430_IEs cr_1430;
  struct LTE_RRCConnectionReconfiguration_v1510_IEs cr_1510;
  struct LTE_RRCConnectionReconfiguration_v1510_IEs__nr_Config_r15 nr;

  memset(&cr_890, 0, sizeof(cr_890));
  memset(&cr_920, 0, sizeof(cr_920));
  memset(&cr_1020, 0, sizeof(cr_1020));
  memset(&cr_1130, 0, sizeof(cr_1130));
  memset(&cr_1250, 0, sizeof(cr_1250));
  memset(&cr_1310, 0, sizeof(cr_1310));
  memset(&cr_1430, 0, sizeof(cr_1430));
  memset(&cr_1510, 0, sizeof(cr_1510));
  memset(&nr, 0, sizeof(nr));

  r->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.nonCriticalExtension = &cr_890;
  cr_890.nonCriticalExtension = &cr_920;
  cr_920.nonCriticalExtension = &cr_1020;
  cr_1020.nonCriticalExtension = &cr_1130;
  cr_1130.nonCriticalExtension = &cr_1250;
  cr_1250.nonCriticalExtension = &cr_1310;
  cr_1310.nonCriticalExtension = &cr_1430;
  cr_1430.nonCriticalExtension = &cr_1510;

  cr_1510.nr_Config_r15 = &nr;
  nr.present = LTE_RRCConnectionReconfiguration_v1510_IEs__nr_Config_r15_PR_setup;
  nr.choice.setup.endc_ReleaseAndAdd_r15 = 0;  /* FALSE */

  OCTET_STRING_t dummy_scg_conf;
  unsigned char scg_conf_buf[4] = { 0, 0, 0, 0 };
  if (scg_group_config!=NULL) {
	  nr.choice.setup.nr_SecondaryCellGroupConfig_r15 = scg_group_config; //&scg_conf;
          LOG_E(RRC, "setting scg_group_config\n");
  }
  else{
	  nr.choice.setup.nr_SecondaryCellGroupConfig_r15 = &dummy_scg_conf;
	  dummy_scg_conf.buf = scg_conf_buf;
	  dummy_scg_conf.size = 4;
  }

#ifdef DEBUG_SCG_CONFIG
	{
		int size_s = nr.choice.setup.nr_SecondaryCellGroupConfig_r15->size;
		int i;
		LOG_I(RRC, "Dumping nr_SecondaryCellGroupConfig: %d", size_s);
		for (i=0; i<size_s; i++) printf("%2.2x", (unsigned char)nr.choice.setup.nr_SecondaryCellGroupConfig_r15->buf[i]);
		printf("\n");

	}
#endif


  long sk_counter = 0;
  cr_1510.sk_Counter_r15 = &sk_counter;

  OCTET_STRING_t dummy_nr1_conf;
  unsigned char nr1_buf[4] = { 0, 0, 0, 0 };

  if(scg_RB_config!=NULL) {
	  cr_1510.nr_RadioBearerConfig1_r15 = scg_RB_config;
          LOG_E(RRC, "setting scg_RB_config\n");
  }
  else{
	  cr_1510.nr_RadioBearerConfig1_r15 = &dummy_nr1_conf;
	  dummy_nr1_conf.buf = nr1_buf;
	  dummy_nr1_conf.size = 4;
  }

#ifdef DEBUG_SCG_CONFIG
  {
	  int size_s = cr_1510.nr_RadioBearerConfig1_r15->size;
	  int i;
	  LOG_I(RRC, "Dumping nr_RadioBearerConfig1: %d", size_s);
	  for (i=0; i<size_s; i++) printf("%2.2x", (unsigned char)cr_1510.nr_RadioBearerConfig1_r15->buf[i]);
	  printf("\n");

  }
#endif


#if 0
  OCTET_STRING_t nr2_conf;
  unsigned char nr2_buf[4] = { 0, 0, 0, 0 };
  cr_1510.nr_RadioBearerConfig2_r15 = &nr2_conf;
  nr2_conf.buf = nr2_buf;
  nr2_conf.size = 4;
#endif

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_DL_DCCH_Message,
                                   NULL,
                                   (void *)&dl_dcch_msg,
                                   buffer,
                                   buffer_size);
  AssertFatal (enc_rval.encoded > 0, "asn_DEF_LTE_DL_DCCH_Message message encoding failed (%s, %jd)!\n",
                 enc_rval.failed_type->name, enc_rval.encoded);
{
int len = (enc_rval.encoded + 7) / 8;
int i;
printf("len = %d\n", len);
for (i = 0; i < len; i++) printf(" %2.2x", buffer[i]);
printf("\n");
}

  return (enc_rval.encoded + 7) / 8;
}
