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

/* MAC related headers */
#include "mac_proto.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "mac_extern.h"
//#include "LAYER2/MAC/mac_vars.h" // TODO Note that mac_vars.h is not NR specific and this should be updated
                                 // also, the use of the same should be updated in nr-softmodem and nr-uesoftmodem

/* PHY UE related headers*/
#include "SCHED_NR_UE/defs.h"

#include "RRC/NR_UE/rrc_proto.h"
#include "assertions.h"
#include "PHY/defs_nr_UE.h"

/*Openair Packet Tracer */
#include "UTIL/OPT/opt.h"
#include "OCG.h"

/* log utils */
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

/* TODO remove this for debugging purpose
#include "LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "LAYER2/MAC/mac.h"
*/

#include <stdio.h>
#include <math.h>

//#define ENABLE_MAC_PAYLOAD_DEBUG 1

extern void mac_rlc_data_ind     (
  const module_id_t         module_idP,
  const rnti_t              rntiP,
  const eNB_index_t         eNB_index,
  const frame_t             frameP,
  const eNB_flag_t          enb_flagP,
  const MBMS_flag_t         MBMS_flagP,
  const logical_chan_id_t   channel_idP,
  char                     *buffer_pP,
  const tb_size_t           tb_sizeP,
  num_tb_t                  num_tbP,
  crc_t                    *crcs_pP);


uint32_t get_ssb_slot(uint32_t ssb_index){
    //  this function now only support f <= 3GHz
    return ssb_index & 0x3 ;

    //  return first_symbol(case, freq, ssb_index) / 14
}

uint8_t table_9_2_2_1[16][8]={
{0,12,2, 0, 0,3,0,0},
{0,12,2, 0, 0,4,8,0},
{0,12,2, 3, 0,4,8,0},
{1,10,4, 0, 0,6,0,0},
{1,10,4, 0, 0,3,6,9},
{1,10,4, 2, 0,3,6,9},
{1,10,4, 4, 0,3,6,9},
{1,4, 10,0, 0,6,0,0},
{1,4, 10,0, 0,3,6,9},
{1,4, 10,2, 0,3,6,9},
{1,4, 10,4, 0,3,6,9},
{1,0, 14,0, 0,6,0,0},
{1,0, 14,0, 0,3,6,9},
{1,0, 14,2, 0,3,6,9},
{1,0, 14,4, 0,3,6,9},
{1,0, 14,26,0,3,0,0}
};
int8_t nr_ue_process_dlsch(module_id_t module_id,
		                   int cc_id,
						   uint8_t gNB_index,
						   fapi_nr_dci_indication_t *dci_ind,
						   void *pduP,
						   uint32_t pdu_len)
{
    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
    fapi_nr_ul_config_request_t *ul_config = &mac->ul_config_request;
    //fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
    nr_phy_config_t *phy_config = &mac->phy_config;

    //ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.rnti = rnti;
    // First we need to verify if DCI ind contains a ul-sch to be perfomred. If it does, we will handle a PUSCH in the UL_CONFIG_REQ.
    ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUCCH;
    for (int i=0; i<10; i++) {
    	if(dci_ind!=NULL){
    		if (dci_ind->dci_list[i].dci_format < 2) ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
    	}
    }
    if (ul_config->ul_config_list[ul_config->number_pdus].pdu_type == FAPI_NR_UL_CONFIG_TYPE_PUSCH) {
        // fill in the elements in config request inside P5 message
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.bandwidth_part_ind = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.number_rbs = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.start_rb = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.frame_offset = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.number_symbols = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.start_symbol = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.pusch_freq_hopping = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.mcs = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.ndi = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.rv = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.harq_process_nbr = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.accumulated_delta_PUSCH = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.absolute_delta_PUSCH = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.n_layers = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.tpmi = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.n_dmrs_cdm_groups = 0;
      for (int i=0;i<4;i++) ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.dmrs_ports[i]=0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.n_front_load_symb = 0;
      //ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.srs_config = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.csi_reportTriggerSize = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.maxCodeBlockGroupsPerTransportBlock = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.ptrs_dmrs_association_port = 0;
      ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15.beta_offset_ind = 0;
    } else { // If DCI ind is not format 0_0 or 0_1, we will handle a PUCCH in the UL_CONFIG_REQ
      ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUCCH;
      // If we handle PUCCH common
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.format              = table_9_2_2_1[phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_resource_common][0];              /* format   0    1    2    3    4    */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.initialCyclicShift  = table_9_2_2_1[phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_resource_common][4];  /*          x    x                   */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSymbols         = table_9_2_2_1[phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_resource_common][2];         /*          x    x    x    x    x    */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.startingSymbolIndex = table_9_2_2_1[phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_resource_common][1]; /*          x    x    x    x    x    */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.timeDomainOCC = 0;       /*               x                   */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofPRBs = 0;            /*                    x    x         */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.startingPRB         = table_9_2_2_1[phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_resource_common][3];         /*                                     maxNrofPhysicalResourceBlocks  = 275 */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.occ_length = 0;          /*                              x    */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.occ_Index = 0;           /*                              x    */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.intraSlotFrequencyHopping = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.secondHopPRB = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.interslotFrequencyHopping = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.additionalDMRS = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.maxCodeRate = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSlots = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pi2PBSK = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.simultaneousHARQ_ACK_CSI = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pucch_GroupHopping  = phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_group_hopping;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.hoppingId           = phy_config->config_req.ul_bwp_common.pucch_config_common.hopping_id;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_nominal          = phy_config->config_req.ul_bwp_common.pucch_config_common.p0_nominal;
      for (int i=0;i<NUMBER_PUCCH_FORMAT_NR;i++) ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.deltaF_PUCCH_f[i] = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_PUCCH_Id = 0;     /* INTEGER (1..8)     */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_PUCCH_Value = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.twoPUCCH_PC_AdjustmentStates = 0;
      // If we handle PUCCH dedicated
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.format              = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].format;              /* format   0    1    2    3    4    */
      switch (ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.format){
        case pucch_format1_nr:
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.interslotFrequencyHopping = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.inter_slot_frequency_hopping;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.additionalDMRS            = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.additional_dmrs;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.maxCodeRate               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.max_code_rate;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSlots                 = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.number_of_slots;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pi2PBSK                   = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.pi2bpsk;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.simultaneousHARQ_ACK_CSI  = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format1.simultaneous_harq_ack_csi;
        break;
        case pucch_format2_nr:
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.interslotFrequencyHopping = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.inter_slot_frequency_hopping;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.additionalDMRS            = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.additional_dmrs;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.maxCodeRate               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.max_code_rate;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSlots                 = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.number_of_slots;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pi2PBSK                   = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.pi2bpsk;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.simultaneousHARQ_ACK_CSI  = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format2.simultaneous_harq_ack_csi;
        break;
        case pucch_format3_nr:
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.interslotFrequencyHopping = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.inter_slot_frequency_hopping;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.additionalDMRS            = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.additional_dmrs;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.maxCodeRate               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.max_code_rate;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSlots                 = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.number_of_slots;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pi2PBSK                   = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.pi2bpsk;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.simultaneousHARQ_ACK_CSI  = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format3.simultaneous_harq_ack_csi;
        break;
        case pucch_format4_nr:
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.interslotFrequencyHopping = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.inter_slot_frequency_hopping;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.additionalDMRS            = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.additional_dmrs;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.maxCodeRate               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.max_code_rate;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSlots                 = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.number_of_slots;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pi2PBSK                   = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.pi2bpsk;
            ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.simultaneousHARQ_ACK_CSI  = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.format4.simultaneous_harq_ack_csi;
        break;
        default:
        break;
      }
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.initialCyclicShift        = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].initial_cyclic_shift;  /*          x    x                   */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofSymbols               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].number_of_symbols;         /*          x    x    x    x    x    */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.startingSymbolIndex       = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].starting_symbol_index; /*          x    x    x    x    x    */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.timeDomainOCC             = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].time_domain_occ;       /*               x                   */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.nrofPRBs                  = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].number_of_prbs;            /*                    x    x         */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.startingPRB               = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].starting_prb;         /*                                     maxNrofPhysicalResourceBlocks  = 275 */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.occ_length                = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].occ_length;          /*                              x    */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.occ_Index                 = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].occ_index;           /*                              x    */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.intraSlotFrequencyHopping = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].intra_slot_frequency_hopping;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.secondHopPRB              = phy_config->config_req.ul_bwp_dedicated.pucch_config_dedicated.multi_csi_pucch_resources[0].second_hop_prb;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.pucch_GroupHopping        = phy_config->config_req.ul_bwp_common.pucch_config_common.pucch_group_hopping;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.hoppingId                 = phy_config->config_req.ul_bwp_common.pucch_config_common.hopping_id;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_nominal                = phy_config->config_req.ul_bwp_common.pucch_config_common.p0_nominal;
      for (int i=0;i<NUMBER_PUCCH_FORMAT_NR; i++) ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.deltaF_PUCCH_f[i] = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_PUCCH_Id = 0;     /* INTEGER (1..8)     */
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.p0_PUCCH_Value = 0;
      ul_config->ul_config_list[ul_config->number_pdus].pucch_config_pdu.twoPUCCH_PC_AdjustmentStates = 0;

    }
    if(mac->if_module != NULL && mac->if_module->scheduled_response != NULL){
        mac->if_module->scheduled_response(&mac->scheduled_response);
    }
    return 0;
}

