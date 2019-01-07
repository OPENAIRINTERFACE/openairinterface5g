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

#include "queues.h"
#include "mobipass.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#define QSIZE 10000

struct mobipass_header {
  uint16_t flags;
  uint16_t fifo_status;
  unsigned char seqno;
  unsigned char ack;
  uint32_t word0;
  uint32_t timestamp;
} __attribute__((__packed__));

struct queue {
  unsigned char buf[QSIZE][14+14+640*2];
  volatile int start;
  volatile int len;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
};

typedef struct {
  struct queue to_mobipass;
  struct queue from_mobipass;
  int samples_per_1024_frames;

  /* variables used by dequeue_from_mobipass */
  int dequeue_from_mobipass_seqno;

  /* variables used to manage logging of "missing samples"
   * coming from mobipass. (Yes, this can happen, mostly
   * at startup.)
   * This idea is to print some logs, but not too much to
   * flood stdout. (This is already a bad idea to call
   * printf in a 'realtime' thread.)
   */
  int no_sample_log_running;
  uint32_t no_sample_log_start_sample;
  uint32_t no_sample_log_next_sample;
} queue_state_t;

static void enqueue(void *data, struct queue *q)
{
  int pos;

  if (pthread_mutex_lock(&q->mutex)) abort();
  if (q->len == QSIZE) {
    printf("mobipass: WARNING: enqueue: full\n");
    goto done;
  }

  pos = (q->start + q->len) % QSIZE;
  memcpy(q->buf[pos], data, 14+14+640*2);
  q->len++;

done:
  if (pthread_cond_signal(&q->cond)) abort();
  if (pthread_mutex_unlock(&q->mutex)) abort();
}

void enqueue_to_mobipass(void *_qstate, void *data)
{
  queue_state_t *qstate = _qstate;
  enqueue(data, &qstate->to_mobipass);
}

void dequeue_to_mobipass(void *_qstate, uint32_t timestamp, void *data)
{
  queue_state_t *qstate = _qstate;
  if (pthread_mutex_lock(&qstate->to_mobipass.mutex)) abort();
  while (qstate->to_mobipass.len == 0) {
    if (pthread_cond_wait(&qstate->to_mobipass.cond, &qstate->to_mobipass.mutex)) abort();
  }

  memcpy(data, qstate->to_mobipass.buf[qstate->to_mobipass.start], 14+14+640*2);
  qstate->to_mobipass.len--;
  qstate->to_mobipass.start = (qstate->to_mobipass.start + 1) % QSIZE;

  if (pthread_mutex_unlock(&qstate->to_mobipass.mutex)) abort();
}

void enqueue_from_mobipass(void *_qstate, void *data)
{
  queue_state_t *qstate = _qstate;
  struct mobipass_header *mh = (struct mobipass_header *)((char*)data+14);
  mh->timestamp = htonl(ntohl(mh->timestamp) % qstate->samples_per_1024_frames);
//printf("from mobipass! timestamp %u seqno %d\n", ntohl(mh->timestamp), mh->seqno);
  enqueue(data, &qstate->from_mobipass);
}

static int cmp_timestamps(uint32_t a, uint32_t b, int samples_per_1024_frames)
{
  if (a == b) return 0;
  if (a < b) {
    if (b-a > samples_per_1024_frames/2) return 1;
    return -1;
  }
  if (a-b > samples_per_1024_frames/2) return -1;
  return 1;
}

/*************************************************/
/* missing samples logging management begin      */
/*************************************************/

static void log_flush(queue_state_t *qstate)
{
  /* print now if there is something to print */
  if (qstate->no_sample_log_running == 0)
    return;
  qstate->no_sample_log_running = 0;
  printf("mobipass: WARNING: missing samples [%u-%u]\n",
      qstate->no_sample_log_start_sample,
      (uint32_t)(qstate->no_sample_log_next_sample-1));
}

static void log_missed_sample(queue_state_t *qstate, uint32_t timestamp)
{
  /* collect data, print if there is a discontinuity */
  if (qstate->no_sample_log_running == 0 ||
      timestamp != qstate->no_sample_log_next_sample) {
    log_flush(qstate);
    qstate->no_sample_log_start_sample = timestamp;
  }

  qstate->no_sample_log_next_sample = timestamp+1;
  qstate->no_sample_log_running = 1;
}

static void log_flush_if_old(queue_state_t *qstate, uint32_t timestamp)
{
  /* log every second (more or less), if we have to */
  /* note that if mobipass stopped, it may take much more
   * than one second to log, due to the sleeps done while
   * waiting for samples (that never come)
   */
  if (qstate->no_sample_log_running == 1 &&
      labs(timestamp-qstate->no_sample_log_start_sample) > qstate->samples_per_1024_frames/10)
    log_flush(qstate);
}

/*************************************************/
/* missing samples logging management end        */
/*************************************************/

