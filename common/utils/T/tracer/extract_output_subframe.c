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
"usage: [options] <file> <frame> <subframe> <number of subframes>\n"
"    the program dumps 'number of subframes' subframes starting\n"
"    at 'frame:suubframe'\n"
"options:\n"
"    -d <database file>        this option is mandatory\n"
"    -o <output file>          output to file (default: stdout)\n"
"    -v                        verbose\n"
  );
  exit(1);
}

int main(int n, char **v)
{
  char *database_filename = NULL;
  void *database;
  int i;
  int output_event_id;
  database_event_format f;
  char *file = NULL;
  int fd;
  int frame = -1, subframe = -1;
  int number_of_subframes = -1;
  int frame_arg, subframe_arg, buffer_arg;
  int verbose = 0;
  char *out_filename = NULL;
  FILE *out;

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-o"))
      { if (i > n-2) usage(); out_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-v")) { verbose = 1; continue; }
    if (file == NULL)              { file = v[i]; continue; }
    if (frame == -1)               { frame = atoi(v[i]); continue; }
    if (subframe == -1)            { subframe = atoi(v[i]); continue; }
    if (number_of_subframes == -1) {number_of_subframes=atoi(v[i]);continue;}
    usage();
  }
  if (file == NULL || frame == -1 || subframe == -1 ||
      number_of_subframes == -1) usage();

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  if (out_filename == NULL) out = stdout;
  else {
    out = fopen(out_filename, "w");
    if (out == NULL) { perror(out_filename); exit(1); }
  }

  database = parse_database(database_filename);

  load_config_file(database_filename);

  output_event_id = event_id_from_name(database, "ENB_PHY_OUTPUT_SIGNAL");
  f = get_format(database, output_event_id);

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
    if (!strcmp(f.name[i], "txdata")) {
      if (buffer_arg != -1) goto err;
      if (strcmp(f.type[i], "buffer")) goto err;
      buffer_arg = i;
    }
    continue;
err:
    printf("cannot deal with ENB_PHY_OUTPUT_SIGNAL from database file\n");
    exit(1);
  }
  if (frame_arg == -1 || subframe_arg == -1 || buffer_arg == -1) goto err;

  fd = open(file, O_RDONLY);
  if (fd == -1) { perror(file); exit(1); }

  int last_frame = -1;
  int last_subframe = -1;
  int subframe_written = 0;

  OBUF ebuf = { osize: 0, omaxsize: 0, obuf: NULL };

  while (1) {
    event e;
    e = get_event(fd, &ebuf, database);
    if (e.type == -1) break;
    if (e.type != output_event_id) continue;
    if (verbose)
      printf("found output frame %d subframe %d size %d\n",
             e.e[frame_arg].i, e.e[subframe_arg].i, e.e[buffer_arg].bsize);
    if (!(last_frame != -1 ||
          (frame == e.e[frame_arg].i && subframe == e.e[subframe_arg].i)))
      continue;
    frame = e.e[frame_arg].i;
    subframe = e.e[subframe_arg].i;
    if (last_frame != -1) {
      if (frame*10+subframe != (last_frame*10+last_subframe + 1) % 10240) {
        printf("error: discontinuity, not what you want...\n");
        exit(1);
      }
    }
    last_frame = frame;
    last_subframe = subframe;
    if (verbose)
      printf("save output frame %d subframe %d size %d\n",
             e.e[frame_arg].i, e.e[subframe_arg].i, e.e[buffer_arg].bsize);
    if (fwrite(e.e[buffer_arg].b, e.e[buffer_arg].bsize, 1, out) != 1) {
      perror(out_filename);
      exit(1);
    }
    subframe_written++;
    if (subframe_written != number_of_subframes)
      continue;

    /* done */
    fflush(out);
    if (out_filename != NULL) {
      if (fclose(out)) perror(out_filename);
    }
    if (verbose)
      printf("%d subframes written\n", subframe_written);
    return 0;
  }

  printf("error with input file, %d subframe written\n", subframe_written);
  fflush(out);
  if (out_filename != NULL) {
    if (fclose(out)) perror(out_filename);
  }
  return 1;
}
