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

/*! \file proto.h
 * \brief RRC functions prototypes for eNB and UE
 * \author Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email navid.nikaein@eurecom.fr
 * \version 1.0

 */
/** \addtogroup _rrc
 *  @{
 */

#include "RRC/LTE/rrc_defs.h"
#include "x2ap_messages_types.h"
#include "flexran_agent_extern.h"

//main.c
int rrc_init_global_param(void);
int L3_xface_init(void);
void openair_rrc_top_init(int eMBMS_active, char *uecap_xer, uint8_t cba_group_active,uint8_t HO_enabled);

char
openair_rrc_eNB_configuration(
  const module_id_t enb_mod_idP,
  RrcConfigurationReq *configuration
);

char openair_rrc_eNB_init(
  const module_id_t module_idP);

char openair_rrc_ue_init(
  const module_id_t module_idP,
  const uint8_t CH_IDX);
void rrc_config_buffer(SRB_INFO *srb_info, uint8_t Lchan_type, uint8_t Role);
void
openair_rrc_on(
  const protocol_ctxt_t *const ctxt_pP);
void
openair_rrc_on_ue(
  const protocol_ctxt_t *const ctxt_pP);

void rrc_top_cleanup(void);

/** \brief Function to update eNB timers every subframe.  
@param ctxt_pP  running context
@param enb_index
@param CC_id
*/
RRC_status_t
rrc_rx_tx(
  protocol_ctxt_t *const ctxt_pP,
  const int          CC_id
);

/** \brief Function to update timers every subframe.  For UE it updates T300,T304 and T310.
@param ctxt_pP  running context
@param enb_index
@param CC_id
*/
RRC_status_t
rrc_rx_tx_ue(
  protocol_ctxt_t *const ctxt_pP,
  const uint8_t      enb_index,
  const int          CC_id
);

// UE RRC Procedures

/** \brief Decodes DL-CCCH message and invokes appropriate routine to handle the message
    \param ctxt_pP Running context
    \param Srb_info Pointer to SRB_INFO structure (SRB0)
    \param eNB_index Index of corresponding eNB/CH*/
int rrc_ue_decode_ccch( const protocol_ctxt_t *const ctxt_pP, const SRB_INFO *const Srb_info, const uint8_t eNB_index );

/** \brief Decodes a DL-DCCH message and invokes appropriate routine to handle the message
    \param ctxt_pP Running context
    \param Srb_id Index of Srb (1,2)
    \param buffer_pP Pointer to received SDU
    \param eNB_index Index of corresponding eNB/CH*/
void
rrc_ue_decode_dcch(
  const protocol_ctxt_t *const ctxt_pP,
  const rb_id_t                Srb_id,
  const uint8_t         *const Buffer,
  const uint8_t                eNB_indexP
);

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
int decode_SL_Discovery_Message(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t                eNB_index,
  const uint8_t               *Sdu,
  const uint8_t                Sdu_len);
#endif

/** \brief Generate/Encodes RRCConnnectionRequest message at UE
    \param ctxt_pP Running context
    \param eNB_index Index of corresponding eNB/CH*/
void
rrc_ue_generate_RRCConnectionRequest(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t                eNB_index
);

/** \brief process the received rrcConnectionReconfiguration message at UE
    \param ctxt_pP Running context
    \param *rrcConnectionReconfiguration pointer to the sturcture
    \param eNB_index Index of corresponding eNB/CH*/
void
rrc_ue_process_rrcConnectionReconfiguration(
  const protocol_ctxt_t *const       ctxt_pP,
  LTE_RRCConnectionReconfiguration_t *rrcConnectionReconfiguration,
  uint8_t eNB_index
);

/** \brief Establish SRB1 based on configuration in SRB_ToAddMod structure.  Configures RLC/PDCP accordingly
    \param module_idP Instance ID of UE
    \param frame Frame index
    \param eNB_index Index of corresponding eNB/CH
    \param SRB_config Pointer to SRB_ToAddMod IE from configuration
    @returns 0 on success*/
int32_t  rrc_ue_establish_srb1(module_id_t module_idP,frame_t frameP,uint8_t eNB_index,struct LTE_SRB_ToAddMod *SRB_config);

/** \brief Establish SRB2 based on configuration in SRB_ToAddMod structure.  Configures RLC/PDCP accordingly
    \param module_idP Instance ID of UE
    \param frame Frame index
    \param eNB_index Index of corresponding eNB/CH
    \param SRB_config Pointer to SRB_ToAddMod IE from configuration
    @returns 0 on success*/
