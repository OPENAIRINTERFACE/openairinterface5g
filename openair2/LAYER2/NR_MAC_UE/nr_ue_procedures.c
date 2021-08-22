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

/* \file ue_procedures.c
 * \brief procedures related to UE
 * \author R. Knopp, K.H. HSU, G. Casati
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr, guido.casati@iis.fraunhofer.de
 * \note
 * \warning
 */


#include <stdio.h>
#include <math.h>

/* exe */
#include "executables/nr-softmodem.h"

/* RRC*/
#include "RRC/NR_UE/rrc_proto.h"
#include "NR_RACH-ConfigCommon.h"
#include "NR_RACH-ConfigGeneric.h"
#include "NR_FrequencyInfoDL.h"
#include "NR_PDCCH-ConfigCommon.h"

/* MAC */
#include "NR_MAC_COMMON/nr_mac.h"
#include "NR_MAC_UE/mac_proto.h"
#include "NR_MAC_UE/mac_extern.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "common/utils/nr/nr_common.h"

/* PHY */
#include "PHY/NR_TRANSPORT/nr_dci.h"
#include "executables/softmodem-common.h"
#include "SCHED_NR_UE/defs.h"

/* utils */
#include "assertions.h"
#include "asn1_conversions.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

//#define DEBUG_MIB
//#define ENABLE_MAC_PAYLOAD_DEBUG 1
//#define DEBUG_EXTRACT_DCI
//#define DEBUG_RAR

extern uint32_t N_RB_DL;

/* TS 38.213 9.2.5.2 UE procedure for multiplexing HARQ-ACK/SR and CSI in a PUCCH */
/* this is a counter of number of pucch format 4 per subframe */
static int nb_pucch_format_4_in_subframes[LTE_NUMBER_OF_SUBFRAMES_PER_FRAME] = { 0 } ;

/* TS 36.213 Table 9.2.3-3: Mapping of values for one HARQ-ACK bit to sequences */
static const int sequence_cyclic_shift_1_harq_ack_bit[2]
/*        HARQ-ACK Value        0    1 */
/* Sequence cyclic shift */ = { 0,   6 };

/* TS 36.213 Table 9.2.5-1: Mapping of values for one HARQ-ACK bit and positive SR to sequences */
static const int sequence_cyclic_shift_1_harq_ack_bit_positive_sr[2]
/*        HARQ-ACK Value        0    1 */
/* Sequence cyclic shift */ = { 3,   9 };

/* TS 36.213 Table 9.2.5-2: Mapping of values for two HARQ-ACK bits and positive SR to sequences */
static const int sequence_cyclic_shift_2_harq_ack_bits_positive_sr[4]
/*        HARQ-ACK Value      (0,0)  (0,1)   (1,0)  (1,1) */
/* Sequence cyclic shift */ = {  1,     4,     10,     7 };

/* TS 38.213 Table 9.2.3-4: Mapping of values for two HARQ-ACK bits to sequences */
static const int sequence_cyclic_shift_2_harq_ack_bits[4]
/*        HARQ-ACK Value       (0,0)  (0,1)  (1,0)  (1,1) */
/* Sequence cyclic shift */ = {   0,     3,     9,     6 };


/* TS 38.211 Table 6.4.1.3.3.2-1: DM-RS positions for PUCCH format 3 and 4 */
static const int nb_symbols_excluding_dmrs[11][2][2]
= {
/*                     No additional DMRS            Additional DMRS   */
/* PUCCH length      No hopping   hopping         No hopping   hopping */
/* index                  0          1                 0          1    */
/*    4     */    {{      3    ,     2   }   ,  {      3     ,    2    }},
/*    5     */    {{      3    ,     3   }   ,  {      3     ,    3    }},
/*    6     */    {{      4    ,     4   }   ,  {      4     ,    4    }},
/*    7     */    {{      5    ,     5   }   ,  {      5     ,    5    }},
/*    8     */    {{      6    ,     6   }   ,  {      6     ,    6    }},
/*    9     */    {{      7    ,     7   }   ,  {      7     ,    7    }},
/*   10     */    {{      8    ,     8   }   ,  {      6     ,    6    }},
/*   11     */    {{      9    ,     9   }   ,  {      7     ,    7    }},
/*   12     */    {{     10    ,    10   }   ,  {      8     ,    8    }},
/*   13     */    {{     11    ,    11   }   ,  {      9     ,    9    }},
/*   14     */    {{     12    ,    12   }   ,  {     10     ,   10    }},
};

/* TS 36.213 Table 9.2.1-1: PUCCH resource sets before dedicated PUCCH resource configuration */
const initial_pucch_resource_t initial_pucch_resource[16] = {
/*              format           first symbol     Number of symbols        PRB offset    nb index for       set of initial CS */
/*  0  */ {  0,      12,                  2,                   0,            2,       {    0,   3,    0,    0  }   },
/*  1  */ {  0,      12,                  2,                   0,            3,       {    0,   4,    8,    0  }   },
/*  2  */ {  0,      12,                  2,                   3,            3,       {    0,   4,    8,    0  }   },
/*  3  */ {  1,      10,                  4,                   0,            2,       {    0,   6,    0,    0  }   },
/*  4  */ {  1,      10,                  4,                   0,            4,       {    0,   3,    6,    9  }   },
/*  5  */ {  1,      10,                  4,                   2,            4,       {    0,   3,    6,    9  }   },
/*  6  */ {  1,      10,                  4,                   4,            4,       {    0,   3,    6,    9  }   },
/*  7  */ {  1,       4,                 10,                   0,            2,       {    0,   6,    0,    0  }   },
/*  8  */ {  1,       4,                 10,                   0,            4,       {    0,   3,    6,    9  }   },
/*  9  */ {  1,       4,                 10,                   2,            4,       {    0,   3,    6,    9  }   },
/* 10  */ {  1,       4,                 10,                   4,            4,       {    0,   3,    6,    9  }   },
/* 11  */ {  1,       0,                 14,                   0,            2,       {    0,   6,    0,    0  }   },
/* 12  */ {  1,       0,                 14,                   0,            4,       {    0,   3,    6,    9  }   },
/* 13  */ {  1,       0,                 14,                   2,            4,       {    0,   3,    6,    9  }   },
/* 14  */ {  1,       0,                 14,                   4,            4,       {    0,   3,    6,    9  }   },
/* 15  */ {  1,       0,                 14,                   0,            4,       {    0,   3,    6,    9  }   },
};


int get_rnti_type(NR_UE_MAC_INST_t *mac, uint16_t rnti){

    RA_config_t *ra = &mac->ra;
    int rnti_type;

    if (rnti == ra->ra_rnti) {
      rnti_type = NR_RNTI_RA;
    } else if (rnti == ra->t_crnti && (ra->ra_state == WAIT_RAR || ra->ra_state == WAIT_CONTENTION_RESOLUTION) ) {
      rnti_type = NR_RNTI_TC;
    } else if (rnti == mac->crnti) {
      rnti_type = NR_RNTI_C;
    } else if (rnti == 0xFFFE) {
      rnti_type = NR_RNTI_P;
    } else if (rnti == 0xFFFF) {
      rnti_type = NR_RNTI_SI;
    } else {
      AssertFatal(1 == 0, "In %s: Not identified/handled rnti %d \n", __FUNCTION__, rnti);
    }

    LOG_D(MAC, "In %s: returning rnti_type %s \n", __FUNCTION__, rnti_types[rnti_type]);

    return rnti_type;

}


int8_t nr_ue_decode_mib(module_id_t module_id,
                        int cc_id,
                        uint8_t gNB_index,
                        uint8_t extra_bits,	//	8bits 38.212 c7.1.1
                        uint32_t ssb_length,
                        uint32_t ssb_index,
                        void *pduP,
                        uint16_t ssb_start_subcarrier,
                        uint16_t cell_id)
{
  LOG_D(MAC,"[L2][MAC] decode mib\n");

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  mac->physCellId = cell_id;

  nr_mac_rrc_data_ind_ue( module_id, cc_id, gNB_index, 0, 0, 0, NR_BCCH_BCH, (uint8_t *) pduP, 3 );    //  fixed 3 bytes MIB PDU
    
  AssertFatal(mac->mib != NULL, "nr_ue_decode_mib() mac->mib == NULL\n");
  //if(mac->mib != NULL){
  uint16_t frame = (mac->mib->systemFrameNumber.buf[0] >> mac->mib->systemFrameNumber.bits_unused);
  uint16_t frame_number_4lsb = 0;

  for (int i=0; i<4; i++)
    frame_number_4lsb |= ((extra_bits>>i)&1)<<(3-i);

  uint8_t ssb_subcarrier_offset_msb = ( extra_bits >> 5 ) & 0x1;    //	extra bits[5]
  uint8_t ssb_subcarrier_offset = (uint8_t)mac->mib->ssb_SubcarrierOffset;

  frame = frame << 4;
  frame = frame | frame_number_4lsb;
  if(ssb_length == 64){
    mac->frequency_range = FR2;
    for (int i=0; i<3; i++)
      ssb_index += (((extra_bits>>(7-i))&0x01)<<(3+i));
  }else{
    mac->frequency_range = FR1;
    if(ssb_subcarrier_offset_msb){
      ssb_subcarrier_offset = ssb_subcarrier_offset | 0x10;
    }
  }

#ifdef DEBUG_MIB
  uint8_t half_frame_bit = ( extra_bits >> 4 ) & 0x1; //	extra bits[4]
  LOG_I(MAC,"system frame number(6 MSB bits): %d\n",  mac->mib->systemFrameNumber.buf[0]);
  LOG_I(MAC,"system frame number(with LSB): %d\n", (int)frame);
  LOG_I(MAC,"subcarrier spacing (0=15or60, 1=30or120): %d\n", (int)mac->mib->subCarrierSpacingCommon);
  LOG_I(MAC,"ssb carrier offset(with MSB):  %d\n", (int)ssb_subcarrier_offset);
  LOG_I(MAC,"dmrs type A position (0=pos2,1=pos3): %d\n", (int)mac->mib->dmrs_TypeA_Position);
  LOG_I(MAC,"controlResourceSetZero: %d\n", (int)mac->mib->pdcch_ConfigSIB1.controlResourceSetZero);
  LOG_I(MAC,"searchSpaceZero: %d\n", (int)mac->mib->pdcch_ConfigSIB1.searchSpaceZero);
  LOG_I(MAC,"cell barred (0=barred,1=notBarred): %d\n", (int)mac->mib->cellBarred);
  LOG_I(MAC,"intra frequency reselection (0=allowed,1=notAllowed): %d\n", (int)mac->mib->intraFreqReselection);
  LOG_I(MAC,"half frame bit(extra bits):    %d\n", (int)half_frame_bit);
  LOG_I(MAC,"ssb index(extra bits):         %d\n", (int)ssb_index);
#endif

  //storing ssb index in the mac structure
  mac->mib_ssb = ssb_index;

  if (get_softmodem_params()->sa == 1) {

    // TODO these values shouldn't be taken from SCC in SA
    uint8_t scs_ssb = get_softmodem_params()->numerology;
    uint32_t band   = get_softmodem_params()->band;
    uint16_t ssb_start_symbol = get_ssb_start_symbol(band,scs_ssb,ssb_index);
    uint16_t ssb_offset_point_a = (ssb_start_subcarrier - ssb_subcarrier_offset)/12;

    get_type0_PDCCH_CSS_config_parameters(&mac->type0_PDCCH_CSS_config,
                                          frame,
                                          mac->mib,
                                          nr_slots_per_frame[scs_ssb],
                                          ssb_subcarrier_offset,
                                          ssb_start_symbol,
                                          scs_ssb,
                                          mac->frequency_range,
                                          ssb_index,
                                          ssb_offset_point_a);


    mac->type0_pdcch_ss_mux_pattern = mac->type0_PDCCH_CSS_config.type0_pdcch_ss_mux_pattern;
    mac->type0_pdcch_ss_sfn_c = mac->type0_PDCCH_CSS_config.sfn_c;
    mac->type0_pdcch_ss_n_c = mac->type0_PDCCH_CSS_config.n_c;
  }

  mac->dl_config_request.sfn = mac->type0_PDCCH_CSS_config.frame;
  mac->dl_config_request.slot = (ssb_index>>1) + ((ssb_index>>4)<<1); // not valid for 240kHz SCS

  return 0;
}

int8_t nr_ue_decode_BCCH_DL_SCH(module_id_t module_id,
                                int cc_id,
                                unsigned int gNB_index,
                                uint8_t ack_nack,
                                uint8_t *pduP,
                                uint32_t pdu_len) {
  if(ack_nack) {
    LOG_D(NR_MAC, "Decoding NR-BCCH-DL-SCH-Message (SIB1 or SI)\n");
    nr_mac_rrc_data_ind_ue(module_id, cc_id, gNB_index, 0, 0, 0, NR_BCCH_DL_SCH, (uint8_t *) pduP, pdu_len);
  }
  else
    LOG_E(NR_MAC, "Got NACK on NR-BCCH-DL-SCH-Message (SIB1 or SI)\n");
  return 0;
}

//  TODO: change to UE parameter, scs: 15KHz, slot duration: 1ms
uint32_t get_ssb_frame(uint32_t test){
  return test;
}

/*
 * This code contains all the functions needed to process all dci fields.
 * These tables and functions are going to be called by function nr_ue_process_dci
 */
int8_t nr_ue_process_dci_freq_dom_resource_assignment(nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu,
						      fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu,
						      uint16_t n_RB_ULBWP,
						      uint16_t n_RB_DLBWP,
						      uint16_t riv
						      ){

  /*
   * TS 38.214 subclause 5.1.2.2 Resource allocation in frequency domain (downlink)
   * when the scheduling grant is received with DCI format 1_0, then downlink resource allocation type 1 is used
   */
  if(dlsch_config_pdu != NULL){

    /*
     * TS 38.214 subclause 5.1.2.2.1 Downlink resource allocation type 0
     */
    /*
     * TS 38.214 subclause 5.1.2.2.2 Downlink resource allocation type 1
     */
    dlsch_config_pdu->number_rbs = NRRIV2BW(riv,n_RB_DLBWP);
    dlsch_config_pdu->start_rb   = NRRIV2PRBOFFSET(riv,n_RB_DLBWP);

    // Sanity check in case a false or erroneous DCI is received
    if ((dlsch_config_pdu->number_rbs < 1 ) || (dlsch_config_pdu->number_rbs > n_RB_DLBWP - dlsch_config_pdu->start_rb)) {
      // DCI is invalid!
      LOG_W(MAC, "Frequency domain assignment values are invalid! #RBs: %d, Start RB: %d, n_RB_DLBWP: %d \n", dlsch_config_pdu->number_rbs, dlsch_config_pdu->start_rb, n_RB_DLBWP);
      return -1;
    }

    LOG_D(MAC,"DLSCH riv = %i\n", riv);
    LOG_D(MAC,"DLSCH n_RB_DLBWP = %i\n", n_RB_DLBWP);
    LOG_D(MAC,"DLSCH number_rbs = %i\n", dlsch_config_pdu->number_rbs);
    LOG_D(MAC,"DLSCH start_rb = %i\n", dlsch_config_pdu->start_rb);

  }
  if(pusch_config_pdu != NULL){
    /*
     * TS 38.214 subclause 6.1.2.2 Resource allocation in frequency domain (uplink)
     */
    /*
     * TS 38.214 subclause 6.1.2.2.1 Uplink resource allocation type 0
     */
    /*
     * TS 38.214 subclause 6.1.2.2.2 Uplink resource allocation type 1
     */

    pusch_config_pdu->rb_size  = NRRIV2BW(riv,n_RB_ULBWP);
    pusch_config_pdu->rb_start = NRRIV2PRBOFFSET(riv,n_RB_ULBWP);

    // Sanity check in case a false or erroneous DCI is received
    if ((pusch_config_pdu->rb_size < 1) || (pusch_config_pdu->rb_size > n_RB_ULBWP - pusch_config_pdu->rb_start)) {
      // DCI is invalid!
      LOG_W(MAC, "Frequency domain assignment values are invalid! #RBs: %d, Start RB: %d, n_RB_ULBWP: %d \n",pusch_config_pdu->rb_size, pusch_config_pdu->rb_start, n_RB_ULBWP);
      return -1;
    }
    LOG_D(MAC,"ULSCH riv = %i\n", riv);
    LOG_D(MAC,"ULSCH n_RB_DLBWP = %i\n", n_RB_ULBWP);
    LOG_D(MAC,"ULSCH number_rbs = %i\n", pusch_config_pdu->rb_size);
    LOG_D(MAC,"ULSCH start_rb = %i\n", pusch_config_pdu->rb_start);
  }
  return 0;
}

int8_t nr_ue_process_dci_time_dom_resource_assignment(NR_UE_MAC_INST_t *mac,
						      nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu,
						      fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu,
						      uint8_t time_domain_ind,
						      bool use_default
						      ){
  int dmrs_typeA_pos = (mac->scc != NULL) ? mac->scc->dmrs_TypeA_Position : mac->mib->dmrs_TypeA_Position;

//  uint8_t k_offset=0;
  uint8_t sliv_S=0;
  uint8_t sliv_L=0;
  uint8_t mu_pusch = 1;

  // definition table j Table 6.1.2.1.1-4
  uint8_t j = (mu_pusch==3)?3:(mu_pusch==2)?2:1;
  uint8_t table_6_1_2_1_1_2_time_dom_res_alloc_A[16][3]={ // for PUSCH from TS 38.214 subclause 6.1.2.1.1
    {j,  0,14}, // row index 1
    {j,  0,12}, // row index 2
    {j,  0,10}, // row index 3
    {j,  2,10}, // row index 4
    {j,  4,10}, // row index 5
    {j,  4,8},  // row index 6
    {j,  4,6},  // row index 7
    {j+1,0,14}, // row index 8
    {j+1,0,12}, // row index 9
    {j+1,0,10}, // row index 10
    {j+2,0,14}, // row index 11
    {j+2,0,12}, // row index 12
    {j+2,0,10}, // row index 13
    {j,  8,6},  // row index 14
    {j+3,0,14}, // row index 15
    {j+3,0,10}  // row index 16
  };
  /*uint8_t table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[16][3]={ // for PUSCH from TS 38.214 subclause 6.1.2.1.1
    {j,  0,8},  // row index 1
    {j,  0,12}, // row index 2
    {j,  0,10}, // row index 3
    {j,  2,10}, // row index 4
    {j,  4,4},  // row index 5
    {j,  4,8},  // row index 6
    {j,  4,6},  // row index 7
    {j+1,0,8},  // row index 8
    {j+1,0,12}, // row index 9
    {j+1,0,10}, // row index 10
    {j+2,0,6},  // row index 11
    {j+2,0,12}, // row index 12
    {j+2,0,10}, // row index 13
    {j,  8,4},  // row index 14
    {j+3,0,8},  // row index 15
    {j+3,0,10}  // row index 16
    };*/

  /*
   * TS 38.214 subclause 5.1.2.1 Resource allocation in time domain (downlink)
   */
  if(dlsch_config_pdu != NULL){
    NR_PDSCH_TimeDomainResourceAllocationList_t *pdsch_TimeDomainAllocationList = NULL;
    if (mac->DLbwp[0] &&
        mac->DLbwp[0]->bwp_Dedicated &&
        mac->DLbwp[0]->bwp_Dedicated->pdsch_Config &&
        mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->pdsch_TimeDomainAllocationList)
      pdsch_TimeDomainAllocationList = mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->pdsch_TimeDomainAllocationList->choice.setup;
    else if (mac->DLbwp[0] && mac->DLbwp[0]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList)
      pdsch_TimeDomainAllocationList = mac->DLbwp[0]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    else if (mac->scc_SIB && mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.pdsch_ConfigCommon->choice.setup)
      pdsch_TimeDomainAllocationList = mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;

    if (pdsch_TimeDomainAllocationList && use_default==false) {

      if (time_domain_ind >= pdsch_TimeDomainAllocationList->list.count) {
        LOG_E(MAC, "time_domain_ind %d >= pdsch->TimeDomainAllocationList->list.count %d\n",
              time_domain_ind, pdsch_TimeDomainAllocationList->list.count);
        dlsch_config_pdu->start_symbol   = 0;
        dlsch_config_pdu->number_symbols = 0;
        return -1;
      }

      int startSymbolAndLength = pdsch_TimeDomainAllocationList->list.array[time_domain_ind]->startSymbolAndLength;
      int S,L;
      SLIV2SL(startSymbolAndLength,&S,&L);
      dlsch_config_pdu->start_symbol=S;
      dlsch_config_pdu->number_symbols=L;

      LOG_D(MAC,"SLIV = %i\n", startSymbolAndLength);
      LOG_D(MAC,"start_symbol = %i\n", dlsch_config_pdu->start_symbol);
      LOG_D(MAC,"number_symbols = %i\n", dlsch_config_pdu->number_symbols);

    }
    else {// Default configuration from tables
//      k_offset = table_5_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][0];

      if(dmrs_typeA_pos == 0) {
        sliv_S = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos2[time_domain_ind][1];
        sliv_L = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos2[time_domain_ind][2];
      } else {
        sliv_S = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos3[time_domain_ind][1];
        sliv_L = table_5_1_2_1_1_2_time_dom_res_alloc_A_dmrs_typeA_pos3[time_domain_ind][2];
      }

      // k_offset = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      // k_offset = table_5_1_2_1_1_4_time_dom_res_alloc_B[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_5_1_2_1_1_4_time_dom_res_alloc_B[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_5_1_2_1_1_4_time_dom_res_alloc_B[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      // k_offset = table_5_1_2_1_1_5_time_dom_res_alloc_C[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_5_1_2_1_1_5_time_dom_res_alloc_C[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_5_1_2_1_1_5_time_dom_res_alloc_C[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      dlsch_config_pdu->number_symbols = sliv_L;
      dlsch_config_pdu->start_symbol = sliv_S;
    }
  }	/*
	 * TS 38.214 subclause 6.1.2.1 Resource allocation in time domain (uplink)
	 */
  if(pusch_config_pdu != NULL){
    NR_PUSCH_TimeDomainResourceAllocationList_t *pusch_TimeDomainAllocationList = NULL;
    if (mac->ULbwp[0] &&
        mac->ULbwp[0]->bwp_Dedicated &&
        mac->ULbwp[0]->bwp_Dedicated->pusch_Config &&
        mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup &&
        mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup->pusch_TimeDomainAllocationList) {
      pusch_TimeDomainAllocationList = mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup->pusch_TimeDomainAllocationList->choice.setup;
    }
    else if (mac->ULbwp[0] &&
      mac->ULbwp[0]->bwp_Common &&
      mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon &&
      mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon->choice.setup &&
      mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList) {
      pusch_TimeDomainAllocationList = mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
    }
    else pusch_TimeDomainAllocationList = mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;

    if (pusch_TimeDomainAllocationList && use_default==false) {
      if (time_domain_ind >= pusch_TimeDomainAllocationList->list.count) {
        LOG_E(MAC, "time_domain_ind %d >= pusch->TimeDomainAllocationList->list.count %d\n",
              time_domain_ind, pusch_TimeDomainAllocationList->list.count);
        pusch_config_pdu->start_symbol_index=0;
        pusch_config_pdu->nr_of_symbols=0;
        return -1;
      }
      
      LOG_D(NR_MAC,"Filling Time-Domain Allocation from pusch_TimeDomainAllocationList\n");
      int startSymbolAndLength = pusch_TimeDomainAllocationList->list.array[time_domain_ind]->startSymbolAndLength;
      int S,L;
      SLIV2SL(startSymbolAndLength,&S,&L);
      pusch_config_pdu->start_symbol_index=S;
      pusch_config_pdu->nr_of_symbols=L;
    }
    else {
      LOG_D(NR_MAC,"Filling Time-Domain Allocation from tables\n");
//      k_offset = table_6_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][0];
      sliv_S   = table_6_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind][1];
      sliv_L   = table_6_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind][2];
      // k_offset = table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      pusch_config_pdu->nr_of_symbols = sliv_L;
      pusch_config_pdu->start_symbol_index = sliv_S;
    }
    LOG_D(NR_MAC,"start_symbol = %i\n", pusch_config_pdu->start_symbol_index);
    LOG_D(NR_MAC,"number_symbols = %i\n", pusch_config_pdu->nr_of_symbols);
  }
  return 0;
}

