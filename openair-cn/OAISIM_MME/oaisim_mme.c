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

#include "assertions.h"
#include "mme_config.h"
#include "gtpv1u_sgw_defs.h"

#include "intertask_interface_init.h"

#include "sctp_primitives_server.h"
#include "udp_primitives_server.h"
#include "s1ap_mme.h"
#include "log.h"
#include "timer.h"
#include "sgw_lite_defs.h"
#include "mme_app_extern.h"
#include "nas_defs.h"
#include "s11_mme.h"

/* FreeDiameter headers for support of S6A interface */
#include <freeDiameter/freeDiameter-host.h>
#include <freeDiameter/libfdcore.h>
#include "s6a_defs.h"

#include "oaisim_mme.h"

int main(int argc, char *argv[])
{
  /* Parse the command line for options and set the mme_config accordingly. */
  CHECK_INIT_RETURN(config_parse_opt_line(argc, argv, &mme_config));

  /* Calling each layer init function */
  CHECK_INIT_RETURN(log_init(&mme_config, oai_mme_log_specific));
  CHECK_INIT_RETURN(itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info, messages_definition_xml, NULL));

  CHECK_INIT_RETURN(nas_init(&mme_config));
  CHECK_INIT_RETURN(sctp_init(&mme_config));
  CHECK_INIT_RETURN(udp_init(&mme_config));
  CHECK_INIT_RETURN(s11_mme_init(&mme_config));
  CHECK_INIT_RETURN(s1ap_mme_init(&mme_config));
  //     if (gtpv1u_init(&mme_config) < 0) return -1;
  CHECK_INIT_RETURN(mme_app_init(&mme_config));

  CHECK_INIT_RETURN(s6a_init(&mme_config));

  /* Handle signals here */
  itti_wait_tasks_end();

  return 0;
}
