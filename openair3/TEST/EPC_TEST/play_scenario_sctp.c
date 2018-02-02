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

/*
                                play_scenario_sctp.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */


#include <errno.h>

#include "intertask_interface.h"
#include "platform_types.h"
#include "assertions.h"
#include "play_scenario.h"

//------------------------------------------------------------------------------
asn_comp_rval_t * et_sctp_data_is_matching(sctp_datahdr_t * const sctp1, sctp_datahdr_t * const sctp2, const uint32_t constraints)
{
  asn_comp_rval_t *rv = NULL;
  // no comparison for ports
  if (sctp1->ppid     != sctp2->ppid) {
    S1AP_WARN("No Matching SCTP PPID %u %u\n", sctp1->ppid, sctp2->ppid);
    rv  = calloc(1, sizeof(asn_comp_rval_t));
    rv->err_code = ET_ERROR_MATCH_PACKET_SCTP_PPID;
    return rv;
  }
  if (sctp1->assoc_id != sctp2->assoc_id) {
    S1AP_WARN("No Matching SCTP assoc id %u %u\n", sctp1->assoc_id, sctp2->assoc_id);
    rv  = calloc(1, sizeof(asn_comp_rval_t));
    rv->err_code = ET_ERROR_MATCH_PACKET_SCTP_ASSOC_ID;
    return rv;
  }
  if (sctp1->stream != sctp2->stream) {
    if (constraints & ET_BIT_MASK_MATCH_SCTP_STREAM) {
      rv  = calloc(1, sizeof(asn_comp_rval_t));
      rv->err_code = ET_ERROR_MATCH_PACKET_SCTP_STREAM_ID;
      return rv;
    } else {
      S1AP_WARN("No Matching SCTP stream %u %u\n", sctp1->stream, sctp2->stream);
    }
  }
  // We do not have SSN from lower layers
//  if (sctp1->ssn != sctp2->ssn) {
//    if (constraints & ET_BIT_MASK_MATCH_SCTP_SSN) {
//      rv  = calloc(1, sizeof(asn_comp_rval_t));
//      rv->err_code = ET_ERROR_MATCH_PACKET_SCTP_SSN;
//      return rv;
//    } else {
//      S1AP_WARN("No Matching SCTP STREAM SN %u %u\n", sctp1->ssn, sctp2->ssn);
//    }
//  }
  return et_s1ap_is_matching(&sctp1->payload, &sctp2->payload,  constraints);
}

//------------------------------------------------------------------------------
asn_comp_rval_t *  et_sctp_is_matching(et_sctp_hdr_t * const sctp1, et_sctp_hdr_t * const sctp2, const uint32_t constraints)
{
  // no comparison for ports
  asn_comp_rval_t *rv = NULL;
  if (sctp1->chunk_type != sctp2->chunk_type){
    S1AP_WARN("No Matching chunk_type %u %u\n", sctp1->chunk_type, sctp2->chunk_type);
    rv  = calloc(1, sizeof(asn_comp_rval_t));
    rv->err_code = ET_ERROR_MATCH_PACKET_SCTP_CHUNK_TYPE;
    return rv;
  }
  switch (sctp1->chunk_type) {
    case SCTP_CID_DATA:
      return et_sctp_data_is_matching(&sctp1->u.data_hdr, &sctp2->u.data_hdr, constraints);
      break;

    case SCTP_CID_INIT:
      AssertFatal(0, "Not needed now");
      break;
    case SCTP_CID_INIT_ACK:
      AssertFatal(0, "Not needed now");
      break;
    default:
      AssertFatal(0, "Not needed now cid %d", sctp1->chunk_type);
  }

  return NULL;
}
