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

#    include "platform_types.h"
#    include "platform_constants.h"
#    include "hashtable.h"
#    include "rlc_am.h"
#    include "rlc_um.h"
#    include "rlc_tm.h"
#    include "rlc_am_structs.h"
#    include "rlc_tm_structs.h"
#    include "rlc_um_structs.h"
#    include "LTE_asn_constant.h"
#    include "common/utils/LOG/log.h"
#    include "mem_block.h"
//#    include "PHY/defs.h"
#    include "LTE_RLC-Config.h"
#    include "LTE_DRB-ToAddMod.h"
#    include "LTE_DRB-ToAddModList.h"
#    include "LTE_SRB-ToAddMod.h"
#    include "LTE_SRB-ToAddModList.h"
#    include "LTE_DRB-ToReleaseList.h"

#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
#include "LTE_PMCH-InfoList-r9.h"
#endif

typedef uint64_t hash_key_t;
#define HASHTABLE_NOT_A_KEY_VALUE ((uint64_t)-1)

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

typedef enum rlc_mode_e {
  RLC_MODE_NONE    = 0,
  RLC_MODE_AM      = 1,
  RLC_MODE_UM      = 2,
  RLC_MODE_TM      = 4
} rlc_mode_t;

/*! \struct  rlc_info_t
* \brief Structure containing RLC protocol configuration parameters.
*/
typedef volatile struct {
  rlc_mode_t             rlc_mode;
  union {
    rlc_am_info_t              rlc_am_info; /*!< \sa rlc_am.h. */
    rlc_tm_info_t              rlc_tm_info; /*!< \sa rlc_tm.h. */
    rlc_um_info_t              rlc_um_info; /*!< \sa rlc_um.h. */
  } rlc;
} rlc_info_t;

/*! \struct  mac_rlc_status_resp_t
* \brief Primitive exchanged between RLC and MAC informing about the buffer occupancy of the RLC protocol instance.
*/
typedef  struct {
  rlc_buffer_occupancy_t       bytes_in_buffer; /*!< \brief Bytes buffered in RLC protocol instance. */
  rlc_buffer_occupancy_t       pdus_in_buffer;  /*!< \brief Number of PDUs buffered in RLC protocol instance (OBSOLETE). */
  frame_t                      head_sdu_creation_time;           /*!< \brief Head SDU creation time. */
  sdu_size_t                   head_sdu_remaining_size_to_send;  /*!< \brief remaining size of sdu: could be the total size or the remaining size of already segmented sdu */
  boolean_t                    head_sdu_is_segmented;     /*!< \brief 0 if head SDU has not been segmented, 1 if already segmented */
} mac_rlc_status_resp_t;


/*! \struct  mac_rlc_max_rx_header_size_t
* \brief Usefull only for debug scenario where we connect 2 RLC protocol instances without the help of the MAC .
*/
typedef struct {
  union {
    struct rlc_am_rx_pdu_management dummy1;
    struct rlc_tm_rx_pdu_management dummy2;
    //struct rlc_um_rx_pdu_management dummy3;
    struct mac_tb_ind dummy4;
    struct mac_rx_tb_management dummy5;
  } dummy;
} mac_rlc_max_rx_header_size_t;



//-----------------------------------------------------------------------------
//   PRIVATE INTERNALS OF RLC
//-----------------------------------------------------------------------------

#define  RLC_MAX_MBMS_LC (LTE_maxSessionPerPMCH * LTE_maxServiceCount)
#define  RLC_MAX_LC  ((max_val_LTE_DRB_Identity+1)* MAX_MOBILES_PER_ENB)

void (*rlc_rrc_data_ind)(
                const protocol_ctxt_t* const ctxtP,
                const rb_id_t     rb_idP,
                const sdu_size_t  sdu_sizeP,
                const uint8_t   * const sduP);

void (*rlc_rrc_data_conf)(
                const protocol_ctxt_t* const ctxtP,
                const rb_id_t         rb_idP,
                const mui_t           muiP,
                const rlc_tx_status_t statusP);

typedef void (rrc_data_ind_cb_t)(
  const protocol_ctxt_t* const ctxtP,
  const rb_id_t     rb_idP,
  const sdu_size_t  sdu_sizeP,
  const uint8_t   * const sduP);

typedef void (rrc_data_conf_cb_t)(
  const protocol_ctxt_t* const ctxtP,
  const rb_id_t         rb_idP,
  const mui_t           muiP,
  const rlc_tx_status_t statusP);


