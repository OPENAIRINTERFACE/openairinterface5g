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
#   include "s1ap_eNB.h"
#   include "sctp_eNB_task.h"
#   include "gtpv1u_eNB_task.h"
# endif

extern unsigned char NB_eNB_INST;
#endif

extern RAN_CONTEXT_t RC;

#if defined(ENABLE_ITTI)

/*------------------------------------------------------------------------------*/
# if defined(ENABLE_USE_MME)
#   define ENB_REGISTER_RETRY_DELAY 10
# endif

/*------------------------------------------------------------------------------*/

/*
static void configure_phy(module_id_t enb_id, const Enb_properties_array_t* enb_properties)
{
  MessageDef *msg_p;
  int CC_id;

  msg_p = itti_alloc_new_message (TASK_ENB_APP, PHY_CONFIGURATION_REQ);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    PHY_CONFIGURATION_REQ (msg_p).frame_type[CC_id]              = enb_properties->properties[enb_id]->frame_type[CC_id];
    PHY_CONFIGURATION_REQ (msg_p).prefix_type[CC_id]             = enb_properties->properties[enb_id]->prefix_type[CC_id];
    PHY_CONFIGURATION_REQ (msg_p).downlink_frequency[CC_id]      = enb_properties->properties[enb_id]->downlink_frequency[CC_id];
    PHY_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[CC_id] = enb_properties->properties[enb_id]->uplink_frequency_offset[CC_id];
    PHY_CONFIGURATION_REQ (msg_p).nb_antennas_tx[CC_id]          = enb_properties->properties[enb_id]->nb_antennas_tx[CC_id];
    PHY_CONFIGURATION_REQ (msg_p).nb_antennas_rx[CC_id]          = enb_properties->properties[enb_id]->nb_antennas_rx[CC_id];
    PHY_CONFIGURATION_REQ (msg_p).tx_gain[CC_id]                 = enb_properties->properties[enb_id]->tx_gain[CC_id];
    PHY_CONFIGURATION_REQ (msg_p).rx_gain[CC_id]                 = enb_properties->properties[enb_id]->rx_gain[CC_id];
  }

  itti_send_msg_to_task (TASK_PHY_ENB, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
}
*/

/*------------------------------------------------------------------------------*/
static void configure_rrc(uint32_t enb_id)
{
  MessageDef *msg_p = NULL;
  //  int CC_id;

  msg_p = itti_alloc_new_message (TASK_ENB_APP, RRC_CONFIGURATION_REQ);

  if (RC.rrc[enb_id]) {
    RCconfig_RRC(msg_p,enb_id, RC.rrc[enb_id]);
    

    LOG_I(ENB_APP,"Sending configuration message to RRC task\n");
    itti_send_msg_to_task (TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);

  }
  else AssertFatal(0,"RRC context for eNB %d not allocated\n",enb_id);
}