int32_t  rrc_ue_establish_srb2(module_id_t module_idP,frame_t frameP, uint8_t eNB_index,struct LTE_SRB_ToAddMod *SRB_config);

/** \brief Establish a DRB according to DRB_ToAddMod structure
    \param module_idP Instance ID of UE
    \param eNB_index Index of corresponding CH/eNB
    \param DRB_config Pointer to DRB_ToAddMod IE from configuration
    @returns 0 on success */
int32_t  rrc_ue_establish_drb(module_id_t module_idP,frame_t frameP,uint8_t eNB_index,struct LTE_DRB_ToAddMod *DRB_config);

/** \brief Process MobilityControlInfo Message to proceed with handover and configure PHY/MAC
    \param ctxt_pP Running context
    \param eNB_index Index of corresponding CH/eNB
    \param mobilityControlInfo Pointer to mobilityControlInfo
*/
void
rrc_ue_process_mobilityControlInfo(
  const protocol_ctxt_t *const       ctxt_pP,
  const uint8_t                      eNB_index,
  struct LTE_MobilityControlInfo *const mobilityControlInfo
);

/** \brief Process a measConfig Message and configure PHY/MAC
    \param  ctxt_pP    Running context
    \param eNB_index Index of corresponding CH/eNB
    \param  measConfig Pointer to MeasConfig  IE from configuration*/
void
rrc_ue_process_measConfig(
  const protocol_ctxt_t *const       ctxt_pP,
  const uint8_t                      eNB_index,
  LTE_MeasConfig_t *const               measConfig
);

/** \brief Process a RadioResourceConfigDedicated Message and configure PHY/MAC
    \param ctxt_pP Running context
    \param eNB_index Index of corresponding CH/eNB
    \param radioResourceConfigDedicated Pointer to RadioResourceConfigDedicated IE from configuration*/
void rrc_ue_process_radioResourceConfigDedicated(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t eNB_index,
  LTE_RadioResourceConfigDedicated_t *radioResourceConfigDedicated);


/** \brief Process a RadioResourceConfig and configure PHY/MAC for SL communication/discovery
    \param Mod_idP
    \param eNB_index Index of corresponding CH/eNB
    \param sib18 Pointer to SIB18 from SI message
    \param sib19 Pointer to SIB19 from SI message
    \param sl_CommConfig Pointer to SL_CommConfig RRCConnectionConfiguration
    \param sl_DiscConfig Pointer to SL_DiscConfig RRCConnectionConfiguration */
void rrc_ue_process_sidelink_radioResourceConfig(
  module_id_t Mod_idP,
  uint8_t eNB_index,
  LTE_SystemInformationBlockType18_r12_t     *sib18,
  LTE_SystemInformationBlockType19_r12_t     *sib19,
  LTE_SL_CommConfig_r12_t *sl_CommConfig,
  LTE_SL_DiscConfig_r12_t *sl_DiscConfig);

/** \brief Init control socket to listen to incoming packets from ProSe App
 *
 */
void rrc_control_socket_init(void);


// eNB/CH RRC Procedures

/**\brief Function to get the next transaction identifier.
   \param module_idP Instance ID for CH/eNB
   \return a transaction identifier*/
uint8_t rrc_eNB_get_next_transaction_identifier(module_id_t module_idP);

/**\brief Entry routine to decode a UL-CCCH-Message.  Invokes PER decoder and parses message.
   \param ctxt_pP Running context
   \param buffer Pointer to SDU
   \param buffer_length length of SDU in bytes
   \param CC_id component carrier index*/
int rrc_eNB_decode_ccch(protocol_ctxt_t* const ctxt_pP,
                        const uint8_t         *buffer,
                        int                    buffer_length,
                        const int              CC_id);

/**\brief Entry routine to decode a UL-DCCH-Message.  Invokes PER decoder and parses message.
   \param ctxt_pP Context
   \param Rx_sdu Pointer Received Message
   \param sdu_size Size of incoming SDU*/
int
rrc_eNB_decode_dcch(
  const protocol_ctxt_t *const ctxt_pP,
  const rb_id_t                Srb_id,
  const uint8_t    *const      Rx_sdu,
  const sdu_size_t             sdu_sizeP
);

/**\brief Generate the RRCConnectionSetup based on information coming from RRM
   \param ctxt_pP       Running context
   \param ue_context_pP UE context*/
