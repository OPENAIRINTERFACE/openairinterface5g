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

/* \file proto.h
 * \brief MAC functions prototypes for gNB and UE
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef __LAYER2_MAC_UE_PROTO_H__
#define __LAYER2_MAC_UE_PROTO_H__

#include "mac_defs.h"
#include "mac.h"
#include "PHY/defs_nr_UE.h"
#include "RRC/NR_UE/rrc_defs.h"


/**\brief decode mib pdu in NR_UE, from if_module ul_ind with P7 tx_ind message
   \param module_id      module id
   \param cc_id          component carrier id
   \param gNB_index      gNB index
   \param extra_bits     extra bits for frame calculation
   \param l_ssb_equal_64 check if ssb number of candicate is equal 64, 1=equal; 0=non equal. Reference 38.212 7.1.1
   \param pduP           pointer to pdu
   \param pdu_length     length of pdu
   \param cell_id        cell id */
int8_t nr_ue_decode_mib(
    module_id_t module_id, 
    int cc_id, 
    uint8_t gNB_index, 
    uint8_t extra_bits, 
    uint32_t ssb_length, 
    uint32_t ssb_index,
    void *pduP, 
    uint16_t cell_id );


/**\brief primitive from RRC layer to MAC layer for configuration L1/L2, now supported 4 rrc messages: MIB, cell_group_config for MAC/PHY, spcell_config(serving cell config)
   \param module_id                 module id
   \param cc_id                     component carrier id
   \param gNB_index                 gNB index
   \param mibP                      pointer to RRC message MIB
   \param sccP                      pointer to ServingCellConfigCommon structure,
   \param spcell_configP            pointer to RRC message serving cell config*/
int nr_rrc_mac_config_req_ue(
    module_id_t                     module_id,
    int                             cc_idP,
    uint8_t                         gNB_index,
    NR_MIB_t                        *mibP,
    //NR_ServingCellConfigCommon_t    *sccP,
    NR_SpCellConfig_t               *spCell_ConfigP);

/**\brief initialization NR UE MAC instance(s), total number of MAC instance based on NB_NR_UE_MAC_INST*/
NR_UE_MAC_INST_t * nr_l2_init_ue(NR_UE_RRC_INST_t* rrc_inst);

/**\brief fetch MAC instance by module_id, within 0 - (NB_NR_UE_MAC_INST-1)
   \param module_id index of MAC instance(s)*/
NR_UE_MAC_INST_t *get_mac_inst(
    module_id_t module_id);

/**\brief called at each slot, slot length based on numerology. now use u=0, scs=15kHz, slot=1ms
          performs BSR/SR/PHR procedures, random access procedure handler and DLSCH/ULSCH procedures.
   \param module_id     module id
   \param gNB_index     corresponding gNB index
   \param cc_id         component carrier id
   \param rx_frame      receive frame number
   \param rx_slot       receive slot number
   \param tx_frame      transmit frame number
   \param tx_slot       transmit slot number*/
NR_UE_L2_STATE_t nr_ue_scheduler(
    const module_id_t module_id,
    const uint8_t gNB_index,
    const int cc_id,
    const frame_t rx_frame,
    const slot_t rx_slot,
    const int32_t ssb_index,
    const frame_t tx_frame,
    const slot_t tx_slot);


/* \brief Get SR payload (0,1) from UE MAC
@param Mod_id Instance id of UE in machine
@param CC_id Component Carrier index
@param eNB_id Index of eNB that UE is attached to
@param rnti C_RNTI of UE
@param subframe subframe number
@returns 0 for no SR, 1 for SR
*/
uint32_t ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP,
       uint8_t eNB_id, rnti_t rnti, sub_frame_t subframe);

int8_t nr_ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP, uint8_t eNB_id, uint16_t rnti, sub_frame_t subframe);

int8_t nr_ue_process_dci(module_id_t module_id, int cc_id, uint8_t gNB_index, nr_dci_pdu_rel15_t *dci, uint16_t rnti, uint32_t dci_format);
int nr_ue_process_dci_indication_pdu(module_id_t module_id,int cc_id, int gNB_index,fapi_nr_dci_indication_pdu_t *dci);

uint32_t get_ssb_frame(uint32_t test);
uint32_t get_ssb_slot(uint32_t ssb_index);


