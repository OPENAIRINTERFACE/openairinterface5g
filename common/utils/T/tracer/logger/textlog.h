#ifndef _TEXTLOG_H_
#define _TEXTLOG_H_

typedef void textlog;

textlog *new_textlog(void *event_handler, void *database,
    char *event_name, char *format);

#include "view/view.h"

void textlog_add_view(textlog *l, view *v);

#endif /* _TEXTLOG_H_ */
