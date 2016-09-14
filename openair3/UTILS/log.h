#include "mme_config.h"

#ifndef LOG_H_
#define LOG_H_

/* asn1c debug */
extern int asn_debug;
extern int asn1_xer_print;
extern int fd_g_debug_lvl;

typedef int (*log_specific_init_t)(int log_level);

int log_init(const mme_config_t *mme_config,
             log_specific_init_t specific_init);

#endif /* LOG_H_ */
