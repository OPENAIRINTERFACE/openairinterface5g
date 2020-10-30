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
#include "common/utils/LOG/log.h"
#include "pdcp.h"
#include "common/ran_context.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"

#include "intertask_interface.h"

#include "NR_MIB.h"
#include "NR_BCCH-BCH-Message.h"

extern RAN_CONTEXT_t RC;


int generate_pdcch_ConfigSIB1(NR_PDCCH_ConfigSIB1_t *pdcch_ConfigSIB1,
                              long ssbSubcarrierSpacing,
                              long subCarrierSpacingCommon,
                              channel_bandwidth_t min_channel_bw) {

    nr_ssb_and_cset_mux_pattern_type_t mux_pattern;

    switch (ssbSubcarrierSpacing) {

        case NR_SubcarrierSpacing_kHz15:
            if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz15) {
                pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_1_NUM_INDEXES;
                mux_pattern = table_38213_13_1_c1[pdcch_ConfigSIB1->controlResourceSetZero];
            } else if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz30) {
                pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_2_NUM_INDEXES;
                mux_pattern = table_38213_13_2_c1[pdcch_ConfigSIB1->controlResourceSetZero];
            } else {
                AssertFatal(true,"Invalid subCarrierSpacingCommon\n");
            }
            break;

        case NR_SubcarrierSpacing_kHz30:
            if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz15) {

                if ( (min_channel_bw == bw_5MHz) || (min_channel_bw == bw_10MHz) ) {
                    pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_3_NUM_INDEXES;
                    mux_pattern = table_38213_13_3_c1[pdcch_ConfigSIB1->controlResourceSetZero];
                } else if (min_channel_bw == bw_40MHz) {
                    pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_5_NUM_INDEXES;
                    mux_pattern = table_38213_13_5_c1[pdcch_ConfigSIB1->controlResourceSetZero];
                } else {
                    AssertFatal(true,"Invalid min_bandwidth\n");
                }

            } else if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz30) {

                if ( (min_channel_bw == bw_5MHz) || (min_channel_bw == bw_10MHz) ) {
                    pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_4_NUM_INDEXES;
                    mux_pattern = table_38213_13_4_c1[pdcch_ConfigSIB1->controlResourceSetZero];
                } else if (min_channel_bw == bw_40MHz) {
                    pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_6_NUM_INDEXES;
                    mux_pattern = table_38213_13_6_c1[pdcch_ConfigSIB1->controlResourceSetZero];
                } else {
                    AssertFatal(true,"Invalid min_bandwidth\n");
                }

            } else {
                AssertFatal(true,"Invalid subCarrierSpacingCommon\n");
            }
            break;

        case NR_SubcarrierSpacing_kHz120:
            if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz60) {
                pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_7_NUM_INDEXES;
                mux_pattern = table_38213_13_7_c1[pdcch_ConfigSIB1->controlResourceSetZero];
            } else if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz120) {
                pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_8_NUM_INDEXES;
                mux_pattern = table_38213_13_8_c1[pdcch_ConfigSIB1->controlResourceSetZero];
            } else {
                AssertFatal(true,"Invalid subCarrierSpacingCommon\n");
            }
            break;

        case NR_SubcarrierSpacing_kHz240:
            if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz60) {
                pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_9_NUM_INDEXES;
                mux_pattern = table_38213_13_9_c1[pdcch_ConfigSIB1->controlResourceSetZero];
            } else if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz120) {
                pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_10_NUM_INDEXES;
                mux_pattern = table_38213_13_10_c1[pdcch_ConfigSIB1->controlResourceSetZero];
            } else {
                AssertFatal(true,"Invalid subCarrierSpacingCommon\n");
            }
            break;

        default:
            AssertFatal(true,"Invalid ssbSubcarrierSpacing\n");
            break;
    }


    frequency_range_t frequency_range = FR1;
    if(ssbSubcarrierSpacing>=60) {
        frequency_range = FR2;
    }

    pdcch_ConfigSIB1->searchSpaceZero = 0;
    if(mux_pattern == NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 && frequency_range == FR1){
        pdcch_ConfigSIB1->searchSpaceZero = rand() % TABLE_38213_13_11_NUM_INDEXES;
    }
    if(mux_pattern == NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 && frequency_range == FR2){
        pdcch_ConfigSIB1->searchSpaceZero = rand() % TABLE_38213_13_12_NUM_INDEXES;
    }

    return 0;
}

