#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>

#include "defs.h"

#define T_ID(x) x
#include "../T_IDs.h"
#include "../T_defs.h"

#define BLUE "#0c0c72"
#define RED "#d72828"

void *ul_plot;
void *chest_plot;
void *pusch_iq_plot;
void *pucch_iq_plot;
void *pucch_plot;

#ifdef T_USE_SHARED_MEMORY

T_cache_t *T_cache;
int T_busylist_head;
int T_pos;

static inline int GET(int s, void *out, int count)
{
  if (count == 1) {
    *(char *)out = T_cache[T_busylist_head].buffer[T_pos];
    T_pos++;
    return 1;
  }
  memcpy(out, T_cache[T_busylist_head].buffer + T_pos, count);
  T_pos += count;
  return count;
}

#else /* T_USE_SHARED_MEMORY */

#define GET fullread

int fullread(int fd, void *_buf, int count)
{
  char *buf = _buf;
  int ret = 0;
  int l;
  while (count) {
    l = read(fd, buf, count);
    if (l <= 0) { printf("read socket problem\n"); abort(); }
    count -= l;
    buf += l;
    ret += l;
  }
  return ret;
}

#endif /* T_USE_SHARED_MEMORY */

int get_connection(char *addr, int port)
{
  struct sockaddr_in a;
  socklen_t alen;
  int s, t;

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) { perror("socket"); exit(1); }
  t = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int)))
    { perror("setsockopt"); exit(1); }

  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = inet_addr(addr);

  if (bind(s, (struct sockaddr *)&a, sizeof(a))) { perror("bind"); exit(1); }
  if (listen(s, 5)) { perror("bind"); exit(1); }
  alen = sizeof(a);
  t = accept(s, (struct sockaddr *)&a, &alen);
  if (t == -1) { perror("accept"); exit(1); }
  close(s);
  return t;
}

void get_string(int s, char *out)
{
  while (1) {
    if (GET(s, out, 1) != 1) abort();
    if (*out == 0) break;
    out++;
  }
}

void get_message(int s)
{
#define S(x, y) do { \
    char str[T_BUFFER_MAX]; \
    get_string(s, str); \
    printf("["x"]["y"] %s", str); \
  } while (0)

  int m;
#ifdef T_USE_SHARED_MEMORY
  T_pos = 0;
