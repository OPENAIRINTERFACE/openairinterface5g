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

#include "secu_defs.h"

#include "common/utils/assertions.h"
#include "common/utils/LOG/log.h"

#include "nas_stream_eea0.h"
#include "nas_stream_eea1.h"
#include "nas_stream_eea2.h"

#include "nas_stream_eia1.h"
#include "nas_stream_eia2.h"

void stream_compute_integrity(eia_alg_id_e alg, nas_stream_cipher_t const* stream_cipher, uint8_t out[4])
{
  if (alg == EIA0_ALG_ID) {
    LOG_E(OSA, "Provided integrity algorithm is currently not supported = %u\n", alg);
  } else if (alg == EIA1_128_ALG_ID) {
    LOG_D(OSA, "EIA1 algorithm applied for integrity\n");
    nas_stream_encrypt_eia1(stream_cipher, out);
  } else if (alg == EIA2_128_ALG_ID) {
    LOG_D(OSA, "EIA2 algorithm applied for integrity\n");
    nas_stream_encrypt_eia2(stream_cipher, out);
  } else {
    LOG_E(OSA, "Provided integrity algorithm is currently not supported = %u\n", alg);
    AssertFatal(0 != 0, "Unknown Algorithm type");
  }
}

void stream_compute_encrypt(eea_alg_id_e alg, nas_stream_cipher_t const* stream_cipher, uint8_t* out)
{
  if (alg == EEA0_ALG_ID) {
    LOG_D(OSA, "EEA0 algorithm applied for encryption\n");
    nas_stream_encrypt_eea0(stream_cipher, out);
  } else if (alg == EEA1_128_ALG_ID) {
    LOG_D(OSA, "EEA1 algorithm applied for encryption\n");
    nas_stream_encrypt_eea1(stream_cipher, out);
  } else if (alg == EEA2_128_ALG_ID) {
    LOG_D(OSA, "EEA2 algorithm applied for  encryption\n");
    nas_stream_encrypt_eea2(stream_cipher, out);
  } else {
    LOG_E(OSA, "Provided encrypt algorithm is currently not supported = %u\n", alg);
    AssertFatal(0 != 0, "Unknown Algorithm type");
  }
}

