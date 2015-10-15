/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "test_util.h"

#include "secu_defs.h"

static
void eea1_encrypt(uint8_t direction, uint32_t count,
                  uint8_t bearer, uint8_t *key, uint32_t key_length, uint8_t *message,
                  uint32_t length, uint8_t *expected)
{
  nas_stream_cipher_t *nas_cipher  = NULL;
  uint8_t             *result      = NULL;
  uint32_t             zero_bits   = length & 7;
  uint32_t             byte_length = length >> 3;

  if (zero_bits > 0)
    byte_length += 1;

  result     = calloc(1, byte_length);
  nas_cipher = calloc(1, sizeof(nas_stream_cipher_t));

  nas_cipher->direction  = direction;
  nas_cipher->count      = count;
  nas_cipher->key        = key;
  nas_cipher->key_length = key_length;
  nas_cipher->bearer     = bearer;
  nas_cipher->blength    = length;
  nas_cipher->message    = message;

  if (nas_stream_encrypt_eea1(nas_cipher, result) != 0)
    fail("Fail: nas_stream_encrypt_eea1\n");

  if (compare_buffer(result, byte_length, expected, byte_length) != 0) {
    fail("Fail: eea1_encrypt\n");
  } else {
    success("Success: eea1_encrypt\n");
  }

  free(nas_cipher);
  free(result);
}


void doit (void)
{
  /* Test suite from Specification of the 3GPP Confidentiality and Integrity Algorithms UEA2 & UIA2,
   * Document 3: Implementors’ Test Data */

  /* Test set 1 #4.3 */
  eea1_encrypt(
    1, 0x72A4F20F, 0x0C, HL("2BD6459F82C5B300952C49104881FF48"),
    H("7EC61272743BF1614726446A6C38CED166F6CA76EB5430044286346CEF130F92922B03450D3A9975E5BD2EA0EB55AD8E1B199E3EC4316020E9A1B285E762795359B7BDFD39BEF4B2484583D5AFE082AEE638BF5FD5A606193901A08F4AB41AAB9B134880"),
    798,
    H("8CEBA62943DCED3A0990B06EA1B0A2C4FB3CEDC71B369F42BA64C1EB6665E72AA1C9BB0DEAA20FE86058B8BAEE2C2E7F0BECCE48B52932A53C9D5F931A3A7C532259AF4325E2A65E3084AD5F6A513B7BDDC1B65F0AA0D97A053DB55A88C4C4F9605E4140")
  );
  /* Test set 2 #4.4 */
  eea1_encrypt(
    0, 0xE28BCF7B, 0x18, HL("EFA8B2229E720C2A7C36EA55E9605695"),
    H("10111231E060253A43FD3F57E37607AB2827B599B6B1BBDA37A8ABCC5A8C550D1BFB2F494624FB50367FA36CE3BC68F11CF93B1510376B02130F812A9FA169D8"),
    510,
    H("E0DA15CA8E2554F5E56C9468DC6C7C129C568AA5032317E04E0729646CABEFA689864C410F24F919E61E3DFDFAD77E560DB0A9CD36C34AE4181490B29F5FA2FC"));
  /* Test set 3 #4.5 */
  eea1_encrypt(
    1, 0xFA556B26, 0x03, HL("5ACB1D644C0D51204EA5F1451010D852"),
    H("AD9C441F890B38C457A49D421407E8"),
    120,
    H("BA0F31300334C56B52A7497CBAC046")
  );
  /* Test set 4 #4.6 */
  eea1_encrypt(
    1, 0x398A59B4, 0x05, HL("D3C5D592327FB11C4035C6680AF8C6D1"),
    H("981BA6824C1BFB1AB485472029B71D808CE33E2CC3C0B5FC1F3DE8A6DC66B1F0"),
    253,
    H("989B719CDC33CEB7CF276A52827CEF94A56C40C0AB9D81F7A2A9BAC60E11C4B0")
  );
  /* Test set 5 #4.7 */
  eea1_encrypt(
    0, 0x72A4F20F, 0x09, HL("6090EAE04C83706EECBF652BE8E36566"),
    H("40981BA6824C1BFB4286B299783DAF442C099F7AB0F58D5C8E46B104F08F01B41AB485472029B71D36BD1A3D90DC3A41B46D51672AC4C9663A2BE063DA4BC8D2808CE33E2CCCBFC634E1B259060876A0FBB5A437EBCC8D31C19E4454318745E3987645987A986F2CB0"),
    837,
    H("5892BBA88BBBCAAEAE769AA06B683D3A17CC04A369881697435E44FED5FF9AF57B9E890D4D5C64709885D48AE40690EC043BAAE9705796E4A9FF5A4B8D8B36D7F3FE57CC6CFD6CD005CD3852A85E94CE6BCD90D0D07839CE09733544CA8E350843248550922AC12818")
  );
}
