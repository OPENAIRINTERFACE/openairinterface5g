/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <freeDiameter/freeDiameter-host.h>
#include <freeDiameter/libfdcore.h>

#include "intertask_interface.h"
#include "s6a_defs.h"
#include "s6a_messages.h"

#include "assertions.h"
#include "msc.h"

#if defined(DISABLE_USE_HSS)
# error "Disabling HSS support is currently not supported   \
Reconfigure with --enable-hss option to disable this error"
#endif

static int gnutls_debug = 0;
struct session_handler *ts_sess_hdl;

s6a_fd_cnf_t s6a_fd_cnf;

void *s6a_thread(void *args);
static void fd_gnutls_debug(int level, const char *str);

static void fd_gnutls_debug(int level, const char *str)
{
  S6A_DEBUG("[GTLS] %s", str);
}

void *s6a_thread(void *args)
{
  itti_mark_task_ready(TASK_S6A);
  MSC_START_USE();

  while(1) {
    MessageDef *received_message_p = NULL;
    /* Trying to fetch a message from the message queue.
    * If the queue is empty, this function will block till a
    * message is sent to the task.
    */
    itti_receive_msg(TASK_S6A, &received_message_p);
    DevAssert(received_message_p != NULL);

    switch (ITTI_MSG_ID(received_message_p))
    {
    case S6A_UPDATE_LOCATION_REQ: {
      s6a_generate_update_location(&received_message_p->ittiMsg.s6a_update_location_req);
    } break;
    case S6A_AUTH_INFO_REQ: {
      s6a_generate_authentication_info_req(&received_message_p->ittiMsg.s6a_auth_info_req);
    } break;
    case TERMINATE_MESSAGE: {
      itti_exit_task();
    } break;
    default: {
      S6A_DEBUG("Unkwnon message ID %d: %s\n",
                ITTI_MSG_ID(received_message_p), ITTI_MSG_NAME(received_message_p));
    } break;
    }
    itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
    received_message_p = NULL;
  }
  return NULL;
}

int s6a_init(const mme_config_t *mme_config_p)
{
  int ret;

  S6A_DEBUG("Initializing S6a interface\n");

  memset(&s6a_fd_cnf, 0, sizeof(s6a_fd_cnf_t));

  /*if (strcmp(fd_core_version(), FREE_DIAMETER_MINIMUM_VERSION) != 0) {
  S6A_ERROR("Freediameter version %s found, expecting %s\n", fd_core_version(),
  FREE_DIAMETER_MINIMUM_VERSION);
  return -1;
  } else {
  S6A_DEBUG("Freediameter version %s\n", fd_core_version());
  }*/


  /* Initializing freeDiameter core */
  S6A_DEBUG("Initializing freeDiameter core...\n");
  ret = fd_core_initialize();
  if (ret != 0) {
    S6A_ERROR("An error occurred during freeDiameter core library initialization: %d\n",ret);
    return ret;
  } else {
    S6A_DEBUG("Initializing freeDiameter core done\n");
  }



  S6A_DEBUG("Default ext path: %s\n", DEFAULT_EXTENSIONS_PATH);

  ret = fd_core_parseconf(mme_config_p->s6a_config.conf_file);
  if (ret != 0) {
    S6A_ERROR("An error occurred during fd_core_parseconf file : %s.\n", mme_config_p->s6a_config.conf_file);
    return ret;
  } else {
    S6A_DEBUG("fd_core_parseconf done\n");
  }

  /* Set gnutls debug level ? */
  if (gnutls_debug) {
    gnutls_global_set_log_function((gnutls_log_func)fd_gnutls_debug);
    gnutls_global_set_log_level (gnutls_debug);
    S6A_DEBUG("Enabled GNUTLS debug at level %d", gnutls_debug);
  }

  /* Starting freeDiameter core */
  ret = fd_core_start();
  if (ret != 0) {
    S6A_ERROR("An error occurred during freeDiameter core library start\n");
    return ret;
  } else {
    S6A_DEBUG("fd_core_start done\n");
  }



  ret = fd_core_waitstartcomplete();
  if (ret != 0) {
    S6A_ERROR("An error occurred during fd_core_waitstartcomplete.\n");
    return ret;
  } else {
    S6A_DEBUG("fd_core_waitstartcomplete done\n");
  }

  ret = s6a_fd_init_dict_objs();
  if (ret != 0) {
    S6A_ERROR("An error occurred during s6a_fd_init_dict_objs.\n");
    return ret;
  } else {
    S6A_DEBUG("s6a_fd_init_dict_objs done\n");
  }

  /* Trying to connect to peers */
  CHECK_FCT(s6a_fd_new_peer());

  if (itti_create_task(TASK_S6A, &s6a_thread, NULL) < 0) {
    S6A_ERROR("s6a create task\n");
    return -1;
  }
  S6A_DEBUG("Initializing S6a interface: DONE\n");

  return 0;
}
