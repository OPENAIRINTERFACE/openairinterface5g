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

/*! \file l2_nr_interface.c
 * \brief layer 2 interface, used to support different RRC sublayer
 * \author Raymond Knopp and Navid Nikaein, WEI-TAI CHEN
 * \date 2010-2014, 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: raymond.knopp@eurecom.fr, kroempa@gmail.com
 */

#include "platform_types.h"
#include "nr_rrc_defs.h"
#include "nr_rrc_extern.h"
#include "common/utils/LOG/log.h"
#include "pdcp.h"
#include "msc.h"
#include "common/ran_context.h"

#include "intertask_interface.h"

#include "NR_MIB.h"
#include "NR_BCCH-BCH-Message.h"

extern RAN_CONTEXT_t RC;

int8_t mac_rrc_nr_data_req(const module_id_t Mod_idP,
                           const int         CC_id,
                           const frame_t     frameP,
                           const rb_id_t     Srb_id,
                           const uint8_t     Nb_tb,
                           uint8_t *const    buffer_pP ){

  asn_enc_rval_t enc_rval;
  //SRB_INFO *Srb_info;
  //uint8_t Sdu_size                = 0;
  uint8_t sfn_msb                     = (uint8_t)((frameP>>4)&0x3f);
  
#ifdef DEBUG_RRC
  LOG_D(RRC,"[eNB %d] mac_rrc_data_req to SRB ID=%ld\n",Mod_idP,Srb_id);
#endif

  gNB_RRC_INST *rrc;
  rrc_gNB_carrier_data_t *carrier;
  NR_BCCH_BCH_Message_t *mib;
  
  rrc     = RC.nrrrc[Mod_idP];
  carrier = &rrc->carrier;
  mib     = &carrier->mib;

  if( (Srb_id & RAB_OFFSET ) == MIBCH) {
    mib->message.choice.mib->systemFrameNumber.buf[0] = sfn_msb << 2;
    enc_rval = uper_encode_to_buffer(&asn_DEF_NR_BCCH_BCH_Message,
                                     NULL,
                                     (void *)mib,
                                     carrier->MIB,
                                     24);
    LOG_D(NR_RRC,"Encoded MIB for frame %d sfn_msb %d (%p), bits %lu\n",frameP,sfn_msb,carrier->MIB,enc_rval.encoded);
    buffer_pP[0]=carrier->MIB[0];
    buffer_pP[1]=carrier->MIB[1];
    buffer_pP[2]=carrier->MIB[2];
    LOG_D(NR_RRC,"MIB PDU buffer_pP[0]=%x , buffer_pP[1]=%x, buffer_pP[2]=%x\n",buffer_pP[0],buffer_pP[1],buffer_pP[2]);
    AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
                 enc_rval.failed_type->name, enc_rval.encoded);
    return(3);
  }

//BCCH SIB1 SIBs

//CCCH

  return(0);

}