int8_t nr_ue_decode_mib(
	UE_nr_rxtx_proc_t *proc,
	module_id_t module_id,
	int 		cc_id,
	uint8_t 	gNB_index,
	uint8_t 	extra_bits,	//	8bits 38.212 c7.1.1
	uint32_t    ssb_length,
	uint32_t 	ssb_index,
	void 		*pduP,
        uint16_t    cell_id ){

    LOG_I(MAC,"[L2][MAC] decode mib\n");

	NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

    nr_mac_rrc_data_ind_ue( module_id, cc_id, gNB_index, NR_BCCH_BCH, (uint8_t *) pduP, 3 );    //  fixed 3 bytes MIB PDU
    
    AssertFatal(mac->mib != NULL, "nr_ue_decode_mib() mac->mib == NULL\n");
    //if(mac->mib != NULL){
	    uint16_t frame = (mac->mib->systemFrameNumber.buf[0] >> mac->mib->systemFrameNumber.bits_unused);
	    uint16_t frame_number_4lsb = 0;
      for (int i=0; i<4; i++)
        frame_number_4lsb |= ((extra_bits>>i)&1)<<(3-i);
	    uint8_t half_frame_bit = ( extra_bits >> 4 ) & 0x1;               //	extra bits[4]
	    uint8_t ssb_subcarrier_offset_msb = ( extra_bits >> 5 ) & 0x1;    //	extra bits[5]
	    
	    uint8_t ssb_subcarrier_offset = (uint8_t)mac->mib->ssb_SubcarrierOffset;

	    //uint32_t ssb_index = 0;    //  TODO: ssb_index should obtain from L1 in case Lssb != 64

	    frame = frame << 4;
	    frame = frame | frame_number_4lsb;

	    if(ssb_length == 64){
	    	ssb_index = ssb_index & (( extra_bits >> 2 ) & 0x1C );    //	{ extra_bits[5:7], ssb_index[2:0] }
	    }else{
			if(ssb_subcarrier_offset_msb){
			    ssb_subcarrier_offset = ssb_subcarrier_offset | 0x10;
			}
	    }

#ifdef DEBUG_MIB
		LOG_I(MAC,"system frame number(6 MSB bits): %d\n",  mac->mib->systemFrameNumber.buf[0]);
		LOG_I(MAC,"system frame number(with LSB): %d\n", (int)frame);
		LOG_I(MAC,"subcarrier spacing (0=15or60, 1=30or120): %d\n", (int)mac->mib->subCarrierSpacingCommon);
		LOG_I(MAC,"ssb carrier offset(with MSB):  %d\n", (int)ssb_subcarrier_offset);
		LOG_I(MAC,"dmrs type A position (0=pos2,1=pos3): %d\n", (int)mac->mib->dmrs_TypeA_Position);
		LOG_I(MAC,"pdcch config sib1:             %d\n", (int)mac->mib->pdcch_ConfigSIB1);
		LOG_I(MAC,"cell barred (0=barred,1=notBarred): %d\n", (int)mac->mib->cellBarred);
		LOG_I(MAC,"intra frequency reselection (0=allowed,1=notAllowed): %d\n", (int)mac->mib->intraFreqReselection);
		LOG_I(MAC,"half frame bit(extra bits):    %d\n", (int)half_frame_bit);
		LOG_I(MAC,"ssb index(extra bits):         %d\n", (int)ssb_index);
#endif

	    subcarrier_spacing_t scs_ssb = scs_30kHz;      //  default for 
        //const uint32_t scs_index = 0;
        const uint32_t num_slot_per_frame = 20;
	    subcarrier_spacing_t scs_pdcch;

        //  assume carrier frequency < 6GHz
        if(mac->mib->subCarrierSpacingCommon == NR_MIB__subCarrierSpacingCommon_scs15or60){
            scs_pdcch = scs_15kHz;
        }else{  //NR_MIB__subCarrierSpacingCommon_scs30or120
            scs_pdcch = scs_30kHz;
        }

	channel_bandwidth_t min_channel_bw = bw_10MHz;  //  deafult for testing
	    
        uint32_t is_condition_A = (ssb_subcarrier_offset == 0);   //  38.213 ch.13
        frequency_range_t frequency_range = FR1;
        uint32_t index_4msb = (mac->mib->pdcch_ConfigSIB1.controlResourceSetZero);
        uint32_t index_4lsb = (mac->mib->pdcch_ConfigSIB1.searchSpaceZero);
        int32_t num_rbs = -1;
        int32_t num_symbols = -1;
        int32_t rb_offset = -1;
        //LOG_I(MAC,"<<<<<<<<<configSIB1 %d index_4msb %d index_4lsb %d scs_ssb %d scs_pdcch %d switch %d ",
        //mac->mib->pdcch_ConfigSIB1,index_4msb,index_4lsb,scs_ssb,scs_pdcch, (scs_ssb << 5)|scs_pdcch);

        //  type0-pdcch coreset
	    switch( (scs_ssb << 5)|scs_pdcch ){
            case (scs_15kHz << 5) | scs_15kHz :
                AssertFatal(index_4msb < 15, "38.213 Table 13-1 4 MSB out of range\n");
                mac->type0_pdcch_ss_mux_pattern = 1;
                num_rbs     = table_38213_13_1_c2[index_4msb];
                num_symbols = table_38213_13_1_c3[index_4msb];
                rb_offset   = table_38213_13_1_c4[index_4msb];
                break;

	        case (scs_15kHz << 5) | scs_30kHz:
                AssertFatal(index_4msb < 14, "38.213 Table 13-2 4 MSB out of range\n");
                mac->type0_pdcch_ss_mux_pattern = 1;
                num_rbs     = table_38213_13_2_c2[index_4msb];
                num_symbols = table_38213_13_2_c3[index_4msb];
                rb_offset   = table_38213_13_2_c4[index_4msb];
                break;

            case (scs_30kHz << 5) | scs_15kHz:
                if((min_channel_bw & bw_5MHz) | (min_channel_bw & bw_10MHz)){
                    AssertFatal(index_4msb < 9, "38.213 Table 13-3 4 MSB out of range\n");
                    mac->type0_pdcch_ss_mux_pattern = 1;
                    num_rbs     = table_38213_13_3_c2[index_4msb];
                    num_symbols = table_38213_13_3_c3[index_4msb];
                    rb_offset   = table_38213_13_3_c4[index_4msb];
                }else if(min_channel_bw & bw_40MHz){
                    AssertFatal(index_4msb < 9, "38.213 Table 13-5 4 MSB out of range\n");
                    mac->type0_pdcch_ss_mux_pattern = 1;
                    num_rbs     = table_38213_13_5_c2[index_4msb];
                    num_symbols = table_38213_13_5_c3[index_4msb];
                    rb_offset   = table_38213_13_5_c4[index_4msb];
                }else{ ; }

                break;

            case (scs_30kHz << 5) | scs_30kHz:
                if((min_channel_bw & bw_5MHz) | (min_channel_bw & bw_10MHz)){
                    mac->type0_pdcch_ss_mux_pattern = 1;
                    num_rbs     = table_38213_13_4_c2[index_4msb];
                    num_symbols = table_38213_13_4_c3[index_4msb];
                    rb_offset   = table_38213_13_4_c4[index_4msb];
                    LOG_I(MAC,"<<<<<<<<<index_4msb %d num_rbs %d num_symb %d rb_offset %d\n",index_4msb,num_rbs,num_symbols,rb_offset );
                }else if(min_channel_bw & bw_40MHz){
                    AssertFatal(index_4msb < 10, "38.213 Table 13-6 4 MSB out of range\n");
                    mac->type0_pdcch_ss_mux_pattern = 1;
                    num_rbs     = table_38213_13_6_c2[index_4msb];
                    num_symbols = table_38213_13_6_c3[index_4msb];
                    rb_offset   = table_38213_13_6_c4[index_4msb];
                }else{ ; }
                break;

            case (scs_120kHz << 5) | scs_60kHz:
                AssertFatal(index_4msb < 12, "38.213 Table 13-7 4 MSB out of range\n");
                if(index_4msb & 0x7){
                    mac->type0_pdcch_ss_mux_pattern = 1;
                }else if(index_4msb & 0x18){
                    mac->type0_pdcch_ss_mux_pattern = 2;
                }else{ ; }

                num_rbs     = table_38213_13_7_c2[index_4msb];
                num_symbols = table_38213_13_7_c3[index_4msb];
                if(!is_condition_A && (index_4msb == 8 || index_4msb == 10)){
                    rb_offset   = table_38213_13_7_c4[index_4msb] - 1;
                }else{
                    rb_offset   = table_38213_13_7_c4[index_4msb];
                }
                break;

            case (scs_120kHz << 5) | scs_120kHz:
                AssertFatal(index_4msb < 8, "38.213 Table 13-8 4 MSB out of range\n");
                if(index_4msb & 0x3){
                    mac->type0_pdcch_ss_mux_pattern = 1;
                }else if(index_4msb & 0x0c){
                    mac->type0_pdcch_ss_mux_pattern = 3;
                }

                num_rbs     = table_38213_13_8_c2[index_4msb];
                num_symbols = table_38213_13_8_c3[index_4msb];
                if(!is_condition_A && (index_4msb == 4 || index_4msb == 6)){
                    rb_offset   = table_38213_13_8_c4[index_4msb] - 1;
                }else{
                    rb_offset   = table_38213_13_8_c4[index_4msb];
                }
                break;

            case (scs_240kHz << 5) | scs_60kHz:
                AssertFatal(index_4msb < 4, "38.213 Table 13-9 4 MSB out of range\n");
                mac->type0_pdcch_ss_mux_pattern = 1;
                num_rbs     = table_38213_13_9_c2[index_4msb];
                num_symbols = table_38213_13_9_c3[index_4msb];
                rb_offset   = table_38213_13_9_c4[index_4msb];
                break;

            case (scs_240kHz << 5) | scs_120kHz:
                AssertFatal(index_4msb < 8, "38.213 Table 13-10 4 MSB out of range\n");
                if(index_4msb & 0x3){
                    mac->type0_pdcch_ss_mux_pattern = 1;
                }else if(index_4msb & 0x0c){
                    mac->type0_pdcch_ss_mux_pattern = 2;
                }
                num_rbs     = table_38213_13_10_c2[index_4msb];
                num_symbols = table_38213_13_10_c3[index_4msb];
                if(!is_condition_A && (index_4msb == 4 || index_4msb == 6)){
                    rb_offset   = table_38213_13_10_c4[index_4msb]-1;
                }else{
                    rb_offset   = table_38213_13_10_c4[index_4msb];
                }
                
                break;

	        default:
	            break;
	    }

        AssertFatal(num_rbs != -1, "Type0 PDCCH coreset num_rbs undefined");
        AssertFatal(num_symbols != -1, "Type0 PDCCH coreset num_symbols undefined");
        AssertFatal(rb_offset != -1, "Type0 PDCCH coreset rb_offset undefined");
        
        //uint32_t cell_id = 0;   //  obtain from L1 later

        //mac->type0_pdcch_dci_config.coreset.rb_start = rb_offset;
        //mac->type0_pdcch_dci_config.coreset.rb_end = rb_offset + num_rbs - 1;
        uint64_t mask = 0x0;
        uint8_t i;
        for(i=0; i<(num_rbs/6); ++i){   //  38.331 Each bit corresponds a group of 6 RBs
            mask = mask >> 1;
            mask = mask | 0x100000000000;
        }
        //LOG_I(MAC,">>>>>>>>mask %x num_rbs %d rb_offset %d\n", mask, num_rbs, rb_offset);
        mac->type0_pdcch_dci_config.coreset.frequency_domain_resource = mask;
        mac->type0_pdcch_dci_config.coreset.rb_offset = rb_offset;  //  additional parameter other than coreset

        //mac->type0_pdcch_dci_config.type0_pdcch_coreset.duration = num_symbols;
        mac->type0_pdcch_dci_config.coreset.cce_reg_mapping_type = CCE_REG_MAPPING_TYPE_INTERLEAVED;
        mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_reg_bundle_size = 6;   //  L 38.211 7.3.2.2
        mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_interleaver_size = 2;  //  R 38.211 7.3.2.2
        mac->type0_pdcch_dci_config.coreset.cce_reg_interleaved_shift_index = cell_id;
        mac->type0_pdcch_dci_config.coreset.precoder_granularity = PRECODER_GRANULARITY_SAME_AS_REG_BUNDLE;
        mac->type0_pdcch_dci_config.coreset.pdcch_dmrs_scrambling_id = cell_id;



        // type0-pdcch search space
        float big_o;
        float big_m;
        uint32_t temp;
        SFN_C_TYPE sfn_c;   //  only valid for mux=1
        uint32_t n_c;
        uint32_t number_of_search_space_per_slot;
        uint32_t first_symbol_index;
        uint32_t search_space_duration;  //  element of search space
        uint32_t coreset_duration;  //  element of coreset
        
        //  38.213 table 10.1-1

        /// MUX PATTERN 1
        if(mac->type0_pdcch_ss_mux_pattern == 1 && frequency_range == FR1){
            big_o = table_38213_13_11_c1[index_4lsb];
            number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
            big_m = table_38213_13_11_c3[index_4lsb];

            temp = (uint32_t)(big_o*pow(2, scs_pdcch)) + (uint32_t)(ssb_index*big_m);
            n_c = temp / num_slot_per_frame;
            if((temp/num_slot_per_frame) & 0x1){
                sfn_c = SFN_C_MOD_2_EQ_1;
            }else{
                sfn_c = SFN_C_MOD_2_EQ_0;
            }

            if((index_4lsb == 1 || index_4lsb == 3 || index_4lsb == 5 || index_4lsb == 7) && (ssb_index&1)){
                first_symbol_index = num_symbols;
            }else{
                first_symbol_index = table_38213_13_11_c4[index_4lsb];
            }
            //  38.213 chapter 13: over two consecutive slots
            search_space_duration = 2;
        }

        if(mac->type0_pdcch_ss_mux_pattern == 1 && frequency_range == FR2){
            big_o = table_38213_13_12_c1[index_4lsb];
            number_of_search_space_per_slot = table_38213_13_11_c2[index_4lsb];
            big_m = table_38213_13_12_c3[index_4lsb];

            if((index_4lsb == 1 || index_4lsb == 3 || index_4lsb == 5 || index_4lsb == 10) && (ssb_index&1)){
                first_symbol_index = 7;
            }else if((index_4lsb == 6 || index_4lsb == 7 || index_4lsb == 8 || index_4lsb == 11) && (ssb_index&1)){
                first_symbol_index = num_symbols;
            }else{
                first_symbol_index = 0;
            }
            //  38.213 chapter 13: over two consecutive slots
            search_space_duration = 2;
        }

        /// MUX PATTERN 2
        if(mac->type0_pdcch_ss_mux_pattern == 2){
            
            if((scs_ssb == scs_120kHz) && (scs_pdcch == scs_60kHz)){
                //  38.213 Table 13-13
                AssertFatal(index_4lsb == 0, "38.213 Table 13-13 4 LSB out of range\n");
                //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
//                sfn_c = SFN_C_EQ_SFN_SSB;
                n_c = get_ssb_slot(ssb_index);
                switch(ssb_index & 0x3){    //  ssb_index(i) mod 4
                    case 0: 
                        first_symbol_index = 0;
                        break;
                    case 1: 
                        first_symbol_index = 1;
                        break;
                    case 2: 
                        first_symbol_index = 6;
                        break;
                    case 3: 
                        first_symbol_index = 7;
                        break;
                    default: break; 
                }
                
            }else if((scs_ssb == scs_240kHz) && (scs_pdcch == scs_120kHz)){
                //  38.213 Table 13-14
                AssertFatal(index_4lsb == 0, "38.213 Table 13-14 4 LSB out of range\n");
                //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
//                sfn_c = SFN_C_EQ_SFN_SSB;
                n_c = get_ssb_slot(ssb_index);
                switch(ssb_index & 0x7){    //  ssb_index(i) mod 8
                    case 0: 
                        first_symbol_index = 0;
                        break;
                    case 1: 
                        first_symbol_index = 1;
                        break;
                    case 2: 
                        first_symbol_index = 2;
                        break;
                    case 3: 
                        first_symbol_index = 3;
                        break;
                    case 4: 
                        first_symbol_index = 12;
                        n_c = get_ssb_slot(ssb_index) - 1;
                        break;
                    case 5: 
                        first_symbol_index = 13;
                        n_c = get_ssb_slot(ssb_index) - 1;
                        break;
                    case 6: 
                        first_symbol_index = 0;
                        break;
                    case 7: 
                        first_symbol_index = 1;
                        break;
                    default: break; 
                }
            }else{ ; }
            //  38.213 chapter 13: over one slot
            search_space_duration = 1;
        }

        /// MUX PATTERN 3
        if(mac->type0_pdcch_ss_mux_pattern == 3){
            if((scs_ssb == scs_120kHz) && (scs_pdcch == scs_120kHz)){
                //  38.213 Table 13-15
                AssertFatal(index_4lsb == 0, "38.213 Table 13-15 4 LSB out of range\n");
                //  PDCCH monitoring occasions (SFN and slot number) same as SSB frame-slot
//                sfn_c = SFN_C_EQ_SFN_SSB;
                n_c = get_ssb_slot(ssb_index);
                switch(ssb_index & 0x3){    //  ssb_index(i) mod 4
                    case 0: 
                        first_symbol_index = 4;
                        break;
                    case 1: 
                        first_symbol_index = 8;
                        break;
                    case 2: 
                        first_symbol_index = 2;
                        break;
                    case 3: 
                        first_symbol_index = 6;
                        break;
                    default: break; 
                }
            }else{ ; }
            //  38.213 chapter 13: over one slot
            search_space_duration = 1;
        }

        coreset_duration = num_symbols * number_of_search_space_per_slot;

        mac->type0_pdcch_dci_config.number_of_candidates[0] = table_38213_10_1_1_c2[0];
        mac->type0_pdcch_dci_config.number_of_candidates[1] = table_38213_10_1_1_c2[1];
        mac->type0_pdcch_dci_config.number_of_candidates[2] = table_38213_10_1_1_c2[2];   //  CCE aggregation level = 4
        mac->type0_pdcch_dci_config.number_of_candidates[3] = table_38213_10_1_1_c2[3];   //  CCE aggregation level = 8
        mac->type0_pdcch_dci_config.number_of_candidates[4] = table_38213_10_1_1_c2[4];   //  CCE aggregation level = 16
        mac->type0_pdcch_dci_config.duration = search_space_duration;
        mac->type0_pdcch_dci_config.coreset.duration = coreset_duration;   //  coreset
        mac->type0_pdcch_dci_config.monitoring_symbols_within_slot = (0x3fff << first_symbol_index) & (0x3fff >> (14-coreset_duration-first_symbol_index)) & 0x3fff;

        mac->type0_pdcch_ss_sfn_c = sfn_c;
        mac->type0_pdcch_ss_n_c = n_c;
        
	    // fill in the elements in config request inside P5 message
	mac->phy_config.Mod_id = module_id;
	mac->phy_config.CC_id = cc_id;

	    mac->phy_config.config_req.pbch_config.system_frame_number = frame;    //  after calculation
	    mac->phy_config.config_req.pbch_config.subcarrier_spacing_common = mac->mib->subCarrierSpacingCommon;
	    mac->phy_config.config_req.pbch_config.ssb_subcarrier_offset = ssb_subcarrier_offset;  //  after calculation
	    mac->phy_config.config_req.pbch_config.dmrs_type_a_position = mac->mib->dmrs_TypeA_Position;
	    mac->phy_config.config_req.pbch_config.pdcch_config_sib1 = (mac->mib->pdcch_ConfigSIB1.controlResourceSetZero) * 16 + (mac->mib->pdcch_ConfigSIB1.searchSpaceZero);
	    mac->phy_config.config_req.pbch_config.cell_barred = mac->mib->cellBarred;
	    mac->phy_config.config_req.pbch_config.intra_frequency_reselection = mac->mib->intraFreqReselection;
	    mac->phy_config.config_req.pbch_config.half_frame_bit = half_frame_bit;
	    mac->phy_config.config_req.pbch_config.ssb_index = ssb_index;
	    mac->phy_config.config_req.config_mask |= FAPI_NR_CONFIG_REQUEST_MASK_PBCH;

	    if(mac->if_module != NULL && mac->if_module->phy_config_request != NULL){
		mac->if_module->phy_config_request(&mac->phy_config);
	    }
	    proc->decoded_frame_rx=frame;
    //}
    return 0;
}



