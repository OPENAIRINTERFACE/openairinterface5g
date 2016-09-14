#include "rc.h"
#include "types.h"

#ifndef XML_PARSE_H_
#define XML_PARSE_H_

extern void        *xml_raw_data;
extern uint32_t     xml_raw_data_size;

extern types_t     *root;

int xml_parse_file(const char *filename);

int xml_parse_buffer(char *xml_buffer, const int size);

int dissect_signal(buffer_t *buffer, ui_set_signal_text_cb_t ui_set_signal_text_cb,
                   gpointer user_data);

int dissect_signal_header(buffer_t *buffer, ui_set_signal_text_cb_t ui_set_signal_text_cb,
                          gpointer cb_user_data);

#endif  /* XML_PARSE_H_ */
