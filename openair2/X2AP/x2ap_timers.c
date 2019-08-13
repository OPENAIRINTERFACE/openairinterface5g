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

#include "x2ap_timers.h"
#include "assertions.h"
#include "PHY/defs_common.h"         /* TODO: try to not include this */
#include "x2ap_messages_types.h"
#include "x2ap_eNB_defs.h"
#include "x2ap_ids.h"
#include "x2ap_eNB_management_procedures.h"
#include "x2ap_eNB_generate_messages.h"

void x2ap_timers_init(x2ap_timers_t *t, int t_reloc_prep, int tx2_reloc_overall)
{
  t->tti               = 0;
  t->t_reloc_prep      = t_reloc_prep;
  t->tx2_reloc_overall = tx2_reloc_overall;
}

void x2ap_check_timers(instance_t instance)
{
  x2ap_eNB_instance_t          *instance_p;
  x2ap_timers_t                *t;
  x2ap_id_manager              *m;
  int                          i;
  x2ap_handover_cancel_cause_t cause;
  void                         *target;
  MessageDef                   *msg;
  int                          x2_ongoing;

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  t = &instance_p->timers;
  m = &instance_p->id_manager;

  /* increment subframe count */
  t->tti++;

  x2_ongoing = 0;

  for (i = 0; i < X2AP_MAX_IDS; i++) {
    if (m->ids[i].rnti == -1) continue;
    x2_ongoing++;

    if (m->ids[i].state == X2ID_STATE_SOURCE_PREPARE &&
        t->tti > m->ids[i].t_reloc_prep_start + t->t_reloc_prep) {
      LOG_I(X2AP, "X2 timeout reloc prep\n");
      /* t_reloc_prep timed out */
      cause = X2AP_T_RELOC_PREP_TIMEOUT;
      goto timeout;
    }

    if (m->ids[i].state == X2ID_STATE_SOURCE_OVERALL &&
        t->tti > m->ids[i].tx2_reloc_overall_start + t->tx2_reloc_overall) {
      LOG_I(X2AP, "X2 timeout reloc overall\n");
      /* tx2_reloc_overall timed out */
      cause = X2AP_TX2_RELOC_OVERALL_TIMEOUT;
      goto timeout;
    }

    /* no timeout -> check next UE */
    continue;

timeout:
    /* inform target about timeout */
    target = x2ap_id_get_target(m, i);
    x2ap_eNB_generate_x2_handover_cancel(instance_p, target, i, cause);

    /* inform RRC of cancellation */
    msg = itti_alloc_new_message(TASK_X2AP, X2AP_HANDOVER_CANCEL);
    X2AP_HANDOVER_CANCEL(msg).rnti  = x2ap_id_get_rnti(m, i);
    X2AP_HANDOVER_CANCEL(msg).cause = cause;
    itti_send_msg_to_task(TASK_RRC_ENB, instance_p->instance, msg);

    /* remove UE from X2AP */
    x2ap_release_id(m, i);
  }

  if (x2_ongoing && t->tti % 1000 == 0)
    LOG_I(X2AP, "X2 has %d process ongoing\n", x2_ongoing);
}

uint64_t x2ap_timer_get_tti(x2ap_timers_t *t)
{
  return t->tti;
}