int8_t mac_rrc_nr_data_req(const module_id_t Mod_idP,
                           const int         CC_id,
                           const frame_t     frameP,
                           const rb_id_t     Srb_id,
                           const uint8_t     Nb_tb,
                           uint8_t *const    buffer_pP ){

  asn_enc_rval_t enc_rval;
  uint8_t Sdu_size = 0;
  uint8_t sfn_msb = (uint8_t)((frameP>>4)&0x3f);


#ifdef DEBUG_RRC
    LOG_D(RRC, "[eNB %d] mac_rrc_data_req to SRB ID=%ld\n", Mod_idP, Srb_id);
#endif

    rrc_gNB_carrier_data_t *carrier;
    NR_BCCH_BCH_Message_t *mib;
    NR_SRB_INFO *srb_info;
    char payload_size, *payload_pP;

    carrier = &RC.nrrrc[Mod_idP]->carrier;
    mib = &carrier->mib;
    srb_info = &carrier->Srb0;

    /* MIBCH */
    if ((Srb_id & RAB_OFFSET) == MIBCH) {

        // Currently we are getting the pdcch_ConfigSIB1 from the configuration file.
        // 3GPP is not clear about the static/semi-static/dynamic behaviour of the pdcch_ConfigSIB1 value
        // on the gNB side.
        // Uncomment this function for a dynamic pdcch_ConfigSIB1.
        // TODO: Update this static value
        //channel_bandwidth_t min_channel_bw = bw_10MHz; // Must be obtained based on TS 38.101-1 Table 5.3.5-1
        //generate_pdcch_ConfigSIB1(carrier->pdcch_ConfigSIB1,
        //                          *carrier->servingcellconfigcommon->ssbSubcarrierSpacing,
        //                          carrier->mib.message.choice.mib->subCarrierSpacingCommon,
        //                          min_channel_bw);

        mib->message.choice.mib->pdcch_ConfigSIB1.controlResourceSetZero = carrier->pdcch_ConfigSIB1->controlResourceSetZero;
        mib->message.choice.mib->pdcch_ConfigSIB1.searchSpaceZero = carrier->pdcch_ConfigSIB1->searchSpaceZero;

        mib->message.choice.mib->systemFrameNumber.buf[0] = sfn_msb << 2;
        enc_rval = uper_encode_to_buffer(&asn_DEF_NR_BCCH_BCH_Message,
                                         NULL,
                                         (void *) mib,
                                         carrier->MIB,
                                         24);
        LOG_D(NR_RRC, "Encoded MIB for frame %d sfn_msb %d (%p), bits %lu\n", frameP, sfn_msb, carrier->MIB,
              enc_rval.encoded);
        buffer_pP[0] = carrier->MIB[0];
        buffer_pP[1] = carrier->MIB[1];
        buffer_pP[2] = carrier->MIB[2];
        LOG_D(NR_RRC, "MIB PDU buffer_pP[0]=%x , buffer_pP[1]=%x, buffer_pP[2]=%x\n", buffer_pP[0], buffer_pP[1],
              buffer_pP[2]);
        AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
                     enc_rval.failed_type->name, enc_rval.encoded);
        return (3);
    }

  /* TODO BCCH SIB1 SIBs */
  if ((Srb_id & RAB_OFFSET ) == BCCH) {
    memcpy(&buffer_pP[0],
           RC.nrrrc[Mod_idP]->carrier.SIB1,
           RC.nrrrc[Mod_idP]->carrier.sizeof_SIB1);

    return RC.nrrrc[Mod_idP]->carrier.sizeof_SIB1;
  }

  /* CCCH */
  if( (Srb_id & RAB_OFFSET ) == CCCH) {
    //struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[Mod_idP],rnti);
    //if (ue_context_p == NULL) return(0);
    //eNB_RRC_UE_t *ue_p = &ue_context_p->ue_context;
    LOG_D(RRC,"[gNB %d] Frame %d CCCH request (Srb_id %ld)\n", Mod_idP, frameP, Srb_id);

    // srb_info=&ue_p->Srb0;

    payload_size = srb_info->Tx_buffer.payload_size;

    // check if data is there for MAC
    if (payload_size > 0) {
      payload_pP = srb_info->Tx_buffer.Payload;
      LOG_D(RRC,"[gNB %d] CCCH (%p) has %d bytes (dest: %p, src %p)\n", Mod_idP, srb_info, payload_size, buffer_pP, payload_pP);
      // Fill buffer
      memcpy((void *)buffer_pP, (void*)payload_pP, payload_size);
      Sdu_size = payload_size;
      srb_info->Tx_buffer.payload_size = 0;
    }
    return Sdu_size;
  }

  return(0);

}