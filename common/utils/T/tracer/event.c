#include "event.h"
#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef T_SEND_TIME
event new_event(struct timespec sending_time, int type,
    int length, char *buffer, void *database)
#else
event new_event(int type, int length, char *buffer, void *database)
#endif
{
  database_event_format f;
  event e;
  int i;
  int offset;

#ifdef T_SEND_TIME
  e.sending_time = sending_time;
#endif
  e.type = type;
  e.buffer = buffer;

  f = get_format(database, type);

  e.ecount = f.count;

  offset = 0;

  /* setup offsets */
  /* TODO: speedup (no strcmp, string event to include length at head) */
  for (i = 0; i < f.count; i++) {
    //e.e[i].offset = offset;
    if (!strcmp(f.type[i], "int")) {
      e.e[i].type = EVENT_INT;
      e.e[i].i = *(int *)(&buffer[offset]);
      offset += 4;
    } else if (!strcmp(f.type[i], "ulong")) {
      e.e[i].type = EVENT_ULONG;
      e.e[i].ul = *(unsigned long *)(&buffer[offset]);
      offset += sizeof(unsigned long);
    } else if (!strcmp(f.type[i], "string")) {
      e.e[i].type = EVENT_STRING;
      e.e[i].s = &buffer[offset];
      while (buffer[offset]) offset++;
      offset++;
    } else if (!strcmp(f.type[i], "buffer")) {
      int len;
      e.e[i].type = EVENT_BUFFER;
      len = *(int *)(&buffer[offset]);
      e.e[i].bsize = len;
      e.e[i].b = &buffer[offset+sizeof(int)];
      offset += len+sizeof(int);
    } else {
      printf("unhandled type '%s'\n", f.type[i]);
      abort();
    }
  }

  if (e.ecount==0) { printf("FORMAT not set in event %d\n", type); abort(); }

  return e;
}
