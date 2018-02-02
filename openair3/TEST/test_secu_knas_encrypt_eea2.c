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
void eea2_encrypt(uint8_t direction, uint32_t count,
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

  if (nas_stream_encrypt_eea2(nas_cipher, result) != 0)
    fail("Fail: nas_stream_encrypt_eea2\n");

  if (compare_buffer(result, byte_length, expected, byte_length) != 0) {
    fail("Fail: eea2_encrypt\n");
  }

  free(nas_cipher);
  free(result);
}


void doit (void)
{
  /* Test suite from 3GPP TS33.401 #Appendix C.1 */

  /* Test set 1 #C.1.1 */
  eea2_encrypt(
    1, 0x398a59b4, 0x15, HL("d3c5d592327fb11c4035c6680af8c6d1"),
    H("981ba6824c1bfb1ab485472029b71d808ce33e2cc3c0b5fc1f3de8a6dc66b1f0"),
    253,
    H("e9fed8a63d155304d71df20bf3e82214b20ed7dad2f233dc3c22d7bdeeed8e78")
  );
  /* Test set 2 #C.1.2 */
  eea2_encrypt(
    1, 0xc675a64b, 0x0c, HL("2bd6459f82c440e0952c49104805ff48"),
    H("7ec61272743bf1614726446a6c38ced166f6ca76eb5430044286346cef130f92"
      "922b03450d3a9975e5bd2ea0eb55ad8e1b199e3ec4316020e9a1b285e7627953"
      "59b7bdfd39bef4b2484583d5afe082aee638bf5fd5a606193901a08f4ab41aab"
      "9b134880"),
    798,
    H("5961605353c64bdca15b195e288553a910632506d6200aa790c4c806c99904cf"
      "2445cc50bb1cf168a49673734e081b57e324ce5259c0e78d4cd97b870976503c"
      "0943f2cb5ae8f052c7b7d392239587b8956086bcab18836042e2e6ce42432a17"
      "105c53d0")
  );
  /* Test set 3 #C.1.3 */
  eea2_encrypt(
    0, 0x544d49cd, 0x04, HL("0a8b6bd8d9b08b08d64e32d1817777fb"),
    H("fd40a41d370a1f65745095687d47ba1d36d2349e23f644392c8ea9c49d40c132"
      "71aff264d0f24800"),
    310,
    H("75750d37b4bba2a4dedb34235bd68c6645acdaaca48138a3b0c471e2a7041a57"
      "6423d2927287f000")
  );
  /* Test set 4 #C.1.4 */
  eea2_encrypt(
    1, 0x72d8c671, 0x10, HL("aa1f95aea533bcb32eb63bf52d8f831a"),
    H("fb1b96c5c8badfb2e8e8edfde78e57f2ad81e74103fc430a534dcc37afcec70e"
      "1517bb06f27219dae49022ddc47a068de4c9496a951a6b09edbdc864c7adbd74"
      "0ac50c022f3082bafd22d78197c5d508b977bca13f32e652e74ba728576077ce"
      "628c535e87dc6077ba07d29068590c8cb5f1088e082cfa0ec961302d69cf3d44"),
    1022,
    H("dfb440acb3773549efc04628aeb8d8156275230bdc690d94b00d8d95f28c4b56"
      "307f60f4ca55eba661ebba72ac808fa8c49e26788ed04a5d606cb418de74878b"
      "9a22f8ef29590bc4eb57c9faf7c41524a885b8979c423f2f8f8e0592a9879201"
      "be7ff9777a162ab810feb324ba74c4c156e04d39097209653ac33e5a5f2d8864")
  );
  /* Test set 5 #C.1.5 */
  eea2_encrypt(
    1, 0xc675a64b, 0x0c, HL("9618ae46891f86578eebe90ef7a1202e"),
    H("8daa17b1ae050529c6827f28c0ef6a1242e93f8b314fb18a77f790ae049fedd6"
      "12267fecaefc450174d76d9f9aa7755a30cd90a9a5874bf48eaf70eea3a62a25"
      "0a8b6bd8d9b08b08d64e32d1817777fb544d49cd49720e219dbf8bbed33904e1"
      "fd40a41d370a1f65745095687d47ba1d36d2349e23f644392c8ea9c49d40c132"
      "71aff264d0f24841d6465f0996ff84e65fc517c53efc3363c38492a8"),
    1245,
    H("919c8c33d66789703d05a0d7ce82a2aeac4ee76c0f4da050335e8a84e7897ba5"
      "df2f36bd513e3d0c8578c7a0fcf043e03aa3a39fbaad7d15be074faa5d9029f7"
      "1fb457b647834714b0e18f117fca10677945096c8c5f326ba8d6095eb29c3e36"
      "cf245d1622aafe921f7566c4f5d644f2f1fc0ec684ddb21349747622e209295d"
      "27ff3f95623371d49b147c0af486171f22cd04b1cbeb2658223e6938")
  );
  /* Test set 6 #C.1.6 */
  eea2_encrypt(
    0, 0xaca4f50f, 0x0b, HL("54f4e2e04c83786eec8fb5abe8e36566"),
    H("40981ba6824c1bfb4286b299783daf442c099f7ab0f58d5c8e46b104f08f01b4"
      "1ab485472029b71d36bd1a3d90dc3a41b46d51672ac4c9663a2be063da4bc8d2"
      "808ce33e2cccbfc634e1b259060876a0fbb5a437ebcc8d31c19e4454318745e3"
      "fa16bb11adae248879fe52db2543e53cf445d3d828ce0bf5c560593d97278a59"
      "762dd0c2c9cd68d4496a792508614014b13b6aa51128c18cd6a90b87978c2ff1"
      "cabe7d9f898a411bfdb84f68f6727b1499cdd30df0443ab4a66653330bcba110"
      "5e4cec034c73e605b4310eaaadcfd5b0ca27ffd89d144df4792759427c9cc1f8"
      "cd8c87202364b8a687954cb05a8d4e2d99e73db160deb180ad0841e96741a5d5"
      "9fe4189f15420026fe4cd12104932fb38f735340438aaf7eca6fd5cfd3a195ce"
      "5abe65272af607ada1be65a6b4c9c0693234092c4d018f1756c6db9dc8a6d80b"
      "888138616b681262f954d0e7711748780d92291d86299972db741cfa4f37b8b5"
      "6cdb18a7ca8218e86e4b4b716a4d04371fbec262fc5ad0b3819b187b97e55b1a"
      "4d7c19ee24c8b4d7723cfedf045b8acae4869517d80e50615d9035d5d9c5a40a"
      "f602280b542597b0cb18619eeb35925759d195e100e8e4aa0c38a3c2abe0f3d8"
      "ff04f3c33c295069c23694b5bbeacdd542e28e8a94edb9119f412d054be1fa72"
      "00b09000"),
    3861,
    H("5cb72c6edc878f1566e10253afc364c9fa540d914db94cbee275d0917ca6af0d"
      "77acb4ef3bbe1a722b2ef5bd1d4b8e2aa5024ec1388a201e7bce7920aec61589"
      "5f763a5564dcc4c482a2ee1d8bfecc4498eca83fbb75f9ab530e0dafbede2fa5"
      "895b82991b6277c529e0f2529d7f79606be96706296dedfa9d7412b616958cb5"
      "63c678c02825c30d0aee77c4c146d2765412421a808d13cec819694c75ad572e"
      "9b973d948b81a9337c3b2a17192e22c2069f7ed1162af44cdea817603665e807"
      "ce40c8e0dd9d6394dc6e31153fe1955c47afb51f2617ee0c5e3b8ef1ad7574ed"
      "343edc2743cc94c990e1f1fd264253c178dea739c0befeebcd9f9b76d49c1015"
      "c9fecf50e53b8b5204dbcd3eed863855dabcdcc94b31e318021568855c8b9e52"
      "a981957a112827f978ba960f1447911b317b5511fbcc7fb13ac153db74251117"
      "e4861eb9e83bffffc4eb7755579038e57924b1f78b3e1ad90bab2a07871b72db"
      "5eef96c334044966db0c37cafd1a89e5646a3580eb6465f121dce9cb88d85b96"
      "cf23ccccd4280767bee8eeb23d8652461db6493103003baf89f5e18261ea43c8"
      "4a92ebffffe4909dc46c5192f825f770600b9602c557b5f8b431a79d45977dd9"
      "c41b863da9e142e90020cfd074d6927b7ab3b6725d1a6f3f98b9c9daa8982aff"
      "06782800")
  );
}
