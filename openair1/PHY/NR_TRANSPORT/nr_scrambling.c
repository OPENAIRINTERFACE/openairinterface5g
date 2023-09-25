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

#include "nr_transport_common_proto.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#define DEBUG_SCRAMBLING(a)
//#define DEBUG_SCRAMBLING(a) a
void nr_codeword_scrambling(uint8_t *in,
                            uint32_t size,
                            uint8_t q,
                            uint32_t Nid,
                            uint32_t n_RNTI,
                            uint32_t* out)
{
  uint32_t x1;
  uint32_t x2 = (n_RNTI<<15) + (q<<14) + Nid;
  uint32_t s = 0;

  s=lte_gold_generic(&x1, &x2, 1);
  for (int i=0; i<((size>>5)+((size&0x1f) > 0 ? 1 : 0)); i++) {
    simde__m256i c = ((simde__m256i*)in)[i];
    uint32_t in32 = simde_mm256_movemask_epi8(simde_mm256_slli_epi16(c,7));
    out[i]=(in32^s);
    DEBUG_SCRAMBLING(LOG_D(PHY, "in[%d] %x => %x\n", i, in32, out[i]));
    s=lte_gold_generic(&x1, &x2, 0);
  }
}

void nr_codeword_unscrambling(int16_t* llr, uint32_t size, uint8_t q, uint32_t Nid, uint32_t n_RNTI)
{
  uint32_t x1;
  uint32_t x2 = (n_RNTI << 15) + (q << 14) + Nid;
  uint32_t s = 0;

#if defined(__x86_64__) || defined(__i386__) || defined(__arm__) || defined(__aarch64__)
  uint8_t *s8=(uint8_t *)&s;
  simde__m128i *llr128 = (simde__m128i*)llr;
  s = lte_gold_generic(&x1, &x2, 1);

  for (int i = 0, j = 0; i < ((size >> 5) + ((size & 0x1f) > 0 ? 1 : 0)); i++, j += 4) {
    llr128[j]   = simde_mm_mullo_epi16(llr128[j],byte2m128i[s8[0]]);
    llr128[j+1] = simde_mm_mullo_epi16(llr128[j+1],byte2m128i[s8[1]]);
    llr128[j+2] = simde_mm_mullo_epi16(llr128[j+2],byte2m128i[s8[2]]);
    llr128[j+3] = simde_mm_mullo_epi16(llr128[j+3],byte2m128i[s8[3]]);
    s = lte_gold_generic(&x1, &x2, 0);
  }
#else
  uint8_t reset = 1;

  for (uint32_t i=0; i<size; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
    }
    if (((s>>(i&0x1f))&1)==1)
      llr[i] = -llr[i];
  }
#endif
}

void nr_codeword_unscrambling_init(int16_t *s2, uint32_t size, uint8_t q, uint32_t Nid, uint32_t n_RNTI)
{
  uint32_t x1;
  uint32_t x2 = (n_RNTI << 15) + (q << 14) + Nid;
  simde__m128i *s128=(simde__m128i *)s2;

  uint32_t s = lte_gold_generic(&x1, &x2, 1);
  uint8_t *s8=(uint8_t *)&s;

  for (int i = 0; i < (size +31) >> 5; i++) {
    *s128++ = byte2m128i[s8[0]];
    *s128++ = byte2m128i[s8[1]];
    *s128++ = byte2m128i[s8[2]];
    *s128++ = byte2m128i[s8[3]];
    s = lte_gold_generic(&x1, &x2, 0);
  }
}
