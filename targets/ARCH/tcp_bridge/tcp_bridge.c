#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int fullread(int fd, void *_buf, int count)
{
  char *buf = _buf;
  int ret = 0;
  int l;
  while (count) {
    l = read(fd, buf, count);
    if (l <= 0) return -1;
    count -= l;
    buf += l;
    ret += l;
  }
  return ret;
}

int fullwrite(int fd, void *_buf, int count)
{
  char *buf = _buf;
  int ret = 0;
  int l;
  while (count) {
    l = write(fd, buf, count);
    if (l <= 0) return -1;
    count -= l;
    buf += l;
    ret += l;
  }
  return ret;
}

#include "common_lib.h"

typedef struct {
  int32_t data[30720]; /* max 20MHz */
  unsigned long timestamp;
  int size;
} input_buffer;

#define BSIZE 16

typedef struct {
  int sock;
  int samples_per_subframe;
  input_buffer b[BSIZE];
  int bstart;
  int blen;
  unsigned long read_timestamp;
} tcp_bridge_state_t;


/****************************************************************************/
/* buffer management                                                        */
/* (could be simpler, we are synchronous)                                   */
/* maybe we should lock                                                     */
/****************************************************************************/

void put_buffer(tcp_bridge_state_t *t, void *data, int size, unsigned long timestamp)
{
  int nextpos;
  if (t->blen == BSIZE) { printf("tcp_bridge: buffer full\n"); exit(1); }
  if (size > 30720*4) abort();
  if (t->blen) {
    int lastpos = (t->bstart+t->blen-1) % BSIZE;
    if (timestamp != t->b[lastpos].timestamp + t->samples_per_subframe)
      { printf("tcp_bridge: discontinuity\n"); exit(1); }
  }
  nextpos = (t->bstart+t->blen) % BSIZE;
  t->b[nextpos].timestamp = timestamp;
  t->b[nextpos].size      = size;
  memcpy(t->b[nextpos].data, data, size);
  t->blen++;
}

void get_buffer(tcp_bridge_state_t *t, void *data, int size, unsigned long timestamp)
{
  int pos;
  if (t->blen == 0) { printf("tcp_bridge: buffer empty\n"); exit(1); }
  if (size >30720*4) abort();
  pos = t->bstart;
  if (size != t->b[pos].size) { printf("tcp_bridge: bad size\n"); exit(1); }
  memcpy(data, t->b[pos].data, size);
  t->bstart = (t->bstart + 1) % BSIZE;
  t->blen--;
}

/****************************************************************************/
/* end of buffer management                                                 */
/****************************************************************************/


/****************************************************************************/
/* network management (read/write)                                          */
/****************************************************************************/

void read_data_from_network(int sock, input_buffer *b)
{
  if (fullread(sock, &b->timestamp, sizeof(b->timestamp)) != sizeof(b->timestamp)) goto err;
  if (fullread(sock, &b->size, sizeof(b->size)) != sizeof(b->size)) goto err;
  if (fullread(sock, b->data, b->size) != b->size) goto err;
  return;

err:
  printf("tcp_bridge: read_data_from_network fails\n");
  exit(1);
}

void write_data_to_network(int sock, input_buffer *b)
{
  if (fullwrite(sock, &b->timestamp, sizeof(b->timestamp)) != sizeof(b->timestamp)) goto err;
  if (fullwrite(sock, &b->size, sizeof(b->size)) != sizeof(b->size)) goto err;
  if (fullwrite(sock, b->data, b->size) != b->size) goto err;
  return;

err:
  printf("tcp_bridge: write_data_to_network fails\n");
  exit(1);
}

/****************************************************************************/
/* end of network management                                                */
/****************************************************************************/


int tcp_bridge_start(openair0_device *device)
{
  int i;
  int port = 4042;
  tcp_bridge_state_t *tcp_bridge = device->priv;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) { perror("tcp_bridge: socket"); exit(1); }

  int enable = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
    { perror("tcp_bridge: SO_REUSEADDR"); exit(1); }

  struct sockaddr_in addr = {
    sin_family: AF_INET,
    sin_port: htons(port),
    sin_addr: { s_addr: INADDR_ANY }
  };

  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)))
    { perror("tcp_bridge: bind"); exit(1); }

  if (listen(sock, 5))
    { perror("tcp_bridge: listen"); exit(1); }

  printf("tcp_bridge: wait for connection on port %d\n", port);

  socklen_t len = sizeof(addr);
  int sock2 = accept(sock, (struct sockaddr *)&addr, &len);
  if (sock2 == -1)
    { perror("tcp_bridge: accept"); exit(1); }

  close(sock);

  tcp_bridge->sock = sock2;

  /* the other end has to send 10 subframes at startup */
  for (i = 0; i < 10; i++)
    read_data_from_network(sock2, &tcp_bridge->b[i]);
  tcp_bridge->bstart = 0;
  tcp_bridge->blen = 10;

  printf("tcp_bridge: connection established\n");

  return 0;
}

