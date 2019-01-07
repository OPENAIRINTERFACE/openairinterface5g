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

/*! \file rlc_primitives.h
* \brief This file contains constants definition for RLC primitives.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
#ifndef __RLC_PRIMITIVES_H__
#    define __RLC_PRIMITIVES_H__

#    include <stdio.h>
#    include <stdlib.h>
#    include "platform_types.h"
#    include "platform_constants.h"
#    include "mem_block.h"


#    define RLC_NO_SAP                  RLC_NONE
#    define RLC_AM_SAP                  RLC_MODE_AM
#    define RLC_UM_SAP                  RLC_MODE_UM
#    define RLC_TM_SAP                  RLC_MODE_TM
//----------------------------------------------------------
// primitives
//----------------------------------------------------------
#    define RLC_AM_DATA_REQ                          0x01
#    define RLC_UM_DATA_REQ                          0x02
#    define RLC_TM_DATA_REQ                          0x04

#    define RLC_AM_DATA_IND                          0x08
#    define RLC_UM_DATA_IND                          0x10
#    define RLC_TM_DATA_IND                          0x20

#    define RLC_AM_DATA_CNF                          0x40
//----------------------------------------------------------
// control primitives
//----------------------------------------------------------
#    define CRLC_CONFIG_REQ                          0x81
#    define CRLC_SUSPEND_REQ                         0x82
#    define CRLC_SUSPEND_CONF                        0x84
#    define CRLC_RESUME_REQ                          0x88
#    define CRLC_STATUS_IND                          0x90
//----------------------------------------------------------
// primitives definition
//----------------------------------------------------------
#    define RLC_AM_SDU_MNGT_LAST_PDU_SN_EXTENDS_ABOVE_CONF_TX_WINDOW      0X4000
#    define RLC_AM_SDU_MNGT_LAST_PDU_SN_UNKNOWN                           0X8000
#    define RLC_TX_CONFIRM_SUCCESSFULL                                    0xCF
#    define RLC_TX_CONFIRM_FAILURE                                        0xFA

struct rlc_am_data_req {
  //struct rlc_am_sdu_management sdu_mngt;
  uint32_t             mui;
  uint16_t             data_size;    // in bytes
  uint16_t             data_offset;  // beginning of payload data may not start just after the header (PDCP header compression, etc)
  uint8_t              conf;         // confirm of sdu delivery to the peer entity
};

struct rlc_tm_data_req {
  uint16_t             data_size;    // in bits
  uint16_t             data_offset;  // beginning of payload data may not start just after the header (PDCP header compression, etc)
};


struct rlc_um_data_req {
  uint16_t             data_size;    // in bytes
  uint16_t             data_offset;  // beginning of payload data may not start just after the header (PDCP header compression, etc)
  uint8_t              use_special_li;
};

typedef struct rlc_am_data_req  rlc_am_data_req_t;
typedef struct rlc_tm_data_req  rlc_tm_data_req_t;
typedef struct rlc_um_data_req  rlc_um_data_req_t;

//----------------------------------------------------------
// control primitives definition
//----------------------------------------------------------
#    define RLC_E_R_RELEASE          0x00
#    define RLC_E_R_MODIFICATION     0x01
#    define RLC_E_R_ESTABLISHMENT    0x02
#    define RLC_E_R_RE_ESTABLISHMENT 0x04
#    define RLC_STOP                 0x08
#    define RLC_CONTINUE             0x10

#    define RLC_TM_SEGMENTATION_NOT_ALLOWED            0x00
#    define RLC_TM_SEGMENTATION_ALLOWED                0x01

#    define RLC_TM_DELIVERY_OF_ERRONEOUS_SDU_NO        0x01
#    define RLC_TM_DELIVERY_OF_ERRONEOUS_SDU_YES       0x02
#    define RLC_TM_DELIVERY_OF_ERRONEOUS_SDU_NO_DETECT 0x04


struct rlc_am_parameters {
  uint8_t              rb_id;
  uint8_t              e_r;          // reestablishment, release or modification of the rlc
  uint8_t              stop;
  uint8_t              cont;         // continue
  // timers
  uint32_t            *frame_tick_milliseconds;

  uint16_t             timer_poll;
  uint16_t             timer_poll_prohibit;
  uint16_t             timer_epc;
  uint16_t             timer_discard;
  uint16_t             timer_poll_periodic;
  uint16_t             timer_status_prohibit;
  uint16_t             timer_status_periodic;
  uint16_t             timer_rst;
  uint8_t              max_rst;
  uint16_t             timer_mrw;

  // protocol_parameters
  uint16_t             pdu_size;     // in bits
  uint8_t              missing_pdu_indicator;
  uint8_t              in_sequence_delivery; // no action : this version always deliver sdus in sequence
  uint16_t             max_dat;
  uint16_t             poll_pdu;
  uint16_t             poll_sdu;
  uint8_t              poll_window;  // in percent of the window
  uint16_t             configured_tx_window_size;
  uint16_t             configured_rx_window_size;
  uint16_t             max_mrw;

  uint8_t              last_transmission_pdu_poll_trigger;
  uint8_t              last_retransmission_pdu_poll_trigger;
  uint8_t              sdu_discard_mode;
  uint8_t              send_mrw;
};

struct rlc_um_parameters {
  uint32_t            *frame_tick_milliseconds;
  uint8_t              rb_id;
  uint8_t              e_r;          // reestablishment, release or modification of the rlc
  uint8_t              stop;
  uint8_t              cont;         // continue
  uint8_t              sdu_discard_mode;
  uint16_t             timer_discard;
  uint16_t             size_input_sdus_buffer;
};

struct rlc_tm_parameters {
  uint32_t            *frame_tick_milliseconds;
  uint8_t              rb_id;
  uint8_t              e_r;          // reestablishment, release or modification of the rlc
  uint8_t              sdu_discard_mode;
  uint8_t              segmentation_indication;
  uint8_t              delivery_of_erroneous_sdu;
  uint16_t             timer_discard;
  uint16_t             size_input_sdus_buffer;
};


struct crlc_config_req {
  union {
    struct rlc_am_parameters am_parameters;
    struct rlc_um_parameters um_parameters;
    struct rlc_tm_parameters tm_parameters;
  } parameters;
};

struct crlc_suspend_conf {
  uint16_t             vt_s_or_vt_us;
};

struct crlc_resume_req {
  // no parameters
  uint8_t              todo;
};

struct crlc_status_ind {
  uint16_t             evc;
};

struct crlc_primitive {
  uint8_t              type;
  union {
    struct crlc_config_req c_config_req;
    struct crlc_suspend_conf c_suspend_conf;
    struct crlc_resume_req c_resume_req;
    struct crlc_status_ind c_status_ind;
  } primitive;
};
#endif
