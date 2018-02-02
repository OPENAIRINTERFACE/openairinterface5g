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
#include <unistd.h>

#include "test_util.h"

#include "secu_defs.h"

typedef struct {
  uint8_t  kasme[32];
  uint8_t  kenb_exp[16];
  uint32_t nas_count;
} test_secu_t;

const test_secu_t kenb_test_vector[] = {
  {
    .kasme = {

    },
    .kenb_exp = {

    },
    .nas_count = 0x001FB39C;
  }
};

void doit (void)
{
  int i;

  for (i = 0; i < sizeof(kenb_test_vector) / sizeof(test_secu_t); i++) {
    uint8_t kenb[32];

    derive_keNB(kenb_test_vector[i].kasme, kenb_test_vector[i].nas_count, kenb);

    if (compare_buffer(kenb_test_vector[i].kenb, 32, kenb, 32) == 0) {
      success("kenb derivation");
    } else {
      fail("kenb derivation");
    }
  }
}
