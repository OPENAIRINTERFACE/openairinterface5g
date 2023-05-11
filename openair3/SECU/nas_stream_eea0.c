/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "nas_stream_eea0.h"

#include "common/utils/assertions.h"
#include "common/utils/LOG/log.h"

#include <string.h>

void nas_stream_encrypt_eea0(nas_stream_cipher_t const *stream_cipher, uint8_t *out)
{
  DevAssert(stream_cipher != NULL);
  DevAssert(out != NULL);

  LOG_D(OSA,
        "Entering stream_encrypt_eea0, bits length %u, bearer %u, "
        "count %u, direction %s\n",
        stream_cipher->blength,
        stream_cipher->bearer,
        stream_cipher->count,
        stream_cipher->direction == SECU_DIRECTION_DOWNLINK ? "Downlink" : "Uplink");

  uint32_t byte_length = (stream_cipher->blength + 7) >> 3;
  memcpy(out, stream_cipher->message, byte_length);
}
