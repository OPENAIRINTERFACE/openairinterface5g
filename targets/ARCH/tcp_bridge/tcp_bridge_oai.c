#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

const int port = 4043;
#define helpTxt "\
\x1b[31m\
tcp_bridge: error: you have to run one UE and one eNB\n\
For this, export TCPBRIDGE=enb (eNB case) or \n\
                 TCPBRIDGE=<an ip address> (UE case)\n\
\x1b[m"

int fullread(int fd, void *_buf, int count) {
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

int fullwrite(int fd, void *_buf, int count) {
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
  int sock;
  int samples_per_subframe;
  uint64_t timestamp;
  uint64_t next_tx_timestamp;
  int is_enb;
  char *ip;
} tcp_bridge_state_t;

void verify_connection(int fd, int is_enb) {
  char c = is_enb;

  if (fullwrite(fd, &c, 1) != 1) exit(1);

  if (fullread(fd, &c, 1) != 1) exit(1);

  if (c == is_enb) {
    printf(helpTxt);
    exit(1);
  }
}


int start_enb(tcp_bridge_state_t *tcp_bridge) {
  struct sockaddr_in addr = {
sin_family:
    AF_INET,
sin_port:
    htons(port),
sin_addr:
    { s_addr: INADDR_ANY }
  };

  if (bind(tcp_bridge->sock, (struct sockaddr *)&addr, sizeof(addr))) {
    perror("tcp_bridge: bind");
    exit(1);
  }

  if (listen(tcp_bridge->sock, 5)) {
    perror("tcp_bridge: listen");
    exit(1);
  }

  printf("tcp_bridge: wait for connection on port %d\n", port);
  socklen_t len = sizeof(addr);
  int sockServ = accept(tcp_bridge->sock, (struct sockaddr *)&addr, &len);

  if ( sockServ == -1) {
    perror("tcp_bridge: accept");
    exit(1);
  }

  verify_connection(sockServ, tcp_bridge->is_enb);
  printf("tcp_bridge: connection established\n");
  close(tcp_bridge->sock);
  tcp_bridge->sock=sockServ;
  return 0;
}

int start_ue(tcp_bridge_state_t *tcp_bridge) {
struct sockaddr_in addr = {sin_family:
    AF_INET,
sin_port:
    htons(port),
sin_addr:
    { s_addr: INADDR_ANY }
  };
  addr.sin_addr.s_addr = inet_addr(tcp_bridge->ip);

  while(1) {
    printf("tcp_bridge: trying to connect to %s:%d\n", tcp_bridge->ip, port);

    if (connect(tcp_bridge->sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
      verify_connection(tcp_bridge->sock, tcp_bridge->is_enb);
      printf("tcp_bridge: connection established\n");
      return 0;
    }

    perror("tcp_bridge");
    sleep(1);
  }

  return 0;
}

int tcp_bridge_start(openair0_device *device) {
  tcp_bridge_state_t *tcp_bridge = device->priv;
  tcp_bridge->sock = socket(AF_INET, SOCK_STREAM, 0);

  if (tcp_bridge->sock == -1) {
    perror("tcp_bridge: socket");
    exit(1);
  }

  int enable = 1;

  if (setsockopt(tcp_bridge->sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
    perror("tcp_bridge: SO_REUSEADDR");
    exit(1);
  }

  if ( tcp_bridge->is_enb )
    return start_enb(tcp_bridge);
  else
    return start_ue(tcp_bridge);
}

int tcp_bridge_request(openair0_device *device, void *msg, ssize_t msg_len) {
  abort();
  return 0;
}
int tcp_bridge_reply(openair0_device *device, void *msg, ssize_t msg_len) {
  abort();
  return 0;
}
int tcp_bridge_get_stats(openair0_device *device) {
  return 0;
}
int tcp_bridge_reset_stats(openair0_device *device) {
  return 0;
}
void tcp_bridge_end(openair0_device *device) {}
int tcp_bridge_stop(openair0_device *device) {
  return 0;
}
int tcp_bridge_set_freq(openair0_device *device, openair0_config_t *openair0_cfg,int exmimo_dump_config) {
  return 0;
}
int tcp_bridge_set_gains(openair0_device *device, openair0_config_t *openair0_cfg) {
  return 0;
}

int tcp_bridge_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {
  if (cc != 1) {
    printf("tcp_bridge: only 1 antenna supported\n");
    exit(1);
  }

  tcp_bridge_state_t *t = device->priv;

  /* deal with discontinuities in output (think: eNB in TDD mode) */
  if (t->next_tx_timestamp && timestamp != t->next_tx_timestamp) {
    uint32_t b[4096];
    uint64_t to_send = timestamp - t->next_tx_timestamp;
    memset(b, 0, 4096 * sizeof(uint32_t));

    while (to_send) {
      int len = to_send > 4096 ? 4096 : to_send;
      int n = fullwrite(t->sock, b, len * 4);

      if (n != len * 4) {
        printf("tcp_bridge: write error ret %d error %s\n", n, strerror(errno));
        abort();
      }

      to_send -= len;
    }
  }

  int n = fullwrite(t->sock, buff[0], nsamps * 4);

  if (n != nsamps * 4) {
    printf("tcp_bridge: write error ret %d (wanted %d) error %s\n", n, nsamps*4, strerror(errno));
    abort();
  }

  t->next_tx_timestamp = timestamp + nsamps;
  return nsamps;
}

int tcp_bridge_read(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc) {
  if (cc != 1) {
    printf("tcp_bridge: only 1 antenna supported\n");
    exit(1);
  }

  tcp_bridge_state_t *t = device->priv;
  int n = fullread(t->sock, buff[0], nsamps * 4);

  if (n != nsamps * 4) {
    printf("tcp_bridge: read error ret %d nsamps*4 %d error %s\n", n, nsamps * 4, strerror(errno));
    abort();
  }

  *timestamp = t->timestamp;
  t->timestamp += nsamps;
  return nsamps;
}

int tcp_bridge_read_ue(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc) {
  if (cc != 1) {
    printf("tcp_bridge: only 1 antenna supported\n");
    exit(1);
  }

  tcp_bridge_state_t *t = device->priv;
  int n;

  /* In synch mode, UE does not write, but we need to
     send something to the eNodeB.
     We know that UE is in synch mode when it reads
     10 subframes at a time.
  */
  if (nsamps == t->samples_per_subframe * 10) {
    uint32_t b[nsamps];
    memset(b, 0, nsamps * 4);
    n = fullwrite(t->sock, b, nsamps * 4);

    if (n != nsamps * 4) {
      printf("tcp_bridge: write error ret %d error %s\n", n, strerror(errno));
      abort();
    }
  }

  return tcp_bridge_read(device, timestamp, buff, nsamps, cc);
}

/* To startup proper communcation between eNB and UE,
   we need to understand that:
   - eNodeB starts reading subframe 0
   - then eNodeB starts sending subframe 4
   and then repeats read/write for each subframe.
   The UE:
   - reads 10 subframes at a time until it is synchronized
   - then reads subframe n and writes subframe n+2
   We also want to enforce that the subframe 0 is read
   at the beginning of the UE RX buffer, not in the middle
   of it.
   So it means:
   - for the eNodeB: let it run as in normal mode (as with a B210)
   - for the UE, on its very first read:
     - we want this read to get data from subframe 0
       but the first write of eNodeB is subframe 4
       so we first need to read and ignore 6 subframes
     - the UE will start its TX only at the subframe 2
       corresponding to the subframe 0 it just read,
       so we need to write 12 subframes before anything
       (the function tcp_bridge_read_ue takes care to
       insert dummy TX data during the synch phase)

   Here is a drawing of the beginning of things to make
   this logic clearer.

   We see that eNB starts RX at subframe 0, starts TX at subfram 4,
   and that UE starts RX at subframe 10 and TX at subframe 12.

   We understand that the UE has to transmit 12 empty
   subframes for the eNodeB to start its processing.

   And because the eNodeB starts its TX at subframe 4 and we
   want the UE to start its RX at subframe 10, we need to
   read and ignore 6 subframes in the UE.

            -------------------------------------------------------------------------
   eNB RX:  | *0* | 1 | 2 | 3 |  4  | 5 | 6 | 7 | 8 | 9 |  10  | 11 |  12  | 13 | 14 ...
            -------------------------------------------------------------------------

            -------------------------------------------------------------------------
   eNB TX:  |  0  | 1 | 2 | 3 | *4* | 5 | 6 | 7 | 8 | 9 |  10  | 11 |  12  | 13 | 14 ...
            -------------------------------------------------------------------------

            -------------------------------------------------------------------------
   UE RX:   |  0  | 1 | 2 | 3 |  4  | 5 | 6 | 7 | 8 | 9 | *10* | 11 |  12  | 13 | 14 ...
            -------------------------------------------------------------------------

            -------------------------------------------------------------------------
   UE TX:   |  0  | 1 | 2 | 3 |  4  | 5 | 6 | 7 | 8 | 9 |  10  | 11 | *12* | 13 | 14 ...
            -------------------------------------------------------------------------

   As a final note, we do TX before RX to ensure that the eNB will
   get some data and send us something so there is no deadlock
   at the beginning of things. Hopefully the kernel buffers for
   the sockets are big enough so that the first (big) TX can
   return to user mode before the buffers are full. If this
   is wrong in some environment, we will need to work by smaller
   units of data at a time.
*/
int tcp_bridge_ue_first_read(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc) {
  if (cc != 1) {
    printf("tcp_bridge: only 1 antenna supported\n");
    exit(1);
  }

  tcp_bridge_state_t *t = device->priv;
  uint32_t b[t->samples_per_subframe * 12];
  memset(b, 0, t->samples_per_subframe * 12 * 4);
  int n = fullwrite(t->sock, b, t->samples_per_subframe * 12 * 4);

  if (n != t->samples_per_subframe * 12 * 4) {
    printf("tcp_bridge: write error ret %d error %s\n", n, strerror(errno));
    abort();
  }

  n = fullread(t->sock, b, t->samples_per_subframe * 6 * 4);

  if (n != t->samples_per_subframe * 6 * 4) {
    printf("tcp_bridge: read error ret %d error %s\n", n, strerror(errno));
    abort();
  }

  /* Due to some unknown bug in the eNB (or UE, or both), the first frames
   * are not correctly generated (or handled), which leads to a bad behavior
   * of the simulator in some cases (seen with 100 RBs: the UE reads a bad
   * MIB and switches to 25 RBs, which results in a deadlock in this driver).
   * Let's skip 10 frames to avoid this issue.
   */
  for (int i = 0; i < 10 * 10; i++) {
    memset(b, 0, t->samples_per_subframe * 4);
    n = fullwrite(t->sock, b, t->samples_per_subframe * 4);

    if (n != t->samples_per_subframe * 4) {
      printf("tcp_bridge: write error ret %d error %s\n", n, strerror(errno));
      abort();
    }

    n = fullread(t->sock, b, t->samples_per_subframe * 4);

    if (n != t->samples_per_subframe * 4) {
      printf("tcp_bridge: read error ret %d error %s\n", n, strerror(errno));
      abort();
    }
  }

  device->trx_read_func = tcp_bridge_read_ue;
  return tcp_bridge_read_ue(device, timestamp, buff, nsamps, cc);
}

__attribute__((__visibility__("default")))
int device_init(openair0_device *device, openair0_config_t *openair0_cfg) {
  tcp_bridge_state_t *tcp_bridge = (tcp_bridge_state_t *)calloc(sizeof(tcp_bridge_state_t),1);

  if ((tcp_bridge->ip=getenv("TCPBRIDGE")) == NULL ) {
    /* for compatibility, we test the ENB environment variable */
    if ((tcp_bridge->ip=getenv("ENODEB")) != NULL ) {
      tcp_bridge->ip=strdup("enb");
    } else {
      tcp_bridge->ip=strdup("127.0.0.1");
    }
  }

  tcp_bridge->is_enb = strncasecmp(tcp_bridge->ip,"enb",3) == 0;
  printf("tcp_bridge: running as %s\n", tcp_bridge->is_enb ? "eNB" : "UE");

  /* only 25, 50 or 100 PRBs handled for the moment */
  if (openair0_cfg[0].sample_rate != 30720000 &&
      openair0_cfg[0].sample_rate != 15360000 &&
      openair0_cfg[0].sample_rate !=  7680000) {
    printf("tcp_bridge: ERROR: only 25, 50 or 100 PRBs supported\n");
    exit(1);
  }

  device->trx_start_func       = tcp_bridge_start;
  device->trx_get_stats_func   = tcp_bridge_get_stats;
  device->trx_reset_stats_func = tcp_bridge_reset_stats;
  device->trx_end_func         = tcp_bridge_end;
  device->trx_stop_func        = tcp_bridge_stop;
  device->trx_set_freq_func    = tcp_bridge_set_freq;
  device->trx_set_gains_func   = tcp_bridge_set_gains;
  device->trx_write_func       = tcp_bridge_write;

  if (tcp_bridge->is_enb) {
    device->trx_read_func      = tcp_bridge_read;
  } else {
    device->trx_read_func      = tcp_bridge_ue_first_read;
  }

  device->priv = tcp_bridge;

  switch ((int)openair0_cfg[0].sample_rate) {
    case 30720000:
      tcp_bridge->samples_per_subframe = 30720;
      break;

    case 15360000:
      tcp_bridge->samples_per_subframe = 15360;
      break;

    case 7680000:
      tcp_bridge->samples_per_subframe = 7680;
      break;
  }

  /* let's pretend to be a b2x0 */
  device->type = USRP_B200_DEV;
  device->openair0_cfg=&openair0_cfg[0];
  return 0;
}
