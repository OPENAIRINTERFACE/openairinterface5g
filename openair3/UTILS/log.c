#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "log.h"

/* mme log */
int log_enabled = 0;

int log_init(const mme_config_t *mme_config_p,
             log_specific_init_t specific_init)
{
  if (mme_config_p->verbosity_level == 1) {
    log_enabled = 1;
  } else if (mme_config_p->verbosity_level == 2) {
    log_enabled = 1;
  } else {
    log_enabled = 0;
  }

  return specific_init(mme_config_p->verbosity_level);
}
