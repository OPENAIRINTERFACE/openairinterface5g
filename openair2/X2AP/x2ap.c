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
      X2AP_WARN(" *** Exiting X2AP thread\n");
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


