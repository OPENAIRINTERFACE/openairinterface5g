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
#include "executables/lte-softmodem.h"

#include "common/utils/LOG/log.h"

# include "intertask_interface.h"
#   include "s1ap_eNB.h"
#   include "sctp_eNB_task.h"
#   include "openair3/ocp-gtpu/gtp_itf.h"

#   include "x2ap_eNB.h"
#   include "x2ap_messages_types.h"
#   include "m2ap_eNB.h"
#   include "m2ap_messages_types.h"
#   define X2AP_ENB_REGISTER_RETRY_DELAY   10

#include "openair1/PHY/INIT/phy_init.h"

extern RAN_CONTEXT_t RC;

#   define ENB_REGISTER_RETRY_DELAY 10

#include "executables/lte-softmodem.h"


/*************************** ENB M2AP **************************/
//static uint32_t eNB_app_register_MBMS_STA(ngran_node_t node_type,uint32_t enb_id_start, uint32_t enb_id_end) {
//  uint32_t         enb_id;
//  //MessageDef      *msg_p;
//  uint32_t         register_enb_pending = 0;
//
//  for (enb_id = enb_id_start; (enb_id < enb_id_end) ; enb_id++) {
//    {
//
//        //msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, S1AP_REGISTER_ENB_REQ);
//        //RCconfig_S1(msg_p, enb_id);
//
//        if (enb_id == 0) RCconfig_gtpu();
//
//        //LOG_I(ENB_APP,"default drx %d\n",((S1AP_REGISTER_ENB_REQ(msg_p)).default_drx));
//
//        //LOG_I(ENB_APP,"[eNB %d] eNB_app_register via S1AP for instance %d\n", enb_id, ENB_MODULE_ID_TO_INSTANCE(enb_id));
//        //itti_send_msg_to_task (TASK_S1AP, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
//
//      register_enb_pending++;
//    }
//  }
//
//  return register_enb_pending;
//}
//*****end M2AP ****/

static uint32_t eNB_app_register_m2(uint32_t enb_id_start, uint32_t enb_id_end)
{
  uint32_t         enb_id;
  MessageDef      *msg_p;
  uint32_t         register_enb_m2_pending = 0;

//  msg_p = itti_alloc_new_message (TASK_ENB_APP, 0,MESSAGE_TEST );
//  itti_send_msg_to_task (TASK_M2AP_MCE, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);

  LOG_D(ENB_APP,"Register ...\n");

  for (enb_id = enb_id_start; (enb_id < enb_id_end) ; enb_id++) {
    {
      msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, M2AP_REGISTER_ENB_REQ);
      RCconfig_M2(msg_p, enb_id);
      itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
      register_enb_m2_pending++;
    }
  }

  return register_enb_m2_pending;
}


/***************************  M2AP ENB handle **********************************/
static uint32_t eNB_app_handle_m2ap_mbms_scheduling_information(instance_t instance){
        //uint32_t         mce_id=0;
        MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, M2AP_MBMS_SCHEDULING_INFORMATION_RESP);
        itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
        return 0;
}
static uint32_t eNB_app_handle_m2ap_mbms_session_start_req(instance_t instance){
        //uint32_t         mce_id=0;
        MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, M2AP_MBMS_SESSION_START_RESP);
        itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
        return 0;
}
static uint32_t eNB_app_handle_m2ap_mbms_session_stop_req(instance_t instance){
        //uint32_t         mce_id=0;
        MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, M2AP_MBMS_SESSION_STOP_RESP);
        itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
        return 0;
}
static uint32_t eNB_app_handle_m2ap_mbms_session_update_req(instance_t instance){
        //uint32_t         mce_id=0;
        MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, M2AP_MBMS_SESSION_UPDATE_RESP);
        itti_send_msg_to_task (TASK_M2AP_ENB, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
        return 0;
}

//// end M2AP ENB handle **********************************/

/*------------------------------------------------------------------------------*/

static uint32_t eNB_app_register(uint32_t enb_id_start, uint32_t enb_id_end)
{
  uint32_t         enb_id;
  MessageDef      *msg_p;
  uint32_t         register_enb_pending = 0;

  for (enb_id = enb_id_start; (enb_id < enb_id_end) ; enb_id++) {
    // S1AP registration
    /* note:  there is an implicit relationship between the data structure and the message name */
    msg_p = itti_alloc_new_message(TASK_ENB_APP, 0, S1AP_REGISTER_ENB_REQ);
    RCconfig_S1(msg_p, enb_id);

    if (enb_id == 0)
      RCconfig_gtpu();

    LOG_I(ENB_APP, "default drx %d\n", ((S1AP_REGISTER_ENB_REQ(msg_p)).default_drx));

    LOG_I(ENB_APP, "[eNB %d] eNB_app_register via S1AP for instance %d\n", enb_id, ENB_MODULE_ID_TO_INSTANCE(enb_id));
    itti_send_msg_to_task(TASK_S1AP, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);

    register_enb_pending++;
  }

  return register_enb_pending;
}


