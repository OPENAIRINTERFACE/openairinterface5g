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

/*! \file multicast.h
 *  \brief
 *  \author Lionel Gauthier and Navid Nikaein
 *  \date 2011
 *  \version 1.1
 *  \company Eurecom
 *  \email: navid.nikaein@eurecom.fr
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define MULTICAST_LINK_C

#include "socket.h"
#include "multicast_link.h"

#include "UTIL/LOG/log.h"

extern unsigned short Master_id;

const char *multicast_group_list[MULTICAST_LINK_NUM_GROUPS] = {
  "239.0.0.161",
  "239.0.0.162",
  "239.0.0.163",
  "239.0.0.164"
};

static multicast_group_t group_list[MULTICAST_LINK_NUM_GROUPS];

/* Socket file descriptors we want to wake up for, using select() */
static fd_set socks;
/* Highest #'d file descriptor, needed for select() */
static int highsock;
#if ! defined(ENABLE_NEW_MULTICAST)
static pthread_t main_loop_thread;
#endif
static void (*rx_handler) (unsigned int, char *);
static unsigned char multicast_group;
static char *multicast_if;

//------------------------------------------------------------------------------
void
multicast_link_init(void)
{
  //------------------------------------------------------------------------------
  int             group;
  int             multicast_loop;
  int             reuse_addr = 1;
  static struct ip_mreq command;
  struct sockaddr_in sin;
  // struct ifreq ifr;

  for (group = 0; group < MULTICAST_LINK_NUM_GROUPS; group++) {
    //strcpy (group_list[group].host_addr, multicast_group_list[group]);
    strncpy (group_list[group].host_addr, multicast_group_list[group], sizeof(group_list[group].host_addr));
    group_list[group].host_addr[sizeof(group_list[group].host_addr) - 1] = 0; // terminate string
    group_list[group].port = 46014 + group;
    group_list[group].socket = make_socket_inet(
                                 SOCK_DGRAM,
                                 &group_list[group].port, &sin);

    LOG_D(EMU, "multicast_link_init(): Created socket %d for group %d, port %d\n",
          group_list[group].socket,group,group_list[group].port);

    /* Used so we can re-bind to our port while a previous connection is still
     * in TIME_WAIT state.
     */
    if (setsockopt(group_list[group].socket, SOL_SOCKET, SO_REUSEADDR,
                   &reuse_addr, sizeof (reuse_addr)) < 0) {
      LOG_E(EMU, "[MULTICAST] ERROR : setsockopt:SO_REUSEADDR, exiting ...");
      exit (EXIT_FAILURE);
    }

    if (multicast_if != NULL) {
      if (setsockopt(group_list[group].socket, SOL_SOCKET,SO_BINDTODEVICE,
                     multicast_if, 4) < 0) {
        LOG_E(EMU,
              "[MULTICAST] ERROR : setsockopt:SO_BINDTODEVICE on interface %s, exiting ...\n",
              multicast_if);
        LOG_E(EMU,
              "[MULTICAST] make sure that you have a root privilage or run with sudo -E \n");
        exit (EXIT_FAILURE);
      }
    }

#if !defined(ENABLE_NEW_MULTICAST)
    /* Make the socket blocking */
    socket_setnonblocking(group_list[group].socket);
#endif

    multicast_loop = 0;

    if (setsockopt (group_list[group].socket, IPPROTO_IP, IP_MULTICAST_LOOP,
                    &multicast_loop, sizeof (multicast_loop)) < 0) {
      LOG_E(EMU,
            "[MULTICAST] ERROR: %s line %d multicast_link_main_loop() IP_MULTICAST_LOOP %m",
            __FILE__, __LINE__);
      exit (EXIT_FAILURE);
    }

    // Join the broadcast group:
    command.imr_multiaddr.s_addr = inet_addr (group_list[group].host_addr);
    command.imr_interface.s_addr = htonl (INADDR_ANY);

    if (command.imr_multiaddr.s_addr == -1) {
      LOG_E(EMU, "[MULTICAST] ERROR: %s line %d NO MULTICAST", __FILE__, __LINE__);
      exit (EXIT_FAILURE);
    }

    if (setsockopt (group_list[group].socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                    &command, sizeof (command)) < 0) {
      LOG_E(EMU, "[MULTICAST] ERROR: %s line %d IP_ADD_MEMBERSHIP %m", __FILE__,
            __LINE__);
      exit (EXIT_FAILURE);
    }

    memset (&group_list[group].sock_remote_addr, 0, sizeof (struct sockaddr_in));
    group_list[group].sock_remote_addr.sin_family = AF_INET;
    group_list[group].sock_remote_addr.sin_addr.s_addr = inet_addr (multicast_group_list[group]);
    group_list[group].sock_remote_addr.sin_port = htons (group_list[group].port);
  }
}

