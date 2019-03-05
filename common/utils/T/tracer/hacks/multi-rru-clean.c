#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"
#include "utils.h"
#include "event.h"
#include "../T_defs.h"

void usage(void)
{
  printf(
    "usage: <number of tags> <input record file> <output curated record file>\n"
    "options:\n"
    "    -d <database file>        this option is mandatory\n"
  );
  exit(1);
}

#define ERR printf("ERROR: read file %s failed\n", input_filename)

typedef struct {
  OBUF b;
  struct timespec t;
  int filled;
  int error;
} cache_t;

void clear_cache(int n, cache_t *c)
{
  int i;
  for (i = 0; i < n; i++) {
    c[i].filled = 0;
    c[i].error = 0;
    c[i].b.osize = 0;
  }
}

void store_in_cache(cache_t *c, int pos, OBUF *b)
{
  int i;
  for (i = 0; i < b->osize; i++)
    PUTC(&c[pos].b, b->obuf[i]);
}

int get_field(database_event_format *f, char *field, char *type)
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

void process_cache(FILE *out, cache_t *c, int n, int frame, int subframe)
{
  int i;
  struct tm *t;

  for (i = 0; i < n; i++)
    if (c[i].filled == 0 || c[i].error == 1)
      goto error;

  for (i = 0; i < n; i++)
    fwrite(c[i].b.obuf, c[i].b.osize, 1, out);

  clear_cache(n, c);
  return;

error:
  printf("ERROR: incorrect data at frame %d subframe %d", frame, subframe);
  for (i = 0; i < n; i++) if (c[i].filled) {
    t = localtime(&c[i].t.tv_sec);
    printf(" [tag %d time %2.2d:%2.2d:%2.2d.%9.9ld]", i,
           t->tm_hour, t->tm_min, t->tm_sec, c[i].t.tv_nsec);
  }
  printf("\n");
  clear_cache(n, c);
}

int main(int n, char **v)
{
  char *database_filename = NULL;
  void *database;
  int  channel_estimate_id;
  int  number_of_tags = -1;
  char *input_filename = NULL;
  char *output_filename = NULL;
  int  i;
  FILE *in;
  FILE *out;
  database_event_format f;
  int  frame_arg;
  int  subframe_arg;
  int  tag_arg;

  cache_t *cache;
  int     cur_frame = -1;
  int     cur_subframe = -1;
  int     frame;
  int     subframe;
  int     tag;

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d")) { if (i > n-2) usage();
      database_filename = v[++i]; continue; }
    if (number_of_tags == -1) { number_of_tags = atoi(v[i]);
      if (number_of_tags <= 0) {usage();} continue; }
    if (input_filename == NULL) { input_filename = v[i]; continue; }
    if (output_filename == NULL) { output_filename = v[i]; continue; }
    usage();
  }

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  if (output_filename == NULL || input_filename == NULL || number_of_tags == -1)
    usage();

  database = parse_database(database_filename);

  channel_estimate_id = event_id_from_name(database, "CALIBRATION_CHANNEL_ESTIMATES");
  f = get_format(database, channel_estimate_id);

  frame_arg    = get_field(&f, "frame",    "int");
  subframe_arg = get_field(&f, "subframe", "int");
  tag_arg      = get_field(&f, "tag",      "int");

  in = fopen(input_filename, "r");
  if (in == NULL) { perror(input_filename); abort(); }
  out = fopen(output_filename, "w");
  if (out == NULL) { perror(output_filename); abort(); }

  cache = calloc(number_of_tags, sizeof(cache_t));
  if (cache == NULL) { perror("malloc"); exit(1); }

  clear_cache(number_of_tags, cache);

  OBUF ebuf = { osize: 0, omaxsize: 0, obuf: NULL };

  while (1) {
    int type;
    int32_t length;
    char *v;
    int vpos = 0;
    struct timespec t;
    char *buf;

    /* read event from file */
    if (fread(&length, 4, 1, in) != 1) break;
    if (ebuf.omaxsize < length) {
      ebuf.omaxsize = (length + 65535) & ~65535;
      ebuf.obuf = realloc(ebuf.obuf, ebuf.omaxsize);
      if (ebuf.obuf == NULL) { printf("out of memory\n"); exit(1); }
    }
    v = ebuf.obuf;
    memcpy(v+vpos, &length, 4);
    vpos += 4;
#ifdef T_SEND_TIME
    if (length < sizeof(struct timespec)) { ERR; break; }
    if (fread(&t, sizeof(struct timespec), 1, in) != 1) { ERR; break; }
    memcpy(v+vpos, &t, sizeof(struct timespec));
    vpos += sizeof(struct timespec);
    length -= sizeof(struct timespec);
#endif
    if (length < sizeof(int)) { ERR; break; }
    if (fread(&type, sizeof(int), 1, in) != 1) { ERR; break; }
    memcpy(v+vpos, &type, sizeof(int));
    vpos += sizeof(int);
    length -= sizeof(int);
    if (length) if (fread(v+vpos, length, 1, in) != 1) { ERR; break; }
    buf = v + vpos;
    vpos += length;
    ebuf.osize = vpos;

    if (type != channel_estimate_id) continue;

    event e;
#ifdef T_SEND_TIME
    e = new_event(t, type, length, buf, database);
#else
    e = new_event(type, length, buf, database);
#endif

    frame    = e.e[frame_arg].i;
    subframe = e.e[subframe_arg].i;
    tag      = e.e[tag_arg].i;

    if (tag < 0 || tag >= number_of_tags) {
      struct tm *tt;
      tt = localtime(&t.tv_sec);
      printf("ERROR: invalid tag (%d), skipping event for frame %d subframe %d (time %2.2d:%2.2d:%2.2d.%9.9ld)\n",
             tag, frame, subframe,
             tt->tm_hour, tt->tm_min, tt->tm_sec, t.tv_nsec);
      continue;
    }

    if (cur_frame != frame || cur_subframe != subframe)
      if (cur_frame != -1)
        process_cache(out, cache, number_of_tags, cur_frame, cur_subframe);

    cur_frame = frame;
    cur_subframe = subframe;

    if (cache[tag].filled) {
      struct tm *tt;
      tt = localtime(&t.tv_sec);
      printf("ERROR: tag %d present twice at frame %d subframe %d (time %2.2d:%2.2d:%2.2d.%9.9ld)\n",
             tag, frame, subframe,
             tt->tm_hour, tt->tm_min, tt->tm_sec, t.tv_nsec);
      cache[tag].error = 1;
      continue;
    }

    store_in_cache(cache, tag, &ebuf);
    cache[tag].filled = 1;
    cache[tag].t = t;
  }

  if (fclose(in)) perror(input_filename);
  if (fclose(out)) perror(output_filename);
  free(cache);

  return 0;
}