#endif
  if (GET(s, &m, sizeof(int)) != sizeof(int)) abort();
  switch (m) {
  case T_first: {
    char str[T_BUFFER_MAX];
    get_string(s, str);
    printf("%s", str);
    break;
  }
  case T_LEGACY_MAC_INFO: S("MAC", "INFO"); break;
  case T_LEGACY_MAC_ERROR: S("MAC", "ERROR"); break;
  case T_LEGACY_MAC_WARNING: S("MAC", "WARNING"); break;
  case T_LEGACY_MAC_DEBUG: S("MAC", "DEBUG"); break;
  case T_LEGACY_MAC_TRACE: S("MAC", "TRACE"); break;
  case T_LEGACY_PHY_INFO: S("PHY", "INFO"); break;
  case T_LEGACY_PHY_ERROR: S("PHY", "ERROR"); break;
  case T_LEGACY_PHY_WARNING: S("PHY", "WARNING"); break;
  case T_LEGACY_PHY_DEBUG: S("PHY", "DEBUG"); break;
  case T_LEGACY_PHY_TRACE: S("PHY", "TRACE"); break;
  case T_LEGACY_S1AP_INFO: S("S1AP", "INFO"); break;
  case T_LEGACY_S1AP_ERROR: S("S1AP", "ERROR"); break;
  case T_LEGACY_S1AP_WARNING: S("S1AP", "WARNING"); break;
  case T_LEGACY_S1AP_DEBUG: S("S1AP", "DEBUG"); break;
  case T_LEGACY_S1AP_TRACE: S("S1AP", "TRACE"); break;
  case T_LEGACY_RRC_INFO: S("RRC", "INFO"); break;
  case T_LEGACY_RRC_ERROR: S("RRC", "ERROR"); break;
  case T_LEGACY_RRC_WARNING: S("RRC", "WARNING"); break;
  case T_LEGACY_RRC_DEBUG: S("RRC", "DEBUG"); break;
  case T_LEGACY_RRC_TRACE: S("RRC", "TRACE"); break;
  case T_LEGACY_RLC_INFO: S("RLC", "INFO"); break;
  case T_LEGACY_RLC_ERROR: S("RLC", "ERROR"); break;
  case T_LEGACY_RLC_WARNING: S("RLC", "WARNING"); break;
  case T_LEGACY_RLC_DEBUG: S("RLC", "DEBUG"); break;
  case T_LEGACY_RLC_TRACE: S("RLC", "TRACE"); break;
  case T_LEGACY_PDCP_INFO: S("PDCP", "INFO"); break;
  case T_LEGACY_PDCP_ERROR: S("PDCP", "ERROR"); break;
  case T_LEGACY_PDCP_WARNING: S("PDCP", "WARNING"); break;
  case T_LEGACY_PDCP_DEBUG: S("PDCP", "DEBUG"); break;
  case T_LEGACY_PDCP_TRACE: S("PDCP", "TRACE"); break;
  case T_LEGACY_ENB_APP_INFO: S("ENB_APP", "INFO"); break;
  case T_LEGACY_ENB_APP_ERROR: S("ENB_APP", "ERROR"); break;
  case T_LEGACY_ENB_APP_WARNING: S("ENB_APP", "WARNING"); break;
  case T_LEGACY_ENB_APP_DEBUG: S("ENB_APP", "DEBUG"); break;
  case T_LEGACY_ENB_APP_TRACE: S("ENB_APP", "TRACE"); break;
  case T_LEGACY_SCTP_INFO: S("SCTP", "INFO"); break;
  case T_LEGACY_SCTP_ERROR: S("SCTP", "ERROR"); break;
  case T_LEGACY_SCTP_WARNING: S("SCTP", "WARNING"); break;
  case T_LEGACY_SCTP_DEBUG: S("SCTP", "DEBUG"); break;
  case T_LEGACY_SCTP_TRACE: S("SCTP", "TRACE"); break;
  case T_LEGACY_UDP__INFO: S("UDP", "INFO"); break;
  case T_LEGACY_UDP__ERROR: S("UDP", "ERROR"); break;
  case T_LEGACY_UDP__WARNING: S("UDP", "WARNING"); break;
  case T_LEGACY_UDP__DEBUG: S("UDP", "DEBUG"); break;
  case T_LEGACY_UDP__TRACE: S("UDP", "TRACE"); break;
  case T_LEGACY_NAS_INFO: S("NAS", "INFO"); break;
  case T_LEGACY_NAS_ERROR: S("NAS", "ERROR"); break;
  case T_LEGACY_NAS_WARNING: S("NAS", "WARNING"); break;
  case T_LEGACY_NAS_DEBUG: S("NAS", "DEBUG"); break;
  case T_LEGACY_NAS_TRACE: S("NAS", "TRACE"); break;
  case T_LEGACY_HW_INFO: S("HW", "INFO"); break;
  case T_LEGACY_HW_ERROR: S("HW", "ERROR"); break;
  case T_LEGACY_HW_WARNING: S("HW", "WARNING"); break;
  case T_LEGACY_HW_DEBUG: S("HW", "DEBUG"); break;
  case T_LEGACY_HW_TRACE: S("HW", "TRACE"); break;
  case T_LEGACY_EMU_INFO: S("EMU", "INFO"); break;
  case T_LEGACY_EMU_ERROR: S("EMU", "ERROR"); break;
  case T_LEGACY_EMU_WARNING: S("EMU", "WARNING"); break;
  case T_LEGACY_EMU_DEBUG: S("EMU", "DEBUG"); break;
  case T_LEGACY_EMU_TRACE: S("EMU", "TRACE"); break;
  case T_LEGACY_OTG_INFO: S("OTG", "INFO"); break;
  case T_LEGACY_OTG_ERROR: S("OTG", "ERROR"); break;
  case T_LEGACY_OTG_WARNING: S("OTG", "WARNING"); break;
  case T_LEGACY_OTG_DEBUG: S("OTG", "DEBUG"); break;
  case T_LEGACY_OTG_TRACE: S("OTG", "TRACE"); break;
  case T_LEGACY_OCG_INFO: S("OCG", "INFO"); break;
  case T_LEGACY_OCG_ERROR: S("OCG", "ERROR"); break;
  case T_LEGACY_OCG_WARNING: S("OCG", "WARNING"); break;
  case T_LEGACY_OCG_DEBUG: S("OCG", "DEBUG"); break;
  case T_LEGACY_OCG_TRACE: S("OCG", "TRACE"); break;
  case T_LEGACY_OCM_INFO: S("OCM", "INFO"); break;
  case T_LEGACY_OCM_ERROR: S("OCM", "ERROR"); break;
  case T_LEGACY_OCM_WARNING: S("OCM", "WARNING"); break;
  case T_LEGACY_OCM_DEBUG: S("OCM", "DEBUG"); break;
  case T_LEGACY_OCM_TRACE: S("OCM", "TRACE"); break;
  case T_LEGACY_OMG_INFO: S("OMG", "INFO"); break;
  case T_LEGACY_OMG_ERROR: S("OMG", "ERROR"); break;
  case T_LEGACY_OMG_WARNING: S("OMG", "WARNING"); break;
  case T_LEGACY_OMG_DEBUG: S("OMG", "DEBUG"); break;
  case T_LEGACY_OMG_TRACE: S("OMG", "TRACE"); break;
  case T_LEGACY_OIP_INFO: S("OIP", "INFO"); break;
  case T_LEGACY_OIP_ERROR: S("OIP", "ERROR"); break;
  case T_LEGACY_OIP_WARNING: S("OIP", "WARNING"); break;
  case T_LEGACY_OIP_DEBUG: S("OIP", "DEBUG"); break;
  case T_LEGACY_OIP_TRACE: S("OIP", "TRACE"); break;
  case T_LEGACY_GTPU_INFO: S("GTPU", "INFO"); break;
  case T_LEGACY_GTPU_ERROR: S("GTPU", "ERROR"); break;
  case T_LEGACY_GTPU_WARNING: S("GTPU", "WARNING"); break;
  case T_LEGACY_GTPU_DEBUG: S("GTPU", "DEBUG"); break;
  case T_LEGACY_GTPU_TRACE: S("GTPU", "TRACE"); break;
  case T_LEGACY_TMR_INFO: S("TMR", "INFO"); break;
  case T_LEGACY_TMR_ERROR: S("TMR", "ERROR"); break;
  case T_LEGACY_TMR_WARNING: S("TMR", "WARNING"); break;
  case T_LEGACY_TMR_DEBUG: S("TMR", "DEBUG"); break;
  case T_LEGACY_TMR_TRACE: S("TMR", "TRACE"); break;
  case T_LEGACY_OSA_INFO: S("OSA", "INFO"); break;
  case T_LEGACY_OSA_ERROR: S("OSA", "ERROR"); break;
  case T_LEGACY_OSA_WARNING: S("OSA", "WARNING"); break;
  case T_LEGACY_OSA_DEBUG: S("OSA", "DEBUG"); break;
  case T_LEGACY_OSA_TRACE: S("OSA", "TRACE"); break;
  case T_LEGACY_component_INFO: S("XXX", "INFO"); break;
  case T_LEGACY_component_ERROR: S("XXX", "ERROR"); break;
  case T_LEGACY_component_WARNING: S("XXX", "WARNING"); break;
  case T_LEGACY_component_DEBUG: S("XXX", "DEBUG"); break;
  case T_LEGACY_component_TRACE: S("XXX", "TRACE"); break;
  case T_LEGACY_componentP_INFO: S("XXX", "INFO"); break;
  case T_LEGACY_componentP_ERROR: S("XXX", "ERROR"); break;
  case T_LEGACY_componentP_WARNING: S("XXX", "WARNING"); break;
  case T_LEGACY_componentP_DEBUG: S("XXX", "DEBUG"); break;
  case T_LEGACY_componentP_TRACE: S("XXX", "TRACE"); break;
  case T_LEGACY_CLI_INFO: S("CLI", "INFO"); break;
  case T_LEGACY_CLI_ERROR: S("CLI", "ERROR"); break;
  case T_LEGACY_CLI_WARNING: S("CLI", "WARNING"); break;
  case T_LEGACY_CLI_DEBUG: S("CLI", "DEBUG"); break;
  case T_LEGACY_CLI_TRACE: S("CLI", "TRACE"); break;
  case T_ENB_INPUT_SIGNAL: {
    unsigned char buf[T_BUFFER_MAX];
    int size;
    int eNB, frame, subframe, antenna;
    GET(s, &eNB, sizeof(int));
    GET(s, &frame, sizeof(int));
    GET(s, &subframe, sizeof(int));
    GET(s, &antenna, sizeof(int));
    GET(s, &size, sizeof(int));
    GET(s, buf, size);
#if 0
    printf("got T_ENB_INPUT_SIGNAL eNB %d frame %d subframe %d antenna "
           "%d size %d %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x "
           "%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n",
           eNB, frame, subframe, antenna, size, buf[0],buf[1],buf[2],
           buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],
           buf[11],buf[12],buf[13],buf[14],buf[15]);
#endif
    if (size != 4 * 7680)
      {printf("bad T_ENB_INPUT_SIGNAL, only 7680 samples allowed "
              "(received %d bytes = %d samples)\n", size, size/4);abort();}
    if (ul_plot) iq_plot_set(ul_plot, (short*)buf, 7680, subframe*7680, 0);
    break;
  }
  case T_ENB_UL_CHANNEL_ESTIMATE: {
    unsigned char buf[T_BUFFER_MAX];
    int size;
    int eNB, UE, frame, subframe, antenna;
    GET(s, &eNB, sizeof(int));
    GET(s, &UE, sizeof(int));
    GET(s, &frame, sizeof(int));
    GET(s, &subframe, sizeof(int));
    GET(s, &antenna, sizeof(int));
    GET(s, &size, sizeof(int));
    GET(s, buf, size);
    if (size != 512*4)
      {printf("bad T_ENB_UL_CHANNEL_ESTIMATE, only 512 samples allowed\n");
       abort();}
    if (chest_plot) iq_plot_set(chest_plot, (short*)buf, 512, 0, 0);
    break;
  }
  case T_PUSCH_IQ: {
    unsigned char buf[T_BUFFER_MAX];
    int size;
    int eNB, UE, frame, subframe, nb_rb;
    GET(s, &eNB, sizeof(int));
    GET(s, &UE, sizeof(int));
    GET(s, &frame, sizeof(int));
    GET(s, &subframe, sizeof(int));
    GET(s, &nb_rb, sizeof(int));
    GET(s, &size, sizeof(int));
    GET(s, buf, size);
    if (size != 12*25*14*4)
      {printf("bad T_PUSCH_IQ, we want 25 RBs and 14 symbols/TTI\n");
       abort();}
    if (pusch_iq_plot) {
      uint32_t *src, *dst;
      int i, l;
      dst = (uint32_t*)buf;
      for (l = 0; l < 14; l++) {
        src = (uint32_t*)buf + l * 12 * 25;
        for (i = 0; i < nb_rb*12; i++) *dst++ = *src++;
      }
      iq_plot_set_sized(pusch_iq_plot, (short*)buf, nb_rb*12*14, 0);
    }
    break;
  }
  case T_PUCCH_1AB_IQ: {
    int eNB, UE, frame, subframe, I, Q;
    GET(s, &eNB, sizeof(int));
    GET(s, &UE, sizeof(int));
    GET(s, &frame, sizeof(int));
    GET(s, &subframe, sizeof(int));
    GET(s, &I, sizeof(int));
    GET(s, &Q, sizeof(int));
    if (pucch_iq_plot) iq_plot_add_iq_point_loop(pucch_iq_plot,I*10,Q*10, 0);
    break;
  }
  case T_PUCCH_1_ENERGY: {
    int eNB, UE, frame, subframe, e, t;
    GET(s, &eNB, sizeof(int));
    GET(s, &UE, sizeof(int));
    GET(s, &frame, sizeof(int));
    GET(s, &subframe, sizeof(int));
    GET(s, &e, sizeof(int));
    GET(s, &t, sizeof(int));
//printf("t %d e %d\n", t, (int)(10*log10(e)));
    if (pucch_plot) {
      iq_plot_add_energy_point_loop(pucch_plot, t, 0);
      iq_plot_add_energy_point_loop(pucch_plot, 10*log10(e), 1);
    }
    break;
  }
  case T_buf_test: {
    unsigned char buf[T_BUFFER_MAX];
    int size;
    GET(s, &size, sizeof(int));
    GET(s, buf, size);
    printf("got buffer size %d %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x"
           " %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n",
           size, buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],
           buf[8],buf[9],buf[10],buf[11],buf[12],buf[13],buf[14],buf[15]);
    break;
  }
  default: printf("unkown message type %d\n", m); abort();
  }

