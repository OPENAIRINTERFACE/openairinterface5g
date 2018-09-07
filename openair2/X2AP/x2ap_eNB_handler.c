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
#include <stdint.h>

#include "intertask_interface.h"

#include "asn1_conversions.h"

#include "x2ap_common.h"
#include "x2ap_eNB_defs.h"
#include "x2ap_eNB_handler.h"

#include "x2ap_eNB_management_procedures.h"

#include "assertions.h"
#include "conversions.h"

void x2ap_handle_x2_setup_message(x2ap_eNB_data_t *enb_desc_p, int sctp_shutdown)
{
  if (sctp_shutdown) {
    /* A previously connected eNB has been shutdown */

    /* TODO check if it was used by some eNB and send a message to inform these eNB if there is no more associated eNB */
    if (enb_desc_p->state == X2AP_ENB_STATE_CONNECTED) {
      enb_desc_p->state = X2AP_ENB_STATE_DISCONNECTED;

      if (enb_desc_p->x2ap_eNB_instance-> x2_target_enb_associated_nb > 0) {
        /* Decrease associated eNB number */
        enb_desc_p->x2ap_eNB_instance-> x2_target_enb_associated_nb --;
      }

      /* If there are no more associated eNB, inform eNB app */
      if (enb_desc_p->x2ap_eNB_instance->x2_target_enb_associated_nb == 0) {
        MessageDef                 *message_p;

        message_p = itti_alloc_new_message(TASK_X2AP, X2AP_DEREGISTERED_ENB_IND);
        X2AP_DEREGISTERED_ENB_IND(message_p).nb_x2 = 0;
        itti_send_msg_to_task(TASK_ENB_APP, enb_desc_p->x2ap_eNB_instance->instance, message_p);
      }
    }
  } else {
    /* Check that at least one setup message is pending */
    DevCheck(enb_desc_p->x2ap_eNB_instance->x2_target_enb_pending_nb > 0,
             enb_desc_p->x2ap_eNB_instance->instance,
             enb_desc_p->x2ap_eNB_instance->x2_target_enb_pending_nb, 0);

    if (enb_desc_p->x2ap_eNB_instance->x2_target_enb_pending_nb > 0) {
      /* Decrease pending messages number */
      enb_desc_p->x2ap_eNB_instance->x2_target_enb_pending_nb --;
    }

    /* If there are no more pending messages, inform eNB app */
    if (enb_desc_p->x2ap_eNB_instance->x2_target_enb_pending_nb == 0) {
      MessageDef                 *message_p;

      message_p = itti_alloc_new_message(TASK_X2AP, X2AP_REGISTER_ENB_CNF);
      X2AP_REGISTER_ENB_CNF(message_p).nb_x2 = enb_desc_p->x2ap_eNB_instance->x2_target_enb_associated_nb;
      itti_send_msg_to_task(TASK_ENB_APP, enb_desc_p->x2ap_eNB_instance->instance, message_p);
    }
  }
}
