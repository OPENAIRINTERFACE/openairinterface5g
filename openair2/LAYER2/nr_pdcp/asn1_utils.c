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

#include "pdcp.h"

int decode_t_reordering(int v)
{
  static int tab[36] = {
    0, 1, 2, 4, 5, 8, 10, 15, 20, 30, 40, 50, 60, 80, 100, 120, 140, 160, 180,
    200, 220, 240, 260, 280, 300, 500, 750, 1000, 1250, 1500, 1750, 2000,
    2250, 2500, 2750, 3000
  };

  if (v < 0 || v > 35) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_sn_size_ul(long s)
{
  if (s == 0) return 12;
  if (s == 1) return 18;
  LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
  exit(1);
}

int decode_sn_size_dl(long s)
{
  if (s == 0) return 12;
  if (s == 1) return 18;
  LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
  exit(1);
}

int decode_discard_timer(long v)
{
  static int tab[16] = {
    10, 20, 30, 40, 50, 60, 75, 100, 150, 200, 250, 300, 500, 750, 1500, -1,
  };

  if (v < 0 || v > 15) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}
