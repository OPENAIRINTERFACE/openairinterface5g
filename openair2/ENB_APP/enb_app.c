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

#include "common/utils/LOG/log.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# if defined(ENABLE_USE_MME)
#   include "s1ap_eNB.h"
#   include "sctp_eNB_task.h"
#   include "gtpv1u_eNB_task.h"
/* temporary warning removale while implementing noS1 */
/* as config option                                   */
#   else
#     ifdef EPC_MODE_ENABLED
#       undef  EPC_MODE_ENABLED
#     endif
#     define EPC_MODE_ENABLED 0
#   endif
#   include "x2ap_eNB.h"
#   include "x2ap_messages_types.h"
#   define X2AP_ENB_REGISTER_RETRY_DELAY   10

#include "openair1/PHY/INIT/phy_init.h"
extern unsigned char NB_eNB_INST;
#endif

extern RAN_CONTEXT_t RC;

#if defined(ENABLE_ITTI)

/*------------------------------------------------------------------------------*/
# if defined(ENABLE_USE_MME)
#   define ENB_REGISTER_RETRY_DELAY 10
# endif

#include "targets/RT/USER/lte-softmodem.h"

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
      /* note:  there is an implicit relationship between the data structure and the message name */
      msg_p = itti_alloc_new_message (TASK_ENB_APP, S1AP_REGISTER_ENB_REQ);

      RCconfig_S1(msg_p, enb_id);

      if (enb_id == 0) RCconfig_gtpu();

      LOG_I(ENB_APP,"default drx %d\n",((S1AP_REGISTER_ENB_REQ(msg_p)).default_drx));

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
static uint32_t eNB_app_register_x2(uint32_t enb_id_start, uint32_t enb_id_end)
{
  uint32_t         enb_id;
  MessageDef      *msg_p;
  uint32_t         register_enb_x2_pending = 0;

  for (enb_id = enb_id_start; (enb_id < enb_id_end) ; enb_id++) {

    {

      msg_p = itti_alloc_new_message (TASK_ENB_APP, X2AP_REGISTER_ENB_REQ);

      RCconfig_X2(msg_p, enb_id);

      itti_send_msg_to_task (TASK_X2AP, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);

      register_enb_x2_pending++;
    }
  }

  return register_enb_x2_pending;
}

/*------------------------------------------------------------------------------*/
void *eNB_app_task(void *args_p)
{
#if defined(ENABLE_ITTI)
  uint32_t                        enb_nb = RC.nb_inst; 
  uint32_t                        enb_id_start = 0;
  uint32_t                        enb_id_end = enb_id_start + enb_nb;
# if defined(ENABLE_USE_MME)
  uint32_t                        register_enb_pending=0;
  uint32_t                        registered_enb;
  long                            enb_register_retry_timer_id;
# endif
  uint32_t                        x2_register_enb_pending;
  uint32_t                        x2_registered_enb;
  long                            x2_enb_register_retry_timer_id;
  uint32_t                        enb_id;
  MessageDef                     *msg_p           = NULL;
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
#else
  /* Start L2L1 task */
    msg_p = itti_alloc_new_message(TASK_ENB_APP, INITIALIZE_MESSAGE);
    itti_send_msg_to_task(TASK_L2L1, INSTANCE_DEFAULT, msg_p);
#endif

  /* Try to register each eNB with each other */
  x2_registered_enb = 0;
  x2_register_enb_pending = eNB_app_register_x2 (enb_id_start, enb_id_end);

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

    case SOFT_RESTART_MESSAGE:
      handle_reconfiguration(instance);
      break;

    case S1AP_REGISTER_ENB_CNF:
# if defined(ENABLE_USE_MME)
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
  	      register_enb_pending = eNB_app_register (enb_id_start, enb_id_end);//, enb_properties_p);
  	    }
  	  }
  	}
