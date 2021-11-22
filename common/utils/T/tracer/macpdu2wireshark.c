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

#define DEFAULT_LIVE_IP   "127.0.0.1"
#define DEFAULT_LIVE_PORT 2021

#define NO_PREAMBLE -1
#define NO_SR_RNTI -1

typedef struct {
  int socket;
  struct sockaddr_in to;
  OBUF buf;

  /* LTE traces */
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
  /* mib */
  int mib_frame;
  int mib_subframe;
  int mib_data;
  /* RA preamble */
  int preamble_frame;
  int preamble_subframe;
  int preamble_preamble;
  /* RAR */
  int rar_rnti;
  int rar_frame;
  int rar_subframe;
  int rar_data;
  /* SR */
  int sr_rnti;
  int sr_frame;
  int sr_subframe;

  /* NR traces */
  /* NR ul */
  int nr_ul_rnti;
  int nr_ul_frame;
  int nr_ul_slot;
  int nr_ul_data;
  /* NR dl */
  int nr_dl_rnti;
  int nr_dl_frame;
  int nr_dl_slot;
  int nr_dl_data;
  /* NR dl retx */
  int nr_dl_retx_rnti;
  int nr_dl_retx_frame;
  int nr_dl_retx_slot;
  int nr_dl_retx_data;
  /* NR mib */
  int nr_mib_frame;
  int nr_mib_slot;
  int nr_mib_data;
  /* NR RAR */
  int nr_rar_rnti;
  int nr_rar_frame;
  int nr_rar_slot;
  int nr_rar_data;

  /* config */
  int no_mib;
  int no_sib;
  int max_mib;
  int max_sib;
  int live;
  int no_bind;
  /* runtime vars */
  int cur_mib;
  int cur_sib;

  /* hack to report UE id:
   * each time we see a rnti, we allocate the next ue_id
   * (two separate versions for lte and nr)
   */
  int lte_rnti_to_ueid[65536];
  int lte_next_ue_id;
  int nr_rnti_to_ueid[65536];
  int nr_next_ue_id;
} ev_data;

/****************************************************************************/
/* LTE                                                                      */
/****************************************************************************/

void trace_lte(ev_data *d, int direction, int rnti_type, int rnti,
        int frame, int subframe, void *buf, int bufsize, int preamble,
        int sr_rnti)
{
  ssize_t ret;
  int fsf;
  int i;
  d->buf.osize = 0;
  PUTS(&d->buf, MAC_LTE_START_STRING);
  PUTC(&d->buf, FDD_RADIO);
  PUTC(&d->buf, direction);
  PUTC(&d->buf, rnti_type);

  if (rnti_type == C_RNTI || rnti_type == RA_RNTI) {
    PUTC(&d->buf, MAC_LTE_RNTI_TAG);
    PUTC(&d->buf, (rnti>>8) & 255);
    PUTC(&d->buf, rnti & 255);

    /* put UE id */
    if (rnti < 0 || rnti > 65535) { printf("bad rnti!\n"); exit(1); }
    /* if no UE id allocated for this rnti then allocate the next one */
    if (d->lte_rnti_to_ueid[rnti] == -1) {
      d->lte_rnti_to_ueid[rnti] = d->lte_next_ue_id;
      d->lte_next_ue_id++;
    }

    PUTC(&d->buf, MAC_LTE_UEID_TAG);
    PUTC(&d->buf, (d->lte_rnti_to_ueid[rnti]>>8) & 255);
    PUTC(&d->buf, d->lte_rnti_to_ueid[rnti] & 255);
  }

  /* for newer version of wireshark? */
  fsf = (frame << 4) + subframe;
  /* for older version? */
  //fsf = subframe;
  PUTC(&d->buf, MAC_LTE_FRAME_SUBFRAME_TAG);
  PUTC(&d->buf, (fsf>>8) & 255);
  PUTC(&d->buf, fsf & 255);

  if (preamble != NO_PREAMBLE) {
    PUTC(&d->buf, MAC_LTE_SEND_PREAMBLE_TAG);
    PUTC(&d->buf, preamble);
    PUTC(&d->buf, 0); /* rach attempt - always 0 for us (not sure of this) */
  }

  if (sr_rnti != NO_SR_RNTI) {
    PUTC(&d->buf, MAC_LTE_SR_TAG);
    PUTC(&d->buf, 0); /* number of items byte 1 */
    PUTC(&d->buf, 1); /* number of items byte 2 */

    /* put UE id */
    if (sr_rnti < 0 || sr_rnti > 65535) { printf("bad sr rnti!\n"); exit(1); }
    /* if no UE id allocated for this rnti then allocate the next one */
    if (d->lte_rnti_to_ueid[sr_rnti] == -1) {
      d->lte_rnti_to_ueid[sr_rnti] = d->lte_next_ue_id;
      d->lte_next_ue_id++;
    }
    PUTC(&d->buf, (d->lte_rnti_to_ueid[sr_rnti]>>8) & 255);
    PUTC(&d->buf, d->lte_rnti_to_ueid[sr_rnti] & 255);

    PUTC(&d->buf, (sr_rnti>>8) & 255);
    PUTC(&d->buf, sr_rnti & 255);
  }

  PUTC(&d->buf, MAC_LTE_PAYLOAD_TAG);

  for (i = 0; i < bufsize; i++)
    PUTC(&d->buf, ((char *)buf)[i]);

  ret = sendto(d->socket, d->buf.obuf, d->buf.osize, 0,
               (struct sockaddr *)&d->to, sizeof(struct sockaddr_in));

  if (ret != d->buf.osize) abort();
}

