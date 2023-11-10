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

#include "cuup_cucp_if.h"

static e1_if_t e1_if;
static bool g_uses_e1;

e1_if_t *get_e1_if(void)
{
  return &e1_if;
}

bool e1_used(void)
{
  return g_uses_e1;
}

void nr_pdcp_e1_if_init(bool uses_e1)
{
  g_uses_e1 = uses_e1;

  if (uses_e1)
    cuup_cucp_init_e1ap(&e1_if);
  else
    cuup_cucp_init_direct(&e1_if);
}