int nr_ue_process_dci_indication_pdu(module_id_t module_id,int cc_id, int gNB_index, frame_t frame, int slot, fapi_nr_dci_indication_pdu_t *dci) {

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  dci_pdu_rel15_t *def_dci_pdu_rel15 = &mac->def_dci_pdu_rel15[dci->dci_format];

  LOG_D(MAC,"Received dci indication (rnti %x,dci format %d,n_CCE %d,payloadSize %d,payload %llx)\n",
	dci->rnti,dci->dci_format,dci->n_CCE,dci->payloadSize,*(unsigned long long*)dci->payloadBits);
  int8_t ret = nr_extract_dci_info(mac, dci->dci_format, dci->payloadSize, dci->rnti, (uint64_t *)dci->payloadBits, def_dci_pdu_rel15);
  if ((ret&1) == 1) return -1;
  else if (ret == 2) dci->dci_format = NR_UL_DCI_FORMAT_0_0;
  int8_t ret_proc = nr_ue_process_dci(module_id, cc_id, gNB_index, frame, slot, def_dci_pdu_rel15, dci);
  memset(def_dci_pdu_rel15, 0, sizeof(dci_pdu_rel15_t));
  return ret_proc;
}

int8_t nr_ue_process_dci(module_id_t module_id, int cc_id, uint8_t gNB_index, frame_t frame, int slot, dci_pdu_rel15_t *dci, fapi_nr_dci_indication_pdu_t *dci_ind) {

  uint16_t rnti = dci_ind->rnti;
  uint8_t dci_format = dci_ind->dci_format;
  int ret = 0;
  int pucch_res_set_cnt = 0, valid = 0;
  frame_t frame_tx = 0;
  int slot_tx = 0;
  bool valid_ptrs_setup = 0;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  RA_config_t *ra = &mac->ra;
  fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
  uint8_t is_Msg3 = 0;

  uint16_t n_RB_DLBWP;
  if (mac->DLbwp[0]) n_RB_DLBWP = NRRIV2BW(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  else if (mac->scc_SIB) n_RB_DLBWP =  NRRIV2BW(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
  else n_RB_DLBWP = mac->type0_PDCCH_CSS_config.num_rbs;

  LOG_D(MAC, "In %s: Processing received DCI format %s (DL BWP %d)\n", __FUNCTION__, dci_formats[dci_format], n_RB_DLBWP);

  switch(dci_format){
  case NR_UL_DCI_FORMAT_0_0: {
    /*
     *  with CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI
     *    0  IDENTIFIER_DCI_FORMATS:
     *    10 FREQ_DOM_RESOURCE_ASSIGNMENT_UL: PUSCH hopping with resource allocation type 1 not considered
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 6.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    17 FREQ_HOPPING_FLAG: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    25 NDI:
     *    26 RV:
     *    27 HARQ_PROCESS_NUMBER:
     *    32 TPC_PUSCH:
     *    49 PADDING_NR_DCI: (Note 2) If DCI format 0_0 is monitored in common search space
     *    50 SUL_IND_0_0:
     */
    // Calculate the slot in which ULSCH should be scheduled. This is current slot + K2,
    // where K2 is the offset between the slot in which UL DCI is received and the slot
    // in which ULSCH should be scheduled. K2 is configured in RRC configuration.  
    // todo:
    // - SUL_IND_0_0

    // Schedule PUSCH
    ret = nr_ue_pusch_scheduler(mac, is_Msg3, frame, slot, &frame_tx, &slot_tx, dci->time_domain_assignment.val);

    if (ret != -1){

      // Get UL config request corresponding slot_tx
      fapi_nr_ul_config_request_t *ul_config = get_ul_config_request(mac, slot_tx);

      if (!ul_config) {
        LOG_W(MAC, "In %s: ul_config request is NULL. Probably due to unexpected UL DCI in frame.slot %d.%d. Ignoring DCI!\n", __FUNCTION__, frame, slot);
        return -1;
      }

      AssertFatal(ul_config->number_pdus<FAPI_NR_UL_CONFIG_LIST_NUM, "ul_config->number_pdus %d out of bounds\n",ul_config->number_pdus);
      nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu = &ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu;

      fill_ul_config(ul_config, frame_tx, slot_tx, FAPI_NR_UL_CONFIG_TYPE_PUSCH);

      // Config PUSCH PDU
      ret = nr_config_pusch_pdu(mac, pusch_config_pdu, dci, NULL, rnti, &dci_format);

    }
    
    break;
  }

  case NR_UL_DCI_FORMAT_0_1: {
    /*
     *  with CRC scrambled by C-RNTI or CS-RNTI or SP-CSI-RNTI or new-RNTI
     *    0  IDENTIFIER_DCI_FORMATS:
     *    1  CARRIER_IND
     *    2  SUL_IND_0_1
     *    7  BANDWIDTH_PART_IND
     *    10 FREQ_DOM_RESOURCE_ASSIGNMENT_UL: PUSCH hopping with resource allocation type 1 not considered
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 6.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    17 FREQ_HOPPING_FLAG: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    25 NDI:
     *    26 RV:
     *    27 HARQ_PROCESS_NUMBER:
     *    29 FIRST_DAI
     *    30 SECOND_DAI
     *    32 TPC_PUSCH:
     *    36 SRS_RESOURCE_IND:
     *    37 PRECOD_NBR_LAYERS:
     *    38 ANTENNA_PORTS:
     *    40 SRS_REQUEST:
     *    42 CSI_REQUEST:
     *    43 CBGTI
     *    45 PTRS_DMRS
     *    46 BETA_OFFSET_IND
     *    47 DMRS_SEQ_INI
     *    48 UL_SCH_IND
     *    49 PADDING_NR_DCI: (Note 2) If DCI format 0_0 is monitored in common search space
     */
    // TODO: 
    // - FIRST_DAI
    // - SECOND_DAI
    // - SRS_RESOURCE_IND

    // Schedule PUSCH
    ret = nr_ue_pusch_scheduler(mac, is_Msg3, frame, slot, &frame_tx, &slot_tx, dci->time_domain_assignment.val);

    if (ret != -1){

      // Get UL config request corresponding slot_tx
      fapi_nr_ul_config_request_t *ul_config = get_ul_config_request(mac, slot_tx);

      if (!ul_config) {
        LOG_W(MAC, "In %s: ul_config request is NULL. Probably due to unexpected UL DCI in frame.slot %d.%d. Ignoring DCI!\n", __FUNCTION__, frame, slot);
        return -1;
      }

      nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu = &ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu;

      fill_ul_config(ul_config, frame_tx, slot_tx, FAPI_NR_UL_CONFIG_TYPE_PUSCH);

      // Config PUSCH PDU
      ret = nr_config_pusch_pdu(mac, pusch_config_pdu, dci, NULL, rnti, &dci_format);

    }
    break;
  }

  case NR_DL_DCI_FORMAT_1_0: {
    /*
     *  with CRC scrambled by C-RNTI or CS-RNTI or new-RNTI
     *    0  IDENTIFIER_DCI_FORMATS:
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    25 NDI:
     *    26 RV:
     *    27 HARQ_PROCESS_NUMBER:
     *    28 DAI_: For format1_1: 4 if more than one serving cell are configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 MSB bits are the counter DAI and the 2 LSB bits are the total DAI
     *    33 TPC_PUCCH:
     *    34 PUCCH_RESOURCE_IND:
     *    35 PDSCH_TO_HARQ_FEEDBACK_TIME_IND:
     *    55 RESERVED_NR_DCI
     *  with CRC scrambled by P-RNTI
     *    8  SHORT_MESSAGE_IND
     *    9  SHORT_MESSAGES
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    31 TB_SCALING
     *    55 RESERVED_NR_DCI
     *  with CRC scrambled by SI-RNTI
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    26 RV:
     *    55 RESERVED_NR_DCI
     *  with CRC scrambled by RA-RNTI
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    31 TB_SCALING
     *    55 RESERVED_NR_DCI
     *  with CRC scrambled by TC-RNTI
     *    0  IDENTIFIER_DCI_FORMATS:
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    24 MCS:
     *    25 NDI:
     *    26 RV:
     *    27 HARQ_PROCESS_NUMBER:
     *    28 DAI_: For format1_1: 4 if more than one serving cell are configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 MSB bits are the counter DAI and the 2 LSB bits are the total DAI
     *    33 TPC_PUCCH:
     */

    dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.rnti = rnti;
    fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu_1_0 = &dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.dlsch_config_rel15;
    NR_PDSCH_Config_t *pdsch_config= (mac->DLbwp[0]) ? mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup : NULL;
    uint16_t BWPSize = n_RB_DLBWP;

    if(rnti == SI_RNTI) {
      dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_SI_DLSCH;
      dlsch_config_pdu_1_0->BWPSize = mac->type0_PDCCH_CSS_config.num_rbs;
      dlsch_config_pdu_1_0->BWPStart = mac->type0_PDCCH_CSS_config.cset_start_rb;
      dlsch_config_pdu_1_0->SubcarrierSpacing = mac->mib->subCarrierSpacingCommon;
      if (pdsch_config) pdsch_config->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = NULL; // For PDSCH with mapping type A, the UE shall assume dmrs-AdditionalPosition='pos2'
      BWPSize = dlsch_config_pdu_1_0->BWPSize;
    } else {
      if (ra->RA_window_cnt >= 0 && rnti == ra->ra_rnti){
        dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_RA_DLSCH;
      } else {
        dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
      }
      if( (ra->RA_window_cnt >= 0 && rnti == ra->ra_rnti) || (rnti == ra->t_crnti) ) {
        if (mac->scc == NULL) {
          dlsch_config_pdu_1_0->BWPSize = NRRIV2BW(mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
          dlsch_config_pdu_1_0->BWPStart = NRRIV2PRBOFFSET(mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
        }
        else {
          dlsch_config_pdu_1_0->BWPSize = NRRIV2BW(mac->scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
          dlsch_config_pdu_1_0->BWPStart = NRRIV2PRBOFFSET(mac->scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
        }
        if (!get_softmodem_params()->sa) { // NSA mode is not using the Initial BWP
          dlsch_config_pdu_1_0->BWPStart = NRRIV2PRBOFFSET(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
          pdsch_config = mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup;
          BWPSize = dlsch_config_pdu_1_0->BWPSize;
        }
      } else if (mac->DLbwp[0]) {
        dlsch_config_pdu_1_0->BWPSize = NRRIV2BW(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
        dlsch_config_pdu_1_0->BWPStart = NRRIV2PRBOFFSET(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
        dlsch_config_pdu_1_0->SubcarrierSpacing = mac->DLbwp[0]->bwp_Common->genericParameters.subcarrierSpacing;
        pdsch_config = mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup;
      } else if (mac->scc_SIB) {
        dlsch_config_pdu_1_0->BWPSize = NRRIV2BW(mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
        dlsch_config_pdu_1_0->BWPStart = NRRIV2PRBOFFSET(mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
        dlsch_config_pdu_1_0->SubcarrierSpacing = mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.genericParameters.subcarrierSpacing;
        pdsch_config = NULL;
      }
    }

    /* IDENTIFIER_DCI_FORMATS */
    /* FREQ_DOM_RESOURCE_ASSIGNMENT_DL */
    if (nr_ue_process_dci_freq_dom_resource_assignment(NULL,dlsch_config_pdu_1_0,0,BWPSize,dci->frequency_domain_assignment.val) < 0) {
      LOG_W(MAC, "[%d.%d] Invalid frequency_domain_assignment. Possibly due to false DCI. Ignoring DCI!\n", frame, slot);
      return -1;
    }
    /* TIME_DOM_RESOURCE_ASSIGNMENT */
    if (nr_ue_process_dci_time_dom_resource_assignment(mac,NULL,dlsch_config_pdu_1_0,dci->time_domain_assignment.val,rnti==SI_RNTI) < 0) {
      LOG_W(MAC, "[%d.%d] Invalid time_domain_assignment. Possibly due to false DCI. Ignoring DCI!\n", frame, slot);
      return -1;
    }

    NR_PDSCH_TimeDomainResourceAllocationList_t *pdsch_TimeDomainAllocationList = NULL;
    if (mac->DLbwp[0] &&
        mac->DLbwp[0]->bwp_Dedicated &&
        mac->DLbwp[0]->bwp_Dedicated->pdsch_Config &&
        mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->pdsch_TimeDomainAllocationList)
      pdsch_TimeDomainAllocationList = mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->pdsch_TimeDomainAllocationList->choice.setup;
    else if (mac->DLbwp[0] && mac->DLbwp[0]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList)
      pdsch_TimeDomainAllocationList = mac->DLbwp[0]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    else if (mac->scc_SIB && mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.pdsch_ConfigCommon->choice.setup)
      pdsch_TimeDomainAllocationList = mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;

    int mappingtype = pdsch_TimeDomainAllocationList ? pdsch_TimeDomainAllocationList->list.array[dci->time_domain_assignment.val]->mappingType : ((dlsch_config_pdu_1_0->start_symbol <= 3)? typeA: typeB);

    /* dmrs symbol positions*/
    dlsch_config_pdu_1_0->dlDmrsSymbPos = fill_dmrs_mask(pdsch_config,
                                                         mac->mib->dmrs_TypeA_Position,
                                                         dlsch_config_pdu_1_0->number_symbols,
                                                         dlsch_config_pdu_1_0->start_symbol,
                                                         mappingtype);
    dlsch_config_pdu_1_0->dmrsConfigType = (mac->DLbwp[0] != NULL) ? (mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NULL ? 0 : 1) : 0;
    /* number of DM-RS CDM groups without data according to subclause 5.1.6.2 of 3GPP TS 38.214 version 15.9.0 Release 15 */
    if (dlsch_config_pdu_1_0->number_symbols == 2)
      dlsch_config_pdu_1_0->n_dmrs_cdm_groups = 1;
    else
      dlsch_config_pdu_1_0->n_dmrs_cdm_groups = 2;
    /* VRB_TO_PRB_MAPPING */
    dlsch_config_pdu_1_0->vrb_to_prb_mapping = (dci->vrb_to_prb_mapping.val == 0) ? vrb_to_prb_mapping_non_interleaved:vrb_to_prb_mapping_interleaved;
    /* MCS TABLE INDEX */
    dlsch_config_pdu_1_0->mcs_table = (pdsch_config) ? ((pdsch_config->mcs_Table) ? (*pdsch_config->mcs_Table + 1) : 0) : 0;
    /* MCS */
    dlsch_config_pdu_1_0->mcs = dci->mcs;
    // Basic sanity check for MCS value to check for a false or erroneous DCI
    if (dlsch_config_pdu_1_0->mcs > 28) {
      LOG_W(MAC, "[%d.%d] MCS value %d out of bounds! Possibly due to false DCI. Ignoring DCI!\n", frame, slot, dlsch_config_pdu_1_0->mcs);
      return -1;
    }
    /* NDI (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
    dlsch_config_pdu_1_0->ndi = dci->ndi;
    /* RV (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
    dlsch_config_pdu_1_0->rv = dci->rv;
    /* HARQ_PROCESS_NUMBER (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
    dlsch_config_pdu_1_0->harq_process_nbr = dci->harq_pid;
    /* TB_SCALING (only if CRC scrambled by P-RNTI or RA-RNTI) */
    // according to TS 38.214 Table 5.1.3.2-3
    if (dci->tb_scaling == 0) dlsch_config_pdu_1_0->scaling_factor_S = 1;
    if (dci->tb_scaling == 1) dlsch_config_pdu_1_0->scaling_factor_S = 0.5;
    if (dci->tb_scaling == 2) dlsch_config_pdu_1_0->scaling_factor_S = 0.25;
    if (dci->tb_scaling == 3) dlsch_config_pdu_1_0->scaling_factor_S = 0; // value not defined in table
    /* TPC_PUCCH (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
    // according to TS 38.213 Table 7.2.1-1
    if (dci->tpc == 0) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = -1;
    if (dci->tpc == 1) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = 0;
    if (dci->tpc == 2) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = 1;
    if (dci->tpc == 3) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = 3;
    // Sanity check for pucch_resource_indicator value received to check for false DCI.
    valid = 0;
    if (mac->ULbwp[0] &&
        mac->ULbwp[0]->bwp_Dedicated &&
        mac->ULbwp[0]->bwp_Dedicated->pucch_Config &&
        mac->ULbwp[0]->bwp_Dedicated->pucch_Config->choice.setup&&
        mac->ULbwp[0]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList) {
      pucch_res_set_cnt = mac->ULbwp[0]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.count;
      for (int id = 0; id < pucch_res_set_cnt; id++) {
	if (dci->pucch_resource_indicator < mac->ULbwp[0]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.array[id]->resourceList.list.count) {
	  valid = 1;
	  break;
	}
      }
    }
    else if (mac->cg &&
             mac->cg->spCellConfig &&
             mac->cg->spCellConfig->spCellConfigDedicated &&
             mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
             mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
             mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config &&
             mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup &&
             mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList){
      pucch_res_set_cnt = mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList->list.count;
      for (int id = 0; id < pucch_res_set_cnt; id++) {
        if (dci->pucch_resource_indicator < mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList->list.array[id]->resourceList.list.count) {
          valid = 1;
          break;
        }
      }
    } else valid=1;
    if (!valid) {
      LOG_W(MAC, "[%d.%d] pucch_resource_indicator value %d is out of bounds. Possibly due to false DCI. Ignoring DCI!\n", frame, slot, dci->pucch_resource_indicator);
      return -1;
    }

   // set the harq status at MAC for feedback
   set_harq_status(mac,dci->pucch_resource_indicator,
                   dci->harq_pid,
                   dlsch_config_pdu_1_0->accumulated_delta_PUCCH,
                   1+dci->pdsch_to_harq_feedback_timing_indicator.val,
                   dci->dai[0].val,
                   dci_ind->n_CCE,dci_ind->N_CCE,
                   frame,slot);

    LOG_D(MAC,"(nr_ue_procedures.c) rnti = %x dl_config->number_pdus = %d\n",
	  dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.rnti,
	  dl_config->number_pdus);
    LOG_D(MAC,"(nr_ue_procedures.c) frequency_domain_resource_assignment=%d \t number_rbs=%d \t start_rb=%d\n",
	  dci->frequency_domain_assignment.val,
	  dlsch_config_pdu_1_0->number_rbs,
	  dlsch_config_pdu_1_0->start_rb);
    LOG_D(MAC,"(nr_ue_procedures.c) time_domain_resource_assignment=%d \t number_symbols=%d \t start_symbol=%d\n",
	  dci->time_domain_assignment.val,
	  dlsch_config_pdu_1_0->number_symbols,
	  dlsch_config_pdu_1_0->start_symbol);
    LOG_D(MAC,"(nr_ue_procedures.c) vrb_to_prb_mapping=%d \n>>> mcs=%d\n>>> ndi=%d\n>>> rv=%d\n>>> harq_process_nbr=%d\n>>> dai=%d\n>>> scaling_factor_S=%f\n>>> tpc_pucch=%d\n>>> pucch_res_ind=%d\n>>> pdsch_to_harq_feedback_time_ind=%d\n",
	  dlsch_config_pdu_1_0->vrb_to_prb_mapping,
	  dlsch_config_pdu_1_0->mcs,
	  dlsch_config_pdu_1_0->ndi,
	  dlsch_config_pdu_1_0->rv,
	  dlsch_config_pdu_1_0->harq_process_nbr,
	  dci->dai[0].val,
	  dlsch_config_pdu_1_0->scaling_factor_S,
	  dlsch_config_pdu_1_0->accumulated_delta_PUCCH,
	  dci->pucch_resource_indicator,
	  1+dci->pdsch_to_harq_feedback_timing_indicator.val);

    //	    dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP = n_RB_DLBWP;
	    
    LOG_D(MAC,"(nr_ue_procedures.c) pdu_type=%d\n\n",dl_config->dl_config_list[dl_config->number_pdus].pdu_type);
            
    dl_config->number_pdus = dl_config->number_pdus + 1;

    break;
  }

  case NR_DL_DCI_FORMAT_1_1: {
    /*
     *  with CRC scrambled by C-RNTI or CS-RNTI or new-RNTI
     *    0  IDENTIFIER_DCI_FORMATS:
     *    1  CARRIER_IND:
     *    7  BANDWIDTH_PART_IND:
     *    11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
     *    12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 5.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
     *    13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
     *    14 PRB_BUNDLING_SIZE_IND:
     *    15 RATE_MATCHING_IND:
     *    16 ZP_CSI_RS_TRIGGER:
     *    18 TB1_MCS:
     *    19 TB1_NDI:
     *    20 TB1_RV:
     *    21 TB2_MCS:
     *    22 TB2_NDI:
     *    23 TB2_RV:
     *    27 HARQ_PROCESS_NUMBER:
     *    28 DAI_: For format1_1: 4 if more than one serving cell are configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 MSB bits are the counter DAI and the 2 LSB bits are the total DAI
     *    33 TPC_PUCCH:
     *    34 PUCCH_RESOURCE_IND:
     *    35 PDSCH_TO_HARQ_FEEDBACK_TIME_IND:
     *    38 ANTENNA_PORTS:
     *    39 TCI:
     *    40 SRS_REQUEST:
     *    43 CBGTI:
     *    44 CBGFI:
     *    47 DMRS_SEQ_INI:
     */

    if (dci->bwp_indicator.val != 1) {
      LOG_W(MAC, "[%d.%d] bwp_indicator != 1! Possibly due to false DCI. Ignoring DCI!\n", frame, slot);
      return -1;
    }
    config_bwp_ue(mac, &dci->bwp_indicator.val, &dci_format);
    NR_BWP_Id_t dl_bwp_id = mac->DL_BWP_Id;
    NR_PDSCH_Config_t *pdsch_config=mac->DLbwp[dl_bwp_id-1]->bwp_Dedicated->pdsch_Config->choice.setup;

    dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
    dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.rnti = rnti;

    fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu_1_1 = &dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.dlsch_config_rel15;

    dlsch_config_pdu_1_1->BWPSize = NRRIV2BW(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    dlsch_config_pdu_1_1->BWPStart = NRRIV2PRBOFFSET(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    dlsch_config_pdu_1_1->SubcarrierSpacing = mac->DLbwp[0]->bwp_Common->genericParameters.subcarrierSpacing;

    /* IDENTIFIER_DCI_FORMATS */
    /* CARRIER_IND */
    /* BANDWIDTH_PART_IND */
    //    dlsch_config_pdu_1_1->bandwidth_part_ind = dci->bandwidth_part_ind;
    /* FREQ_DOM_RESOURCE_ASSIGNMENT_DL */
    if (nr_ue_process_dci_freq_dom_resource_assignment(NULL,dlsch_config_pdu_1_1,0,n_RB_DLBWP,dci->frequency_domain_assignment.val) < 0) {
      LOG_W(MAC, "[%d.%d] Invalid frequency_domain_assignment. Possibly due to false DCI. Ignoring DCI!\n", frame, slot);
      return -1;
    }
    /* TIME_DOM_RESOURCE_ASSIGNMENT */
    if (nr_ue_process_dci_time_dom_resource_assignment(mac,NULL,dlsch_config_pdu_1_1,dci->time_domain_assignment.val,false) < 0) {
      LOG_W(MAC, "[%d.%d] Invalid time_domain_assignment. Possibly due to false DCI. Ignoring DCI!\n", frame, slot);
      return -1;
    }

    NR_PDSCH_TimeDomainResourceAllocationList_t *pdsch_TimeDomainAllocationList = NULL;
    if (mac->DLbwp[0] &&
        mac->DLbwp[0]->bwp_Dedicated &&
        mac->DLbwp[0]->bwp_Dedicated->pdsch_Config &&
        mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->pdsch_TimeDomainAllocationList)
      pdsch_TimeDomainAllocationList = mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->pdsch_TimeDomainAllocationList->choice.setup;
    else if (mac->DLbwp[0] && mac->DLbwp[0]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList)
      pdsch_TimeDomainAllocationList = mac->DLbwp[0]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    else if (mac->scc_SIB && mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.pdsch_ConfigCommon->choice.setup)
      pdsch_TimeDomainAllocationList = mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;

    int mappingtype = pdsch_TimeDomainAllocationList ? pdsch_TimeDomainAllocationList->list.array[dci->time_domain_assignment.val]->mappingType : ((dlsch_config_pdu_1_1->start_symbol <= 3)? typeA: typeB);

    /* dmrs symbol positions*/
    dlsch_config_pdu_1_1->dlDmrsSymbPos = fill_dmrs_mask(pdsch_config,
							 mac->scc->dmrs_TypeA_Position,
							 dlsch_config_pdu_1_1->number_symbols,
               dlsch_config_pdu_1_1->start_symbol,
               mappingtype);
    dlsch_config_pdu_1_1->dmrsConfigType = mac->DLbwp[dl_bwp_id-1]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NULL ? NFAPI_NR_DMRS_TYPE1 : NFAPI_NR_DMRS_TYPE2;
    /* TODO: fix number of DM-RS CDM groups without data according to subclause 5.1.6.2 of 3GPP TS 38.214,
             using tables 7.3.1.2.2-1, 7.3.1.2.2-2, 7.3.1.2.2-3, 7.3.1.2.2-4 of 3GPP TS 38.212 */
    dlsch_config_pdu_1_1->n_dmrs_cdm_groups = 1;
    /* VRB_TO_PRB_MAPPING */
    if ((pdsch_config->resourceAllocation == 1) && (pdsch_config->vrb_ToPRB_Interleaver != NULL))
      dlsch_config_pdu_1_1->vrb_to_prb_mapping = (dci->vrb_to_prb_mapping.val == 0) ? vrb_to_prb_mapping_non_interleaved:vrb_to_prb_mapping_interleaved;
    /* PRB_BUNDLING_SIZE_IND */
    dlsch_config_pdu_1_1->prb_bundling_size_ind = dci->prb_bundling_size_indicator.val;
    /* RATE_MATCHING_IND */
    dlsch_config_pdu_1_1->rate_matching_ind = dci->rate_matching_indicator.val;
    /* ZP_CSI_RS_TRIGGER */
    dlsch_config_pdu_1_1->zp_csi_rs_trigger = dci->zp_csi_rs_trigger.val;
    /* MCS (for transport block 1)*/
    dlsch_config_pdu_1_1->mcs = dci->mcs;
    // Basic sanity check for MCS value to check for a false or erroneous DCI
    if (dlsch_config_pdu_1_1->mcs > 28) {
      LOG_W(MAC, "[%d.%d] MCS value %d out of bounds! Possibly due to false DCI. Ignoring DCI!\n", frame, slot, dlsch_config_pdu_1_1->mcs);
      return -1;
    }
    /* NDI (for transport block 1)*/
    dlsch_config_pdu_1_1->ndi = dci->ndi;
    /* RV (for transport block 1)*/
    dlsch_config_pdu_1_1->rv = dci->rv;
    /* MCS (for transport block 2)*/
    dlsch_config_pdu_1_1->tb2_mcs = dci->mcs2.val;
    // Basic sanity check for MCS value to check for a false or erroneous DCI
    if (dlsch_config_pdu_1_1->tb2_mcs > 28) {
      LOG_W(MAC, "[%d.%d] MCS value %d out of bounds! Possibly due to false DCI. Ignoring DCI!\n", frame, slot, dlsch_config_pdu_1_1->tb2_mcs);
      return -1;
    }
    /* NDI (for transport block 2)*/
    dlsch_config_pdu_1_1->tb2_ndi = dci->ndi2.val;
    /* RV (for transport block 2)*/
    dlsch_config_pdu_1_1->tb2_rv = dci->rv2.val;
    /* HARQ_PROCESS_NUMBER */
    dlsch_config_pdu_1_1->harq_process_nbr = dci->harq_pid;
    /* TPC_PUCCH */
    // according to TS 38.213 Table 7.2.1-1
    if (dci->tpc == 0) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = -1;
    if (dci->tpc == 1) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = 0;
    if (dci->tpc == 2) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = 1;
    if (dci->tpc == 3) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = 3;

    // Sanity check for pucch_resource_indicator value received to check for false DCI.
    valid = 0;
    pucch_res_set_cnt = mac->ULbwp[0]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.count;
    for (int id = 0; id < pucch_res_set_cnt; id++) {
      if (dci->pucch_resource_indicator < mac->ULbwp[0]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.array[id]->resourceList.list.count) {
        valid = 1;
        break;
      }
    }
    if (!valid) {
      LOG_W(MAC, "[%d.%d] pucch_resource_indicator value %d is out of bounds. Possibly due to false DCI. Ignoring DCI!\n", frame, slot, dci->pucch_resource_indicator);
      return -1;
    }

    /* ANTENNA_PORTS */
    uint8_t n_codewords = 1; // FIXME!!!
    long *max_length = NULL;
    long *dmrs_type = NULL;
    if (pdsch_config->dmrs_DownlinkForPDSCH_MappingTypeA) {
      max_length = pdsch_config->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->maxLength;
      dmrs_type = pdsch_config->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type;
    }
    if (pdsch_config->dmrs_DownlinkForPDSCH_MappingTypeB) {
      max_length = pdsch_config->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->maxLength;
      dmrs_type = pdsch_config->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->dmrs_Type;
    }
    if ((dmrs_type == NULL) && (max_length == NULL)){
      // Table 7.3.1.2.2-1: Antenna port(s) (1000 + DMRS port), dmrs-Type=1, maxLength=1
      dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_1[dci->antenna_ports.val][0];
      dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_1[dci->antenna_ports.val][1];
      dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_1[dci->antenna_ports.val][2];
      dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_1[dci->antenna_ports.val][3];
      dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_1[dci->antenna_ports.val][4];
    }
    if ((dmrs_type == NULL) && (max_length != NULL)){
      // Table 7.3.1.2.2-2: Antenna port(s) (1000 + DMRS port), dmrs-Type=1, maxLength=2
      if (n_codewords == 1) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][4];
	dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports.val][5];
      }
      if (n_codewords == 2) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][4];
	dlsch_config_pdu_1_1->dmrs_ports[4]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][5];
	dlsch_config_pdu_1_1->dmrs_ports[5]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][6];
	dlsch_config_pdu_1_1->dmrs_ports[6]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][7];
	dlsch_config_pdu_1_1->dmrs_ports[7]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][8];
	dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports.val][9];
      }
    }
    if ((dmrs_type != NULL) && (max_length == NULL)){
      // Table 7.3.1.2.2-3: Antenna port(s) (1000 + DMRS port), dmrs-Type=2, maxLength=1
      if (n_codewords == 1) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports.val][4];
      }
      if (n_codewords == 2) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][4];
	dlsch_config_pdu_1_1->dmrs_ports[4]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][5];
	dlsch_config_pdu_1_1->dmrs_ports[5]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports.val][6];
      }
    }
    if ((dmrs_type != NULL) && (max_length != NULL)){
      // Table 7.3.1.2.2-4: Antenna port(s) (1000 + DMRS port), dmrs-Type=2, maxLength=2
      if (n_codewords == 1) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][4];
	dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports.val][5];
      }
      if (n_codewords == 2) {
	dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][0];
	dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][1];
	dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][2];
	dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][3];
	dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][4];
	dlsch_config_pdu_1_1->dmrs_ports[4]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][5];
	dlsch_config_pdu_1_1->dmrs_ports[5]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][6];
	dlsch_config_pdu_1_1->dmrs_ports[6]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][7];
	dlsch_config_pdu_1_1->dmrs_ports[7]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][8];
	dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports.val][9];
      }
    }
    /* TCI */
    if (mac->dl_config_request.dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.tci_present_in_dci == 1){
      // 0 bit if higher layer parameter tci-PresentInDCI is not enabled
      // otherwise 3 bits as defined in Subclause 5.1.5 of [6, TS38.214]
      dlsch_config_pdu_1_1->tci_state = dci->transmission_configuration_indication.val;
    }
    /* SRS_REQUEST */
    // if SUL is supported in the cell, there is an additional bit in this field and the value of this bit represents table 7.1.1.1-1 TS 38.212 FIXME!!!
    dlsch_config_pdu_1_1->srs_config.aperiodicSRS_ResourceTrigger = (dci->srs_request.val & 0x11); // as per Table 7.3.1.1.2-24 TS 38.212
    /* CBGTI */
    dlsch_config_pdu_1_1->cbgti = dci->cbgti.val;
    /* CBGFI */
    dlsch_config_pdu_1_1->codeBlockGroupFlushIndicator = dci->cbgfi.val;
    /* DMRS_SEQ_INI */
    //FIXME!!!

    //	    dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP = n_RB_DLBWP;

    /* PDSCH_TO_HARQ_FEEDBACK_TIME_IND */
    // according to TS 38.213 Table 9.2.3-1
    NR_BWP_Id_t ul_bwp_id = mac->UL_BWP_Id;
    uint8_t feedback_ti =
      mac->ULbwp[ul_bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->dl_DataToUL_ACK->list.array[dci->pdsch_to_harq_feedback_timing_indicator.val][0];

   // set the harq status at MAC for feedback
   set_harq_status(mac,dci->pucch_resource_indicator,
                   dci->harq_pid,
                   dlsch_config_pdu_1_1->accumulated_delta_PUCCH,
                   feedback_ti,
                   dci->dai[0].val,
                   dci_ind->n_CCE,dci_ind->N_CCE,
                   frame,slot);

    dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
    LOG_D(MAC,"(nr_ue_procedures.c) pdu_type=%d\n\n",dl_config->dl_config_list[dl_config->number_pdus].pdu_type);
            
    dl_config->number_pdus = dl_config->number_pdus + 1;
    /* TODO same calculation for MCS table as done in UL */
    dlsch_config_pdu_1_1->mcs_table = (pdsch_config->mcs_Table) ? (*pdsch_config->mcs_Table + 1) : 0;
    /*PTRS configuration */
    if(mac->DLbwp[dl_bwp_id-1]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS != NULL) {
      valid_ptrs_setup = set_dl_ptrs_values(mac->DLbwp[dl_bwp_id-1]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup,
                                            dlsch_config_pdu_1_1->number_rbs, dlsch_config_pdu_1_1->mcs, dlsch_config_pdu_1_1->mcs_table,
                                            &dlsch_config_pdu_1_1->PTRSFreqDensity,&dlsch_config_pdu_1_1->PTRSTimeDensity,
                                            &dlsch_config_pdu_1_1->PTRSPortIndex,&dlsch_config_pdu_1_1->nEpreRatioOfPDSCHToPTRS,
                                            &dlsch_config_pdu_1_1->PTRSReOffset, dlsch_config_pdu_1_1->number_symbols);
      if(valid_ptrs_setup==true) {
        dlsch_config_pdu_1_1->pduBitmap |= 0x1;
        LOG_D(MAC, "DL PTRS values: PTRS time den: %d, PTRS freq den: %d\n", dlsch_config_pdu_1_1->PTRSTimeDensity, dlsch_config_pdu_1_1->PTRSFreqDensity);
      }
    }

    break;
  }

  case NR_DL_DCI_FORMAT_2_0:
    break;

  case NR_DL_DCI_FORMAT_2_1:        
    break;

  case NR_DL_DCI_FORMAT_2_2:        
    break;

  case NR_DL_DCI_FORMAT_2_3:
    break;

  default: 
    break;
  }


  if(rnti == SI_RNTI){

    //    }else if(rnti == mac->ra_rnti){

  }else if(rnti == P_RNTI){

  }else{  //  c-rnti

    ///  check if this is pdcch order 
    //dci->random_access_preamble_index;
    //dci->ss_pbch_index;
    //dci->prach_mask_index;

    ///  else normal DL-SCH grant
  }

  return ret;

}

