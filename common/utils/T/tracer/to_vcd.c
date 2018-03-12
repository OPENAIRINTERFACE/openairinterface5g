#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include "database.h"
#include "utils.h"
#include "handler.h"
#include "config.h"
#include "logger/logger.h"
#include "view/view.h"

typedef struct {
  char *event;
  char *arg;
  char *vcd_name;
  int boolean;
} vcd_vars;

/****************************************************************************/
/* VCD file handling begin                                                  */
/****************************************************************************/

FILE *out;

uint64_t start_time;
int start_time_inited;

void vcd_init(char *file)
{
  out = fopen(file, "w"); if (out == NULL) { perror(file); exit(1); }
}

void vcd_write_header(vcd_vars *v, int n)
{
  int i;

  if (fprintf(out,
"$date\n"
"  January, 1, 1970.\n"
"$end\n"
"$version\n"
"  to_vcd\n"
"$end\n"
"$timescale 1ns $end\n"
"$scope module logic $end\n") <= 0) abort();

  for (i = 0; i < n; i++)
    if (fprintf(out, "$var wire %d %s %s $end\n",
           v[i].boolean ? 1 : 64,
           v[i].vcd_name, v[i].vcd_name) <= 0) abort();

  if (fprintf(out,
"$upscope $end\n"
"$enddefinitions $end\n"
"$dumpvars\n") <= 0) abort();

  for (i = 0; i < n; i++)
    if (v[i].boolean) {
      if (fprintf(out, "0%s\n", v[i].vcd_name) <= 0) abort();
    } else {
      if (fprintf(out, "b0 %s\n", v[i].vcd_name) <= 0) abort();
    }

  if (fprintf(out,
"$end\n") <= 0) abort();

}

void vcd_end(void)
{
  if (fclose(out)) { perror("error closing VCD file"); exit(1); }
}

char *b64(uint64_t val)
{
  static char v[65];
  char *s = &v[64];
  *s = 0;
  if (val == 0) { s--; *s = '0'; return s; }
  while (val) {
    s--;
    *s = val&1 ? '1' : '0';
    val >>= 1;
  }
  return s;
}

void vcd_dump(char *v)
{
  uint64_t h, m, s, ns;
  char t;
  uint64_t val;
  uint64_t time;
  char var[256];
  if (sscanf(v, "%"SCNu64":%"SCNu64":%"SCNu64".%"SCNu64": %c %"SCNu64" %s",
      &h, &m, &s, &ns, &t, &val, var) != 7)
    goto err;
  time = h*60*60*1000000000 +
            m*60*1000000000 +
               s*1000000000 +
                         ns;
  if (!start_time_inited) {
    start_time = time;
    start_time_inited = 1;
  }
  if (fprintf(out, "#%"PRIu64"\n", time - start_time) <= 0) abort();
  switch (t) {
  case 'b': if (fprintf(out, "%d%s\n", val!=0, var) <= 0) abort(); break;
  case 'l': if (fprintf(out, "b%s %s\n", b64(val), var) <= 0) abort(); break;
  default: goto err;
  }

  return;

err:
  printf("bad vcd_dump line '%s'\n", v);
  exit(1);
}

/****************************************************************************/
/* VCD file handling end                                                    */
/****************************************************************************/


/****************************************************************************/
/* vcd view start                                                           */
/****************************************************************************/

struct vcd_view {
  view common;
};

static void vcd_view_clear(view *this)
{
  /* nothing */
}

static void vcd_view_append(view *_this, char *s)
{
  vcd_dump(s);
}

static view *new_view_vcd(void)
{
  struct vcd_view *ret = calloc(1, sizeof(struct vcd_view));
  if (ret == NULL) abort();
  ret->common.clear = vcd_view_clear;
  ret->common.append = (void (*)(view *, ...))vcd_view_append;
  return (view *)ret;
}

/****************************************************************************/
/* vcd view end                                                             */
/****************************************************************************/

void activate_traces(int socket, int number_of_events, int *is_on)
{
  char t = 1;
  if (socket_send(socket, &t, 1) == -1 ||
      socket_send(socket, &number_of_events, sizeof(int)) == -1 ||
      socket_send(socket, is_on, number_of_events * sizeof(int)) == -1)
    abort();
}

void usage(void)
{
  printf(
"options:\n"
"    -d <database file>           this option is mandatory\n"
"    -o <output file>             this option is mandatory\n"
"    -ip <host>                   connect to given IP address (default %s)\n"
"    -p <port>                    connect to given port (default %d)\n"
"    -b <event> <arg> <vcd name>  trace as binary (0 off, anything else on)\n"
"    -l <event> <arg> <vcd name>  trace as uint64_t\n",
  DEFAULT_REMOTE_IP,
  DEFAULT_REMOTE_PORT
  );
  exit(1);
}

int run = 1;
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
  char *output_filename = NULL;
  char *database_filename = NULL;
  void *database;
  char *ip = DEFAULT_REMOTE_IP;
  int port = DEFAULT_REMOTE_PORT;
  int *is_on;
  int number_of_events;
  int i;
  vcd_vars vars[n];
  int nvars = 0;
  view *vcd_view;
  event_handler *h;
  logger *textlog;

  /* write on a socket fails if the other end is closed and we get SIGPIPE */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) abort();

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-o"))
      { if (i > n-2) usage(); output_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-ip")) { if (i > n-2) usage(); ip = v[++i]; continue; }
    if (!strcmp(v[i], "-p"))
      { if (i > n-2) usage(); port = atoi(v[++i]); continue; }
    if (!strcmp(v[i], "-b")) { if(i>n-4)usage();
      vars[nvars].event     = v[++i];
      vars[nvars].arg       = v[++i];
      vars[nvars].vcd_name  = v[++i];
      vars[nvars++].boolean = 1;
      continue;
    }
    if (!strcmp(v[i], "-l")) { if(i>n-4)usage();
      vars[nvars].event     = v[++i];
      vars[nvars].arg       = v[++i];
      vars[nvars].vcd_name  = v[++i];
      vars[nvars++].boolean = 0;
      continue;
    }
    usage();
  }

  if (output_filename == NULL) {
    printf("ERROR; provide an output file (-o)\n");
    exit(1);
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

  /* create the view */
  vcd_view = new_view_vcd();

  /* setup traces */
  for (i = 0; i < nvars; i++) {
    char format[256];
    if (strlen(vars[i].arg) > 256-3) abort();
    if (strlen(vars[i].vcd_name) > 256-1) abort();
    sprintf(format, "%c [%s] %s",
        vars[i].boolean ? 'b' : 'l',
        vars[i].arg,
        vars[i].vcd_name);
    textlog = new_textlog(h, database, vars[i].event, format);
    logger_add_view(textlog, vcd_view);
    on_off(database, vars[i].event, is_on, 1);
  }

  socket = connect_to(ip, port);

  /* activate selected traces */
  activate_traces(socket, number_of_events, is_on);

  vcd_init(output_filename);
  vcd_write_header(vars, nvars);

  /* exit on ctrl+c and ctrl+z */
  if (signal(SIGQUIT, force_stop) == SIG_ERR) abort();
  if (signal(SIGINT, force_stop) == SIG_ERR) abort();
  if (signal(SIGTSTP, force_stop) == SIG_ERR) abort();

  /* read messages */
  while (run) {
    char v[T_BUFFER_MAX];
    event e;
    e = get_event(socket, v, database);
    if (e.type == -1) { printf("disconnected? let's quit gently\n"); break; }
    handle_event(h, e);
  }

  vcd_end();

  return 0;
}