#ifdef T_USE_SHARED_MEMORY
  T_cache[T_busylist_head].busy = 0;
  T_busylist_head++;
  T_busylist_head &= T_CACHE_SIZE - 1;
#endif
}

#ifdef T_USE_SHARED_MEMORY

void wait_message(void)
{
  while (T_cache[T_busylist_head].busy == 0) usleep(1000);
}

void init_shm(void)
{
  int i;
  int s = shm_open(T_SHM_FILENAME, O_RDWR | O_CREAT /*| O_SYNC*/, 0666);
  if (s == -1) { perror(T_SHM_FILENAME); abort(); }
  if (ftruncate(s, T_CACHE_SIZE * sizeof(T_cache_t)))
    { perror(T_SHM_FILENAME); abort(); }
  T_cache = mmap(NULL, T_CACHE_SIZE * sizeof(T_cache_t),
                 PROT_READ | PROT_WRITE, MAP_SHARED, s, 0);
  if (T_cache == NULL)
    { perror(T_SHM_FILENAME); abort(); }
  close(s);

  /* let's garbage the memory to catch some potential problems
   * (think multiprocessor sync issues, barriers, etc.)
   */
  memset(T_cache, 0x55, T_CACHE_SIZE * sizeof(T_cache_t));
  for (i = 0; i < T_CACHE_SIZE; i++) T_cache[i].busy = 0;
}