//------------------------------------------------------------------------------
void
multicast_link_build_select_list (void)
{
  //------------------------------------------------------------------------------
  int             group;

  /* First put together fd_set for select(), which will
     consist of the sock veriable in case a new connection
     is coming in, plus all the sockets we have already
     accepted. */


  /* FD_ZERO() clears out the fd_set called socks, so that
     it doesn't contain any file descriptors. */

  FD_ZERO (&socks);


  /* Loops through all the possible connections and adds
     those sockets to the fd_set */

  for (group = 0; group < MULTICAST_LINK_NUM_GROUPS; group++) {
    if (group_list[group].socket != 0) {
      FD_SET (group_list[group].socket, &socks);

      if (group_list[group].socket > highsock) {
        highsock = group_list[group].socket;
      }
    }
  }
}

void
multicast_link_read_data (int groupP)
{
  int num_bytes;

  if ((groupP < MULTICAST_LINK_NUM_GROUPS) && (groupP >= 0)) {
    if ((num_bytes = recvfrom (group_list[groupP].socket,
                               group_list[groupP].rx_buffer, 40000, 0, 0, 0)) < 0) {
      LOG_E(EMU, "[MULTICAST] recvfrom has failed (%d:%s)\n   (%s:%d)\n",
            errno, strerror(errno), __FILE__, __LINE__);
    } else {
      rx_handler(num_bytes,group_list[groupP].rx_buffer);
    }
  } else {
    LOG_E(EMU, "[MULTICAST] ERROR: groupP out of bounds %d\n", groupP);
  }
}

//------------------------------------------------------------------------------
void
multicast_link_read (void)
{
  //------------------------------------------------------------------------------
  int             group;        /* Current item in connectlist for for loops */

  /* Now check connectlist for available data */

  /* Run through our sockets and check to see if anything
     happened with them, if so 'service' them. */

  for (group = multicast_group; group < MULTICAST_LINK_NUM_GROUPS ; group++) {
    if (FD_ISSET (group_list[group].socket, &socks)) {
      multicast_link_read_data (group);
      break;
    }
  }                             /* for (all entries in queue) */
}

//------------------------------------------------------------------------------
int
multicast_link_write_sock(int groupP, char *dataP, uint32_t sizeP)
{
  //------------------------------------------------------------------------------
  int             num;

  if ((num = sendto (group_list[groupP].socket, dataP, sizeP, 0,
                     (struct sockaddr *) &group_list[groupP].sock_remote_addr,
                     sizeof (group_list[groupP].sock_remote_addr))) < 0) {
    LOG_E(EMU, "[MULTICAST] sendto has failed (%d:%s)\n   (%s:%d)\n",
          errno, strerror(errno),
          __FILE__, __LINE__);
  }

  return num;
}

int multicast_link_read_data_from_sock(uint8_t is_master)
{
  struct timeval timeout, *timeout_p;
  int readsocks;    /* Number of sockets ready for reading */

  timeout.tv_sec = 0;
  timeout.tv_usec = 15000;

  if (is_master == 0) {
    /* UE will block indefinetely if no data is sent from eNB
     * NOTE: a NULL timeout for select will block indefinetely
     */
    timeout_p = NULL;
  } else {
    /* In case of eNB set the timeout to 500 usecs after which we consider
     * the packets as dropped.
     */
    timeout_p = &timeout;
  }

  highsock = -1;

  multicast_link_build_select_list ();

  LOG_D(EMU, "Stuck on select with timeout %s\n",
        timeout_p == NULL ? "infinite" : "15000 usecs");

  readsocks = select(highsock + 1, &socks, (fd_set *) 0, (fd_set *) 0, timeout_p);

  if (readsocks < 0) {
    LOG_E(EMU, "Multicast select failed (%d:%s)\n", errno, strerror(errno));
    exit(EXIT_FAILURE);
  } else if (readsocks > 0) {
#ifdef DEBUG_EMU
    LOG_D(EMU, "Multicast Normal read\n");
#endif
    multicast_link_read();
  } else {
    /* Timeout */
    LOG_I(EMU, "Multicast select time-out\n");
    return 1;
  }

  return 0;
}

void* multicast_link_main_loop (void *param)
{
  while (1) {
    multicast_link_read_data_from_sock(0);
  }

  return NULL;
}

void multicast_link_start(void (*rx_handlerP) (unsigned int, char *),
                          unsigned char _multicast_group, char *multicast_ifname)
{
  rx_handler = rx_handlerP;
  multicast_group = _multicast_group;
  multicast_if =  multicast_ifname;
  LOG_I(EMU, "[MULTICAST] LINK START on interface=%s for group=%d: handler=%p\n",
        (multicast_if == NULL) ? "not specified" : multicast_if, multicast_group,
        rx_handler);
  multicast_link_init ();
#if ! defined(ENABLE_NEW_MULTICAST)
  LOG_D(EMU, "[MULTICAST] multicast link start thread\n");

  if (pthread_create (&main_loop_thread, NULL, multicast_link_main_loop,
                      NULL) != 0) {
    LOG_E(EMU, "[MULTICAST LINK] Error in pthread_create (%d:%s)\n",
          errno, strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    pthread_detach(main_loop_thread);  // disassociate from parent
    LOG_I(EMU, "[MULTICAST LINK] Thread detached\n");
  }

#endif
}
