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

#include "mac_rrc_dl_handler.h"

#include "mac_proto.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"

#include "NR_RRCSetup.h"
#include "NR_DL-CCCH-Message.h"
#include "NR_CellGroupConfig.h"

int dl_rrc_message(module_id_t module_id, const f1ap_dl_rrc_message_t *dl_rrc)
{
  LOG_I(NR_MAC, "DL RRC Message Transfer with %d bytes for RNTI %04x SRB %d\n", dl_rrc->rrc_container_length, dl_rrc->rnti, dl_rrc->srb_id);

  nr_rlc_srb_recv_sdu(dl_rrc->rnti, dl_rrc->srb_id, dl_rrc->rrc_container, dl_rrc->rrc_container_length);
  return 0;
}
