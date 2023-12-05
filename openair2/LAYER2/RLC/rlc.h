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

/*! \file rlc.h
* \brief This file, and only this file must be included by external code that interact with RLC layer.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_impl_ RLC
* @ingroup _oai2
* @{
*/
#ifndef __RLC_H__
#    define __RLC_H__

#include "common/platform_types.h"
#include "common/platform_constants.h"
#    include "hashtable.h"
#    include "LTE_asn_constant.h"
#    include "common/utils/LOG/log.h"
#include "intertask_interface.h"
//#    include "PHY/defs.h"
#    include "LTE_RLC-Config.h"
#    include "LTE_DRB-ToAddMod.h"
#    include "LTE_DRB-ToAddModList.h"
#    include "LTE_SRB-ToAddMod.h"
#    include "LTE_SRB-ToAddModList.h"
#    include "LTE_DRB-ToReleaseList.h"
#    include "LTE_PMCH-InfoList-r9.h"


//-----------------------------------------------------------------------------
#define  RLC_OP_STATUS_OK                1
#define  RLC_OP_STATUS_BAD_PARAMETER     22
#define  RLC_OP_STATUS_INTERNAL_ERROR    2
#define  RLC_OP_STATUS_OUT_OF_RESSOURCES 3

#define  RLC_MUI_UNDEFINED     (mui_t)0

#define  RLC_RB_UNALLOCATED    (rb_id_t)0
#define  RLC_LC_UNALLOCATED    (logical_chan_id_t)0

//-----------------------------------------------------------------------------
//   PUBLIC RLC CONSTANTS
//-----------------------------------------------------------------------------

typedef enum rlc_confirm_e {
  RLC_SDU_CONFIRM_NO    = 0,
  RLC_SDU_CONFIRM_YES   = 1,
} rlc_confirm_t;

//-----------------------------------------------------------------------------
//   PRIVATE INTERNALS OF RLC
//-----------------------------------------------------------------------------

#define  RLC_MAX_MBMS_LC (LTE_maxSessionPerPMCH * LTE_maxServiceCount)
#define  RLC_MAX_LC  ((max_val_LTE_DRB_Identity+1)* MAX_MOBILES_PER_ENB)

extern void (*rlc_rrc_data_ind)(
  const protocol_ctxt_t *const ctxtP,
  const rb_id_t     rb_idP,
  const sdu_size_t  sdu_sizeP,
  const uint8_t    *const sduP)  __attribute__ ((aligned(32)));

extern void (*rlc_rrc_data_conf)(
  const protocol_ctxt_t *const ctxtP,
  const rb_id_t         rb_idP,
  const mui_t           muiP,
  const rlc_tx_status_t statusP) __attribute__ ((aligned(32)));

typedef void (rrc_data_ind_cb_t)(
  const protocol_ctxt_t *const ctxtP,
  const rb_id_t     rb_idP,
  const sdu_size_t  sdu_sizeP,
  const uint8_t    *const sduP);

typedef void (rrc_data_conf_cb_t)(
  const protocol_ctxt_t *const ctxtP,
  const rb_id_t         rb_idP,
  const mui_t           muiP,
  const rlc_tx_status_t statusP);


//-----------------------------------------------------------------------------
//   PUBLIC INTERFACE WITH RRC
//-----------------------------------------------------------------------------

/*! \fn rlc_op_status_t rrc_rlc_config_asn1_req (const protocol_ctxt_t* const ctxtP, const srb_flag_t srb_flagP, const SRB_ToAddMod_t* const srb2addmod, const DRB_ToAddModList_t* const drb2add_listP, const DRB_ToReleaseList_t*  const drb2release_listP, const PMCH_InfoList_r9_t * const pmch_info_listP)
* \brief  Function for RRC to configure a Radio Bearer.
* \param[in]  ctxtP              Running context.
* \param[in]  srb2add_listP      SRB configuration list to be created.
* \param[in]  drb2add_listP      DRB configuration list to be created.
* \param[in]  drb2release_listP  DRB configuration list to be released.
* \param[in]  pmch_info_listP    eMBMS pmch info list to be created.
* \return     A status about the processing, OK or error code.
*/
rlc_op_status_t rrc_rlc_config_asn1_req (
  const protocol_ctxt_t *const,
  const LTE_SRB_ToAddModList_t *const,
  const LTE_DRB_ToAddModList_t *const,
  const LTE_DRB_ToReleaseList_t *const,
  const LTE_PMCH_InfoList_r9_t *const pmch_info_listP,
  const uint32_t,
  const uint32_t );

