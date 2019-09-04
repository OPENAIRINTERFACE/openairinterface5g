#include "logger.h"
#include "logger_defs.h"
#include "handler.h"
#include "database.h"
#include "view/view.h"
#include "utils.h"
#include "filter/filter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum format_item_type {
  INSTRING,
  INT, ULONG, STRING, BUFFER };

struct format_item {
  enum format_item_type type;
  union {
    /* INSTRING */
    char *s;
    /* others */
    int event_arg;
  };
};

struct textlog {
  struct logger common;
  char *format;
  void *database;
  /* parsed format string */
  struct format_item *f;
  int fsize;
  /* local output buffer */
  OBUF o;
  int dump_buffer;
  int raw_time;
};

static void _event(void *p, event e)
{
  struct textlog *l = p;
  int i, j;
#ifdef T_SEND_TIME
  struct tm *t;
  char tt[64];
#endif

  if (l->common.filter != NULL && filter_eval(l->common.filter, e) == 0)
    return;

  l->o.osize = 0;

#ifdef T_SEND_TIME
  t = localtime(&e.sending_time.tv_sec);
  if (l->raw_time)
    sprintf(tt, "%2.2d:%2.2d:%2.2d.%9.9ld [%ld]: ",
        t->tm_hour, t->tm_min, t->tm_sec,
        e.sending_time.tv_nsec, e.sending_time.tv_sec);
  else
    sprintf(tt, "%2.2d:%2.2d:%2.2d.%9.9ld: ", t->tm_hour, t->tm_min, t->tm_sec,
        e.sending_time.tv_nsec);
  PUTS(&l->o, tt);
#endif

  for (i = 0; i < l->fsize; i++)
  switch(l->f[i].type) {
  case INSTRING: PUTS(&l->o, l->f[i].s); break;
  case INT:      PUTI(&l->o, e.e[l->f[i].event_arg].i); break;
  case ULONG:    PUTUL(&l->o, e.e[l->f[i].event_arg].ul); break;
  case STRING:   PUTS_CLEAN(&l->o, e.e[l->f[i].event_arg].s); break;
  case BUFFER:
    PUTS(&l->o, "{buffer size:");
    PUTI(&l->o, e.e[l->f[i].event_arg].bsize);
    if (l->dump_buffer) {
      PUTS(&l->o, " [");
      for (j = 0; j < e.e[l->f[i].event_arg].bsize; j++) {
        PUTX2(&l->o, ((unsigned char *)e.e[l->f[i].event_arg].b)[j]);
        PUTC(&l->o, ' ');
      }
      PUTS(&l->o, "]");
    }
    PUTS(&l->o, "}");
    break;
  }
  PUTC(&l->o, 0);

  for (i = 0; i < l->common.vsize; i++)
    l->common.v[i]->append(l->common.v[i], l->o.obuf);
}

enum chunk_type { C_ERROR, C_STRING, C_ARG_NAME, C_EVENT_NAME };
struct chunk {
  enum chunk_type type;
  char *s;
  enum format_item_type it;
  int event_arg;
};

/* TODO: speed it up? */
static int find_argument(char *name, database_event_format f,
    enum format_item_type *it, int *event_arg)
{
  int i;
  for (i = 0; i < f.count; i++) if (!strcmp(name, f.name[i])) break;
  if (i == f.count) return 0;
  *event_arg = i;
  if (!strcmp(f.type[i], "int"))         *it = INT;
  else if (!strcmp(f.type[i], "ulong"))  *it = ULONG;
  else if (!strcmp(f.type[i], "string")) *it = STRING;
  else if (!strcmp(f.type[i], "buffer")) *it = BUFFER;
  else return 0;
  return 1;
}

static struct chunk next_chunk(char **s, database_event_format f)
{
  char *cur = *s;
  char *name;
  enum format_item_type it;
  int event_arg;

  /* argument in [ ] */
  if (*cur == '[') {
    *cur = 0;
    cur++;
    name = cur;
    /* no \ allowed there */
    while (*cur && *cur != ']' && *cur != '\\') cur++;
    if (*cur != ']') goto error;
    *cur = 0;
    cur++;
    *s = cur;
    if (find_argument(name, f, &it, &event_arg) == 0) goto error;
    return (struct chunk){type:C_ARG_NAME, s:name, it:it, event_arg:event_arg};
  }

  /* { } is name of event (anything in between is smashed) */
  if (*cur == '{') {
    *cur = 0;
    cur++;
    while (*cur && *cur != '}') cur++;
    if (*cur != '}') goto error;
    *cur = 0;
    cur++;
    *s = cur;
    return (struct chunk){type:C_EVENT_NAME};
  }

  /* anything but [ and { is raw string */
  /* TODO: deal with \ */
  name = cur;
  while (*cur && *cur != '[' && *cur != '{') cur++;
  *s = cur;
  return (struct chunk){type:C_STRING, s:name};

error:
  return (struct chunk){type:C_ERROR};
}

logger *new_textlog(event_handler *h, void *database,
    char *event_name, char *format)
{
  struct textlog *ret;
  int event_id;
  database_event_format f;
  char *cur;

  ret = calloc(1, sizeof(struct textlog)); if (ret == NULL) abort();

  ret->common.event_name = strdup(event_name);
  if (ret->common.event_name == NULL) abort();
  ret->format = strdup(format); if (ret->format == NULL) abort();
  ret->database = database;

  event_id = event_id_from_name(database, event_name);

  ret->common.handler_id = register_handler_function(h,event_id,_event,ret);

  f = get_format(database, event_id);

  /* we won't get more than strlen(format) "chunks" */
  ret->f = malloc(sizeof(struct format_item) * strlen(format));
  if (ret->f == NULL) abort();

  cur = ret->format;

  while (*cur) {
    struct chunk c = next_chunk(&cur, f);
    switch (c.type) {
    case C_ERROR: goto error;
    case C_STRING:
      ret->f[ret->fsize].type = INSTRING;
      ret->f[ret->fsize].s = c.s;
      break;
    case C_ARG_NAME:
      ret->f[ret->fsize].type = c.it;
      ret->f[ret->fsize].event_arg = c.event_arg;
      break;
    case C_EVENT_NAME:
      ret->f[ret->fsize].type = INSTRING;
      ret->f[ret->fsize].s = ret->common.event_name;
      break;
    }
    ret->fsize++;
  }

  return ret;

error:
  printf("%s:%d: bad format '%s'\n", __FILE__, __LINE__, format);
  abort();
}

/****************************************************************************/
/*                             public functions                             */
/****************************************************************************/

void textlog_dump_buffer(logger *_this, int dump_buffer)
{
  struct textlog *l = _this;
  l->dump_buffer = dump_buffer;
}

void textlog_raw_time(logger *_this, int raw_time)
{
  struct textlog *l = _this;
  l->raw_time = raw_time;
}
