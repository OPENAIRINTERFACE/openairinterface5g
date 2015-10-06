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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "intertask_interface.h"

#include "x2ap.h"

#include "assertions.h"
#include "conversions.h"


void *x2ap_task(void *arg)
{
  MessageDef *received_msg = NULL;
  int         result;

  X2AP_DEBUG("Starting X2AP layer\n");

  x2ap_prepare_internal_data();

  itti_mark_task_ready(TASK_X2AP);

  while (1) {
    itti_receive_msg(TASK_X2AP, &received_msg);

    switch (ITTI_MSG_ID(received_msg)) {
    case TERMINATE_MESSAGE:
      itti_exit_task();
      break;

    default:
      X2AP_ERROR("Received unhandled message: %d:%s\n",
                 ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);

    received_msg = NULL;
  }

  return NULL;
}