void set_harq_status(NR_UE_MAC_INST_t *mac,
                     uint8_t pucch_id,
                     uint8_t harq_id,
                     int8_t delta_pucch,
                     uint8_t data_toul_fb,
                     uint8_t dai,
                     int n_CCE,
                     int N_CCE,
                     frame_t frame,
                     int slot) {

  NR_UE_HARQ_STATUS_t *current_harq = &mac->dl_harq_info[harq_id];

  current_harq->active = true;
  current_harq->ack_received = false;
  current_harq->pucch_resource_indicator = pucch_id;
  current_harq->feedback_to_ul = data_toul_fb;
  current_harq->dai = dai;
  current_harq->n_CCE = n_CCE;
  current_harq->N_CCE = N_CCE;
  current_harq->delta_pucch = delta_pucch;
  // FIXME k0 != 0 currently not taken into consideration
  current_harq->dl_frame = frame;
  current_harq->dl_slot = slot;

}


void update_harq_status(nr_downlink_indication_t *dl_info, int pdu_id) {

  NR_UE_MAC_INST_t *mac = get_mac_inst(dl_info->module_id);
  uint8_t harq_pid = dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.harq_pid;
  NR_UE_HARQ_STATUS_t *current_harq = &mac->dl_harq_info[harq_pid];

  if (current_harq->active) {
    current_harq->ack = dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.ack_nack;
    current_harq->ack_received = true;
  }
  else {
    //shouldn't get here
    LOG_E(MAC, "Trying to process acknack for an inactive harq process (%d)\n", harq_pid);
  }
}