void
rrc_eNB_generate_RRCConnectionSetup(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t *const ue_context_pP,
  const int                    CC_id
);

/**\brief Generate RRCConnectionReestablishmentReject
   \param ctxt_pP       Running context
   \param ue_context_pP UE context
   \param CC_id         Component Carrier ID*/
void
rrc_eNB_generate_RRCConnectionReestablishmentReject(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t *const ue_context_pP,
  const int                    CC_id
);

/**\brief Process the RRCConnectionSetupComplete based on information coming from UE
   \param ctxt_pP       Running context
   \param ue_context_pP RRC UE context
   \param rrcConnectionSetupComplete Pointer to RRCConnectionSetupComplete message*/
void
rrc_eNB_process_RRCConnectionSetupComplete(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t        *ue_context_pP,
  LTE_RRCConnectionSetupComplete_r8_IEs_t *rrcConnectionSetupComplete
);

/**\brief Process the RRCConnectionReconfigurationComplete based on information coming from UE
   \param ctxt_pP       Running context
   \param ue_context_pP RRC UE context
   \param rrcConnectionReconfigurationComplete Pointer to RRCConnectionReconfigurationComplete message
   \param xid         the transaction id for the rrcconnectionreconfiguration procedure
*/
void
rrc_eNB_process_RRCConnectionReconfigurationComplete(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t        *ue_context_pP,
  const uint8_t xid
);

/**\brief Generate the RRCConnectionRelease
   \param ctxt_pP Running context
   \param ue_context_pP UE context of UE receiving the message*/
void
rrc_eNB_generate_RRCConnectionRelease(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP
);

void
rrc_eNB_generate_defaultRRCConnectionReconfiguration(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP,
  const uint8_t                ho_state
);


void
flexran_rrc_eNB_generate_defaultRRCConnectionReconfiguration(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t *const ue_context_pP,
  const uint8_t ho_state
);
void
rrc_eNB_generate_HO_RRCConnectionReconfiguration(const protocol_ctxt_t *const ctxt_pP,
    rrc_eNB_ue_context_t  *const ue_context_pP,
    uint8_t               *buffer,
                                                 int                    *_size
                                                 //const uint8_t        ho_state
                                                 );
void
rrc_eNB_configure_rbs_handover(struct rrc_eNB_ue_context_s *ue_context_p, protocol_ctxt_t *const ctxt_pP);

int freq_to_arfcn10(int band, unsigned long freq);

void
rrc_eNB_generate_dedeicatedRRCConnectionReconfiguration(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP,
  const uint8_t                ho_state
);

/**\brief release Data Radio Bearer between ENB and UE
   \param ctxt_pP Running context
   \param ue_context_pP UE context of UE receiving the message*/
void
rrc_eNB_generate_dedicatedRRCConnectionReconfiguration_release(
  const protocol_ctxt_t   *const ctxt_pP,
  rrc_eNB_ue_context_t    *const ue_context_pP,
  uint8_t                  xid,
  uint32_t                 nas_length,
  uint8_t                 *nas_buffer
);

void 
rrc_eNB_reconfigure_DRBs (const protocol_ctxt_t *const ctxt_pP,
                          rrc_eNB_ue_context_t  *ue_context_pP);



void  rrc_enb_init(void);
void *rrc_enb_process_itti_msg(void *);

/**\brief RRC eNB task.
   \param void *args_p Pointer on arguments to start the task. */
void *rrc_enb_task(void *args_p);

/**\brief RRC UE task.
   \param void *args_p Pointer on arguments to start the task. */
void *rrc_ue_task(void *args_p);

void rrc_eNB_process_x2_setup_request(int mod_id, x2ap_setup_req_t *m);

void rrc_eNB_process_x2_setup_response(int mod_id, x2ap_setup_resp_t *m);

void rrc_eNB_process_handoverPreparationInformation(int mod_id, x2ap_handover_req_t *m);

/**\brief Generate/decode the handover RRCConnectionReconfiguration at eNB
   \param module_idP Instance ID for eNB/CH
   \param frame Frame index
   \param ue_module_idP Index of UE transmitting the messages*/
void
rrc_eNB_generate_RRCConnectionReconfiguration_handover(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t           *const ue_context_pP,
  uint8_t                *const nas_pdu,
  const uint32_t                nas_length
);

/**\brief Generate/decode the RRCConnectionReconfiguration for Sidelink at eNB
   \param ctxt_pP       Running context
   \param ue_context_pP RRC UE context
   \param destinationInfoList List of the destinations
   \param n_discoveryMessages Number of discovery messages*/