void ul(void *_d, event e)
{
  ev_data *d = _d;
  trace_lte(d, DIRECTION_UPLINK, C_RNTI, e.e[d->ul_rnti].i,
            e.e[d->ul_frame].i, e.e[d->ul_subframe].i,
            e.e[d->ul_data].b, e.e[d->ul_data].bsize,
            NO_PREAMBLE, NO_SR_RNTI);
}

void dl(void *_d, event e)
{
  ev_data *d = _d;

  if (e.e[d->dl_rnti].i == 0xffff) {
    if (d->no_sib) return;

    if (d->max_sib && d->cur_sib == d->max_sib) return;

    d->cur_sib++;
  }

  trace_lte(d, DIRECTION_DOWNLINK,
            e.e[d->dl_rnti].i != 0xffff ? C_RNTI : SI_RNTI, e.e[d->dl_rnti].i,
            e.e[d->dl_frame].i, e.e[d->dl_subframe].i,
            e.e[d->dl_data].b, e.e[d->dl_data].bsize,
            NO_PREAMBLE, NO_SR_RNTI);
}

void mib(void *_d, event e)
{
  ev_data *d = _d;

  if (d->no_mib) return;

  if (d->max_mib && d->cur_mib == d->max_mib) return;

  d->cur_mib++;
  trace_lte(d, DIRECTION_DOWNLINK, NO_RNTI, 0,
            e.e[d->mib_frame].i, e.e[d->mib_subframe].i,
            e.e[d->mib_data].b, e.e[d->mib_data].bsize,
            NO_PREAMBLE, NO_SR_RNTI);
}

void preamble(void *_d, event e)
{
  ev_data *d = _d;
  trace_lte(d, DIRECTION_UPLINK, NO_RNTI, 0,
            e.e[d->preamble_frame].i, e.e[d->preamble_subframe].i,
            NULL, 0,
            e.e[d->preamble_preamble].i, NO_SR_RNTI);
}

void rar(void *_d, event e)
{
  ev_data *d = _d;
  trace_lte(d, DIRECTION_DOWNLINK, RA_RNTI, e.e[d->rar_rnti].i,
            e.e[d->rar_frame].i, e.e[d->rar_subframe].i,
            e.e[d->rar_data].b, e.e[d->rar_data].bsize,
            NO_PREAMBLE, NO_SR_RNTI);
}

void sr(void *_d, event e)
{
  ev_data *d = _d;
  trace_lte(d, DIRECTION_UPLINK, NO_RNTI, 0,
            e.e[d->sr_frame].i, e.e[d->sr_subframe].i,
            NULL, 0,
            NO_PREAMBLE, e.e[d->sr_rnti].i);
}

