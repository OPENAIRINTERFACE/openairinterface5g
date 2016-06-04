#ifndef _EVENT_SELECTOR_H_
#define _EVENT_SELECTOR_H_

#include "gui/gui.h"

typedef void event_selector;

event_selector *setup_event_selector(gui *g, void *database, int *is_on,
    void (*change_callback)(void *), void *change_callback_data);

#endif /* _EVENT_SELECTOR_H_ */
