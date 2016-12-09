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
logger *new_ticked_ttilog(void *event_handler, void *database,
    char *tick_event_name, char *frame_varname, char *subframe_varname,
    char *event_name, char *data_varname,
    int convert_to_dB, float empty_value);
logger *new_throughputlog(void *event_handler, void *database,
    char *tick_event_name, char *frame_varname, char *subframe_varname,
    char *event_name, char *data_varname);
logger *new_timelog(void *event_handler, void *database, char *event_name);
logger *new_ticklog(void *event_handler, void *database,
    char *event_name, char *frame_name, char *subframe_name);
logger *new_iqlog(void *event_handler, void *database,
    char *event_name, char *nb_rb, char *N_RB_UL, char *symbols_per_tti,
    char *buffer_varname);
logger *new_iqdotlog(void *event_handler, void *database,
    char *event_name, char *I, char *Q);

void framelog_set_skip(logger *_this, int skip_delay);
void framelog_set_update_only_at_sf9(logger *_this, int update_only_at_sf9);

void textlog_dump_buffer(logger *_this, int dump_buffer);

#include "view/view.h"

void logger_add_view(logger *l, view *v);
void logger_set_filter(logger *l, void *filter);
void ticked_ttilog_set_tick_filter(logger *l, void *filter);

#endif /* _LOGGER_H_ */
