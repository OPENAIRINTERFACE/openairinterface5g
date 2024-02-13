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
#include "oai_asn1.h"
#include "RRC/NR_UE/rrc_defs.h"

#define NR_DL_MAX_DAI                            (4)                      /* TS 38.213 table 9.1.3-1 Value of counter DAI for DCI format 1_0 and 1_1 */
#define NR_DL_MAX_NB_CW                          (2)                      /* number of downlink code word */

// Macro updates DESTINATION with configuration from ORIGIN by swapping pointers
// Old configuration is freed after completing configuration
#define UPDATE_MAC_IE(DESTINATION, ORIGIN, TYPE) \
  do {                                           \
    TYPE *tmp = ORIGIN;                          \
    ORIGIN = DESTINATION;                        \
    DESTINATION = tmp;                           \
  } while(0);                                    \

// Same as above but swapping ASN1 elements that are not pointers
#define UPDATE_MAC_NP_IE(DESTINATION, ORIGIN, TYPE) \
  do {                                              \
    TYPE tmp = ORIGIN;                              \
    ORIGIN = DESTINATION;                           \
    DESTINATION = tmp;                              \
  } while(0);                                       \

// Macro handles reception of SetupRelease element ORIGIN (see NR_SetupRelease.h)
// If release (NR_SetupRelease_xxx_PR_release equivalent to 1), removing structure from DESTINATION
// If setup (NR_SetupRelease_xxx_PR_setup equivalent to 2), add or modify structure in DESTINATION
// Destination is not a SetupRelease structure
#define HANDLE_SETUPRELEASE_DIRECT(DESTINATION, ORIGIN, TYPE, ASN_DEF) \
  do {                                                                 \
    if (ORIGIN->present == 1) {                                        \
      asn1cFreeStruc(ASN_DEF, DESTINATION);                            \
    }                                                                  \
    if (ORIGIN->present == 2)                                          \
      UPDATE_MAC_IE(DESTINATION, ORIGIN->choice.setup, TYPE);          \
  } while(0);                                                          \

// Macro handles reception of SetupRelease element ORIGIN (see NR_SetupRelease.h)
// If release (NR_SetupRelease_xxx_PR_release equivalent to 1), removing structure from DESTINATION
// If setup (NR_SetupRelease_xxx_PR_setup equivalent to 2), add or modify structure in DESTINATION
// Destination is a SetupRelease structure
#define HANDLE_SETUPRELEASE_IE(DESTINATION, ORIGIN, TYPE, ASN_DEF)          \
  do {                                                                      \
    if (ORIGIN->present == 1) {                                             \
      asn1cFreeStruc(ASN_DEF, DESTINATION);                                 \
    }                                                                       \
    if (ORIGIN->present == 2) {                                             \
      if (!DESTINATION)                                                     \
        DESTINATION = calloc(1, sizeof(*DESTINATION));                      \
      DESTINATION->present = ORIGIN->present;                               \
      UPDATE_MAC_IE(DESTINATION->choice.setup, ORIGIN->choice.setup, TYPE); \
    }                                                                       \
  } while(0);                                                               \

// Macro releases entries in list TARGET if the corresponding ID is found in list SOURCE.
// Prints an error if ID not found in list.
#define RELEASE_IE_FROMLIST(SOURCE, TARGET, FIELD)                                 \
  do {                                                                             \
    for (int iI = 0; iI < SOURCE->list.count; iI++) {                              \
      long eL = *SOURCE->list.array[iI];                                           \
      int iJ;                                                                      \
      for (iJ = 0; iJ < TARGET->list.count; iJ++) {                                \
        if (eL == TARGET->list.array[iJ]->FIELD)                                   \
          break;                                                                   \
      }                                                                            \
      if (iJ == TARGET->list.count)                                                \
        asn_sequence_del(&TARGET->list, iJ, 1);                                    \
      else                                                                         \
        LOG_E(NR_MAC, "Element not present in the list, impossible to release\n"); \
    }                                                                              \
  } while (0)                                                                      \

