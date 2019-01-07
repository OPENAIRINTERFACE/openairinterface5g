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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <pthread.h>

#include "queues.h"
#include "mobipass.h"

/******************************************************************/
/* time begin                                                     */
/******************************************************************/

#include <time.h>

static void init_time(mobipass_state_t *mobi)
{
  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC_RAW, &now)) abort();
  mobi->t0 = (uint64_t)now.tv_sec * (uint64_t)1000000000 + (uint64_t)now.tv_nsec;
}

/* called before sending data to mobipass
 * waits if called too early with respect to system clock
 * does not wait more than 1ms in any case
 */
static void synch_time(mobipass_state_t *mobi, uint32_t ts)
{
  if (ts < mobi->synch_time_last_ts) mobi->synch_time_mega_ts++;
  mobi->synch_time_last_ts = ts;

  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC_RAW, &now)) abort();

  uint64_t tnow;
  tnow = (uint64_t)now.tv_sec * (uint64_t)1000000000 + (uint64_t)now.tv_nsec;

  uint64_t cur = tnow - mobi->t0;

  /* 15360000 samples/second, in nanoseconds:
   *  = 15360000 / 1000000000 = 1536 / 100000 = 48 / 3125*/

  uint64_t ts_ns = ((uint64_t)ts + mobi->synch_time_mega_ts * (uint64_t)mobi->samples_per_1024_frames) * (uint64_t)3125 / (uint64_t)48;

  /* TODO: if cur is way higher than ts_ns, we are very late, log something? */
  if (cur >= ts_ns) return;

  uint64_t delta = ts_ns - cur;
  /* don't sleep more than 1 ms */
  if (delta > 1000*1000) delta = 1000*1000;
  delta = delta/1000;
  if (delta) usleep(delta);
}

/******************************************************************/
/* time end                                                       */
/******************************************************************/

struct ethernet_header {
  unsigned char dst[6];
  unsigned char src[6];
  uint16_t packet_type;
} __attribute__((__packed__));

struct mobipass_header {
  uint16_t flags;
  uint16_t fifo_status;
  unsigned char seqno;
  unsigned char ack;
  uint32_t word0;
  uint32_t timestamp;
} __attribute__((__packed__));

static void do_receive(mobipass_state_t *mobi, unsigned char *b)
{
  if (recv(mobi->sock, b, 14+14+1280, 0) != 14+14+1280) { perror("recv"); exit(1); }
  struct mobipass_header *mh = (struct mobipass_header *)(b+14);
  mh->timestamp = htonl((ntohl(mh->timestamp)-45378/*40120*/) % mobi->samples_per_1024_frames);
//printf("recv timestamp %u\n", ntohl(mh->timestamp));
}

/* receiver thread */
static void *receiver(void *_mobi)
{
  mobipass_state_t *mobi = _mobi;
  unsigned char receive_packet[14 + 14 + 1280];
  while (1) {
    do_receive(mobi, receive_packet);
    enqueue_from_mobipass(mobi->qstate, receive_packet);
  }
  return 0;
}

static void do_send(mobipass_state_t *mobi, int seqno, uint32_t ts,
    unsigned char *packet)
{
  struct ethernet_header *eh = (struct ethernet_header *)packet;
  struct mobipass_header *mh = (struct mobipass_header *)(packet+14);

  ts %= mobi->samples_per_1024_frames;
//printf("SEND seqno %d ts %d\n", seqno, ts);

  memcpy(eh->dst, mobi->eth_remote, 6);
  memcpy(eh->src, mobi->eth_local, 6);

  eh->packet_type = htons(0xbffe);

  mh->flags = 0;
  mh->fifo_status = 0;
  mh->seqno = seqno;
  mh->ack = 0;
  mh->word0 = 0;
  mh->timestamp = htonl(ts);

  synch_time(mobi, ts);

  if (send(mobi->sock, packet, 14+14+1280, 0) != 14+14+1280) { perror("send"); exit(1); }
}

/* sender thread */
static void *sender(void *_mobi)
{
  mobipass_state_t *mobi = _mobi;
  unsigned char packet[14 + 14 + 1280];
  uint32_t ts = 0;
  unsigned char seqno = 0;
  while (1) {
    dequeue_to_mobipass(mobi->qstate, ts, packet);
    do_send(mobi, seqno, ts, packet);
    seqno++;
    ts += 640;
    ts %= mobi->samples_per_1024_frames;
  }
  return 0;
}

static void new_thread(void *(*f)(void *), void *data)
{
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att))
    { fprintf(stderr, "pthread_attr_init err\n"); exit(1); }
  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED))
    { fprintf(stderr, "pthread_attr_setdetachstate err\n"); exit(1); }
  if (pthread_attr_setstacksize(&att, 10000000))
    { fprintf(stderr, "pthread_attr_setstacksize err\n"); exit(1); }
  if (pthread_create(&t, &att, f, data))
    { fprintf(stderr, "pthread_create err\n"); exit(1); }
  if (pthread_attr_destroy(&att))
    { fprintf(stderr, "pthread_attr_destroy err\n"); exit(1); }
}

void init_mobipass(mobipass_state_t *mobi)
{
  int i;
  unsigned char data[14+14+640];
  memset(data, 0, 14+14+640);

  init_time(mobi);

  mobi->qstate = init_queues(mobi->samples_per_1024_frames);

  for (i = 0; i < 24*4; i++) {
    uint32_t timestamp = i*640;
    unsigned char seqno = i;
    struct mobipass_header *mh = (struct mobipass_header *)(data+14);
    mh->seqno = seqno;
    mh->timestamp = htonl(timestamp);
    enqueue_to_mobipass(mobi->qstate, data);
  }

  mobi->sock = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
  if (mobi->sock == -1) { perror("socket"); exit(1); }

  /* get if index */
  struct ifreq if_index;
  memset(&if_index, 0, sizeof(struct ifreq));
  strcpy(if_index.ifr_name, mobi->eth.if_name);
  if (ioctl(mobi->sock, SIOCGIFINDEX, &if_index)<0) {perror("SIOCGIFINDEX");exit(1);}

  struct sockaddr_ll local_addr;
  local_addr.sll_family   = AF_PACKET;
  local_addr.sll_ifindex  = if_index.ifr_ifindex;
  local_addr.sll_protocol = htons(0xbffe);
  local_addr.sll_halen    = ETH_ALEN;
  local_addr.sll_pkttype  = PACKET_OTHERHOST;

  if (bind(mobi->sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_ll))<0)
    { perror("bind"); exit(1); }

  new_thread(receiver, mobi);
  new_thread(sender, mobi);
}
