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
void do_derive_kenb(uint32_t nas_count, const uint8_t *kasme, const unsigned length,
                    const uint8_t *kenb_exp)
{
  uint8_t kenb[32];

  memset(kenb, 0, sizeof(kenb));
  derive_keNB(kasme, nas_count, kenb);

  if (compare_buffer(kenb_exp, length, kenb, length) != 0) {
    fail("Fail: kenb derivation\n");
  }

}

void doit (void)
{
  do_derive_kenb(0xDB1A3569,
                 HL("238E457E0F758BADBCA8D34BB2612C10428D426757CB5553B2B184FA64BFC549"),
                 H("8EB1BF0083BD79281EF7034BF677E9EC529F196E15287514A2D122ACF713B8E8"));
  do_derive_kenb(0x001FB39C,
                 HL("564CB4D2007E4F293B67D9B29392A64ADD4C776B133D895AF6499AA6882AAB62"),
                 H("009010688F85855E218339DE6C5BD7B6394958DA12DDFBF7559E978CE43408F1"));
  do_derive_kenb(0x00000012,
                 HL("FA77E41F693C2A6E71455CB8687E6E6058EF91E2F5ABD1BD3C496179481F308C"),
                 H("0AFE81266DE52B8C8F1C3F4FE799BE883F364B018E7368C41F14DD6D050E13E1"));
  do_derive_kenb(0xFE56A1D3,
                 HL("FA77E41F693C2A6E71455CB8687E6E6058EF91E2F5ABD1BD3C496179481F308C"),
                 H("3622874B06C683586A009DEC81DBE28BDD8E3E6E67A2C22C31630EC97641828E"));
  do_derive_kenb(0xFE56A1D3,
                 HL("70D7071AA016A087F9D888AD51F3A83E2C83443AB27843B35BD1B4923615091C"),
                 H("E59BE6F0FBEFA1207DA3FF05D0F82014100E7A63A11EEBFE4F8AA92E7CF8B847"));
  do_derive_kenb(0xAABBCCDD,
                 HL("70D7071AA016A087F9D888AD51F3A83E2C83443AB27843B35BD1B4923615091C"),
                 H("158781A2FDF2CE53ABFA186D22EB751DECB8273471DC792B5016C9016947D1AE"));
}
