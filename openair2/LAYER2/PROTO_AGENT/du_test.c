#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "ENB_APP/enb_paramdef.h"
#include "LAYER2/PROTO_AGENT/proto_agent.h"

#define BUF_MAX 1400

void usage(char *prg_name)
{
  fprintf(stderr, "usage: %s <file or ->\n", prg_name);
  fprintf(stderr, " - is stdin\n");
  fprintf(stderr, " received packets are written to stdout\n");
}

long uelapsed(struct timeval *s, struct timeval *e)
{
  return e->tv_sec * 1000000 + e->tv_usec - (s->tv_sec * 1000000 + s->tv_usec);
}


rlc_op_status_t rlc_data_req     (const protocol_ctxt_t* const ctxt_pP,
                                  const srb_flag_t   srb_flagP,
                                  const MBMS_flag_t  MBMS_flagP,
                                  const rb_id_t      rb_idP,
                                  const mui_t        muiP,
                                  confirm_t    confirmP,
                                  sdu_size_t   sdu_sizeP,
                                  mem_block_t *sdu_pP
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                  ,const uint32_t * const sourceL2Id
                                  ,const uint32_t * const destinationL2Id
#endif
                                  )
{
  fwrite(sdu_pP->data, sdu_sizeP, 1, stdout);
  fflush(stdout);
  free_mem_block(sdu_pP, __func__);
  //free(ctxt_pP);
  return 0;
}

void close_proto_agent(void)
{
  proto_agent_stop(0);
}

int main(int argc, char *argv[])
{
  const cudu_params_t params = {
    .local_ipv4_address = "192.168.12.45",
    .local_port = 6465,
    .remote_ipv4_address = "192.168.12.45",
    .remote_port = 6464
  };

  protocol_ctxt_t p;
  memset(&p, 0, sizeof p);
  mem_block_t mem;
  char s[BUF_MAX];
  size_t size, totsize = 0;
  struct timeval t_start, t_end;
  FILE *f;

  if (argc != 2) {
    usage(argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "-") == 0) {
    f = stdin;
  } else {
    f = fopen(argv[1], "r");
  }
  if (!f) {
    fprintf(stderr, "cannot open %s: %s\n", argv[1], strerror(errno));
    return 2;
  }

  pool_buffer_init();
  if (proto_agent_start(0, &params) != 0) {
    fprintf(stderr, "error on proto_agent_start()\n");
    fclose(f);
    return 3;
  }
  atexit(close_proto_agent);

  gettimeofday(&t_start, NULL);
  while ((size = fread(s, 1, BUF_MAX, f)) > 0) {
    usleep(10);
    totsize += size;
    mem.data = &s[0];
    proto_agent_send_pdcp_data_ind(&p, 0, 0, 0, size, &mem);
  }
  gettimeofday(&t_end, NULL);
  fclose(f);
  long us = uelapsed(&t_start, &t_end);
  fprintf(stderr, "read %zu bytes in %ld ms -> %.3fMB/s, %.3fMbps\n",
          totsize, us / 1000, ((float) totsize ) / us,
          ((float) totsize) / us * 8);
  fprintf(stderr, "check files using 'diff afile bfile'\n");

  /* wait, we are possibly receiving data */
  sleep(5);
  return 0;
}

/*
 *********************************************************
 * arbitrary functions, needed for linking (used or not) *
 *********************************************************
 */

boolean_t
pdcp_data_ind(
  const protocol_ctxt_t* const ctxt_pP,
  const srb_flag_t   srb_flagP,
  const MBMS_flag_t  MBMS_flagP,
  const rb_id_t      rb_idP,
  const sdu_size_t   sdu_buffer_sizeP,
  mem_block_t* const sdu_buffer_pP
)
{
  fprintf(stderr, "This should never be called on the DU\n");
  exit(1);
}

pthread_t new_thread(void *(*f)(void *), void *b) {
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att)){
    fprintf(stderr, "pthread_attr_init err\n");
    exit(1);
  }
  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED)) {
    fprintf(stderr, "pthread_attr_setdetachstate err\n");
    exit(1);
  }
  if (pthread_create(&t, &att, f, b)) {
    fprintf(stderr, "pthread_create err\n");
    exit(1);
  }
  if (pthread_attr_destroy(&att)) {
    fprintf(stderr, "pthread_attr_destroy err\n");
    exit(1);
  }

  return t;
}

int log_header(char *log_buffer, int buffsize, int comp, int level,const char *format)
{
  return 0;
}

int config_get(paramdef_t *params,int numparams, char *prefix)
{
  return 0;
}

int config_process_cmdline(paramdef_t *cfgoptions,int numoptions, char *prefix)
{
  return 0;
}
