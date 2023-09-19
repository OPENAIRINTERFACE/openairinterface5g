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

#include <simde/x86/avx2.h>

void simde_mm128_separate_real_imag_parts(simde__m128i *out_re, simde__m128i *out_im, simde__m128i in0, simde__m128i in1)
{
  // Put in0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
  in0 = simde_mm_shufflelo_epi16(in0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
  in0 = simde_mm_shufflehi_epi16(in0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
  in0 = simde_mm_shuffle_epi32(in0, 0xd8);   //_MM_SHUFFLE(0,2,1,3));

  // Put xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
  in1 = simde_mm_shufflelo_epi16(in1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
  in1 = simde_mm_shufflehi_epi16(in1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
  in1 = simde_mm_shuffle_epi32(in1, 0xd8);   //_MM_SHUFFLE(0,2,1,3));

  *out_re = simde_mm_unpacklo_epi64(in0, in1);
  *out_im = simde_mm_unpackhi_epi64(in0, in1);
}

void simde_mm256_separate_real_imag_parts(simde__m256i *out_re, simde__m256i *out_im, simde__m256i in0, simde__m256i in1)
{
  // Put in0 = [Re(0,1,2,3)   Im(0,1,2,3)   Re(4,5,6,7)     Im(4,5,6,7)]
  in0 = simde_mm256_shufflelo_epi16(in0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
  in0 = simde_mm256_shufflehi_epi16(in0, 0xd8); //_MM_SHUFFLE(0,2,1,3));
  in0 = simde_mm256_shuffle_epi32(in0, 0xd8);   //_MM_SHUFFLE(0,2,1,3));

  // Put in1 = [Re(8,9,10,11) Im(8,9,10,11) Re(12,13,14,15) Im(12,13,14,15)]
  in1 = simde_mm256_shufflelo_epi16(in1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
  in1 = simde_mm256_shufflehi_epi16(in1, 0xd8); //_MM_SHUFFLE(0,2,1,3));
  in1 = simde_mm256_shuffle_epi32(in1, 0xd8);   //_MM_SHUFFLE(0,2,1,3));

  // Put tmp0 =[Re(0,1,2,3) Re(8,9,10,11) Re(4,5,6,7) Re(12,13,14,15)]
  simde__m256i tmp0 = simde_mm256_unpacklo_epi64(in0, in1);

  // Put tmp1 = [Im(0,1,2,3) Im(8,9,10,11) Im(4,5,6,7) Im(12,13,14,15)]
  simde__m256i tmp1 = simde_mm256_unpackhi_epi64(in0, in1);

  *out_re = simde_mm256_permute4x64_epi64(tmp0, 0xd8);
  *out_im = simde_mm256_permute4x64_epi64(tmp1, 0xd8);
}