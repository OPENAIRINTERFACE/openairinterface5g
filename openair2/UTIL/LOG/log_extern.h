#include"log.h"

extern log_t *g_log;

#if !defined(LOG_NO_THREAD)
extern LOG_params log_list[2000];
extern pthread_mutex_t log_lock;
extern pthread_cond_t log_notify;
extern int log_shutdown;
#endif

extern mapping log_level_names[];
extern mapping log_verbosity_names[];