#endif /* T_USE_SHARED_MEMORY */

void new_thread(void *(*f)(void *), void *data)
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

void usage(void)
{
  printf(
"common options:\n"
"    -d <database file>        this option is mandatory\n"
"    -li                       print IDs in the database\n"
"    -lg                       print GROUPs in the database\n"
"    -dump                     dump the database\n"
"    -x                        run with XFORMS (revisited)\n"
"    -on <GROUP or ID>         turn log ON for given GROUP or ID\n"
"    -off <GROUP or ID>        turn log OFF for given GROUP or ID\n"
"    -ON                       turn all logs ON\n"
"    -OFF                      turn all logs OFF\n"
"note: you may pass several -on/-off/-ON/-OFF, they will be processed in order\n"
"      by default, all is off\n"
"\n"
"remote mode options: in this mode you run a local tracer and a remote one\n"
"    -r <port>                 remote side (use given port)\n"
"    -l <IP address> <port>    local side (forwards packets to remote IP:port)\n"
  );
  exit(1);
}

int main(int n, char **v)
{
  char *database_filename = NULL;
  void *database;
  int s;
  int l;
  char t;
  int i;
  int do_list_ids = 0;
  int do_list_groups = 0;
  int do_dump_database = 0;
  int do_xforms = 0;
  char **on_off_name;
  int *on_off_action;
  int on_off_n = 0;
  int is_on[T_NUMBER_OF_IDS];
  int remote_local = 0;
  int remote_remote = 0;
  char *remote_ip = NULL;
  int remote_port = -1;
  int port = 2020;
#ifdef T_USE_SHARED_MEMORY
  void *f;
#endif

  memset(is_on, 0, sizeof(is_on));

  on_off_name = malloc(n * sizeof(char *)); if (on_off_name == NULL) abort();
  on_off_action = malloc(n * sizeof(int)); if (on_off_action == NULL) abort();

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-li")) { do_list_ids = 1; continue; }
    if (!strcmp(v[i], "-lg")) { do_list_groups = 1; continue; }
    if (!strcmp(v[i], "-dump")) { do_dump_database = 1; continue; }
    if (!strcmp(v[i], "-x")) { do_xforms = 1; continue; }
    if (!strcmp(v[i], "-on")) { if (i > n-2) usage();
      on_off_name[on_off_n]=v[++i]; on_off_action[on_off_n++]=1; continue; }
    if (!strcmp(v[i], "-off")) { if (i > n-2) usage();
      on_off_name[on_off_n]=v[++i]; on_off_action[on_off_n++]=0; continue; }
    if (!strcmp(v[i], "-ON"))
      { on_off_name[on_off_n]=NULL; on_off_action[on_off_n++]=1; continue; }
    if (!strcmp(v[i], "-OFF"))
      { on_off_name[on_off_n]=NULL; on_off_action[on_off_n++]=0; continue; }
    if (!strcmp(v[i], "-r")) { if (i > n-2) usage(); remote_remote = 1;
      port = atoi(v[++i]); continue; }
    if (!strcmp(v[i], "-l")) { if (i > n-3) usage(); remote_local = 1;
      remote_ip = v[++i]; remote_port = atoi(v[++i]); continue; }
    printf("ERROR: unknown option %s\n", v[i]);
    usage();
  }