/*------------------------------------------------------------------------------*/
# if defined(ENABLE_USE_MME)
static uint32_t eNB_app_register(uint32_t enb_id_start, uint32_t enb_id_end)//, const Enb_properties_array_t *enb_properties)
{
  uint32_t         enb_id;
  MessageDef      *msg_p;
  uint32_t         register_enb_pending = 0;

  for (enb_id = enb_id_start; (enb_id < enb_id_end) ; enb_id++) {
    {
      s1ap_register_enb_req_t *s1ap_register_eNB;

      /* note:  there is an implicit relationship between the data structure and the message name */
      msg_p = itti_alloc_new_message (TASK_ENB_APP, S1AP_REGISTER_ENB_REQ);

      RCconfig_S1(msg_p, enb_id);

      if (enb_id == 0) RCconfig_gtpu();

      s1ap_register_eNB = &S1AP_REGISTER_ENB_REQ(msg_p);
      LOG_I(ENB_APP,"default drx %d\n",s1ap_register_eNB->default_drx);

      LOG_I(ENB_APP,"[eNB %d] eNB_app_register for instance %d\n", enb_id, ENB_MODULE_ID_TO_INSTANCE(enb_id));

      itti_send_msg_to_task (TASK_S1AP, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);

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
  const char                     *msg_name        = NULL;
  instance_t                      instance;
  int                             result;
  /* for no gcc warnings */
  (void)instance;

  itti_mark_task_ready (TASK_ENB_APP);

  LOG_I(PHY, "%s() Task ready initialise structures\n", __FUNCTION__);

  RCconfig_L1();

  RCconfig_macrlc();

  LOG_I(PHY, "%s() RC.nb_L1_inst:%d\n", __FUNCTION__, RC.nb_L1_inst);

  if (RC.nb_L1_inst>0) AssertFatal(l1_north_init_eNB()==0,"could not initialize L1 north interface\n");

  AssertFatal (enb_nb <= RC.nb_inst,
               "Number of eNB is greater than eNB defined in configuration file (%d/%d)!",
               enb_nb, RC.nb_inst);

  LOG_I(ENB_APP,"Allocating eNB_RRC_INST for %d instances\n",RC.nb_inst);

  RC.rrc = (eNB_RRC_INST **)malloc(RC.nb_inst*sizeof(eNB_RRC_INST *));
  LOG_I(PHY, "%s() RC.nb_inst:%d RC.rrc:%p\n", __FUNCTION__, RC.nb_inst, RC.rrc);

  for (enb_id = enb_id_start; (enb_id < enb_id_end) ; enb_id++) {
    RC.rrc[enb_id] = (eNB_RRC_INST*)malloc(sizeof(eNB_RRC_INST));
    LOG_I(PHY, "%s() Creating RRC instance RC.rrc[%d]:%p (%d of %d)\n", __FUNCTION__, enb_id, RC.rrc[enb_id], enb_id+1, enb_id_end);
    memset((void *)RC.rrc[enb_id],0,sizeof(eNB_RRC_INST));
    configure_rrc(enb_id);
  }

# if defined(ENABLE_USE_MME)
  /* Try to register each eNB */
  registered_enb = 0;
  register_enb_pending = eNB_app_register (enb_id_start, enb_id_end);//, enb_properties_p);
# else
  /* Start L2L1 task */
  msg_p = itti_alloc_new_message(TASK_ENB_APP, INITIALIZE_MESSAGE);
  itti_send_msg_to_task(TASK_L2L1, INSTANCE_DEFAULT, msg_p);
# endif

  do {
    // Wait for a message
    itti_receive_msg (TASK_ENB_APP, &msg_p);

    msg_name = ITTI_MSG_NAME (msg_p);
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
      LOG_I(ENB_APP, "[eNB %d] Received %s: associated MME %d\n", instance, msg_name,
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
          uint32_t not_associated = enb_nb - registered_enb;

          LOG_W(ENB_APP, " %d eNB %s not associated with a MME, retrying registration in %d seconds ...\n",
                not_associated, not_associated > 1 ? "are" : "is", ENB_REGISTER_RETRY_DELAY);

          /* Restart the eNB registration process in ENB_REGISTER_RETRY_DELAY seconds */
          if (timer_setup (ENB_REGISTER_RETRY_DELAY, 0, TASK_ENB_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
                           NULL, &enb_register_retry_timer_id) < 0) {
            LOG_E(ENB_APP, " Can not start eNB register retry timer, use \"sleep\" instead!\n");

            sleep(ENB_REGISTER_RETRY_DELAY);
            /* Restart the registration process */
            registered_enb = 0;
            register_enb_pending = eNB_app_register (enb_id_start, enb_id_end);//, enb_properties_p);
          }
        }
      }

      break;

    case S1AP_DEREGISTERED_ENB_IND:
      LOG_W(ENB_APP, "[eNB %d] Received %s: associated MME %d\n", instance, msg_name,
            S1AP_DEREGISTERED_ENB_IND(msg_p).nb_mme);

      /* TODO handle recovering of registration */
      break;

    case TIMER_HAS_EXPIRED:
      LOG_I(ENB_APP, " Received %s: timer_id %ld\n", msg_name, TIMER_HAS_EXPIRED(msg_p).timer_id);

      if (TIMER_HAS_EXPIRED (msg_p).timer_id == enb_register_retry_timer_id) {
        /* Restart the registration process */
        registered_enb = 0;
        register_enb_pending = eNB_app_register (enb_id_start, enb_id_end);//, enb_properties_p);
      }

      break;
# endif

    default:
      LOG_E(ENB_APP, "Received unexpected message %s\n", msg_name);
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);

#endif


  return NULL;
}
