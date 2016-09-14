#include <stdio.h>
#include <stdint.h>

/** @defgroup _s1ap_impl_ S1AP Layer Reference Implementation for eNB
 * @ingroup _ref_implementation_
 * @{
 */

#ifndef S1AP_ENB_H_
#define S1AP_ENB_H_

typedef struct s1ap_eNB_config_s {
  // MME related params
  unsigned char mme_enabled;          ///< MME enabled ?
} s1ap_eNB_config_t;

#if defined(OAI_EMU)
# define EPC_MODE_ENABLED       oai_emulation.info.s1ap_config.mme_enabled
#else
extern s1ap_eNB_config_t s1ap_config;

# define EPC_MODE_ENABLED       s1ap_config.mme_enabled
#endif

void *s1ap_eNB_task(void *arg);

uint32_t s1ap_generate_eNB_id(void);

#endif /* S1AP_ENB_H_ */

/**
 * @}
 */