/****************************************************************************/
/* NR                                                                       */
/****************************************************************************/

#define MAC_NR_START_STRING "mac-nr"

#define MAC_NR_PAYLOAD_TAG    0x01
#define MAC_NR_RNTI_TAG       0x02
#define MAC_NR_UEID_TAG       0x03
#define MAC_NR_FRAME_SLOT_TAG 0x07

#define NR_FDD_RADIO 1
#define NR_TDD_RADIO 2

#define NR_DIRECTION_UPLINK   0
#define NR_DIRECTION_DOWNLINK 1

#define NR_NO_RNTI 0
#define NR_RA_RNTI 2
#define NR_C_RNTI  3

void trace_nr(ev_data *d, int direction, int rnti_type, int rnti,
        int frame, int slot, void *buf, int bufsize, int preamble)
{
  ssize_t ret;
  int i;
  d->buf.osize = 0;
  PUTS(&d->buf, MAC_NR_START_STRING);
  PUTC(&d->buf, NR_TDD_RADIO);
  PUTC(&d->buf, direction);
  PUTC(&d->buf, rnti_type);

  if (rnti_type == NR_C_RNTI || rnti_type == NR_RA_RNTI) {
    PUTC(&d->buf, MAC_NR_RNTI_TAG);
    PUTC(&d->buf, (rnti>>8) & 255);
    PUTC(&d->buf, rnti & 255);

    /* put UE id */
    if (rnti < 0 || rnti > 65535) { printf("bad rnti!\n"); exit(1); }
    /* if no UE id allocated for this rnti then allocate the next one */
    if (d->nr_rnti_to_ueid[rnti] == -1) {
      d->nr_rnti_to_ueid[rnti] = d->nr_next_ue_id;
      d->nr_next_ue_id++;
    }

    PUTC(&d->buf, MAC_NR_UEID_TAG);
    PUTC(&d->buf, (d->nr_rnti_to_ueid[rnti]>>8) & 255);
    PUTC(&d->buf, d->nr_rnti_to_ueid[rnti] & 255);
  }

#if 0
  /* for old versions of wireshark; not sure if correct */
  int fsf = (frame << 4) + slot;
  PUTC(&d->buf, 4 /* MAC_NR_FRAME_SUBFRAME_TAG */);
  PUTC(&d->buf, (fsf>>8) & 255);
  PUTC(&d->buf, fsf & 255);
#else
  PUTC(&d->buf, MAC_NR_FRAME_SLOT_TAG);
  PUTC(&d->buf, (frame>>8) & 255);
  PUTC(&d->buf, frame & 255);
  PUTC(&d->buf, (slot>>8) & 255);
  PUTC(&d->buf, slot & 255);
#endif

  PUTC(&d->buf, MAC_NR_PAYLOAD_TAG);

  for (i = 0; i < bufsize; i++)
    PUTC(&d->buf, ((char *)buf)[i]);

  ret = sendto(d->socket, d->buf.obuf, d->buf.osize, 0,
               (struct sockaddr *)&d->to, sizeof(struct sockaddr_in));

  if (ret != d->buf.osize) abort();
}

void nr_ul(void *_d, event e)
{
  ev_data *d = _d;

  trace_nr(d, NR_DIRECTION_UPLINK, NR_C_RNTI, e.e[d->nr_ul_rnti].i,
           e.e[d->nr_ul_frame].i, e.e[d->nr_ul_slot].i,
           e.e[d->nr_ul_data].b, e.e[d->nr_ul_data].bsize, NO_PREAMBLE);
}

void nr_dl(void *_d, event e)
{
  ev_data *d = _d;

  trace_nr(d, NR_DIRECTION_DOWNLINK, NR_C_RNTI, e.e[d->nr_dl_rnti].i,
           e.e[d->nr_dl_frame].i, e.e[d->nr_dl_slot].i,
           e.e[d->nr_dl_data].b, e.e[d->nr_dl_data].bsize, NO_PREAMBLE);
}

