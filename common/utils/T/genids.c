#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char **unique_ids;
int unique_ids_size;
int unique_ids_maxsize;

int cmp(const void *p1, const void *p2)
{
  return strcmp(*(char * const *)p1, *(char * const *)p2);
}

/* return 1 if s was not already known, 0 if it was */
int new_unique_id(char *s)
{
  if (unique_ids_size)
  if (bsearch(&s, unique_ids, unique_ids_size, sizeof(char *), cmp) != NULL) {
    printf("FATAL: ID %s is not unique\n", s);
    return 0;
  }
  if (unique_ids_size == unique_ids_maxsize) {
    unique_ids_maxsize += 256;
    unique_ids = realloc(unique_ids, unique_ids_maxsize * sizeof(char *));
    if (unique_ids == NULL) { printf("out of memory\n"); abort(); }
  }
  unique_ids[unique_ids_size] = strdup(s);
  if (unique_ids[unique_ids_size] == NULL)
    { printf("out of memory\n"); abort(); }
  unique_ids_size++;
  qsort(unique_ids, unique_ids_size, sizeof(char *), cmp);
  return 1;
}

char *bufname;
int bufname_size;
int bufname_maxsize;

void putname(int c)
{
  if (bufname_size == bufname_maxsize) {
    bufname_maxsize += 256;
    bufname = realloc(bufname, bufname_maxsize);
    if (bufname == NULL) { printf("memory allocation error\n"); exit(1); }
  }
  bufname[bufname_size] = c;
  bufname_size++;
}

char *bufvalue;
int bufvalue_size;
int bufvalue_maxsize;

void putvalue(int c)
{
  if (bufvalue_size == bufvalue_maxsize) {
    bufvalue_maxsize += 256;
    bufvalue = realloc(bufvalue, bufvalue_maxsize);
    if (bufvalue == NULL) { printf("memory allocation error\n"); exit(1); }
  }
  bufvalue[bufvalue_size] = c;
  bufvalue_size++;
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

void get_line(FILE *f, char **name, char **value)
{
  int c;
  bufname_size = 0;
  bufvalue_size = 0;
  *name = NULL;
  *value = NULL;
  smash_spaces(f);
  c = fgetc(f);
  while (!(c == '=' || isspace(c) || c == EOF)) { putname(c); c = fgetc(f); }
  if (c == EOF) return;
  putname(0);
  while (!(c == EOF || c == '=')) c = fgetc(f);
  if (c == EOF) return;
  smash_spaces(f);
  c = fgetc(f);
  while (!(c == 10 || c == 13 || c == EOF)) { putvalue(c); c = fgetc(f); }
  putvalue(0);
  if (bufname_size <= 1) return;
  if (bufvalue_size <= 1) return;
  *name = bufname;
  *value = bufvalue;
}

int main(int n, char **v)
{
  FILE *in;
  FILE *out;
  char *name;
  char *value;
  char *in_name;
  char *out_name;

  if (n != 3) { printf("gimme <source> <dest>\n"); exit(1); }

  n = 0;

  in_name = v[1];
  out_name = v[2];

  in = fopen(in_name, "r"); if (in == NULL) { perror(in_name); exit(1); }
  out = fopen(out_name, "w"); if (out == NULL) { perror(out_name); exit(1); }

  fprintf(out, "/* generated file, do not edit by hand */\n\n");

  while (1) {
    get_line(in, &name, &value);
    if (name == NULL) break;
    printf("name '%s' value '%s'\n", name, value);
    if (isspace(value[strlen(value)-1])) {
      printf("bad value '%s' (no space at the end please!)\n", value);
      unlink(out_name);
      exit(1);
    }
    if (!strcmp(name, "ID")) {
      if (!new_unique_id(value)) { unlink(out_name); exit(1); }
      fprintf(out, "#define T_%s T_ID(%d)\n", value, n);
      n++;
    }
  }
  fprintf(out, "#define T_NUMBER_OF_IDS %d\n", n);

  fclose(in);
  fclose(out);

  return 0;
}
