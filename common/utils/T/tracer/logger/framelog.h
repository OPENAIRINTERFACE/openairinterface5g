#ifndef _FRAMELOG_H_
#define _FRAMELOG_H_

typedef void framelog;

framelog *new_framelog(void *event_handler, void *database,
    char *event_name, char *subframe_varname, char *buffer_varname);

#include "view/view.h"

void framelog_add_view(framelog *l, view *v);

#endif /* _FRAMELOG_H_ */
