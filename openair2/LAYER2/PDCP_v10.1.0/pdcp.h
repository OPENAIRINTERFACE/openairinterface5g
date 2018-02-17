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

/*! \file LAYER2/PDCP_v10.1.0/pdcp.h
* \brief pdcp interface with RLC, RRC
* \author  Lionel GAUTHIER and Navid Nikaein
* \date 2009-2012
* \version 1.0
*/

/** @defgroup _pdcp PDCP 
* @ingroup _oai2
* @{
*/

#ifndef __PDCP_H__
#    define __PDCP_H__
//-----------------------------------------------------------------------------
#    ifdef PDCP_C
#        define private_pdcp(x) x
#        define protected_pdcp(x) x
#        define public_pdcp(x) x
#    else
#        define private_pdcp(x)
#        define public_pdcp(x) extern x
#        ifdef PDCP_FIFO_C
#            define protected_pdcp(x) extern x
#        else
#            define protected_pdcp(x)
#        endif
#    endif

#    ifdef PDCP_FIFO_C
#        define private_pdcp_fifo(x) x
#        define protected_pdcp_fifo(x) x
#        define public_pdcp_fifo(x) x
#    else
#        define private_pdcp_fifo(x)
#        define public_pdcp_fifo(x) extern x
#        ifdef PDCP_C
#            define protected_pdcp_fifo(x) extern x
#        else
#            define protected_pdcp_fifo(x)
#        endif
#    endif
//-----------------------------------------------------------------------------
#ifndef NON_ACCESS_STRATUM
#include "UTIL/MEM/mem_block.h"
#include "UTIL/LISTS/list.h"
#endif //NON_ACCESS_STRATUM
//-----------------------------------------------------------------------------
#include "RRC/LITE/defs.h"
#include "COMMON/platform_constants.h"
#include "COMMON/platform_types.h"
#include "DRB-ToAddMod.h"
#include "DRB-ToAddModList.h"
#include "SRB-ToAddMod.h"
#include "SRB-ToAddModList.h"
#if defined(Rel10) || defined(Rel14)
#include "MBMS-SessionInfoList-r9.h"
#include "PMCH-InfoList-r9.h"
#endif


extern pthread_t       pdcp_thread;
extern pthread_attr_t  pdcp_thread_attr;
extern pthread_mutex_t pdcp_mutex;
extern pthread_cond_t  pdcp_cond;
extern int             pdcp_instance_cnt;

#define PROTOCOL_PDCP_CTXT_FMT PROTOCOL_CTXT_FMT"[%s %02u] "

#define PROTOCOL_PDCP_CTXT_ARGS(CTXT_Pp, pDCP_Pp) PROTOCOL_CTXT_ARGS(CTXT_Pp),\
          (pDCP_Pp->is_srb) ? "SRB" : "DRB",\
          pDCP_Pp->rb_id
int init_pdcp_thread(void);
void cleanup_pdcp_thread(void);

