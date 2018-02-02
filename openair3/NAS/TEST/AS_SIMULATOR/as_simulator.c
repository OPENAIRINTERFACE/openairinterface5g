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

/*****************************************************************************
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    as_simulator.c

Version   0.1

Date    2013/04/11

Product   Access-Stratum sublayer simulator

Subsystem Main process

Author    Frederic Maurel

Description Implements the Access-Stratum simulator operating between
    the Non-Access-Stratum running in the UE at the user side
    and the Non-Access-Stratum running in the MME at the network
    side for testing purpose.

*****************************************************************************/

#include "as_simulator_parser.h"

#include "as_data.h"
#include "as_process.h"

#include "commonDef.h"
#include "socket.h"

#include "nas_log.h"

#include <stdio.h>  // printf, perror, snprintf
#include <errno.h>  // errno
#include <netdb.h>  // gai_strerror
#include <stdlib.h> // exit
#include <string.h> // memset
#include <poll.h> // poll
#include <signal.h> // sigaction
#include <pthread.h>

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

#define SLEEP_TIMEOUT 1000  /* 1 second */

#define AS_SIMULATOR_NO_TRANSFER  (0)
#define AS_SIMULATOR_TRANSFER_TO_UE (1 << 0)
#define AS_SIMULATOR_TRANSFER_TO_MME  (1 << 1)

/*
 * Direction of the message to be transfered to the UE NAS process
 */
static uint8_t _as_simulator_ue_transfer = AS_SIMULATOR_NO_TRANSFER;

/*
 * Direction of the message to be transfered to the MME NAS process
 */
static uint8_t _as_simulator_mme_transfer = AS_SIMULATOR_NO_TRANSFER;

/*
 * Size of the message to be transfered to the UE NAS process
 */
static int _as_simulator_ue_size = 0;

/*
 * Size of the message to be transfered to the MME NAS process
 */
static int _as_simulator_mme_size = 0;

/*
 * String buffer used to send/receive messages to/from the UE NAS process
 */
#define AS_SIMULATOR_UE_BUFFER_SIZE   1024
static char _as_simulator_ue_buffer [AS_SIMULATOR_UE_BUFFER_SIZE];

/*
 * String buffer used to send/receive messages to/from the MME NAS process
 */
#define AS_SIMULATOR_MME_BUFFER_SIZE    1024
static char _as_simulator_mme_buffer [AS_SIMULATOR_MME_BUFFER_SIZE];

/*
 * The connection endpoint used for communication with the UE NAS process
 */
static socket_id_t * _as_simulator_ue_sid = NULL;

/*
 * The connection endpoint used for communication with the MME NAS process
 */
static socket_id_t * _as_simulator_mme_sid = NULL;

/*
 * UE NAS process connection manager's running indicator
 */
static int _as_simulator_ue_is_running = FALSE;

/*
 * MME NAS process connection manager's running indicator
 */
static int _as_simulator_mme_is_running = FALSE;

static int _set_signal_handler(int signal, void (handler)(int));
static void _signal_handler(int signal_number);

static void* _as_simulator_ue_mngr(void*);
static void* _as_simulator_mme_mngr(void*);

static int _as_simulator_ue_process(int msg_id, as_message_t* msg);
static int _as_simulator_mme_process(int msg_id, as_message_t* msg);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * -----------------------------------------------------------------------------
 *        RRC simulator main process
 * -----------------------------------------------------------------------------
 */
