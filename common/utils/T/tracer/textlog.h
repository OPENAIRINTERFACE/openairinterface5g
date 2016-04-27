#ifndef _TEXTLOG_H_
#define _TEXTLOG_H_

typedef void textlog;

textlog *new_textlog(void *event_handler, void *database,
    char *event_name, char *format);

#endif /* _TEXTLOG_H_ */