/*! \struct  rlc_t
* \brief Structure to be instanciated to allocate memory for RLC protocol instances.
*/
typedef struct rlc_union_s {
  rlc_mode_t           mode;
  union {
    rlc_am_entity_t  am;
    rlc_um_entity_t  um;
    rlc_tm_entity_t  tm;
  } rlc;
} rlc_union_t;

typedef struct rlc_mbms_s {
  rb_id_t           rb_id;
  module_id_t       instanciated_instance;
  rlc_um_entity_t   um;
} rlc_mbms_t;

typedef struct rlc_mbms_id_s {
  mbms_service_id_t       service_id;
  mbms_session_id_t       session_id;
} rlc_mbms_id_t;

#if (LTE_RRC_VERSION < MAKE_VERSION(10, 0, 0))
#    if !defined(maxServiceCount)
//unused arrays rlc_mbms_array_ue rlc_mbms_array_eNB
#        define maxServiceCount 1
#    endif
#    if !defined(maxSessionPerPMCH)
//unused arrays rlc_mbms_array_ue rlc_mbms_array_eNB
#        define maxSessionPerPMCH 1
#    endif
#endif
//rlc_mbms_t           rlc_mbms_array_ue[MAX_MOBILES_PER_ENB][maxServiceCount][maxSessionPerPMCH];   // some constants from openair2/RRC/LTE/MESSAGES/asn1_constants.h
//rlc_mbms_t           rlc_mbms_array_eNB[NUMBER_OF_eNB_MAX][maxServiceCount][maxSessionPerPMCH]; // some constants from openair2/RRC/LTE/MESSAGES/asn1_constants.h
rlc_mbms_id_t        rlc_mbms_lcid2service_session_id_ue[MAX_MOBILES_PER_ENB][RLC_MAX_MBMS_LC];    // some constants from openair2/RRC/LTE/MESSAGES/asn1_constants.h
rlc_mbms_id_t        rlc_mbms_lcid2service_session_id_eNB[MAX_eNB][RLC_MAX_MBMS_LC];  // some constants from openair2/RRC/LTE/MESSAGES/asn1_constants.h

#define rlc_mbms_enb_get_lcid_by_rb_id(Enb_mOD,rB_iD) rlc_mbms_rbid2lcid_eNB[Enb_mOD][rB_iD]
;

#define rlc_mbms_enb_set_lcid_by_rb_id(Enb_mOD,rB_iD,lOG_cH_iD) do { \
            rlc_mbms_rbid2lcid_eNB[Enb_mOD][rB_iD] = lOG_cH_iD; \
        } while (0);

#define rlc_mbms_ue_get_lcid_by_rb_id(uE_mOD,rB_iD) rlc_mbms_rbid2lcid_ue[uE_mOD][rB_iD]

#define rlc_mbms_ue_set_lcid_by_rb_id(uE_mOD,rB_iD,lOG_cH_iD) do { \
            AssertFatal(rB_iD<NB_RB_MBMS_MAX, "INVALID RB ID %u", rB_iD); \
            rlc_mbms_rbid2lcid_ue[uE_mOD][rB_iD] = lOG_cH_iD; \
        } while (0);

logical_chan_id_t    rlc_mbms_rbid2lcid_ue [MAX_MOBILES_PER_ENB][NB_RB_MBMS_MAX];              /*!< \brief Pairing logical channel identifier with radio bearer identifer. */
logical_chan_id_t    rlc_mbms_rbid2lcid_eNB[MAX_eNB][NB_RB_MBMS_MAX];              /*!< \brief Pairing logical channel identifier with radio bearer identifer. */


#define RLC_COLL_KEY_VALUE(eNB_iD, rNTI, iS_eNB, rB_iD, iS_sRB) \
   ((hash_key_t)eNB_iD             | \
    (((hash_key_t)(rNTI))   << 8)  | \
    (((hash_key_t)(iS_eNB)) << 24) | \
    (((hash_key_t)(rB_iD))  << 25) | \
    (((hash_key_t)(iS_sRB)) << 33) | \
    (((hash_key_t)(0x05))   << 34))

