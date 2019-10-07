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


#include "PHY/NR_TRANSPORT/nr_transport_proto.h"

//#define NR_CSIRS_DEBUG

int nr_generate_csi_rs(uint32_t **gold_csi_rs,
                       int32_t *txdataF,
                       NR_DL_FRAME_PARMS frame_parms,
                       nfapi_nr_csi_rs_pdu_t csi_params)
{

  uint16_t b = csi_params.freq_domain;
  uint8_t size, ports, kprime, lprime, i;
  uint8_t j[16], k_n[6], koverline[16], loverline[16];
  int found = 0;
  uint8_t fi = 0;

  switch (csi_params.row) {
  // implementation of table 7.4.1.5.3-1 of 38.211
  // lprime and kprime are the max value of l' and k'
  case 1:
    ports = 1;
    kprime = 0;
    lprime = 0;
    size = 3;
    while (found < 1) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi;
        found++;
      }
      else
        fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = 0;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[0] + i<<2;
    }
    break;

  case 2:
    ports = 1;
    kprime = 0;
    lprime = 0;
    size = 1;
    while (found < 1) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi;
        found++;
      }
      else
        fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = 0;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[0];
    }
    break;

  case 3:
    ports = 2;
    kprime = 1;
    lprime = 0;
    size = 1;
    while (found < 1) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      else
        fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = 0;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[0];
    }
    break;

  case 4:
    ports = 4;
    kprime = 1;
    lprime = 0;
    size = 2;
    while (found < 1) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<2;
        found++;
      }
      else
        fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[0] + i<<1;
    }
    break;

  case 5:
    ports = 4;
    kprime = 1;
    lprime = 0;
    size = 2;
    while (found < 1) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      else
        fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0 + i;
      koverline[i] = k_n[0];
    }
    break;

  case 6:
    ports = 8;
    kprime = 1;
    lprime = 0;
    size = 4;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 7:
    ports = 8;
    kprime = 1;
    lprime = 0;
    size = 4;
    while (found < 2) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0 + i>>1;
      koverline[i] = k_n[i%2];
    }
    break;

  case 8:
    ports = 8;
    kprime = 1;
    lprime = 1;
    size = 2;
    while (found < 2) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 9:
    ports = 12;
    kprime = 1;
    lprime = 0;
    size = 6;
    while (found < 6) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 10:
    ports = 12;
    kprime = 1;
    lprime = 1;
    size = 3;
    while (found < 3) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[i];
    }
    break;
  }

  case 11:
    ports = 16;
    kprime = 1;
    lprime = 0;
    size = 8;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0 + i>>2;
      koverline[i] = k_n[i%4];
    }
    break;
  }

  case 12:
    ports = 16;
    kprime = 1;
    lprime = 1;
    size = 4;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 13:
    ports = 24;
    kprime = 1;
    lprime = 0;
    size = 12;
    while (found < 3) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      if (i<6)
        loverline[i] = csi_params.symb_l0 + i/3;
      else
        loverline[i] = csi_params.symb_l1 + i/9;
      koverline[i] = k_n[i%3];
    }
    break;

  case 14:
    ports = 24;
    kprime = 1;
    lprime = 1;
    size = 6;
    while (found < 3) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      if (i<3)
        loverline[i] = csi_params.symb_l0;
      else
        loverline[i] = csi_params.symb_l1;
      koverline[i] = k_n[i%3];
    }
    break;

  case 15:
    ports = 24;
    kprime = 1;
    lprime = 3;
    size = 3;
    while (found < 3) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 16:
    ports = 32;
    kprime = 1;
    lprime = 0;
    size = 16;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      if (i<8)
        loverline[i] = csi_params.symb_l0 + i>>2;
      else
        loverline[i] = csi_params.symb_l1 + i>>4;
      koverline[i] = k_n[i%4];
    }
    break;

  case 17:
    ports = 32;
    kprime = 1;
    lprime = 1;
    size = 8;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      if (i<4)
        loverline[i] = csi_params.symb_l0;
      else
        loverline[i] = csi_params.symb_l1;
      koverline[i] = k_n[i%4];
    }
    break;

  case 18:
    ports = 32;
    kprime = 1;
    lprime = 3;
    size = 4;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params.symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  }

  return 0;
}
