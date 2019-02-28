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

/*! \file PHY/NR_UE_TRANSPORT/nr_ulsch.c
* \brief Top-level routines for transmission of the PUSCH TS 38.211 v 15.4.0
* \author Khalid Ahmed
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: khalid.ahmed@iis.fraunhofer.de
* \note
* \warning
*/
#include <stdint.h>
#include "common/utils/assertions.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/defs_nr_common.h"

extern short nr_mod_table[NR_MOD_TABLE_SIZE_SHORT];



void nr_pusch_codeword_scrambling(uint8_t *in,
                         uint16_t size,
                         uint32_t Nid,
                         uint32_t n_RNTI,
                         uint32_t* out) {

  uint8_t reset, b_idx;
  uint32_t x1, x2, s=0;

  reset = 1;
  x2 = (n_RNTI<<15) + Nid;

  for (int i=0; i<size; i++) {
    b_idx = i&0x1f;
    if (b_idx==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
      if (i)
        out++;
    }
    if (in[i]==NR_PUSCH_x)
      *out ^= 1<<b_idx;
    else if (in[i]==NR_PUSCH_y)
      *out ^= (*out & (1<<b_idx-1))<<b_idx;
    else
      *out ^= (((in[i])&1) ^ ((s>>b_idx)&1))<<b_idx;
    //printf("i %d b_idx %d in %d s 0x%08x out 0x%08x\n", i, b_idx, in[i], s, *out);
  }

}

// This function is copied from PHY/NR_TRANSPORT/nr_dlsch.c
void nr_pusch_codeword_modulation(uint32_t *in,
                         uint8_t  Qm,
                         uint32_t length,
                         int16_t *out) {

  uint16_t offset = (Qm==2)? NR_MOD_TABLE_QPSK_OFFSET : (Qm==4)? NR_MOD_TABLE_QAM16_OFFSET : \
                    (Qm==6)? NR_MOD_TABLE_QAM64_OFFSET: (Qm==8)? NR_MOD_TABLE_QAM256_OFFSET : 0;
  AssertFatal(offset, "Invalid modulation order %d\n", Qm);

  for (int i=0; i<length/Qm; i++) {
    uint8_t idx = 0, b_idx;

    for (int j=0; j<Qm; j++) {
      b_idx = (i*Qm+j)&0x1f;
      if (i && (!b_idx))
        in++;
      idx ^= (((*in)>>b_idx)&1)<<(Qm-j-1);
    }

    out[i<<1] = nr_mod_table[(offset+idx)<<1];
    out[(i<<1)+1] = nr_mod_table[((offset+idx)<<1)+1];
  }
}
