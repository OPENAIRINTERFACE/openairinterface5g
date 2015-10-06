/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "EpsBearerIdentity.h"
#include "ProcedureTransactionIdentity.h"
#include "MessageType.h"
#include "EsmCause.h"

#ifndef ESM_STATUS_H_
#define ESM_STATUS_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ESM_STATUS_MINIMUM_LENGTH ( \
    ESM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define ESM_STATUS_MAXIMUM_LENGTH ( \
    ESM_CAUSE_MAXIMUM_LENGTH )


/*
 * Message name: ESM status
 * Description: This message is sent by the network or the UE to pass information on the status of the indicated EPS bearer context and report certain error conditions (e.g. as listed in clause 7). See table 8.3.15.1.
 * Significance: dual
 * Direction: both
 */

typedef struct esm_status_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator        protocoldiscriminator:4;
  EpsBearerIdentity            epsbeareridentity:4;
  ProcedureTransactionIdentity proceduretransactionidentity;
  MessageType                  messagetype;
  EsmCause                     esmcause;
} esm_status_msg;

int decode_esm_status(esm_status_msg *esmstatus, uint8_t *buffer, uint32_t len);

int encode_esm_status(esm_status_msg *esmstatus, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ESM_STATUS_H_) */