void nr_ue_configure_pucch(NR_UE_MAC_INST_t *mac,
                           int slot,
                           uint16_t rnti,
                           PUCCH_sched_t *pucch,
                           fapi_nr_ul_config_pucch_pdu *pucch_pdu,
                           int O_SR, int O_ACK, int O_CSI) {

  int O_CRC = 0; //FIXME
  uint16_t O_uci = O_CSI + O_ACK;
  NR_BWP_Id_t bwp_id = mac->UL_BWP_Id;
  NR_PUCCH_FormatConfig_t *pucchfmt;
  long *pusch_id = NULL;
  long *id0 = NULL;
  int scs;
  NR_BWP_UplinkCommon_t *initialUplinkBWP;
  if (mac->scc) initialUplinkBWP = mac->scc->uplinkConfigCommon->initialUplinkBWP;
  else          initialUplinkBWP = &mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP;
  NR_BWP_Uplink_t *ubwp = mac->ULbwp[0];
  if (mac->cg && ubwp &&
      mac->cg->spCellConfig &&
      mac->cg->spCellConfig->spCellConfigDedicated &&
      mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
      mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP) {
    scs = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
  }
  else
    scs = initialUplinkBWP->genericParameters.subcarrierSpacing;

  int subframe_number = slot / (nr_slots_per_frame[scs]/10);
  nb_pucch_format_4_in_subframes[subframe_number] = 0;

  pucch_pdu->rnti = rnti;

  // configure pucch from Table 9.2.1-1
  if (pucch->initial_pucch_id > -1 &&
      pucch->pucch_resource == NULL) {

    pucch_pdu->format_type = initial_pucch_resource[pucch->initial_pucch_id].format;
    pucch_pdu->start_symbol_index = initial_pucch_resource[pucch->initial_pucch_id].startingSymbolIndex;
    pucch_pdu->nr_of_symbols = initial_pucch_resource[pucch->initial_pucch_id].nrofSymbols;

    pucch_pdu->bwp_size = NRRIV2BW(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth,
                                   MAX_BWP_SIZE);
    pucch_pdu->bwp_start = NRRIV2PRBOFFSET(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth,
                                           MAX_BWP_SIZE);

    pucch_pdu->prb_size = 1; // format 0 or 1
    int RB_BWP_offset;
    if (pucch->initial_pucch_id == 15)
      RB_BWP_offset = pucch_pdu->bwp_size>>2;
    else
      RB_BWP_offset = initial_pucch_resource[pucch->initial_pucch_id].PRB_offset;

    int N_CS = initial_pucch_resource[pucch->initial_pucch_id].nb_CS_indexes;
    pucch_pdu->prb_start = RB_BWP_offset + (pucch->initial_pucch_id/N_CS);
    if (pucch->initial_pucch_id>>3 == 0) {
      pucch_pdu->second_hop_prb = pucch_pdu->bwp_size - 1 - RB_BWP_offset - (pucch->initial_pucch_id/N_CS);
      pucch_pdu->initial_cyclic_shift = initial_pucch_resource[pucch->initial_pucch_id].initial_CS_indexes[pucch->initial_pucch_id%N_CS];
    }
    else {
      pucch_pdu->second_hop_prb = pucch_pdu->bwp_size - 1 - RB_BWP_offset - ((pucch->initial_pucch_id - 8)/N_CS);
      pucch_pdu->initial_cyclic_shift =  initial_pucch_resource[pucch->initial_pucch_id].initial_CS_indexes[(pucch->initial_pucch_id - 8)%N_CS];
    }
    pucch_pdu->freq_hop_flag = 1;
    pucch_pdu->time_domain_occ_idx = 0;

    if (O_SR == 0 || pucch->sr_payload == 0) {  /* only ack is transmitted TS 36.213 9.2.3 UE procedure for reporting HARQ-ACK */
      if (O_ACK == 1)
        pucch_pdu->mcs = sequence_cyclic_shift_1_harq_ack_bit[pucch->ack_payload & 0x1];   /* only harq of 1 bit */
      else
        pucch_pdu->mcs = sequence_cyclic_shift_2_harq_ack_bits[pucch->ack_payload & 0x3];  /* only harq with 2 bits */
    }
    else { /* SR + eventually ack are transmitted TS 36.213 9.2.5.1 UE procedure for multiplexing HARQ-ACK or CSI and SR */
      if (pucch->sr_payload == 1) {                /* positive scheduling request */
        if (O_ACK == 1)
          pucch_pdu->mcs = sequence_cyclic_shift_1_harq_ack_bit_positive_sr[pucch->ack_payload & 0x1];   /* positive SR and harq of 1 bit */
        else if (O_ACK == 2)
          pucch_pdu->mcs = sequence_cyclic_shift_2_harq_ack_bits_positive_sr[pucch->ack_payload & 0x3];  /* positive SR and harq with 2 bits */
        else
          pucch_pdu->mcs = 0;  /* only positive SR */
      }
    }

    // TODO verify if SR can be transmitted in this mode
    pucch_pdu->payload = (pucch->sr_payload << O_ACK) | pucch->ack_payload;

  }
  else if (pucch->pucch_resource != NULL) {

    NR_PUCCH_Resource_t *pucchres = pucch->pucch_resource;

    if (mac->cg &&
        mac->cg->physicalCellGroupConfig &&
        (mac->cg->physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH != NULL ||
        mac->cg->physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook != 1)) {
      LOG_E(PHY,"PUCCH Unsupported cell group configuration : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
      return;
    }
    else if (mac->cg &&
             mac->cg->spCellConfig &&
             mac->cg->spCellConfig->spCellConfigDedicated &&
             mac->cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig &&
             mac->cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup &&
             mac->cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup->codeBlockGroupTransmission != NULL) {
      LOG_E(MAC,"PUCCH Unsupported code block group for serving cell config : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
      return;
    }
    NR_PUCCH_Config_t *pucch_Config;
    if (bwp_id>0 &&
        mac->ULbwp[bwp_id-1] &&
        mac->ULbwp[bwp_id-1]->bwp_Dedicated &&
        mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config &&
        mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup) {
      NR_PUSCH_Config_t *pusch_Config = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pusch_Config->choice.setup;
      pusch_id = pusch_Config->dataScramblingIdentityPUSCH;
      if (pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA != NULL)
        id0 = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup->transformPrecodingDisabled->scramblingID0;
      else if (pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB != NULL)
        id0 = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->transformPrecodingDisabled->scramblingID0;
      else *id0 = mac->physCellId;
      pucch_Config =  mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup;
      pucch_pdu->bwp_size = NRRIV2BW(mac->ULbwp[bwp_id-1]->bwp_Common->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
      pucch_pdu->bwp_start = NRRIV2PRBOFFSET(mac->ULbwp[bwp_id-1]->bwp_Common->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
    }
    else if (bwp_id==0 &&
             mac->cg &&
             mac->cg->spCellConfig &&
             mac->cg->spCellConfig->spCellConfigDedicated &&
             mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
             mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
             mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config &&
             mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup) {
      pucch_Config = mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup;
      pucch_pdu->bwp_size = NRRIV2BW(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
      pucch_pdu->bwp_start = NRRIV2PRBOFFSET(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
    }
    else AssertFatal(1==0,"no pucch_Config\n");

    NR_PUCCH_ConfigCommon_t *pucch_ConfigCommon;
    if (mac->scc) pucch_ConfigCommon = mac->scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup;
    else          pucch_ConfigCommon = mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pucch_ConfigCommon->choice.setup;
    if (pucch_ConfigCommon->hoppingId != NULL)
      pucch_pdu->hopping_id = *pucch_ConfigCommon->hoppingId;
    else
      pucch_pdu->hopping_id = mac->physCellId;

    switch (pucch_ConfigCommon->pucch_GroupHopping){
      case 0 :
      // if neither, both disabled
      pucch_pdu->group_hop_flag = 0;
      pucch_pdu->sequence_hop_flag = 0;
      break;
    case 1 :
      // if enable, group enabled
      pucch_pdu->group_hop_flag = 1;
      pucch_pdu->sequence_hop_flag = 0;
      break;
    case 2 :
      // if disable, sequence disabled
      pucch_pdu->group_hop_flag = 0;
      pucch_pdu->sequence_hop_flag = 1;
      break;
    default:
      AssertFatal(1==0,"Group hopping flag undefined (0,1,2) \n");
    }

    pucch_pdu->prb_start = pucchres->startingPRB;
    pucch_pdu->freq_hop_flag = pucchres->intraSlotFrequencyHopping!= NULL ?  1 : 0;
    pucch_pdu->second_hop_prb = pucchres->secondHopPRB!= NULL ?  *pucchres->secondHopPRB : 0;
    pucch_pdu->prb_size = 1; // format 0 or 1

    if ((O_SR+O_CSI+O_SR) > (sizeof(uint64_t)*8)) {
      LOG_E(MAC,"PUCCH number of UCI bits exceeds payload size : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
      return;
    }
    pucch_pdu->payload = (pucch->csi_part1_payload << (O_ACK + O_SR)) |  (pucch->sr_payload << O_ACK) | pucch->ack_payload;

    switch(pucchres->format.present) {
      case NR_PUCCH_Resource__format_PR_format0 :
        pucch_pdu->format_type = 0;
        pucch_pdu->initial_cyclic_shift = pucchres->format.choice.format0->initialCyclicShift;
        pucch_pdu->nr_of_symbols = pucchres->format.choice.format0->nrofSymbols;
        pucch_pdu->start_symbol_index = pucchres->format.choice.format0->startingSymbolIndex;
        if (O_SR == 0 || pucch->sr_payload == 0) {  /* only ack is transmitted TS 36.213 9.2.3 UE procedure for reporting HARQ-ACK */
          if (O_ACK == 1)
            pucch_pdu->mcs = sequence_cyclic_shift_1_harq_ack_bit[pucch->ack_payload & 0x1];   /* only harq of 1 bit */
          else
            pucch_pdu->mcs = sequence_cyclic_shift_2_harq_ack_bits[pucch->ack_payload & 0x3];  /* only harq with 2 bits */
        }
        else { /* SR + eventually ack are transmitted TS 36.213 9.2.5.1 UE procedure for multiplexing HARQ-ACK or CSI and SR */
          if (pucch->sr_payload == 1) {                /* positive scheduling request */
            if (O_ACK == 1)
              pucch_pdu->mcs = sequence_cyclic_shift_1_harq_ack_bit_positive_sr[pucch->ack_payload & 0x1];   /* positive SR and harq of 1 bit */
            else if (O_ACK == 2)
              pucch_pdu->mcs = sequence_cyclic_shift_2_harq_ack_bits_positive_sr[pucch->ack_payload & 0x3];  /* positive SR and harq with 2 bits */
            else
              pucch_pdu->mcs = 0;  /* only positive SR */
          }
        }
        break;
      case NR_PUCCH_Resource__format_PR_format1 :
        pucch_pdu->format_type = 1;
        pucch_pdu->initial_cyclic_shift = pucchres->format.choice.format1->initialCyclicShift;
        pucch_pdu->nr_of_symbols = pucchres->format.choice.format1->nrofSymbols;
        pucch_pdu->start_symbol_index = pucchres->format.choice.format1->startingSymbolIndex;
        pucch_pdu->time_domain_occ_idx = pucchres->format.choice.format1->timeDomainOCC;
        break;
      case NR_PUCCH_Resource__format_PR_format2 :
        pucch_pdu->format_type = 2;
        pucch_pdu->n_bit = O_uci+O_SR;
        pucch_pdu->nr_of_symbols = pucchres->format.choice.format2->nrofSymbols;
        pucch_pdu->start_symbol_index = pucchres->format.choice.format2->startingSymbolIndex;
        pucch_pdu->data_scrambling_id = pusch_id!= NULL ? *pusch_id : mac->physCellId;
        pucch_pdu->dmrs_scrambling_id = id0!= NULL ? *id0 : mac->physCellId;
        pucch_pdu->prb_size = compute_pucch_prb_size(2,pucchres->format.choice.format2->nrofPRBs,
                                                     O_uci+O_SR,O_CSI,pucch_Config->format2->choice.setup->maxCodeRate,
                                                     2,pucchres->format.choice.format2->nrofSymbols,8);
        break;
      case NR_PUCCH_Resource__format_PR_format3 :
        pucch_pdu->format_type = 3;
        pucch_pdu->n_bit = O_uci+O_SR;
        pucch_pdu->nr_of_symbols = pucchres->format.choice.format3->nrofSymbols;
        pucch_pdu->start_symbol_index = pucchres->format.choice.format3->startingSymbolIndex;
        pucch_pdu->data_scrambling_id = pusch_id!= NULL ? *pusch_id : mac->physCellId;
        if (pucch_Config->format3 == NULL) {
          pucch_pdu->pi_2bpsk = 0;
          pucch_pdu->add_dmrs_flag = 0;
        }
        else {
          pucchfmt = pucch_Config->format3->choice.setup;
          pucch_pdu->pi_2bpsk = pucchfmt->pi2BPSK!= NULL ?  1 : 0;
          pucch_pdu->add_dmrs_flag = pucchfmt->additionalDMRS!= NULL ?  1 : 0;
        }
        int f3_dmrs_symbols;
        if (pucchres->format.choice.format3->nrofSymbols==4)
          f3_dmrs_symbols = 1<<pucch_pdu->freq_hop_flag;
        else {
          if(pucchres->format.choice.format3->nrofSymbols<10)
            f3_dmrs_symbols = 2;
          else
            f3_dmrs_symbols = 2<<pucch_pdu->add_dmrs_flag;
        }
        pucch_pdu->prb_size = compute_pucch_prb_size(3,pucchres->format.choice.format3->nrofPRBs,
                                                     O_uci+O_SR,O_CSI,pucch_Config->format3->choice.setup->maxCodeRate,
                                                     2-pucch_pdu->pi_2bpsk,pucchres->format.choice.format3->nrofSymbols-f3_dmrs_symbols,12);
        break;
      case NR_PUCCH_Resource__format_PR_format4 :
        pucch_pdu->format_type = 4;
        pucch_pdu->nr_of_symbols = pucchres->format.choice.format4->nrofSymbols;
        pucch_pdu->start_symbol_index = pucchres->format.choice.format4->startingSymbolIndex;
        pucch_pdu->pre_dft_occ_len = pucchres->format.choice.format4->occ_Length;
        pucch_pdu->pre_dft_occ_idx = pucchres->format.choice.format4->occ_Index;
        pucch_pdu->data_scrambling_id = pusch_id!= NULL ? *pusch_id : mac->physCellId;
        if (pucch_Config->format3 == NULL) {
          pucch_pdu->pi_2bpsk = 0;
          pucch_pdu->add_dmrs_flag = 0;
        }
        else {
          pucchfmt = pucch_Config->format3->choice.setup;
          pucch_pdu->pi_2bpsk = pucchfmt->pi2BPSK!= NULL ?  1 : 0;
          pucch_pdu->add_dmrs_flag = pucchfmt->additionalDMRS!= NULL ?  1 : 0;
        }
        break;
      default :
        AssertFatal(1==0,"Undefined PUCCH format \n");
    }
    pucch_pdu->pucch_tx_power = get_pucch_tx_power_ue(mac,
                                                      pucch_Config,
                                                      pucch,
                                                      pucch_pdu->format_type,
                                                      pucch_pdu->prb_size,
                                                      pucch_pdu->freq_hop_flag,
                                                      pucch_pdu->add_dmrs_flag,
                                                      pucch_pdu->nr_of_symbols,
                                                      subframe_number,
                                                      O_ACK, O_SR,
                                                      O_CSI, O_CRC);
  }
}


int16_t get_pucch_tx_power_ue(NR_UE_MAC_INST_t *mac,
                              NR_PUCCH_Config_t *pucch_Config,
                              PUCCH_sched_t *pucch,
                              uint8_t format_type,
                              uint16_t nb_of_prbs,
                              uint8_t  freq_hop_flag,
                              uint8_t  add_dmrs_flag,
                              uint8_t N_symb_PUCCH,
                              int subframe_number,
                              int O_ACK, int O_SR,
                              int O_CSI, int O_CRC) {

  int PUCCH_POWER_DEFAULT = 0;
  int16_t P_O_NOMINAL_PUCCH;
  if (mac->scc) P_O_NOMINAL_PUCCH = *mac->scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->p0_nominal;
  else          P_O_NOMINAL_PUCCH = *mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pucch_ConfigCommon->choice.setup->p0_nominal;

  struct NR_PUCCH_PowerControl *power_config = pucch_Config->pucch_PowerControl;

  if (!power_config)
    return (PUCCH_POWER_DEFAULT);

  int16_t P_O_UE_PUCCH;
  int16_t G_b_f_c = 0;

  if (pucch_Config->spatialRelationInfoToAddModList != NULL) {  /* FFS TODO NR */
    LOG_D(MAC,"PUCCH Spatial relation infos are not yet implemented : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
    return (PUCCH_POWER_DEFAULT);
  }

  if (power_config->p0_Set != NULL) {
    P_O_UE_PUCCH = power_config->p0_Set->list.array[0]->p0_PUCCH_Value; /* get from index 0 if no spatial relation set */
    G_b_f_c = 0;
  }
  else {
    G_b_f_c = pucch->delta_pucch;
    LOG_D(MAC,"PUCCH Transmit power control command not yet implemented for NR : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
    return (PUCCH_POWER_DEFAULT);
  }

  int P_O_PUCCH = P_O_NOMINAL_PUCCH + P_O_UE_PUCCH;

  int16_t delta_F_PUCCH;
  int DELTA_TF;
  uint16_t N_ref_PUCCH;
  int N_sc_ctrl_RB = 0;

  /* computing of pucch transmission power adjustment */
  switch (format_type) {
    case 0:
      N_ref_PUCCH = 2;
      DELTA_TF = 10 * log10(N_ref_PUCCH/N_symb_PUCCH);
      delta_F_PUCCH =  *power_config->deltaF_PUCCH_f0;
      break;
    case 1:
      N_ref_PUCCH = 14;
      DELTA_TF = 10 * log10(N_ref_PUCCH/N_symb_PUCCH);
      delta_F_PUCCH =  *power_config->deltaF_PUCCH_f1;
      break;
    case 2:
      N_sc_ctrl_RB = 10;
      DELTA_TF = get_deltatf(nb_of_prbs,
                             N_symb_PUCCH,
                             freq_hop_flag,
                             add_dmrs_flag,
                             N_sc_ctrl_RB,
                             pucch->n_HARQ_ACK,
                             O_ACK, O_SR,
                             O_CSI, O_CRC);
      delta_F_PUCCH =  *power_config->deltaF_PUCCH_f2;
      break;
    case 3:
      N_sc_ctrl_RB = 14;
      DELTA_TF = get_deltatf(nb_of_prbs,
                             N_symb_PUCCH,
                             freq_hop_flag,
                             add_dmrs_flag,
                             N_sc_ctrl_RB,
                             pucch->n_HARQ_ACK,
                             O_ACK, O_SR,
                             O_CSI, O_CRC);
      delta_F_PUCCH =  *power_config->deltaF_PUCCH_f3;
      break;
    case 4:
      N_sc_ctrl_RB = 14/(nb_pucch_format_4_in_subframes[subframe_number]);
      DELTA_TF = get_deltatf(nb_of_prbs,
                             N_symb_PUCCH,
                             freq_hop_flag,
                             add_dmrs_flag,
                             N_sc_ctrl_RB,
                             pucch->n_HARQ_ACK,
                             O_ACK, O_SR,
                             O_CSI, O_CRC);
      delta_F_PUCCH =  *power_config->deltaF_PUCCH_f4;
      break;
    default:
    {
      LOG_E(MAC,"PUCCH unknown pucch format : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
      return (0);
    }
  }

  if (*power_config->twoPUCCH_PC_AdjustmentStates > 1) {
    LOG_E(MAC,"PUCCH power control adjustment states with 2 states not yet implemented : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
    return (PUCCH_POWER_DEFAULT);
  }

  int16_t pucch_power = P_O_PUCCH + delta_F_PUCCH + DELTA_TF + G_b_f_c;

  NR_TST_PHY_PRINTF("PUCCH ( Tx power : %d dBm ) ( 10Log(...) : %d ) ( from Path Loss : %d ) ( delta_F_PUCCH : %d ) ( DELTA_TF : %d ) ( G_b_f_c : %d ) \n",
                    pucch_power, contributor, PL, delta_F_PUCCH, DELTA_TF, G_b_f_c);

  return (pucch_power);
}

int get_deltatf(uint16_t nb_of_prbs,
                uint8_t N_symb_PUCCH,
                uint8_t freq_hop_flag,
                uint8_t add_dmrs_flag,
                int N_sc_ctrl_RB,
                int n_HARQ_ACK,
                int O_ACK, int O_SR,
                int O_CSI, int O_CRC){

  int DELTA_TF;
  int O_UCI = O_ACK + O_SR + O_CSI + O_CRC;
  int N_symb = nb_symbols_excluding_dmrs[N_symb_PUCCH-4][add_dmrs_flag][freq_hop_flag];
  float N_RE = nb_of_prbs * N_sc_ctrl_RB * N_symb;
  float K1 = 6;
  if (O_UCI < 12)
    DELTA_TF = 10 * log10((double)(((K1 * (n_HARQ_ACK + O_SR + O_CSI))/N_RE)));
  else {
    float K2 = 2.4;
    float BPRE = O_UCI/N_RE;
    DELTA_TF = 10 * log10((double)(pow(2,(K2*BPRE)) - 1));
  }
  return DELTA_TF;
}

/*******************************************************************
*
* NAME :         find_pucch_resource_set
*
* PARAMETERS :   ue context
*                gNB_id identifier
*
*
* RETURN :       harq process identifier
*
* DESCRIPTION :  return tx harq process identifier for given transmission slot
*                YS 38.213 9.2.2  PUCCH Formats for UCI transmission
*
*********************************************************************/

int find_pucch_resource_set(NR_UE_MAC_INST_t *mac, int uci_size) {
  int pucch_resource_set_id = 0;
  NR_BWP_Id_t bwp_id = mac->DL_BWP_Id;

  //long *pucch_max_pl_bits = NULL;

  /* from TS 38.331 field maxPayloadMinus1
    -- Maximum number of payload bits minus 1 that the UE may transmit using this PUCCH resource set. In a PUCCH occurrence, the UE
    -- chooses the first of its PUCCH-ResourceSet which supports the number of bits that the UE wants to transmit.
    -- The field is not present in the first set (Set0) since the maximum Size of Set0 is specified to be 3 bit.
    -- The field is not present in the last configured set since the UE derives its maximum payload size as specified in 38.213.
    -- This field can take integer values that are multiples of 4. Corresponds to L1 parameter 'N_2' or 'N_3' (see 38.213, section 9.2)
  */
  /* look for the first resource set which supports uci_size number of bits for payload */
  while (pucch_resource_set_id < MAX_NB_OF_PUCCH_RESOURCE_SETS) {
    if ((bwp_id>0 &&
         mac->ULbwp[bwp_id-1] &&
         mac->ULbwp[bwp_id-1]->bwp_Dedicated &&
         mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config &&
         mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup &&
         mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList &&
         mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.array[pucch_resource_set_id] != NULL) ||
        (bwp_id==0 &&
         mac->cg &&
         mac->cg->spCellConfig &&
         mac->cg->spCellConfig->spCellConfigDedicated &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList->list.array[pucch_resource_set_id] != NULL)) {
      if (uci_size <= 2) {
        pucch_resource_set_id = 0;
        return (pucch_resource_set_id);
        break;
      }
      else {
        pucch_resource_set_id = 1;
        return (pucch_resource_set_id);
        break;
      }
    }
    pucch_resource_set_id++;
  }

  pucch_resource_set_id = MAX_NB_OF_PUCCH_RESOURCE_SETS;

  return (pucch_resource_set_id);
}


/*******************************************************************
*
* NAME :         select_pucch_format
*
* PARAMETERS :   ue context
*                processing slots of reception/transmission
*                gNB_id identifier
*
* RETURN :       TRUE a valid resource has been found
*
* DESCRIPTION :  return tx harq process identifier for given transmission slot
*                TS 38.213 9.2.1  PUCCH Resource Sets
*                TS 38.213 9.2.2  PUCCH Formats for UCI transmission
*                In the case of pucch for scheduling request only, resource is already get from scheduling request configuration
*
*********************************************************************/

void select_pucch_resource(NR_UE_MAC_INST_t *mac,
                           PUCCH_sched_t *pucch) {

  NR_PUCCH_ResourceId_t *current_resource_id = NULL;
  NR_BWP_Id_t bwp_id = mac->UL_BWP_Id;
  int n_list;

  if ((bwp_id == 0 &&
       mac->cg == NULL) ||
      (bwp_id == 0 &&
       mac->cg &&
       mac->cg->spCellConfig &&
       mac->cg->spCellConfig->spCellConfigDedicated &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList->list.array[0] == NULL) ||
      (mac->ULbwp[bwp_id-1] &&
       mac->ULbwp[bwp_id-1]->bwp_Dedicated &&
       mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config &&
       mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup &&
       mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList &&
       mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.array[0] == NULL)
      ){

    /* see TS 38.213 9.2.1  PUCCH Resource Sets */
    int delta_PRI = pucch->resource_indicator;
    int n_CCE_0 = pucch->n_CCE;
    int N_CCE_0 = pucch->N_CCE;
    if (N_CCE_0 == 0) {
      AssertFatal(1==0,"PUCCH No compatible pucch format found : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    }
    int r_PUCCH = ((2 * n_CCE_0)/N_CCE_0) + (2 * delta_PRI);
    pucch->initial_pucch_id = r_PUCCH;
    pucch->pucch_resource = NULL;
  }
  else {
    struct NR_PUCCH_Config__resourceSetToAddModList *resourceSetToAddModList = NULL;
    struct NR_PUCCH_Config__resourceToAddModList *resourceToAddModList = NULL;
    if (bwp_id > 0 && mac->ULbwp[bwp_id-1]) {
      AssertFatal(mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList!=NULL,
                  "mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList is null\n");
      resourceSetToAddModList = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList;
      resourceToAddModList = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceToAddModList;
    }
    else if (bwp_id == 0 && mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList!=NULL) {
      resourceSetToAddModList = mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList;
      resourceToAddModList = mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceToAddModList;
    }

    n_list = resourceSetToAddModList->list.count;
    if (pucch->resource_set_id > n_list) {
      LOG_E(MAC,"Invalid PUCCH resource set id %d\n",pucch->resource_set_id);
      pucch->pucch_resource = NULL;
      return;
    }
    n_list = resourceSetToAddModList->list.array[pucch->resource_set_id]->resourceList.list.count;
    if (pucch->resource_indicator > n_list) {
      LOG_E(MAC,"Invalid PUCCH resource id %d\n",pucch->resource_indicator);
      pucch->pucch_resource = NULL;
      return;
    }
    current_resource_id = resourceSetToAddModList->list.array[pucch->resource_set_id]->resourceList.list.array[pucch->resource_indicator];
    n_list = resourceToAddModList->list.count;
    int res_found = 0;
    for (int i=0; i<n_list; i++) {
      if (resourceToAddModList->list.array[i]->pucch_ResourceId == *current_resource_id) {
        pucch->pucch_resource = resourceToAddModList->list.array[i];
        res_found = 1;
        break;
      }
    }
    if (res_found == 0) {
      LOG_E(MAC,"Couldn't find PUCCH Resource\n");
      pucch->pucch_resource = NULL;
    }
  }
}

/*******************************************************************
*
* NAME :         get_downlink_ack
*
* PARAMETERS :   ue context
*                processing slots of reception/transmission
*                gNB_id identifier
*
* RETURN :       o_ACK acknowledgment data
*                o_ACK_number_bits number of bits for acknowledgment
*
* DESCRIPTION :  return acknowledgment value
*                TS 38.213 9.1.3 Type-2 HARQ-ACK codebook determination
*
*          --+--------+-------+--------+-------+---  ---+-------+--
*            | PDCCH1 |       | PDCCH2 |PDCCH3 |        | PUCCH |
*          --+--------+-------+--------+-------+---  ---+-------+--
*    DAI_DL      1                 2       3              ACK for
*                V                 V       V        PDCCH1, PDDCH2 and PCCH3
*                |                 |       |               ^
*                +-----------------+-------+---------------+
*
*                PDCCH1, PDCCH2 and PDCCH3 are PDCCH monitoring occasions
*                M is the total of monitoring occasions
*
*********************************************************************/

uint8_t get_downlink_ack(NR_UE_MAC_INST_t *mac,
                         frame_t frame,
                         int slot,
                         PUCCH_sched_t *pucch) {


  uint32_t ack_data[NR_DL_MAX_NB_CW][NR_DL_MAX_DAI] = {{0},{0}};
  uint32_t dai[NR_DL_MAX_NB_CW][NR_DL_MAX_DAI] = {{0},{0}};       /* for serving cell */
  uint32_t dai_total[NR_DL_MAX_NB_CW][NR_DL_MAX_DAI] = {{0},{0}}; /* for multiple cells */
  int number_harq_feedback = 0;
  uint32_t dai_current = 0;
  uint32_t dai_max = 0;
  bool two_transport_blocks = FALSE;
  int number_of_code_word = 1;
  int U_DAI_c = 0;
  int N_m_c_rx = 0;
  int V_DAI_m_DL = 0;
  NR_UE_HARQ_STATUS_t *current_harq;
  int sched_frame,sched_slot;
  NR_BWP_UplinkCommon_t *initialUplinkBWP;
  if (mac->scc) initialUplinkBWP = mac->scc->uplinkConfigCommon->initialUplinkBWP;
  else          initialUplinkBWP = &mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP;
  int slots_per_frame,scs;

  if (mac->DLbwp[0] &&
      mac->DLbwp[0]->bwp_Dedicated &&
      mac->DLbwp[0]->bwp_Dedicated->pdsch_Config &&
      mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup &&
      mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->maxNrofCodeWordsScheduledByDCI[0] == 2) {
    two_transport_blocks = TRUE;
    number_of_code_word = 2;
  }
  else {
    number_of_code_word = 1;
  }

  NR_BWP_Uplink_t *ubwp = mac->ULbwp[0];
  if (mac->cg && ubwp &&
      mac->cg->spCellConfig &&
      mac->cg->spCellConfig->spCellConfigDedicated &&
      mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
      mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP) {
    scs = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
  }
  else
    scs = initialUplinkBWP->genericParameters.subcarrierSpacing;

  slots_per_frame = nr_slots_per_frame[scs];

  /* look for dl acknowledgment which should be done on current uplink slot */
  for (int code_word = 0; code_word < number_of_code_word; code_word++) {

    for (int dl_harq_pid = 0; dl_harq_pid < 16; dl_harq_pid++) {

      current_harq = &mac->dl_harq_info[dl_harq_pid];

      if (current_harq->active) {

        sched_slot = current_harq->dl_slot + current_harq->feedback_to_ul;
        sched_frame = current_harq->dl_frame;
        if (sched_slot>=slots_per_frame){
          sched_slot %= slots_per_frame;
          sched_frame++;
        }

        /* check if current tx slot should transmit downlink acknowlegment */
        if (sched_frame == frame && sched_slot == slot) {

          if (current_harq->dai > NR_DL_MAX_DAI) {
            LOG_E(MAC,"PUCCH Downlink DAI has an invalid value : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
          }
          else {

            if ((pucch->resource_indicator != -1) && (pucch->resource_indicator != current_harq->pucch_resource_indicator))
              LOG_E(MAC, "Value of pucch_resource_indicator %d not matching with what set before %d (Possibly due to a false DCI) \n",
                    current_harq->pucch_resource_indicator,pucch->resource_indicator);
            else{
              dai_current = current_harq->dai+1; // DCI DAI to counter DAI conversion

              if (dai_current == 0) {
                LOG_E(MAC,"PUCCH Downlink dai is invalid : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
                return(0);
              } else if (dai_current > dai_max) {
                dai_max = dai_current;
              }

              number_harq_feedback++;
              if (current_harq->ack_received)
                ack_data[code_word][dai_current - 1] = current_harq->ack;
              else
                ack_data[code_word][dai_current - 1] = 0;
              dai[code_word][dai_current - 1] = dai_current;

              pucch->resource_indicator = current_harq->pucch_resource_indicator;
              pucch->n_CCE = current_harq->n_CCE;
              pucch->N_CCE = current_harq->N_CCE;
              pucch->delta_pucch = current_harq->delta_pucch;
              current_harq->active = false;
              current_harq->ack_received = false;
            }
          }
        }
      }
    }
  }

  /* no any ack to transmit */
  if (number_harq_feedback == 0) {
    pucch->n_HARQ_ACK = 0;
    return(0);
  }
  else  if (number_harq_feedback > (sizeof(uint32_t)*8)) {
    LOG_E(MAC,"PUCCH number of ack bits exceeds payload size : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    return(0);
  }

  /* for computing n_HARQ_ACK for power */
  V_DAI_m_DL = dai_max;
  U_DAI_c = number_harq_feedback/number_of_code_word;
  N_m_c_rx = number_harq_feedback;
  int N_SPS_c = 0; /* FFS TODO_NR multicells and SPS are not supported at the moment */
  if (mac->cg != NULL &&
      mac->cg->physicalCellGroupConfig != NULL &&
      mac->cg->physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH != NULL) {
    int N_TB_max_DL = mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->maxNrofCodeWordsScheduledByDCI[0];
    pucch->n_HARQ_ACK = (((V_DAI_m_DL - U_DAI_c)%4) * N_TB_max_DL) + N_m_c_rx + N_SPS_c;
    NR_TST_PHY_PRINTF("PUCCH power n(%d) = ( V(%d) - U(%d) )mod4 * N_TB(%d) + N(%d) \n", pucch->n_HARQ_ACK, V_DAI_m_DL, U_DAI_c, N_TB_max_DL, N_m_c_rx);
  }

  /*
  * For a monitoring occasion of a PDCCH with DCI format 1_0 or DCI format 1_1 in at least one serving cell,
  * when a UE receives a PDSCH with one transport block and the value of higher layer parameter maxNrofCodeWordsScheduledByDCI is 2,
  * the HARQ-ACK response is associated with the first transport block and the UE generates a NACK for the second transport block
  * if spatial bundling is not applied (HARQ-ACK-spatial-bundling-PUCCH = FALSE) and generates HARQ-ACK value of ACK for the second
  * transport block if spatial bundling is applied.
  */

  for (int code_word = 0; code_word < number_of_code_word; code_word++) {
    for (uint32_t i = 0; i < dai_max ; i++ ) {
      if (dai[code_word][i] != i + 1) { /* fill table with consistent value for each dai */
        dai[code_word][i] = i + 1;      /* it covers case for which PDCCH DCI has not been successfully decoded and so it has been missed */
        ack_data[code_word][i] = 0;     /* nack data transport block which has been missed */
        number_harq_feedback++;
      }
      if (two_transport_blocks == TRUE) {
        dai_total[code_word][i] = dai[code_word][i]; /* for a single cell, dai_total is the same as dai of first cell */
      }
    }
  }

  int M = dai_max;
  int j = 0;
  uint32_t V_temp = 0;
  uint32_t V_temp2 = 0;
  int O_ACK = 0;
  int o_ACK = 0;
  int O_bit_number_cw0 = 0;
  int O_bit_number_cw1 = 0;

  for (int m = 0; m < M ; m++) {

    if (dai[0][m] <= V_temp) {
      j = j + 1;
    }

    V_temp = dai[0][m]; /* value of the counter DAI for format 1_0 and format 1_1 on serving cell c */

    if (dai_total[0][m] == 0) {
      V_temp2 = dai[0][m];
    } else {
      V_temp2 = dai[1][m];         /* second code word has been received */
      O_bit_number_cw1 = (8 * j) + 2*(V_temp - 1) + 1;
      o_ACK = o_ACK | (ack_data[1][m] << O_bit_number_cw1);
    }

    if (two_transport_blocks == TRUE) {
      O_bit_number_cw0 = (8 * j) + 2*(V_temp - 1);
    }
    else {
      O_bit_number_cw0 = (4 * j) + (V_temp - 1);
    }

    o_ACK = o_ACK | (ack_data[0][m] << O_bit_number_cw0);
  }

  if (V_temp2 < V_temp) {
    j = j + 1;
  }

  if (two_transport_blocks == TRUE) {
    O_ACK = 2 * ( 4 * j + V_temp2);  /* for two transport blocks */
  }
  else {
    O_ACK = 4 * j + V_temp2;         /* only one transport block */
  }

  if (number_harq_feedback != O_ACK) {
    LOG_E(MAC,"PUCCH Error for number of bits for acknowledgment : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    return (0);
  }

  pucch->ack_payload = o_ACK;

  return(number_harq_feedback);
}


bool trigger_periodic_scheduling_request(NR_UE_MAC_INST_t *mac,
                                         PUCCH_sched_t *pucch,
                                         frame_t frame,
                                         int slot) {

  NR_BWP_Id_t bwp_id = mac->UL_BWP_Id;
  NR_PUCCH_Config_t *pucch_Config = NULL;
  int scs;
  NR_BWP_Uplink_t *ubwp = mac->ULbwp[0];
  NR_BWP_UplinkCommon_t *initialUplinkBWP;
  if (mac->scc) initialUplinkBWP = mac->scc->uplinkConfigCommon->initialUplinkBWP;
  else          initialUplinkBWP = &mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP;

  if (mac->cg && ubwp &&
      mac->cg->spCellConfig &&
      mac->cg->spCellConfig->spCellConfigDedicated &&
      mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
      mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP) {
    scs = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
  }
  else
    scs = initialUplinkBWP->genericParameters.subcarrierSpacing;

  const int n_slots_frame = nr_slots_per_frame[scs];

  if (bwp_id>0 &&
      mac->ULbwp[bwp_id-1] &&
      mac->ULbwp[bwp_id-1]->bwp_Dedicated &&
      mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config &&
      mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup) {
    pucch_Config =  mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup;
  }
  else if (bwp_id==0 &&
           mac->cg &&
           mac->cg->spCellConfig &&
           mac->cg->spCellConfig->spCellConfigDedicated &&
           mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
           mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
           mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config &&
           mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup) {
    pucch_Config = mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup;
  }
  if(!pucch_Config || pucch_Config->schedulingRequestResourceToAddModList->list.count==0)
    return false; // SR not configured

  for (int SR_resource_id =0; SR_resource_id < pucch_Config->schedulingRequestResourceToAddModList->list.count;SR_resource_id++) {
    NR_SchedulingRequestResourceConfig_t *SchedulingRequestResourceConfig = pucch_Config->schedulingRequestResourceToAddModList->list.array[SR_resource_id];
    int SR_period; int SR_offset;

    find_period_offest_SR(SchedulingRequestResourceConfig,&SR_period,&SR_offset);
    int sfn_sf = frame * n_slots_frame + slot;

    if ((sfn_sf - SR_offset) % SR_period == 0) {
      LOG_D(MAC, "Scheduling Request active in frame %d slot %d \n", frame, slot);
      NR_PUCCH_ResourceId_t *PucchResourceId = SchedulingRequestResourceConfig->resource;

      int found = -1;
      NR_PUCCH_ResourceSet_t *pucchresset = pucch_Config->resourceSetToAddModList->list.array[0]; // set with formats 0,1
      int n_list = pucchresset->resourceList.list.count;
       for (int i=0; i<n_list; i++) {
        if (*pucchresset->resourceList.list.array[i] == *PucchResourceId ) {
          found = i;
          break;
        }
      }
      if (found == -1) {
        LOG_E(MAC,"Couldn't find PUCCH resource for SR\n");
        return false;
      }
      pucch->resource_indicator = found;
      return true;
    }
  }
  return false;
}


int8_t nr_ue_get_SR(module_id_t module_idP, frame_t frameP, int slotP){
  return 0;
}


uint8_t nr_get_csi_measurements(NR_UE_MAC_INST_t *mac,
                                frame_t frame,
                                int slot,
                                PUCCH_sched_t *pucch) {

  NR_BWP_Id_t bwp_id = mac->UL_BWP_Id;
  NR_PUCCH_Config_t *pucch_Config = NULL;
  int csi_bits = 0;

  if(mac->cg &&
     mac->cg->spCellConfig &&
     mac->cg->spCellConfig->spCellConfigDedicated &&
     mac->cg->spCellConfig->spCellConfigDedicated->csi_MeasConfig) {

    NR_CSI_MeasConfig_t *csi_measconfig = mac->cg->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup;

    for (int csi_report_id = 0; csi_report_id < csi_measconfig->csi_ReportConfigToAddModList->list.count; csi_report_id++){
      NR_CSI_ReportConfig_t *csirep = csi_measconfig->csi_ReportConfigToAddModList->list.array[csi_report_id];

      if(csirep->reportConfigType.present == NR_CSI_ReportConfig__reportConfigType_PR_periodic){
        int period, offset;
        csi_period_offset(csirep, NULL, &period, &offset);

        int scs;
        NR_BWP_Uplink_t *ubwp = mac->ULbwp[0];
        NR_BWP_UplinkCommon_t *initialUplinkBWP;
        if (mac->scc) initialUplinkBWP = mac->scc->uplinkConfigCommon->initialUplinkBWP;
        else          initialUplinkBWP = &mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP;

        if (ubwp &&
            mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
            mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP)
          scs = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
        else
          scs = initialUplinkBWP->genericParameters.subcarrierSpacing;

        if (bwp_id>0 &&
            mac->ULbwp[bwp_id-1] &&
            mac->ULbwp[bwp_id-1]->bwp_Dedicated &&
            mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config &&
            mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup) {
          pucch_Config =  mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup;
        }
        else if (bwp_id==0 &&
                 mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
                 mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
                 mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config &&
                 mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup) {
          pucch_Config = mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup;
        }

        const int n_slots_frame = nr_slots_per_frame[scs];
        if (((n_slots_frame*frame + slot - offset)%period) == 0 && pucch_Config) {

          NR_PUCCH_CSI_Resource_t *pucchcsires = csirep->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list.array[0];
          NR_PUCCH_ResourceSet_t *pucchresset = pucch_Config->resourceSetToAddModList->list.array[1]; // set with formats >1
          int n = pucchresset->resourceList.list.count;

          int res_index;
          int found = -1;
          for (res_index = 0; res_index < n; res_index++) {
            if (*pucchresset->resourceList.list.array[res_index] == pucchcsires->pucch_Resource) {
              found = res_index;
              break;
            }
          }
          AssertFatal(found != -1,
                      "CSI resource not found among PUCCH resources\n");

          pucch->resource_indicator = found;
          csi_bits = nr_get_csi_payload(mac, pucch, csi_measconfig);
        }
      }
      else
        AssertFatal(1==0,"Only periodic CSI reporting is currently implemented\n");
    }
  }

  return csi_bits;
}


uint8_t nr_get_csi_payload(NR_UE_MAC_INST_t *mac,
                           PUCCH_sched_t *pucch,
                           NR_CSI_MeasConfig_t *csi_MeasConfig) {

  int n_csi_bits = 0;

  AssertFatal(csi_MeasConfig->csi_ReportConfigToAddModList->list.count>0,"No CSI Report configuration available\n");

  for (int csi_report_id=0; csi_report_id < csi_MeasConfig->csi_ReportConfigToAddModList->list.count; csi_report_id++){
    struct NR_CSI_ReportConfig *csi_reportconfig = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id];
    NR_CSI_ResourceConfigId_t csi_ResourceConfigId = csi_reportconfig->resourcesForChannelMeasurement;
    switch(csi_reportconfig->reportQuantity.present) {
      case NR_CSI_ReportConfig__reportQuantity_PR_none:
        break;
      case NR_CSI_ReportConfig__reportQuantity_PR_ssb_Index_RSRP:
        n_csi_bits += get_ssb_rsrp_payload(mac,pucch,csi_reportconfig,csi_ResourceConfigId,csi_MeasConfig);
        break;
      case NR_CSI_ReportConfig__reportQuantity_PR_cri_RSRP:
      case NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_PMI_CQI:
      case NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_i1:
      case NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_i1_CQI:
      case NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_CQI:
      case NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_LI_PMI_CQI:
        AssertFatal(1==0,"Measurement report based on CSI-RS not availalble\n");
      default:
        AssertFatal(1==0,"Invalid CSI report quantity type %d\n",csi_reportconfig->reportQuantity.present);
    }
  }
  return (n_csi_bits);
}


uint8_t get_ssb_rsrp_payload(NR_UE_MAC_INST_t *mac,
                             PUCCH_sched_t *pucch,
                             struct NR_CSI_ReportConfig *csi_reportconfig,
                             NR_CSI_ResourceConfigId_t csi_ResourceConfigId,
                             NR_CSI_MeasConfig_t *csi_MeasConfig) {

  int nb_ssb = 0;  // nb of ssb in the resource
  int nb_meas = 0; // nb of ssb to report measurements on
  int bits = 0;
  uint32_t temp_payload = 0;

  for (int csi_resourceidx = 0; csi_resourceidx < csi_MeasConfig->csi_ResourceConfigToAddModList->list.count; csi_resourceidx++) {
    struct NR_CSI_ResourceConfig *csi_resourceconfig = csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[csi_resourceidx];
    if (csi_resourceconfig->csi_ResourceConfigId == csi_ResourceConfigId) {

      if (csi_reportconfig->groupBasedBeamReporting.present == NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled) {
        if (csi_reportconfig->groupBasedBeamReporting.choice.disabled->nrofReportedRS != NULL)
          nb_meas = *(csi_reportconfig->groupBasedBeamReporting.choice.disabled->nrofReportedRS)+1;
        else
          nb_meas = 1;
      } else
        nb_meas = 2;

      for (int csi_ssb_idx = 0; csi_ssb_idx < csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.count; csi_ssb_idx++) {
        if (csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[csi_ssb_idx]->csi_SSB_ResourceSetId ==
            *(csi_resourceconfig->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList->list.array[0])){

          ///only one SSB resource set from spec 38.331 IE CSI-ResourceConfig
          nb_ssb = csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[csi_ssb_idx]->csi_SSB_ResourceList.list.count;
          break;
        }
      }

      AssertFatal(nb_ssb>0,"No SSB found in the resource set\n");
      int ssbri_bits = ceil(log2(nb_ssb));

      //TODO measurement of multiple SSBs at PHY and indication to MAC
      if(nb_ssb>1)
        LOG_E(MAC, "In current implementation only the SSB of synchronization is measured at PHY. This works only for a single SSB scenario\n");

      int ssb_rsrp[2][nb_meas]; // the array contains index and RSRP of each SSB to be reported (nb_meas highest RSRPs)

      //TODO replace the following 2 lines with a function to order the nb_meas highest SSB RSRPs
      ssb_rsrp[0][0] = mac->mib_ssb;
      ssb_rsrp[1][0] = mac->ssb_rsrp_dBm;

      uint8_t ssbi;

      if (ssbri_bits > 0) {
        ssbi = ssb_rsrp[0][0];
        reverse_n_bits(&ssbi, ssbri_bits);
        temp_payload = ssbi;
        bits += ssbri_bits;
      }

      uint8_t rsrp_idx = get_rsrp_index(ssb_rsrp[1][0]);
      reverse_n_bits(&rsrp_idx, 7);
      temp_payload |= (rsrp_idx<<bits);
      bits += 7; // 7 bits for highest RSRP

      // from the second SSB, differential report
      for (int i=1; i<nb_meas; i++){
        ssbi = ssb_rsrp[0][i];
        reverse_n_bits(&ssbi, ssbri_bits);
        temp_payload = ssbi;
        bits += ssbri_bits;

        rsrp_idx = get_rsrp_diff_index(ssb_rsrp[1][0],ssb_rsrp[1][i]);
        reverse_n_bits(&rsrp_idx, 4);
        temp_payload |= (rsrp_idx<<bits);
        bits += 4; // 7 bits for highest RSRP
      }
      break; // resorce found
    }
  }
  pucch->csi_part1_payload = temp_payload;
  return bits;
}


// returns index from RSRP
// according to Table 10.1.6.1-1 in 38.133

uint8_t get_rsrp_index(int rsrp) {

  int index = rsrp + 157;
  if (rsrp>-44)
    index = 113;
  if (rsrp<-140)
    index = 16;

  return index;
}


// returns index from differential RSRP
// according to Table 10.1.6.1-2 in 38.133
uint8_t get_rsrp_diff_index(int best_rsrp,int current_rsrp) {

  int diff = best_rsrp-current_rsrp;
  if (diff>30)
    return 15;
  else
    return (diff>>1);

}

void nr_ue_send_sdu(nr_downlink_indication_t *dl_info, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment, int pdu_id){

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_IN);

  #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
    LOG_T(MAC, "[UE %d] First 32 bytes of DLSCH : \n", module_idP);
    for (i = 0; i < 32; i++) {
      LOG_T(MAC, "%x.", sdu[i]);
    }
    LOG_T(MAC, "\n");
  #endif

  LOG_D(MAC, "In %s [%d.%d] Handling DLSCH PDU...\n", __FUNCTION__, dl_info->frame, dl_info->slot);

  // Processing MAC PDU
  // it parses MAC CEs subheaders, MAC CEs, SDU subheaderds and SDUs
  switch (dl_info->rx_ind->rx_indication_body[pdu_id].pdu_type){
    case FAPI_NR_RX_PDU_TYPE_DLSCH:
    nr_ue_process_mac_pdu(dl_info, ul_time_alignment, pdu_id);
    break;
    case FAPI_NR_RX_PDU_TYPE_RAR:
    nr_ue_process_rar(dl_info, ul_time_alignment, pdu_id);
    break;
    default:
    break;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_OUT);

}

// N_RB configuration according to 7.3.1.0 (DCI size alignment) of TS 38.212
int get_n_rb(NR_UE_MAC_INST_t *mac, int rnti_type){

  int N_RB = 0, start_RB;
  switch(rnti_type) {
    case NR_RNTI_RA:
    case NR_RNTI_TC:
    case NR_RNTI_P: {
      NR_BWP_Id_t dl_bwp_id = mac->DL_BWP_Id;
      if (mac->DLbwp[dl_bwp_id-1]->bwp_Common->pdcch_ConfigCommon->choice.setup->controlResourceSetZero) {
        uint8_t coreset_id = 0; // assuming controlResourceSetId is 0 for controlResourceSetZero
        NR_ControlResourceSet_t *coreset = mac->coreset[dl_bwp_id-1][coreset_id];
        get_coreset_rballoc(coreset->frequencyDomainResources.buf,&N_RB,&start_RB);
      } else {
        N_RB = NRRIV2BW(mac->scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
      }
      break;
    }
    case NR_RNTI_SI:
      N_RB = mac->type0_PDCCH_CSS_config.num_rbs;
      break;
    case NR_RNTI_C:
      N_RB = NRRIV2BW(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
      break;
  }
  return N_RB;

}

uint8_t nr_extract_dci_info(NR_UE_MAC_INST_t *mac,
                            uint8_t dci_format,
                            uint8_t dci_size,
                            uint16_t rnti,
                            uint64_t *dci_pdu,
                            dci_pdu_rel15_t *dci_pdu_rel15) {

  int N_RB = 0;
  int pos = 0;
  int fsize = 0;

  int rnti_type = get_rnti_type(mac, rnti);

  int N_RB_UL = 0;
  if(mac->scc_SIB) {
    N_RB_UL = NRRIV2BW(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  } else if(mac->ULbwp[0]) {
    N_RB_UL = NRRIV2BW(mac->ULbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  } else if(mac->scc) {
    N_RB_UL = NRRIV2BW(mac->scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  }

  LOG_D(MAC,"nr_extract_dci_info : dci_pdu %lx, size %d\n",*dci_pdu,dci_size);
  switch(dci_format) {

  case NR_DL_DCI_FORMAT_1_0:
    switch(rnti_type) {
    case NR_RNTI_RA:
      if(mac->scc_SIB) {
        N_RB = NRRIV2BW(mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
      } else {
        N_RB = get_n_rb(mac, rnti_type);
      }
      // Freq domain assignment
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      pos=fsize;
      dci_pdu_rel15->frequency_domain_assignment.val = *dci_pdu>>(dci_size-pos)&((1<<fsize)-1);
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"frequency-domain assignment %d (%d bits) N_RB_BWP %d=> %d (0x%lx)\n",dci_pdu_rel15->frequency_domain_assignment.val,fsize,N_RB,dci_size-pos,*dci_pdu);
#endif
      // Time domain assignment
      pos+=4;
      dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu >> (dci_size-pos))&0xf;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"time-domain assignment %d  (4 bits)=> %d (0x%lx)\n",dci_pdu_rel15->time_domain_assignment.val,dci_size-pos,*dci_pdu);
#endif
      // VRB to PRB mapping
	
      pos++;
      dci_pdu_rel15->vrb_to_prb_mapping.val = (*dci_pdu>>(dci_size-pos))&0x1;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"vrb to prb mapping %d  (1 bits)=> %d (0x%lx)\n",dci_pdu_rel15->vrb_to_prb_mapping.val,dci_size-pos,*dci_pdu);
#endif
      // MCS
      pos+=5;
      dci_pdu_rel15->mcs = (*dci_pdu>>(dci_size-pos))&0x1f;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"mcs %d  (5 bits)=> %d (0x%lx)\n",dci_pdu_rel15->mcs,dci_size-pos,*dci_pdu);
#endif
      // TB scaling
      pos+=2;
      dci_pdu_rel15->tb_scaling = (*dci_pdu>>(dci_size-pos))&0x3;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"tb_scaling %d  (2 bits)=> %d (0x%lx)\n",dci_pdu_rel15->tb_scaling,dci_size-pos,*dci_pdu);
#endif
      break;

    case NR_RNTI_C:

      //Identifier for DCI formats
      pos++;
      dci_pdu_rel15->format_indicator = (*dci_pdu>>(dci_size-pos))&1;

      //switch to DCI_0_0
      if (dci_pdu_rel15->format_indicator == 0)
        return 2+nr_extract_dci_info(mac, NR_UL_DCI_FORMAT_0_0, dci_size, rnti, dci_pdu, dci_pdu_rel15);

#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"Format indicator %d (%d bits) N_RB_BWP %d => %d (0x%lx)\n",dci_pdu_rel15->format_indicator,1,N_RB,dci_size-pos,*dci_pdu);
#endif

      // check BWP id
      if (mac->DLbwp[0]) N_RB=NRRIV2BW(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
      else         N_RB=NRRIV2BW(mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

      // Freq domain assignment (275rb >> fsize = 16)
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      pos+=fsize;
      dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<fsize)-1);
  	
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"Freq domain assignment %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->frequency_domain_assignment.val,fsize,dci_size-pos,*dci_pdu);
#endif
    	
      uint16_t is_ra = 1;
      for (int i=0; i<fsize; i++)
	if (!((dci_pdu_rel15->frequency_domain_assignment.val>>i)&1)) {
	  is_ra = 0;
	  break;
	}
      if (is_ra) //fsize are all 1  38.212 p86
	{
	  // ra_preamble_index 6 bits
	  pos+=6;
	  dci_pdu_rel15->ra_preamble_index = (*dci_pdu>>(dci_size-pos))&0x3f;
	    
	  // UL/SUL indicator  1 bit
	  pos++;
	  dci_pdu_rel15->ul_sul_indicator.val = (*dci_pdu>>(dci_size-pos))&1;
	    
	  // SS/PBCH index  6 bits
	  pos+=6;
	  dci_pdu_rel15->ss_pbch_index = (*dci_pdu>>(dci_size-pos))&0x3f;
	    
	  //  prach_mask_index  4 bits
	  pos+=4;
	  dci_pdu_rel15->prach_mask_index = (*dci_pdu>>(dci_size-pos))&0xf;
	    
	}  //end if
      else {
	  
	// Time domain assignment 4bit
		  
	pos+=4;
	dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&0xf;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"Time domain assignment %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->time_domain_assignment.val,4,dci_size-pos,*dci_pdu);
#endif
	  
	// VRB to PRB mapping  1bit
	pos++;
	dci_pdu_rel15->vrb_to_prb_mapping.val = (*dci_pdu>>(dci_size-pos))&1;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"VRB to PRB %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->vrb_to_prb_mapping.val,1,dci_size-pos,*dci_pdu);
#endif
	
	// MCS 5bit  //bit over 32, so dci_pdu ++
	pos+=5;
	dci_pdu_rel15->mcs = (*dci_pdu>>(dci_size-pos))&0x1f;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"MCS %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->mcs,5,dci_size-pos,*dci_pdu);
#endif
	  
	// New data indicator 1bit
	pos++;
	dci_pdu_rel15->ndi = (*dci_pdu>>(dci_size-pos))&1;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"NDI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->ndi,1,dci_size-pos,*dci_pdu);
#endif      
	  
	// Redundancy version  2bit
	pos+=2;
	dci_pdu_rel15->rv = (*dci_pdu>>(dci_size-pos))&0x3;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"RV %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->rv,2,dci_size-pos,*dci_pdu);
#endif
	  
	// HARQ process number  4bit
	pos+=4;
	dci_pdu_rel15->harq_pid  = (*dci_pdu>>(dci_size-pos))&0xf;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"HARQ_PID %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->harq_pid,4,dci_size-pos,*dci_pdu);
#endif
	  
	// Downlink assignment index  2bit
	pos+=2;
	dci_pdu_rel15->dai[0].val = (*dci_pdu>>(dci_size-pos))&3;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"DAI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->dai[0].val,2,dci_size-pos,*dci_pdu);
#endif
	  
	// TPC command for scheduled PUCCH  2bit
	pos+=2;
	dci_pdu_rel15->tpc = (*dci_pdu>>(dci_size-pos))&3;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"TPC %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->tpc,2,dci_size-pos,*dci_pdu);
#endif
	  
	// PUCCH resource indicator  3bit
	pos+=3;
	dci_pdu_rel15->pucch_resource_indicator = (*dci_pdu>>(dci_size-pos))&0x7;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"PUCCH RI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->pucch_resource_indicator,3,dci_size-pos,*dci_pdu);
#endif
	  
	// PDSCH-to-HARQ_feedback timing indicator 3bit
	pos+=3;
	dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val = (*dci_pdu>>(dci_size-pos))&0x7;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"PDSCH to HARQ TI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val,3,dci_size-pos,*dci_pdu);
#endif
	  
      } //end else
      break;

    case NR_RNTI_P:
      /*
      // Short Messages Indicator  E2 bits
      for (int i=0; i<2; i++)
      dci_pdu |= (((uint64_t)dci_pdu_rel15->short_messages_indicator>>(1-i))&1)<<(dci_size-pos++);
      // Short Messages  E8 bits
      for (int i=0; i<8; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->short_messages>>(7-i))&1)<<(dci_size-pos++);
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      for (int i=0; i<fsize; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_size-pos++);
      // Time domain assignment 4 bit
      for (int i=0; i<4; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_size-pos++);
      // VRB to PRB mapping 1 bit
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping.val&1)<<(dci_size-pos++);
      // MCS 5 bit
      for (int i=0; i<5; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs>>(4-i))&1)<<(dci_size-pos++);
	
      // TB scaling 2 bit
      for (int i=0; i<2; i++)
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->tb_scaling>>(1-i))&1)<<(dci_size-pos++);
      */	
	
      break;
  	
    case NR_RNTI_SI:
      N_RB = mac->type0_PDCCH_CSS_config.num_rbs;
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      pos+=fsize;
      dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<fsize)-1);

      // Time domain assignment 4 bit
      pos+=4;
      dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&0xf;

      // VRB to PRB mapping 1 bit
      pos++;
      dci_pdu_rel15->vrb_to_prb_mapping.val = (*dci_pdu>>(dci_size-pos))&0x1;

      // MCS 5bit  //bit over 32, so dci_pdu ++
      pos+=5;
      dci_pdu_rel15->mcs = (*dci_pdu>>(dci_size-pos))&0x1f;

      // Redundancy version  2 bit
      pos+=2;
      dci_pdu_rel15->rv = (*dci_pdu>>(dci_size-pos))&3;

      // System information indicator 1 bit
      pos++;
      dci_pdu_rel15->system_info_indicator = (*dci_pdu>>(dci_size-pos))&0x1;

      LOG_D(MAC,"N_RB = %i\n", N_RB);
      LOG_D(MAC,"dci_size = %i\n", dci_size);
      LOG_D(MAC,"fsize = %i\n", fsize);
      LOG_D(MAC,"dci_pdu_rel15->frequency_domain_assignment.val = %i\n", dci_pdu_rel15->frequency_domain_assignment.val);
      LOG_D(MAC,"dci_pdu_rel15->time_domain_assignment.val = %i\n", dci_pdu_rel15->time_domain_assignment.val);
      LOG_D(MAC,"dci_pdu_rel15->vrb_to_prb_mapping.val = %i\n", dci_pdu_rel15->vrb_to_prb_mapping.val);
      LOG_D(MAC,"dci_pdu_rel15->mcs = %i\n", dci_pdu_rel15->mcs);
      LOG_D(MAC,"dci_pdu_rel15->rv = %i\n", dci_pdu_rel15->rv);
      LOG_D(MAC,"dci_pdu_rel15->system_info_indicator = %i\n", dci_pdu_rel15->system_info_indicator);

      break;
	
    case NR_RNTI_TC:

      // check BWP id
      if (mac->DLbwp[0]) N_RB=NRRIV2BW(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
      else         N_RB=NRRIV2BW(mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

      // indicating a DL DCI format - 1 bit
      pos++;
      dci_pdu_rel15->format_indicator = (*dci_pdu>>(dci_size-pos))&1;

      if (dci_pdu_rel15->format_indicator == 0)
        return 1; // discard dci, format indicator not corresponding to dci_format

        // Freq domain assignment 0-16 bit
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      pos+=fsize;
      dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<fsize)-1);

      // Time domain assignment - 4 bits
      pos+=4;
      dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&0xf;

      // VRB to PRB mapping - 1 bit
      pos++;
      dci_pdu_rel15->vrb_to_prb_mapping.val = (*dci_pdu>>(dci_size-pos))&1;

      // MCS 5bit  //bit over 32, so dci_pdu ++
      pos+=5;
      dci_pdu_rel15->mcs = (*dci_pdu>>(dci_size-pos))&0x1f;

      // New data indicator - 1 bit
      pos++;
      dci_pdu_rel15->ndi = (*dci_pdu>>(dci_size-pos))&1;

      // Redundancy version - 2 bits
      pos+=2;
      dci_pdu_rel15->rv = (*dci_pdu>>(dci_size-pos))&3;

      // HARQ process number - 4 bits
      pos+=4;
      dci_pdu_rel15->harq_pid = (*dci_pdu>>(dci_size-pos))&0xf;

      // Downlink assignment index - 2 bits
      pos+=2;
      dci_pdu_rel15->dai[0].val = (*dci_pdu>>(dci_size-pos))&3;

      // TPC command for scheduled PUCCH - 2 bits
      pos+=2;
      dci_pdu_rel15->tpc  = (*dci_pdu>>(dci_size-pos))&3;

      // PUCCH resource indicator - 3 bits
      pos+=3;
      dci_pdu_rel15->pucch_resource_indicator = (*dci_pdu>>(dci_size-pos))&7;

      // PDSCH-to-HARQ_feedback timing indicator - 3 bits
      pos+=3;
      dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val = (*dci_pdu>>(dci_size-pos))&7;

      LOG_D(NR_MAC,"N_RB = %i\n", N_RB);
      LOG_D(NR_MAC,"dci_size = %i\n", dci_size);
      LOG_D(NR_MAC,"fsize = %i\n", fsize);
      LOG_D(NR_MAC,"dci_pdu_rel15->format_indicator = %i\n", dci_pdu_rel15->format_indicator);
      LOG_D(NR_MAC,"dci_pdu_rel15->frequency_domain_assignment.val = %i\n", dci_pdu_rel15->frequency_domain_assignment.val);
      LOG_D(NR_MAC,"dci_pdu_rel15->time_domain_assignment.val = %i\n", dci_pdu_rel15->time_domain_assignment.val);
      LOG_D(NR_MAC,"dci_pdu_rel15->vrb_to_prb_mapping.val = %i\n", dci_pdu_rel15->vrb_to_prb_mapping.val);
      LOG_D(NR_MAC,"dci_pdu_rel15->mcs = %i\n", dci_pdu_rel15->mcs);
      LOG_D(NR_MAC,"dci_pdu_rel15->rv = %i\n", dci_pdu_rel15->rv);
      LOG_D(NR_MAC,"dci_pdu_rel15->harq_pid = %i\n", dci_pdu_rel15->harq_pid);
      LOG_D(NR_MAC,"dci_pdu_rel15->dai[0].val = %i\n", dci_pdu_rel15->dai[0].val);
      LOG_D(NR_MAC,"dci_pdu_rel15->tpc = %i\n", dci_pdu_rel15->tpc);
      LOG_D(NR_MAC,"dci_pdu_rel15->pucch_resource_indicator = %i\n", dci_pdu_rel15->pucch_resource_indicator);
      LOG_D(NR_MAC,"dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val = %i\n", dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val);

      break;
    }
    break;
  
  case NR_UL_DCI_FORMAT_0_0:
    if (mac->ULbwp[0]) N_RB_UL=NRRIV2BW(mac->ULbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    else         N_RB_UL=NRRIV2BW(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

    switch(rnti_type)
      {
      case NR_RNTI_C:
        //Identifier for DCI formats
        pos++;
        dci_pdu_rel15->format_indicator = (*dci_pdu>>(dci_size-pos))&1;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"Format indicator %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->format_indicator,1,dci_size-pos,*dci_pdu);
#endif
        if (dci_pdu_rel15->format_indicator == 1)
          return 1; // discard dci, format indicator not corresponding to dci_format
	fsize = (int)ceil( log2( (N_RB_UL*(N_RB_UL+1))>>1 ) );
	pos+=fsize;
	dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<fsize)-1);
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"Freq domain assignment %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->frequency_domain_assignment.val,fsize,dci_size-pos,*dci_pdu);
#endif
	// Time domain assignment 4bit
	pos+=4;
	dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&0xf;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"time-domain assignment %d  (4 bits)=> %d (0x%lx)\n",dci_pdu_rel15->time_domain_assignment.val,dci_size-pos,*dci_pdu);
#endif
	// Frequency hopping flag  E1 bit
	pos++;
	dci_pdu_rel15->frequency_hopping_flag.val= (*dci_pdu>>(dci_size-pos))&1;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"frequency_hopping %d  (1 bit)=> %d (0x%lx)\n",dci_pdu_rel15->frequency_hopping_flag.val,dci_size-pos,*dci_pdu);
#endif
	// MCS  5 bit
	pos+=5;
	dci_pdu_rel15->mcs= (*dci_pdu>>(dci_size-pos))&0x1f;
#ifdef DEBUG_EXTRACT_DCI
      LOG_D(MAC,"mcs %d  (5 bits)=> %d (0x%lx)\n",dci_pdu_rel15->mcs,dci_size-pos,*dci_pdu);
#endif
	// New data indicator 1bit
	pos++;
	dci_pdu_rel15->ndi= (*dci_pdu>>(dci_size-pos))&1;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"NDI %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->ndi,1,dci_size-pos,*dci_pdu);
#endif
	// Redundancy version  2bit
	pos+=2;
	dci_pdu_rel15->rv= (*dci_pdu>>(dci_size-pos))&3;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"RV %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->rv,2,dci_size-pos,*dci_pdu);
#endif
	// HARQ process number  4bit
	pos+=4;
	dci_pdu_rel15->harq_pid = (*dci_pdu>>(dci_size-pos))&0xf;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"HARQ_PID %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->harq_pid,4,dci_size-pos,*dci_pdu);
#endif
	// TPC command for scheduled PUSCH  E2 bits
	pos+=2;
	dci_pdu_rel15->tpc = (*dci_pdu>>(dci_size-pos))&3;
#ifdef DEBUG_EXTRACT_DCI
	LOG_D(MAC,"TPC %d (%d bits)=> %d (0x%lx)\n",dci_pdu_rel15->tpc,2,dci_size-pos,*dci_pdu);
#endif
	// UL/SUL indicator  E1 bit
	/* commented for now (RK): need to get this from BWP descriptor
	   if (cfg->pucch_config.pucch_GroupHopping.value)
	   dci_pdu->= ((uint64_t)*dci_pdu>>(dci_size-pos)ul_sul_indicator&1)<<(dci_size-pos++);
	*/
	break;
	
      case NR_RNTI_TC:
	/*	
	// indicating a DL DCI format 1bit
	dci_pdu->= (*dci_pdu>>(dci_size-pos)format_indicator&1)<<(dci_size-pos++);
	// Freq domain assignment  max 16 bit
	fsize = (int)ceil( log2( (N_RB_UL*(N_RB_UL+1))>>1 ) );
	for (int i=0; i<fsize; i++)
	dci_pdu->= ((*dci_pdu>>(dci_size-pos)frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_size-pos++);
	// Time domain assignment 4bit
	for (int i=0; i<4; i++)
	dci_pdu->= (((uint64_t)*dci_pdu>>(dci_size-pos)time_domain_assignment>>(3-i))&1)<<(dci_size-pos++);
	// Frequency hopping flag  E1 bit
	dci_pdu->= ((uint64_t)*dci_pdu>>(dci_size-pos)frequency_hopping_flag&1)<<(dci_size-pos++);
	// MCS  5 bit
	for (int i=0; i<5; i++)
	dci_pdu->= (((uint64_t)*dci_pdu>>(dci_size-pos)mcs>>(4-i))&1)<<(dci_size-pos++);
	// New data indicator 1bit
	dci_pdu->= ((uint64_t)*dci_pdu>>(dci_size-pos)ndi&1)<<(dci_size-pos++);
	// Redundancy version  2bit
	for (int i=0; i<2; i++)
	dci_pdu->= (((uint64_t)*dci_pdu>>(dci_size-pos)rv>>(1-i))&1)<<(dci_size-pos++);
	// HARQ process number  4bit
	for (int i=0; i<4; i++)
	*dci_pdu  |= (((uint64_t)*dci_pdu>>(dci_size-pos)harq_pid>>(3-i))&1)<<(dci_size-pos++);
	
	// TPC command for scheduled PUSCH  E2 bits
	for (int i=0; i<2; i++)
	dci_pdu->= (((uint64_t)*dci_pdu>>(dci_size-pos)tpc>>(1-i))&1)<<(dci_size-pos++);
	*/	
	// UL/SUL indicator  E1 bit
	/*
	  commented for now (RK): need to get this information from BWP descriptor
	  if (cfg->pucch_config.pucch_GroupHopping.value)
	  dci_pdu->= ((uint64_t)dci_pdu_rel15->ul_sul_indicator&1)<<(dci_size-pos++);
	*/
	break;
	
      }
    break;

  case NR_DL_DCI_FORMAT_1_1:
  switch(rnti_type)
    {
      case NR_RNTI_C:
        //Identifier for DCI formats
        pos++;
        dci_pdu_rel15->format_indicator = (*dci_pdu>>(dci_size-pos))&1;
        if (dci_pdu_rel15->format_indicator == 0)
          return 1; // discard dci, format indicator not corresponding to dci_format
        // Carrier indicator
        pos+=dci_pdu_rel15->carrier_indicator.nbits;
        dci_pdu_rel15->carrier_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->carrier_indicator.nbits)-1);
        // BWP Indicator
        pos+=dci_pdu_rel15->bwp_indicator.nbits;
        dci_pdu_rel15->bwp_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->bwp_indicator.nbits)-1);
        // Frequency domain resource assignment
        pos+=dci_pdu_rel15->frequency_domain_assignment.nbits;
        dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->frequency_domain_assignment.nbits)-1);
        // Time domain resource assignment
        pos+=dci_pdu_rel15->time_domain_assignment.nbits;
        dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->time_domain_assignment.nbits)-1);
        // VRB-to-PRB mapping
        pos+=dci_pdu_rel15->vrb_to_prb_mapping.nbits;
        dci_pdu_rel15->vrb_to_prb_mapping.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->vrb_to_prb_mapping.nbits)-1);
        // PRB bundling size indicator
        pos+=dci_pdu_rel15->prb_bundling_size_indicator.nbits;
        dci_pdu_rel15->prb_bundling_size_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->prb_bundling_size_indicator.nbits)-1);
        // Rate matching indicator
        pos+=dci_pdu_rel15->rate_matching_indicator.nbits;
        dci_pdu_rel15->rate_matching_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->rate_matching_indicator.nbits)-1);
        // ZP CSI-RS trigger
        pos+=dci_pdu_rel15->zp_csi_rs_trigger.nbits;
        dci_pdu_rel15->zp_csi_rs_trigger.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->zp_csi_rs_trigger.nbits)-1);
        //TB1
        // MCS 5bit
        pos+=5;
        dci_pdu_rel15->mcs = (*dci_pdu>>(dci_size-pos))&0x1f;
        // New data indicator 1bit
        pos+=1;
        dci_pdu_rel15->ndi = (*dci_pdu>>(dci_size-pos))&0x1;
        // Redundancy version  2bit
        pos+=2;
        dci_pdu_rel15->rv = (*dci_pdu>>(dci_size-pos))&0x3;
        //TB2
        // MCS 5bit
        pos+=dci_pdu_rel15->mcs2.nbits;
        dci_pdu_rel15->mcs2.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->mcs2.nbits)-1);
        // New data indicator 1bit
        pos+=dci_pdu_rel15->ndi2.nbits;
        dci_pdu_rel15->ndi2.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->ndi2.nbits)-1);
        // Redundancy version  2bit
        pos+=dci_pdu_rel15->rv2.nbits;
        dci_pdu_rel15->rv2.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->rv2.nbits)-1);
        // HARQ process number  4bit
        pos+=4;
        dci_pdu_rel15->harq_pid = (*dci_pdu>>(dci_size-pos))&0xf;
        // Downlink assignment index
        pos+=dci_pdu_rel15->dai[0].nbits;
        dci_pdu_rel15->dai[0].val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->dai[0].nbits)-1);
        // TPC command for scheduled PUCCH  2bit
        pos+=2;
        dci_pdu_rel15->tpc = (*dci_pdu>>(dci_size-pos))&0x3;
        // PUCCH resource indicator  3bit
        pos+=3;
        dci_pdu_rel15->pucch_resource_indicator = (*dci_pdu>>(dci_size-pos))&0x3;
        // PDSCH-to-HARQ_feedback timing indicator
        pos+=dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.nbits;
        dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.nbits)-1);
        // Antenna ports
        pos+=dci_pdu_rel15->antenna_ports.nbits;
        dci_pdu_rel15->antenna_ports.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->antenna_ports.nbits)-1);
        // TCI
        pos+=dci_pdu_rel15->transmission_configuration_indication.nbits;
        dci_pdu_rel15->transmission_configuration_indication.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->transmission_configuration_indication.nbits)-1);
        // SRS request
        pos+=dci_pdu_rel15->srs_request.nbits;
        dci_pdu_rel15->srs_request.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->srs_request.nbits)-1);
        // CBG transmission information
        pos+=dci_pdu_rel15->cbgti.nbits;
        dci_pdu_rel15->cbgti.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->cbgti.nbits)-1);
        // CBG flushing out information
        pos+=dci_pdu_rel15->cbgfi.nbits;
        dci_pdu_rel15->cbgfi.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->cbgfi.nbits)-1);
        // DMRS sequence init
        pos+=1;
        dci_pdu_rel15->dmrs_sequence_initialization.val = (*dci_pdu>>(dci_size-pos))&0x1;
        break;
      }
      break;

  case NR_UL_DCI_FORMAT_0_1:
    switch(rnti_type)
      {
      case NR_RNTI_C:
        //Identifier for DCI formats
        pos++;
        dci_pdu_rel15->format_indicator = (*dci_pdu>>(dci_size-pos))&1;
        if (dci_pdu_rel15->format_indicator == 1)
          return 1; // discard dci, format indicator not corresponding to dci_format
        // Carrier indicator
        pos+=dci_pdu_rel15->carrier_indicator.nbits;
        dci_pdu_rel15->carrier_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->carrier_indicator.nbits)-1);
        
        // UL/SUL Indicator
        pos+=dci_pdu_rel15->ul_sul_indicator.nbits;
        dci_pdu_rel15->ul_sul_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->ul_sul_indicator.nbits)-1);
        
        // BWP Indicator
        pos+=dci_pdu_rel15->bwp_indicator.nbits;
        dci_pdu_rel15->bwp_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->bwp_indicator.nbits)-1);

        // Freq domain assignment  max 16 bit
        fsize = (int)ceil( log2( (N_RB_UL*(N_RB_UL+1))>>1 ) );
        pos+=fsize;
        dci_pdu_rel15->frequency_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&((1<<fsize)-1);
        
        // Time domain assignment 4bit
        //pos+=4;
        pos+=dci_pdu_rel15->time_domain_assignment.nbits;
        dci_pdu_rel15->time_domain_assignment.val = (*dci_pdu>>(dci_size-pos))&0x3;
        
        // Not supported yet - skip for now
        // Frequency hopping flag – 1 bit 
        //pos++;
        //dci_pdu_rel15->frequency_hopping_flag.val= (*dci_pdu>>(dci_size-pos))&1;

        // MCS  5 bit
        pos+=5;
        dci_pdu_rel15->mcs= (*dci_pdu>>(dci_size-pos))&0x1f;

        // New data indicator 1bit
        pos++;
        dci_pdu_rel15->ndi= (*dci_pdu>>(dci_size-pos))&1;

        // Redundancy version  2bit
        pos+=2;
        dci_pdu_rel15->rv= (*dci_pdu>>(dci_size-pos))&3;

        // HARQ process number  4bit
        pos+=4;
        dci_pdu_rel15->harq_pid = (*dci_pdu>>(dci_size-pos))&0xf;

        // 1st Downlink assignment index
        pos+=dci_pdu_rel15->dai[0].nbits;
        dci_pdu_rel15->dai[0].val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->dai[0].nbits)-1);

        // 2nd Downlink assignment index
        pos+=dci_pdu_rel15->dai[1].nbits;
        dci_pdu_rel15->dai[1].val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->dai[1].nbits)-1);

        // TPC command for scheduled PUSCH – 2 bits
        pos+=2;
        dci_pdu_rel15->tpc = (*dci_pdu>>(dci_size-pos))&3;

        // SRS resource indicator
        pos+=dci_pdu_rel15->srs_resource_indicator.nbits;
        dci_pdu_rel15->srs_resource_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->srs_resource_indicator.nbits)-1);

        // Precoding info and n. of layers
        pos+=dci_pdu_rel15->precoding_information.nbits;
        dci_pdu_rel15->precoding_information.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->precoding_information.nbits)-1);

        // Antenna ports
        pos+=dci_pdu_rel15->antenna_ports.nbits;
        dci_pdu_rel15->antenna_ports.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->antenna_ports.nbits)-1);

        // SRS request
        pos+=dci_pdu_rel15->srs_request.nbits;
        dci_pdu_rel15->srs_request.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->srs_request.nbits)-1);

        // CSI request
        pos+=dci_pdu_rel15->csi_request.nbits;
        dci_pdu_rel15->csi_request.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->csi_request.nbits)-1);

        // CBG transmission information
        pos+=dci_pdu_rel15->cbgti.nbits;
        dci_pdu_rel15->cbgti.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->cbgti.nbits)-1);

        // PTRS DMRS association
        pos+=dci_pdu_rel15->ptrs_dmrs_association.nbits;
        dci_pdu_rel15->ptrs_dmrs_association.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->ptrs_dmrs_association.nbits)-1);

        // Beta offset indicator
        pos+=dci_pdu_rel15->beta_offset_indicator.nbits;
        dci_pdu_rel15->beta_offset_indicator.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->beta_offset_indicator.nbits)-1);

        // DMRS sequence initialization
        pos+=dci_pdu_rel15->dmrs_sequence_initialization.nbits;
        dci_pdu_rel15->dmrs_sequence_initialization.val = (*dci_pdu>>(dci_size-pos))&((1<<dci_pdu_rel15->dmrs_sequence_initialization.nbits)-1);

        // UL-SCH indicator
        pos+=1;
        dci_pdu_rel15->ulsch_indicator = (*dci_pdu>>(dci_size-pos))&0x1;

        // UL/SUL indicator – 1 bit
        /* commented for now (RK): need to get this from BWP descriptor
          if (cfg->pucch_config.pucch_GroupHopping.value)
          dci_pdu->= ((uint64_t)*dci_pdu>>(dci_size-pos)ul_sul_indicator&1)<<(dci_size-pos++);
        */
        break;
      }
    break;
       }
    
    return 0;
}

