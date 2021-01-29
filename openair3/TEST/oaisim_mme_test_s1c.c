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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <syscall.h>


#include "UTIL/LOG/log_extern.h"
#include "assertions.h"
#include "intertask_interface.h"
#include "sctp_eNB_task.h"
#include "s1ap_eNB.h"
#include "enb_config.h"

#include "oaisim_mme_test_s1c.h"

#   define ENB_REGISTER_RETRY_DELAY 5
//------------------------------------------------------------------------------
static void get_options (int argc, char **argv);
static uint32_t eNB_app_register(const uint32_t enb_id_start, const uint32_t enb_id_end, const Enb_properties_array_t *enb_properties);
static void    *eNB_app_task    (void *args_p);
void     mme_test_s1_start_test(instance_t instance);
//------------------------------------------------------------------------------
static char                    *conf_config_file_name = NULL;
const Enb_properties_array_t   *enb_properties        = NULL;
int16_t                         glog_level            = LOG_INFO;
int16_t                         glog_verbosity        = LOG_MED;
s1c_test_t                      s1c_test = {
	    0, // scenario_index
	    0, // tx_next_message_index
	    0, // rx_next_message_index
	    0  // assoc_id
      };

//------------------------------------------------------------------------------
static void get_options (int argc, char **argv)
//------------------------------------------------------------------------------
{
  int c;


  while ((c = getopt_long (argc, argv, "K:g:G:O:",NULL,NULL)) != -1) {
    switch (c) {
    case 'O':
      conf_config_file_name = optarg;
      break;

    case 'g':
      glog_level=atoi(optarg); // value between 1 - 9
      break;

    case 'G':
      glog_verbosity=atoi(optarg);// value from 0, 0x5, 0x15, 0x35, 0x75
      break;

    default:
      break;
    }
  }

  AssertFatal(conf_config_file_name != NULL,"Please provide a configuration file\n");


}


//------------------------------------------------------------------------------
static uint32_t eNB_app_register(const uint32_t enb_id_start, const uint32_t enb_id_end, const Enb_properties_array_t *enb_properties)
//------------------------------------------------------------------------------
{
  uint32_t         enb_id;
  uint32_t         mme_id;
  MessageDef      *msg_p;
  uint32_t         register_enb_pending = 0;
  char            *str                  = NULL;
  struct in_addr   addr;

  // 1 eNB should be max.
  // 1 eNB should be max.
  for (enb_id = enb_id_start; (enb_id < enb_id_end) ; enb_id++) {
    {
      s1ap_register_enb_req_t *s1ap_register_eNB;

      /* note:  there is an implicit relationship between the data structure and the message name */
      msg_p = itti_alloc_new_message (TASK_ENB_APP, S1AP_REGISTER_ENB_REQ);

      s1ap_register_eNB = &S1AP_REGISTER_ENB_REQ(msg_p);

      /* Some default/random parameters */
      s1ap_register_eNB->eNB_id           = enb_properties->properties[enb_id]->eNB_id;
      s1ap_register_eNB->cell_type        = enb_properties->properties[enb_id]->cell_type;
      s1ap_register_eNB->eNB_name         = enb_properties->properties[enb_id]->eNB_name;
      s1ap_register_eNB->tac              = enb_properties->properties[enb_id]->tac;
      s1ap_register_eNB->mcc              = enb_properties->properties[enb_id]->mcc;
      s1ap_register_eNB->mnc              = enb_properties->properties[enb_id]->mnc;
      s1ap_register_eNB->mnc_digit_length = enb_properties->properties[enb_id]->mnc_digit_length;
      s1ap_register_eNB->default_drx      = enb_properties->properties[enb_id]->pcch_defaultPagingCycle[0];

      s1ap_register_eNB->nb_mme =         enb_properties->properties[enb_id]->nb_mme;
      AssertFatal (s1ap_register_eNB->nb_mme <= S1AP_MAX_NB_MME_IP_ADDRESS, "Too many MME for eNB %d (%d/%d)!", enb_id, s1ap_register_eNB->nb_mme,
                   S1AP_MAX_NB_MME_IP_ADDRESS);

      for (mme_id = 0; mme_id < s1ap_register_eNB->nb_mme; mme_id++) {
        s1ap_register_eNB->mme_ip_address[mme_id].ipv4 = enb_properties->properties[enb_id]->mme_ip_address[mme_id].ipv4;
        s1ap_register_eNB->mme_ip_address[mme_id].ipv6 = enb_properties->properties[enb_id]->mme_ip_address[mme_id].ipv6;
        strncpy (s1ap_register_eNB->mme_ip_address[mme_id].ipv4_address,
                 enb_properties->properties[enb_id]->mme_ip_address[mme_id].ipv4_address,
                 sizeof(s1ap_register_eNB->mme_ip_address[0].ipv4_address));
        strncpy (s1ap_register_eNB->mme_ip_address[mme_id].ipv6_address,
                 enb_properties->properties[enb_id]->mme_ip_address[mme_id].ipv6_address,
                 sizeof(s1ap_register_eNB->mme_ip_address[0].ipv6_address));
      }

      s1ap_register_eNB->enb_ip_address.ipv6 = 0;
      s1ap_register_eNB->enb_ip_address.ipv4 = 1;
      addr.s_addr = enb_properties->properties[enb_id]->enb_ipv4_address_for_S1_MME;
      str = inet_ntoa(addr);
      strcpy(s1ap_register_eNB->enb_ip_address.ipv4_address, str);

      itti_send_msg_to_task (TASK_S1AP, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);

      register_enb_pending++;
    }
  }

  return register_enb_pending;
}

