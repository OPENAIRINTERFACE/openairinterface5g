#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "event.h"
#include "database.h"
#include "config.h"
#include "../T_defs.h"

void usage(void)
{
  printf(
"options:\n"
"    -d <database file>        this option is mandatory\n"
"    -ip <host>                connect to given IP address (default %s)\n"
"    -p <port>                 connect to given port (default %d)\n",
  DEFAULT_REMOTE_IP,
  DEFAULT_REMOTE_PORT
  );
  exit(1);
}

int main(int n, char **v)
{
  char *database_filename = NULL;
  void *database;
  char *ip = DEFAULT_REMOTE_IP;
  int port = DEFAULT_REMOTE_PORT;
  int i;
  char t;
  int number_of_events;
  int socket;
  int *is_on;
  int ev_input, ev_nack;
  int ev_ack;

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-ip")) { if (i > n-2) usage(); ip = v[++i]; continue; }
    if (!strcmp(v[i], "-p"))
      { if (i > n-2) usage(); port = atoi(v[++i]); continue; }
    usage();
  }

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  database = parse_database(database_filename);

  load_config_file(database_filename);

  number_of_events = number_of_ids(database);
  is_on = calloc(number_of_events, sizeof(int));
  if (is_on == NULL) abort();

  on_off(database, "ENB_PHY_INPUT_SIGNAL", is_on, 1);
  on_off(database, "ENB_PHY_ULSCH_UE_NACK", is_on, 1);
  on_off(database, "ENB_PHY_ULSCH_UE_ACK", is_on, 1);

  ev_input = event_id_from_name(database, "ENB_PHY_INPUT_SIGNAL");
  ev_nack = event_id_from_name(database, "ENB_PHY_ULSCH_UE_NACK");
  ev_ack = event_id_from_name(database, "ENB_PHY_ULSCH_UE_ACK");

  socket = connect_to(ip, port);

  t = 1;
  if (socket_send(socket, &t, 1) == -1 ||
      socket_send(socket, &number_of_events, sizeof(int)) == -1 ||
      socket_send(socket, is_on, number_of_events * sizeof(int)) == -1)
    abort();

  char dump[10][T_BUFFER_MAX];
  event dump_ev[10];
FILE *z = fopen("/tmp/dd", "w"); if (z == NULL) abort();
  while (1) {
    char v[T_BUFFER_MAX];
    event e;
    e = get_event(socket, v, database);
    if (e.type == -1) break;
    if (e.type == ev_input) {
      int sf = e.e[2].i;
      memcpy(dump[sf], v, T_BUFFER_MAX);
      dump_ev[sf] = e;
      printf("input %d/%d\n", e.e[1].i, sf);
if (fwrite(dump_ev[sf].e[4].b, dump_ev[sf].e[4].bsize, 1, z) != 1) abort();
fflush(z);
    }
    if (e.type == ev_nack) {
      int sf = e.e[2].i;
      printf("nack %d/%d\n", e.e[1].i, sf);
      FILE *f = fopen("/tmp/dump.raw", "w"); if (f == NULL) abort();
      if (fwrite(dump[sf] + ((char *)dump_ev[sf].e[4].b - v),
          dump_ev[sf].e[4].bsize, 1, f) != 1) abort();
      if (fclose(f)) abort();
      printf("dumped... press enter (delta %d)\n", (int)((char *)dump_ev[sf].e[4].b - v));
//      getchar();
    }
    if (e.type == ev_ack) {
      int sf = e.e[2].i;
      printf("ack %d/%d\n", e.e[1].i, sf);
      FILE *f = fopen("/tmp/dump.raw", "w"); if (f == NULL) abort();
      if (fwrite(dump[sf] + ((char *)dump_ev[sf].e[4].b - v),
          dump_ev[sf].e[4].bsize, 1, f) != 1) abort();
      if (fclose(f)) abort();
      printf("dumped... press enter (delta %d)\n", (int)((char *)dump_ev[sf].e[4].b - v));
//      getchar();
    }
  }

  return 0;
}