uint32_t mr_ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP, uint8_t eNB_id, uint16_t rnti, sub_frame_t subframe);

/* \brief Get payload (MAC PDU) from UE PHY
@param module_idP Instance id of UE in machine
@param CC_id Component Carrier index
@param frameP Current Rx frame
@param slotP Current Rx slot
@param pdu Pointer to the MAC PDU
@param pdu_len Length of the MAC PDU
@param gNB_id Index of gNB that UE is attached to
@param ul_time_alignment of struct handling the timing advance parameters
@returns void
*/
void nr_ue_send_sdu(module_id_t module_idP, 
                    uint8_t CC_id,
                    frame_t frameP,
                    int slotP,
                    uint8_t * pdu,
                    uint16_t pdu_len,
                    uint8_t gNB_index,
                    NR_UL_TIME_ALIGNMENT_t *ul_time_alignment);

void nr_ue_process_mac_pdu(module_id_t module_idP,
                           uint8_t CC_id,
                           frame_t frameP,
                           uint8_t *pduP, 
                           uint16_t mac_pdu_len,
                           uint8_t gNB_index,
                           NR_UL_TIME_ALIGNMENT_t *ul_time_alignment);


unsigned char nr_generate_ulsch_pdu(uint8_t *sdus_payload,
                                    uint8_t *pdu,
                                    uint8_t num_sdus,
                                    uint16_t *sdu_lengths,
                                    uint8_t *sdu_lcids,
                                    uint8_t power_headroom,
                                    uint16_t crnti,
                                    uint16_t truncated_bsr,
                                    uint16_t short_bsr,
                                    uint16_t long_bsr,
                                    unsigned short post_padding);

int8_t nr_ue_process_dlsch(module_id_t module_id, int cc_id, uint8_t gNB_index, fapi_nr_dci_indication_t *dci_ind, void *pduP, uint32_t pdu_len);

void ue_dci_configuration(NR_UE_MAC_INST_t *mac,fapi_nr_dl_config_request_t *dl_config,int frame,int slot);

void nr_extract_dci_info(NR_UE_MAC_INST_t *mac,
                         int dci_format,
                         uint8_t dci_length,
                         uint16_t rnti,
                         uint64_t *dci_pdu,
                         nr_dci_pdu_rel15_t *nr_pdci_info_extracted);


int set_tdd_config_nr_ue(fapi_nr_config_request_t *cfg, int mu,
                         int nrofDownlinkSlots, int nrofDownlinkSymbols,
                         int nrofUplinkSlots,   int nrofUplinkSymbols);

/** \brief Function for UE/PHY to compute PUSCH transmit power in power-control procedure.
    @param Mod_id Module id of UE
    @returns Po_NOMINAL_PUSCH (PREAMBLE_RECEIVED_TARGET_POWER+DELTA_PREAMBLE
*/
int8_t nr_get_Po_NOMINAL_PUSCH(module_id_t module_idP, uint8_t CC_id);

/** \brief Function to compute DELTA_PREAMBLE from 38.321 subclause 7.3
   (for RA power ramping procedure and Msg3 PUSCH power control policy)
    @param Mod_id Module id of UE
    @returns DELTA_PREAMBLE
*/
int8_t nr_get_DELTA_PREAMBLE(module_id_t mod_id, int CC_id);

/* \brief Function called by PHY to process the received RAR and check that the preamble matches what was sent by the gNB. It provides the timing advance and t-CRNTI.
@param Mod_id Index of UE instance
@param CC_id Index to a component carrier
@param frame Frame index
@param ra_rnti RA_RNTI value
@param dlsch_buffer  Pointer to dlsch_buffer containing RAR PDU
@param t_crnti Pointer to PHY variable containing the T_CRNTI
@param preamble_index Preamble Index used by PHY to transmit the PRACH.  This should match the received RAR to trigger the rest of
random-access procedure
@param selected_rar_buffer the output buffer for storing the selected RAR header and RAR payload
@returns timing advance or 0xffff if preamble doesn't match
*/
uint16_t nr_ue_process_rar(module_id_t mod_id,
                           int CC_id,
                           frame_t frameP,
                           uint8_t * dlsch_buffer,
                           rnti_t * t_crnti,
                           uint8_t preamble_index,
                           uint8_t * selected_rar_buffer);

void nr_process_rar(nr_downlink_indication_t *dl_info);

#endif
/** @}*/
