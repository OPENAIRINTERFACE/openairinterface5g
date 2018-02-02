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

/*! \file rlc_um.h
* \brief This file, and only this file must be included by code that interact with RLC UM layer.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_um_impl_ RLC UM Layer Reference Implementation
* @ingroup _rlc_impl_
* @{
*/
#    ifndef __RLC_UM_H__
#        define __RLC_UM_H__
#        ifdef RLC_UM_C
#            define private_rlc_um(x)
#            define protected_rlc_um(x)
#            define public_rlc_um(x)
#        else
#            ifdef RLC_UM_MODULE
#                define private_rlc_um(x)
#                define protected_rlc_um(x)  extern x
#                define public_rlc_um(x)     extern x
#            else
#                define private_rlc_um(x)
#                define protected_rlc_um(x)
#                define public_rlc_um(x)     extern x
#            endif
#        endif
#        include <errno.h>
#        include "platform_types.h"
#        include "rlc_def.h"
#        include "rlc_def_lte.h"
#        include "rlc_um_constants.h"
#        include "rlc_um_structs.h"
#        include "rlc_um_entity.h"
#        include "mem_block.h"
#        include "rlc_um_control_primitives.h"
#        include "rlc_um_dar.h"
#        include "rlc_um_fsm.h"
#        include "rlc_um_reassembly.h"
#        include "rlc_um_receiver.h"
#        include "rlc_um_segment.h"
#        include "rlc_um_test.h"

#define PROTOCOL_RLC_UM_CTXT_FMT PROTOCOL_CTXT_FMT"[%s %02u] %s()"
#define PROTOCOL_RLC_UM_CTXT_ARGS(CTXT_Pp, rLC_Pp) PROTOCOL_CTXT_ARGS(CTXT_Pp),\
          (rLC_Pp->is_data_plane) ? "DRB UM" : "SRB UM",\
          rLC_Pp->rb_id,\
          __FUNCTION__

#define PROTOCOL_RLC_UM_MSC_FMT "[RNTI %"PRIx16" %s %02u]"
#define PROTOCOL_RLC_UM_MSC_ARGS(CTXT_Pp, rLC_Pp) \
        CTXT_Pp->rnti,\
          (rLC_Pp->is_data_plane) ? "DRB UM" : "SRB UM",\
          rLC_Pp->rb_id