//  TODO: change to UE parameter, scs: 15KHz, slot duration: 1ms
uint32_t get_ssb_frame(uint32_t test){
	return test;
}

// Performs :
// 1. TODO: Call RRC for link status return to PHY
// 2. TODO: Perform SR/BSR procedures for scheduling feedback
// 3. TODO: Perform PHR procedures
NR_UE_L2_STATE_t nr_ue_scheduler(
    const module_id_t module_id,
    const uint8_t gNB_index,
    const int cc_id,
    const frame_t rx_frame,
    const slot_t rx_slot,
    const int32_t ssb_index,
    const frame_t tx_frame,
    const slot_t tx_slot ){

    uint32_t search_space_mask = 0;
    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
    
    //  check type0 from 38.213 13
    if(ssb_index != -1){

        if(mac->type0_pdcch_ss_mux_pattern == 1){
            //	38.213 chapter 13
            if((mac->type0_pdcch_ss_sfn_c == SFN_C_MOD_2_EQ_0) && !(rx_frame & 0x1) && (rx_slot == mac->type0_pdcch_ss_n_c)){
            	search_space_mask = search_space_mask | type0_pdcch;
                mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.duration;
            }
            if((mac->type0_pdcch_ss_sfn_c == SFN_C_MOD_2_EQ_1) &&  (rx_frame & 0x1) && (rx_slot == mac->type0_pdcch_ss_n_c)){
            	search_space_mask = search_space_mask | type0_pdcch;
                mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.duration;
            }
        }
        if(mac->type0_pdcch_ss_mux_pattern == 2){
            //	38.213 Table 13-13, 13-14
            if((rx_frame == get_ssb_frame(rx_frame)) && (rx_slot == mac->type0_pdcch_ss_n_c)){
                search_space_mask = search_space_mask | type0_pdcch;
                mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.duration;
            }
        }
        if(mac->type0_pdcch_ss_mux_pattern == 3){
        	//	38.213 Table 13-15
            if((rx_frame == get_ssb_frame(rx_frame)) && (rx_slot == mac->type0_pdcch_ss_n_c)){
                search_space_mask = search_space_mask | type0_pdcch;
                mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_dci_config.duration;
            }
        }
    }

    fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
    //  Type0 PDCCH search space
    if((search_space_mask & type0_pdcch) || ( mac->type0_pdcch_consecutive_slots != 0 )){
        mac->type0_pdcch_consecutive_slots = mac->type0_pdcch_consecutive_slots - 1;

        dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15 = mac->type0_pdcch_dci_config;
        dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DCI;
    	
	/*
    	dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.rnti = 0xaaaa;	//	to be set
    	dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP = 106;	//	to be set

	LOG_I(MAC,"nr_ue_scheduler Type0 PDCCH with rnti %x, BWP %d\n",
	       dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.rnti,
	       dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP);  
	*/   
        dl_config->number_pdus = dl_config->number_pdus + 1;
    }

    if(search_space_mask & type0a_pdcch){
    }

    if(search_space_mask & type1_pdcch){
    }

    if(search_space_mask & type2_pdcch){
    }

    if(search_space_mask & type3_pdcch){
    }


    mac->scheduled_response.dl_config = dl_config;
    

	return UE_CONNECTION_OK;
}

//////////////
/*
 * This code contains all the functions needed to process all dci fields.
 * These tables and functions are going to be called by function nr_ue_process_dci
 */
