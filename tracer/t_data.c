#include "defs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

typedef struct {
  char *name;
  char *desc;
} id;

typedef struct {
  char *name;
  char **ids;
  int size;
} group;

typedef struct {
  id *i;
  int isize;
  group *g;
  int gsize;
} database;

typedef struct {
  char *data;
  int size;
  int maxsize;
} buffer;

typedef struct {
  buffer name;
  buffer value;
} parser;

void put(buffer *b, int c)
{
  if (b->size == b->maxsize) {
    b->maxsize += 256;
    b->data = realloc(b->data, b->maxsize);
    if (b->data == NULL) { printf("memory allocation error\n"); exit(1); }
  }
  b->data[b->size] = c;
  b->size++;
}

void smash_spaces(FILE *f)
{
  int c;
  while (1) {
    c = fgetc(f);
    if (isspace(c)) continue;
    if (c == ' ') continue;
    if (c == '\t') continue;
    if (c == '\n') continue;
    if (c == 10 || c == 13) continue;
    if (c == '#') {
      while (1) {
        c = fgetc(f);
        if (c == '\n' || c == EOF) break;
      }
      continue;
    }
    break;
  }
  if (c != EOF) ungetc(c, f);
}

void get_line(parser *p, FILE *f, char **name, char **value)
{
  int c;
  p->name.size = 0;
  p->value.size = 0;
  *name = NULL;
  *value = NULL;
  smash_spaces(f);
  c = fgetc(f);
  while (!(c == '=' || isspace(c) || c == EOF))
    { put(&p->name, c); c = fgetc(f); }
  if (c == EOF) return;
  put(&p->name, 0);
  while (!(c == EOF || c == '=')) c = fgetc(f);
  if (c == EOF) return;
  smash_spaces(f);
  c = fgetc(f);
  while (!(c == 10 || c == 13 || c == EOF))
    { put(&p->value, c); c = fgetc(f); }
  put(&p->value, 0);
  if (p->name.size <= 1) return;
  if (p->value.size <= 1) return;
  *name = p->name.data;
  *value = p->value.data;
}

void add_id(database *r, char *id)
{
  if ((r->isize & 1023) == 0) {
    r->i = realloc(r->i, (r->isize + 1024) * sizeof(id));
    if (r->i == NULL) { printf("out of memory\n"); exit(1); }
  }
  r->i[r->isize].name = strdup(id);
  if (r->i[r->isize].name == NULL) { printf("out of memory\n"); exit(1); }
  r->isize++;
}

int group_cmp(const void *_p1, const void *_p2)
{
  const group *p1 = _p1;
  const group *p2 = _p2;
  return strcmp(p1->name, p2->name);
}

group *get_group(database *r, char *group_name)
{
  group gsearch;
  group *ret;

  gsearch.name = group_name;
  ret = bsearch(&gsearch, r->g, r->gsize, sizeof(group), group_cmp);
  if (ret != NULL) return ret;

  if ((r->gsize & 1023) == 0) {
    r->g = realloc(r->g, (r->gsize + 1024) * sizeof(group));
    if (r->g == NULL) abort();
  }
  r->g[r->gsize].name = strdup(group_name);
  if (r->g[r->gsize].name == NULL) abort();
  r->g[r->gsize].ids = NULL;
  r->g[r->gsize].size = 0;
  r->gsize++;

  qsort(r->g, r->gsize, sizeof(group), group_cmp);

  return bsearch(&gsearch, r->g, r->gsize, sizeof(group), group_cmp);
}

void group_add_id(group *g, char *id)
{
  if ((g->size & 1023) == 0) {
    g->ids = realloc(g->ids, (g->size + 1024) * sizeof(char *));
    if (g->ids == NULL) abort();
  }
  g->ids[g->size] = id;
  g->size++;
}

void add_groups(database *r, char *groups)
{
  group *g;

  while (1) {
    char *start = groups;
    char *end = start;
    while (!isspace(*end) && *end != ':' && *end != 0) end++;
    if (end == start) {
      printf("bad group line: groups are seperated by ':'\n");
      abort();
    }
    if (*end == 0) end = NULL; else *end = 0;

    g = get_group(r, start);
    group_add_id(g, r->i[r->isize-1].name);

    if (end == NULL) break;
    end++;
    while ((isspace(*end) || *end == ':') && *end != 0) end++;
    if (*end == 0) break;
    groups = end;
  }
}

void *parse_database(char *filename)
{
  FILE *in;
  parser p;
  database *r;
  char *name, *value;

  r = calloc(1, sizeof(*r)); if (r == NULL) abort();
  memset(&p, 0, sizeof(p));

  in = fopen(filename, "r"); if (in == NULL) { perror(filename); abort(); }

  while (1) {
    get_line(&p, in, &name, &value);
    if (name == NULL) break;
    printf("%s %s\n", name, value);
    if (!strcmp(name, "ID")) add_id(r, value);
    if (!strcmp(name, "GROUP")) add_groups(r, value);
  }

  fclose(in);
  free(p.name.data);
  free(p.value.data);

  return r;
}

void dump_database(void *_d)
{
  database *d = _d;
  int i;

  printf("database: %d IDs, %d GROUPs\n", d->isize, d->gsize);
  for (i = 0; i < d->isize; i++)
    printf("ID %s [%s]\n", d->i[i].name, d->i[i].desc);
  for (i = 0; i < d->gsize; i++) {
    int j;
    printf("GROUP %s [size %d]\n", d->g[i].name, d->g[i].size);
    for (j = 0; j < d->g[i].size; j++)
      printf("  ID %s\n", d->g[i].ids[j]);
  }
}