/*! \fn rrc_rlc_remove_ue
 * \brief  Remove all RLC protocol instances from all radio bearers allocated to a UE.
 * \param[in]  ctxtP              Running context.
 * \return     A status about the processing, OK or error code.
 */
rlc_op_status_t rrc_rlc_remove_ue(const protocol_ctxt_t *const ctxtP);

/*! \fn rlc_op_status_t rrc_rlc_config_req (
     const protocol_ctxt_t* const ctxtP,
     const srb_flag_t   srb_flagP,
     const MBMS_flag_t  MBMS_flagP,
     config_action_t actionP,
     const  rb_id_t rb_idP,
     rlc_info_t rlc_infoP)
* \brief  Function for RRC to configure a Radio Bearer.
* \param[in]  ctxtP            Running context.
* \param[in]  srb_flagP        Flag to indicate SRB (1) or DRB (0)
* \param[in]  MBMS_flag        Flag to indicate whether this is an MBMS service (1) or not (0)
* \param[in]  actionP          Action for this radio bearer (add, modify, remove).
* \param[in]  rb_idP           Radio bearer identifier.
* \return     A status about the processing, OK or error code.
*/
rlc_op_status_t rrc_rlc_config_req(const protocol_ctxt_t *const ctxtP,
                                   const srb_flag_t srb_flagP,
                                   const MBMS_flag_t MBMS_flag,
                                   config_action_t actionP,
                                   const rb_id_t rb_idP);

/*! \fn rrc_rlc_data_req
 * \brief  Function for RRC to send a SDU through a Signalling Radio Bearer.
 * \param[in]  ctxtP            Running context.
 * \param[in]  MBMS_flag        Flag to indicate whether this is an MBMS service (1) or not (0)
 * \param[in]  rb_idP           Radio bearer identifier.
 * \param[in]  muiP             Message Unit identifier.
 * \param[in]  confirmP         Boolean, is confirmation requested.
 * \param[in]  sdu_sizeP        Size of SDU in bytes.
 * \param[in]  sduP             SDU.
 * \return     A status about the processing, OK or error code.
 */
rlc_op_status_t rrc_rlc_data_req(const protocol_ctxt_t *const ctxtP,
                                 const MBMS_flag_t MBMS_flag,
                                 const rb_id_t rb_idP,
                                 mui_t muiP,
                                 confirm_t confirmP,
                                 sdu_size_t sdu_sizeP,
                                 char *sduP);

//-----------------------------------------------------------------------------
//   PUBLIC INTERFACE WITH MAC
//-----------------------------------------------------------------------------
/*! \fn tbs_size_t mac_rlc_data_req     (const module_id_t mod_idP, const rnti_t rntiP, const frame_t frameP, const  MBMS_flag_t MBMS_flagP, logical_chan_id_t rb_idP, char* bufferP)
* \brief    Interface with MAC layer, map data request to the RLC corresponding to the radio bearer.
* \param [in]     mod_idP          Virtualized module identifier.
* \param [in]     rntiP            UE identifier.
* \param [in]     frameP            Frame index
* \param [in]     eNB_flagP        Flag to indicate eNB (1) or UE (0)
* \param [in]     MBMS_flagP       Flag to indicate whether this is the MBMS service (1) or not (0)
* \param [in]     rb_idP           Radio bearer identifier.
* \param [in]     tb_sizeP         Requested Tx TBS in bytes.
* \param [in,out] bufferP          Memory area to fill with the bytes requested by MAC.
* \return     A status about the processing, OK or error code.
*/
tbs_size_t            mac_rlc_data_req     (const module_id_t, const rnti_t, const eNB_index_t, const frame_t, const  eNB_flag_t, const  MBMS_flag_t, logical_chan_id_t, const tb_size_t,char *
    ,const uint32_t sourceL2Id
    ,const uint32_t destinationL2Id
                                           );