#if defined(TRACE_RLC_MUTEX)
#define RLC_UM_MUTEX_LOCK(mUTEX, cTXT, rLC) \
	do {\
      int pmtl_rc = pthread_mutex_trylock(mUTEX);\
	  if (pmtl_rc != 0){\
        if (pmtl_rc == EBUSY) {\
          MSC_LOG_EVENT((cTXT->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                       "0 "PROTOCOL_RLC_UM_MSC_FMT" Warning try lock %s busy",\
                       PROTOCOL_RLC_UM_MSC_ARGS(cTXT,rLC),\
                       #mUTEX);\
          pthread_mutex_lock(mUTEX);\
        } else {\
            MSC_LOG_EVENT((cTXT->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                       "0 "PROTOCOL_RLC_UM_MSC_FMT" Error try lock %s %d",\
                       PROTOCOL_RLC_UM_MSC_ARGS(cTXT,rLC),\
                       #mUTEX, pmtl_rc);\
        }\
      }\
	} while (0);
#else
#define RLC_UM_MUTEX_LOCK(mUTEX, cTXT, rLC) pthread_mutex_lock(mUTEX)
#endif

#define RLC_UM_MUTEX_UNLOCK(mUTEX) pthread_mutex_unlock(mUTEX)

/*! \fn void     rlc_um_stat_req     (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP,
                        unsigned int* stat_tx_pdcp_sdu,
                        unsigned int* stat_tx_pdcp_bytes,
                        unsigned int* stat_tx_pdcp_sdu_discarded,
                        unsigned int* stat_tx_pdcp_bytes_discarded,
                        unsigned int* stat_tx_data_pdu,
                        unsigned int* stat_tx_data_bytes,
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
                        unsigned int* stat_timer_reordering_timed_out)
* \brief    Request TX and RX statistics of a RLC AM protocol instance.
* \param[in]  ctxt_pP                              Running context.
* \param[in]  rlc_pP                               RLC UM protocol instance pointer.
* \param[out] stat_tx_pdcp_sdu                     Number of SDUs coming from upper layers.
* \param[out] stat_tx_pdcp_bytes                   Number of bytes coming from upper layers.
* \param[out] stat_tx_pdcp_sdu_discarded           Number of discarded SDUs coming from upper layers.
* \param[out] stat_tx_pdcp_bytes_discarded         Number of discarded bytes coming from upper layers.
* \param[out] stat_tx_data_pdu                     Number of transmitted data PDUs to lower layers.
* \param[out] stat_tx_data_bytes                   Number of transmitted data bytes to lower layers.
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
* \param[out] stat_timer_reordering_timed_out      Number of times the timer "reordering" has timed-out.
*/
public_rlc_um(void     rlc_um_stat_req     (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP,
              unsigned int* stat_tx_pdcp_sdu,
              unsigned int* stat_tx_pdcp_bytes,
              unsigned int* stat_tx_pdcp_sdu_discarded,
              unsigned int* stat_tx_pdcp_bytes_discarded,
              unsigned int* stat_tx_data_pdu,
              unsigned int* stat_tx_data_bytes,
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
              unsigned int* stat_timer_reordering_timed_out);)

/*! \fn void     rlc_um_get_pdus (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP)
* \brief    Request the segmentation of SDUs based on status previously sent by MAC.
* \param[in]  ctxt_pP                   Running context.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
*/
private_rlc_um(   void     rlc_um_get_pdus (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP);)

/*! \fn void rlc_um_rx (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, struct mac_data_ind data_indication)
* \brief    Process the received PDUs from lower layer.
* \param[in]  ctxt_pP                   Running context.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
* \param[in]  data_indication           PDUs from MAC.
*/
protected_rlc_um( void     rlc_um_rx (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, struct mac_data_ind data_indication);)

/*! \fn struct mac_status_resp rlc_um_mac_status_indication (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, uint16_t tbs_sizeP, struct mac_status_ind tx_statusP)
* \brief    Request the maximum number of bytes that can be served by RLC instance to MAC and fix the amount of bytes requested by MAC for next RLC transmission.
* \param[in]  ctxt_pP                   Running context.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
* \param[in]  tbs_sizeP                 Number of bytes requested by MAC for next transmission.
* \param[in]  tx_statusP                Transmission status given by MAC on previous MAC transmission of the PDU.
* \return     The maximum number of bytes that can be served by RLC instance to MAC.
*/
public_rlc_um(    struct mac_status_resp rlc_um_mac_status_indication (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, uint16_t tbs_sizeP, struct mac_status_ind tx_statusP, const eNB_flag_t enb_flagP);)

/*! \fn void     rlc_um_set_nb_bytes_requested_by_mac (rlc_um_entity_t * const rlc_pP, const tb_size_t		tb_sizeP)
* \brief    Set available TBS size for MAC Tx.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
* \param[in]  tb_sizeP                 remaining TBS in bytes.
*/
public_rlc_um(   void     rlc_um_set_nb_bytes_requested_by_mac (rlc_um_entity_t * const rlc_pP, const tb_size_t		tb_sizeP);)

/*! \fn struct mac_data_req rlc_um_mac_data_request (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP,const eNB_flag_t  enb_flagP)
* \brief    Gives PDUs to lower layer MAC.
* \param[in]  ctxt_pP                   Running context.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
* \param[in]  enb_flagP                 eNB or UE flag.
* \return     A PDU of the previously requested number of bytes, and the updated maximum number of bytes that can be served by RLC instance to MAC for next RLC transmission.
*/
public_rlc_um(    struct mac_data_req rlc_um_mac_data_request (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP,const eNB_flag_t  enb_flagP);)


/*! \fn void     rlc_um_mac_data_indication (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP,struct mac_data_ind data_indP)
* \brief    Receive PDUs from lower layer MAC.
* \param[in]  ctxt_pP                   Running context.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
* \param[in]  data_indP                 PDUs from MAC.
*/
public_rlc_um(   void     rlc_um_mac_data_indication (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, struct mac_data_ind data_indP);)

/*! \fn uint32_t rlc_um_get_buffer_occupancy (rlc_um_entity_t *rlc_pP)
* \brief    Gets Tx Buffer Occupancy.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.)
*/
public_rlc_um(    uint32_t rlc_um_get_buffer_occupancy (rlc_um_entity_t *rlc_pP);)

/*! \fn void     rlc_um_data_req (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, mem_block_t *sduP)
* \brief    Interface with higher layers, buffer higher layer SDUS for transmission.
* \param[in]  ctxt_pP                   Running context.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
* \param[in]  sduP                      SDU. (A struct rlc_um_data_req is mapped on sduP->data.)
*/
public_rlc_um(    void     rlc_um_data_req (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, mem_block_t *sduP);)
/** @} */
#    endif
