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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "test_util.h"

#include "secu_defs.h"

static
void derive_knas_keys(algorithm_type_dist_t atd, uint8_t *kasme, unsigned length,
                      uint8_t *knas_enc_exp, uint8_t *knas_int_exp)
{
  uint8_t *knas_enc = NULL;
  uint8_t *knas_int = NULL;

  knas_enc = calloc(1, 32);
  knas_int = calloc(1, 32);

  derive_key_nas_enc(atd, kasme, knas_enc);
  derive_key_nas_int(atd, kasme, knas_int);

  /* Compare both keys with expected */
  if (compare_buffer(knas_enc, 16, &knas_enc_exp[16], 16) != 0) {
    fail("Fail: knas_enc derivation\n");
  }

  if (compare_buffer(knas_int, 16, &knas_int_exp[16], 16) != 0) {
    fail("Fail: knas_int derivation\n");
  }
  free(knas_enc);
  free(knas_int);
}

void doit(void)
{
  derive_knas_keys(EIA0_ALG_ID,
                   HL("70D7071AA016A087F9D888AD51F3A83E2C83443AB27843B35BD1B4923615091C"),
                   H("64BA839B29C451085C552F4DE9D278B263CB5BDD7FC21A38120637B2A9E5CD39"),
                   H("A66A2D198AF2A8D6A5FF2FAA51676037DF204187C61EDD3AAA70F3B7D8B59E8B"));
  derive_knas_keys(EIA1_128_ALG_ID,
                   HL("70D7071AA016A087F9D888AD51F3A83E2C83443AB27843B35BD1B4923615091C"),
                   H("DF8FEBA477891617C42FB16F750E572C9E59ED7564879150F6BB0DAEF5932E89"),
                   H("EA1158BA3F387BC96C967BC32FB43F65AE172A3267343479CAA826034A90A250"));
  derive_knas_keys(EIA2_128_ALG_ID,
                   HL("70D7071AA016A087F9D888AD51F3A83E2C83443AB27843B35BD1B4923615091C"),
                   H("99D63BD2D43AF81EBB7599F7E8F8B3E81CF7897F31D6270C19C4836070FE11F0"),
                   H("FC7A3850D19AE29EC7000B9DF98787F11A4329FD23FD3A93C9D2D92D853EC9DB"));
  derive_knas_keys(EIA0_ALG_ID,
                   HL("9EA141DA4B24CDEBC8F5FB3F61A0511216681F121199B23EBCFACC75B358BE43"),
                   H("1E48E1B5EDF98DEDF339DE686544AA1088C8E5616EDB706201837AA106D37691"),
                   H("C83DC420F97AA42D1B8488FA5D8F74865D833416D5851556100B41FEC8E38139"));
  derive_knas_keys(EIA1_128_ALG_ID,
                   HL("9EA141DA4B24CDEBC8F5FB3F61A0511216681F121199B23EBCFACC75B358BE43"),
                   H("207700CD92B4635B439E40176F92F7ADA824B9D699ABE15F86F3346C25343A33"),
                   H("FAA39E382611CDFED52042E72AF8CECDF92CCD799141857B77B6901741E486B2"));
  derive_knas_keys(EIA2_128_ALG_ID,
                   HL("9EA141DA4B24CDEBC8F5FB3F61A0511216681F121199B23EBCFACC75B358BE43"),
                   H("2A6854D25282FFD738FA8BBCFDCE853C0C4DFB9F559DCBB71D5120DB2CAC66A6"),
                   H("5EDCAE62A35BC42399C55F64ECAE7B17524BED1ED1601218D2772E55DDFAD959"));
  derive_knas_keys(EIA0_ALG_ID,
                   HL("546A79BC6D1613A72A4D631EE0351D66036B2A0C44A3831BE6D365E24F023013"),
                   H("7F684F909A5910DED6E4B4CE02AC92DF4690EB3788068600E9CB2C3EF8F0956E"),
                   H("51F3B142D357371726873FAC5E853CE0EEEFC52A695DE45FA6F4A23D20C13155"));
  derive_knas_keys(EIA1_128_ALG_ID,
                   HL("546A79BC6D1613A72A4D631EE0351D66036B2A0C44A3831BE6D365E24F023013"),
                   H("B6B222E9C07C56D15176C8E95EC1805561492682E4B9CD7228237A6C78702347"),
                   H("4DC5ABF6896DA460473288D16DE83C480388C6FD9E803C7EA88090F311354A15"));
  derive_knas_keys(EIA2_128_ALG_ID,
                   HL("546A79BC6D1613A72A4D631EE0351D66036B2A0C44A3831BE6D365E24F023013"),
                   H("7BD2A0864452D3D9EDE3B2E38A9958777F231AC4F4B9AF3FC8CE0E1EAAB425A7"),
                   H("89E8049EBE9374F238B0F3278EEA47D9BF8877111D99D6378FFF66FEBB06F3F8"));
}
