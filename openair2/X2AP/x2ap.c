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


