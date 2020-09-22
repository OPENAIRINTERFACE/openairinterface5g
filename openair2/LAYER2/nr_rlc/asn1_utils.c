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

int decode_t_reassembly(int v)
{
  static int tab[31] = {
    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90,
    95, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200
  };

  if (v < 0 || v > 30) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_t_status_prohibit(int v)
{
  static int tab[62] = {
    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90,
    95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165,
    170, 175, 180, 185, 190, 195, 200, 205, 210, 215, 220, 225, 230, 235, 240,
    245, 250, 300, 350, 400, 450, 500, 800, 1000, 1200, 1600, 2000, 2400
  };

  if (v < 0 || v > 61) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_t_poll_retransmit(int v)
{
  static int tab[59] = {
    5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95,
    100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165, 170,
    175, 180, 185, 190, 195, 200, 205, 210, 215, 220, 225, 230, 235, 240, 245,
    250, 300, 350, 400, 450, 500, 800, 1000, 2000, 4000
  };

  if (v < 0 || v > 58) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_poll_pdu(int v)
{
  static int tab[24] = {
    4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 6144, 8192, 12288,
    16384, 20480, 24576, 28672, 32768, 40960, 49152, 57344, 65536
    -1 /* -1 means infinity */
  };

  if (v < 0 || v > 23) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_poll_byte(int v)
{
  static int tab[44] = {
    /* KB */
    1024 * 1,    1024 * 2,    1024 * 5,    1024 * 8,    1024 * 10,
    1024 * 15,   1024 * 25,   1024 * 50,   1024 * 75,   1024 * 100,
    1024 * 125,  1024 * 250,  1024 * 375,  1024 * 500,  1024 * 750,
    1024 * 1000, 1024 * 1250, 1024 * 1500, 1024 * 2000, 1024 * 3000,
    1024 * 4000, 1024 * 4500, 1024 * 5000, 1024 * 5500, 1024 * 6000,
    1024 * 6500, 1024 * 7000, 1024 * 7500,
    /* MB */
    1024 * 1024 * 8,  1024 * 1024 * 9,  1024 * 1024 * 10, 1024 * 1024 * 11,
    1024 * 1024 * 12, 1024 * 1024 * 13, 1024 * 1024 * 14, 1024 * 1024 * 15,
    1024 * 1024 * 16, 1024 * 1024 * 17, 1024 * 1024 * 18, 1024 * 1024 * 20,
    1024 * 1024 * 25, 1024 * 1024 * 30, 1024 * 1024 * 40,
    -1 /* -1 means infinity */
  };

  if (v < 0 || v > 43) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if (tab[v] == -1) return -1;
  return tab[v] * 1024;
}

int decode_max_retx_threshold(int v)
{
  static int tab[8] = {
    1, 2, 3, 4, 6, 8, 16, 32
  };

  if (v < 0 || v > 7) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_sn_field_length_um(int v)
{
  static int tab[2] = {
    6, 12
  };

  if (v < 0 || v > 1) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}

int decode_sn_field_length_am(int v)
{
  static int tab[2] = {
    12, 18
  };

  if (v < 0 || v > 1) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  return tab[v];
}