void nr_dl_retx(void *_d, event e)
{
  ev_data *d = _d;

  trace_nr(d, NR_DIRECTION_DOWNLINK, NR_C_RNTI, e.e[d->nr_dl_retx_rnti].i,
           e.e[d->nr_dl_retx_frame].i, e.e[d->nr_dl_retx_slot].i,
           e.e[d->nr_dl_retx_data].b, e.e[d->nr_dl_retx_data].bsize,
           NO_PREAMBLE);
}

void nr_mib(void *_d, event e)
{
  ev_data *d = _d;

  if (d->no_mib) return;

  if (d->max_mib && d->cur_mib == d->max_mib) return;

  d->cur_mib++;

  trace_nr(d, NR_DIRECTION_DOWNLINK, NR_NO_RNTI, 0,
           e.e[d->nr_mib_frame].i, e.e[d->nr_mib_slot].i,
           e.e[d->nr_mib_data].b, e.e[d->nr_mib_data].bsize, NO_PREAMBLE);
}

void nr_rar(void *_d, event e)
{
  ev_data *d = _d;

  trace_nr(d, NR_DIRECTION_DOWNLINK, NR_RA_RNTI, e.e[d->nr_rar_rnti].i,
           e.e[d->nr_rar_frame].i, e.e[d->nr_rar_slot].i,
           e.e[d->nr_rar_data].b, e.e[d->nr_rar_data].bsize, NO_PREAMBLE);
}

/****************************************************************************/
/****************************************************************************/

