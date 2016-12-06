#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "database.h"
#include "event.h"
#include "handler.h"
#include "config.h"
#include "utils.h"
#include "packet-mac-lte.h"

#define DEFAULT_IP   "127.0.0.1"
#define DEFAULT_PORT 9999

typedef struct {
  int socket;
  struct sockaddr_in to;
  OBUF buf;
  /* ul */
  int ul_rnti;
  int ul_frame;
  int ul_subframe;
  int ul_data;
  /* dl */
  int dl_rnti;
  int dl_frame;
  int dl_subframe;
  int dl_data;
} ev_data;

void ul(void *_d, event e)
{
  ev_data *d = _d;
  ssize_t ret;
  int fsf;
  int i;

  d->buf.osize = 0;

  PUTS(&d->buf, MAC_LTE_START_STRING);
  PUTC(&d->buf, FDD_RADIO);
  PUTC(&d->buf, DIRECTION_UPLINK);
  PUTC(&d->buf, C_RNTI);

  PUTC(&d->buf, MAC_LTE_RNTI_TAG);
  PUTC(&d->buf, (e.e[d->ul_rnti].i>>8) & 255);
  PUTC(&d->buf, e.e[d->ul_rnti].i & 255);

  /* for newer version of wireshark? */
  fsf = (e.e[d->ul_frame].i << 4) + e.e[d->ul_subframe].i;
  /* for older version? */
  fsf = e.e[d->ul_subframe].i;
  PUTC(&d->buf, MAC_LTE_FRAME_SUBFRAME_TAG);
  PUTC(&d->buf, (fsf>>8) & 255);
  PUTC(&d->buf, fsf & 255);

  PUTC(&d->buf, MAC_LTE_PAYLOAD_TAG);
  for (i = 0; i < e.e[d->ul_data].bsize; i++)
    PUTC(&d->buf, ((char*)e.e[d->ul_data].b)[i]);

  ret = sendto(d->socket, d->buf.obuf, d->buf.osize, 0,
      (struct sockaddr *)&d->to, sizeof(struct sockaddr_in));
  if (ret != d->buf.osize) abort();
}

void dl(void *_d, event e)
{
  ev_data *d = _d;
  ssize_t ret;
  int fsf;
  int i;

  d->buf.osize = 0;

  PUTS(&d->buf, MAC_LTE_START_STRING);
  PUTC(&d->buf, FDD_RADIO);
  PUTC(&d->buf, DIRECTION_DOWNLINK);
  PUTC(&d->buf, C_RNTI);

  PUTC(&d->buf, MAC_LTE_RNTI_TAG);
  PUTC(&d->buf, (e.e[d->dl_rnti].i>>8) & 255);
  PUTC(&d->buf, e.e[d->dl_rnti].i & 255);

  /* for newer version of wireshark? */
  fsf = (e.e[d->dl_frame].i << 4) + e.e[d->dl_subframe].i;
  /* for older version? */
  fsf = e.e[d->dl_subframe].i;
  PUTC(&d->buf, MAC_LTE_FRAME_SUBFRAME_TAG);
  PUTC(&d->buf, (fsf>>8) & 255);
  PUTC(&d->buf, fsf & 255);

  PUTC(&d->buf, MAC_LTE_PAYLOAD_TAG);
  for (i = 0; i < e.e[d->dl_data].bsize; i++)
    PUTC(&d->buf, ((char*)e.e[d->dl_data].b)[i]);

  ret = sendto(d->socket, d->buf.obuf, d->buf.osize, 0,
      (struct sockaddr *)&d->to, sizeof(struct sockaddr_in));
  if (ret != d->buf.osize) abort();
}