int
rrc_eNB_generate_RRCConnectionReconfiguration_Sidelink(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t           *const ue_context_pP,
      LTE_SL_DestinationInfoList_r12_t  *destinationInfoList,
      int n_discoveryMessages
);

/** \brief process the received SidelinkUEInformation message at eNB
    \param ctxt_pP Running context
    \param sidelinkUEInformation sidelinkUEInformation message from UE*/
uint8_t
rrc_eNB_process_SidelinkUEInformation(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t         *ue_context_pP,
  LTE_SidelinkUEInformation_r12_t  *sidelinkUEInformation
);

/** \brief Get a Resource Pool to transmit SL communication
    \param ctxt_pP Running context
    \param ue_context_pP UE context
    \param destinationInfoList Pointer to the list of SL destinations*/
LTE_SL_CommConfig_r12_t rrc_eNB_get_sidelink_commTXPool(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t *const ue_context_pP,
      LTE_SL_DestinationInfoList_r12_t  *destinationInfoList
);

/** \brief Get a Resource Pool for Discovery
    \param ctxt_pP Running context
    \param ue_context_pP UE context
    \param n_discoveryMessages Number of discovery messages*/
LTE_SL_DiscConfig_r12_t rrc_eNB_get_sidelink_discTXPool(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t *const ue_context_pP,
      int n_discoveryMessages
);

/** \brief Process request from control socket
 *  \param arg
 */
void *rrc_control_socket_thread_fct(void *arg);

//L2_interface.c
int8_t
mac_rrc_data_req(
  const module_id_t Mod_idP,
  const int         CC_id,
  const frame_t     frameP,
  const rb_id_t     Srb_id,
  const rnti_t      rnti,
  const uint8_t     Nb_tb,
  uint8_t    *const buffer_pP,
  const uint8_t     mbsfn_sync_area
);

int8_t
mac_rrc_data_ind(
  const module_id_t     module_idP,
  const int             CC_id,
  const frame_t         frameP,
  const sub_frame_t     sub_frameP,
  const int             UE_id,
  const rnti_t          rntiP,
  const rb_id_t         srb_idP,
  const uint8_t        *sduP,
  const sdu_size_t      sdu_lenP,
  const uint8_t         mbsfn_sync_areaP
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  , const boolean_t		brOption
#endif
);

int8_t
mac_rrc_data_req_ue(
  const module_id_t Mod_idP,
  const int         CC_id,
  const frame_t     frameP,
  const rb_id_t     Srb_id,
  const uint8_t     Nb_tb,
  uint8_t    *const buffer_pP,
  const mac_enb_index_t eNB_indexP,
  const uint8_t     mbsfn_sync_area
);

int8_t
mac_rrc_data_ind_ue(
  const module_id_t     module_idP,
  const int         CC_id,
  const frame_t         frameP,
  const sub_frame_t     sub_frameP,
  const rnti_t          rntiP,
  const rb_id_t         srb_idP,
  const uint8_t        *sduP,
  const sdu_size_t      sdu_lenP,
  const mac_enb_index_t eNB_indexP,
  const uint8_t         mbsfn_sync_areaP
);

void mac_sync_ind( module_id_t Mod_instP, uint8_t status);

void mac_eNB_rrc_ul_failure(const module_id_t Mod_instP,
			    const int CC_id,
			    const frame_t frameP,
			    const sub_frame_t subframeP,
			    const rnti_t rnti);

void mac_eNB_rrc_uplane_failure(const module_id_t Mod_instP,
                const int CC_id,
                const frame_t frameP,
                const sub_frame_t subframeP,
                const rnti_t rnti);

void mac_eNB_rrc_ul_in_sync(const module_id_t Mod_instP, 
			    const int CC_id, 
			    const frame_t frameP,
			    const sub_frame_t subframeP,
			    const rnti_t rnti);

uint8_t
rrc_data_req(
  const protocol_ctxt_t   *const ctxt_pP,
  const rb_id_t                  rb_idP,
  const mui_t                    muiP,
  const confirm_t                confirmP,
  const sdu_size_t               sdu_size,
  uint8_t                 *const buffer_pP,
  const pdcp_transmission_mode_t modeP
);

uint8_t

rrc_data_req_ue(
  const protocol_ctxt_t   *const ctxt_pP,
  const rb_id_t                  rb_idP,
  const mui_t                    muiP,
  const confirm_t                confirmP,
  const sdu_size_t               sdu_sizeP,
  uint8_t                 *const buffer_pP,
  const pdcp_transmission_mode_t modeP
);


