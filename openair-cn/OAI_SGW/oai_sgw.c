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
/*! \file oai_sgw.c
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

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

#include "intertask_interface_init.h"

#include "udp_primitives_server.h"
#include "log.h"
#include "timer.h"
#include "s11_sgw.h"
#include "sgw_lite_defs.h"
#include "gtpv1u_sgw_defs.h"

#include "oai_sgw.h"

int main(int argc, char *argv[])
{
  /* Parse the command line for options and set the mme_config accordingly. */
  CHECK_INIT_RETURN(config_parse_opt_line(argc, argv, &mme_config));

  /* Calling each layer init function */
  CHECK_INIT_RETURN(log_init(&mme_config, oai_sgw_log_specific));
  CHECK_INIT_RETURN(itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info, messages_definition_xml, NULL));

  CHECK_INIT_RETURN(udp_init(&mme_config));
  CHECK_INIT_RETURN(s11_sgw_init(&mme_config));
  CHECK_INIT_RETURN(gtpv1u_init(&mme_config));

  CHECK_INIT_RETURN(sgw_lite_init(mme_config.config_file));

  /* Handle signals here */
  itti_wait_tasks_end();

  return 0;
}
