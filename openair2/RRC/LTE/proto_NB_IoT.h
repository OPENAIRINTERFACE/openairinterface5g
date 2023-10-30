/* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
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

/*! \file proto_NB_IoT.h
 * \brief RRC functions prototypes for eNB and UE for NB-IoT
 * \author Navid Nikaein, Raymond Knopp and Michele Paffetti
 * \date 2010 - 2014
 * \email navid.nikaein@eurecom.fr, michele.paffetti@studio.unibo.it
 * \version 1.0

 */
/** \addtogroup _rrc
 *  @{
 */
#ifndef __PROTO_NB_IOT_H
#define __PROTO_NB_IOT_H
#include "RRC/LTE/defs_NB_IoT.h"
#include "pdcp.h"
#include "rlc.h"
#include "extern_NB_IoT.h"
#include "LAYER2/MAC/defs_NB_IoT.h"
/*NOTE: no static function should be declared in this header file (e.g. init_SI_NB)*/

/*------------------------common_nb_iot.c----------------------------------------*/

/** \brief configure  BCCH & CCCH Logical Channels and associated rrc_buffers, configure associated SRBs
 */
void openair_rrc_on_NB_IoT(const protocol_ctxt_t* const ctxt_pP);

void rrc_config_buffer_NB_IoT(SRB_INFO_NB_IoT *srb_info, uint8_t Lchan_type, uint8_t Role);

int L3_xface_init_NB_IoT(void);

void openair_rrc_top_init_eNB_NB_IoT(void);

//void rrc_top_cleanup(void); -->seems not to be used

//rrc_t310_expiration-->seems not to be used

/** \brief Function to update timers every subframe.  For UE it updates T300,T304 and T310.
@param ctxt_pP  running context
@param enb_index
@param CC_id
*/
RRC_status_t rrc_rx_tx_NB_IoT(protocol_ctxt_t* const ctxt_pP, const uint8_t  enb_index, const int CC_id);

//long binary_search_int(int elements[], long numElem, int value);--> seems not to be used
//long binary_search_float(float elements[], long numElem, float value);--> used only at UE side



//---------------------------------------


//defined in L2_interface
//called by rx_sdu only in case of CCCH message (e.g RRCConnectionRequest-NB)
int8_t mac_rrc_data_ind_eNB_NB_IoT(
  const module_id_t     module_idP,
  const int             CC_id,
  const frame_t         frameP,
  const sub_frame_t     sub_frameP,
  const rnti_t          rntiP,
  const rb_id_t         srb_idP,//could be skipped since always go through the CCCH channel
  const uint8_t*        sduP,
  const sdu_size_t      sdu_lenP
);
//-------------------------------------------

//defined in L2_interface
void dump_ue_list_NB_IoT(UE_list_NB_IoT_t *listP, int ul_flag);
//-------------------------------------------


//defined in L2_interface
void mac_eNB_rrc_ul_failure_NB_IoT(
		const module_id_t mod_idP,
	    const int CC_idP,
	    const frame_t frameP,
	    const sub_frame_t subframeP,
	    const rnti_t rntiP);
//------------------------------------------

//defined in eNB_scheduler_primitives.c
int rrc_mac_remove_ue_NB_IoT(
		module_id_t mod_idP,
		rnti_t rntiP);
//------------------------------------------
//defined in L2_interface
void mac_eNB_rrc_ul_in_sync_NB_IoT(
				const module_id_t mod_idP,
			    const int CC_idP,
			    const frame_t frameP,
			    const sub_frame_t subframeP,
			    const rnti_t rntiP);
//------------------------------------------
//defined in L2_interface
int mac_eNB_get_rrc_status_NB_IoT(
  const module_id_t Mod_idP,
  const rnti_t      rntiP
);
//---------------------------


/*-----------eNB procedures (rrc_eNB_nb_iot.c)---------------*/

//---Initialization--------------
void openair_eNB_rrc_on_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP
);

void rrc_config_buffer_NB_IoT(
  SRB_INFO_NB_IoT* Srb_info,
  uint8_t Lchan_type,
  uint8_t Role
);

char openair_rrc_eNB_configuration_NB_IoT(
  const module_id_t enb_mod_idP,
  NbIoTRrcConfigurationReq* configuration
);

//-----------------------------
/**\brief RRC eNB task. (starting of the RRC state machine)
   \param args_p Pointer on arguments to start the task. */