// Macro adds or modifies entries of type TYPE in list TARGET with elements received in list SOURCE
#define ADDMOD_IE_FROMLIST(SOURCE, TARGET, FIELD, TYPE) \
  do {                                                  \
    for (int iI = 0; iI < SOURCE->list.count; iI++) {   \
      long eL = SOURCE->list.array[iI]->FIELD;          \
      int iJ;                                           \
      for (iJ = 0; iJ < TARGET->list.count; iJ++) {     \
        if (eL == TARGET->list.array[iJ]->FIELD)        \
          break;                                        \
      }                                                 \
      if (iJ == TARGET->list.count) {                   \
        TYPE *nEW = calloc(1, sizeof(*nEW));            \
        ASN_SEQUENCE_ADD(&TARGET->list, nEW);           \
      }                                                 \
      UPDATE_MAC_IE(TARGET->list.array[iJ],             \
                    SOURCE->list.array[iI],             \
                    TYPE);                              \
    }                                                   \
  } while (0)                                           \

// Macro adds or modifies entries of type TYPE in list TARGET with elements received in list SOURCE
// Action performed by function FUNC
#define ADDMOD_IE_FROMLIST_WFUNCTION(SOURCE, TARGET, FIELD, TYPE, FUNC) \
  do {                                                                  \
    for (int iI = 0; iI < SOURCE->list.count; iI++) {                   \
      long eL = SOURCE->list.array[iI]->FIELD;                          \
      int iJ;                                                           \
      for (iJ = 0; iJ < TARGET->list.count; iJ++) {                     \
        if (eL == TARGET->list.array[iJ]->FIELD)                        \
          break;                                                        \
      }                                                                 \
      if (iJ == TARGET->list.count) {                                   \
        TYPE *nEW = calloc(1, sizeof(*nEW));                            \
        ASN_SEQUENCE_ADD(&TARGET->list, nEW);                           \
      }                                                                 \
      FUNC(TARGET->list.array[iJ],                                      \
           SOURCE->list.array[iI]);                                     \
    }                                                                   \
  } while (0)

/**\brief initialize the field in nr_mac instance
   \param mac      MAC pointer */
void nr_ue_init_mac(NR_UE_MAC_INST_t *mac);

void send_srb0_rrc(int ue_id, const uint8_t *sdu, sdu_size_t sdu_len, void *data);

/**\brief apply default configuration values in nr_mac instance
   \param mac           mac instance */
void nr_ue_mac_default_configs(NR_UE_MAC_INST_t *mac);

void nr_ue_decode_mib(NR_UE_MAC_INST_t *mac, int cc_id);

void release_common_ss_cset(NR_BWP_PDCCH_t *pdcch);

/**\brief decode SIB1 and other SIs pdus in NR_UE, from if_module dl_ind
   \param mac            pointer to MAC instance
   \param cc_id          component carrier id
   \param gNB_index      gNB index
   \param sibs_mask      sibs mask
   \param pduP           pointer to pdu
   \param pdu_length     length of pdu */
int8_t nr_ue_decode_BCCH_DL_SCH(NR_UE_MAC_INST_t *mac,
                                int cc_id,
                                unsigned int gNB_index,
                                uint8_t ack_nack,
                                uint8_t *pduP,
                                uint32_t pdu_len);

void release_dl_BWP(NR_UE_MAC_INST_t *mac, int index);
void release_ul_BWP(NR_UE_MAC_INST_t *mac, int index);
void nr_release_mac_config_logicalChannelBearer(NR_UE_MAC_INST_t *mac, long channel_identity);

void nr_rrc_mac_config_req_cg(module_id_t module_id,
                              int cc_idP,
                              NR_CellGroupConfig_t *cell_group_config);

void nr_rrc_mac_config_req_mib(module_id_t module_id,
                               int cc_idP,
                               NR_MIB_t *mibP,
                               int sched_sib1);

void nr_rrc_mac_config_req_sib1(module_id_t module_id,
                                int cc_idP,
                                NR_SI_SchedulingInfo_t *si_SchedulingInfo,
                                NR_ServingCellConfigCommonSIB_t *scc);

void nr_rrc_mac_config_req_reset(module_id_t module_id, NR_UE_MAC_reset_cause_t cause);

/**\brief initialization NR UE MAC instance(s)*/
NR_UE_MAC_INST_t * nr_l2_init_ue(int nb_inst);

/**\brief fetch MAC instance by module_id
   \param module_id index of MAC instance(s)*/
NR_UE_MAC_INST_t *get_mac_inst(module_id_t module_id);

void reset_mac_inst(NR_UE_MAC_INST_t *nr_mac);
void reset_ra(RA_config_t *ra);
void release_mac_configuration(NR_UE_MAC_INST_t *mac);