int tcp_bridge_request(openair0_device *device, void *msg, ssize_t msg_len) { abort(); return 0; }
int tcp_bridge_reply(openair0_device *device, void *msg, ssize_t msg_len) { abort(); return 0; }
int tcp_bridge_get_stats(openair0_device* device) { return 0; }
int tcp_bridge_reset_stats(openair0_device* device) { return 0; }
void tcp_bridge_end(openair0_device *device) {}
int tcp_bridge_stop(openair0_device *device) { return 0; }
int tcp_bridge_set_freq(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config) { return 0; }
int tcp_bridge_set_gains(openair0_device* device, openair0_config_t *openair0_cfg) { return 0; }

int tcp_bridge_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags)
{
  tcp_bridge_state_t *t = device->priv;
  input_buffer out;
  int i;
//printf("write ts %ld nsamps %d\n", timestamp, nsamps);

  if (nsamps > 30720) abort();

  /* read buffer timestamped with 'timestamp' if not already here */
  for (i = 0; i < t->blen; i++) {
    input_buffer *b = &t->b[(t->bstart + i) % BSIZE];
    if (b->timestamp == timestamp) break;
  }
  if (i == t->blen) {
    int nextpos = (t->bstart + t->blen) % BSIZE;
    if (t->blen == BSIZE) abort();
    read_data_from_network(t->sock, &t->b[nextpos]);
    t->blen++;
    if (t->b[nextpos].timestamp != timestamp) abort();
  }

  memcpy(out.data, buff[0], nsamps * 4);
  out.timestamp = timestamp;
  out.size = nsamps * 4;
  write_data_to_network(t->sock, &out);

  return nsamps;
}

int tcp_bridge_read(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc)
{
  tcp_bridge_state_t *t = device->priv;
  input_buffer *b;

  if (cc != 1) abort();
  if (t->blen == 0) abort();

  b = &t->b[t->bstart];

#if 0
typedef struct {
  int32_t data[30720]; /* max 20MHz */
  unsigned long timestamp;
  int size;
} input_buffer;
#endif

  if (b->timestamp != t->read_timestamp) abort();
  if (b->size != nsamps * 4) abort();

  *timestamp = b->timestamp;
  memcpy(buff[0], b->data, b->size);

  t->read_timestamp += nsamps;
  t->bstart = (t->bstart + 1) % BSIZE;
  t->blen--;

static unsigned long ts = 0;
//printf("read ts %ld nsamps %d\n", ts, nsamps);
*timestamp = ts;
ts += nsamps;

  return nsamps;
}

__attribute__((__visibility__("default")))
int device_init(openair0_device* device, openair0_config_t *openair0_cfg)
{
  tcp_bridge_state_t *tcp_bridge = (tcp_bridge_state_t*)malloc(sizeof(tcp_bridge_state_t));
  memset(tcp_bridge, 0, sizeof(tcp_bridge_state_t));

  /* only 25 or 50 PRBs handled for the moment */
  if (openair0_cfg[0].sample_rate != 30720000 &&
      openair0_cfg[0].sample_rate != 15360000 &&
      openair0_cfg[0].sample_rate !=  7680000) {
    printf("tcp_bridge: ERROR: only 25, 50 or 100 PRBs supported\n");
    exit(1);
  }

  device->trx_start_func   = tcp_bridge_start;
  device->trx_get_stats_func   = tcp_bridge_get_stats;
  device->trx_reset_stats_func = tcp_bridge_reset_stats;
  device->trx_end_func         = tcp_bridge_end;
  device->trx_stop_func        = tcp_bridge_stop;
  device->trx_set_freq_func = tcp_bridge_set_freq;
  device->trx_set_gains_func = tcp_bridge_set_gains;
  device->trx_write_func   = tcp_bridge_write;
  device->trx_read_func    = tcp_bridge_read;

  device->priv = tcp_bridge;

  switch ((int)openair0_cfg[0].sample_rate) {
  case 30720000: tcp_bridge->samples_per_subframe = 30720; break;
  case 15360000: tcp_bridge->samples_per_subframe = 15360; break;
  case 7680000:  tcp_bridge->samples_per_subframe = 7680; break;
  }

  /* let's pretend to be a b2x0 */
  device->type = USRP_B200_DEV;

  device->openair0_cfg=&openair0_cfg[0];

  return 0;
}
