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

typedef struct {
  uint8_t  kasme[32];
  uint8_t  kenb_exp[16];
  uint32_t nas_count;
} test_secu_t;

const test_secu_t kenb_test_vector[] = {
  {
    .kasme = {

    },
    .kenb_exp = {

    },
    .nas_count = 0x001FB39C;
  }
};

void doit (void)
{
  int i;

  for (i = 0; i < sizeof(kenb_test_vector) / sizeof(test_secu_t); i++) {
    uint8_t kenb[32];

    derive_keNB(kenb_test_vector[i].kasme, kenb_test_vector[i].nas_count, kenb);

    if (compare_buffer(kenb_test_vector[i].kenb, 32, kenb, 32) == 0) {
      success("kenb derivation");
    } else {
      fail("kenb derivation");
    }
  }
}
