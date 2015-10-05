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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EPS_ATTACH_RESULT_H_
#define EPS_ATTACH_RESULT_H_

#define EPS_ATTACH_RESULT_MINIMUM_LENGTH 1
#define EPS_ATTACH_RESULT_MAXIMUM_LENGTH 1

#define EPS_ATTACH_RESULT_EPS   0b001
#define EPS_ATTACH_RESULT_EPS_IMSI  0b010
typedef uint8_t EpsAttachResult;

int encode_eps_attach_result(EpsAttachResult *epsattachresult, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_attach_result_xml(EpsAttachResult *epsattachresult, uint8_t iei);

uint8_t encode_u8_eps_attach_result(EpsAttachResult *epsattachresult);

int decode_eps_attach_result(EpsAttachResult *epsattachresult, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_eps_attach_result(EpsAttachResult *epsattachresult, uint8_t iei, uint8_t value, uint32_t len);

#endif /* EPS ATTACH RESULT_H_ */

