#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "database.h"
#include "event.h"
#include "configuration.h"

void usage(void)
{
  printf(
"usage: [options] <file> <frame> <subframe>\n"
"options:\n"
"    -d <database file>        this option is mandatory\n"
"    -o <output file>          this option is mandatory\n"
"    -v                        verbose\n"
"    -c <number of subframes>  default to 1\n"
  );
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
  int frame = -1, subframe = -1;
  int frame_arg, subframe_arg, buffer_arg;
  int verbose = 0;
  int number_of_subframes = 1;
  int processed_subframes = 0;

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-o"))
      { if (i > n-2) usage(); output_file = v[++i]; continue; }
    if (!strcmp(v[i], "-c"))
      { if (i > n-2) usage(); number_of_subframes = atoi(v[++i]); continue; }
    if (!strcmp(v[i], "-v")) { verbose = 1; continue; }
    if (file == NULL) { file = v[i]; continue; }
    if (frame == -1) { frame = atoi(v[i]); continue; }
    if (subframe == -1) { subframe = atoi(v[i]); continue; }
    usage();
  }
  if (file == NULL || frame == -1 || subframe == -1) usage();

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  if (number_of_subframes < 1) {
    printf("bad value for option -c, must be at least 1 and is %d\n",
           number_of_subframes);
    exit(1);
  }

  if (output_file == NULL) {
    printf("gimme -o <output file>, thanks\n");
    exit(1);
  }

  out = fopen(output_file, "w"); if(out==NULL){perror(output_file);exit(1);}

  database = parse_database(database_filename);

  load_config_file(database_filename);

  input_event_id = event_id_from_name(database, "ENB_PHY_INPUT_SIGNAL");
  f = get_format(database, input_event_id);

  frame_arg = subframe_arg = buffer_arg = -1;
  for (i = 0; i < f.count; i++) {
    if (!strcmp(f.name[i], "frame")) {
      if (frame_arg != -1) goto err;
      if (strcmp(f.type[i], "int")) goto err;
      frame_arg = i;
    }
    if (!strcmp(f.name[i], "subframe")) {
      if (subframe_arg != -1) goto err;
      if (strcmp(f.type[i], "int")) goto err;
      subframe_arg = i;
    }
    if (!strcmp(f.name[i], "rxdata")) {
      if (buffer_arg != -1) goto err;
      if (strcmp(f.type[i], "buffer")) goto err;
      buffer_arg = i;
    }
    continue;
err:
    printf("cannot deal with ENB_PHY_INPUT_SIGNAL from database file\n");
    exit(1);
  }
  if (frame_arg == -1 || subframe_arg == -1 || buffer_arg == -1) goto err;

  fd = open(file, O_RDONLY);
  if (fd == -1) { perror(file); exit(1); }

  OBUF ebuf = { osize: 0, omaxsize: 0, obuf: NULL };

  /* get wanted frame/subframe */
  while (1) {
    event e;
    e = get_event(fd, &ebuf, database);
    if (e.type == -1) break;
    if (e.type != input_event_id) continue;
    if (verbose)
      printf("input frame %d subframe %d size %d\n",
             e.e[frame_arg].i, e.e[subframe_arg].i, e.e[buffer_arg].bsize);
    if (!(frame == e.e[frame_arg].i && subframe == e.e[subframe_arg].i))
      continue;

    if (fwrite(e.e[buffer_arg].b, e.e[buffer_arg].bsize, 1, out) != 1)
      { perror(output_file); exit(1); }
    processed_subframes++;
    number_of_subframes--;
    if (!number_of_subframes) {
      if (fclose(out)) perror(output_file);
      printf("%d subframes dumped\n", processed_subframes);
      return 0;
    }
    subframe++;
    if (subframe == 10) { subframe = 0; frame=(frame+1)%1024; }
  }

  printf("frame %d subframe %d not found\n", frame, subframe);
  printf("%d subframes dumped\n", processed_subframes);
  fclose(out);

  return 0;
}