void
rrc_data_ind(
  const protocol_ctxt_t *const ctxt_pP,
  const rb_id_t                Srb_id,
  const sdu_size_t             sdu_sizeP,
  const uint8_t   *const       buffer_pP
);

void rrc_in_sync_ind(module_id_t module_idP, frame_t frameP, uint16_t eNB_index);

void rrc_out_of_sync_ind(module_id_t module_idP, frame_t frameP, unsigned short eNB_index);

int decode_MCCH_Message( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index, const uint8_t *const Sdu, const uint8_t Sdu_len, const uint8_t mbsfn_sync_area );

int decode_BCCH_DLSCH_Message(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t                eNB_index,
  uint8_t               *const Sdu,
  const uint8_t                Sdu_len,
  const uint8_t                rsrq,
  const uint8_t                rsrp );

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
int decode_BCCH_MBMS_DLSCH_Message(
  const protocol_ctxt_t* const ctxt_pP,
  const uint8_t                eNB_index,
  uint8_t*               const Sdu,
  const uint8_t                Sdu_len,
  const uint8_t                rsrq,
  const uint8_t                rsrp );
#endif

int decode_PCCH_DLSCH_Message(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t                eNB_index,
  uint8_t               *const Sdu,
  const uint8_t                Sdu_len);

void
ue_meas_filtering(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t                eNB_index
);

void
ue_measurement_report_triggering(
  protocol_ctxt_t        *const ctxt_pP,
  const uint8_t                 eNB_index
);

int
mac_eNB_get_rrc_status(
  const module_id_t Mod_idP,
  const rnti_t      rntiP
);

int
mac_UE_get_rrc_status(
  const module_id_t Mod_idP,
  const uint8_t     indexP
);

void
rrc_eNB_generate_UECapabilityEnquiry(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP
);

void
rrc_eNB_generate_SecurityModeCommand(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP
);

void
rrc_eNB_process_MeasurementReport(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t        *ue_context_pP,
  const LTE_MeasResults_t   *const measResults2
);

void
rrc_eNB_generate_HandoverPreparationInformation(
  //const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP,
  uint8_t                     *buffer,
  int                          *_size
  //LTE_PhysCellId_t targetPhyId
);

int
flexran_rrc_eNB_trigger_handover (int mod_id,
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t  *ue_context_pP,
  int target_cell_id);

void
check_handovers(
  protocol_ctxt_t *const ctxt_pP
);

//void rrc_ue_process_ueCapabilityEnquiry(uint8_t module_idP,uint32_t frame,UECapabilityEnquiry_t *UECapabilityEnquiry,uint8_t eNB_index);
/*void
rrc_ue_process_securityModeCommand(
                const protocol_ctxt_t* const ctxt_pP,
                SecurityModeCommand_t *const securityModeCommand,
                const uint8_t                eNB_index
                );
*/

void rrc_eNB_emulation_notify_ue_module_id(
  const module_id_t ue_module_idP,
  const rnti_t      rntiP,
  const uint8_t     cell_identity_byte0P,
  const uint8_t     cell_identity_byte1P,
  const uint8_t     cell_identity_byte2P,
  const uint8_t     cell_identity_byte3P);



void
rrc_eNB_free_mem_UE_context(
  const protocol_ctxt_t               *const ctxt_pP,
  struct rrc_eNB_ue_context_s         *const ue_context_pP
);


void
rrc_eNB_free_UE(
		const module_id_t enb_mod_idP,
  const struct rrc_eNB_ue_context_s         *const ue_context_pP
);

long binary_search_int(int elements[], long numElem, int value);

long binary_search_float(float elements[], long numElem, float value);

void openair_rrc_top_init_eNB(int eMBMS_active,uint8_t HO_active);

void openair_rrc_top_init_ue(
                        int eMBMS_active,
  char *uecap_xer,
                        uint8_t cba_group_active,
                        uint8_t HO_active
);

extern pthread_mutex_t      rrc_release_freelist;
extern RRC_release_list_t   rrc_release_info;
extern pthread_mutex_t      lock_ue_freelist;

void remove_UE_from_freelist(module_id_t mod_id, rnti_t rnti);
void put_UE_in_freelist(module_id_t mod_id, rnti_t rnti, boolean_t removeFlag);
void release_UE_in_freeList(module_id_t mod_id);

/** @}*/
