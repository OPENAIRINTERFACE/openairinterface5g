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

#include "rlc.h"

int decode_t_reordering(int v)
{
  static const int tab[32] = {0,  5,  10, 15, 20,  25,  30,  35,  40,  45,  50,  55,  60,  65,  70,  75,
                              80, 85, 90, 95, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 1600};

  if (v < 0 || v > 31) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_t_status_prohibit(int v)
{
  static const int tab[62] = {0,   5,   10,  15,  20,  25,  30,  35,  40,  45,   50,   55,   60,   65,  70,  75,
                              80,  85,  90,  95,  100, 105, 110, 115, 120, 125,  130,  135,  140,  145, 150, 155,
                              160, 165, 170, 175, 180, 185, 190, 195, 200, 205,  210,  215,  220,  225, 230, 235,
                              240, 245, 250, 300, 350, 400, 450, 500, 800, 1000, 1200, 1600, 2000, 2400};

  if (v < 0 || v > 61) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_t_poll_retransmit(int v)
{
  static const int tab[59] = {5,   10,  15,  20,  25,  30,  35,  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,   90,   95,  100,
                              105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165, 170, 175, 180, 185,  190,  195, 200,
                              205, 210, 215, 220, 225, 230, 235, 240, 245, 250, 300, 350, 400, 450, 500, 800, 1000, 2000, 4000};

  if (v < 0 || v > 58) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_poll_pdu(int v)
{
  static const int tab[8] = {
      4,
      8,
      16,
      32,
      64,
      128,
      256,
      -1 /* -1 means infinity */
  };

  if (v < 0 || v > 7) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_poll_byte(int v)
{
  static const int tab[15] = {
      25,
      50,
      75,
      100,
      125,
      250,
      375,
      500,
      750,
      1000,
      1250,
      1500,
      2000,
      3000,
      -1 /* -1 means infinity */
  };

  if (v < 0 || v > 14) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if (tab[v] == -1) return -1;
  return tab[v] * 1024;
}

int decode_max_retx_threshold(int v)
{
  static const int tab[8] = {1, 2, 3, 4, 6, 8, 16, 32};

  if (v < 0 || v > 7) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_sn_field_length(int v)
{
  static const int tab[2] = {5, 10};

  if (v < 0 || v > 1) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}
