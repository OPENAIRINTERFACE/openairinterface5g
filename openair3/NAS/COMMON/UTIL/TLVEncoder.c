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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "TLVEncoder.h"

int errorCodeEncoder = 0;

const char *const errorCodeStringEncoder[] = {
    "No error",
    "Buffer NULL",
    "Buffer too short",
    "Octet string too long for IEI",
    "Wrong message type",
    "Protocol not supported",
};

void tlv_encode_perror(void)
{
  if (errorCodeEncoder >= 0)
    // No error or TLV_DECODE_ERR_OK
    return;

  printf("error: (%d, %s)\n", errorCodeEncoder, errorCodeStringEncoder[errorCodeEncoder * -1]);
}