#ifndef T_USE_SHARED_MEMORY
  /* gcc shut up */
  (void)remote_port;
  (void)remote_ip;
#endif

#ifdef T_USE_SHARED_MEMORY
  if (remote_remote) {
    printf("ERROR: remote 'remote side' does not run with shared memory\n");
    printf("recompile without T_USE_SHARED_MEMORY (edit Makefile)\n");
    exit(1);
  }
#endif

  if (remote_remote) {
    /* TODO: setup 'secure' connection with remote part */
  }

#ifndef T_USE_SHARED_MEMORY
  if (remote_local) {
    printf("ERROR: remote 'local side' does not run without shared memory\n");
    printf("recompile with T_USE_SHARED_MEMORY (edit Makefile)\n");
    exit(1);
  }
#endif

#ifdef T_USE_SHARED_MEMORY
  if (remote_local) f = forwarder(remote_ip, remote_port);
#endif

  if (remote_local) goto no_database;

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  database = parse_database(database_filename);

  if (do_list_ids + do_list_groups + do_dump_database > 1) usage();
  if (do_list_ids) { list_ids(database); return 0; }
  if (do_list_groups) { list_groups(database); return 0; }
  if (do_dump_database) { dump_database(database); return 0; }

  for (i = 0; i < on_off_n; i++)
    on_off(database, on_off_name[i], is_on, on_off_action[i]);

