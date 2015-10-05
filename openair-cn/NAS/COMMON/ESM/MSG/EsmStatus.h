/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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