///////////////////////////////////
// brief:     nr_ue_process_mac_pdu
// function:  parsing DL PDU header
///////////////////////////////////
//  Header for DLSCH:
//  Except:
//   - DL-SCH: fixed-size MAC CE(known by LCID)
//   - DL-SCH: padding
//
//  |0|1|2|3|4|5|6|7|  bit-wise
//  |R|F|   LCID    |
//  |       L       |
//  |0|1|2|3|4|5|6|7|  bit-wise
//  |R|F|   LCID    |
//  |       L       |
//  |       L       |
////////////////////////////////
//  Header for DLSCH:
//   - DLSCH: fixed-size MAC CE(known by LCID)
//   - DLSCH: padding, for single/multiple 1-oct padding CE(s)
//
//  |0|1|2|3|4|5|6|7|  bit-wise
//  |R|R|   LCID    |
//  LCID: The Logical Channel ID field identifies the logical channel instance of the corresponding MAC SDU or the type of the corresponding MAC CE or padding as described
//         in Tables 6.2.1-1 and 6.2.1-2 for the DL-SCH and UL-SCH respectively. There is one LCID field per MAC subheader. The LCID field size is 6 bits;
//  L:    The Length field indicates the length of the corresponding MAC SDU or variable-sized MAC CE in bytes. There is one L field per MAC subheader except for subheaders
//         corresponding to fixed-sized MAC CEs and padding. The size of the L field is indicated by the F field;
//  F:    lenght of L is 0:8 or 1:16 bits wide
//  R:    Reserved bit, set to zero.
////////////////////////////////
void nr_ue_process_mac_pdu(nr_downlink_indication_t *dl_info,
                           NR_UL_TIME_ALIGNMENT_t *ul_time_alignment,
                           int pdu_id){

  uint8_t rx_lcid;
  uint16_t mac_ce_len;
  uint16_t mac_subheader_len;
  uint16_t mac_sdu_len;
  module_id_t module_idP = dl_info->module_id;
  frame_t frameP         = dl_info->frame;
  int slot               = dl_info->slot;
  uint8_t *pduP          = (dl_info->rx_ind->rx_indication_body + pdu_id)->pdsch_pdu.pdu;
  int16_t pdu_len        = (int16_t)(dl_info->rx_ind->rx_indication_body + pdu_id)->pdsch_pdu.pdu_length;
  uint8_t gNB_index      = dl_info->gNB_index;
  uint8_t CC_id          = dl_info->cc_id;
  uint8_t done           = 0;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_idP);
  RA_config_t *ra = &mac->ra;

  if (!pduP){
    return;
  }

  LOG_D(MAC, "In %s [%d.%d]: processing PDU %d (with length %d) of %d total number of PDUs...\n", __FUNCTION__, frameP, slot, pdu_id, pdu_len, dl_info->rx_ind->number_pdus);

    while (!done && pdu_len > 0){
        mac_ce_len = 0x0000;
        mac_subheader_len = 0x0001; //  default to fixed-length subheader = 1-oct
        mac_sdu_len = 0x0000;
        rx_lcid = ((NR_MAC_SUBHEADER_FIXED *)pduP)->LCID;

        LOG_D(MAC, "[UE] LCID %d, PDU length %d\n", rx_lcid, pdu_len);
        switch(rx_lcid){
            //  MAC CE

            case DL_SCH_LCID_CCCH:
              //  MSG4 RRC Setup 38.331
              //  variable length
              if(((NR_MAC_SUBHEADER_SHORT *)pduP)->F){
                mac_sdu_len = ((uint16_t)(((NR_MAC_SUBHEADER_LONG *) pduP)->L1 & 0x7f) << 8)
                              | ((uint16_t)((NR_MAC_SUBHEADER_LONG *) pduP)->L2 & 0xff);
                mac_subheader_len = 3;
              } else {
                mac_sdu_len = ((NR_MAC_SUBHEADER_SHORT *) pduP)->L;
                mac_subheader_len = 2;
              }

              // Check if it is a valid CCCH message, we get all 00's messages very often
              int i = 0;
              for(i=0; i<(mac_subheader_len+mac_sdu_len); i++) {
                if(pduP[i] != 0) {
                  break;
                }
              }
              if (i == (mac_subheader_len+mac_sdu_len)) {
                LOG_D(NR_MAC, "%s() Invalid CCCH message!, pdu_len: %d\n", __func__, pdu_len);
                done = 1;
                break;
              }

              if ( mac_sdu_len > 0 ) {
                LOG_D(NR_MAC,"DL_SCH_LCID_CCCH (e.g. RRCSetup) with payload len %d\n", mac_sdu_len);
                for (int i = 0; i < mac_subheader_len; i++) {
                  LOG_D(NR_MAC, "MAC header %d: 0x%x\n", i, pduP[i]);
                }
                for (int i = 0; i < mac_sdu_len; i++) {
                  LOG_D(NR_MAC, "%d: 0x%x\n", i, pduP[mac_subheader_len + i]);
                }
                nr_mac_rrc_data_ind_ue(module_idP, CC_id, gNB_index, frameP, 0, mac->crnti, CCCH, pduP+mac_subheader_len, mac_sdu_len);
              }
              break;

            case DL_SCH_LCID_TCI_STATE_ACT_UE_SPEC_PDSCH:

                //  38.321 Ch6.1.3.14
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pduP)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pduP)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pduP)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_APERIODIC_CSI_TRI_STATE_SUBSEL:
                //  38.321 Ch6.1.3.13
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pduP)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pduP)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pduP)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_SP_CSI_RS_CSI_IM_RES_SET_ACT:
                //  38.321 Ch6.1.3.12
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pduP)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pduP)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pduP)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_SP_SRS_ACTIVATION:
                //  38.321 Ch6.1.3.17
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pduP)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pduP)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pduP)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            
            case DL_SCH_LCID_RECOMMENDED_BITRATE:
                //  38.321 Ch6.1.3.20
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_SP_ZP_CSI_RS_RES_SET_ACT:
                //  38.321 Ch6.1.3.19
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_PUCCH_SPATIAL_RELATION_ACT:
                //  38.321 Ch6.1.3.18
                mac_ce_len = 3;
                break;
            case DL_SCH_LCID_SP_CSI_REP_PUCCH_ACT:
                //  38.321 Ch6.1.3.16
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_TCI_STATE_IND_UE_SPEC_PDCCH:
                //  38.321 Ch6.1.3.15
                mac_ce_len = 2;
                break;
            case DL_SCH_LCID_DUPLICATION_ACT:
                //  38.321 Ch6.1.3.11
                mac_ce_len = 1;
                break;
            case DL_SCH_LCID_SCell_ACT_4_OCT:
                //  38.321 Ch6.1.3.10
                mac_ce_len = 4;
                break;
            case DL_SCH_LCID_SCell_ACT_1_OCT:
                //  38.321 Ch6.1.3.10
                mac_ce_len = 1;
                break;
            case DL_SCH_LCID_L_DRX:
                //  38.321 Ch6.1.3.6
                //  fixed length but not yet specify.
                mac_ce_len = 0;
                break;
            case DL_SCH_LCID_DRX:
                //  38.321 Ch6.1.3.5
                //  fixed length but not yet specify.
                mac_ce_len = 0;
                break;
            case DL_SCH_LCID_TA_COMMAND:
                //  38.321 Ch6.1.3.4
                mac_ce_len = 1;

                /*uint8_t ta_command = ((NR_MAC_CE_TA *)pduP)[1].TA_COMMAND;
                uint8_t tag_id = ((NR_MAC_CE_TA *)pduP)[1].TAGID;*/

                ul_time_alignment->apply_ta = 1;
                ul_time_alignment->ta_command = ((NR_MAC_CE_TA *)pduP)[1].TA_COMMAND;
                ul_time_alignment->tag_id = ((NR_MAC_CE_TA *)pduP)[1].TAGID;

                /*
                #ifdef DEBUG_HEADER_PARSING
                LOG_D(MAC, "[UE] CE %d : UE Timing Advance : %d\n", i, pduP[1]);
                #endif
                */

                LOG_I(MAC, "[%d.%d] Received TA_COMMAND %u TAGID %u CC_id %d\n", frameP, slot, ul_time_alignment->ta_command, ul_time_alignment->tag_id, CC_id);

                break;
            case DL_SCH_LCID_CON_RES_ID:
              //  Clause 5.1.5 and 6.1.3.3 of 3GPP TS 38.321 version 16.2.1 Release 16
              // MAC Header: 1 byte (R/R/LCID)
              // MAC SDU: 6 bytes (UE Contention Resolution Identity)
              mac_ce_len = 6;

              if(ra->ra_state == WAIT_CONTENTION_RESOLUTION) {
                LOG_I(MAC, "[UE %d][RAPROC] Frame %d : received contention resolution identity: 0x%02x%02x%02x%02x%02x%02x Terminating RA procedure\n",
                      module_idP, frameP, pduP[1], pduP[2], pduP[3], pduP[4], pduP[5], pduP[6]);

                bool ra_success = true;
                for(int i = 0; i<mac_ce_len; i++) {
                  if(ra->cont_res_id[i] != pduP[i+1]) {
                    ra_success = false;
                    break;
                  }
                }

                if ( (ra->RA_active == 1) && ra_success) {
                  nr_ra_succeeded(module_idP, frameP, slot);
                } else if (!ra_success){
                  // TODO: Handle failure of RA procedure @ MAC layer
                  //  nr_ra_failed(module_idP, CC_id, prach_resources, frameP, slot); // prach_resources is a PHY structure
                  ra->ra_state = RA_UE_IDLE;
                  ra->RA_active = 0;
                }
              }

              break;
            case DL_SCH_LCID_PADDING:
                done = 1;
                //  end of MAC PDU, can ignore the rest.
                break;

            //  MAC SDU

            case DL_SCH_LCID_DCCH:
                //  check if LCID is valid at current time.

            case DL_SCH_LCID_DCCH1:
                //  check if LCID is valid at current time.

            default:
                //  check if LCID is valid at current time.
                if(((NR_MAC_SUBHEADER_SHORT *)pduP)->F){
                    //mac_sdu_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pduP)->L2)<<8;
                    mac_subheader_len = 3;
                    mac_sdu_len = ((uint16_t)(((NR_MAC_SUBHEADER_LONG *) pduP)->L1 & 0x7f) << 8)
                    | ((uint16_t)((NR_MAC_SUBHEADER_LONG *) pduP)->L2 & 0xff);

                } else {
                  mac_sdu_len = (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pduP)->L;
                  mac_subheader_len = 2;
                }

                LOG_D(MAC, "[UE %d] Frame %d : DLSCH -> DL-DTCH %d (gNB %d, %d bytes)\n", module_idP, frameP, rx_lcid, gNB_index, mac_sdu_len);

                #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
                    LOG_T(MAC, "[UE %d] First 32 bytes of DLSCH : \n", module_idP);

                    for (i = 0; i < 32; i++)
                      LOG_T(MAC, "%x.", (pduP + mac_subheader_len)[i]);

                    LOG_T(MAC, "\n");
                #endif

