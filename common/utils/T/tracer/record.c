#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "database.h"
#include "utils.h"
#include "../T_defs.h"
#include "configuration.h"

void usage(void)
{
  printf(
"options:\n"
"    -d <database file>        this option is mandatory\n"
"    -o <output file>          this option is mandatory\n"
"    -on <GROUP or ID>         turn log ON for given GROUP or ID\n"
"    -off <GROUP or ID>        turn log OFF for given GROUP or ID\n"
"    -ON                       turn all logs ON\n"
"    -OFF                      turn all logs OFF\n"
"                              note: you may pass several -on/-off/-ON/-OFF,\n"
"                                    they will be processed in order\n"
"                                    by default, all is off\n"
"    -ip <host>                connect to given IP address (default %s)\n"
"    -p <port>                 connect to given port (default %d)\n",
  DEFAULT_REMOTE_IP,
  DEFAULT_REMOTE_PORT
  );
  exit(1);
}

volatile int run = 1;

static int socket = -1;

void force_stop(int x)
{
  printf("\ngently quit...\n");
  close(socket);
  socket = -1;
  run = 0;
}

int main(int n, char **v)
{
  char *database_filename = NULL;
  char *output_filename = NULL;
  FILE *out;
  void *database;
  char *ip = DEFAULT_REMOTE_IP;
  int port = DEFAULT_REMOTE_PORT;
  char **on_off_name;
  int *on_off_action;
  int on_off_n = 0;
  int *is_on;
  int number_of_events;
  int i;
  char mt;

  /* write on a socket fails if the other end is closed and we get SIGPIPE */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) abort();

  on_off_name = malloc(n * sizeof(char *)); if (on_off_name == NULL) abort();
  on_off_action = malloc(n * sizeof(int)); if (on_off_action == NULL) abort();

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-o"))
      { if (i > n-2) usage(); output_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-ip")) { if (i > n-2) usage(); ip = v[++i]; continue; }
    if (!strcmp(v[i], "-p"))
      { if (i > n-2) usage(); port = atoi(v[++i]); continue; }
    if (!strcmp(v[i], "-on")) { if (i > n-2) usage();
      on_off_name[on_off_n]=v[++i]; on_off_action[on_off_n++]=1; continue; }
    if (!strcmp(v[i], "-off")) { if (i > n-2) usage();
      on_off_name[on_off_n]=v[++i]; on_off_action[on_off_n++]=0; continue; }
    if (!strcmp(v[i], "-ON"))
      { on_off_name[on_off_n]=NULL; on_off_action[on_off_n++]=1; continue; }
    if (!strcmp(v[i], "-OFF"))
      { on_off_name[on_off_n]=NULL; on_off_action[on_off_n++]=0; continue; }
    usage();
  }

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }
  if (output_filename == NULL) {
    printf("ERROR: provide an output file (-o)\n");
    exit(1);
  }

  database = parse_database(database_filename);

  load_config_file(database_filename);

  number_of_events = number_of_ids(database);
  is_on = calloc(number_of_events, sizeof(int));
  if (is_on == NULL) abort();

  for (i = 0; i < on_off_n; i++)
    on_off(database, on_off_name[i], is_on, on_off_action[i]);

  socket = connect_to(ip, port);

  /* activate selected traces */
  mt = 1;
  if (socket_send(socket, &mt, 1) == -1 ||
      socket_send(socket, &number_of_events, sizeof(int)) == -1 ||
      socket_send(socket, is_on, number_of_events * sizeof(int)) == -1)
    abort();

  out = fopen(output_filename, "w");
  if (out == NULL) {
    perror(output_filename);
    exit(1);
  }

  /* exit on ctrl+c and ctrl+z */
  if (signal(SIGQUIT, force_stop) == SIG_ERR) abort();
  if (signal(SIGINT, force_stop) == SIG_ERR) abort();
  if (signal(SIGTSTP, force_stop) == SIG_ERR) abort();

  OBUF ebuf = { osize: 0, omaxsize: 0, obuf: NULL };

  /* read messages */
  while (run) {
    int type;
    int32_t length;
    char *v;
    int vpos = 0;

    if (fullread(socket, &length, 4) == -1) goto read_error;
    if (ebuf.omaxsize < length) {
      ebuf.omaxsize = (length + 65535) & ~65535;
      ebuf.obuf = realloc(ebuf.obuf, ebuf.omaxsize);
      if (ebuf.obuf == NULL) { printf("out of memory\n"); exit(1); }
    }
    v = ebuf.obuf;
    memcpy(v+vpos, &length, 4);
    vpos += 4;
#ifdef T_SEND_TIME
    if (fullread(socket,v+vpos,sizeof(struct timespec))==-1) goto read_error;
    vpos += sizeof(struct timespec);
    length -= sizeof(struct timespec);
#endif
    if (fullread(socket, &type, sizeof(int)) == -1) goto read_error;
    memcpy(v+vpos, &type, sizeof(int));
    vpos += sizeof(int);
    length -= sizeof(int);
    if (fullread(socket, v+vpos, length) == -1) goto read_error;
    vpos += length;

    if (type == -1) append_received_config_chunk(v+vpos-length, length);
    if (type == -2) verify_config();

    if (fwrite(v, vpos, 1, out) != 1) {
      printf("error saving data to file %s\n", output_filename);
      fclose(out);
      exit(1);
    }
  }

read_error:
  fclose(out);

  return 0;
}
