#ifndef _LOGGER_DEFS_H_
#define _LOGGER_DEFS_H_

#include "view/view.h"

struct logger {
  char *event_name;
  unsigned long handler_id;
  /* list of views */
  view **v;
  int vsize;
  /* filter - NULL if no filter set */
  void *filter;
};

#endif /* _LOGGER_DEFS_H_ */