/*! \fn void mac_rlc_data_ind     (const module_id_t mod_idP, const rnti_t rntiP, const frame_t frameP, const  eNB_flag_t eNB_flagP, const  MBMS_flag_t MBMS_flagP, logical_chan_id_t rb_idP, uint32_t frameP, char* bufferP, tb_size_t tb_sizeP, num_tb_t num_tbP, crc_t *crcs)
* \brief    Interface with MAC layer, deserialize the transport blocks sent by MAC, then map data indication to the RLC instance corresponding to the radio bearer identifier.
* \param[in]  mod_idP          Virtualized module identifier.
* \param[in]  rntiP            UE identifier.
* \param[in]  frameP            Frame index
* \param[in]  eNB_flagP        Flag to indicate eNB (1) or UE (0)
* \param[in]  MBMS_flagP       Flag to indicate whether this is the MBMS service (1) or not (0)
* \param[in]  rb_idP           Radio bearer identifier.
* \param[in]  bufferP          Memory area containing the transport blocks sent by MAC.
* \param[in]  tb_sizeP         Size of a transport block in bits.
* \param[in]  num_tbP          Number of transport blocks.
* \param[in]  crcs             Array of CRC decoding.
*/
void                  mac_rlc_data_ind     (const module_id_t, const rnti_t, const eNB_index_t,const frame_t, const  eNB_flag_t, const  MBMS_flag_t, logical_chan_id_t, char *, tb_size_t, num_tb_t,
    crc_t * );

/*! \fn mac_rlc_status_resp_t mac_rlc_status_ind     (const module_id_t mod_idP, const rnti_t rntiP, const frame_t frameP, const sub_frame_t subframeP, const  eNB_flag_t eNB_flagP, const  MBMS_flag_t MBMS_flagP, logical_chan_id_t rb_idP)
* \brief    Interface with MAC layer, request and set the number of bytes scheduled for transmission by the RLC instance corresponding to the radio bearer identifier.
* \param[in]  mod_idP          Virtualized module identifier.
* \param[in]  rntiP            UE identifier.
* \param[in]  frameP            Frame index.
* \param[in]  subframeP         SubFrame index.
* \param[in]  eNB_flagP         Flag to indicate eNB operation (1 true, 0 false)
* \param[in]  MBMS_flagP       Flag to indicate whether this is the MBMS service (1) or not (0)
* \param[in]  rb_idP           Radio bearer identifier.
* \return     The maximum number of bytes that the RLC instance can send in the next transmission sequence.
*/
mac_rlc_status_resp_t mac_rlc_status_ind   (const module_id_t, const rnti_t, const eNB_index_t, const frame_t, const sub_frame_t, const  eNB_flag_t, const  MBMS_flag_t, logical_chan_id_t
    ,const uint32_t sourceL2Id
    ,const uint32_t destinationL2Id
                                           );

/*! \fn mac_rlc_get_buffer_occupancy_ind
 * \brief    Interface with MAC layer, UE only: request and get the number of bytes scheduled for transmission by the RLC instance
 * corresponding to the radio bearer identifier. \param[in]  mod_idP          Virtualized module identifier. \param[in]  rntiP UE
 * identifier. \param[in]  frameP            Frame index. \param[in]  subframeP         SubFrame index. \param[in]  eNB_flagP Flag
 * to indicate eNB operation (1 true, 0 false) \param[in]  channel_idP       Logical Channel identifier. \return     The maximum
 * number of bytes that the RLC instance can send in the next transmission sequence.
 */
rlc_buffer_occupancy_t mac_rlc_get_buffer_occupancy_ind(const module_id_t mod_idP,
                                                        const rnti_t rntiP,
                                                        const eNB_index_t enb,
                                                        const frame_t frameP,
                                                        const sub_frame_t subframeP,
                                                        const eNB_flag_t eNB_flagP,
                                                        const logical_chan_id_t channel_idP);
