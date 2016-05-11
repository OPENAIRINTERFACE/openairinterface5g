#ifndef _TTILOG_H_
#define _TTILOG_H_

typedef void ttilog;

ttilog *new_ttilog(void *event_handler, void *database,
    char *event_name, char *frame_varname, char *subframe_varname,
    char *data_varname, int convert_to_dB);

#include "view/view.h"

void ttilog_add_view(ttilog *l, view *v);

#endif /* _TTILOG_H_ */