no_database:
  if (do_xforms) {
    ul_plot = make_plot(512, 100, "UL Input Signal", 1,
                        7680*10, PLOT_VS_TIME, BLUE);
    chest_plot = make_plot(512, 100, "UL Channel Estimate UE 0", 1,
                           512, PLOT_VS_TIME, BLUE);
    pusch_iq_plot = make_plot(100, 100, "PUSCH IQ", 1,
                              12*25*14, PLOT_IQ_POINTS, BLUE);
    pucch_iq_plot = make_plot(100, 100, "PUCCH IQ", 1,
                              1000, PLOT_IQ_POINTS, BLUE);
    pucch_plot = make_plot(512, 100, "PUCCH 1 energy (SR)", 2,
                           /* threshold */
                           10240, PLOT_MINMAX, RED,
                           /* pucch 1 */
                           10240, PLOT_MINMAX, BLUE);
  }

#ifdef T_USE_SHARED_MEMORY
  init_shm();
#endif
  s = get_connection("127.0.0.1", port);

  if (remote_local) {
#ifdef T_USE_SHARED_MEMORY
    forward_start_client(f, s);
#endif
    goto no_init_message;
  }

  /* send the first message - activate all traces */
  t = 0;
  if (write(s, &t, 1) != 1) abort();
  l = 0;
  for (i = 0; i < T_NUMBER_OF_IDS; i++) if (is_on[i]) l++;
  if (write(s, &l, sizeof(int)) != sizeof(int)) abort();
  for (l = 0; l < T_NUMBER_OF_IDS; l++)
    if (is_on[l])
      if (write(s, &l, sizeof(int)) != sizeof(int)) abort();

no_init_message:

  /* read messages */
  while (1) {
#ifdef T_USE_SHARED_MEMORY
    wait_message();
    __sync_synchronize();
#endif

#ifdef T_USE_SHARED_MEMORY
    if (remote_local) {
      forward(f, T_cache[T_busylist_head].buffer,
              T_cache[T_busylist_head].length);
      T_cache[T_busylist_head].busy = 0;
      T_busylist_head++;
      T_busylist_head &= T_CACHE_SIZE - 1;
      continue;
    }
#endif

    get_message(s);
  }
  return 0;
}
