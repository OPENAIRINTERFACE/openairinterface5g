#ifndef _EVENT_H_
#define _EVENT_H_

#include "../T_defs.h"

enum event_arg_type {
  EVENT_INT,
  EVENT_STRING,
  EVENT_BUFFER
};

typedef struct {
  enum event_arg_type type;
  //int offset;
  union {
    int i;
    char *s;
    struct {
      int bsize;
      void *b;
    };
  };
} event_arg;

typedef struct {
  int type;
  char *buffer;
  event_arg e[T_MAX_ARGS];
  int ecount;
} event;

event new_event(int type, int length, char *buffer, void *database);

#endif /* _EVENT_H_ */