void *rrc_enb_task_NB_IoT(void *args_p);

/**\brief Entry routine to decode a UL-CCCH-Message-NB.  Invokes PER decoder and parses message.
   \param ctxt_pP Running context
   \param Srb_info Pointer to SRB0 information structure (buffer, etc.)
\param CC_id*/
int rrc_eNB_decode_ccch_NB_IoT(
  protocol_ctxt_t* const ctxt_pP,
  const SRB_INFO_NB_IoT*        const Srb_info,
  const int              CC_id
);

/**\brief Entry routine to decode a UL-DCCH-Message-NB.  Invokes PER decoder and parses message.
   \param ctxt_pP Context
\param Srb_id
   \param Rx_sdu Pointer Received Message
   \param sdu_sizeP Size of incoming SDU*/
int rrc_eNB_decode_dcch_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  const rb_id_t                Srb_id,
  const uint8_t*    const      Rx_sdu,
  const sdu_size_t             sdu_sizeP
);

/**\brief Generate RRCConnectionReestablishmentReject-NB
   \param ctxt_pP       Running context
   \param ue_context_pP UE context
   \param CC_id         Component Carrier ID*/
void rrc_eNB_generate_RRCConnectionReestablishmentReject_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_NB_IoT_t*          const ue_context_pP,
  const int                    CC_id
);

void rrc_eNB_generate_RRCConnectionReject_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_NB_IoT_t*          const ue_context_pP,
  const int                    CC_id
);

void rrc_eNB_generate_RRCConnectionSetup_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_NB_IoT_t*          const ue_context_pP,
  const int                    CC_id
);

void rrc_eNB_process_RRCConnectionReconfigurationComplete_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_NB_IoT_t*        ue_context_pP,
  const uint8_t xid //transaction identifier
);


void //was under ITTI
rrc_eNB_reconfigure_DRBs_NB_IoT(const protocol_ctxt_t* const ctxt_pP,
			       rrc_eNB_ue_context_NB_IoT_t*  ue_context_pP);

void //was under ITTI
rrc_eNB_generate_dedicatedRRCConnectionReconfiguration_NB_IoT(
		const protocol_ctxt_t* const ctxt_pP,
	    rrc_eNB_ue_context_NB_IoT_t*          const ue_context_pP
//            const uint8_t      ho_state
	     );

void rrc_eNB_process_RRCConnectionSetupComplete_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_NB_IoT_t*         ue_context_pP,
  LTE_RRCConnectionSetupComplete_NB_r13_IEs_t * rrcConnectionSetupComplete_NB
);

void rrc_eNB_generate_SecurityModeCommand_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_NB_IoT_t*          const ue_context_pP
);

void rrc_eNB_generate_UECapabilityEnquiry_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_NB_IoT_t*          const ue_context_pP
);

void rrc_eNB_generate_defaultRRCConnectionReconfiguration_NB_IoT(const protocol_ctxt_t* const ctxt_pP,
						                                                     rrc_eNB_ue_context_NB_IoT_t*          const ue_context_pP
						                                                     //no HO flag
						                                                    );


/// Utilities------------------------------------------------

void rrc_eNB_free_UE_NB_IoT(
		const module_id_t enb_mod_idP,
		const struct rrc_eNB_ue_context_NB_IoT_s*        const ue_context_pP
		);

void rrc_eNB_free_mem_UE_context_NB_IoT(
  const protocol_ctxt_t*               const ctxt_pP,
  struct rrc_eNB_ue_context_NB_IoT_s*         const ue_context_pP
);



/**\brief Function to get the next transaction identifier.
   \param module_idP Instance ID for CH/eNB
   \return a transaction identifier*/
uint8_t rrc_eNB_get_next_transaction_identifier_NB_IoT(module_id_t module_idP);


int rrc_init_global_param_NB_IoT(void);

//L2_interface.c
int8_t mac_rrc_data_req_eNB_NB_IoT(
  const module_id_t Mod_idP,
  const int         CC_id,
  const frame_t     frameP,
  const frame_t   h_frameP,
  const sub_frame_t   subframeP, //need for the case in which both SIB1-NB_IoT and SIB23-NB_IoT will be scheduled in the same frame
  const rb_id_t     Srb_id,
  uint8_t* const buffer_pP,
  uint8_t   flag
);

/** @}*/
#endif