/*------------------------------------------------------------------------------*/
static uint32_t eNB_app_register_x2(uint32_t enb_id_start, uint32_t enb_id_end) {
  uint32_t         enb_id;
  MessageDef      *msg_p;
  uint32_t         register_enb_x2_pending = 0;

  for (enb_id = enb_id_start; (enb_id < enb_id_end) ; enb_id++) {
    {
      msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, X2AP_REGISTER_ENB_REQ);
      RCconfig_X2(msg_p, enb_id);
      itti_send_msg_to_task (TASK_X2AP, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
      register_enb_x2_pending++;
    }
  }

  return register_enb_x2_pending;
}

/*------------------------------------------------------------------------------*/
void *eNB_app_task(void *args_p) {
  uint32_t                        enb_nb = RC.nb_inst; 
  uint32_t                        enb_id_start = 0;
  uint32_t                        enb_id_end = enb_id_start + enb_nb;
  uint32_t                        register_enb_pending=0;
  uint32_t                        registered_enb=0;
  long                            enb_register_retry_timer_id;
  uint32_t                        x2_register_enb_pending = 0;
  uint32_t                        x2_registered_enb = 0;
  long                            x2_enb_register_retry_timer_id;
  uint32_t                        m2_register_enb_pending = 0;
  uint32_t                        m2_registered_enb = 0;
  //long                            m2_enb_register_retry_timer_id;
  MessageDef                     *msg_p           = NULL;
  instance_t                      instance;
  int                             result;
  /* for no gcc warnings */
  (void)instance;
  itti_mark_task_ready (TASK_ENB_APP);

  /* Try to register each eNB */
  if (EPC_MODE_ENABLED) {
    register_enb_pending = eNB_app_register(enb_id_start, enb_id_end);
  }

    /* Try to register each eNB with each other */
  if (is_x2ap_enabled()) {
    x2_register_enb_pending = eNB_app_register_x2 (enb_id_start, enb_id_end);
  }

  /* Try to register each eNB with MCE each other */
  if (is_m2ap_eNB_enabled()) {
    //eNB_app_register_MBMS_STA(enb_id_start, enb_id_end);
    m2_register_enb_pending = eNB_app_register_m2 (enb_id_start, enb_id_end);

    //if (timer_setup (5, 0, TASK_ENB_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
    //                           NULL, &m2_enb_register_retry_timer_id) < 0) {
    //}

  }

  LOG_I(ENB_APP,"TASK_ENB_APP is ready\n");

  do {
    // Wait for a message
    itti_receive_msg (TASK_ENB_APP, &msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      LOG_W(ENB_APP, " *** Exiting ENB_APP thread\n");
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(ENB_APP, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;

    case S1AP_REGISTER_ENB_CNF:
        if (EPC_MODE_ENABLED) {
          LOG_I(ENB_APP, "[eNB %ld] Received %s: associated MME %d\n", instance, ITTI_MSG_NAME (msg_p),
                S1AP_REGISTER_ENB_CNF(msg_p).nb_mme);
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
              msg_init_p = itti_alloc_new_message (TASK_ENB_APP, 0, INITIALIZE_MESSAGE);
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
                register_enb_pending = eNB_app_register(enb_id_start, enb_id_end);
              }
            }
          }
        } /* if (EPC_MODE_ENABLED) */

      break;

    case S1AP_DEREGISTERED_ENB_IND:
      if (EPC_MODE_ENABLED) {
  	LOG_W(ENB_APP, "[eNB %ld] Received %s: associated MME %d\n", instance, ITTI_MSG_NAME (msg_p),
  	      S1AP_DEREGISTERED_ENB_IND(msg_p).nb_mme);
  	/* TODO handle recovering of registration */
      }

      break;

    case TIMER_HAS_EXPIRED:
        if (EPC_MODE_ENABLED) {
      LOG_I(ENB_APP, " Received %s: timer_id %ld\n", ITTI_MSG_NAME (msg_p), TIMER_HAS_EXPIRED(msg_p).timer_id);

      if (TIMER_HAS_EXPIRED (msg_p).timer_id == enb_register_retry_timer_id) {
        /* Restart the registration process */
        registered_enb = 0;
        register_enb_pending = eNB_app_register(enb_id_start, enb_id_end);
      }

      if (TIMER_HAS_EXPIRED (msg_p).timer_id == x2_enb_register_retry_timer_id) {
        /* Restart the registration process */
	x2_registered_enb = 0;
        x2_register_enb_pending = eNB_app_register_x2 (enb_id_start, enb_id_end);
      }
        } /* if (EPC_MODE_ENABLED) */

//      if(TIMER_HAS_EXPIRED (msg_p).timer_id == m2_enb_register_retry_timer_id) {
//
//               LOG_I(ENB_APP, " Received %s: timer_id %ld M2 register\n", ITTI_MSG_NAME (msg_p), TIMER_HAS_EXPIRED(msg_p).timer_id);
//               m2_register_enb_pending = eNB_app_register_m2 (enb_id_start, enb_id_end);
//	}

      break;

    case X2AP_DEREGISTERED_ENB_IND:
      LOG_W(ENB_APP, "[eNB %ld] Received %s: associated eNB %d\n", instance, ITTI_MSG_NAME (msg_p),
            X2AP_DEREGISTERED_ENB_IND(msg_p).nb_x2);
      /* TODO handle recovering of registration */
      break;

    case X2AP_REGISTER_ENB_CNF:
      LOG_I(ENB_APP, "[eNB %ld] Received %s: associated eNB %d\n", instance, ITTI_MSG_NAME (msg_p),
            X2AP_REGISTER_ENB_CNF(msg_p).nb_x2);
      DevAssert(x2_register_enb_pending > 0);
      x2_register_enb_pending--;

      /* Check if at least eNB is registered with one target eNB */
      if (X2AP_REGISTER_ENB_CNF(msg_p).nb_x2 > 0) {
        x2_registered_enb++;
      }

      /* Check if all register eNB requests have been processed */
      if (x2_register_enb_pending == 0) {
        if (x2_registered_enb == enb_nb) {
          /* If all eNB are registered, start RRC HO task */
          } else {
          uint32_t x2_not_associated = enb_nb - x2_registered_enb;
          LOG_W(ENB_APP, " %d eNB %s not associated with the target\n",
                x2_not_associated, x2_not_associated > 1 ? "are" : "is");

	  // timer to retry
	  /* Restart the eNB registration process in ENB_REGISTER_RETRY_DELAY seconds */
          if (timer_setup (X2AP_ENB_REGISTER_RETRY_DELAY, 0, TASK_ENB_APP,
			   INSTANCE_DEFAULT, TIMER_ONE_SHOT, NULL,
			   &x2_enb_register_retry_timer_id) < 0) {
            LOG_E(ENB_APP, " Can not start eNB X2AP register: retry timer, use \"sleep\" instead!\n");
            sleep(X2AP_ENB_REGISTER_RETRY_DELAY);
            /* Restart the registration process */
            x2_registered_enb = 0;
            x2_register_enb_pending = eNB_app_register_x2 (enb_id_start, enb_id_end);
          }
        }
      }

      break;

    case M2AP_DEREGISTERED_ENB_IND:
      LOG_W(ENB_APP, "[eNB %ld] Received %s: associated eNB %d\n", instance, ITTI_MSG_NAME (msg_p),
            M2AP_DEREGISTERED_ENB_IND(msg_p).nb_m2);
      /* TODO handle recovering of registration */
      break;

    case M2AP_REGISTER_ENB_CNF:
      LOG_W(ENB_APP, "[eNB %ld] Received %s: associated eNB %d\n", instance, ITTI_MSG_NAME (msg_p),
            M2AP_REGISTER_ENB_CNF(msg_p).nb_m2);
      DevAssert(m2_register_enb_pending > 0);
      m2_register_enb_pending--;

      /* Check if at least eNB is registered with one target eNB */
      if (M2AP_REGISTER_ENB_CNF(msg_p).nb_m2 > 0) {
        m2_registered_enb++;
      }

      /* Check if all register eNB requests have been processed */
      if (m2_register_enb_pending == 0) {
        if (m2_registered_enb == enb_nb) {
          /* If all eNB are registered, start RRC HO task */
          } else {
          uint32_t m2_not_associated = enb_nb - m2_registered_enb;
          LOG_W(ENB_APP, " %d eNB %s not associated with the target\n",
                m2_not_associated, m2_not_associated > 1 ? "are" : "is");

         // timer to retry
         /* Restart the eNB registration process in ENB_REGISTER_RETRY_DELAY seconds */
          //if (timer_setup (X2AP_ENB_REGISTER_RETRY_DELAY, 0, TASK_ENB_APP,
         //               INSTANCE_DEFAULT, TIMER_ONE_SHOT, NULL,
         //               &x2_enb_register_retry_timer_id) < 0) {
          //  LOG_E(ENB_APP, " Can not start eNB X2AP register: retry timer, use \"sleep\" instead!\n");
          //  sleep(X2AP_ENB_REGISTER_RETRY_DELAY);
          //  /* Restart the registration process */
          //  x2_registered_enb = 0;
          //  x2_register_enb_pending = eNB_app_register_x2 (enb_id_start, enb_id_end);
          //}
        }
      }
      break;

    case M2AP_SETUP_RESP:
     LOG_I(ENB_APP,"M2AP_SETUP_RESP RESPONSE received\n");

     // /* Check if all register eNB requests have been processed */
     // if (register_enb_pending == 0) {
     //   if (registered_enb == enb_nb) {
     //     /* If all eNB cells are registered, start L2L1 task */
     //     MessageDef *msg_init_p;

     //     msg_init_p = itti_alloc_new_message (TASK_ENB_APP, 0, INITIALIZE_MESSAGE);
     //     itti_send_msg_to_task (TASK_L2L1, INSTANCE_DEFAULT, msg_init_p);

     //   } else {
     //     LOG_W(ENB_APP, " %d eNB not associated with a MME, retrying registration in %d seconds ...\n",
     //           enb_nb - registered_enb,  ENB_REGISTER_RETRY_DELAY);

     //     /* Restart the eNB registration process in ENB_REGISTER_RETRY_DELAY seconds */
     //     if (timer_setup (ENB_REGISTER_RETRY_DELAY, 0, TASK_ENB_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
     //                      NULL, &enb_register_retry_timer_id) < 0) {
     //       LOG_E(ENB_APP, " Can not start eNB register retry timer, use \"sleep\" instead!\n");

     //       sleep(ENB_REGISTER_RETRY_DELAY);
     //       /* Restart the registration process */
     //       registered_enb = 0;
     //       register_enb_pending = eNB_app_register(enb_id_start, enb_id_end);//, enb_properties_p);
     //     }
     //   }
     // }
     break;

  case M2AP_MBMS_SCHEDULING_INFORMATION:
     LOG_I(ENB_APP,"M2AP_SCHEDULING_INFORMATION received\n");
     eNB_app_handle_m2ap_mbms_scheduling_information(ITTI_MSG_DESTINATION_INSTANCE(msg_p));
     break;
  case M2AP_MBMS_SESSION_START_REQ:
     LOG_I(ENB_APP,"M2AP_MBMS_SESSION_START_REQ received\n");
     eNB_app_handle_m2ap_mbms_session_start_req(ITTI_MSG_DESTINATION_INSTANCE(msg_p));
     break;
  case M2AP_MBMS_SESSION_STOP_REQ:
     LOG_I(ENB_APP,"M2AP_MBMS_SESSION_STOP_REQ received\n");
     eNB_app_handle_m2ap_mbms_session_stop_req(ITTI_MSG_DESTINATION_INSTANCE(msg_p));
     break;
  case M2AP_RESET:
     LOG_I(ENB_APP,"M2AP_RESET received\n");
     break;
  case M2AP_ENB_CONFIGURATION_UPDATE_ACK:
     LOG_I(ENB_APP,"M2AP_ENB_CONFIGURATION_UPDATE_ACK received\n");
     break;
  case M2AP_ENB_CONFIGURATION_UPDATE_FAILURE:
     LOG_I(ENB_APP,"M2AP_ENB_CONFIGURATION_UPDATE_FAILURE received\n");
     break;
  case M2AP_ERROR_INDICATION:
     LOG_I(ENB_APP,"M2AP_MBMS_SESSION_UPDATE_REQ\n");
     eNB_app_handle_m2ap_mbms_session_update_req(ITTI_MSG_DESTINATION_INSTANCE(msg_p));
     break;
  case M2AP_MBMS_SERVICE_COUNTING_REQ:
     LOG_I(ENB_APP,"M2AP_MBMS_SERVICE_COUNTING_REQ\n");
     break;
  case M2AP_MCE_CONFIGURATION_UPDATE:
     LOG_I(ENB_APP,"M2AP_MCE_CONFIGURATION_UPDATE\n");
     break;

    default:
      LOG_E(ENB_APP, "Received unexpected message %s\n", ITTI_MSG_NAME (msg_p));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);

  return NULL;
}
