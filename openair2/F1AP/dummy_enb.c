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

#include "COMMON/platform_types.h"
#include "COMMON/platform_constants.h"
#include "common/ran_context.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "NR_BCCH-BCH-Message.h"
#include "NR_ServingCellConfigCommon.h"

#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "SCHED_NR/phy_frame_config_nr.h"

#include "NR_MIB.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "../../../../nfapi/oai_integration/vendor_ext.h"
/* Softmodem params */
#include "executables/softmodem-common.h"


int rrc_mac_config_req_gNB(module_id_t Mod_idP,
                           int ssb_SubcarrierOffset,
                           int pdsch_AntennaPorts,
                           int pusch_AntennaPorts,
                           NR_ServingCellConfigCommon_t *scc,
                           int add_ue,
                           uint32_t rnti,
                           NR_CellGroupConfig_t *CellGroup){
abort();
return 0;
}
rlc_op_status_t nr_rrc_rlc_config_asn1_req (const protocol_ctxt_t   * const ctxt_pP,
    const NR_SRB_ToAddModList_t   * const srb2add_listP,
    const NR_DRB_ToAddModList_t   * const drb2add_listP,
    const NR_DRB_ToReleaseList_t  * const drb2release_listP,
    const LTE_PMCH_InfoList_r9_t * const pmch_InfoList_r9_pP,
    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list){
abort();
return 0;
}