// index to the same RLC entity as RLC_COLL_KEY_VALUE(), but using LC_id instead
// the hidden last key indicates if this is a hash-key with RB_id (0x05) or LC_id (0x0a)
#define RLC_COLL_KEY_LCID_VALUE(eNB_iD, rNTI, iS_eNB, lC_iD, iS_sRB) \
   ((hash_key_t)eNB_iD             | \
    (((hash_key_t)(rNTI))   << 8)  | \
    (((hash_key_t)(iS_eNB)) << 24) | \
    (((hash_key_t)(lC_iD))  << 25) | \
    (((hash_key_t)(iS_sRB)) << 33) | \
    (((hash_key_t)(0x0a))   << 34))

#define RLC_COLL_KEY_SOURCE_DEST_VALUE(eNB_iD, rNTI, iS_eNB, lC_iD, sOURCE_iD, dEST_iD, iS_sRB) \
   ((hash_key_t)eNB_iD             | \
    (((hash_key_t)(rNTI))   << 8)  | \
    (((hash_key_t)(iS_eNB)) << 24) | \
    (((hash_key_t)(lC_iD))  << 25) | \
    (((hash_key_t)(dEST_iD)) << 33) | \
    (((hash_key_t)(0x05))   << 57))

#define RLC_COLL_KEY_LCID_SOURCE_DEST_VALUE(eNB_iD, rNTI, iS_eNB, lC_iD, sOURCE_iD, dEST_iD, iS_sRB) \
   ((hash_key_t)eNB_iD             | \
    (((hash_key_t)(rNTI))   << 8)  | \
    (((hash_key_t)(iS_eNB)) << 24) | \
    (((hash_key_t)(lC_iD))  << 25) | \
    (((hash_key_t)(dEST_iD)) << 33) | \
    (((hash_key_t)(0x0a))   << 57))


// service id max val is maxServiceCount = 16 (asn1_constants.h)

#define RLC_COLL_KEY_MBMS_VALUE(eNB_iD, rNTI, iS_eNB, sERVICE_ID, sESSION_ID) \
   ((hash_key_t)eNB_iD             | \
    (((hash_key_t)(rNTI))       << 8)  | \
    (((hash_key_t)(iS_eNB))     << 24) | \
    (((hash_key_t)(sERVICE_ID)) << 32) | \
    (((hash_key_t)(sESSION_ID)) << 37) | \
    (((hash_key_t)(0x0000000000000001))  << 63))

hash_table_t  *rlc_coll_p;

/*! \fn tbs_size_t mac_rlc_serialize_tb (char* bufferP, list_t transport_blocksP)
* \brief  Serialize a list of transport blocks coming from RLC in order to be processed by MAC.
* \param[in]  bufferP                 Memory area where the transport blocks have to be serialized.
* \param[in]  transport_blocksP       List of transport blocks.
* \return     The amount of bytes that have been written due to serialization.
*/
tbs_size_t            mac_rlc_serialize_tb   (char*, list_t);

/*! \fn struct mac_data_ind mac_rlc_deserialize_tb (char* bufferP, tb_size_t tb_sizeP, num_tb_t num_tbP, crc_t *crcsP)
* \brief  Serialize a list of transport blocks coming from RLC in order to be processed by MAC.
* \param[in]  bufferP       Memory area where the transport blocks are serialized.
* \param[in]  tb_sizeP      Size of transport blocks.
* \param[in]  num_tbP       Number of transport blocks.
* \param[in]  crcsP         Array of CRC for each transport block.
* \return     A mac_data_ind structure containing a list of transport blocks.
*/
struct mac_data_ind   mac_rlc_deserialize_tb (char*, tb_size_t, num_tb_t, crc_t *);


//-----------------------------------------------------------------------------
//   PUBLIC INTERFACE WITH RRC
//-----------------------------------------------------------------------------
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
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
                  const protocol_ctxt_t* const,
                  const LTE_SRB_ToAddModList_t* const ,
                  const LTE_DRB_ToAddModList_t* const ,
                  const LTE_DRB_ToReleaseList_t* const ,
                  const LTE_PMCH_InfoList_r9_t * const pmch_info_listP ,
                  const uint32_t ,
                  const uint32_t );