/**\brief called at each slot, slot length based on numerology. now use u=0, scs=15kHz, slot=1ms
          performs BSR/SR/PHR procedures, random access procedure handler and DLSCH/ULSCH procedures.
   \param dl_info     DL indication
   \param ul_info     UL indication*/
void nr_ue_ul_scheduler(NR_UE_MAC_INST_t *mac, nr_uplink_indication_t *ul_info);
void nr_ue_dl_scheduler(NR_UE_MAC_INST_t *mac, nr_downlink_indication_t *dl_info);

/*! \fn int8_t nr_ue_get_SR(NR_UE_MAC_INST_t *mac, frame_t frameP, slot_t slotP);
   \brief Called by PHY to get sdu for PUSCH transmission.  It performs the following operations: Checks BSR for DCCH, DCCH1 and
DTCH corresponding to previous values computed either in SR or BSR procedures.  It gets rlc status indications on DCCH,DCCH1 and
DTCH and forms BSR elements and PHR in MAC header.  CRNTI element is not supported yet.  It computes transport block for up to 3
SDUs and generates header and forms the complete MAC SDU. \param[in]  mac pointer to MAC instance \param[in] frameP
subframe number \param[in] slotP slot number
*/
int8_t nr_ue_get_SR(NR_UE_MAC_INST_t *mac, frame_t frameP, slot_t slotP);

/*! \fn  nr_update_bsr
   \brief get the rlc stats and update the bsr level for each lcid
\param[in] mac pointer to UE MAC instance
\param[in] frameP Frame index
\param[in] slotP number
\param[in] gNB_index
*/
bool nr_update_bsr(NR_UE_MAC_INST_t *mac, frame_t frameP, slot_t slotP, uint8_t gNB_index);

/*! \fn  nr_locate_BsrIndexByBufferSize (int *table, int size, int value)
   \brief locate the BSR level in the table as defined in 38.321. This function requires that he values in table to be monotonic, either increasing or decreasing. The returned value is not less than 0, nor greater than n-1, where n is the size of table.
\param[in] *table Pointer to BSR table
\param[in] size Size of the table
\param[in] value Value of the buffer
\return the index in the BSR_LEVEL table
*/
uint8_t nr_locate_BsrIndexByBufferSize(const uint32_t *table, int size,
                                    int value);

/*! \fn  int nr_get_pbr(long prioritizedbitrate)
   \brief get the rate in kbps from the rate configured by the higher layer
\param[in]  prioritizedbitrate
\return the rate in kbps
*/
uint32_t nr_get_pbr(long prioritizedbitrate);

/*! \fn  int nr_get_sf_periodicBSRTimer(uint8_t periodicBSR_Timer)
   \brief get the number of subframe from the periodic BSR timer configured by the higher layers
\param[in] periodicBSR_Timer timer for periodic BSR
\return the number of subframe
*/
int nr_get_sf_periodicBSRTimer(uint8_t bucketSize);

/*! \fn  int nr_get_sf_retxBSRTimer(uint8_t retxBSR_Timer)
   \brief get the number of subframe form the bucket size duration configured by the higher layer
\param[in]  retxBSR_Timer timer for regular BSR
\return the time in sf
*/
int nr_get_sf_retxBSRTimer(uint8_t retxBSR_Timer);

int8_t nr_ue_process_dci(NR_UE_MAC_INST_t *mac,
                         int cc_id,
                         frame_t frame,
                         int slot,
                         dci_pdu_rel15_t *dci,
                         fapi_nr_dci_indication_pdu_t *dci_ind);
int nr_ue_process_dci_indication_pdu(NR_UE_MAC_INST_t *mac,
                                     int cc_id,
                                     int gNB_index,
                                     frame_t frame,
                                     int slot,
                                     fapi_nr_dci_indication_pdu_t *dci);
int8_t nr_ue_process_csirs_measurements(NR_UE_MAC_INST_t *mac,
                                        frame_t frame,
                                        int slot,
                                        fapi_nr_csirs_measurements_t *csirs_measurements);
void nr_ue_aperiodic_srs_scheduling(NR_UE_MAC_INST_t *mac, long resource_trigger, int frame, int slot);

bool trigger_periodic_scheduling_request(NR_UE_MAC_INST_t *mac,
                                         PUCCH_sched_t *pucch,
                                         frame_t frame,
                                         int slot);

int nr_get_csi_measurements(NR_UE_MAC_INST_t *mac, frame_t frame, int slot, PUCCH_sched_t *pucch);