void setup_data(ev_data *d, void *database, int ul_id, int dl_id, int mib_id,
                int preamble_id, int rar_id, int sr_id,
                int nr_ul_id, int nr_dl_id, int nr_dl_retx_id, int nr_mib_id,
                int nr_rar_id)
{
  database_event_format f;
  int i;
  d->ul_rnti           = -1;
  d->ul_frame          = -1;
  d->ul_subframe       = -1;
  d->ul_data           = -1;
  d->dl_rnti           = -1;
  d->dl_frame          = -1;
  d->dl_subframe       = -1;
  d->dl_data           = -1;
  d->mib_frame         = -1;
  d->mib_subframe      = -1;
  d->mib_data          = -1;
  d->preamble_frame    = -1;
  d->preamble_subframe = -1;
  d->preamble_preamble = -1;
  d->rar_rnti          = -1;
  d->rar_frame         = -1;
  d->rar_subframe      = -1;
  d->rar_data          = -1;
  d->sr_rnti           = -1;
  d->sr_frame          = -1;
  d->sr_subframe       = -1;

  d->nr_ul_rnti        = -1;
  d->nr_ul_frame       = -1;
  d->nr_ul_slot        = -1;
  d->nr_ul_data        = -1;
  d->nr_dl_rnti        = -1;
  d->nr_dl_frame       = -1;
  d->nr_dl_slot        = -1;
  d->nr_dl_data        = -1;
  d->nr_dl_retx_rnti   = -1;
  d->nr_dl_retx_frame  = -1;
  d->nr_dl_retx_slot   = -1;
  d->nr_dl_retx_data   = -1;
  d->nr_mib_frame      = -1;
  d->nr_mib_slot       = -1;
  d->nr_mib_data       = -1;
  d->nr_rar_rnti       = -1;
  d->nr_rar_frame      = -1;
  d->nr_rar_slot       = -1;
  d->nr_rar_data       = -1;

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

  /* MIB: frame, subframe, data */
  f = get_format(database, mib_id);

  for (i = 0; i < f.count; i++) {
    G("frame",    "int",    d->mib_frame);
    G("subframe", "int",    d->mib_subframe);
    G("data",     "buffer", d->mib_data);
  }

  if (d->mib_frame == -1 || d->mib_subframe == -1 || d->mib_data == -1)
    goto error;

  /* preamble: frame, subframe, preamble */
  f = get_format(database, preamble_id);

  for (i = 0; i < f.count; i++) {
    G("frame",    "int", d->preamble_frame);
    G("subframe", "int", d->preamble_subframe);
    G("preamble", "int", d->preamble_preamble);
  }

  if (d->preamble_frame == -1 || d->preamble_subframe == -1 ||
      d->preamble_preamble == -1) goto error;

  /* rar: rnti, frame, subframe, data */
  f = get_format(database, rar_id);

  for (i = 0; i < f.count; i++) {
    G("rnti",     "int",    d->rar_rnti);
    G("frame",    "int",    d->rar_frame);
    G("subframe", "int",    d->rar_subframe);
    G("data",     "buffer", d->rar_data);
  }

  if (d->rar_rnti == -1 || d->rar_frame == -1 || d->rar_subframe == -1 ||
      d->rar_data == -1) goto error;

  /* sr: rnti, frame, subframe */
  f = get_format(database, sr_id);

  for (i = 0; i < f.count; i++) {
    G("rnti",     "int",    d->sr_rnti);
    G("frame",    "int",    d->sr_frame);
    G("subframe", "int",    d->sr_subframe);
  }

  if (d->sr_rnti == -1 || d->sr_frame == -1 || d->sr_subframe == -1)
    goto error;

  /* NR ul: rnti, frame, slot, data */
  f = get_format(database, nr_ul_id);

  for (i = 0; i < f.count; i++) {
    G("rnti",  "int",    d->nr_ul_rnti);
    G("frame", "int",    d->nr_ul_frame);
    G("slot",  "int",    d->nr_ul_slot);
    G("data",  "buffer", d->nr_ul_data);
  }

  if (d->nr_ul_rnti == -1 || d->nr_ul_frame == -1 || d->nr_ul_slot == -1 ||
      d->nr_ul_data == -1)
    goto error;

  /* NR dl: rnti, frame, slot, data */
  f = get_format(database, nr_dl_id);

  for (i = 0; i < f.count; i++) {
    G("rnti",  "int",    d->nr_dl_rnti);
    G("frame", "int",    d->nr_dl_frame);
    G("slot",  "int",    d->nr_dl_slot);
    G("data",  "buffer", d->nr_dl_data);
  }

  if (d->nr_dl_rnti == -1 || d->nr_dl_frame == -1 || d->nr_dl_slot == -1 ||
      d->nr_dl_data == -1)
    goto error;

  /* NR dl retx: rnti, frame, slot, data */
  f = get_format(database, nr_dl_retx_id);

  for (i = 0; i < f.count; i++) {
    G("rnti",  "int",    d->nr_dl_retx_rnti);
    G("frame", "int",    d->nr_dl_retx_frame);
    G("slot",  "int",    d->nr_dl_retx_slot);
    G("data",  "buffer", d->nr_dl_retx_data);
  }

  if (d->nr_dl_retx_rnti == -1 || d->nr_dl_retx_frame == -1 ||
      d->nr_dl_retx_slot == -1 || d->nr_dl_retx_data == -1)
    goto error;

  /* NR MIB: frame, slot, data */
  f = get_format(database, nr_mib_id);

  for (i = 0; i < f.count; i++) {
    G("frame", "int",    d->nr_mib_frame);
    G("slot",  "int",    d->nr_mib_slot);
    G("data",  "buffer", d->nr_mib_data);
  }

  if (d->nr_mib_frame == -1 || d->nr_mib_slot== -1 || d->nr_mib_data == -1)
    goto error;

  /* NR RAR: rnti, frame, slot, data */
  f = get_format(database, nr_rar_id);

  for (i = 0; i < f.count; i++) {
    G("rnti",  "int",    d->nr_rar_rnti);
    G("frame", "int",    d->nr_rar_frame);
    G("slot",  "int",    d->nr_rar_slot);
    G("data",  "buffer", d->nr_rar_data);
  }

  if (d->nr_rar_rnti == -1 || d->nr_rar_frame == -1 || d->nr_rar_slot == -1 ||
      d->nr_rar_data == -1)
    goto error;

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

  if (s == -1) {
    perror("socket");
    abort();
  }

  if (d->no_bind == 0) {
    if (bind(s, (struct sockaddr *)&d->to, sizeof(struct sockaddr_in)) == -1) {
      perror("bind");
      abort();
    }
  }

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
    "    -p <port>                 send packets to this port (default %d)\n"
    "    -no-mib                   do not report MIB\n"
    "    -no-sib                   do not report SIBs\n"
    "    -max-mib <n>              report at maximum n MIB\n"
    "    -max-sib <n>              report at maximum n SIBs\n"
    "    -live                     run live\n"
    "    -live-ip <IP address>     tracee's IP address (default %s)\n"
    "    -live-port <port>         tracee's port (default %d)\n"
    "    -no-bind                  don't bind to IP address (for remote logging)\n"
    "-i and -live are mutually exclusive options. One of them must be provided\n"
    "but not both.\n",
    DEFAULT_IP,
    DEFAULT_PORT,
    DEFAULT_LIVE_IP,
    DEFAULT_LIVE_PORT
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
  int ul_id, dl_id, mib_id, preamble_id, rar_id;
  int nr_ul_id, nr_dl_id, nr_dl_retx_id, nr_mib_id, nr_rar_id;
  int sr_id;
  ev_data d;
  char *ip = DEFAULT_IP;
  int port = DEFAULT_PORT;
  char *live_ip = DEFAULT_LIVE_IP;
  int live_port = DEFAULT_LIVE_PORT;
  int live = 0;
  memset(&d, 0, sizeof(ev_data));

  for (i = 0; i < 65536; i++) {
    d.lte_rnti_to_ueid[i] = -1;
    d.nr_rnti_to_ueid[i] = -1;
  }

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))         { if(i>n-2)usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-i"))         { if(i>n-2)usage(); input_filename = v[++i];    continue; }
    if (!strcmp(v[i], "-ip"))        { if(i>n-2)usage(); ip = v[++i];                continue; }
    if (!strcmp(v[i], "-p"))         { if(i>n-2)usage(); port = atoi(v[++i]);        continue; }
    if (!strcmp(v[i], "-no-mib"))    {                   d.no_mib = 1;               continue; }
    if (!strcmp(v[i], "-no-sib"))    {                   d.no_sib = 1;               continue; }
    if (!strcmp(v[i], "-max-mib"))   { if(i>n-2)usage(); d.max_mib = atoi(v[++i]);   continue; }
    if (!strcmp(v[i], "-max-sib"))   { if(i>n-2)usage(); d.max_sib = atoi(v[++i]);   continue; }
    if (!strcmp(v[i], "-live"))      {                   live = 1;                   continue; }
    if (!strcmp(v[i], "-live-ip"))   { if(i>n-2)usage(); live_ip = v[++i];           continue; }
    if (!strcmp(v[i], "-live-port")) { if(i>n-2)usage(); live_port = atoi(v[++i]);   continue; }
    if (!strcmp(v[i], "-no-bind"))   {                   d.no_bind = 1;              continue; }
    usage();
  }

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  if (input_filename == NULL && live == 0) {
    printf("ERROR: provide an input file (-i) or run live (-live)\n");
    exit(1);
  }

  if (input_filename != NULL && live != 0) {
    printf("ERROR: cannot use both -i and -live\n");
    exit(1);
  }

  if (live == 0) {
    in = open(input_filename, O_RDONLY);

    if (in == -1) {
      perror(input_filename);
      return 1;
    }
  } else
    in = connect_to(live_ip, live_port);

  database = parse_database(database_filename);
  load_config_file(database_filename);
  h = new_handler(database);

  if (live) {
    char mt = 1;
    int  number_of_events = number_of_ids(database);
    int *is_on = calloc(number_of_events, sizeof(int));

    if (is_on == NULL) {
      printf("ERROR: out of memory\n");
      exit(1);
    }

    on_off(database, "ENB_MAC_UE_UL_PDU_WITH_DATA", is_on, 1);
    on_off(database, "ENB_MAC_UE_DL_PDU_WITH_DATA", is_on, 1);
    on_off(database, "ENB_PHY_MIB", is_on, 1);
    on_off(database, "ENB_PHY_INITIATE_RA_PROCEDURE", is_on, 1);
    on_off(database, "ENB_MAC_UE_DL_RAR_PDU_WITH_DATA", is_on, 1);
    on_off(database, "ENB_MAC_SCHEDULING_REQUEST", is_on, 1);

    on_off(database, "GNB_MAC_UL_PDU_WITH_DATA", is_on, 1);
    on_off(database, "GNB_MAC_DL_PDU_WITH_DATA", is_on, 1);
    on_off(database, "GNB_MAC_RETRANSMISSION_DL_PDU_WITH_DATA", is_on, 1);
    on_off(database, "GNB_PHY_MIB", is_on, 1);
    on_off(database, "GNB_MAC_DL_RAR_PDU_WITH_DATA", is_on, 1);

    /* activate selected traces */
    if (socket_send(in, &mt, 1) == -1 ||
        socket_send(in, &number_of_events, sizeof(int)) == -1 ||
        socket_send(in, is_on, number_of_events * sizeof(int)) == -1) {
      printf("ERROR: socket_send failed\n");
      exit(1);
    }

    free(is_on);
  }

  ul_id = event_id_from_name(database, "ENB_MAC_UE_UL_PDU_WITH_DATA");
  dl_id = event_id_from_name(database, "ENB_MAC_UE_DL_PDU_WITH_DATA");
  mib_id = event_id_from_name(database, "ENB_PHY_MIB");
  preamble_id = event_id_from_name(database, "ENB_PHY_INITIATE_RA_PROCEDURE");
  rar_id = event_id_from_name(database, "ENB_MAC_UE_DL_RAR_PDU_WITH_DATA");
  sr_id = event_id_from_name(database, "ENB_MAC_SCHEDULING_REQUEST");

  nr_ul_id = event_id_from_name(database, "GNB_MAC_UL_PDU_WITH_DATA");
  nr_dl_id = event_id_from_name(database, "GNB_MAC_DL_PDU_WITH_DATA");
  nr_dl_retx_id = event_id_from_name(database, "GNB_MAC_RETRANSMISSION_DL_PDU_WITH_DATA");
  nr_mib_id = event_id_from_name(database, "GNB_PHY_MIB");
  nr_rar_id = event_id_from_name(database, "GNB_MAC_DL_RAR_PDU_WITH_DATA");

  setup_data(&d, database, ul_id, dl_id, mib_id, preamble_id, rar_id, sr_id,
             nr_ul_id, nr_dl_id, nr_dl_retx_id, nr_mib_id, nr_rar_id);

  register_handler_function(h, ul_id, ul, &d);
  register_handler_function(h, dl_id, dl, &d);
  register_handler_function(h, mib_id, mib, &d);
  register_handler_function(h, preamble_id, preamble, &d);
  register_handler_function(h, rar_id, rar, &d);
  register_handler_function(h, sr_id, sr, &d);

  register_handler_function(h, nr_ul_id, nr_ul, &d);
  register_handler_function(h, nr_dl_id, nr_dl, &d);
  register_handler_function(h, nr_dl_retx_id, nr_dl_retx, &d);
  register_handler_function(h, nr_mib_id, nr_mib, &d);
  register_handler_function(h, nr_rar_id, nr_rar, &d);

  d.socket = socket(AF_INET, SOCK_DGRAM, 0);

  if (d.socket == -1) {
    perror("socket");
    exit(1);
  }

  d.to.sin_family = AF_INET;
  d.to.sin_port = htons(port);
  d.to.sin_addr.s_addr = inet_addr(ip);
  new_thread(receiver, &d);
  OBUF ebuf = { osize: 0, omaxsize: 0, obuf: NULL };

  /* read messages */
  while (1) {
    event e;
    e = get_event(in, &ebuf, database);

    if (e.type == -1) break;

    if (!(e.type == ul_id         || e.type == dl_id     || e.type == mib_id ||
          e.type == preamble_id   || e.type == rar_id    || e.type == sr_id  ||
          e.type == nr_ul_id      || e.type == nr_dl_id  ||
          e.type == nr_dl_retx_id || e.type == nr_mib_id ||
          e.type == nr_rar_id)) continue;

    handle_event(h, e);
  }

  return 0;
}
