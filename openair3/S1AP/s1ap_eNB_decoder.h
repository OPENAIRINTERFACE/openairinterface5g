#include <stdint.h>
#include "s1ap_ies_defs.h"

#ifndef S1AP_ENB_DECODER_H_
#define S1AP_ENB_DECODER_H_

int s1ap_eNB_decode_pdu(s1ap_message *message, const uint8_t * const buffer,
                        const uint32_t length) __attribute__ ((warn_unused_result));

#endif /* S1AP_ENB_DECODER_H_ */