#else
/*! \fn rlc_op_status_t rrc_rlc_config_asn1_req (const protocol_ctxt_t* const ctxtP, const SRB_ToAddModList_t* const srb2add_listP, const DRB_ToAddModList_t* const drb2add_listP, const DRB_ToReleaseList_t* const drb2release_listP)
* \brief  Function for RRC to configure a Radio Bearer.
* \param[in]  ctxtP              Running context.
* \param[in]  srb2add_listP      SRB configuration list to be created.
* \param[in]  drb2add_listP      DRB configuration list to be created.
* \param[in]  drb2release_listP  DRB configuration list to be released.
* \return     A status about the processing, OK or error code.
*/
rlc_op_status_t rrc_rlc_config_asn1_req (
                  const protocol_ctxt_t* const,
                  const LTE_SRB_ToAddModList_t* const ,
                  const LTE_DRB_ToAddModList_t* const ,
                  const LTE_DRB_ToReleaseList_t* const );
#endif


/*! \fn void rb_free_rlc_union (void *rlcu_pP)
 * \brief  Free the rlc memory contained in the RLC embedded in the rlc_union_t
 *  struct pointed by of the rlcu_pP parameter. Free the rlc_union_t struct also.
 * \param[in]  rlcu_pP          Pointer on the rlc_union_t struct.
 */
void
               rb_free_rlc_union (void *rlcu_pP);


/*! \fn rlc_op_status_t rrc_rlc_remove_ue   (const protocol_ctxt_t* const ctxtP)
 * \brief  Remove all RLC protocol instances from all radio bearers allocated to a UE.
 * \param[in]  ctxtP              Running context.
 * \return     A status about the processing, OK or error code.
*/
rlc_op_status_t rrc_rlc_remove_ue (const protocol_ctxt_t* const);


/*! \fn rlc_op_status_t rrc_rlc_remove_rlc   (const protocol_ctxt_t* const ctxtP, const srb_flag_t srb_flagP, const MBMS_flag_t MBMS_flagP, const  rb_id_t rb_idP)
* \brief  Remove a RLC protocol instance from a radio bearer.
* \param[in]  ctxtP              Running context.
* \param[in]  srb_flagP          Flag to indicate SRB (1) or DRB (0)
* \param[in]  MBMS_flag          Flag to indicate whether this is an MBMS service (1) or not (0)
* \param[in]  rb_idP             Radio bearer identifier.
* \return     A status about the processing, OK or error code.
*/
rlc_op_status_t rrc_rlc_remove_rlc   (const protocol_ctxt_t* const, const srb_flag_t, const MBMS_flag_t, const  rb_id_t );

/*! \fn rlc_union_t*  rrc_rlc_add_rlc   (const protocol_ctxt_t* const ctxtP, const srb_flag_t srb_flagP, const  MBMS_flag_t MBMS_flagP, const  rb_id_t rb_idP, logical_chan_id_t chan_idP, rlc_mode_t rlc_modeP)
* \brief  Add a RLC protocol instance to a radio bearer.
* \param[in]  ctxtP              Running context.
* \param[in]  srb_flagP          Flag to indicate SRB (1) or DRB (0)
* \param[in]  MBMS_flag          Flag to indicate whether this is an MBMS service (1) or not (0)
* \param[in]  rb_idP             Radio bearer identifier.
* \param[in]  chan_idP           Logical channel identifier.
* \param[in]  rlc_modeP          Mode of RLC (AM, UM, TM).
* \return     A status about the processing, OK or error code.
*/
rlc_union_t*  rrc_rlc_add_rlc      (const protocol_ctxt_t* const, const srb_flag_t,  const  MBMS_flag_t MBMS_flagP, const  rb_id_t, logical_chan_id_t, rlc_mode_t
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  ,const uint32_t  sourceL2Id,
  const uint32_t  destinationL2Id
#endif
);

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
* \param[in]  rlc_infoP        RLC configuration parameters issued from Radio Resource Manager.
* \return     A status about the processing, OK or error code.
*/
rlc_op_status_t rrc_rlc_config_req   (
                  const protocol_ctxt_t* const,
                  const srb_flag_t,
                  const MBMS_flag_t,
                  config_action_t,
                  const  rb_id_t,
                  rlc_info_t );

