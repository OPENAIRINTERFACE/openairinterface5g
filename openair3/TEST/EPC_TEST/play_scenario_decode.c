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
                                play_scenario_decode.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */

#include "intertask_interface.h"
#include "platform_types.h"
#include "s1ap_eNB_decoder.h"
#include "assertions.h"
#include "play_scenario.h"

//------------------------------------------------------------------------------
int et_s1ap_decode_pdu(S1AP_PDU_t *const pdu, const uint8_t *const buffer, const uint32_t length)
{
  asn_dec_rval_t dec_ret;

  DevAssert(buffer != NULL);

  memset((void *)pdu, 0, sizeof(S1AP_S1AP_PDU_t));
  dec_ret = aper_decode(NULL,
                        &asn_DEF_S1AP_S1AP_PDU,
                        (void **)&pdu,
                        buffer,
                        length,
                        0,
                        0);

  if (dec_ret.code != RC_OK) {
    S1AP_ERROR("Failed to decode pdu\n");
    return -1;
  }

  return dec_ret.code;
}

//------------------------------------------------------------------------------
void et_decode_s1ap(et_s1ap_t *const s1ap)
{
  if (NULL != s1ap) {
    if (et_s1ap_decode_pdu(&s1ap->pdu, s1ap->binary_stream, s1ap->binary_stream_allocated_size) < 0) {
      AssertFatal (0, "ERROR %s() Cannot decode S1AP message!\n", __FUNCTION__);
    }
  }
}
