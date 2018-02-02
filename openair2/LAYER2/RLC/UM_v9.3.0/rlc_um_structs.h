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

/*! \file rlc_um_structs.h
* \brief This file defines structures used inside the RLC UM.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
#    ifndef __RLC_UM_STRUCTS_H__
#        define __RLC_UM_STRUCTS_H__

#        include "platform_types.h"
#        include "list.h"
#        include "rlc_um_constants.h"
#        include "mac_primitives.h"
#        include "rlc_primitives.h"
#        include "mac_rlc_primitives.h"
//-----------------------
/**
* @addtogroup _rlc_um_impl_
* @{
*/
/*! \struct  rlc_um_tx_sdu_management_t
* \brief Structure containing SDU variables related to its segmentation and transmission.
*/
typedef struct rlc_um_tx_sdu_management {
  uint8_t             *first_byte;                 /*!< \brief Pointer on SDU payload. */
  int32_t             sdu_creation_time;          /*!< \brief Time stamped with mac_xface->frame. */
  uint16_t             sdu_remaining_size;         /*!< \brief Remaining size in bytes to be filled in a PDU. */
  uint16_t             sdu_test_remaining_size;
  uint16_t             sdu_segmented_size;           /*!< \brief Bytes already segmented in a/several PDU(s). */
  uint16_t             sdu_size;                     /*!< \brief SDU size in bytes. */
} rlc_um_tx_sdu_management_t;
/** @} */

/**
* @addtogroup _rlc_um_segment_impl_
* @{
*/
/*! \struct  rlc_um_pdu_sn_5_t
* \brief Structure helping coding and decoding the first byte of a UMD PDU.
*/
typedef struct rlc_um_pdu_sn_5 {
  uint8_t     b1;      /*!< \brief 1st byte. */
  uint8_t     data[3]; /*!< \brief Following bytes. */
} __attribute__((__packed__)) rlc_um_pdu_sn_5_t ;

/*! \struct  rlc_um_pdu_sn_10_t
* \brief Structure helping coding and decoding the first 2 bytes of a UMD PDU.
*/
typedef struct rlc_um_pdu_sn_10 {
  uint8_t  b1;      /*!< \brief 1st byte. */
  uint8_t  b2;      /*!< \brief 2nd byte. */
  uint8_t  data[2]; /*!< \brief Following bytes. */
} __attribute__((__packed__)) rlc_um_pdu_sn_10_t ;


/*! \struct  rlc_am_e_li_t
* \brief Structure helping coding and decoding LI and e bits in UMD PDUs.
*/
typedef struct rlc_um_e_li {
  uint8_t  b1; /*!< \brief 1st byte. */
  uint8_t  b2; /*!< \brief 2nd byte. */
  uint8_t  b3; /*!< \brief 3rd byte. */
} rlc_um_e_li_t;
/** @} */
/**
* @addtogroup _rlc_um_segment_impl_
* @{
*/
/*! \struct  rlc_um_pdu_info_t
* \brief Structure for storing decoded informations from the header of a UMD PDU.
*/
typedef struct rlc_um_pdu_info {
  uint16_t  free_bits:3; /*!< \brief unused bits in bitfield. */
  uint16_t  fi:2;        /*!< \brief Framing Info field. */
  uint16_t  e:1;         /*!< \brief Extension bit field. */
  uint16_t  sn:10;       /*!< \brief Sequence Number field. */
  uint16_t  num_li;      /*!< \brief Number of Length Indicators. */
  int16_t  li_list[RLC_UM_SEGMENT_NB_MAX_LI_PER_PDU]; /*!< \brief List of Length Indicators. */
  int16_t  hidden_size; /*!< \brief Part of payload size in bytes that is not included in the sum of LI fields. */;
  uint8_t*  payload;     /*!< \brief Pointer on PDU payload. */
  int16_t  payload_size;/*!< \brief Size of payload in bytes. */
  int16_t  header_size; /*!< \brief Size of header in bytes (including SO field and LI fields). */
} rlc_um_pdu_info_t ;
/** @} */


struct rlc_um_data_req_alloc {  // alloc enought bytes for sdu mngt also
  union {
    struct rlc_um_data_req dummy1;
    struct rlc_um_tx_sdu_management dummy2;
  } dummy;
};

#    endif