void setup_data(ev_data *d, void *database, int ul_id, int dl_id)
{
  database_event_format f;
  int i;

  d->ul_rnti     = -1;
  d->ul_frame    = -1;
  d->ul_subframe = -1;
  d->ul_data     = -1;

  d->dl_rnti     = -1;
  d->dl_frame    = -1;
  d->dl_subframe = -1;
  d->dl_data     = -1;

#define G(var_name, var_type, var) \
  if (!strcmp(f.name[i], var_name)) { \
    if (strcmp(f.type[i], var_type)) goto error; \
    var = i; \
    continue; \
  }

  /* ul: rnti, frame, subframe, data */
  f = get_format(database, ul_id);
  for (i = 0; i < f.count; i++) {
    G("rnti",     "int",    d->ul_rnti);
    G("frame",    "int",    d->ul_frame);
    G("subframe", "int",    d->ul_subframe);
    G("data",     "buffer", d->ul_data);
  }
  if (d->ul_rnti == -1 || d->ul_frame == -1 || d->ul_subframe == -1 ||
      d->ul_data == -1) goto error;

  /* dl: rnti, frame, subframe, data */
  f = get_format(database, dl_id);
  for (i = 0; i < f.count; i++) {
    G("rnti",     "int",    d->dl_rnti);
    G("frame",    "int",    d->dl_frame);
    G("subframe", "int",    d->dl_subframe);
    G("data",     "buffer", d->dl_data);
  }
  if (d->dl_rnti == -1 || d->dl_frame == -1 || d->dl_subframe == -1 ||
      d->dl_data == -1) goto error;

#undef G

  return;

error:
  printf("bad T_messages.txt\n");
  abort();
}

void *receiver(void *_d)
{
  ev_data *d = _d;
  int s;
  char buf[100000];

  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s == -1) { perror("socket"); abort(); }

  if (bind(s, (struct sockaddr *)&d->to, sizeof(struct sockaddr_in)) == -1)
    { perror("bind"); abort(); }

  while (1) {
    if (recv(s, buf, 100000, 0) <= 0) abort();
  }

  return 0;
}

void usage(void)
{
  printf(
"options:\n"
"    -d <database file>        this option is mandatory\n"
"    -i <dump file>            read events from this dump file\n"
"    -ip <IP address>          send packets to this IP address (default %s)\n"
"    -p <port>                 send packets to this port (default %d)\n",
  DEFAULT_IP,
  DEFAULT_PORT
  );
  exit(1);
}

int main(int n, char **v)
{
  char *database_filename = NULL;
  char *input_filename = NULL;
  void *database;
  event_handler *h;
  int in;
  int i;
  int ul_id, dl_id;
  ev_data d;
  char *ip = DEFAULT_IP;
  int port = DEFAULT_PORT;

  memset(&d, 0, sizeof(ev_data));

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-i"))
      { if (i > n-2) usage(); input_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-ip")) { if (i > n-2) usage(); ip = v[++i]; continue; }
    if (!strcmp(v[i], "-p")) {if(i>n-2)usage(); port=atoi(v[++i]); continue; }
    usage();
  }

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  if (input_filename == NULL) {
    printf("ERROR: provide an input file (-i)\n");
    exit(1);
  }

  in = open(input_filename, O_RDONLY);
  if (in == -1) { perror(input_filename); return 1; }

  database = parse_database(database_filename);
  load_config_file(database_filename);

  h = new_handler(database);

  ul_id = event_id_from_name(database, "ENB_MAC_UE_UL_PDU_WITH_DATA");
  dl_id = event_id_from_name(database, "ENB_MAC_UE_DL_PDU_WITH_DATA");
  setup_data(&d, database, ul_id, dl_id);

  register_handler_function(h, ul_id, ul, &d);
  register_handler_function(h, dl_id, dl, &d);

  d.socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (d.socket == -1) { perror("socket"); exit(1); }

  d.to.sin_family = AF_INET;
  d.to.sin_port = htons(port);
  d.to.sin_addr.s_addr = inet_addr(ip);

  new_thread(receiver, &d);

  /* read messages */
  while (1) {
    char v[T_BUFFER_MAX];
    event e;
    e = get_event(in, v, database);
    if (e.type == -1) break;
    if (!(e.type == ul_id || e.type == dl_id)) continue;
    handle_event(h, e);
  }

  return 0;
}