// table_7_3_1_1_2_2_3_4_5 contains values for number of layers and precoding information for tables 7.3.1.1.2-2/3/4/5 from TS 38.212 subclause 7.3.1.1.2
// the first 6 columns contain table 7.3.1.1.2-2: Precoding information and number of layers, for 4 antenna ports, if transformPrecoder=disabled and maxRank = 2 or 3 or 4
// next six columns contain table 7.3.1.1.2-3: Precoding information and number of layers for 4 antenna ports, if transformPrecoder= enabled, or if transformPrecoder=disabled and maxRank = 1
// next four columns contain table 7.3.1.1.2-4: Precoding information and number of layers, for 2 antenna ports, if transformPrecoder=disabled and maxRank = 2
// next four columns contain table 7.3.1.1.2-5: Precoding information and number of layers, for 2 antenna ports, if transformPrecoder= enabled, or if transformPrecoder= disabled and maxRank = 1
uint8_t table_7_3_1_1_2_2_3_4_5[64][20] = {
{1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0},
{1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1},
{1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  2,  0,  2,  0,  1,  2,  0,  0},
{1,  3,  1,  3,  1,  3,  1,  3,  1,  3,  1,  3,  1,  2,  0,  0,  1,  3,  0,  0},
{2,  0,  2,  0,  2,  0,  1,  4,  1,  4,  0,  0,  1,  3,  0,  0,  1,  4,  0,  0},
{2,  1,  2,  1,  2,  1,  1,  5,  1,  5,  0,  0,  1,  4,  0,  0,  1,  5,  0,  0},
{2,  2,  2,  2,  2,  2,  1,  6,  1,  6,  0,  0,  1,  5,  0,  0,  0,  0,  0,  0},
{2,  3,  2,  3,  2,  3,  1,  7,  1,  7,  0,  0,  2,  1,  0,  0,  0,  0,  0,  0},
{2,  4,  2,  4,  2,  4,  1,  8,  1,  8,  0,  0,  2,  2,  0,  0,  0,  0,  0,  0},
{2,  5,  2,  5,  2,  5,  1,  9,  1,  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{3,  0,  3,  0,  3,  0,  1,  10, 1,  10, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{4,  0,  4,  0,  4,  0,  1,  11, 1,  11, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  4,  1,  4,  0,  0,  1,  12, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  5,  1,  5,  0,  0,  1,  13, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  6,  1,  6,  0,  0,  1,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  7,  1,  7,  0,  0,  1,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  8,  1,  8,  0,  0,  1,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  9,  1,  9,  0,  0,  1,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  10, 1,  10, 0,  0,  1,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  11, 1,  11, 0,  0,  1,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  6,  2,  6,  0,  0,  1,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  7,  2,  7,  0,  0,  1,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  8,  2,  8,  0,  0,  1,  22, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  9,  2,  9,  0,  0,  1,  23, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  10, 2,  10, 0,  0,  1,  24, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  11, 2,  11, 0,  0,  1,  25, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  12, 2,  12, 0,  0,  1,  26, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  13, 2,  13, 0,  0,  1,  27, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{3,  1,  3,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{3,  2,  3,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{4,  1,  4,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{4,  2,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  12, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  13, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  22, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  23, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  24, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  25, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  26, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{1,  27, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{2,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{3,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{3,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{3,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{4,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}
};
uint8_t table_7_3_1_1_2_12[14][3] = {
{1,0,1},
{1,1,1},
{2,0,1},
{2,1,1},
{2,2,1},
{2,3,1},
{2,0,2},
{2,1,2},
{2,2,2},
{2,3,2},
{2,4,2},
{2,5,2},
{2,6,2},
{2,7,2}
};
uint8_t table_7_3_1_1_2_13[10][4] = {
{1,0,1,1},
{2,0,1,1},
{2,2,3,1},
{2,0,2,1},
{2,0,1,2},
{2,2,3,2},
{2,4,5,2},
{2,6,7,2},
{2,0,4,2},
{2,2,6,2}
};
uint8_t table_7_3_1_1_2_14[3][5] = {
{2,0,1,2,1},
{2,0,1,4,2},
{2,2,3,6,2}
};
uint8_t table_7_3_1_1_2_15[4][6] = {
{2,0,1,2,3,1},
{2,0,1,4,5,2},
{2,2,3,6,7,2},
{2,0,2,4,6,2}
};
uint8_t table_7_3_1_1_2_16[12][2] = {
{1,0},
{1,1},
{2,0},
{2,1},
{2,2},
{2,3},
{3,0},
{3,1},
{3,2},
{3,3},
{3,4},
{3,5}
};
uint8_t table_7_3_1_1_2_17[7][3] = {
{1,0,1},
{2,0,1},
{2,2,3},
{3,0,1},
{3,2,3},
{3,4,5},
{2,0,2}
};
uint8_t table_7_3_1_1_2_18[3][4] = {
{2,0,1,2},
{3,0,1,2},
{3,3,4,5}
};
uint8_t table_7_3_1_1_2_19[2][5] = {
{2,0,1,2,3},
{3,0,1,2,3}
};
uint8_t table_7_3_1_1_2_20[28][3] = {
{1,0,1},
{1,1,1},
{2,0,1},
{2,1,1},
{2,2,1},
{2,3,1},
{3,0,1},
{3,1,1},
{3,2,1},
{3,3,1},
{3,4,1},
{3,5,1},
{3,0,2},
{3,1,2},
{3,2,2},
{3,3,2},
{3,4,2},
{3,5,2},
{3,6,2},
{3,7,2},
{3,8,2},
{3,9,2},
{3,10,2},
{3,11,2},
{1,0,2},
{1,1,2},
{1,6,2},
{1,7,2}
};
uint8_t table_7_3_1_1_2_21[19][4] = {
{1,0,1,1},
{2,0,1,1},
{2,2,3,1},
{3,0,1,1},
{3,2,3,1},
{3,4,5,1},
{2,0,2,1},
{3,0,1,2},
{3,2,3,2},
{3,4,5,2},
{3,6,7,2},
{3,8,9,2},
{3,10,11,2},
{1,0,1,2},
{1,6,7,2},
{2,0,1,2},
{2,2,3,2},
{2,6,7,2},
{2,8,9,2}
};
uint8_t table_7_3_1_1_2_22[6][5] = {
{2,0,1,2,1},
{3,0,1,2,1},
{3,3,4,5,1},
{3,0,1,6,2},
{3,2,3,8,2},
{3,4,5,10,2}
};
uint8_t table_7_3_1_1_2_23[5][6] = {
{2,0,1,2,3,1},
{3,0,1,2,3,1},
{3,0,1,6,7,2},
{3,2,3,8,9,2},
{3,4,5,10,11,2}
};
uint8_t table_7_3_2_3_3_1[12][5] = {
{1,0,0,0,0},
{1,1,0,0,0},
{1,0,1,0,0},
{2,0,0,0,0},
{2,1,0,0,0},
{2,2,0,0,0},
{2,3,0,0,0},
{2,0,1,0,0},
{2,2,3,0,0},
{2,0,1,2,0},
{2,0,1,2,3},
{2,0,2,0,0}
};
uint8_t table_7_3_2_3_3_2_oneCodeword[31][6] = {
{1,0,0,0,0,1},
{1,1,0,0,0,1},
{1,0,1,0,0,1},
{2,0,0,0,0,1},
{2,1,0,0,0,1},
{2,2,0,0,0,1},
{2,3,0,0,0,1},
{2,0,1,0,0,1},
{2,2,3,0,0,1},
{2,0,1,2,0,1},
{2,0,1,2,3,1},
{2,0,2,0,0,1},
{2,0,0,0,0,2},
{2,1,0,0,0,2},
{2,2,0,0,0,2},
{2,3,0,0,0,2},
{2,4,0,0,0,2},
{2,5,0,0,0,2},
{2,6,0,0,0,2},
{2,7,0,0,0,2},
{2,0,1,0,0,2},
{2,2,3,0,0,2},
{2,4,5,0,0,2},
{2,6,7,0,0,2},
{2,0,4,0,0,2},
{2,2,6,0,0,2},
{2,0,1,4,0,2},
{2,2,3,6,0,2},
{2,0,1,4,5,2},
{2,2,3,6,7,2},
{2,0,2,4,6,2}
};
uint8_t table_7_3_2_3_3_2_twoCodeword[4][10] = {
{2,0,1,2,3,4,0,0,0,2},
{2,0,1,2,3,4,6,0,0,2},
{2,0,1,2,3,4,5,6,0,2},
{2,0,1,2,3,4,5,6,7,2}
};
uint8_t table_7_3_2_3_3_3_oneCodeword[24][5] = {
{1,0,0,0,0},
{1,1,0,0,0},
{1,0,1,0,0},
{2,0,0,0,0},
{2,1,0,0,0},
{2,2,0,0,0},
{2,3,0,0,0},
{2,0,1,0,0},
{2,2,3,0,0},
{2,0,1,2,0},
{2,0,1,2,3},
{3,0,0,0,0},
{3,1,0,0,0},
{3,2,0,0,0},
{3,3,0,0,0},
{3,4,0,0,0},
{3,5,0,0,0},
{3,0,1,0,0},
{3,2,3,0,0},
{3,4,5,0,0},
{3,0,1,2,0},
{3,3,4,5,0},
{3,0,1,2,3},
{2,0,2,0,0}
};
uint8_t table_7_3_2_3_3_3_twoCodeword[2][7] = {
{3,0,1,2,3,4,0},
{3,0,1,2,3,4,5}
};
uint8_t table_7_3_2_3_3_4_oneCodeword[58][6] = {
{1,0,0,0,0,1},
{1,1,0,0,0,1},
{1,0,1,0,0,1},
{2,0,0,0,0,1},
{2,1,0,0,0,1},
{2,2,0,0,0,1},
{2,3,0,0,0,1},
{2,0,1,0,0,1},
{2,2,3,0,0,1},
{2,0,1,2,0,1},
{2,0,1,2,3,1},
{3,0,0,0,0,1},
{3,1,0,0,0,1},
{3,2,0,0,0,1},
{3,3,0,0,0,1},
{3,4,0,0,0,1},
{3,5,0,0,0,1},
{3,0,1,0,0,1},
{3,2,3,0,0,1},
{3,4,5,0,0,1},
{3,0,1,2,0,1},
{3,3,4,5,0,1},
{3,0,1,2,3,1},
{2,0,2,0,0,1},
{3,0,0,0,0,2},
{3,1,0,0,0,2},
{3,2,0,0,0,2},
{3,3,0,0,0,2},
{3,4,0,0,0,2},
{3,5,0,0,0,2},
{3,6,0,0,0,2},
{3,7,0,0,0,2},
{3,8,0,0,0,2},
{3,9,0,0,0,2},
{3,10,0,0,0,2},
{3,11,0,0,0,2},
{3,0,1,0,0,2},
{3,2,3,0,0,2},
{3,4,5,0,0,2},
{3,6,7,0,0,2},
{3,8,9,0,0,2},
{3,10,11,0,0,2},
{3,0,1,6,0,2},
{3,2,3,8,0,2},
{3,4,5,10,0,2},
{3,0,1,6,7,2},
{3,2,3,8,9,2},
{3,4,5,10,11,2},
{1,0,0,0,0,2},
{1,1,0,0,0,2},
{1,6,0,0,0,2},
{1,7,0,0,0,2},
{1,0,1,0,0,2},
{1,6,7,0,0,2},
{2,0,1,0,0,2},
{2,2,3,0,0,2},
{2,6,7,0,0,2},
{2,8,9,0,0,2}
};
uint8_t table_7_3_2_3_3_4_twoCodeword[6][10] = {
{3,0,1,2,3,4,0,0,0,1},
{3,0,1,2,3,4,5,0,0,1},
{2,0,1,2,3,6,0,0,0,2},
{2,0,1,2,3,6,8,0,0,2},
{2,0,1,2,3,6,7,8,0,2},
{2,0,1,2,3,6,7,8,9,2}
};
int8_t nr_ue_process_dci_freq_dom_resource_assignment(
  fapi_nr_ul_config_pusch_pdu_rel15_t *ulsch_config_pdu,
  fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu,
  uint16_t n_RB_ULBWP,
  uint16_t n_RB_DLBWP,
  uint16_t riv
){
  uint16_t l_RB;
  uint16_t start_RB;
  uint16_t tmp_RIV;

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
    // For resource allocation type 1, the resource allocation field consists of a resource indication value (RIV):
    // RIV = n_RB_DLBWP * (l_RB - 1) + start_RB                                  if (l_RB - 1) <= floor (n_RB_DLBWP/2)
    // RIV = n_RB_DLBWP * (n_RB_DLBWP - l_RB + 1) + (n_RB_DLBWP - 1 - start_RB)  if (l_RB - 1)  > floor (n_RB_DLBWP/2)
    // the following two expressions apply only if (l_RB - 1) <= floor (n_RB_DLBWP/2)
    l_RB = floor(riv/n_RB_DLBWP) + 1;
    start_RB = riv%n_RB_DLBWP;
    // if (l_RB - 1)  > floor (n_RB_DLBWP/2) we need to recalculate them using the following lines
    tmp_RIV = n_RB_DLBWP * (l_RB - 1) + start_RB;
    if ((tmp_RIV != riv) || ((start_RB+l_RB)>n_RB_DLBWP)) { // then (l_RB - 1)  > floor (n_RB_DLBWP/2) and we need to recalculate l_RB and start_RB
      l_RB = n_RB_DLBWP - l_RB + 2;
      start_RB = n_RB_DLBWP - start_RB - 1;
    }
    dlsch_config_pdu->number_rbs = l_RB;
    dlsch_config_pdu->start_rb = start_RB;
  }
  if(ulsch_config_pdu != NULL){
/*
 * TS 38.214 subclause 6.1.2.2 Resource allocation in frequency domain (uplink)
 */
  /*
   * TS 38.214 subclause 6.1.2.2.1 Uplink resource allocation type 0
   */
  /*
   * TS 38.214 subclause 6.1.2.2.2 Uplink resource allocation type 1
   */
    // For resource allocation type 1, the resource allocation field consists of a resource indication value (RIV):
    // RIV = n_RB_ULBWP * (l_RB - 1) + start_RB                                  if (l_RB - 1) <= floor (n_RB_ULBWP/2)
    // RIV = n_RB_ULBWP * (n_RB_ULBWP - l_RB + 1) + (n_RB_ULBWP - 1 - start_RB)  if (l_RB - 1)  > floor (n_RB_ULBWP/2)
    // the following two expressions apply only if (l_RB - 1) <= floor (n_RB_ULBWP/2)
    l_RB = floor(riv/n_RB_ULBWP) + 1;
    start_RB = riv%n_RB_ULBWP;
    // if (l_RB - 1)  > floor (n_RB_ULBWP/2) we need to recalculate them using the following lines
    tmp_RIV = n_RB_ULBWP * (l_RB - 1) + start_RB;
    if (tmp_RIV != riv) { // then (l_RB - 1)  > floor (n_RB_ULBWP/2) and we need to recalculate l_RB and start_RB
        l_RB = n_RB_ULBWP - l_RB + 2;
        start_RB = n_RB_ULBWP - start_RB - 1;
    }
    ulsch_config_pdu->number_rbs = l_RB;
    ulsch_config_pdu->start_rb = start_RB;
  }
  return 0;
}

int8_t nr_ue_process_dci_time_dom_resource_assignment(
  fapi_nr_ul_config_pusch_pdu_rel15_t *ulsch_config_pdu,
  fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu,
  uint8_t time_domain_ind,
  long dmrs_typeA_pos //0=pos2,1=pos3
){
  uint8_t k_offset=0;
  uint8_t sliv_S=0;
  uint8_t sliv_L=0;
  uint8_t table_5_1_2_1_1_2_time_dom_res_alloc_A[16][3]={ // for PDSCH from TS 38.214 subclause 5.1.2.1.1
  {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?12:11}, // row index 1
  {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?10:9},  // row index 2
  {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?9:8},   // row index 3
  {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?7:6},   // row index 4
  {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?5:4},   // row index 5
  {0,(dmrs_typeA_pos == 0)?9:10,(dmrs_typeA_pos == 0)?4:4},   // row index 6
  {0,(dmrs_typeA_pos == 0)?4:6, (dmrs_typeA_pos == 0)?4:4},   // row index 7
  {0,5,7},  // row index 8
  {0,5,2},  // row index 9
  {0,9,2},  // row index 10
  {0,12,2}, // row index 11
  {0,1,13}, // row index 12
  {0,1,6},  // row index 13
  {0,2,4},  // row index 14
  {0,4,7},  // row index 15
  {0,8,4}   // row index 16
  };
  /*uint8_t table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[16][3]={ // for PDSCH from TS 38.214 subclause 5.1.2.1.1
  {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?6:5},   // row index 1
  {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?10:9},  // row index 2
  {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?9:8},   // row index 3
  {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?7:6},   // row index 4
  {0,(dmrs_typeA_pos == 0)?2:3, (dmrs_typeA_pos == 0)?5:4},   // row index 5
  {0,(dmrs_typeA_pos == 0)?6:8, (dmrs_typeA_pos == 0)?4:2},   // row index 6
  {0,(dmrs_typeA_pos == 0)?4:6, (dmrs_typeA_pos == 0)?4:4},   // row index 7
  {0,5,6},  // row index 8
  {0,5,2},  // row index 9
  {0,9,2},  // row index 10
  {0,10,2}, // row index 11
  {0,1,11}, // row index 12
  {0,1,6},  // row index 13
  {0,2,4},  // row index 14
  {0,4,6},  // row index 15
  {0,8,4}   // row index 16
  };*/
  /*uint8_t table_5_1_2_1_1_4_time_dom_res_alloc_B[16][3]={ // for PDSCH from TS 38.214 subclause 5.1.2.1.1
  {0,2,2},  // row index 1
  {0,4,2},  // row index 2
  {0,6,2},  // row index 3
  {0,8,2},  // row index 4
  {0,10,2}, // row index 5
  {1,2,2},  // row index 6
  {1,4,2},  // row index 7
  {0,2,4},  // row index 8
  {0,4,4},  // row index 9
  {0,6,4},  // row index 10
  {0,8,4},  // row index 11
  {0,10,4}, // row index 12
  {0,2,7},  // row index 13
  {0,(dmrs_typeA_pos == 0)?2:3,(dmrs_typeA_pos == 0)?12:11},  // row index 14
  {1,2,4},  // row index 15
  {0,0,0}   // row index 16
  };*/
  /*uint8_t table_5_1_2_1_1_5_time_dom_res_alloc_C[16][3]={ // for PDSCH from TS 38.214 subclause 5.1.2.1.1
  {0,2,2},  // row index 1
  {0,4,2},  // row index 2
  {0,6,2},  // row index 3
  {0,8,2},  // row index 4
  {0,10,2}, // row index 5
  {0,0,0},  // row index 6
  {0,0,0},  // row index 7
  {0,2,4},  // row index 8
  {0,4,4},  // row index 9
  {0,6,4},  // row index 10
  {0,8,4},  // row index 11
  {0,10,4}, // row index 12
  {0,2,7},  // row index 13
  {0,(dmrs_typeA_pos == 0)?2:3,(dmrs_typeA_pos == 0)?12:11},  // row index 14
  {0,0,6},  // row index 15
  {0,2,6}   // row index 16
  };*/
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
      k_offset = table_5_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][0];
      sliv_S   = table_5_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][1];
      sliv_L   = table_5_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][2];
      // k_offset = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_5_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      // k_offset = table_5_1_2_1_1_4_time_dom_res_alloc_B[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_5_1_2_1_1_4_time_dom_res_alloc_B[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_5_1_2_1_1_4_time_dom_res_alloc_B[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      // k_offset = table_5_1_2_1_1_5_time_dom_res_alloc_C[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_5_1_2_1_1_5_time_dom_res_alloc_C[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_5_1_2_1_1_5_time_dom_res_alloc_C[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      dlsch_config_pdu->frame_offset = k_offset;
      dlsch_config_pdu->number_symbols = sliv_L;
      dlsch_config_pdu->start_symbol = sliv_S;
  }	/*
 * TS 38.214 subclause 6.1.2.1 Resource allocation in time domain (uplink)
 */
  if(ulsch_config_pdu != NULL){
      k_offset = table_6_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][0];
      sliv_S   = table_6_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][1];
      sliv_L   = table_6_1_2_1_1_2_time_dom_res_alloc_A[time_domain_ind-1][2];
      // k_offset = table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][0];
      // sliv_S   = table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][1];
      // sliv_L   = table_6_1_2_1_1_3_time_dom_res_alloc_A_extCP[nr_pdci_info_extracted->time_dom_resource_assignment][2];
      ulsch_config_pdu->frame_offset = k_offset;
      ulsch_config_pdu->number_symbols = sliv_L;
      ulsch_config_pdu->start_symbol = sliv_S;
  }
  return 0;
}
//////////////

int8_t nr_ue_process_dci(module_id_t module_id, int cc_id, uint8_t gNB_index, fapi_nr_dci_pdu_rel15_t *dci, uint16_t rnti, uint32_t dci_format){

    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
    fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;
    fapi_nr_ul_config_request_t *ul_config = &mac->ul_config_request;
    
    //const uint16_t n_RB_DLBWP = dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP; //make sure this has been set
    const uint16_t n_RB_DLBWP = mac->initial_bwp_dl.N_RB;
    const uint16_t n_RB_ULBWP = mac->initial_bwp_ul.N_RB;

    LOG_I(MAC,"nr_ue_process_dci at MAC layer with dci_format=%d (DL BWP %d, UL BWP %d)\n",dci_format,n_RB_DLBWP,n_RB_ULBWP);

    switch(dci_format){
        case format0_0:
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
            ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
            ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.rnti = rnti;
            fapi_nr_ul_config_pusch_pdu_rel15_t *ulsch_config_pdu_0_0 = &ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15;
        /* IDENTIFIER_DCI_FORMATS */
        /* FREQ_DOM_RESOURCE_ASSIGNMENT_UL */
            nr_ue_process_dci_freq_dom_resource_assignment(ulsch_config_pdu_0_0,NULL,n_RB_ULBWP,0,dci->freq_dom_resource_assignment_UL);
        /* TIME_DOM_RESOURCE_ASSIGNMENT */
            nr_ue_process_dci_time_dom_resource_assignment(ulsch_config_pdu_0_0,NULL,dci->time_dom_resource_assignment,mac->mib->dmrs_TypeA_Position);
        /* FREQ_HOPPING_FLAG */
            if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.resource_allocation != 0) &&
                (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.frequency_hopping !=0))
              ulsch_config_pdu_0_0->pusch_freq_hopping = (dci->freq_hopping_flag == 0)? pusch_freq_hopping_disabled:pusch_freq_hopping_enabled;
        /* MCS */
            ulsch_config_pdu_0_0->mcs = dci->mcs;
        /* NDI */
            ulsch_config_pdu_0_0->ndi = dci->ndi;
        /* RV */
            ulsch_config_pdu_0_0->rv = dci->rv;
        /* HARQ_PROCESS_NUMBER */
            ulsch_config_pdu_0_0->harq_process_nbr = dci->harq_process_number;
        /* TPC_PUSCH */
            // according to TS 38.213 Table Table 7.1.1-1
            if (dci->tpc_pusch == 0) {
              ulsch_config_pdu_0_0->accumulated_delta_PUSCH = -1;
              ulsch_config_pdu_0_0->absolute_delta_PUSCH = -4;
            }
            if (dci->tpc_pusch == 1) {
              ulsch_config_pdu_0_0->accumulated_delta_PUSCH = 0;
              ulsch_config_pdu_0_0->absolute_delta_PUSCH = -1;
            }
            if (dci->tpc_pusch == 2) {
              ulsch_config_pdu_0_0->accumulated_delta_PUSCH = 1;
              ulsch_config_pdu_0_0->absolute_delta_PUSCH = 1;
            }
            if (dci->tpc_pusch == 3) {
              ulsch_config_pdu_0_0->accumulated_delta_PUSCH = 3;
              ulsch_config_pdu_0_0->absolute_delta_PUSCH = 4;
            }
        /* SUL_IND_0_0 */ // To be implemented, FIXME!!!

            ul_config->number_pdus = ul_config->number_pdus + 1;
            break;

        case format0_1:
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
            ul_config->ul_config_list[ul_config->number_pdus].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
            ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.rnti = rnti;
            fapi_nr_ul_config_pusch_pdu_rel15_t *ulsch_config_pdu_0_1 = &ul_config->ul_config_list[ul_config->number_pdus].ulsch_config_pdu.ulsch_pdu_rel15;
        /* IDENTIFIER_DCI_FORMATS */
        /* CARRIER_IND */
        /* SUL_IND_0_1 */
        /* BANDWIDTH_PART_IND */
            ulsch_config_pdu_0_1->bandwidth_part_ind = dci->bandwidth_part_ind;
        /* FREQ_DOM_RESOURCE_ASSIGNMENT_UL */
            nr_ue_process_dci_freq_dom_resource_assignment(ulsch_config_pdu_0_1,NULL,n_RB_ULBWP,0,dci->freq_dom_resource_assignment_UL);
        /* TIME_DOM_RESOURCE_ASSIGNMENT */
            nr_ue_process_dci_time_dom_resource_assignment(ulsch_config_pdu_0_1,NULL,dci->time_dom_resource_assignment,mac->mib->dmrs_TypeA_Position);
        /* FREQ_HOPPING_FLAG */
            if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.resource_allocation != 0) &&
                (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.frequency_hopping !=0))
              ulsch_config_pdu_0_1->pusch_freq_hopping = (dci->freq_hopping_flag == 0)? pusch_freq_hopping_disabled:pusch_freq_hopping_enabled;
        /* MCS */
            ulsch_config_pdu_0_1->mcs = dci->mcs;
        /* NDI */
            ulsch_config_pdu_0_1->ndi = dci->ndi;
        /* RV */
            ulsch_config_pdu_0_1->rv = dci->rv;
        /* HARQ_PROCESS_NUMBER */
            ulsch_config_pdu_0_1->harq_process_nbr = dci->harq_process_number;
        /* FIRST_DAI */ //To be implemented, FIXME!!!
        /* SECOND_DAI */ //To be implemented, FIXME!!!
        /* TPC_PUSCH */
            // according to TS 38.213 Table Table 7.1.1-1
            if (dci->tpc_pusch == 0) {
              ulsch_config_pdu_0_1->accumulated_delta_PUSCH = -1;
              ulsch_config_pdu_0_1->absolute_delta_PUSCH = -4;
            }
            if (dci->tpc_pusch == 1) {
              ulsch_config_pdu_0_1->accumulated_delta_PUSCH = 0;
              ulsch_config_pdu_0_1->absolute_delta_PUSCH = -1;
            }
            if (dci->tpc_pusch == 2) {
              ulsch_config_pdu_0_1->accumulated_delta_PUSCH = 1;
              ulsch_config_pdu_0_1->absolute_delta_PUSCH = 1;
            }
            if (dci->tpc_pusch == 3) {
              ulsch_config_pdu_0_1->accumulated_delta_PUSCH = 3;
              ulsch_config_pdu_0_1->absolute_delta_PUSCH = 4;
            }
        /* SRS_RESOURCE_IND */
            //FIXME!!
        /* PRECOD_NBR_LAYERS */
            if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.tx_config == tx_config_nonCodebook));
              // 0 bits if the higher layer parameter txConfig = nonCodeBook
            if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.tx_config == tx_config_codebook)){
              uint8_t n_antenna_port = 0; //FIXME!!!
              if (n_antenna_port == 1); // 1 antenna port and the higher layer parameter txConfig = codebook 0 bits
              if (n_antenna_port == 4){ // 4 antenna port and the higher layer parameter txConfig = codebook
                // Table 7.3.1.1.2-2: transformPrecoder=disabled and maxRank = 2 or 3 or 4
                if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled)
                     && ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 2) ||
                         (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 3) ||
                         (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 4))){
                    if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
                      ulsch_config_pdu_0_1->n_layers = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][0];
                      ulsch_config_pdu_0_1->tpmi     = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][1];
                    }
                    if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_partialAndNonCoherent){
                      ulsch_config_pdu_0_1->n_layers = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][2];
                      ulsch_config_pdu_0_1->tpmi     = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][3];
                    }
                    if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_nonCoherent){
                      ulsch_config_pdu_0_1->n_layers = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][4];
                      ulsch_config_pdu_0_1->tpmi     = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][5];
                    }
                }
                // Table 7.3.1.1.2-3: transformPrecoder= enabled, or transformPrecoder=disabled and maxRank = 1
                if (((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_enabled)
                  || (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled))
                  && (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 1)){
                  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
                    ulsch_config_pdu_0_1->n_layers = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][6];
                    ulsch_config_pdu_0_1->tpmi     = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][7];
                  }
                  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_partialAndNonCoherent){
                    ulsch_config_pdu_0_1->n_layers = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][8];
                    ulsch_config_pdu_0_1->tpmi     = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][9];
                  }
                  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_nonCoherent){
                    ulsch_config_pdu_0_1->n_layers = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][10];
                    ulsch_config_pdu_0_1->tpmi     = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][11];
                  }
                }
              }
              if (n_antenna_port == 4){ // 2 antenna port and the higher layer parameter txConfig = codebook
                // Table 7.3.1.1.2-4: transformPrecoder=disabled and maxRank = 2
                if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled)
                  && (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 2)){
                  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
                    ulsch_config_pdu_0_1->n_layers = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][12];
                    ulsch_config_pdu_0_1->tpmi     = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][13];
                  }
                  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_nonCoherent){
                    ulsch_config_pdu_0_1->n_layers = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][14];
                    ulsch_config_pdu_0_1->tpmi     = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][15];
                  }
                }
                // Table 7.3.1.1.2-5: transformPrecoder= enabled, or transformPrecoder= disabled and maxRank = 1
                if (((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_enabled)
                  || (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled))
                  && (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 1)){
                  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
                    ulsch_config_pdu_0_1->n_layers = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][16];
                    ulsch_config_pdu_0_1->tpmi     = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][17];
                  }
                  if (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.codebook_subset == codebook_subset_nonCoherent){
                    ulsch_config_pdu_0_1->n_layers = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][18];
                    ulsch_config_pdu_0_1->tpmi     = table_7_3_1_1_2_2_3_4_5[dci->precod_nbr_layers][19];
                  }
                }
              }
            }
        /* ANTENNA_PORTS */
            uint8_t rank=0; // We need to initialize rank FIXME!!!
            if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_enabled) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 1)) { // tables 7.3.1.1.2-6
              ulsch_config_pdu_0_1->n_dmrs_cdm_groups = 2;
              ulsch_config_pdu_0_1->dmrs_ports[0] = dci->antenna_ports;
            }
            if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_enabled) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 2)) { // tables 7.3.1.1.2-7
              ulsch_config_pdu_0_1->n_dmrs_cdm_groups = 2;
              ulsch_config_pdu_0_1->dmrs_ports[0] = (dci->antenna_ports > 3)?(dci->antenna_ports-4):(dci->antenna_ports);
              ulsch_config_pdu_0_1->n_front_load_symb = (dci->antenna_ports > 3)?2:1;
            }
            if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 1)) { // tables 7.3.1.1.2-8/9/10/11
              if (rank == 1){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = (dci->antenna_ports > 1)?2:1;
                ulsch_config_pdu_0_1->dmrs_ports[0] = (dci->antenna_ports > 1)?(dci->antenna_ports-2):(dci->antenna_ports);
              }
              if (rank == 2){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = (dci->antenna_ports > 0)?2:1;
                ulsch_config_pdu_0_1->dmrs_ports[0] = (dci->antenna_ports > 1)?(dci->antenna_ports > 2 ?0:2):0;
                ulsch_config_pdu_0_1->dmrs_ports[1] = (dci->antenna_ports > 1)?(dci->antenna_ports > 2 ?2:3):1;
              }
              if (rank == 3){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = 2;
                ulsch_config_pdu_0_1->dmrs_ports[0] = 0;
                ulsch_config_pdu_0_1->dmrs_ports[1] = 1;
                ulsch_config_pdu_0_1->dmrs_ports[2] = 2;
              }
              if (rank == 4){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = 2;
                ulsch_config_pdu_0_1->dmrs_ports[0] = 0;
                ulsch_config_pdu_0_1->dmrs_ports[1] = 1;
                ulsch_config_pdu_0_1->dmrs_ports[2] = 2;
                ulsch_config_pdu_0_1->dmrs_ports[3] = 3;
              }
            }
            if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 2)) { // tables 7.3.1.1.2-12/13/14/15
              if (rank == 1){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = (dci->antenna_ports > 1)?2:1;
                ulsch_config_pdu_0_1->dmrs_ports[0] = (dci->antenna_ports > 1)?(dci->antenna_ports > 5 ?(dci->antenna_ports-6):(dci->antenna_ports-2)):dci->antenna_ports;
                ulsch_config_pdu_0_1->n_front_load_symb = (dci->antenna_ports > 6)?2:1;
              }
              if (rank == 2){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = (dci->antenna_ports > 0)?2:1;
                ulsch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_13[dci->antenna_ports][1];
                ulsch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_13[dci->antenna_ports][2];
                ulsch_config_pdu_0_1->n_front_load_symb = (dci->antenna_ports > 3)?2:1;
              }
              if (rank == 3){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = 2;
                ulsch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_14[dci->antenna_ports][1];
                ulsch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_14[dci->antenna_ports][2];
                ulsch_config_pdu_0_1->dmrs_ports[2] = table_7_3_1_1_2_14[dci->antenna_ports][3];
                ulsch_config_pdu_0_1->n_front_load_symb = (dci->antenna_ports > 1)?2:1;
              }
              if (rank == 4){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = 2;
                ulsch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_15[dci->antenna_ports][1];
                ulsch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_15[dci->antenna_ports][2];
                ulsch_config_pdu_0_1->dmrs_ports[2] = table_7_3_1_1_2_15[dci->antenna_ports][3];
                ulsch_config_pdu_0_1->dmrs_ports[3] = table_7_3_1_1_2_15[dci->antenna_ports][4];
                ulsch_config_pdu_0_1->n_front_load_symb = (dci->antenna_ports > 1)?2:1;
              }
            }
            if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 2) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 1)) { // tables 7.3.1.1.2-16/17/18/19
              if (rank == 1){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = (dci->antenna_ports > 1)?((dci->antenna_ports > 5)?3:2):1;
                ulsch_config_pdu_0_1->dmrs_ports[0] = (dci->antenna_ports > 1)?(dci->antenna_ports > 5 ?(dci->antenna_ports-6):(dci->antenna_ports-2)):dci->antenna_ports;
              }
              if (rank == 2){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = (dci->antenna_ports > 0)?((dci->antenna_ports > 2)?3:2):1;
                ulsch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_17[dci->antenna_ports][1];
                ulsch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_17[dci->antenna_ports][2];
              }
              if (rank == 3){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = (dci->antenna_ports > 0)?3:2;
                ulsch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_18[dci->antenna_ports][1];
                ulsch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_18[dci->antenna_ports][2];
                ulsch_config_pdu_0_1->dmrs_ports[2] = table_7_3_1_1_2_18[dci->antenna_ports][3];
              }
              if (rank == 4){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = dci->antenna_ports + 2;
                ulsch_config_pdu_0_1->dmrs_ports[0] = 0;
                ulsch_config_pdu_0_1->dmrs_ports[1] = 1;
                ulsch_config_pdu_0_1->dmrs_ports[2] = 2;
                ulsch_config_pdu_0_1->dmrs_ports[3] = 3;
              }
            }
            if ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 2) &&
            (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.max_length == 2)) { // tables 7.3.1.1.2-20/21/22/23
              if (rank == 1){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = table_7_3_1_1_2_20[dci->antenna_ports][0];
                ulsch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_20[dci->antenna_ports][1];
                ulsch_config_pdu_0_1->n_front_load_symb = table_7_3_1_1_2_20[dci->antenna_ports][2];
              }
              if (rank == 2){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = table_7_3_1_1_2_21[dci->antenna_ports][0];
                ulsch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_21[dci->antenna_ports][1];
                ulsch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_21[dci->antenna_ports][2];
                ulsch_config_pdu_0_1->n_front_load_symb = table_7_3_1_1_2_21[dci->antenna_ports][3];
              }
              if (rank == 3){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = table_7_3_1_1_2_22[dci->antenna_ports][0];
                ulsch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_22[dci->antenna_ports][1];
                ulsch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_22[dci->antenna_ports][2];
                ulsch_config_pdu_0_1->dmrs_ports[2] = table_7_3_1_1_2_22[dci->antenna_ports][3];
                ulsch_config_pdu_0_1->n_front_load_symb = table_7_3_1_1_2_22[dci->antenna_ports][4];
                }
              if (rank == 4){
                ulsch_config_pdu_0_1->n_dmrs_cdm_groups = table_7_3_1_1_2_23[dci->antenna_ports][0];
                ulsch_config_pdu_0_1->dmrs_ports[0] = table_7_3_1_1_2_23[dci->antenna_ports][1];
                ulsch_config_pdu_0_1->dmrs_ports[1] = table_7_3_1_1_2_23[dci->antenna_ports][2];
                ulsch_config_pdu_0_1->dmrs_ports[2] = table_7_3_1_1_2_23[dci->antenna_ports][3];
                ulsch_config_pdu_0_1->dmrs_ports[3] = table_7_3_1_1_2_23[dci->antenna_ports][4];
                ulsch_config_pdu_0_1->n_front_load_symb = table_7_3_1_1_2_23[dci->antenna_ports][5];
              }
            }
        /* SRS_REQUEST */
            // if SUL is supported in the cell, there is an additional bit in thsi field and the value of this bit represents table 7.1.1.1-1 TS 38.212 FIXME!!!
            ulsch_config_pdu_0_1->srs_config.aperiodicSRS_ResourceTrigger = (dci->srs_request & 0x11); // as per Table 7.3.1.1.2-24 TS 38.212
        /* CSI_REQUEST */
            ulsch_config_pdu_0_1->csi_reportTriggerSize = dci->csi_request;
        /* CBGTI */
            ulsch_config_pdu_0_1->maxCodeBlockGroupsPerTransportBlock = dci->cbgti;
        /* PTRS_DMRS */
            if (((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_disabled) &&
                 (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.dmrs_ul_for_pusch_mapping_type_a.ptrs_uplink_config == 0)) ||
                ((mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.transform_precoder == transform_precoder_enabled) &&
                 (mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated.max_rank == 1))){
            } else {
              ulsch_config_pdu_0_1->ptrs_dmrs_association_port = dci->ptrs_dmrs;
            }
        /* BETA_OFFSET_IND */
            // Table 9.3-3 in [5, TS 38.213]
            ulsch_config_pdu_0_1->beta_offset_ind = dci->beta_offset_ind;
        /* DMRS_SEQ_INI */
            // FIXME!!
        /* UL_SCH_IND */
            // A value of "1" indicates UL-SCH shall be transmitted on the PUSCH and
            // a value of "0" indicates UL-SCH shall not be transmitted on the PUSCH

            ul_config->number_pdus = ul_config->number_pdus + 1;
            break;

        case format1_0: 
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
            //fapi_nr_dl_config_dlsch_pdu_rel15_t dlsch_config_pdu_1_0 = dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.dlsch_config_rel15;
            fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu_1_0 = &dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.dlsch_config_rel15;
        /* IDENTIFIER_DCI_FORMATS */
        /* FREQ_DOM_RESOURCE_ASSIGNMENT_DL */
           nr_ue_process_dci_freq_dom_resource_assignment(NULL,dlsch_config_pdu_1_0,0,n_RB_DLBWP,dci->freq_dom_resource_assignment_DL);
        /* TIME_DOM_RESOURCE_ASSIGNMENT */
            nr_ue_process_dci_time_dom_resource_assignment(NULL,dlsch_config_pdu_1_0,dci->time_dom_resource_assignment,mac->mib->dmrs_TypeA_Position);
        /* VRB_TO_PRB_MAPPING */
            dlsch_config_pdu_1_0->vrb_to_prb_mapping = (dci->vrb_to_prb_mapping == 0) ? vrb_to_prb_mapping_non_interleaved:vrb_to_prb_mapping_interleaved;
        /* MCS */
            dlsch_config_pdu_1_0->mcs = dci->mcs;
       /* NDI (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
            dlsch_config_pdu_1_0->ndi = dci->ndi;
        /* RV (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
            dlsch_config_pdu_1_0->rv = dci->rv;
        /* HARQ_PROCESS_NUMBER (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
            dlsch_config_pdu_1_0->harq_process_nbr = dci->harq_process_number;
        /* DAI (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
            dlsch_config_pdu_1_0->dai = dci ->dai;
        /* TB_SCALING (only if CRC scrambled by P-RNTI or RA-RNTI) */
            // according to TS 38.214 Table 5.1.3.2-3
            if (dci->tb_scaling == 0) dlsch_config_pdu_1_0->scaling_factor_S = 1;
            if (dci->tb_scaling == 1) dlsch_config_pdu_1_0->scaling_factor_S = 0.5;
            if (dci->tb_scaling == 2) dlsch_config_pdu_1_0->scaling_factor_S = 0.25;
            if (dci->tb_scaling == 3) dlsch_config_pdu_1_0->scaling_factor_S = 0; // value not defined in table
        /* TPC_PUCCH (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI or TC-RNTI)*/
            // according to TS 38.213 Table 7.2.1-1
            if (dci->tpc_pucch == 0) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = -1;
            if (dci->tpc_pucch == 1) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = 0;
            if (dci->tpc_pucch == 2) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = 1;
            if (dci->tpc_pucch == 3) dlsch_config_pdu_1_0->accumulated_delta_PUCCH = 3;
        /* PUCCH_RESOURCE_IND (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI)*/
            //if (dci->pucch_resource_ind == 0) dlsch_config_pdu_1_0->pucch_resource_id = 1; //pucch-ResourceId obtained from the 1st value of resourceList FIXME!!!
            //if (dci->pucch_resource_ind == 1) dlsch_config_pdu_1_0->pucch_resource_id = 2; //pucch-ResourceId obtained from the 2nd value of resourceList FIXME!!
            //if (dci->pucch_resource_ind == 2) dlsch_config_pdu_1_0->pucch_resource_id = 3; //pucch-ResourceId obtained from the 3rd value of resourceList FIXME!!
            //if (dci->pucch_resource_ind == 3) dlsch_config_pdu_1_0->pucch_resource_id = 4; //pucch-ResourceId obtained from the 4th value of resourceList FIXME!!
            //if (dci->pucch_resource_ind == 4) dlsch_config_pdu_1_0->pucch_resource_id = 5; //pucch-ResourceId obtained from the 5th value of resourceList FIXME!!
            //if (dci->pucch_resource_ind == 5) dlsch_config_pdu_1_0->pucch_resource_id = 6; //pucch-ResourceId obtained from the 6th value of resourceList FIXME!!
            //if (dci->pucch_resource_ind == 6) dlsch_config_pdu_1_0->pucch_resource_id = 7; //pucch-ResourceId obtained from the 7th value of resourceList FIXME!!
            //if (dci->pucch_resource_ind == 7) dlsch_config_pdu_1_0->pucch_resource_id = 8; //pucch-ResourceId obtained from the 8th value of resourceList FIXME!!
            dlsch_config_pdu_1_0->pucch_resource_id = dci->pucch_resource_ind;
        /* PDSCH_TO_HARQ_FEEDBACK_TIME_IND (only if CRC scrambled by C-RNTI or CS-RNTI or new-RNTI)*/
            dlsch_config_pdu_1_0->pdsch_to_harq_feedback_time_ind = dci->pdsch_to_harq_feedback_time_ind;

            LOG_D(MAC,"(nr_ue_procedures.c) rnti=%d dl_config->number_pdus=%d\n",
                    dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.rnti,
                    dl_config->number_pdus);
            LOG_D(MAC,"(nr_ue_procedures.c) frequency_domain_resource_assignment=%d \t number_rbs=%d \t start_rb=%d\n",
                    dci->freq_dom_resource_assignment_DL,
                    dlsch_config_pdu_1_0->number_rbs,
                    dlsch_config_pdu_1_0->start_rb);
            LOG_D(MAC,"(nr_ue_procedures.c) time_domain_resource_assignment=%d \t number_symbols=%d \t start_symbol=%d\n",
                    dci->time_dom_resource_assignment,
                    dlsch_config_pdu_1_0->number_symbols,
                    dlsch_config_pdu_1_0->start_symbol);
            LOG_D(MAC,"(nr_ue_procedures.c) vrb_to_prb_mapping=%d \n>>> mcs=%d\n>>> ndi=%d\n>>> rv=%d\n>>> harq_process_nbr=%d\n>>> dai=%d\n>>> scaling_factor_S=%f\n>>> tpc_pucch=%d\n>>> pucch_res_ind=%d\n>>> pdsch_to_harq_feedback_time_ind=%d\n",
                  dlsch_config_pdu_1_0->vrb_to_prb_mapping,
                  dlsch_config_pdu_1_0->mcs,
                  dlsch_config_pdu_1_0->ndi,
                  dlsch_config_pdu_1_0->rv,
                  dlsch_config_pdu_1_0->harq_process_nbr,
                  dlsch_config_pdu_1_0->dai,
                  dlsch_config_pdu_1_0->scaling_factor_S,
                  dlsch_config_pdu_1_0->accumulated_delta_PUCCH,
                  dlsch_config_pdu_1_0->pucch_resource_id,
                  dlsch_config_pdu_1_0->pdsch_to_harq_feedback_time_ind);

            dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
	    dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP = n_RB_DLBWP;
	    
            LOG_D(MAC,"(nr_ue_procedures.c) pdu_type=%d\n\n",dl_config->dl_config_list[dl_config->number_pdus].pdu_type);
            
            dl_config->number_pdus = dl_config->number_pdus + 1;
            break;

        case format1_1:        
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
            dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
            dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.rnti = rnti;
            fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu_1_1 = &dl_config->dl_config_list[dl_config->number_pdus].dlsch_config_pdu.dlsch_config_rel15;
        /* IDENTIFIER_DCI_FORMATS */
        /* CARRIER_IND */
        /* BANDWIDTH_PART_IND */
            dlsch_config_pdu_1_1->bandwidth_part_ind = dci->bandwidth_part_ind;
        /* FREQ_DOM_RESOURCE_ASSIGNMENT_DL */
            nr_ue_process_dci_freq_dom_resource_assignment(NULL,dlsch_config_pdu_1_1,0,n_RB_DLBWP,dci->freq_dom_resource_assignment_DL);
        /* TIME_DOM_RESOURCE_ASSIGNMENT */
            nr_ue_process_dci_time_dom_resource_assignment(NULL,dlsch_config_pdu_1_1,dci->time_dom_resource_assignment,mac->mib->dmrs_TypeA_Position);
        /* VRB_TO_PRB_MAPPING */
            if (mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.resource_allocation != 0)
              dlsch_config_pdu_1_1->vrb_to_prb_mapping = (dci->vrb_to_prb_mapping == 0) ? vrb_to_prb_mapping_non_interleaved:vrb_to_prb_mapping_interleaved;
        /* PRB_BUNDLING_SIZE_IND */
            dlsch_config_pdu_1_1->prb_bundling_size_ind = dci->prb_bundling_size_ind;
        /* RATE_MATCHING_IND */
            dlsch_config_pdu_1_1->rate_matching_ind = dci->rate_matching_ind;
        /* ZP_CSI_RS_TRIGGER */
            dlsch_config_pdu_1_1->zp_csi_rs_trigger = dci->zp_csi_rs_trigger;
        /* MCS (for transport block 1)*/
            dlsch_config_pdu_1_1->mcs = dci->tb1_mcs;
        /* NDI (for transport block 1)*/
            dlsch_config_pdu_1_1->ndi = dci->tb1_ndi;
        /* RV (for transport block 1)*/
            dlsch_config_pdu_1_1->rv = dci->tb1_rv;
        /* MCS (for transport block 2)*/
            dlsch_config_pdu_1_1->tb2_mcs = dci->tb2_mcs;
        /* NDI (for transport block 2)*/
            dlsch_config_pdu_1_1->tb2_ndi = dci->tb2_ndi;
        /* RV (for transport block 2)*/
            dlsch_config_pdu_1_1->tb2_rv = dci->tb2_rv;
        /* HARQ_PROCESS_NUMBER */
            dlsch_config_pdu_1_1->harq_process_nbr = dci->harq_process_number;
        /* DAI */
            dlsch_config_pdu_1_1->dai = dci ->dai;
        /* TPC_PUCCH */
            // according to TS 38.213 Table 7.2.1-1
            if (dci->tpc_pucch == 0) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = -1;
            if (dci->tpc_pucch == 1) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = 0;
            if (dci->tpc_pucch == 2) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = 1;
            if (dci->tpc_pucch == 3) dlsch_config_pdu_1_1->accumulated_delta_PUCCH = 3;
        /* PUCCH_RESOURCE_IND */
            if (dci->pucch_resource_ind == 0) dlsch_config_pdu_1_1->pucch_resource_id = 0; //pucch-ResourceId obtained from the 1st value of resourceList FIXME!!!
            if (dci->pucch_resource_ind == 1) dlsch_config_pdu_1_1->pucch_resource_id = 0; //pucch-ResourceId obtained from the 2nd value of resourceList FIXME!!
            if (dci->pucch_resource_ind == 2) dlsch_config_pdu_1_1->pucch_resource_id = 0; //pucch-ResourceId obtained from the 3rd value of resourceList FIXME!!
            if (dci->pucch_resource_ind == 3) dlsch_config_pdu_1_1->pucch_resource_id = 0; //pucch-ResourceId obtained from the 4th value of resourceList FIXME!!
            if (dci->pucch_resource_ind == 4) dlsch_config_pdu_1_1->pucch_resource_id = 0; //pucch-ResourceId obtained from the 5th value of resourceList FIXME!!
            if (dci->pucch_resource_ind == 5) dlsch_config_pdu_1_1->pucch_resource_id = 0; //pucch-ResourceId obtained from the 6th value of resourceList FIXME!!
            if (dci->pucch_resource_ind == 6) dlsch_config_pdu_1_1->pucch_resource_id = 0; //pucch-ResourceId obtained from the 7th value of resourceList FIXME!!
            if (dci->pucch_resource_ind == 7) dlsch_config_pdu_1_1->pucch_resource_id = 0; //pucch-ResourceId obtained from the 8th value of resourceList FIXME!!
        /* PDSCH_TO_HARQ_FEEDBACK_TIME_IND */
            // according to TS 38.213 Table 9.2.3-1
            dlsch_config_pdu_1_1-> pdsch_to_harq_feedback_time_ind = mac->phy_config.config_req.ul_bwp_dedicated.pucch_config_dedicated.dl_data_to_ul_ack[dci->pdsch_to_harq_feedback_time_ind];
        /* ANTENNA_PORTS */
            uint8_t n_codewords = 1; // FIXME!!!
            if ((mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.dmrs_type == 1) &&
                (mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.max_length == 1)){
            // Table 7.3.1.2.2-1: Antenna port(s) (1000 + DMRS port), dmrs-Type=1, maxLength=1
              dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_1[dci->antenna_ports][0];
              dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_1[dci->antenna_ports][1];
              dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_1[dci->antenna_ports][2];
              dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_1[dci->antenna_ports][3];
              dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_1[dci->antenna_ports][4];
            }
            if ((mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.dmrs_type == 1) &&
                (mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.max_length == 2)){
            // Table 7.3.1.2.2-2: Antenna port(s) (1000 + DMRS port), dmrs-Type=1, maxLength=2
              if (n_codewords == 1) {
                dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports][0];
                dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports][1];
                dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports][2];
                dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports][3];
                dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports][4];
                dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_2_oneCodeword[dci->antenna_ports][5];
              }
              if (n_codewords == 1) {
                dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports][0];
                dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports][1];
                dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports][2];
                dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports][3];
                dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports][4];
                dlsch_config_pdu_1_1->dmrs_ports[4]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports][5];
                dlsch_config_pdu_1_1->dmrs_ports[5]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports][6];
                dlsch_config_pdu_1_1->dmrs_ports[6]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports][7];
                dlsch_config_pdu_1_1->dmrs_ports[7]     = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports][8];
                dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_2_twoCodeword[dci->antenna_ports][9];
              }
            }
            if ((mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.dmrs_type == 2) &&
                (mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.max_length == 1)){
            // Table 7.3.1.2.2-3: Antenna port(s) (1000 + DMRS port), dmrs-Type=2, maxLength=1
                if (n_codewords == 1) {
                  dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports][0];
                  dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports][1];
                  dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports][2];
                  dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports][3];
                  dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_3_oneCodeword[dci->antenna_ports][4];
                }
                if (n_codewords == 1) {
                  dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports][0];
                  dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports][1];
                  dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports][2];
                  dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports][3];
                  dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports][4];
                  dlsch_config_pdu_1_1->dmrs_ports[4]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports][5];
                  dlsch_config_pdu_1_1->dmrs_ports[5]     = table_7_3_2_3_3_3_twoCodeword[dci->antenna_ports][6];
                }
            }
            if ((mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.dmrs_type == 2) &&
                (mac->phy_config.config_req.dl_bwp_dedicated.pdsch_config_dedicated.dmrs_dl_for_pdsch_mapping_type_a.max_length == 2)){
            // Table 7.3.1.2.2-4: Antenna port(s) (1000 + DMRS port), dmrs-Type=2, maxLength=2
                if (n_codewords == 1) {
                  dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports][0];
                  dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports][1];
                  dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports][2];
                  dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports][3];
                  dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports][4];
                  dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_4_oneCodeword[dci->antenna_ports][5];
                }
                if (n_codewords == 1) {
                  dlsch_config_pdu_1_1->n_dmrs_cdm_groups = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports][0];
                  dlsch_config_pdu_1_1->dmrs_ports[0]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports][1];
                  dlsch_config_pdu_1_1->dmrs_ports[1]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports][2];
                  dlsch_config_pdu_1_1->dmrs_ports[2]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports][3];
                  dlsch_config_pdu_1_1->dmrs_ports[3]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports][4];
                  dlsch_config_pdu_1_1->dmrs_ports[4]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports][5];
                  dlsch_config_pdu_1_1->dmrs_ports[5]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports][6];
                  dlsch_config_pdu_1_1->dmrs_ports[6]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports][7];
                  dlsch_config_pdu_1_1->dmrs_ports[7]     = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports][8];
                  dlsch_config_pdu_1_1->n_front_load_symb = table_7_3_2_3_3_4_twoCodeword[dci->antenna_ports][9];
                }
            }
        /* TCI */
            if (mac->dl_config_request.dl_config_list[0].dci_config_pdu.dci_config_rel15.coreset.tci_present_in_dci == 1){
              // 0 bit if higher layer parameter tci-PresentInDCI is not enabled
              // otherwise 3 bits as defined in Subclause 5.1.5 of [6, TS38.214]
              dlsch_config_pdu_1_1->tci_state = dci->tci;
            }
        /* SRS_REQUEST */
            // if SUL is supported in the cell, there is an additional bit in this field and the value of this bit represents table 7.1.1.1-1 TS 38.212 FIXME!!!
            dlsch_config_pdu_1_1->srs_config.aperiodicSRS_ResourceTrigger = (dci->srs_request & 0x11); // as per Table 7.3.1.1.2-24 TS 38.212
        /* CBGTI */
            dlsch_config_pdu_1_1->cbgti = dci->cbgti;
        /* CBGFI */
            dlsch_config_pdu_1_1->codeBlockGroupFlushIndicator = dci->cbgfi;
        /* DMRS_SEQ_INI */
            //FIXME!!!

	    dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15.N_RB_BWP = n_RB_DLBWP;
	    
            dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DLSCH;
            LOG_D(MAC,"(nr_ue_procedures.c) pdu_type=%d\n\n",dl_config->dl_config_list[dl_config->number_pdus].pdu_type);
            
            dl_config->number_pdus = dl_config->number_pdus + 1;

            break;

        case format2_0:        
            break;

        case format2_1:        
            break;

        case format2_2:        
            break;

        case format2_3:
            break;

        default: 
            break;
    }


    if(rnti == SI_RNTI){

    }else if(rnti == mac->ra_rnti){

    }else if(rnti == P_RNTI){

    }else{  //  c-rnti

        ///  check if this is pdcch order 
        //dci->random_access_preamble_index;
        //dci->ss_pbch_index;
        //dci->prach_mask_index;

        ///  else normal DL-SCH grant
    }
    return 0;
}

