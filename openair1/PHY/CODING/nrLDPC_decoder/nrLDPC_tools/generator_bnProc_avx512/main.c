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
#include <stdint.h>
#include <stdlib.h>
#define NB_R  3
void nrLDPC_bnProc_BG1_generator_AVX512(const char *, int);
void nrLDPC_bnProc_BG2_generator_AVX512(const char *, int);
void nrLDPC_bnProcPc_BG1_generator_AVX512(const char *, int);
void nrLDPC_bnProcPc_BG2_generator_AVX512(const char *, int);

const char *__asan_default_options()
{
  /* don't do leak checking in nr_ulsim, creates problems in the CI */
  return "detect_leaks=0";
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <output-dir>\n", argv[0]);
    return 1;
  }
  const char *dir = argv[1];

  int R[NB_R]={0,1,2};
  for(int i=0; i<NB_R;i++){
    nrLDPC_bnProc_BG1_generator_AVX512(dir, R[i]);
    nrLDPC_bnProc_BG2_generator_AVX512(dir, R[i]);

    nrLDPC_bnProcPc_BG1_generator_AVX512(dir, R[i]);
    nrLDPC_bnProcPc_BG2_generator_AVX512(dir, R[i]);
  }

  return(0);
}

