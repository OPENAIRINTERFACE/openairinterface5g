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

# include "intertask_interface.h"
# include "create_tasks.h"
# include "common/utils/LOG/log.h"
# include "executables/lte-softmodem.h"
# include "common/ran_context.h"

  #include "sctp_eNB_task.h"
  #include "x2ap_eNB.h"
  #include "s1ap_eNB.h"
  #include "openair3/ocp-gtpu/gtp_itf.h"
  #if ENABLE_RAL
    #include "lteRALue.h"
    #include "lteRALenb.h"
  #endif
  #include "RRC/LTE/rrc_defs.h"
# include "enb_app.h"
# include "openair2/LAYER2/MAC/mac_proto.h"
#include <openair3/ocp-gtpu/gtp_itf.h>

extern RAN_CONTEXT_t RC;

int create_tasks(uint32_t enb_nb) {
  LOG_D(ENB_APP, "%s(enb_nb:%d\n", __FUNCTION__, enb_nb);
  int rc;

  if (enb_nb == 0) return 0;

  LOG_I(ENB_APP, "Creating ENB_APP eNB Task\n");
  rc = itti_create_task (TASK_ENB_APP, eNB_app_task, NULL);
  AssertFatal(rc >= 0, "Create task for eNB APP failed\n");
  rrc_enb_init();
  itti_mark_task_ready(TASK_RRC_ENB);

  if (get_softmodem_params()->emulate_l1 || EPC_MODE_ENABLED) {
    rc = itti_create_task(TASK_SCTP, sctp_eNB_task, NULL);
    AssertFatal(rc >= 0, "Create task for SCTP failed\n");
  }

  if (EPC_MODE_ENABLED) {
    rc = itti_create_task(TASK_S1AP, s1ap_eNB_task, NULL);
    AssertFatal(rc >= 0, "Create task for S1AP failed\n");
    rc = itti_create_task(TASK_GTPV1_U, gtpv1uTask, NULL);
    AssertFatal(rc >= 0, "Create task for GTPV1U failed\n");
  }

  if (is_x2ap_enabled()) {
      rc = itti_create_task(TASK_X2AP, x2ap_task, NULL);
      AssertFatal(rc >= 0, "Create task for X2AP failed\n");
  } else {
      LOG_I(X2AP, "X2AP is disabled.\n");
  }

  return 0;
}