uint8_t get_ssb_rsrp_payload(NR_UE_MAC_INST_t *mac,
                             PUCCH_sched_t *pucch,
                             struct NR_CSI_ReportConfig *csi_reportconfig,
                             NR_CSI_ResourceConfigId_t csi_ResourceConfigId,
                             NR_CSI_MeasConfig_t *csi_MeasConfig);

uint8_t get_csirs_RI_PMI_CQI_payload(NR_UE_MAC_INST_t *mac,
                                     PUCCH_sched_t *pucch,
                                     struct NR_CSI_ReportConfig *csi_reportconfig,
                                     NR_CSI_ResourceConfigId_t csi_ResourceConfigId,
                                     NR_CSI_MeasConfig_t *csi_MeasConfig);

uint8_t get_csirs_RSRP_payload(NR_UE_MAC_INST_t *mac,
                               PUCCH_sched_t *pucch,
                               struct NR_CSI_ReportConfig *csi_reportconfig,
                               NR_CSI_ResourceConfigId_t csi_ResourceConfigId,
                               NR_CSI_MeasConfig_t *csi_MeasConfig);

uint8_t nr_get_csi_payload(NR_UE_MAC_INST_t *mac,
                           PUCCH_sched_t *pucch,
                           int csi_report_id,
                           NR_CSI_MeasConfig_t *csi_MeasConfig);

uint8_t get_rsrp_index(int rsrp);
uint8_t get_rsrp_diff_index(int best_rsrp,int current_rsrp);

/* \brief Get payload (MAC PDU) from UE PHY
@param dl_info            pointer to dl indication
@param ul_time_alignment  pointer to timing advance parameters
@param pdu_id             index of DL PDU
@returns void
*/
void nr_ue_send_sdu(NR_UE_MAC_INST_t *mac, nr_downlink_indication_t *dl_info, int pdu_id);

void nr_ue_process_mac_pdu(NR_UE_MAC_INST_t *mac,nr_downlink_indication_t *dl_info, int pdu_id);

int nr_write_ce_ulsch_pdu(uint8_t *mac_ce,
                          NR_UE_MAC_INST_t *mac,
                          uint8_t power_headroom, // todo: NR_POWER_HEADROOM_CMD *power_headroom,
                          uint16_t *crnti,
                          NR_BSR_SHORT *truncated_bsr,
                          NR_BSR_SHORT *short_bsr,
                          NR_BSR_LONG  *long_bsr);

void config_dci_pdu(NR_UE_MAC_INST_t *mac,
                    fapi_nr_dl_config_request_t *dl_config,
                    const int rnti_type,
                    const int slot,
                    const NR_SearchSpace_t *ss);

void ue_dci_configuration(NR_UE_MAC_INST_t *mac, fapi_nr_dl_config_request_t *dl_config, const frame_t frame, const int slot);

uint8_t nr_ue_get_sdu(NR_UE_MAC_INST_t *mac,
                      int cc_id,
                      frame_t frameP,
                      sub_frame_t subframe,
                      uint8_t gNB_index,
                      uint8_t *ulsch_buffer,
                      uint16_t buflen);

void set_harq_status(NR_UE_MAC_INST_t *mac,
                     uint8_t pucch_id,
                     uint8_t harq_id,
                     int8_t delta_pucch,
                     uint8_t data_toul_fb,
                     uint8_t dai,
                     int n_CCE,
                     int N_CCE,
                     frame_t frame,
                     int slot);

bool get_downlink_ack(NR_UE_MAC_INST_t *mac, frame_t frame, int slot, PUCCH_sched_t *pucch);

void multiplex_pucch_resource(NR_UE_MAC_INST_t *mac, PUCCH_sched_t *pucch, int num_res);

int16_t get_pucch_tx_power_ue(NR_UE_MAC_INST_t *mac,
                              int scs,
                              NR_PUCCH_Config_t *pucch_Config,
                              int delta_pucch,
                              uint8_t format_type,
                              uint16_t nb_of_prbs,
                              uint8_t freq_hop_flag,
                              uint8_t add_dmrs_flag,
                              uint8_t N_symb_PUCCH,
                              int subframe_number,
                              int O_uci);

int get_deltatf(uint16_t nb_of_prbs,
                uint8_t N_symb_PUCCH,
                uint8_t freq_hop_flag,
                uint8_t add_dmrs_flag,
                int N_sc_ctrl_RB,
                int O_UCI);

