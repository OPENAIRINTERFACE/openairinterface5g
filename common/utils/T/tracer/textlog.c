#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "database.h"
#include "event.h"
#include "handler.h"
#include "logger/logger.h"
#include "view/view.h"
#include "utils.h"
#include "configuration.h"

void activate_traces(int socket, int *is_on, int nevents)
{
  char t;

  t = 1;
  if (socket_send(socket, &t, 1) == -1 ||
      socket_send(socket, &nevents, sizeof(int)) == -1 ||
      socket_send(socket, is_on, nevents * sizeof(int)) == -1)
    abort();
}

void usage(void)
{
  printf(
"options:\n"
"    -d <database file>        this option is mandatory\n"
"    -on <GROUP or ID>         turn log ON for given GROUP or ID\n"
"    -off <GROUP or ID>        turn log OFF for given GROUP or ID\n"
"    -ON                       turn all logs ON\n"
"    -OFF                      turn all logs OFF\n"
"                              note: you may pass several -on/-off/-ON/-OFF,\n"
"                                    they will be processed in order\n"
"                                    by default, all is off\n"
"    -full                     also dump buffers' content\n"
"    -raw-time                 also prints 'raw time'\n"
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
  int socket;
  char *ip = DEFAULT_REMOTE_IP;
  int port = DEFAULT_REMOTE_PORT;
  char **on_off_name;
  int *on_off_action;
  int on_off_n = 0;
  int *is_on;
  int number_of_events;
  int i;
  event_handler *h;
  logger *textlog;
  view *out;
  int full = 0;
  int raw_time = 0;

  /* write on a socket fails if the other end is closed and we get SIGPIPE */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) abort();

  on_off_name = malloc(n * sizeof(char *)); if (on_off_name == NULL) abort();
  on_off_action = malloc(n * sizeof(int)); if (on_off_action == NULL) abort();

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
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
    if (!strcmp(v[i], "-full")) { full = 1; continue; }
    if (!strcmp(v[i], "-raw-time")) { raw_time = 1; continue; }
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

  h = new_handler(database);

  out = new_view_stdout();

  for (i = 0; i < number_of_events; i++) {
    char *name, *desc;
    database_get_generic_description(database, i, &name, &desc);
    textlog = new_textlog(h, database, name, desc);
    logger_add_view(textlog, out);
    if (full) textlog_dump_buffer(textlog, 1);
    if (raw_time) textlog_raw_time(textlog, 1);
    free(name);
    free(desc);
  }

  for (i = 0; i < on_off_n; i++)
    on_off(database, on_off_name[i], is_on, on_off_action[i]);

  socket = connect_to(ip, port);
  if (socket == -1) {
    printf("fatal: connection failed\n");
    return 1;
  }

  /* send the first message - activate selected traces */
  activate_traces(socket, is_on, number_of_events);

  OBUF ebuf = { osize: 0, omaxsize: 0, obuf: NULL };

  /* read messages */
  while (1) {
    event e;
    e = get_event(socket, &ebuf, database);
    if (e.type == -1) break;
    handle_event(h, e);
  }

  free(on_off_name);
  free(on_off_action);

  return 0;
}
