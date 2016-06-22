#ifndef _LOGGER_H_
#define _LOGGER_H_

typedef void logger;

logger *new_framelog(void *event_handler, void *database,
    char *event_name, char *subframe_varname, char *buffer_varname);
logger *new_textlog(void *event_handler, void *database,
    char *event_name, char *format);
logger *new_ttilog(void *event_handler, void *database,
    char *event_name, char *frame_varname, char *subframe_varname,
    char *data_varname, int convert_to_dB);
logger *new_timelog(void *event_handler, void *database, char *event_name);
logger *new_ticklog(void *event_handler, void *database,
    char *event_name, char *frame_name, char *subframe_name);

void framelog_set_skip(logger *_this, int skip_delay);

#include "view/view.h"

void logger_add_view(logger *l, view *v);
void logger_set_filter(logger *l, void *filter);

#endif /* _LOGGER_H_ */