//-----------------------------------------------------------------------------
//   RLC methods
//-----------------------------------------------------------------------------
/*
 * Prints incoming byte stream in hexadecimal and readable form
 *
 * @param componentP Component identifier, see macros defined in UTIL/LOG/log.h
 * @param dataP      Pointer to data buffer to be displayed
 * @param sizeP      Number of octets in data buffer
 */
void rlc_util_print_hex_octets(
  const comp_name_t componentP,
  unsigned char *const dataP,
  const signed long sizeP);



/*! \fn rlc_op_status_t rlc_data_req     (const protocol_ctxt_t* const ctxtP, const  srb_flag_t srb_flagP,  const  MBMS_flag_t MBMS_flagP, const  rb_id_t rb_idP, mui_t muiP, confirm_t confirmP, sdu_size_t sdu_sizeP, mem_block_t *sduP)
* \brief    Interface with higher layers, map request to the RLC corresponding to the radio bearer.
* \param[in]  ctxtP            Running context.
* \param[in]  srb_flagP        Flag to indicate SRB (1) or DRB (0)
* \param[in]  MBMS_flagP       Flag to indicate whether this is the MBMS service (1) or not (0)
* \param[in]  rb_idP           Radio bearer identifier.
* \param[in]  muiP             Message Unit identifier.
* \param[in]  confirmP         Boolean, is confirmation requested.
* \param[in]  sdu_sizeP        Size of SDU in bytes.
 * \param[in]  sduP             SDU.
 * \return     A status about the processing, OK or error code.
 */
rlc_op_status_t rlc_data_req(const protocol_ctxt_t *const,
                             const srb_flag_t,
                             const MBMS_flag_t,
                             const rb_id_t,
                             const mui_t,
                             const confirm_t,
                             const sdu_size_t,
                             uint8_t *const,
                             const uint32_t *const,
                             const uint32_t *const);

/*! \fn void rlc_data_ind     (const protocol_ctxt_t* const ctxtP, const  srb_flag_t srb_flagP, const  MBMS_flag_t MBMS_flagP, const  rb_id_t rb_idP, const sdu_size_t sdu_sizeP, mem_block_t* sduP) {
* \brief    Interface with higher layers, route SDUs coming from RLC protocol instances to upper layer instance.
* \param[in]  ctxtP            Running context.
* \param[in]  srb_flagP        Flag to indicate SRB (1) or DRB (0)
* \param[in]  MBMS_flagP       Flag to indicate whether this is the MBMS service (1) or not (0)
 * \param[in]  rb_idP           Radio bearer identifier.
 * \param[in]  sdu_sizeP        Size of SDU in bytes.
 * \param[in]  sduP             SDU.
 */
void rlc_data_ind(const protocol_ctxt_t *const,
                  const srb_flag_t,
                  const MBMS_flag_t,
                  const rb_id_t,
                  const sdu_size_t,
                  uint8_t *const);

/*! \fn void rlc_data_conf     (const protocol_ctxt_t* const ctxtP, const srb_flag_t srb_flagP, const  rb_id_t rb_idP, const mui_t muiP, const rlc_tx_status_t statusP)
* \brief    Interface with higher layers, confirm to upper layer the transmission status for a SDU stamped with a MUI, scheduled for transmission.
* \param[in]  ctxtP            Running context.
* \param[in]  srb_flagP        Flag to indicate SRB (1) or DRB (0)
* \param[in]  rb_idP           Radio bearer identifier.
* \param[in]  muiP             Message Unit identifier.
* \param[in]  statusP          Status of the transmission (RLC_SDU_CONFIRM_YES, RLC_SDU_CONFIRM_NO).
*/
void rlc_data_conf(
  const protocol_ctxt_t *const,
  const  srb_flag_t,
  const  rb_id_t,
  const mui_t,
  const rlc_tx_status_t );