/*! \fn rlc_op_status_t rrc_rlc_data_req     (const protocol_ctxt_t* const ctxtP, const  MBMS_flag_t MBMS_flagP, const  rb_id_t rb_idP, mui_t muiP, confirm_t confirmP, sdu_size_t sdu_sizeP, char* sduP)
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
rlc_op_status_t rrc_rlc_data_req     (const protocol_ctxt_t* const , const  MBMS_flag_t, const  rb_id_t, mui_t, confirm_t, sdu_size_t, char *);

/*! \fn void  rrc_rlc_register_rrc ( void (*rrc_data_indP)  (const protocol_ctxt_t* const ctxtP, const  rb_id_t rb_idP, sdu_size_t sdu_sizeP, char* sduP), void (*rrc_data_confP) (const protocol_ctxt_t* const ctxtP, const  rb_id_t rb_idP, mui_t muiP, rlc_tx_status_t statusP)
* \brief  This function is called by RRC to register its DATA-INDICATE and DATA-CONFIRM handlers to RLC layer.
* \param[in]  rrc_data_indP       Pointer on RRC data indicate function.
* \param[in]  rrc_data_confP      Pointer on RRC data confirm callback function.
*/
void rrc_rlc_register_rrc (rrc_data_ind_cb_t rrc_data_indP, rrc_data_conf_cb_t rrc_data_confP);

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
* \param [in]     tb_sizeP         Available Tx TBS in bytes. For UE only.
* \param [in,out] bufferP          Memory area to fill with the bytes requested by MAC.
* \return     A status about the processing, OK or error code.
*/
tbs_size_t            mac_rlc_data_req     (const module_id_t, const rnti_t, const eNB_index_t, const frame_t, const  eNB_flag_t, const  MBMS_flag_t, logical_chan_id_t, const tb_size_t,char*
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                                           ,const uint32_t sourceL2Id
                                                           ,const uint32_t destinationL2Id
#endif
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
void                  mac_rlc_data_ind     (const module_id_t, const rnti_t, const eNB_index_t,const frame_t, const  eNB_flag_t, const  MBMS_flag_t, logical_chan_id_t, char*, tb_size_t, num_tb_t,
               crc_t* );

/*! \fn mac_rlc_status_resp_t mac_rlc_status_ind     (const module_id_t mod_idP, const rnti_t rntiP, const frame_t frameP, const sub_frame_t subframeP, const  eNB_flag_t eNB_flagP, const  MBMS_flag_t MBMS_flagP, logical_chan_id_t rb_idP, tb_size_t tb_sizeP)
* \brief    Interface with MAC layer, request and set the number of bytes scheduled for transmission by the RLC instance corresponding to the radio bearer identifier.
* \param[in]  mod_idP          Virtualized module identifier.
* \param[in]  rntiP            UE identifier.
* \param[in]  frameP            Frame index.
* \param[in]  subframeP         SubFrame index.
* \param[in]  eNB_flagP         Flag to indicate eNB operation (1 true, 0 false)
* \param[in]  MBMS_flagP       Flag to indicate whether this is the MBMS service (1) or not (0)
* \param[in]  rb_idP           Radio bearer identifier.
* \param[in]  tb_sizeP         Size of a transport block set in bytes.
* \return     The maximum number of bytes that the RLC instance can send in the next transmission sequence.
*/
mac_rlc_status_resp_t mac_rlc_status_ind   (const module_id_t, const rnti_t, const eNB_index_t, const frame_t, const sub_frame_t, const  eNB_flag_t, const  MBMS_flag_t, logical_chan_id_t, tb_size_t
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                                           ,const uint32_t sourceL2Id
                                                           ,const uint32_t destinationL2Id
#endif
  );

/*! \fn rlc_buffer_occupancy_t mac_rlc_get_buffer_occupancy_ind(const module_id_t module_idP, const rnti_t rntiP, const eNB_index_t eNB_index, const frame_t frameP, const sub_frame_t subframeP,const eNB_flag_t enb_flagP, const logical_chan_id_t channel_idP)
* \brief    Interface with MAC layer, UE only: request and get the number of bytes scheduled for transmission by the RLC instance corresponding to the radio bearer identifier.
* \param[in]  mod_idP          Virtualized module identifier.
* \param[in]  rntiP            UE identifier.
* \param[in]  frameP            Frame index.
* \param[in]  subframeP         SubFrame index.
* \param[in]  eNB_flagP         Flag to indicate eNB operation (1 true, 0 false)
* \param[in]  channel_idP       Logical Channel identifier.
* \return     The maximum number of bytes that the RLC instance can send in the next transmission sequence.
*/
rlc_buffer_occupancy_t mac_rlc_get_buffer_occupancy_ind(const module_id_t, const rnti_t, const eNB_index_t, const frame_t, const sub_frame_t, const eNB_flag_t, const logical_chan_id_t );
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
             unsigned char* const dataP,
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
rlc_op_status_t rlc_data_req     (
             const protocol_ctxt_t* const,
             const  srb_flag_t,
             const  MBMS_flag_t ,
             const  rb_id_t ,
             const  mui_t ,
             const confirm_t ,
             const sdu_size_t ,
             mem_block_t * const
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
             ,const uint32_t * const
             ,const uint32_t * const
#endif
             );

/*! \fn void rlc_data_ind     (const protocol_ctxt_t* const ctxtP, const  srb_flag_t srb_flagP, const  MBMS_flag_t MBMS_flagP, const  rb_id_t rb_idP, const sdu_size_t sdu_sizeP, mem_block_t* sduP) {
* \brief    Interface with higher layers, route SDUs coming from RLC protocol instances to upper layer instance.
* \param[in]  ctxtP            Running context.
* \param[in]  srb_flagP        Flag to indicate SRB (1) or DRB (0)
* \param[in]  MBMS_flagP       Flag to indicate whether this is the MBMS service (1) or not (0)
* \param[in]  rb_idP           Radio bearer identifier.
* \param[in]  sdu_sizeP        Size of SDU in bytes.
* \param[in]  sduP             SDU.
*/
void rlc_data_ind(
             const protocol_ctxt_t* const,
             const srb_flag_t,
             const MBMS_flag_t ,
             const rb_id_t,
             const sdu_size_t,
             mem_block_t* const);


/*! \fn void rlc_data_conf     (const protocol_ctxt_t* const ctxtP, const srb_flag_t srb_flagP, const  rb_id_t rb_idP, const mui_t muiP, const rlc_tx_status_t statusP)
* \brief    Interface with higher layers, confirm to upper layer the transmission status for a SDU stamped with a MUI, scheduled for transmission.
* \param[in]  ctxtP            Running context.
* \param[in]  srb_flagP        Flag to indicate SRB (1) or DRB (0)
* \param[in]  rb_idP           Radio bearer identifier.
* \param[in]  muiP             Message Unit identifier.
* \param[in]  statusP          Status of the transmission (RLC_SDU_CONFIRM_YES, RLC_SDU_CONFIRM_NO).
*/
void rlc_data_conf(
             const protocol_ctxt_t* const,
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
             const protocol_ctxt_t* const ctxtP,
             const srb_flag_t    srb_flagP,
             const rb_id_t       rb_idP,
             unsigned int* const stat_rlc_mode,
	     unsigned int* const stat_tx_pdcp_sdu,
             unsigned int* const stat_tx_pdcp_bytes,
             unsigned int* const stat_tx_pdcp_sdu_discarded,
             unsigned int* const stat_tx_pdcp_bytes_discarded,
             unsigned int* const stat_tx_data_pdu,
             unsigned int* const stat_tx_data_bytes,
             unsigned int* const stat_tx_retransmit_pdu_by_status,
             unsigned int* const stat_tx_retransmit_bytes_by_status,
             unsigned int* const stat_tx_retransmit_pdu,
             unsigned int* const stat_tx_retransmit_bytes,
             unsigned int* const stat_tx_control_pdu,
             unsigned int* const stat_tx_control_bytes,
             unsigned int* const stat_rx_pdcp_sdu,
             unsigned int* const stat_rx_pdcp_bytes,
             unsigned int* const stat_rx_data_pdus_duplicate,
             unsigned int* const stat_rx_data_bytes_duplicate,
             unsigned int* const stat_rx_data_pdu,
             unsigned int* const stat_rx_data_bytes,
             unsigned int* const stat_rx_data_pdu_dropped,
             unsigned int* const stat_rx_data_bytes_dropped,
             unsigned int* const stat_rx_data_pdu_out_of_window,
             unsigned int* const stat_rx_data_bytes_out_of_window,
             unsigned int* const stat_rx_control_pdu,
             unsigned int* const stat_rx_control_bytes,
             unsigned int* const stat_timer_reordering_timed_out,
             unsigned int* const stat_timer_poll_retransmit_timed_out,
             unsigned int* const stat_timer_status_prohibit_timed_out);

/*! \fn int rlc_module_init(void)
* \brief    RAZ the memory of the RLC layer, initialize the memory pool manager (mem_block_t structures mainly used in RLC module).
*/
int rlc_module_init(void);

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