#endif
      break;

    case S1AP_DEREGISTERED_ENB_IND:
      if (EPC_MODE_ENABLED) {
  	LOG_W(ENB_APP, "[eNB %d] Received %s: associated MME %d\n", instance, ITTI_MSG_NAME (msg_p),
  	      S1AP_DEREGISTERED_ENB_IND(msg_p).nb_mme);

  	/* TODO handle recovering of registration */
      }
      break;

    case TIMER_HAS_EXPIRED:
# if defined(ENABLE_USE_MME)
      LOG_I(ENB_APP, " Received %s: timer_id %ld\n", ITTI_MSG_NAME (msg_p), TIMER_HAS_EXPIRED(msg_p).timer_id);

      if (TIMER_HAS_EXPIRED (msg_p).timer_id == enb_register_retry_timer_id) {
        /* Restart the registration process */
        registered_enb = 0;
        register_enb_pending = eNB_app_register (enb_id_start, enb_id_end);//, enb_properties_p);
      }

      if (TIMER_HAS_EXPIRED (msg_p).timer_id == x2_enb_register_retry_timer_id) {
        /* Restart the registration process */
	x2_registered_enb = 0;
        x2_register_enb_pending = eNB_app_register_x2 (enb_id_start, enb_id_end);
      }
# endif
      break;

    case X2AP_DEREGISTERED_ENB_IND:
      LOG_W(ENB_APP, "[eNB %d] Received %s: associated eNB %d\n", instance, ITTI_MSG_NAME (msg_p),
            X2AP_DEREGISTERED_ENB_IND(msg_p).nb_x2);

      /* TODO handle recovering of registration */
      break;

    case X2AP_REGISTER_ENB_CNF:
      LOG_I(ENB_APP, "[eNB %d] Received %s: associated eNB %d\n", instance, ITTI_MSG_NAME (msg_p),
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

	}else {
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

void handle_reconfiguration(module_id_t mod_id)
{
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  flexran_agent_info_t *flexran = RC.flexran[mod_id];

  LOG_I(ENB_APP, "lte-softmodem soft-restart requested\n");

  if (ENB_WAIT == flexran->node_ctrl_state) {
    /* this is already waiting, just release */
    pthread_mutex_lock(&flexran->mutex_node_ctrl);
    flexran->node_ctrl_state = ENB_NORMAL_OPERATION;
    pthread_mutex_unlock(&flexran->mutex_node_ctrl);
    pthread_cond_signal(&flexran->cond_node_ctrl);
    return;
  }

  if (stop_L1L2(mod_id) < 0) {
    LOG_E(ENB_APP, "can not stop lte-softmodem, aborting restart\n");
    return;
  }

  /* node_ctrl_state should have value ENB_MAKE_WAIT only if this method is not
   * executed by the FlexRAN thread */
  if (ENB_MAKE_WAIT == flexran->node_ctrl_state) {
    LOG_I(ENB_APP, " * eNB %d: Waiting for FlexRAN RTController command *\n", mod_id);
    pthread_mutex_lock(&flexran->mutex_node_ctrl);
    flexran->node_ctrl_state = ENB_WAIT;
    while (ENB_NORMAL_OPERATION != flexran->node_ctrl_state)
      pthread_cond_wait(&flexran->cond_node_ctrl, &flexran->mutex_node_ctrl);
    pthread_mutex_unlock(&flexran->mutex_node_ctrl);
  }

  if (restart_L1L2(mod_id) < 0) {
    LOG_E(ENB_APP, "can not restart, killing lte-softmodem\n");
    exit_fun("can not restart L1L2, killing lte-softmodem");
    return;
  }

  clock_gettime(CLOCK_MONOTONIC, &end);
  end.tv_sec -= start.tv_sec;
  if (end.tv_nsec >= start.tv_nsec) {
    end.tv_nsec -= start.tv_nsec;
  } else {
    end.tv_sec -= 1;
    end.tv_nsec = end.tv_nsec - start.tv_nsec + 1000000000;
  }
  LOG_I(ENB_APP, "lte-softmodem restart succeeded in %ld.%ld s\n", end.tv_sec, end.tv_nsec / 1000000);
}