//------------------------------------------------------------------------------
static void *eNB_app_task(void *args_p)
//------------------------------------------------------------------------------
{
  const Enb_properties_array_t   *enb_properties_p  = NULL;
  uint32_t                        enb_nb = 1; /* Default number of eNB is 1 */
  uint32_t                        enb_id_start = 0;
  uint32_t                        enb_id_end = enb_id_start + enb_nb;
  uint32_t                        register_enb_pending;
  uint32_t                        registered_enb;
  long                            enb_register_retry_timer_id;
  MessageDef                     *msg_p           = NULL;
  const char                     *msg_name        = NULL;
  instance_t                      instance;
  int                             result;

  itti_mark_task_ready (TASK_ENB_APP);

  enb_properties_p = enb_config_get();

  AssertFatal (enb_nb <= enb_properties_p->number,
               "Number of eNB is greater than eNB defined in configuration file (%d/%d)!",
               enb_nb, enb_properties_p->number);


  /* Try to register each eNB */
  registered_enb = 0;
  register_enb_pending = eNB_app_register (enb_id_start, enb_id_end, enb_properties_p);

  do {
    // Wait for a message
    itti_receive_msg (TASK_ENB_APP, &msg_p);

    msg_name = ITTI_MSG_NAME (msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(ENB_APP, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;


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
          mme_test_s1_start_test(instance);


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
            register_enb_pending = eNB_app_register (enb_id_start, enb_id_end, enb_properties_p);
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
      LOG_I(ENB_APP, " Received %s: timer_id %d\n", msg_name, TIMER_HAS_EXPIRED(msg_p).timer_id);

      if (TIMER_HAS_EXPIRED (msg_p).timer_id == enb_register_retry_timer_id) {
        /* Restart the registration process */
        registered_enb = 0;
        register_enb_pending = eNB_app_register (enb_id_start, enb_id_end, enb_properties_p);
      }

      break;

    default:
      LOG_E(ENB_APP, "Received unexpected message %s\n", msg_name);
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);

  return NULL;
}


//------------------------------------------------------------------------------
void mme_test_s1_start_test(instance_t instance)
//------------------------------------------------------------------------------
{
	s1ap_eNB_itti_send_sctp_data_req(instance,
			s1c_test.assoc_id,
			s1ap_scenarios[s1c_test.scenario_index][s1c_test.tx_next_message_index].buffer,
			s1ap_scenarios[s1c_test.scenario_index][s1c_test.tx_next_message_index].buf_len,
			s1ap_scenarios[s1c_test.scenario_index][s1c_test.tx_next_message_index].sctp_stream_id);
}


//------------------------------------------------------------------------------
void mme_test_s1_notify_sctp_data_ind(uint32_t assoc_id, int32_t stream, const uint8_t * const data, const uint32_t data_length)
//------------------------------------------------------------------------------
{

}


//------------------------------------------------------------------------------
int main( int argc, char **argv )
//------------------------------------------------------------------------------
{
  // initialize the log (see log.h for details)
  logInit();

  get_options (argc, argv); //Command-line options
  set_glog(glog_level, glog_verbosity);
  log_set_instance_type (LOG_INSTANCE_ENB);

  /* Read eNB configuration file */
  enb_properties = enb_config_init(conf_config_file_name);

  itti_init(TASK_MAX,  tasks_info);

  itti_wait_ready(1);

  if (itti_create_task (TASK_SCTP, sctp_eNB_task, NULL) < 0) {
    LOG_E(SCTP, "Create task for SCTP failed\n");
    return -1;
  }

  if (itti_create_task (TASK_S1AP, s1ap_eNB_task, NULL) < 0) {
    LOG_E(S1AP, "Create task for S1AP failed\n");
    return -1;
  }

  if (itti_create_task (TASK_ENB_APP, eNB_app_task, NULL) < 0) {
    LOG_E(S1AP, "Create task for S1AP failed\n");
    return -1;
  }

  itti_wait_ready(0);



  sleep(30);

  logClean();
  return 0;
}