int8_t nr_ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP, uint8_t eNB_id, uint16_t rnti, sub_frame_t subframe){

    return 0;
}

void nr_ue_send_sdu(module_id_t module_idP,
                    uint8_t CC_id,
                    frame_t frameP,
                    uint8_t ttiP,
                    uint8_t * pdu, uint16_t pdu_len, uint8_t gNB_index,
                    NR_UL_TIME_ALIGNMENT_t *ul_time_alignment){

  LOG_D(MAC, "Handling PDU frame %d slot %d\n", frameP, ttiP);

  // Changes wrt LTE: replaced subframeP with ttiP
  // TODO double check this and eventually create a type for TTI (tti_t)
  // Note: pdu_len was previously named sdu and sdu_len

  uint8_t * pduP = pdu;
  NR_UE_MAC_INST_t *UE_mac_inst = get_mac_inst(module_idP);

  /*
  #if UE_TIMING_TRACE
    start_meas(&UE_mac_inst[module_idP].rx_dlsch_sdu);
  #endif
  */

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_IN);

  //LOG_D(MAC,"sdu: %x.%x.%x\n",sdu[0],sdu[1],sdu[2]);

  if (opt_enabled) {
    trace_pdu(DIRECTION_DOWNLINK, pduP, pdu_len, module_idP, WS_C_RNTI,
    UE_mac_inst[module_idP].cs_RNTI, frameP, ttiP, 0, 0); //subframeP
    LOG_D(OPT, "[UE %d][DLSCH] Frame %d trace pdu for rnti %x  with size %d\n",
      module_idP, frameP, UE_mac_inst[module_idP].cs_RNTI, pdu_len);
    }

  /*
  #ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC, "[UE %d] ue_send_sdu : Frame %d gNB_index %d : num_ce %d num_sdu %d\n",
      module_idP, frameP, gNB_index, num_ce, num_sdu);
  #endif
  */

  /*
  #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
    LOG_T(MAC, "[UE %d] First 32 bytes of DLSCH : \n", module_idP);
    for (i = 0; i < 32; i++) {
      LOG_T(MAC, "%x.", sdu[i]);
    }
    LOG_T(MAC, "\n");
  #endif
  */

  // Processing MAC PDU
  // it parses MAC CEs subheaders, MAC CEs, SDU subheaderds and SDUs
  nr_ue_process_mac_pdu(module_idP, CC_id, pduP, pdu_len, gNB_index, ul_time_alignment);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_OUT);

  /*
  #if UE_TIMING_TRACE
    stop_meas(&UE_mac_inst[module_idP].rx_dlsch_sdu);
  #endif
  */
}

