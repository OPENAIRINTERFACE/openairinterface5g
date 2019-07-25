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

Source    user_simulator.c

Version   0.1

Date    2012/10/09

Product   User simulator

Subsystem User simulator main process

Author    Frederic Maurel

Description Implements the user simulator running at the user application
    layer for NAS testing purpose.

*****************************************************************************/

#include "user_parser.h"

#include "include/commonDef.h"
#include "util/socket.h"
#include "util/device.h"

#include <stdio.h>  // printf, perror
#include <errno.h>  // errno
#include <netdb.h>  // gai_strerror
#include <ctype.h>  // isspace
#include <stdlib.h> // exit
#include <signal.h> // sigaction
#include <string.h> // memset
#include <pthread.h>
#include <poll.h>

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

#define USER_SIMULATOR_BUFFER_SIZE 1024

/*
 * String buffer used to send AT commands to the NAS sublayer
 */
static char _user_simulator_send_buffer [USER_SIMULATOR_BUFFER_SIZE];

/*
 * String buffer used to receive responses from the NAS sublayer
 */
static char _user_simulator_recv_buffer [USER_SIMULATOR_BUFFER_SIZE];

/*
 * The connection endpoint used for communication with the
 * NAS sublayer (see src/api/user/user_api.c)
 */
static struct {
  void* endpoint;
  void*   (*open) (int, const char*, const char*);
  int     (*getfd)(const void*);
  ssize_t (*recv) (void*, char*, size_t);
  ssize_t (*send) (const void*, const char*, size_t);
  void    (*close)(void*);
} _user_simulator_id;
//static socket_id_t * _user_simulator_sid;

#define USER_OPEN(a, b, c)  _user_simulator_id.open(a, b, c)
#define USER_GETFD()    _user_simulator_id.getfd(_user_simulator_id.endpoint)
#define USER_RECV(a, b)   _user_simulator_id.recv(_user_simulator_id.endpoint, a, b)
#define USER_SEND(a, b)   _user_simulator_id.send(_user_simulator_id.endpoint, a, b)
#define USER_CLOSE()    _user_simulator_id.close(_user_simulator_id.endpoint)

static int _set_signal_handler(int signal, void (handler)(int));
static void _signal_handler(int signal_number);
static void* _receive_thread(void* arg);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************/
int main (int argc, const char* argv[])
{
  /*
   * Get the command line options
   */
  if ( user_parser_get_options(argc, argv) != RETURNok ) {
    user_parser_print_usage();
    exit(EXIT_FAILURE);
  }

  const char* host = user_parser_get_host();
  const char* port = user_parser_get_port();
  const char* devpath = user_parser_get_devpath();
  const char* devattr = user_parser_get_devattr();

  printf("%s -host %s -port %s -dev %s -params %s\n",
         argv[0], host, port, devpath, devattr);

  /*
   * Initialize the communication channel to the NAS sublayer
   */
  if (devpath) {
    /* Initialize device handlers */
    _user_simulator_id.open  = device_open;
    _user_simulator_id.getfd = device_get_fd;
    _user_simulator_id.recv  = device_read;
    _user_simulator_id.send  = device_write;
    _user_simulator_id.close = device_close;

    /* Initialize communication channel */
    _user_simulator_id.endpoint = USER_OPEN(DEVICE, devpath, devattr);

    if (_user_simulator_id.endpoint == NULL) {
      perror("ERROR\t: Failed to open connection endpoint");
      exit(EXIT_FAILURE);
    }

    printf("INFO\t: The User Simulator is now connected to %s (%d)\n",
           devpath, (int)USER_GETFD());
  } else {
    /* Initialize network socket handlers */
    _user_simulator_id.open  = socket_udp_open;
    _user_simulator_id.getfd = socket_get_fd;
    _user_simulator_id.recv  = socket_recv;
    _user_simulator_id.send  = socket_send;
    _user_simulator_id.close = socket_close;

    /* Initialize communication channel */
    _user_simulator_id.endpoint = USER_OPEN(SOCKET_CLIENT, host, port);

    if (_user_simulator_id.endpoint == NULL) {
      const char* error = ( (errno < 0) ?
                            gai_strerror(errno) : strerror(errno) );
      printf("ERROR\t: Failed to open connection endpoint: %s\n", error);
      exit(EXIT_FAILURE);
    }

    printf("INFO\t: The User Simulator is now connected to %s/%s (%d)\n",
           host, port, (int)USER_GETFD());
  }


  /*
   * Set up signal handler
   */
  (void) _set_signal_handler(SIGINT, _signal_handler);
  (void) _set_signal_handler(SIGTERM, _signal_handler);

  /*
   * Create the receiving thread
   */
  pthread_attr_t attr;
  pthread_t thread_id;
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  int rc = pthread_create (&thread_id, &attr, _receive_thread, NULL);

  if (rc != 0) {
    perror("ERROR\t: pthread_create() failed");
    USER_CLOSE();
    exit(EXIT_FAILURE);
  }

  /*
   * User simulator main loop
   */
  while (TRUE) {
    char c;
    int len;

    /* Get the AT command */
    printf("AT command (q to quit) > ");
    c = getchar();

    if (c == 'q') break;

    if (c == '\n') continue;

    len = 0;

    while ( (len < USER_SIMULATOR_BUFFER_SIZE - 1) && (c != '\n') ) {
      if (!isspace(c)) {
        _user_simulator_send_buffer[len++] = c;
      }

      c = getchar();
    }

    _user_simulator_send_buffer[len++] = '\r';
    _user_simulator_send_buffer[len] = '\0';

    /* Send the AT command to the NAS sublayer */
    int sbytes = USER_SEND(_user_simulator_send_buffer, len);

    if (sbytes == RETURNerror) {
      perror("ERROR\t: Failed to send data to the NAS sublayer");
      break;
    }

    (void)poll(0, 0, 100);
  }

  /*
   * Termination cleanup
   */
  printf("INFO\t: Closing user endpoint descriptor %d\n", (int)USER_GETFD());
  USER_CLOSE();

  printf("INFO\t: User simulator exited\n");
  exit(EXIT_SUCCESS);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * Signal handler setup function
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
 * Signal handler
 */
static void _signal_handler(int signal_number)
{
  printf("\nWARNING\t: Signal %d received\n", signal_number);
  printf("INFO\t: Closing user socket %d\n", (int)USER_GETFD());
  USER_CLOSE();
  printf("INFO\t: User simulator exited\n");
  exit(EXIT_SUCCESS);
}

/*
 * Receiving thread
 */
static void* _receive_thread(void* arg)
{
  while (TRUE) {
    /* Receive AT response from the NAS sublayer */
    int rbytes = USER_RECV(_user_simulator_recv_buffer,
                           USER_SIMULATOR_BUFFER_SIZE);

    if (rbytes != 0) {
      if (rbytes == RETURNerror) {
        perror("ERROR\t: Failed to receive data from the NAS sublayer");
        break;
      }

      _user_simulator_recv_buffer[rbytes] = '\0';

      /* Display AT response */
      printf("%s\n", _user_simulator_recv_buffer);
    }
  }

  return (NULL);
}

