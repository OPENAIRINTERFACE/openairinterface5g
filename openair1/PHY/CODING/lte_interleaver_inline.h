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

extern unsigned int threegpplte_interleaver_output;
extern unsigned int threegpplte_interleaver_tmp;

static inline void threegpplte_interleaver_reset(void)
{
  threegpplte_interleaver_output = 0;
  threegpplte_interleaver_tmp    = 0;
}

static inline unsigned short threegpplte_interleaver(unsigned short f1,
    unsigned short f2,
    unsigned short K)
{

  threegpplte_interleaver_tmp = (threegpplte_interleaver_tmp+(f2<<1));

  threegpplte_interleaver_output = (threegpplte_interleaver_output + threegpplte_interleaver_tmp + f1 - f2)%K;

#ifdef DEBUG_TURBO_ENCODER
  printf("Interleaver output %d\n",threegpplte_interleaver_output);
#endif
  return(threegpplte_interleaver_output);
}


static inline short threegpp_interleaver_parameters(unsigned short bytes_per_codeword)
{
  if (bytes_per_codeword<=64)
    return (bytes_per_codeword-5);
  else if (bytes_per_codeword <=128)
    return (59 + ((bytes_per_codeword-64)>>1));
  else if (bytes_per_codeword <= 256)
    return (91 + ((bytes_per_codeword-128)>>2));
  else if (bytes_per_codeword <= 768)
    return (123 + ((bytes_per_codeword-256)>>3));
  else {
#ifdef DEBUG_TURBO_ENCODER
    printf("Illegal codeword size !!!\n");
#endif
    return(-1);
  }
}
