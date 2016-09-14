#include <stdio.h>
#include <stdint.h>

/** @defgroup _x2ap_impl_ X2AP Layer Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

#ifndef X2AP_H_
#define X2AP_H_

typedef struct x2ap_config_s {
} x2ap_config_t;

#if defined(OAI_EMU)
#else
extern x2ap_config_t x2ap_config;
#endif

void *x2ap_task(void *arg);

#endif /* X2AP_H_ */

/**
 * @}
 */
