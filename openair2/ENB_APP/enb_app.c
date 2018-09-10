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
                                enb_app.c
                             -------------------
  AUTHOR  : Laurent Winckel, Sebastien ROUX, Lionel GAUTHIER, Navid Nikaein
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr and Navid Nikaein
*/

#include <string.h>
#include <stdio.h>

#include "enb_app.h"
#include "enb_config.h"
#include "assertions.h"
#include "common/ran_context.h"

#include "log.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# include "timer.h"
# if defined(ENABLE_USE_MME)
#   define ENB_REGISTER_RETRY_DELAY 10
#   include "s1ap_eNB.h"
# endif

#if defined(FLEXRAN_AGENT_SB_IF)
#   include "flexran_agent.h"
#endif

#include "openair1/PHY/INIT/phy_init.h"
extern unsigned char NB_eNB_INST;
#endif

extern RAN_CONTEXT_t RC;
extern int emulate_rf;

#if defined(ENABLE_ITTI)

/*------------------------------------------------------------------------------*/
# if defined(ENABLE_USE_MME)
static uint32_t eNB_app_register(ngran_node_t node_type,uint32_t enb_id_start, uint32_t enb_id_end)//, const Enb_properties_array_t *enb_properties)
{
  uint32_t         enb_id;
  MessageDef      *msg_p;
  uint32_t         register_enb_pending = 0;

  for (enb_id = enb_id_start; (enb_id < enb_id_end) ; enb_id++) {
    {
      if (node_type == ngran_eNB_DU) { // F1AP registration
        // configure F1AP here for F1C
        LOG_I(ENB_APP,"ngran_eNB_DU: Allocating ITTI message for F1AP_SETUP_REQ\n");
        msg_p = itti_alloc_new_message (TASK_ENB_APP, F1AP_SETUP_REQ);
        RCconfig_DU_F1(msg_p, enb_id);

        LOG_I(ENB_APP,"[eNB %d] eNB_app_register via F1AP for instance %d\n", enb_id, ENB_MODULE_ID_TO_INSTANCE(enb_id));
        itti_send_msg_to_task (TASK_DU_F1, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
        // configure GTPu here for F1U
      }
      else { // S1AP registration
        /* note:  there is an implicit relationship between the data structure and the message name */
        msg_p = itti_alloc_new_message (TASK_ENB_APP, S1AP_REGISTER_ENB_REQ);

        RCconfig_S1(msg_p, enb_id);

        if (enb_id == 0) RCconfig_gtpu();

        LOG_I(ENB_APP,"default drx %d\n",((S1AP_REGISTER_ENB_REQ(msg_p)).default_drx));

        LOG_I(ENB_APP,"[eNB %d] eNB_app_register via S1AP for instance %d\n", enb_id, ENB_MODULE_ID_TO_INSTANCE(enb_id));
        itti_send_msg_to_task (TASK_S1AP, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
      }

      register_enb_pending++;
    }
  }

  return register_enb_pending;
}
# endif
#endif

/*------------------------------------------------------------------------------*/
void *eNB_app_task(void *args_p)
{
#if defined(ENABLE_ITTI)
  uint32_t                        enb_nb = RC.nb_inst; 
  uint32_t                        enb_id_start = 0;
  uint32_t                        enb_id_end = enb_id_start + enb_nb;
# if defined(ENABLE_USE_MME)
  uint32_t                        register_enb_pending;
  uint32_t                        registered_enb;
  long                            enb_register_retry_timer_id;
# endif
  uint32_t                        enb_id;
  MessageDef                     *msg_p           = NULL;
  instance_t                      instance;
  int                             result;
  /* for no gcc warnings */
  (void)instance;

  itti_mark_task_ready (TASK_ENB_APP);

# if defined(ENABLE_USE_MME)
  /* Try to register each eNB */
  registered_enb = 0;
  // This assumes that node_type of all RRC instances is the same
  register_enb_pending = eNB_app_register (RC.rrc[0]->node_type,enb_id_start, enb_id_end);//, enb_properties_p);
# else
  /* Start L2L1 task */
  msg_p = itti_alloc_new_message(TASK_ENB_APP, INITIALIZE_MESSAGE);
  itti_send_msg_to_task(TASK_L2L1, INSTANCE_DEFAULT, msg_p);
# endif

  do {
    // Wait for a message
    itti_receive_msg (TASK_ENB_APP, &msg_p);

    instance = ITTI_MSG_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      LOG_W(ENB_APP, " *** Exiting ENB_APP thread\n");
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(ENB_APP, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;

# if defined(ENABLE_USE_MME)

    case S1AP_REGISTER_ENB_CNF:
      AssertFatal(RC.rrc[0]->node_type != ngran_eNB_DU, "Should not have received S1AP_REGISTER_ENB_CNF\n");

      LOG_I(ENB_APP, "[eNB %d] Received %s: associated MME %d\n", instance, ITTI_MSG_NAME (msg_p),
	    S1AP_REGISTER_ENB_CNF(msg_p).nb_mme);
      

      DevAssert(register_enb_pending > 0);
      register_enb_pending--;

      /* Check if at least eNB is registered with one MME */
      if (S1AP_REGISTER_ENB_CNF(msg_p).nb_mme > 0) {
        registered_enb++;
      }

      /* Check if all register eNB requests have been processed */
      if (register_enb_pending == 0) {
        if (registered_enb == enb_nb) {
          /* If all eNB are registered, start L2L1 task */
          MessageDef *msg_init_p;

          msg_init_p = itti_alloc_new_message (TASK_ENB_APP, INITIALIZE_MESSAGE);
          itti_send_msg_to_task (TASK_L2L1, INSTANCE_DEFAULT, msg_init_p);

        } else {
          LOG_W(ENB_APP, " %d eNB not associated with a MME, retrying registration in %d seconds ...\n",
                enb_nb - registered_enb,  ENB_REGISTER_RETRY_DELAY);

          /* Restart the eNB registration process in ENB_REGISTER_RETRY_DELAY seconds */
          if (timer_setup (ENB_REGISTER_RETRY_DELAY, 0, TASK_ENB_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
                           NULL, &enb_register_retry_timer_id) < 0) {
            LOG_E(ENB_APP, " Can not start eNB register retry timer, use \"sleep\" instead!\n");

            sleep(ENB_REGISTER_RETRY_DELAY);
            /* Restart the registration process */
            registered_enb = 0;
            register_enb_pending = eNB_app_register (RC.rrc[0]->node_type,enb_id_start, enb_id_end);//, enb_properties_p);
          }
        }
      }

      break;

    case F1AP_SETUP_RESP:
      AssertFatal(RC.rrc[0]->node_type == ngran_eNB_DU, "Should not have received F1AP_REGISTER_ENB_CNF in CU/eNB\n");

      LOG_I(ENB_APP, "[eNB %d] Received %s: associated ngran_eNB_CU %s with %d cells to activate\n", instance, ITTI_MSG_NAME (msg_p),
	    F1AP_SETUP_RESP(msg_p).gNB_CU_name,F1AP_SETUP_RESP(msg_p).num_cells_to_activate);
      
      handle_f1ap_setup_resp(&F1AP_SETUP_RESP(msg_p));

      DevAssert(register_enb_pending > 0);
      register_enb_pending--;

      /* Check if at least eNB is registered with one MME */
      if (F1AP_SETUP_RESP(msg_p).num_cells_to_activate > 0) {
        registered_enb++;
      }

      /* Check if all register eNB requests have been processed */
      if (register_enb_pending == 0) {
        if (registered_enb == enb_nb) {
          /* If all eNB cells are registered, start L2L1 task */
          MessageDef *msg_init_p;

          msg_init_p = itti_alloc_new_message (TASK_ENB_APP, INITIALIZE_MESSAGE);
          itti_send_msg_to_task (TASK_L2L1, INSTANCE_DEFAULT, msg_init_p);

        } else {
          LOG_W(ENB_APP, " %d eNB not associated with a MME, retrying registration in %d seconds ...\n",
                enb_nb - registered_enb,  ENB_REGISTER_RETRY_DELAY);

          /* Restart the eNB registration process in ENB_REGISTER_RETRY_DELAY seconds */
          if (timer_setup (ENB_REGISTER_RETRY_DELAY, 0, TASK_ENB_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
                           NULL, &enb_register_retry_timer_id) < 0) {
            LOG_E(ENB_APP, " Can not start eNB register retry timer, use \"sleep\" instead!\n");

            sleep(ENB_REGISTER_RETRY_DELAY);
            /* Restart the registration process */
            registered_enb = 0;
            register_enb_pending = eNB_app_register (RC.rrc[0]->node_type,enb_id_start, enb_id_end);//, enb_properties_p);
          }
        }
      }

      break;

    case S1AP_DEREGISTERED_ENB_IND:
      LOG_W(ENB_APP, "[eNB %d] Received %s: associated MME %d\n", instance, ITTI_MSG_NAME (msg_p),
            S1AP_DEREGISTERED_ENB_IND(msg_p).nb_mme);

      /* TODO handle recovering of registration */
      break;

    case TIMER_HAS_EXPIRED:
      LOG_I(ENB_APP, " Received %s: timer_id %ld\n", ITTI_MSG_NAME (msg_p), TIMER_HAS_EXPIRED(msg_p).timer_id);

      if (TIMER_HAS_EXPIRED (msg_p).timer_id == enb_register_retry_timer_id) {
        /* Restart the registration process */
        registered_enb = 0;
        register_enb_pending = eNB_app_register (RC.rrc[0]->node_type,enb_id_start, enb_id_end);//, enb_properties_p);
      }

      break;
# endif

    default:
      LOG_E(ENB_APP, "Received unexpected message %s\n", ITTI_MSG_NAME (msg_p));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);

#endif


  return NULL;
}