/*! \fn rlc_op_status_t rlc_stat_req     (
                        const protocol_ctxt_t* const ctxtP,
                        const  srb_flag_t    srb_flagP,
                        const  rb_id_t       rb_idP,
                        unsigned int* stat_rlc_mode,
      unsigned int* stat_tx_pdcp_sdu,
                        unsigned int* stat_tx_pdcp_bytes,
                        unsigned int* stat_tx_pdcp_sdu_discarded,
                        unsigned int* stat_tx_pdcp_bytes_discarded,
                        unsigned int* stat_tx_data_pdu,
                        unsigned int* stat_tx_data_bytes,
                        unsigned int* stat_tx_retransmit_pdu_by_status,
                        unsigned int* stat_tx_retransmit_bytes_by_status,
                        unsigned int* stat_tx_retransmit_pdu,
                        unsigned int* stat_tx_retransmit_bytes,
                        unsigned int* stat_tx_control_pdu,
                        unsigned int* stat_tx_control_bytes,
                        unsigned int* stat_rx_pdcp_sdu,
                        unsigned int* stat_rx_pdcp_bytes,
                        unsigned int* stat_rx_data_pdus_duplicate,
                        unsigned int* stat_rx_data_bytes_duplicate,
                        unsigned int* stat_rx_data_pdu,
                        unsigned int* stat_rx_data_bytes,
                        unsigned int* stat_rx_data_pdu_dropped,
                        unsigned int* stat_rx_data_bytes_dropped,
                        unsigned int* stat_rx_data_pdu_out_of_window,
                        unsigned int* stat_rx_data_bytes_out_of_window,
                        unsigned int* stat_rx_control_pdu,
                        unsigned int* stat_rx_control_bytes,
                        unsigned int* stat_timer_reordering_timed_out,
                        unsigned int* stat_timer_poll_retransmit_timed_out,
                        unsigned int* stat_timer_status_prohibit_timed_out)

* \brief    Request RLC statistics of a particular radio bearer.
* \param[in]  ctxtP                Running context.
* \param[in]  srb_flagP            Flag to indicate signalling radio bearer (1) or data radio bearer (0).
* \param[in]  rb_idP                       .
* \param[out] stat_rlc_mode                        RLC mode
* \param[out] stat_tx_pdcp_sdu                     Number of SDUs coming from upper layers.
* \param[out] stat_tx_pdcp_bytes                   Number of bytes coming from upper layers.
* \param[out] stat_tx_pdcp_sdu_discarded           Number of discarded SDUs coming from upper layers.
* \param[out] stat_tx_pdcp_bytes_discarded         Number of discarded bytes coming from upper layers.
* \param[out] stat_tx_data_pdu                     Number of transmitted data PDUs to lower layers.
* \param[out] stat_tx_data_bytes                   Number of transmitted data bytes to lower layers.
* \param[out] stat_tx_retransmit_pdu_by_status     Number of re-transmitted data PDUs due to status reception.
* \param[out] stat_tx_retransmit_bytes_by_status   Number of re-transmitted data bytes due to status reception.
* \param[out] stat_tx_retransmit_pdu               Number of re-transmitted data PDUs to lower layers.
* \param[out] stat_tx_retransmit_bytes             Number of re-transmitted data bytes to lower layers.
* \param[out] stat_tx_control_pdu                  Number of transmitted control PDUs to lower layers.
* \param[out] stat_tx_control_bytes                Number of transmitted control bytes to lower layers.
* \param[out] stat_rx_pdcp_sdu                     Number of SDUs delivered to upper layers.
* \param[out] stat_rx_pdcp_bytes                   Number of bytes delivered to upper layers.
* \param[out] stat_rx_data_pdus_duplicate          Number of duplicate PDUs received.
* \param[out] stat_rx_data_bytes_duplicate         Number of duplicate bytes received.
* \param[out] stat_rx_data_pdu                     Number of received PDUs from lower layers.
* \param[out] stat_rx_data_bytes                   Number of received bytes from lower layers.
* \param[out] stat_rx_data_pdu_dropped             Number of received PDUs from lower layers, then dropped.
* \param[out] stat_rx_data_bytes_dropped           Number of received bytes from lower layers, then dropped.
* \param[out] stat_rx_data_pdu_out_of_window       Number of data PDUs received out of the receive window.
* \param[out] stat_rx_data_bytes_out_of_window     Number of data bytes received out of the receive window.
* \param[out] stat_rx_control_pdu                  Number of control PDUs received.
* \param[out] stat_rx_control_bytes                Number of control bytes received.
* \param[out] stat_timer_reordering_timed_out      Number of times the timer "reordering" has timed-out.
* \param[out] stat_timer_poll_retransmit_timed_out Number of times the timer "poll_retransmit" has timed-out.
* \param[out] stat_timer_status_prohibit_timed_out Number of times the timer "status_prohibit" has timed-out.
*/