int main (int argc, const char* argv[])
{
  /*
   * Get the command line options
   */
  if ( as_simulator_parser_get_options(argc, argv) != RETURNok ) {
    as_simulator_parser_print_usage();
    exit(EXIT_FAILURE);
  }

  const char* uhost = as_simulator_parser_get_uhost();
  const char* uport = as_simulator_parser_get_uport();
  const char* mhost = as_simulator_parser_get_mhost();
  const char* mport = as_simulator_parser_get_mport();

  nas_log_init(0x2f);

  /*
   * Initialize the communication channel to the UE NAS process
   */
  _as_simulator_ue_sid = socket_udp_open(SOCKET_SERVER, uhost, uport);

  if (_as_simulator_ue_sid == NULL) {
    const char* error = ( (errno < 0) ?
                          gai_strerror(errno) : strerror(errno) );
    printf("ERROR\t: socket_udp_open() failed: %s\n", error);
    exit(EXIT_FAILURE);
  }

  printf("INFO\t: %s - The RRC Simulator is now connected to %s/%s (%d)\n",
         __FUNCTION__, uhost, uport, socket_get_fd(_as_simulator_ue_sid));

  /*
   * Initialize the communication channel to the MME NAS process
   */
  _as_simulator_mme_sid = socket_udp_open(SOCKET_CLIENT, mhost, mport);

  if (_as_simulator_mme_sid == NULL) {
    const char* error = ( (errno < 0) ?
                          gai_strerror(errno) : strerror(errno) );
    printf("ERROR\t: socket_udp_open() failed: %s\n", error);
    socket_close(_as_simulator_ue_sid);
    exit(EXIT_FAILURE);
  }

  printf("INFO\t: %s - The RRC Simulator is now connected to %s/%s (%d)\n",
         __FUNCTION__, mhost, mport, socket_get_fd(_as_simulator_mme_sid));

  MSCGEN("[MSC_NEW][%s][AS=%s]\n", getTime(), _as_id);

  /*
   * Set up signal handler
   */
  (void) _set_signal_handler(SIGINT, _signal_handler);
  (void) _set_signal_handler(SIGTERM, _signal_handler);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  /*
   * Start thread use to manage the connection endpoint with the
   * UE NAS process
   */
  pthread_t ue_mngr;

  if ( pthread_create (&ue_mngr, &attr, _as_simulator_ue_mngr, NULL) != 0 ) {
    perror("ERROR\t: Failed to create the UE management thread\n");
    socket_close(_as_simulator_ue_sid);
    socket_close(_as_simulator_mme_sid);
    exit(EXIT_FAILURE);
  }

  /*
   * Start thread use to manage the connection endpoint with the
   * MME NAS process
   */
  pthread_t mme_mngr;

  if ( pthread_create (&mme_mngr, &attr, _as_simulator_mme_mngr, NULL) != 0 ) {
    perror("ERROR\t: Failed to create the MME management thread\n");
    socket_close(_as_simulator_ue_sid);
    socket_close(_as_simulator_mme_sid);
    exit(EXIT_FAILURE);
  }

  pthread_attr_destroy(&attr);

  /*
   * Suspend execution of the main process until connection
   * managers are running
   */
  poll(NULL, 0, SLEEP_TIMEOUT);

  while (_as_simulator_ue_is_running && _as_simulator_mme_is_running) {
    poll(NULL, 0, SLEEP_TIMEOUT);
  }

  /*
   * Termination cleanup
   */
  printf("INFO\t: %s - Closing UE's connection endpoint %d\n",
         __FUNCTION__, socket_get_fd(_as_simulator_ue_sid));
  socket_close(_as_simulator_ue_sid);
  printf("INFO\t: %s - Closing MME's connection endpoint %d\n",
         __FUNCTION__, socket_get_fd(_as_simulator_mme_sid));
  socket_close(_as_simulator_mme_sid);

  printf("INFO\t: %s - RRC simulator exited\n", __FUNCTION__);
  exit(EXIT_SUCCESS);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * -----------------------------------------------------------------------------
 *      Signal handler setup function
 * -----------------------------------------------------------------------------
 */
static int _set_signal_handler(int signal, void (handler)(int))
{
  struct sigaction act;

  /* Initialize signal set */
  (void) memset (&act, 0, sizeof (act));
  (void) sigfillset (&act.sa_mask);
  (void) sigdelset (&act.sa_mask, SIGHUP);
  (void) sigdelset (&act.sa_mask, SIGINT);
  (void) sigdelset (&act.sa_mask, SIGTERM);
  (void) sigdelset (&act.sa_mask, SIGILL);
  (void) sigdelset (&act.sa_mask, SIGTRAP);
  (void) sigdelset (&act.sa_mask, SIGIOT);
#ifndef LINUX
  (void) sigdelset (&act.sa_mask, SIGEMT);
#endif
  (void) sigdelset (&act.sa_mask, SIGFPE);
  (void) sigdelset (&act.sa_mask, SIGBUS);
  (void) sigdelset (&act.sa_mask, SIGSEGV);
  (void) sigdelset (&act.sa_mask, SIGSYS);

  /* Initialize signal handler */
  act.sa_handler = handler;

  if ( sigaction (signal, &act, 0) < 0 ) {
    return RETURNerror;
  }

  return RETURNok;
}

/*
 * -----------------------------------------------------------------------------
 *          Signal handler
 * -----------------------------------------------------------------------------
 */
static void _signal_handler(int signal_number)
{
  printf("\nWARNING\t: %s - Signal %d received\n", __FUNCTION__,
         signal_number);
  _as_simulator_ue_is_running = FALSE;
  _as_simulator_mme_is_running = FALSE;
}

/*
 * -----------------------------------------------------------------------------
 *        UE connection manager
 * -----------------------------------------------------------------------------
 */
static void* _as_simulator_ue_mngr(void* args)
{
  int msg_id;
  as_message_t msg;

  _as_simulator_ue_is_running = TRUE;

  printf("INFO\t: %s - UE connection manager started\n", __FUNCTION__);

  MSCGEN("[MSC_NEW][%s][NAS-UE=%s]\n", getTime(), _ue_id);

  /* Receiving loop */
  while (_as_simulator_ue_is_running) {
    int rc, rbytes, sbytes;

    /* Receive message from the UE NAS process */
    rbytes = socket_recv(_as_simulator_ue_sid, _as_simulator_ue_buffer,
                         AS_SIMULATOR_UE_BUFFER_SIZE);

    if (rbytes == RETURNerror) {
      perror("ERROR\t: socket_recv() failed");
      break;
    }

    /* Decode the received message */
    msg_id = as_message_decode(_as_simulator_ue_buffer, &msg, rbytes);

    if (msg_id == RETURNerror) {
      printf("WARNING\t: %s - Failed to decode AS message\n",
             __FUNCTION__);
      continue;
    }

    /* Reset the size of the message to be transfered */
    _as_simulator_ue_size = 0;
    _as_simulator_mme_size = 0;

    /* Process the received message */
    rc = _as_simulator_ue_process(msg_id, &msg);

    if (rc != RETURNok) {
      printf("WARNING\t: %s - Failed to process AS message (0x%x)\n",
             __FUNCTION__, msg_id);
      continue;
    }

    if (_as_simulator_ue_size > 0) {
      /* Return the response message to the UE NAS process */
      sbytes = socket_send(_as_simulator_ue_sid,
                           _as_simulator_ue_buffer,
                           _as_simulator_ue_size);

      if (sbytes == RETURNerror) {
        perror("ERROR\t: socket_send() failed");
        break;
      }
    }

    if (_as_simulator_mme_size > 0) {
      /* Forward the received message to the MME NAS process */
      sbytes = socket_send(_as_simulator_mme_sid,
                           _as_simulator_mme_buffer,
                           _as_simulator_mme_size);

      if (sbytes == RETURNerror) {
        perror("ERROR\t: socket_send() failed");
        break;
      }
    }
  }

  /* Close the connection to the network sublayer */
  _as_simulator_ue_is_running = FALSE;
  printf("ERROR\t: The UE connection manager exited\n");
  return (NULL);
}

/*
 * -----------------------------------------------------------------------------
 *        MME connection manager
 * -----------------------------------------------------------------------------
 */
static void* _as_simulator_mme_mngr(void* args)
{
  int msg_id;
  as_message_t msg;

  _as_simulator_mme_is_running = TRUE;

  printf("INFO\t: %s - MME connection manager started\n", __FUNCTION__);

  MSCGEN("[MSC_NEW][%s][NAS-MME=%s]\n", getTime(), _mme_id);

  /* Receiving loop */
  while (_as_simulator_mme_is_running) {
    int rc, rbytes, sbytes;

    /* Receive message from the MME NAS process */
    rbytes = socket_recv(_as_simulator_mme_sid, _as_simulator_mme_buffer,
                         AS_SIMULATOR_MME_BUFFER_SIZE);

    if (rbytes == RETURNerror) {
      perror("ERROR\t: socket_recv() failed");
      break;
    }

    /* Decode the received message */
    msg_id = as_message_decode(_as_simulator_mme_buffer, &msg, rbytes);

    if (msg_id == RETURNerror) {
      printf("ERROR\t: %s - as_message_decode() failed\n", __FUNCTION__);
      continue;
    }

    /* Reset the size of the message to be transfered */
    _as_simulator_ue_size = 0;
    _as_simulator_mme_size = 0;

    /* Process the received message */
    rc = _as_simulator_mme_process(msg_id, &msg);

    if (rc != RETURNok) {
      printf("WARNING\t: %s - Failed to process AS message (0x%x)\n",
             __FUNCTION__, msg_id);
      continue;
    }

    if (_as_simulator_mme_size > 0) {
      /* Return the response message to the MME NAS process */
      sbytes = socket_send(_as_simulator_mme_sid,
                           _as_simulator_mme_buffer,
                           _as_simulator_mme_size);

      if (sbytes == RETURNerror) {
        perror("ERROR\t: socket_send() failed");
        break;
      }
    }

    if (_as_simulator_ue_size > 0) {
      /* Forward the received message to the UE NAS process */
      sbytes = socket_send(_as_simulator_ue_sid,
                           _as_simulator_ue_buffer,
                           _as_simulator_ue_size);

      if (sbytes == RETURNerror) {
        perror("ERROR\t: socket_send() failed");
        break;
      }
    }
  }

  /* Close the connection to the network sublayer */
  _as_simulator_mme_is_running = FALSE;
  printf("ERROR\t: %s - The MME connection manager exited\n", __FUNCTION__);
  return (NULL);
}

/*
 * -----------------------------------------------------------------------------
 *  Functions used to process messages received from the UE NAS process
 * -----------------------------------------------------------------------------
 */
static int _as_simulator_ue_process(int msg_id, as_message_t* msg)
{
  int rc = RETURNok;

  as_message_t cnf;
  as_message_t ind;

  printf("\nINFO\t: %s - Received AS message 0x%x from UE NAS process\n",
         __FUNCTION__, msg_id);

  _as_simulator_ue_transfer = AS_SIMULATOR_NO_TRANSFER;

  memset(&cnf, 0, sizeof(as_message_t));
  memset(&ind, 0, sizeof(as_message_t));

  switch (msg_id) {
  case AS_CELL_INFO_REQ:
    cnf.msgID = process_cell_info_req(msg_id,
                                      &msg->msg.cell_info_req,
                                      &cnf.msg.cell_info_cnf);
    /* Return confirm message to the UE */
    _as_simulator_ue_transfer |= AS_SIMULATOR_TRANSFER_TO_UE;
    break;

  case AS_NAS_ESTABLISH_REQ:
    ind.msgID = process_nas_establish_req(msg_id,
                                          &msg->msg.nas_establish_req,
                                          &ind.msg.nas_establish_ind,
                                          &cnf.msg.nas_establish_cnf);

    if (ind.msgID) {
      /* Forward indication message to the MME */
      _as_simulator_ue_transfer |= AS_SIMULATOR_TRANSFER_TO_MME;
    }

    /* Return confirm message to the UE */
    cnf.msgID = AS_NAS_ESTABLISH_CNF;
    _as_simulator_ue_transfer |= AS_SIMULATOR_TRANSFER_TO_UE;
    break;

  case AS_UL_INFO_TRANSFER_REQ:
    ind.msgID = process_ul_info_transfer_req(msg_id,
                &msg->msg.ul_info_transfer_req,
                &ind.msg.ul_info_transfer_ind,
                &cnf.msg.ul_info_transfer_cnf);

    if (ind.msgID) {
      /* Forward indication message to the MME */
      _as_simulator_ue_transfer |= AS_SIMULATOR_TRANSFER_TO_MME;
    }

    /* Return confirm message to the UE */
    cnf.msgID = AS_UL_INFO_TRANSFER_CNF;
    _as_simulator_ue_transfer |= AS_SIMULATOR_TRANSFER_TO_UE;
    break;

  case AS_NAS_RELEASE_REQ:
    ind.msgID = process_nas_release_req(msg_id,
                                        &msg->msg.nas_release_req);
    break;

  default:
    printf("WARNING\t: %s - AS message is not valid (0x%x)\n",
           __FUNCTION__, msg_id);
    rc = RETURNerror;
    break;
  }

  if (_as_simulator_ue_transfer & AS_SIMULATOR_TRANSFER_TO_UE) {
    /* Encode the message confirmation returned to the NAS running
     * at the UE side */
    _as_simulator_ue_size = as_message_encode(_as_simulator_ue_buffer, &cnf,
                            AS_SIMULATOR_UE_BUFFER_SIZE);

    if (_as_simulator_ue_size == RETURNerror) {
      _as_simulator_ue_size = 0;
      rc = RETURNerror;
    }
  }

  if (_as_simulator_ue_transfer & AS_SIMULATOR_TRANSFER_TO_MME) {
    /* Encode the message indication forwarded to the NAS running
     * at the MME side */
    _as_simulator_mme_size = as_message_encode(_as_simulator_mme_buffer, &ind,
                             AS_SIMULATOR_MME_BUFFER_SIZE);

    if (_as_simulator_mme_size == RETURNerror) {
      _as_simulator_mme_size = 0;
      rc = RETURNerror;
    }
  }

  return (rc);
}

/*
 * -----------------------------------------------------------------------------
 *  Functions used to process messages received from the MME NAS process
 * -----------------------------------------------------------------------------
 */
static int _as_simulator_mme_process(int msg_id, as_message_t* msg)
{
  int rc = RETURNok;

  as_message_t cnf;
  as_message_t ind;

  printf("\nINFO\t: %s - Received AS message 0x%x from MME NAS process\n",
         __FUNCTION__, msg_id);

  _as_simulator_mme_transfer = AS_SIMULATOR_NO_TRANSFER;

  memset(&cnf, 0, sizeof(as_message_t));
  memset(&ind, 0, sizeof(as_message_t));

  switch (msg_id) {
  case AS_NAS_ESTABLISH_RSP:
    cnf.msgID = process_nas_establish_rsp(msg_id,
                                          &msg->msg.nas_establish_rsp,
                                          &cnf.msg.nas_establish_cnf);

    if (cnf.msgID) {
      /* Forward confirm message to the UE */
      _as_simulator_mme_transfer |= AS_SIMULATOR_TRANSFER_TO_UE;
    }

    break;

  case AS_DL_INFO_TRANSFER_REQ:
    ind.msgID = process_dl_info_transfer_req(msg_id,
                &msg->msg.dl_info_transfer_req,
                &ind.msg.dl_info_transfer_ind,
                &cnf.msg.dl_info_transfer_cnf);

    if (ind.msgID) {
      /* Forward indication message to the UE */
      _as_simulator_mme_transfer |= AS_SIMULATOR_TRANSFER_TO_UE;
    }

    /* Return confirm message to the MME */
    cnf.msgID = AS_DL_INFO_TRANSFER_CNF;
    _as_simulator_mme_transfer |= AS_SIMULATOR_TRANSFER_TO_MME;
    break;

  case AS_NAS_RELEASE_REQ:
    ind.msgID = process_nas_release_ind(msg_id,
                                        &msg->msg.nas_release_req,
                                        &ind.msg.nas_release_ind);
    /* Forward indication message to the UE */
    _as_simulator_mme_transfer |= AS_SIMULATOR_TRANSFER_TO_UE;
    break;

  default:
    printf("WARNING\t: %s - AS message is not valid (0x%x)\n",
           __FUNCTION__, msg_id);
    rc = RETURNerror;
    break;
  }

  if (_as_simulator_mme_transfer & AS_SIMULATOR_TRANSFER_TO_UE) {
    if (ind.msgID > 0) {
      /* Encode the message indication forwarded to the NAS running
       * at the UE side */
      _as_simulator_ue_size = as_message_encode(_as_simulator_ue_buffer,
                              &ind, AS_SIMULATOR_UE_BUFFER_SIZE);
    } else if (cnf.msgID > 0) {
      /* Encode the message confirmation forwarded to the NAS running
       * at the UE side */
      _as_simulator_ue_size = as_message_encode(_as_simulator_ue_buffer,
                              &cnf, AS_SIMULATOR_UE_BUFFER_SIZE);
    }

    if (_as_simulator_ue_size == RETURNerror) {
      _as_simulator_ue_size = 0;
      rc = RETURNerror;
    }
  }

  if (_as_simulator_mme_transfer & AS_SIMULATOR_TRANSFER_TO_MME) {
    /* Encode the message confirmation returned to the NAS running
     * at the MME side */
    _as_simulator_mme_size = as_message_encode(_as_simulator_mme_buffer,
                             &cnf, AS_SIMULATOR_MME_BUFFER_SIZE);

    if (_as_simulator_mme_size == RETURNerror) {
      _as_simulator_mme_size = 0;
      rc = RETURNerror;
    }
  }

  return (rc);
}