public_pdcp(uint32_t Pdcp_stats_tx_window_ms[MAX_NUM_CCs][NUMBER_OF_UE_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_bytes[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_bytes_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_bytes_tmp_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_tmp_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_sn[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_throughput_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_aiat[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_aiat_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_aiat_tmp_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_tx_iat[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);

public_pdcp(uint32_t Pdcp_stats_rx_window_ms[MAX_NUM_CCs][NUMBER_OF_UE_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_tmp_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_bytes[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_bytes_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_bytes_tmp_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_sn[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_goodput_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_aiat[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_aiat_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_aiat_tmp_w[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_iat[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);
public_pdcp(uint32_t Pdcp_stats_rx_outoforder[MAX_NUM_CCs][NUMBER_OF_UE_MAX][NB_RB_MAX]);

public_pdcp(void pdcp_update_perioidical_stats(const protocol_ctxt_t* const  ctxt_pP));


/*Packet Probing for agent PDCP*/
//public_pdcp(uint64_t *pdcp_packet_counter);
//public_pdcp(uint64_t *pdcp_size_packet);
typedef struct pdcp_enb_s {
  // used for eNB stats generation
  uint16_t uid[NUMBER_OF_UE_MAX];
  rnti_t rnti[NUMBER_OF_UE_MAX];
  uint16_t num_ues;
  
  uint64_t sfn;
  frame_t  frame;
  sub_frame_t subframe;
  
} pdcp_enb_t; 

public_pdcp(pdcp_enb_t pdcp_enb[MAX_NUM_CCs]);

typedef struct pdcp_stats_s {
  time_stats_t pdcp_run;
  time_stats_t data_req;
  time_stats_t data_ind;
  time_stats_t apply_security; //
  time_stats_t validate_security;
  time_stats_t pdcp_ip;
  time_stats_t ip_pdcp; // separte thread
} pdcp_stats_t; // common to eNB and UE


typedef struct pdcp_s {
  //boolean_t     instanciated_instance;
  uint16_t       header_compression_profile;

  /* SR: added this flag to distinguish UE/eNB instance as pdcp_run for virtual
   * mode can receive data on NETLINK for eNB while eNB_flag = 0 and for UE when eNB_flag = 1
   */
  boolean_t is_ue;
  boolean_t is_srb;

   /* Configured security algorithms */
  uint8_t cipheringAlgorithm;
  uint8_t integrityProtAlgorithm;

  /* User-Plane encryption key
   * Control-Plane RRC encryption key
   * Control-Plane RRC integrity key
   * These keys are configured by RRC layer
   */
  uint8_t *kUPenc;
  uint8_t *kRRCint;
  uint8_t *kRRCenc;

  uint8_t security_activated;

  rlc_mode_t rlc_mode;
  uint8_t status_report;
  uint8_t seq_num_size;

  logical_chan_id_t lcid;
  rb_id_t           rb_id;
  /*
   * Sequence number state variables
   *
   * TX and RX window
   */
  pdcp_sn_t next_pdcp_tx_sn;
  pdcp_sn_t next_pdcp_rx_sn;
  pdcp_sn_t next_pdcp_rx_sn_before_integrity;
  /*
   * TX and RX Hyper Frame Numbers
   */
  pdcp_hfn_t tx_hfn;
  pdcp_hfn_t rx_hfn;
  pdcp_hfn_offset_t rx_hfn_offset; // related to sn mismatch

  /*
   * SN of the last PDCP SDU delivered to upper layers
   */
  pdcp_sn_t  last_submitted_pdcp_rx_sn;

  /*
   * Following array is used as a bitmap holding missing sequence
   * numbers to generate a PDCP Control PDU for PDCP status
   * report (see 6.2.6)
   */
  uint8_t missing_pdu_bitmap[512];
  /*
   * This is intentionally signed since we need a 'NULL' value
   * which is not also a valid sequence number
   */
  short int first_missing_pdu;
  /*
   * decipher using a different rx_hfn
   */


} pdcp_t;

#if defined(Rel10) || defined(Rel14)
typedef struct pdcp_mbms_s {
  boolean_t instanciated_instance;
  rb_id_t   rb_id;
} pdcp_mbms_t;
#endif
/*
 * Following symbolic constant alters the behaviour of PDCP
 * and makes it linked to PDCP test code under targets/TEST/PDCP/
 *
 * For the version at SVN repository this should be UNDEFINED!
 * XXX And later this should be configured through the Makefile
 * under targets/TEST/PDCP/
 */

/*! \fn boolean_t pdcp_data_req(const protocol_ctxt_t* const  , srb_flag_t , rb_id_t , mui_t , confirm_t ,sdu_size_t , unsigned char* , pdcp_transmission_mode_t )
* \brief This functions handles data transfer requests coming either from RRC or from IP
* \param[in] ctxt_pP        Running context.
* \param[in] rab_id         Radio Bearer ID
* \param[in] muiP
* \param[in] confirmP
* \param[in] sdu_buffer_size Size of incoming SDU in bytes
* \param[in] sdu_buffer      Buffer carrying SDU
* \param[in] mode            flag to indicate whether the userplane data belong to the control plane or data plane or transparent
* \return TRUE on success, FALSE otherwise
* \note None
* @ingroup _pdcp
*/
public_pdcp(boolean_t pdcp_data_req(
              protocol_ctxt_t*  ctxt_pP,
              const srb_flag_t srb_flagP,
              const rb_id_t rb_id,
              const mui_t muiP,
              const confirm_t confirmP, \
              const sdu_size_t sdu_buffer_size,
              unsigned char* const sdu_buffer,
              const pdcp_transmission_mode_t mode));

/*! \fn boolean_t pdcp_data_ind(const protocol_ctxt_t* const, srb_flag_t, MBMS_flag_t, rb_id_t, sdu_size_t, mem_block_t*, boolean_t)
* \brief This functions handles data transfer indications coming from RLC
* \param[in] ctxt_pP        Running context.
* \param[in] Shows if rb is SRB
* \param[in] Tells if MBMS traffic
* \param[in] rab_id Radio Bearer ID
* \param[in] sdu_buffer_size Size of incoming SDU in bytes
* \param[in] sdu_buffer Buffer carrying SDU
* \param[in] is_data_plane flag to indicate whether the userplane data belong to the control plane or data plane
* \return TRUE on success, FALSE otherwise
* \note None
* @ingroup _pdcp
*/
public_pdcp(boolean_t pdcp_data_ind(
              const protocol_ctxt_t* const  ctxt_pP,
              const srb_flag_t srb_flagP,
              const MBMS_flag_t MBMS_flagP,
              const rb_id_t rb_id,
              const sdu_size_t sdu_buffer_size,
              mem_block_t* const sdu_buffer));

/*! \fn void rrc_pdcp_config_req(const protocol_ctxt_t* const ,uint32_t,rb_id_t,uint8_t)
* \brief This functions initializes relevant PDCP entity
* \param[in] ctxt_pP        Running context.
* \param[in] actionP flag for action: add, remove , modify
* \param[in] rb_idP Radio Bearer ID of relevant PDCP entity
* \param[in] security_modeP Radio Bearer ID of relevant PDCP entity
* \return none
* \note None
* @ingroup _pdcp
*/
public_pdcp(void rrc_pdcp_config_req (
              const protocol_ctxt_t* const  ctxt_pP,
              const srb_flag_t  srb_flagP,
              const uint32_t    actionP,
              const rb_id_t     rb_idP,
              const uint8_t     security_modeP);)

/*! \fn bool rrc_pdcp_config_asn1_req (const protocol_ctxt_t* const , SRB_ToAddModList_t* srb2add_list, DRB_ToAddModList_t* drb2add_list, DRB_ToReleaseList_t*  drb2release_list)
* \brief  Function for RRC to configure a Radio Bearer.
* \param[in]  ctxt_pP           Running context.
* \param[in]  index             index of UE or eNB depending on the eNB_flag
* \param[in]  srb2add_list      SRB configuration list to be created.
* \param[in]  drb2add_list      DRB configuration list to be created.
* \param[in]  drb2release_list  DRB configuration list to be released.
* \param[in]  security_mode     Security algorithm to apply for integrity/ciphering
* \param[in]  kRRCenc           RRC encryption key
* \param[in]  kRRCint           RRC integrity key
* \param[in]  kUPenc            User-Plane encryption key
* \param[in]  defaultDRB        Default DRB ID
* \return     A status about the processing, OK or error code.
*/
public_pdcp(
  boolean_t rrc_pdcp_config_asn1_req (
    const protocol_ctxt_t* const  ctxt_pP,
    SRB_ToAddModList_t  *const srb2add_list,
    DRB_ToAddModList_t  *const drb2add_list,
    DRB_ToReleaseList_t *const drb2release_list,
    const uint8_t                   security_modeP,
    uint8_t                  *const kRRCenc,
    uint8_t                  *const kRRCint,
    uint8_t                  *const kUPenc
#if defined(Rel10) || defined(Rel14)
    ,PMCH_InfoList_r9_t  *pmch_InfoList_r9
#endif
    ,rb_id_t                 *const defaultDRB 
  ));

/*! \fn boolean_t pdcp_config_req_asn1 (const protocol_ctxt_t* const ctxt_pP, srb_flag_t srb_flagP, uint32_t  action, rb_id_t rb_id, uint8_t rb_sn, uint8_t rb_report, uint16_t header_compression_profile, uint8_t security_mode)
* \brief  Function for RRC to configure a Radio Bearer.
* \param[in]  ctxt_pP           Running context.
* \param[in]  pdcp_pP            Pointer on PDCP structure.
* \param[in]  enb_mod_idP        Virtualized enb module identifier, Not used if eNB_flagP = 0.
* \param[in]  ue_mod_idP         Virtualized ue module identifier.
* \param[in]  frame              Frame index.
* \param[in]  eNB_flag           Flag to indicate eNB (1) or UE (0)
* \param[in]  srb_flagP          Flag to indicate SRB (1) or DRB (0)
* \param[in]  action             add, remove, modify a RB
* \param[in]  rb_id              radio bearer id
* \param[in]  rb_sn              sequence number for this radio bearer
* \param[in]  drb_report         set a pdcp report for this drb
* \param[in]  header_compression set the rohc profile
* \param[in]  security_mode      set the integrity and ciphering algs
* \param[in]  kRRCenc            RRC encryption key
* \param[in]  kRRCint            RRC integrity key
* \param[in]  kUPenc             User-Plane encryption key
* \return     A status about the processing, OK or error code.
*/
public_pdcp(boolean_t pdcp_config_req_asn1 (
              const protocol_ctxt_t* const  ctxt_pP,
              pdcp_t         *const pdcp_pP,
              const srb_flag_t       srb_flagP,
              const rlc_mode_t       rlc_mode,
              const uint32_t         action,
              const uint16_t         lc_id,
              const uint16_t         mch_id,
              const rb_id_t          rb_id,
              const uint8_t          rb_sn,
              const uint8_t          rb_report,
              const uint16_t         header_compression_profile,
              const uint8_t          security_mode,
              uint8_t         *const kRRCenc,
              uint8_t         *const kRRCint,
              uint8_t         *const kUPenc));

/*! \fn void pdcp_add_UE(const protocol_ctxt_t* const  ctxt_pP)
* \brief  Function (for RRC) to add a new UE in PDCP module
* \param[in]  ctxt_pP           Running context.
* \return     A status about the processing, OK or error code.
*/
public_pdcp(void pdcp_add_UE(const protocol_ctxt_t* const  ctxt_pP));
  
/*! \fn boolean_t pdcp_remove_UE(const protocol_ctxt_t* const  ctxt_pP)
* \brief  Function for RRC to remove UE from PDCP module hashtable 
* \param[in]  ctxt_pP           Running context.
* \return     A status about the processing, OK or error code.
*/
public_pdcp(boolean_t pdcp_remove_UE(
              const protocol_ctxt_t* const  ctxt_pP));

/*! \fn void rrc_pdcp_config_release( const protocol_ctxt_t* const, rb_id_t)
* \brief This functions is unused
* \param[in]  ctxt_pP           Running context.
* \param[in] rab_id Radio Bearer ID of relevant PDCP entity
* \return none
* \note None
* @ingroup _pdcp
*/
//public_pdcp(void rrc_pdcp_config_release ( const protocol_ctxt_t* const  ctxt_pP, rb_id_t);)

/*! \fn void pdcp_run(const protocol_ctxt_t* const  ctxt_pP)
* \brief Runs PDCP entity to let it handle incoming/outgoing SDUs
* \param[in]  ctxt_pP           Running context.
* \return none
* \note None
* @ingroup _pdcp
*/
public_pdcp(void pdcp_run            (
              const protocol_ctxt_t* const  ctxt_pP);)
public_pdcp(int pdcp_module_init     (void);)
public_pdcp(void pdcp_module_cleanup (void);)
public_pdcp(void pdcp_layer_init     (void);)
public_pdcp(void pdcp_layer_cleanup  (void);)
#if defined(PDCP_USE_NETLINK_QUEUES)
public_pdcp(int pdcp_netlink_init    (void);)

#endif
#define PDCP2NW_DRIVER_FIFO 21
#define NW_DRIVER2PDCP_FIFO 22

protected_pdcp_fifo(int pdcp_fifo_flush_sdus                      (
                      const protocol_ctxt_t* const  ctxt_pP);)
protected_pdcp_fifo(int pdcp_fifo_read_input_sdus_remaining_bytes (
                      const protocol_ctxt_t* const  ctxt_pP);)
protected_pdcp_fifo(int pdcp_fifo_read_input_sdus                 (
                      const protocol_ctxt_t* const  ctxt_pP);)
protected_pdcp_fifo(void pdcp_fifo_read_input_sdus_from_otg       (
                      const protocol_ctxt_t* const  ctxt_pP);)

//-----------------------------------------------------------------------------

/*
 * Following two types are utilized between NAS driver and PDCP
 */


typedef struct pdcp_data_req_header_s {
  rb_id_t             rb_id;
  sdu_size_t          data_size;
  signed int          inst;
  ip_traffic_type_t   traffic_type;
} pdcp_data_req_header_t;

typedef struct pdcp_data_ind_header_s {
  rb_id_t             rb_id;
  sdu_size_t          data_size;
  signed int          inst;
  ip_traffic_type_t   dummy_traffic_type;
} pdcp_data_ind_header_t;

struct pdcp_netlink_element_s {
  pdcp_data_req_header_t pdcp_read_header;

  /* Data part of the message */
  uint8_t *data;
};

#if 0
/*
 * Missing PDU information struct, a copy of this will be enqueued
 * into pdcp.missing_pdus for every missing PDU
 */
typedef struct pdcp_missing_pdu_info_t {
  pdcp_sn_t sequence_number;
} pdcp_missing_pdu_info_t;
#endif

/*
 * PDCP limit values
 */
#define PDCP_MAX_SDU_SIZE 8188 // octets, see 4.3.1 Services provided to upper layers
#define PDCP_MAX_SN_5BIT  31   // 2^5-1
#define PDCP_MAX_SN_7BIT  127  // 2^7-1
#define PDCP_MAX_SN_12BIT 4095 // 2^12-1

/*
 * Reordering_Window: half of the PDCP SN space
 */
#define REORDERING_WINDOW_SN_5BIT 16
#define REORDERING_WINDOW_SN_7BIT 64
#define REORDERING_WINDOW_SN_12BIT 2048

/*
 * SN size
 */
#define PDCP_SN_5BIT  5
#define PDCP_SN_7BIT  7
#define PDCP_SN_12BIT 12


protected_pdcp(signed int             pdcp_2_nas_irq;)
public_pdcp(pdcp_stats_t              UE_pdcp_stats[NUMBER_OF_UE_MAX];)
public_pdcp(pdcp_stats_t              eNB_pdcp_stats[NUMBER_OF_eNB_MAX];)
//protected_pdcp(pdcp_t                 pdcp_array_srb_ue[NUMBER_OF_UE_MAX][2];)
//protected_pdcp(pdcp_t                 pdcp_array_drb_ue[NUMBER_OF_UE_MAX][maxDRB];)
//public_pdcp(pdcp_t                    pdcp_array_srb_eNB[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX][2];)
//protected_pdcp(pdcp_t                 pdcp_array_drb_eNB[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX][maxDRB];)

// for UE code conly
protected_pdcp(rnti_t                 pdcp_UE_UE_module_id_to_rnti[NUMBER_OF_UE_MAX];)
protected_pdcp(rnti_t                 pdcp_eNB_UE_instance_to_rnti[NUMBER_OF_UE_MAX];) // for noS1 mode
protected_pdcp(unsigned int           pdcp_eNB_UE_instance_to_rnti_index;)
#if defined(Rel10) || defined(Rel14)
public_pdcp(pdcp_mbms_t               pdcp_mbms_array_ue[NUMBER_OF_UE_MAX][maxServiceCount][maxSessionPerPMCH];)   // some constants from openair2/RRC/LITE/MESSAGES/asn1_constants.h
public_pdcp(pdcp_mbms_t               pdcp_mbms_array_eNB[NUMBER_OF_eNB_MAX][maxServiceCount][maxSessionPerPMCH];) // some constants from openair2/RRC/LITE/MESSAGES/asn1_constants.h
#endif
protected_pdcp(sdu_size_t             pdcp_output_sdu_bytes_to_write;)
protected_pdcp(sdu_size_t             pdcp_output_header_bytes_to_write;)
protected_pdcp(list_t                 pdcp_sdu_list;)
protected_pdcp(int                    pdcp_sent_a_sdu;)
protected_pdcp(pdcp_data_req_header_t pdcp_input_header;)
protected_pdcp(unsigned char          pdcp_input_sdu_buffer[MAX_IP_PACKET_SIZE];)
protected_pdcp(sdu_size_t             pdcp_input_index_header;)
protected_pdcp(sdu_size_t             pdcp_input_sdu_size_read;)
protected_pdcp(sdu_size_t             pdcp_input_sdu_remaining_size_to_read;)

#define PDCP_COLL_KEY_VALUE(mODULE_iD, rNTI, iS_eNB, rB_iD, iS_sRB) \
   ((hash_key_t)mODULE_iD          | \
    (((hash_key_t)(rNTI))   << 8)  | \
    (((hash_key_t)(iS_eNB)) << 24) | \
    (((hash_key_t)(rB_iD))  << 25) | \
    (((hash_key_t)(iS_sRB)) << 33) | \
    (((hash_key_t)(0x55))   << 34))

// hash key to the same PDCP as indexed by PDCP_COLL_KEY_VALUE(... rB_iD, iS_sRB=0) where rB_iD
// is the default DRB ID. The hidden code 0x55 indicates the key is indexed by (rB_iD,is_sRB)
// whereas the hidden code 0xaa indicates the key is for default DRB only
#define PDCP_COLL_KEY_DEFAULT_DRB_VALUE(mODULE_iD, rNTI, iS_eNB) \
    ((hash_key_t)mODULE_iD          | \
     (((hash_key_t)(rNTI))   << 8)  | \
     (((hash_key_t)(iS_eNB)) << 24) | \
     (((hash_key_t)(0xff))   << 25) | \
     (((hash_key_t)(0x00))   << 33) | \
     (((hash_key_t)(0xaa))   << 34))

// service id max val is maxServiceCount = 16 (asn1_constants.h)

#define PDCP_COLL_KEY_MBMS_VALUE(mODULE_iD, rNTI, iS_eNB, sERVICE_ID, sESSION_ID) \
   ((hash_key_t)mODULE_iD              | \
    (((hash_key_t)(rNTI))       << 8)  | \
    (((hash_key_t)(iS_eNB))     << 24) | \
    (((hash_key_t)(sERVICE_ID)) << 32) | \
    (((hash_key_t)(sESSION_ID)) << 37) | \
    (((hash_key_t)(0x0000000000000001))  << 63))

public_pdcp(hash_table_t  *pdcp_coll_p;)

#endif
/*@}*/
