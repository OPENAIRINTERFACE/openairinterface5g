#ifndef _EVENT_SELECTOR_H_
#define _EVENT_SELECTOR_H_

#include "gui/gui.h"

typedef void event_selector;

event_selector *setup_event_selector(gui *g, void *database, int socket,
    int *is_on);

#endif /* _EVENT_SELECTOR_H_ */