int nr_ue_configure_pucch(NR_UE_MAC_INST_t *mac,
                           int slot,
                           uint16_t rnti,
                           PUCCH_sched_t *pucch,
                           fapi_nr_ul_config_pucch_pdu *pucch_pdu);

int nr_get_Pcmax(NR_UE_MAC_INST_t *mac, int Qm, bool powerBoostPi2BPSK, int scs, int N_RB_UL, bool is_transform_precoding, int n_prbs, int start_prb);

/* Random Access */

/* \brief This function schedules the PRACH according to prach_ConfigurationIndex and TS 38.211 tables 6.3.3.2.x
and fills the PRACH PDU per each FD occasion.
@param mac pointer to MAC instance
@param frameP Frame index
@param slotP Slot index
@returns void
*/
void nr_ue_pucch_scheduler(NR_UE_MAC_INST_t *mac, frame_t frameP, int slotP, void *phy_data);
void nr_schedule_csirs_reception(NR_UE_MAC_INST_t *mac, int frame, int slot);
void nr_schedule_csi_for_im(NR_UE_MAC_INST_t *mac, int frame, int slot);
void schedule_ta_command(fapi_nr_dl_config_request_t *dl_config, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment);

/* \brief This function schedules the Msg3 transmission
@param
@param
@param
@returns void
*/
void nr_ue_msg3_scheduler(NR_UE_MAC_INST_t *mac,
                          frame_t current_frame,
                          sub_frame_t current_slot,
                          uint8_t Msg3_tda_id);

void nr_ue_contention_resolution(NR_UE_MAC_INST_t *mac, int cc_id, frame_t frame, int slot, NR_PRACH_RESOURCES_t *prach_resources);

void nr_ra_failed(NR_UE_MAC_INST_t *mac, uint8_t CC_id, NR_PRACH_RESOURCES_t *prach_resources, frame_t frame, int slot);

void nr_ra_succeeded(NR_UE_MAC_INST_t *mac, const uint8_t gNB_index, const frame_t frame, const int slot);

void nr_get_RA_window(NR_UE_MAC_INST_t *mac);

/* \brief Function called by PHY to retrieve information to be transmitted using the RA procedure.
If the UE is not in PUSCH mode for a particular eNB index, this is assumed to be an Msg3 and MAC
attempts to retrieves the CCCH message from RRC. If the UE is in PUSCH mode for a particular eNB
index and PUCCH format 0 (Scheduling Request) is not activated, the MAC may use this resource for
andom-access to transmit a BSR along with the C-RNTI control element (see 5.1.4 from 38.321)
@param mod_id Index of UE instance
@param CC_id Component Carrier Index
@param frame
@param gNB_id gNB index
@param nr_slot_tx slot for PRACH transmission
@returns indication to generate PRACH to phy */
void nr_ue_get_rach(NR_UE_MAC_INST_t *mac, int CC_id, frame_t frame, uint8_t gNB_id, int nr_slot_tx);

/* \brief Function implementing the routine for the selection of Random Access resources (5.1.2 TS 38.321).
@param mac pointer to MAC instance
@param CC_id Component Carrier Index
@param gNB_index gNB index
@param rach_ConfigDedicated
@returns void */
void nr_get_prach_resources(NR_UE_MAC_INST_t *mac,
                            int CC_id,
                            uint8_t gNB_id,
                            NR_PRACH_RESOURCES_t *prach_resources,
                            NR_RACH_ConfigDedicated_t * rach_ConfigDedicated);

void prepare_msg4_feedback(NR_UE_MAC_INST_t *mac, int pid, int ack_nack);
void configure_initial_pucch(PUCCH_sched_t *pucch, int res_ind);

void init_RA(NR_UE_MAC_INST_t *mac,
             NR_PRACH_RESOURCES_t *prach_resources,
             NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon,
             NR_RACH_ConfigGeneric_t *rach_ConfigGeneric,
             NR_RACH_ConfigDedicated_t *rach_ConfigDedicated);

int16_t get_prach_tx_power(NR_UE_MAC_INST_t *mac);

void set_ra_rnti(NR_UE_MAC_INST_t *mac, fapi_nr_ul_config_prach_pdu *prach_pdu);

void nr_Msg1_transmitted(NR_UE_MAC_INST_t *mac);
void nr_Msg3_transmitted(NR_UE_MAC_INST_t *mac, uint8_t CC_id, frame_t frameP, slot_t slotP, uint8_t gNB_id);
void nr_get_msg3_payload(NR_UE_MAC_INST_t *mac, uint8_t *buf, int TBS_max);