/* to be called with lock on */
static void get_sample_from_mobipass(queue_state_t *qstate, char *I, char *Q, uint32_t timestamp)
{
  unsigned char *b = NULL;
  unsigned char *data = NULL;
  struct mobipass_header *mh = NULL;
  uint32_t packet_timestamp = 0;

  while (qstate->from_mobipass.len) {
    b = qstate->from_mobipass.buf[qstate->from_mobipass.start];
    mh = (struct mobipass_header *)(b+14);
    data = b + 14*2;
    packet_timestamp = ntohl(mh->timestamp);
    if (cmp_timestamps(timestamp, packet_timestamp, qstate->samples_per_1024_frames) < 0) goto nodata;
    if (cmp_timestamps(timestamp, (packet_timestamp+640) % qstate->samples_per_1024_frames, qstate->samples_per_1024_frames) < 0) break;
    qstate->from_mobipass.len--;
    qstate->from_mobipass.start = (qstate->from_mobipass.start+1) % QSIZE;
  }

  if (qstate->from_mobipass.len == 0) goto nodata;

  if (timestamp == (packet_timestamp + 639) % qstate->samples_per_1024_frames) {
    qstate->from_mobipass.len--;
    qstate->from_mobipass.start = (qstate->from_mobipass.start+1) % QSIZE;
  }

  if (timestamp < packet_timestamp) timestamp += qstate->samples_per_1024_frames;

  *I = data[(timestamp - packet_timestamp) * 2];
  *Q = data[(timestamp - packet_timestamp) * 2 + 1];

  return;

nodata:
  *I = 0;
  *Q = 0;

  log_missed_sample(qstate, timestamp);
}

/* doesn't work with delay more than 1s */
static void wait_for_data(pthread_cond_t *cond, pthread_mutex_t *mutex, int delay_us)
{
  struct timeval now;
  struct timespec target;
  gettimeofday(&now, NULL);
  target.tv_sec = now.tv_sec;
  target.tv_nsec = (now.tv_usec + delay_us) * 1000;
  if (target.tv_nsec >= 1000 * 1000 * 1000) { target.tv_nsec -= 1000 * 1000 * 1000; target.tv_sec++; }
  int err = pthread_cond_timedwait(cond, mutex, &target);
  if (err != 0 && err != ETIMEDOUT) { printf("mobipass: ERROR: pthread_cond_timedwait: err (%d) %s\n", err, strerror(err)); abort(); }
}

/* don't block infinitely when waiting for data
 * if waiting for too long, just return some zeros
 */
void dequeue_from_mobipass(void *_qstate, uint32_t timestamp, void *data)
{
  queue_state_t *qstate = _qstate;
  int i;
//  int ts = timestamp;
  int waiting_allowed;

  if (pthread_mutex_lock(&qstate->from_mobipass.mutex)) abort();

  if (qstate->from_mobipass.len == 0) {
//printf("sleep 1\n");
    wait_for_data(&qstate->from_mobipass.cond, &qstate->from_mobipass.mutex, 2000); //1000/3);
  }

  waiting_allowed = qstate->from_mobipass.len != 0;

  for (i = 0; i < 640*2; i+=2) {
    if (qstate->from_mobipass.len == 0 && waiting_allowed) {
//printf("sleep 2\n");
      wait_for_data(&qstate->from_mobipass.cond, &qstate->from_mobipass.mutex, 2000); //1000/3);
      waiting_allowed = qstate->from_mobipass.len != 0;
    }

    get_sample_from_mobipass(qstate, (char*)data + 14*2 + i, (char*)data + 14*2 + i+1, timestamp % qstate->samples_per_1024_frames);
    timestamp++;
  }

  log_flush_if_old(qstate, timestamp);

  if (pthread_mutex_unlock(&qstate->from_mobipass.mutex)) abort();

  struct mobipass_header *mh = (struct mobipass_header *)(((char *)data) + 14);
  mh->flags = 0;
  mh->fifo_status = 0;
  mh->seqno = qstate->dequeue_from_mobipass_seqno++;
  mh->ack = 0;
  mh->word0 = 0;
  mh->timestamp = htonl(timestamp);
}

void *init_queues(int samples_per_1024_frames)
{
  queue_state_t *q;
  q = malloc(sizeof(queue_state_t));
  if (q == NULL) abort();
  memset(q, 0, sizeof(queue_state_t));

  if (pthread_mutex_init(&q->to_mobipass.mutex, NULL)) abort();
  if (pthread_mutex_init(&q->from_mobipass.mutex, NULL)) abort();
  if (pthread_cond_init(&q->to_mobipass.cond, NULL)) abort();
  if (pthread_cond_init(&q->from_mobipass.cond, NULL)) abort();

  q->samples_per_1024_frames = samples_per_1024_frames;

  return q;
}