//                if (IS_SOFTMODEM_NOS1){
                  if (rx_lcid < NB_RB_MAX && rx_lcid >= DL_SCH_LCID_DCCH) {

                    mac_rlc_data_ind(module_idP,
                                     mac->crnti,
                                     gNB_index,
                                     frameP,
                                     ENB_FLAG_NO,
                                     MBMS_FLAG_NO,
                                     rx_lcid,
                                     (char *) (pduP + mac_subheader_len),
                                     mac_sdu_len,
                                     1,
                                     NULL);
                  } else {
                    LOG_E(MAC, "[UE %d] Frame %d : unknown LCID %d (gNB %d)\n", module_idP, frameP, rx_lcid, gNB_index);
                  }
//                }

            break;
        }
        pduP += ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        pdu_len -= ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        if (pdu_len < 0)
          LOG_E(MAC, "[UE %d][%d.%d] nr_ue_process_mac_pdu, residual mac pdu length %d < 0!\n", module_idP, frameP, slot, pdu_len);
    }
}

////////////////////////////////////////////////////////
/////* ULSCH MAC PDU generation (6.1.2 TS 38.321) */////
////////////////////////////////////////////////////////

uint16_t nr_generate_ulsch_pdu(uint8_t *sdus_payload,
                                    uint8_t *pdu,
                                    uint8_t num_sdus,
                                    uint16_t *sdu_lengths,
                                    uint8_t *sdu_lcids,
                                    uint8_t power_headroom,
                                    uint16_t crnti,
                                    uint16_t truncated_bsr,
                                    uint16_t short_bsr,
                                    uint16_t long_bsr,
                                    unsigned short post_padding,
                                    uint16_t buflen) {

  NR_MAC_SUBHEADER_FIXED *mac_pdu_ptr = (NR_MAC_SUBHEADER_FIXED *) pdu;

  LOG_D(MAC, "[UE] Generating ULSCH PDU : num_sdus %d\n", num_sdus);

  #ifdef DEBUG_HEADER_PARSING

    for (int i = 0; i < num_sdus; i++)
      LOG_D(MAC, "[UE] MAC subPDU %d (lcid %d length %d bytes \n", i, sdu_lcids[i], sdu_lengths[i]);

  #endif

  // Generating UL MAC subPDUs including MAC SDU and subheader

  for (int i = 0; i < num_sdus; i++) {
    LOG_D(MAC, "[UE] Generating UL MAC subPDUs for SDU with lenght %d ( num_sdus %d )\n", sdu_lengths[i], num_sdus);

    if (sdu_lcids[i] != UL_SCH_LCID_CCCH){
      if (sdu_lengths[i] < 128) {
        ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->R = 0;
        ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->F = 0;
        ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->LCID = sdu_lcids[i];
        ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->L = (unsigned char) sdu_lengths[i];
        mac_pdu_ptr += sizeof(NR_MAC_SUBHEADER_SHORT);
      } else {
        ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->R = 0;
        ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->F = 1;
        ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->LCID = sdu_lcids[i];
        ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->L1 = ((unsigned short) sdu_lengths[i] >> 8) & 0x7f;
        ((NR_MAC_SUBHEADER_LONG *) mac_pdu_ptr)->L2 = (unsigned short) sdu_lengths[i] & 0xff;
        mac_pdu_ptr += sizeof(NR_MAC_SUBHEADER_LONG);
      }
    } else { // UL CCCH SDU
      mac_pdu_ptr->R = 0;
      mac_pdu_ptr->LCID = sdu_lcids[i];
      mac_pdu_ptr ++;
    }

    // cycle through SDUs, compute each relevant and place ulsch_buffer in
    memcpy((void *) mac_pdu_ptr, (void *) sdus_payload, sdu_lengths[i]);
    sdus_payload += sdu_lengths[i]; 
    mac_pdu_ptr  += sdu_lengths[i];
  }

  // Generating UL MAC subPDUs including MAC CEs (MAC CE and subheader)

  if (power_headroom) {
    // MAC CE fixed subheader
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = UL_SCH_LCID_SINGLE_ENTRY_PHR;
    mac_pdu_ptr++;

    // PHR MAC CE (1 octet)
    ((NR_SINGLE_ENTRY_PHR_MAC_CE *) mac_pdu_ptr)->PH = power_headroom;
    ((NR_SINGLE_ENTRY_PHR_MAC_CE *) mac_pdu_ptr)->R1 = 0;
    ((NR_SINGLE_ENTRY_PHR_MAC_CE *) mac_pdu_ptr)->PCMAX = 0; // todo
    ((NR_SINGLE_ENTRY_PHR_MAC_CE *) mac_pdu_ptr)->R2 = 0;
    mac_pdu_ptr += sizeof(NR_SINGLE_ENTRY_PHR_MAC_CE);
  }

  if (crnti) {
    // MAC CE fixed subheader
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = UL_SCH_LCID_C_RNTI;
    mac_pdu_ptr++;

    // C-RNTI MAC CE (2 octets)
    * (uint16_t *) mac_pdu_ptr = crnti;
    mac_pdu_ptr += sizeof(uint16_t);
  }

  if (truncated_bsr) {
    // MAC CE fixed subheader
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = UL_SCH_LCID_S_TRUNCATED_BSR;
    mac_pdu_ptr++;

    // Short truncated BSR MAC CE (1 octet)
    ((NR_BSR_SHORT_TRUNCATED *) mac_pdu_ptr)-> Buffer_size = truncated_bsr;
    ((NR_BSR_SHORT_TRUNCATED *) mac_pdu_ptr)-> LcgID = 0; // todo
    mac_pdu_ptr+= sizeof(NR_BSR_SHORT_TRUNCATED);
  } else if (short_bsr) {
    // MAC CE fixed subheader
    mac_pdu_ptr->R = 0;
    mac_pdu_ptr->LCID = UL_SCH_LCID_S_BSR;
    mac_pdu_ptr++;

    // Short truncated BSR MAC CE (1 octet)
    ((NR_BSR_SHORT *) mac_pdu_ptr)->Buffer_size = short_bsr;
    ((NR_BSR_SHORT *) mac_pdu_ptr)->LcgID = 0; // todo
     mac_pdu_ptr+= sizeof(NR_BSR_SHORT);
  } else if (long_bsr) {
    // MAC CE variable subheader
    // todo ch 6.1.3.1. TS 38.321
    // ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->R = 0;
    // ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->F = 0;
    // ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->LCID = UL_SCH_LCID_L_BSR;
    // ((NR_MAC_SUBHEADER_SHORT *) mac_pdu_ptr)->L = 0;
    // last_size = 2;
    // mac_pdu_ptr += last_size;

    // Short truncated BSR MAC CE (1 octet)
    // ((NR_BSR_LONG *) ce_ptr)->Buffer_size0 = short_bsr;
    // ((NR_BSR_LONG *) ce_ptr)->LCGID0 = 0;
    // mac_ce_size = sizeof(NR_BSR_LONG); // size is variable
  }
// compute offset before adding padding (if necessary)
  int padding_bytes = 0; 

  if(buflen > 0) // If the buflen is provided
    padding_bytes = buflen + pdu - (unsigned char *) mac_pdu_ptr;

  AssertFatal(padding_bytes>=0,"");

  // Compute final offset for padding
  if (post_padding || padding_bytes>0) {
    ((NR_MAC_SUBHEADER_FIXED *) mac_pdu_ptr)->R = 0;
    ((NR_MAC_SUBHEADER_FIXED *) mac_pdu_ptr)->LCID = UL_SCH_LCID_PADDING;
    mac_pdu_ptr++;
  } 
  return (uint8_t *)mac_pdu_ptr-pdu;
}