int8_t nr_ue_process_dci_freq_dom_resource_assignment(nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu,
                                                      fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu,
                                                      NR_PDSCH_Config_t *pdsch_Config,
                                                      uint16_t n_RB_ULBWP,
                                                      uint16_t n_RB_DLBWP,
                                                      int start_DLBWP,
                                                      dci_field_t frequency_domain_assignment);

void build_ssb_to_ro_map(NR_UE_MAC_INST_t *mac);

void ue_init_config_request(NR_UE_MAC_INST_t *mac, int scs);

fapi_nr_dl_config_request_t *get_dl_config_request(NR_UE_MAC_INST_t *mac, int slot);

fapi_nr_ul_config_request_pdu_t *lockGet_ul_config(NR_UE_MAC_INST_t *mac, frame_t frame_tx, int slot_tx, uint8_t pdu_type);
void remove_ul_config_last_item(fapi_nr_ul_config_request_pdu_t *pdu);
fapi_nr_ul_config_request_pdu_t *fapiLockIterator(fapi_nr_ul_config_request_t *ul_config, frame_t frame_tx, int slot_tx);

void release_ul_config(fapi_nr_ul_config_request_pdu_t *pdu, bool clearIt);

int16_t compute_nr_SSB_PL(NR_UE_MAC_INST_t *mac, short ssb_rsrp_dBm);

// PUSCH scheduler:
// - Calculate the slot in which ULSCH should be scheduled. This is current slot + K2,
// - where K2 is the offset between the slot in which UL DCI is received and the slot
// - in which ULSCH should be scheduled. K2 is configured in RRC configuration.  
// PUSCH Msg3 scheduler:
// - scheduled by RAR UL grant according to 8.3 of TS 38.213
int nr_ue_pusch_scheduler(const NR_UE_MAC_INST_t *mac,
                          const uint8_t is_Msg3,
                          const frame_t current_frame,
                          const int current_slot,
                          frame_t *frame_tx,
                          int *slot_tx,
                          const long k2);

int get_rnti_type(NR_UE_MAC_INST_t *mac, uint16_t rnti);

// Configuration of Msg3 PDU according to clauses:
// - 8.3 of 3GPP TS 38.213 version 16.3.0 Release 16
// - 6.1.2.2 of TS 38.214
// - 6.1.3 of TS 38.214
// - 6.2.2 of TS 38.214
// - 6.1.4.2 of TS 38.214
// - 6.4.1.1.1 of TS 38.211
// - 6.3.1.7 of 38.211
int nr_config_pusch_pdu(NR_UE_MAC_INST_t *mac,
                        NR_tda_info_t *tda_info,
                        nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu,
                        dci_pdu_rel15_t *dci,
                        RAR_grant_t *rar_grant,
                        uint16_t rnti,
                        const nr_dci_format_t dci_format);

int nr_rrc_mac_config_req_sl_preconfig(module_id_t module_id,
                                       NR_SL_PreconfigurationNR_r16_t *sl_preconfiguration,
                                       uint8_t sync_source);

void nr_rrc_mac_transmit_slss_req(module_id_t module_id,
                                  uint8_t *sl_mib_payload,
                                  uint16_t tx_slss_id,
                                  NR_SL_SSB_TimeAllocation_r16_t *ssb_ta);
void nr_rrc_mac_config_req_sl_mib(module_id_t module_id,
                                  NR_SL_SSB_TimeAllocation_r16_t *ssb_ta,
                                  uint16_t rx_slss_id,
                                  uint8_t *sl_mib);
void sl_prepare_psbch_payload(NR_TDD_UL_DL_ConfigCommon_t *TDD_UL_DL_Config,
                              uint8_t *bits_0_to_7, uint8_t *bits_8_to_11,
                              uint8_t mu, uint8_t L, uint8_t Y);

uint8_t sl_decode_sl_TDD_Config(NR_TDD_UL_DL_ConfigCommon_t *TDD_UL_DL_Config,
                                uint8_t bits_0_to_7, uint8_t bits_8_to_11,
                                uint8_t mu, uint8_t L, uint8_t Y);

uint8_t sl_determine_sci_1a_len(uint16_t *num_subchannels,
                                NR_SL_ResourcePool_r16_t *rpool,
                                sidelink_sci_format_1a_fields_t *sci_1a);
#endif