void nr_ue_process_mac_pdu(
    module_id_t module_idP,
    uint8_t CC_id,
    uint8_t *pduP, 
    uint16_t mac_pdu_len,
    uint8_t gNB_index,
    NR_UL_TIME_ALIGNMENT_t *ul_time_alignment){

    // This function is adapting code from the old
    // parse_header(...) and ue_send_sdu(...) functions of OAI LTE

    uint8_t *pdu_ptr = pduP;
    int pdu_len = mac_pdu_len;

    uint16_t mac_ce_len;
    uint16_t mac_subheader_len;
    uint16_t mac_sdu_len;

    NR_UE_MAC_INST_t *UE_mac_inst = get_mac_inst(module_idP);
    //uint8_t scs = UE_mac_inst->mib->subCarrierSpacingCommon;
    //uint16_t bwp_ul_NB_RB = UE_mac_inst->initial_bwp_ul.N_RB;

    //  For both DL/UL-SCH
    //  Except:
    //   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
    //   - UL/DL-SCH: padding
    //   - UL-SCH:    MSG3 48-bits
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|F|   LCID    |
    //  |       L       |
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|F|   LCID    |
    //  |       L       |
    //  |       L       |

    //  For both DL/UL-SCH
    //  For:
    //   - UL/DL-SCH: fixed-size MAC CE(known by LCID)
    //   - UL/DL-SCH: padding, for single/multiple 1-oct padding CE(s)
    //   - UL-SCH:    MSG3 48-bits
    //  |0|1|2|3|4|5|6|7|  bit-wise
    //  |R|R|   LCID    |
    //  LCID: The Logical Channel ID field identifies the logical channel instance of the corresponding MAC SDU or the type of the corresponding MAC CE or padding as described in Tables 6.2.1-1 and 6.2.1-2 for the DL-SCH and UL-SCH respectively. There is one LCID field per MAC subheader. The LCID field size is 6 bits;
    //  L: The Length field indicates the length of the corresponding MAC SDU or variable-sized MAC CE in bytes. There is one L field per MAC subheader except for subheaders corresponding to fixed-sized MAC CEs and padding. The size of the L field is indicated by the F field;
    //  F: lenght of L is 0:8 or 1:16 bits wide
    //  R: Reserved bit, set to zero.

    uint8_t done = 0;
    
    while (!done && pdu_len > 0){
        mac_ce_len = 0;
        mac_subheader_len = 1; //  default to fixed-length subheader = 1-oct
        mac_sdu_len = 0;
        switch(((NR_MAC_SUBHEADER_FIXED *)pdu_ptr)->LCID){
            //  MAC CE

            /*#ifdef DEBUG_HEADER_PARSING
              LOG_D(MAC, "[UE] LCID %d, PDU length %d\n", ((NR_MAC_SUBHEADER_FIXED *)pdu_ptr)->LCID, pdu_len);
            #endif*/
            case DL_SCH_LCID_CCCH:
                //  MSG4 RRC Connection Setup 38.331
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }

                /*LOG_D(MAC, "[UE %d] rnti %x Frame %d : DLSCH -> DL-CCCH, RRC message (eNB %d, %d bytes)\n",
                    module_idP, UE_mac_inst[module_idP].crnti, frameP, eNB_index, rx_lengths[i]);*/

                /*#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
                  int j;
                  for (j = 0; j < rx_lengths[i]; j++) {
                    LOG_T(MAC, "%x.", (uint8_t) payload_ptr[j]);
                  }
                  LOG_T(MAC, "\n");
                #endif*/

                /*mac_rrc_data_ind_ue(module_idP,
                            CC_id,
                            frameP, subframeP,
                            UE_mac_inst[module_idP].crnti,
                            CCCH,
                            (uint8_t *) payload_ptr,
                            rx_lengths[i], eNB_index, 0);*/
                break;

            case DL_SCH_LCID_TCI_STATE_ACT_UE_SPEC_PDSCH:

                //  38.321 Ch6.1.3.14
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_APERIODIC_CSI_TRI_STATE_SUBSEL:
                //  38.321 Ch6.1.3.13
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_SP_CSI_RS_CSI_IM_RES_SET_ACT:
                //  38.321 Ch6.1.3.12
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                }
                break;
            case DL_SCH_LCID_SP_SRS_ACTIVATION:
                //  38.321 Ch6.1.3.17
                //  varialbe length
                mac_ce_len |= (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                mac_subheader_len = 2;
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    mac_ce_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
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

                /*uint8_t ta_command = ((NR_MAC_CE_TA *)pdu_ptr)[1].TA_COMMAND;
                uint8_t tag_id = ((NR_MAC_CE_TA *)pdu_ptr)[1].TAGID;*/

                ul_time_alignment->apply_ta = 1;
                ul_time_alignment->ta_command = ((NR_MAC_CE_TA *)pdu_ptr)[1].TA_COMMAND;
                ul_time_alignment->tag_id = ((NR_MAC_CE_TA *)pdu_ptr)[1].TAGID;

                /*
                #ifdef DEBUG_HEADER_PARSING
                LOG_D(MAC, "[UE] CE %d : UE Timing Advance : %d\n", i, pdu_ptr[1]);
                #endif
                */

                LOG_D(MAC, "Received TA_COMMAND %u TAGID %u CC_id %d\n",
                  ul_time_alignment->ta_command, ul_time_alignment->tag_id, CC_id);

                break;
            case DL_SCH_LCID_CON_RES_ID:
                //  38.321 Ch6.1.3.3
                mac_ce_len = 6;

                // TODO index should be shifted by 1
                // LOG_I(MAC,"[UE %d][RAPROC] Frame %d : received contention resolution msg: %x.%x.%x.%x.%x.%x, Terminating RA procedure\n", module_idP, frameP, payload_ptr[0], payload_ptr[1], payload_ptr[2], payload_ptr[3], payload_ptr[4], payload_ptr[5]);

                // TODO RACH is missing

                /*if (UE_mac_inst[module_idP].RA_active == 1) {
                  LOG_I(MAC, "[UE %d][RAPROC] Frame %d : Clearing RA_active flag\n",
                    module_idP, frameP);
                    UE_mac_inst[module_idP].RA_active = 0;
                    // check if RA procedure has finished completely (no contention)
                    tx_sdu = &UE_mac_inst[module_idP].CCCH_pdu.payload[3];

                  //Note: 3 assumes sizeof(SCH_SUBHEADER_SHORT) + PADDING CE, which is when UL-Grant has TBS >= 9 (64 bits)
                  // (other possibility is 1 for TBS=7 (SCH_SUBHEADER_FIXED), or 2 for TBS=8 (SCH_SUBHEADER_FIXED+PADDING or SCH_SUBHEADER_SHORT)
                  for (i = 0; i < 6; i++)
                    if (tx_sdu[i] != payload_ptr[i]) {
                      LOG_E(MAC, "[UE %d][RAPROC] Contention detected, RA failed\n", module_idP);
                      if(nfapi_mode == 3) { // phy_stub mode
                      //  Modification for phy_stub mode operation here. We only need to make sure that the ue_mode is back to
                      // PRACH state.
                        LOG_I(MAC, "nfapi_mode3: Setting UE_mode BACK to PRACH 1\n");
                        UE_mac_inst[module_idP].UE_mode[eNB_index] = PRACH;
                      //ra_failed(module_idP,CC_id,eNB_index);UE_mac_inst[module_idP].RA_contention_resolution_timer_active = 0;
                      }
                      else{
                        ra_failed(module_idP, CC_id, eNB_index);
                      }
                    UE_mac_inst[module_idP].RA_contention_resolution_timer_active = 0;
                    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_OUT);
                    return;
                    }
                    LOG_I(MAC,"[UE %d][RAPROC] Frame %d : Clearing contention resolution timer\n", module_idP, frameP);
                    UE_mac_inst[module_idP].RA_contention_resolution_timer_active = 0;
                    if(nfapi_mode == 3){
                      // phy_stub mode
                      // Modification for phy_stub mode operation here. We only need to change the ue_mode to PUSCH
                      UE_mac_inst[module_idP].UE_mode[eNB_index] = PUSCH;
                  } else { // Full stack mode
                    ra_succeeded(module_idP,CC_id,eNB_index);
                  }
                }*/

                break;
            case DL_SCH_LCID_PADDING:
                done = 1;
                //  end of MAC PDU, can ignore the rest.
                break;

            //  MAC SDU
            /* TODO double check SRBs 1-3 & DRB 1-3 */

            case DL_SCH_LCID_SRB1:
            //  check if LCID is valid at current time.

              /*if ((rx_lcids[i] == DCCH) || (rx_lcids[i] == DCCH1)) {

              LOG_D(MAC, "[UE %d] Frame %d : DLSCH -> DL-DCCH%d, RRC message (eNB %d, %d bytes)\n",
              module_idP, frameP, rx_lcids[i], eNB_index, rx_lengths[i]);

              mac_rlc_data_ind(module_idP, UE_mac_inst[module_idP].crnti, eNB_index, frameP, ENB_FLAG_NO,
              MBMS_FLAG_NO, rx_lcids[i], (char *) payload_ptr, rx_lengths[i], 1, NULL);
              } else if ((rx_lcids[i] < NB_RB_MAX) && (rx_lcids[i] > DCCH1)) {
              LOG_D(MAC, "[UE %d] Frame %d : DLSCH -> DL-DTCH%d (eNB %d, %d bytes)\n",
              module_idP, frameP, rx_lcids[i], eNB_index, rx_lengths[i]);

              #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
                int j;
                for (j = 0; j < rx_lengths[i]; j++)
                LOG_T(MAC, "%x.", (unsigned char) payload_ptr[j]);
                LOG_T(MAC, "\n");
              #endif

              mac_rlc_data_ind(module_idP,
                         UE_mac_inst[module_idP].crnti,
                         eNB_index,
                         frameP,
                         ENB_FLAG_NO,
                         MBMS_FLAG_NO,
                         rx_lcids[i],
                         (char *) payload_ptr, rx_lengths[i], 1, NULL);
            }*/

            case UL_SCH_LCID_SRB2:
                //  check if LCID is valid at current time.
            case UL_SCH_LCID_SRB3:
                //  check if LCID is valid at current time.
            default:
                //  check if LCID is valid at current time.
                if(((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->F){
                    //mac_sdu_len |= (uint16_t)(((NR_MAC_SUBHEADER_LONG *)pdu_ptr)->L2)<<8;
                    mac_subheader_len = 3;
                    mac_sdu_len = ((uint16_t)(((NR_MAC_SUBHEADER_LONG *) pdu_ptr)->L1 & 0x7f) << 8)
                    | ((uint16_t)((NR_MAC_SUBHEADER_LONG *) pdu_ptr)->L2 & 0xff);

                } else {
                  mac_sdu_len = (uint16_t)((NR_MAC_SUBHEADER_SHORT *)pdu_ptr)->L;
                  mac_subheader_len = 2;
                }

                //  DRB LCID by RRC
                break;
        }
        pdu_ptr += ( mac_subheader_len + mac_ce_len + mac_sdu_len );
        pdu_len -= ( mac_subheader_len + mac_ce_len + mac_sdu_len );

        AssertFatal(pdu_len >= 0, "[MAC] nr_ue_process_mac_pdu, residual mac pdu length < 0!\n");
    }
}