rlc_op_status_t rlc_stat_req     (
  const protocol_ctxt_t *const ctxtP,
  const srb_flag_t    srb_flagP,
  const rb_id_t       rb_idP,
  unsigned int *const stat_rlc_mode,
  unsigned int *const stat_tx_pdcp_sdu,
  unsigned int *const stat_tx_pdcp_bytes,
  unsigned int *const stat_tx_pdcp_sdu_discarded,
  unsigned int *const stat_tx_pdcp_bytes_discarded,
  unsigned int *const stat_tx_data_pdu,
  unsigned int *const stat_tx_data_bytes,
  unsigned int *const stat_tx_retransmit_pdu_by_status,
  unsigned int *const stat_tx_retransmit_bytes_by_status,
  unsigned int *const stat_tx_retransmit_pdu,
  unsigned int *const stat_tx_retransmit_bytes,
  unsigned int *const stat_tx_control_pdu,
  unsigned int *const stat_tx_control_bytes,
  unsigned int *const stat_rx_pdcp_sdu,
  unsigned int *const stat_rx_pdcp_bytes,
  unsigned int *const stat_rx_data_pdus_duplicate,
  unsigned int *const stat_rx_data_bytes_duplicate,
  unsigned int *const stat_rx_data_pdu,
  unsigned int *const stat_rx_data_bytes,
  unsigned int *const stat_rx_data_pdu_dropped,
  unsigned int *const stat_rx_data_bytes_dropped,
  unsigned int *const stat_rx_data_pdu_out_of_window,
  unsigned int *const stat_rx_data_bytes_out_of_window,
  unsigned int *const stat_rx_control_pdu,
  unsigned int *const stat_rx_control_bytes,
  unsigned int *const stat_timer_reordering_timed_out,
  unsigned int *const stat_timer_poll_retransmit_timed_out,
  unsigned int *const stat_timer_status_prohibit_timed_out);

/*! \fn int rlc_module_init(int enb_flag)
 * \brief    RAZ the memory of the RLC layer, initialize the memory pool manager (uint8_t structures mainly used in RLC module).
 */
int rlc_module_init(int enb_flag);
/** @} */

#define RLC_FG_COLOR_BLACK            "\e[0;30m"
#define RLC_FG_COLOR_RED              "\e[0;31m"
#define RLC_FG_COLOR_GREEN            "\e[0;32m"
#define RLC_FG_COLOR_ORANGE           "\e[0;33m"
#define RLC_FG_COLOR_BLUE             "\e[0;34m"
#define RLC_FG_COLOR_MAGENTA          "\e[0;35m"
#define RLC_FG_COLOR_CYAN             "\e[0;36m"
#define RLC_FG_COLOR_GRAY_BLACK       "\e[0;37m"
#define RLC_FG_COLOR_DEFAULT          "\e[0;39m"
#define RLC_FG_BRIGHT_COLOR_DARK_GRAY "\e[1;30m"
#define RLC_FG_BRIGHT_COLOR_RED       "\e[1;31m"
#define RLC_FG_BRIGHT_COLOR_GREEN     "\e[1;32m"
#define RLC_FG_BRIGHT_COLOR_YELLOW    "\e[1;33m"
#define RLC_FG_BRIGHT_COLOR_BLUE      "\e[1;34m"
#define RLC_FG_BRIGHT_COLOR_MAGENTA   "\e[1;35m"
#define RLC_FG_BRIGHT_COLOR_CYAN      "\e[1;36m"
#define RLC_FG_BRIGHT_COLOR_WHITE     "\e[1;37m"
#define RLC_FG_BRIGHT_COLOR_DEFAULT   "\e[0;39m"
#define RLC_REVERSE_VIDEO             "\e[7m"
#define RLC_NORMAL_VIDEO              "\e[27m"

#endif
