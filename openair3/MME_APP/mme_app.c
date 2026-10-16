/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
                                mme_app.c
                             -------------------
  AUTHOR  : Javier Morgade
  COMPANY : VICOMTECH Spain
  EMAIL   : javier.morgade@ieee.org
*/

#include <string.h>
#include <stdio.h>

#include "mme_app.h"
#include "mme_config.h"
#include "assertions.h"
#include "common/ran_context.h"
#include "executables/lte-softmodem.h"

#include "common/utils/LOG/log.h"

# include "intertask_interface.h"
#   include "m2ap_eNB.h"
#   include "m3ap_MME.h"
#   include "m2ap_messages_types.h"
//#   include "m3ap_eNB.h"
#   include "m3ap_messages_types.h"
#   define X2AP_ENB_REGISTER_RETRY_DELAY   10

#include "openair1/PHY/INIT/phy_init.h"

extern RAN_CONTEXT_t RC;

#   define MCE_REGISTER_RETRY_DELAY 10

#include "executables/lte-softmodem.h"

static uint32_t MME_app_handle_m3ap_setup_req(instance_t instance){
	
  	//uint32_t         mce_id=0;
  	MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_MME_APP, 0, M3AP_SETUP_RESP);
        itti_send_msg_to_task (TASK_M3AP_MME, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
	
	return 0;
}

static uint32_t MME_app_handle_m3ap_session_start_resp(instance_t instance){
	
  
	return 0;
}

static uint32_t MME_app_send_m3ap_session_start_req(instance_t instance){
	
  	//uint32_t         mce_id=0;
  	MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_MME_APP, 0, M3AP_MBMS_SESSION_START_REQ);
        itti_send_msg_to_task (TASK_M3AP_MME, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
	
	return 0;
}

static uint32_t MME_app_send_m3ap_session_update_req(instance_t instance){
	
  	//uint32_t         mce_id=0;
  	MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_MME_APP, 0, M3AP_MBMS_SESSION_UPDATE_REQ);
        itti_send_msg_to_task (TASK_M3AP_MME, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
	
	return 0;
}
/*------------------------------------------------------------------------------*/
void *MME_app_task(void *args_p)
{
  long                            m3_mme_register_session_start_timer_id;
  MessageDef                     *msg_p           = NULL;
  instance_t                      instance;
  int                             result;
  /* for no gcc warnings */
  (void)instance;
  itti_mark_task_ready (TASK_MME_APP);

  /* Try to register each MCE */
  // This assumes that node_type of all RRC instances is the same
  if (!IS_SOFTMODEM_NOS1) {
    //register_mce_pending = MCE_app_register(RC.rrc[0]->node_type, mce_id_start, mce_id_end);
  }

    /* Try to register each MCE with each other */
 // if (is_x2ap_enabled() && !NODE_IS_DU(RC.rrc[0]->node_type)) {
 //   x2_register_enb_pending = MCE_app_register_x2 (enb_id_start, enb_id_end);
 // }
  if ( is_m3ap_MME_enabled() ){
 	RCconfig_MME();
  }
 // /* Try to register each MCE with MCE each other */
 //if (is_m3ap_MME_enabled() /*&& !NODE_IS_DU(RC.rrc[0]->node_type)*/) {
   //m2_register_mce_pending = MCE_app_register_m2 (mce_id_start, mce_id_end);
 //}

  do {
    // Wait for a message
    itti_receive_msg (TASK_MME_APP, &msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      LOG_W(MME_APP, " *** Exiting MME_APP thread\n");
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(MME_APP, "MME_APP Received %s\n", ITTI_MSG_NAME(msg_p));
      break;

    case M3AP_REGISTER_MCE_CNF: //M3AP_REGISTER_MCE_CNF debería
      break;

    case M3AP_DEREGISTERED_MCE_IND: //M3AP_DEREGISTERED_MCE_IND debería
      if (!IS_SOFTMODEM_NOS1) {
        LOG_W(MME_APP,
              "[MCE %ld] Received %s: associated MME %d\n",
              instance,
              ITTI_MSG_NAME(msg_p),
              M3AP_DEREGISTERED_MCE_IND(msg_p).nb_mme);
        /* TODO handle recovering of registration */
      }

      break;

    case TIMER_HAS_EXPIRED:
      if (TIMER_HAS_EXPIRED(msg_p).timer_id == m3_mme_register_session_start_timer_id) {
        MME_app_send_m3ap_session_start_req(0);
	}

      break;
	case M3AP_RESET:
	LOG_I(MME_APP,"Received M3AP_RESET message %s\n",ITTI_MSG_NAME(msg_p));	
	break;

	case M3AP_SETUP_REQ:
	LOG_I(MME_APP,"Received M3AP_REQ message %s\n",ITTI_MSG_NAME(msg_p));	
	MME_app_handle_m3ap_setup_req(0);
	if(timer_setup(1,0,TASK_MME_APP,INSTANCE_DEFAULT,TIMER_ONE_SHOT,NULL,&m3_mme_register_session_start_timer_id)<0){
	}
	//MME_app_send_m3ap_session_start_req(0);
	break;

	case M3AP_MBMS_SESSION_START_RESP:
	LOG_I(MME_APP,"Received M3AP_MBMS_SESSION_START_RESP message %s\n",ITTI_MSG_NAME(msg_p));
	MME_app_handle_m3ap_session_start_resp(0);
	//MME_app_send_m3ap_session_stop_req(0);
	break;

	case M3AP_MBMS_SESSION_STOP_RESP:
	LOG_I(MME_APP,"Received M3AP_MBMS_SESSION_STOP_RESP message %s\n",ITTI_MSG_NAME(msg_p));
	MME_app_send_m3ap_session_update_req(0);
	break;

	case M3AP_MBMS_SESSION_UPDATE_RESP:
	LOG_I(MME_APP,"Received M3AP_MBMS_SESSION_UPDATE_RESP message %s\n",ITTI_MSG_NAME(msg_p));
	// trigger something new here !!!!!!
	break;
 
	case M3AP_MBMS_SESSION_UPDATE_FAILURE:
	LOG_I(MME_APP,"Received M3AP_MBMS_SESSION_UPDATE_FAILURE message %s\n",ITTI_MSG_NAME(msg_p));	
	break;
 
	case M3AP_MCE_CONFIGURATION_UPDATE:
	LOG_I(MME_APP,"Received M3AP_MCE_CONFIGURATION_UPDATE message %s\n",ITTI_MSG_NAME(msg_p)); 	   
	break;
 
    default:
      LOG_E(MME_APP, "Received unexpected message %s\n", ITTI_MSG_NAME (msg_p));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);

  return NULL;
}
