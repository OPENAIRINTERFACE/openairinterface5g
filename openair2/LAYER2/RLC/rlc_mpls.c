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
                                rlc_mpls.c
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
*/

#define RLC_MPLS_C
#include "rlc.h"


//-----------------------------------------------------------------------------
rlc_op_status_t mpls_rlc_data_req     (
  const protocol_ctxt_t* const ctxtP,
  const rb_id_t rb_idP,
  const sdu_size_t sdu_sizeP,
  mem_block_t* const sduP)
{
  //-----------------------------------------------------------------------------
  // third arg should be set to 1 or 0
  return rlc_data_req(ctxtP, SRB_FLAG_NO, MBMS_FLAG_NO, rb_idP, RLC_MUI_UNDEFINED, RLC_SDU_CONFIRM_NO, sdu_sizeP, sduP
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                      ,NULL, NULL
#endif
                      );
}

