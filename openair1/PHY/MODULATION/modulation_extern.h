/*
   Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
   contributor license agreements.  See the NOTICE file distributed with
   this work for additional information regarding copyright ownership.
   The OpenAirInterface Software Alliance licenses this file to You under
   the OAI Public License, Version 1.1  (the "License"); you may not use this file
   except in compliance with the License.
   You may obtain a copy of the License at

        http://www.openairinterface.org/?page_id=698

   Unless required by applicable law or agreed to in writing, software
  ; * distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
  -------------------------------------------------------------------------------
   For more information about the OpenAirInterface (OAI) Software Alliance:
        contact@openairinterface.org
*/
#ifdef __cplusplus
extern "C" {
#endif

extern int16_t *s6n_kHz_7_5;
extern int16_t *s6e_kHz_7_5;
extern int16_t *s15n_kHz_7_5;
extern int16_t *s15e_kHz_7_5;
extern int16_t *s25n_kHz_7_5;
extern int16_t *s25e_kHz_7_5;
extern int16_t *s50n_kHz_7_5;
extern int16_t *s50e_kHz_7_5;
extern int16_t *s75n_kHz_7_5;
extern int16_t *s75e_kHz_7_5;
extern int16_t *s100n_kHz_7_5;
extern int16_t *s100e_kHz_7_5;

static const short conjugate75[8] __attribute__((aligned(16))) = {-1, 1, -1, 1, -1, 1, -1, 1};
static const short conjugate75_2[8] __attribute__((aligned(16))) = {1, -1, 1, -1, 1, -1, 1, -1};
static const short negate[8] __attribute__((aligned(16))) = {-1, -1, -1, -1, -1, -1, -1, -1};

#ifdef __cplusplus
}
#endif
