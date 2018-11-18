#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "database.h"
#include "event.h"
#include "config.h"

void usage(void)
{
  printf(
"usage: [options] <file> <event> <buffer name>\n"
"options:\n"
"    -d <database file>        this option is mandatory\n"
"    -o <output file>          this option is mandatory\n"
"    -f <name> <value>         field 'name' of 'event' has to match 'value'\n"
"                              type of 'name' must be int\n"
"                              (you can use several -f options)\n"
"    -after <raw time> <nsec>  'event' time has to be greater than this\n"
"    -count <n>                dump 'n' matching events (less if EOF reached)\n"
  );
  exit(1);
}

int get_filter_arg(database_event_format *f, char *field, char *type)
{
  int i;
  for (i = 0; i < f->count; i++)
    if (!strcmp(f->name[i], field)) {
      if (strcmp(f->type[i], type)) break;
      return i;
    }
  printf("bad field %s, check that it exists and has type '%s'\n",field,type);
  exit(1);
}

int main(int n, char **v)
{
  char *database_filename = NULL;
  void *database;
  int i;
  int input_event_id;
  database_event_format f;
  char *file = NULL;
  char *output_file = NULL;
  FILE *out;
  int fd;
  char *event_name = NULL;
  char *buffer_name = NULL;
  char *filter[n];
  int filter_arg[n];
  int filter_value[n];
  int filter_count = 0;
  int buffer_arg;
  int found;
  int count = 1;
  int check_time = 0;
  time_t sec;
  long nsec;

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-o"))
      { if (i > n-2) usage(); output_file = v[++i]; continue; }
    if (!strcmp(v[i], "-f")) { if (i>n-3) usage();
      filter[filter_count]         = v[++i];
      filter_value[filter_count++] = atoi(v[++i]);
      continue;
    }
    if (!strcmp(v[i], "-after")) { if (i>n-3) usage();
      check_time = 1;
      sec        = atoll(v[++i]);
      nsec       = atol(v[++i]);
      continue;
    }
    if (!strcmp(v[i], "-count"))
      { if (i > n-2) usage(); count = atoi(v[++i]); continue; }
    if (file == NULL) { file = v[i]; continue; }
    if (event_name == NULL) { event_name = v[i]; continue; }
    if (buffer_name == NULL) { buffer_name = v[i]; continue; }
    usage();
  }
  if (file == NULL || event_name == NULL || buffer_name == NULL) usage();

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  if (output_file == NULL) {
    printf("gimme -o <output file>, thanks\n");
    exit(1);
  }

  out = fopen(output_file, "w"); if(out==NULL){perror(output_file);exit(1);}

  database = parse_database(database_filename);

  load_config_file(database_filename);

  input_event_id = event_id_from_name(database, event_name);
  f = get_format(database, input_event_id);

  buffer_arg = get_filter_arg(&f, buffer_name, "buffer");

  for (i = 0; i < filter_count; i++)
    filter_arg[i] = get_filter_arg(&f, filter[i], "int");

  fd = open(file, O_RDONLY);
  if (fd == -1) { perror(file); exit(1); }

  found = 0;

  OBUF ebuf = { osize: 0, omaxsize: 0, obuf: NULL };

  while (1) {
    event e;
    e = get_event(fd, &ebuf, database);
    if (e.type == -1) break;
    if (e.type != input_event_id) continue;
    for (i = 0; i < filter_count; i++)
      if (filter_value[i] != e.e[filter_arg[i]].i)
        break;
    if (i != filter_count)
      continue;
    if (check_time &&
        !(e.sending_time.tv_sec > sec ||
         (e.sending_time.tv_sec == sec && e.sending_time.tv_nsec >= nsec)))
      continue;
    if (fwrite(e.e[buffer_arg].b, e.e[buffer_arg].bsize, 1, out) != 1)
      { perror(output_file); exit(1); }
    found++;
    if (found == count)
      break;
  }

  if (found == 0) printf("ERROR: event not found\n");
  if (found != count)
    printf("WARNING: dumped %d events (wanted %d)\n", found, count);

  fclose(out);

  return 0;
}
