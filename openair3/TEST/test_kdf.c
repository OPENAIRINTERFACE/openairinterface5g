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
#include <unistd.h>
#include <string.h>

#include "test_util.h"
#include "secu_defs.h"

static
void do_kdf(uint8_t *key, unsigned key_length, uint8_t *data, unsigned data_length,
            uint8_t *exp, unsigned exp_length)
{
  uint8_t result[32];

  kdf(key, key_length, data, data_length, result, 32);

  if (compare_buffer(result, exp_length, exp, exp_length) != 0) {
    fail("Fail: kdf\n");
  }
}

void
doit (void)
{
  /* RFC 4231 */

  /* Test case 1 #4.2 */
  do_kdf(HL("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b"
            "0b0b0b0b"),
         HL("4869205468657265"),
         HL("b0344c61d8db38535ca8afceaf0bf12b"
            "881dc200c9833da726e9376c2e32cff7"));
  /* Test case 2 #4.3 */
  do_kdf(HL("4a656665"),
         HL("7768617420646f2079612077616e7420"
            "666f72206e6f7468696e673f"),
         HL("5bdcc146bf60754e6a042426089575c7"
            "5a003f089d2739839dec58b964ec3843"));
  /* Test case 3 #4.4 */
  do_kdf(HL("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaa"),
         HL("dddddddddddddddddddddddddddddddd"
            "dddddddddddddddddddddddddddddddd"
            "dddddddddddddddddddddddddddddddd"
            "dddd"),
         HL("773ea91e36800e46854db8ebd09181a7"
            "2959098b3ef8c122d9635514ced565fe"));
  /* Test case 4 #4.5 */
  do_kdf(HL("0102030405060708090a0b0c0d0e0f10"
            "111213141516171819"),
         HL("cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
            "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
            "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
            "cdcd"),
         HL("82558a389a443c0ea4cc819899f2083a"
            "85f0faa3e578f8077a2e3ff46729665b"));
  /* Test case 5 #4.6 */
  do_kdf(HL("0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c"
            "0c0c0c0c"),
         HL("546573742057697468205472756e6361"
            "74696f6e"),
         HL("a3b6167473100ee06e0c796c2955552b"));
  /* Test case 6 #4.7 */
  do_kdf(HL("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaa"),
         HL("54657374205573696e67204c61726765"
            "72205468616e20426c6f636b2d53697a"
            "65204b6579202d2048617368204b6579"
            "204669727374"),
         HL("60e431591ee0b67f0d8a26aacbf5b77f"
            "8e0bc6213728c5140546040f0ee37f54"));
  /* Test case 6 #4.7 */
  do_kdf(HL("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaa"),
         HL("54686973206973206120746573742075"
            "73696e672061206c6172676572207468"
            "616e20626c6f636b2d73697a65206b65"
            "7920616e642061206c61726765722074"
            "68616e20626c6f636b2d73697a652064"
            "6174612e20546865206b6579206e6565"
            "647320746f2062652068617368656420"
            "6265666f7265206265696e6720757365"
            "642062792074686520484d414320616c"
            "676f726974686d2e"),
         HL("9b09ffa71b942fcb27635fbcd5b0e944"
            "bfdc63644f0713938a7f51535c3a35e2"));
}