/////////////////////////////////////
//    Random Access Response PDU   //
//         TS 38.213 ch 8.2        //
//        TS 38.321 ch 6.2.3       //
/////////////////////////////////////
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |// bit-wise
//| E | T |       R A P I D       |//
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |//
//| R |           T A             |//
//|       T A         |  UL grant |//
//|            UL grant           |//
//|            UL grant           |//
//|            UL grant           |//
//|         T C - R N T I         |//
//|         T C - R N T I         |//
/////////////////////////////////////
//       UL grant  (27 bits)       //
/////////////////////////////////////
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |// bit-wise
//|-------------------|FHF|F_alloc|//
//|        Freq allocation        |//
//|    F_alloc    |Time allocation|//
//|      MCS      |     TPC   |CSI|//
/////////////////////////////////////
// TbD WIP Msg3 development ongoing
// - apply UL grant freq alloc & time alloc as per 8.2 TS 38.213
// - apply tpc command
// WIP fix:
// - time domain indication hardcoded to 0 for k2 offset
// - extend TS 38.213 ch 8.3 Msg3 PUSCH
// - b buffer
// - ulsch power offset
// - optimize: mu_pusch, j and table_6_1_2_1_1_2_time_dom_res_alloc_A are already defined in nr_ue_procedures
int nr_ue_process_rar(nr_downlink_indication_t *dl_info, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment, int pdu_id){

  module_id_t mod_id       = dl_info->module_id;
  frame_t frame            = dl_info->frame;
  int slot                 = dl_info->slot;

  if(dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.ack_nack == 0) {
    LOG_W(NR_MAC,"[UE %d][RAPROC][%d.%d] CRC check failed on RAR (NAK)\n", mod_id, frame, slot);
    return 0;
  }

  int cc_id                = dl_info->cc_id;
  uint8_t gNB_id           = dl_info->gNB_index;
  NR_UE_MAC_INST_t *mac    = get_mac_inst(mod_id);
  RA_config_t *ra          = &mac->ra;
  uint8_t n_subPDUs        = 0;  // number of RAR payloads
  uint8_t n_subheaders     = 0;  // number of MAC RAR subheaders
  uint8_t *dlsch_buffer    = dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.pdu;
  uint8_t is_Msg3          = 1;
  frame_t frame_tx         = 0;
  int slot_tx              = 0;
  uint16_t rnti            = 0;
  int ret                  = 0;
  NR_RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *) dlsch_buffer; // RAR subheader pointer
  NR_MAC_RAR *rar          = (NR_MAC_RAR *) (dlsch_buffer + 1);   // RAR subPDU pointer
  uint8_t preamble_index   = get_ra_PreambleIndex(mod_id, cc_id, gNB_id); //prach_resources->ra_PreambleIndex;

  LOG_D(NR_MAC, "In %s:[%d.%d]: [UE %d][RAPROC] invoking MAC for received RAR (current preamble %d)\n", __FUNCTION__, frame, slot, mod_id, preamble_index);

  while (1) {
    n_subheaders++;
    if (rarh->T == 1) {
      n_subPDUs++;
      LOG_I(NR_MAC, "[UE %d][RAPROC] Got RAPID RAR subPDU\n", mod_id);
    } else {
      ra->RA_backoff_indicator = table_7_2_1[((NR_RA_HEADER_BI *)rarh)->BI];
      ra->RA_BI_found = 1;
      LOG_I(NR_MAC, "[UE %d][RAPROC] Got BI RAR subPDU %d ms\n", mod_id, ra->RA_backoff_indicator);
      if ( ((NR_RA_HEADER_BI *)rarh)->E == 1) {
        rarh += sizeof(NR_RA_HEADER_BI);
        continue;
      } else {
        break;
      }
    }
    if (rarh->RAPID == preamble_index) {
      LOG_I(NR_MAC, "[UE %d][RAPROC][%d.%d] Found RAR with the intended RAPID %d\n", mod_id, frame, slot, rarh->RAPID);
      rar = (NR_MAC_RAR *) (dlsch_buffer + n_subheaders + (n_subPDUs - 1) * sizeof(NR_MAC_RAR));
      ra->RA_RAPID_found = 1;
      break;
    }
    if (rarh->E == 0) {
      LOG_W(NR_MAC,"[UE %d][RAPROC][%d.%d] Received RAR preamble (%d) doesn't match the intended RAPID (%d)\n", mod_id, frame, slot, rarh->RAPID, preamble_index);
      break;
    } else {
      rarh += sizeof(NR_MAC_RAR) + 1;
    }
  }

  #ifdef DEBUG_RAR
  LOG_D(MAC, "[DEBUG_RAR] (%d,%d) number of RAR subheader %d; number of RAR pyloads %d\n", frame, slot, n_subheaders, n_subPDUs);
  LOG_D(MAC, "[DEBUG_RAR] Received RAR (%02x|%02x.%02x.%02x.%02x.%02x.%02x) for preamble %d/%d\n", *(uint8_t *) rarh, rar[0], rar[1], rar[2], rar[3], rar[4], rar[5], rarh->RAPID, preamble_index);
  #endif

  if (ra->RA_RAPID_found) {

    RAR_grant_t rar_grant;

    unsigned char tpc_command;
#ifdef DEBUG_RAR
    unsigned char csi_req;
#endif

    // TC-RNTI
    ra->t_crnti = rar->TCRNTI_2 + (rar->TCRNTI_1 << 8);

    // TA command
    ul_time_alignment->apply_ta = 1;
    ul_time_alignment->ta_command = 31 + rar->TA2 + (rar->TA1 << 5);

#ifdef DEBUG_RAR
    // CSI
    csi_req = (unsigned char) (rar->UL_GRANT_4 & 0x01);
#endif

    // TPC
    tpc_command = (unsigned char) ((rar->UL_GRANT_4 >> 1) & 0x07);
    switch (tpc_command){
      case 0:
        ra->Msg3_TPC = -6;
        break;
      case 1:
        ra->Msg3_TPC = -4;
        break;
      case 2:
        ra->Msg3_TPC = -2;
        break;
      case 3:
        ra->Msg3_TPC = 0;
        break;
      case 4:
        ra->Msg3_TPC = 2;
        break;
      case 5:
        ra->Msg3_TPC = 4;
        break;
      case 6:
        ra->Msg3_TPC = 6;
        break;
      case 7:
        ra->Msg3_TPC = 8;
        break;
    }
    // MCS
    rar_grant.mcs = (unsigned char) (rar->UL_GRANT_4 >> 4);
    // time alloc
    rar_grant.Msg3_t_alloc = (unsigned char) (rar->UL_GRANT_3 & 0x07);
    // frequency alloc
    rar_grant.Msg3_f_alloc = (uint16_t) ((rar->UL_GRANT_3 >> 4) | (rar->UL_GRANT_2 << 4) | ((rar->UL_GRANT_1 & 0x03) << 12));
    // frequency hopping
    rar_grant.freq_hopping = (unsigned char) (rar->UL_GRANT_1 >> 2);
    // TC-RNTI
    if (ra->t_crnti) {
      rnti = ra->t_crnti;
    } else {
      rnti = mac->crnti;
    }

#ifdef DEBUG_RAR
    LOG_I(NR_MAC, "rarh->E = 0x%x\n", rarh->E);
    LOG_I(NR_MAC, "rarh->T = 0x%x\n", rarh->T);
    LOG_I(NR_MAC, "rarh->RAPID = 0x%x (%i)\n", rarh->RAPID, rarh->RAPID);

    LOG_I(NR_MAC, "rar->R = 0x%x\n", rar->R);
    LOG_I(NR_MAC, "rar->TA1 = 0x%x\n", rar->TA1);

    LOG_I(NR_MAC, "rar->TA2 = 0x%x\n", rar->TA2);
    LOG_I(NR_MAC, "rar->UL_GRANT_1 = 0x%x\n", rar->UL_GRANT_1);

    LOG_I(NR_MAC, "rar->UL_GRANT_2 = 0x%x\n", rar->UL_GRANT_2);
    LOG_I(NR_MAC, "rar->UL_GRANT_3 = 0x%x\n", rar->UL_GRANT_3);
    LOG_I(NR_MAC, "rar->UL_GRANT_4 = 0x%x\n", rar->UL_GRANT_4);

    LOG_I(NR_MAC, "rar->TCRNTI_1 = 0x%x\n", rar->TCRNTI_1);
    LOG_I(NR_MAC, "rar->TCRNTI_2 = 0x%x\n", rar->TCRNTI_2);

    LOG_I(NR_MAC, "In %s:[%d.%d]: [UE %d] Received RAR with t_alloc %d f_alloc %d ta_command %d mcs %d freq_hopping %d tpc_command %d t_crnti %x \n",
      __FUNCTION__,
      frame,
      slot,
      mod_id,
      rar_grant.Msg3_t_alloc,
      rar_grant.Msg3_f_alloc,
      ul_time_alignment->ta_command,
      rar_grant.mcs,
      rar_grant.freq_hopping,
      tpc_command,
      ra->t_crnti);
#endif

    // Schedule Msg3
    ret = nr_ue_pusch_scheduler(mac, is_Msg3, frame, slot, &frame_tx, &slot_tx, rar_grant.Msg3_t_alloc);

    if (ret != -1){

      fapi_nr_ul_config_request_t *ul_config = get_ul_config_request(mac, slot_tx);

      if (!ul_config) {
        LOG_W(MAC, "In %s: ul_config request is NULL. Probably due to unexpected UL DCI in frame.slot %d.%d. Ignoring DCI!\n", __FUNCTION__, frame, slot);
        return -1;
      }

      nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu = &ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu;

      fill_ul_config(ul_config, frame_tx, slot_tx, FAPI_NR_UL_CONFIG_TYPE_PUSCH);

      // Config Msg3 PDU
      nr_config_pusch_pdu(mac, pusch_config_pdu, NULL, &rar_grant, rnti, NULL);

    }

  } else {

    ra->t_crnti = 0;
    ul_time_alignment->ta_command = (0xffff);

  }

  return ret;

}
