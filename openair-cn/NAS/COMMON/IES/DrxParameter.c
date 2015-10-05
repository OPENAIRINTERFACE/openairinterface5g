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


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "DrxParameter.h"

int decode_drx_parameter(DrxParameter *drxparameter, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  drxparameter->splitpgcyclecode = *(buffer + decoded);
  decoded++;
  drxparameter->cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode = (*(buffer + decoded) >> 4) & 0xf;
  drxparameter->splitonccch = (*(buffer + decoded) >> 3) & 0x1;
  drxparameter->nondrxtimer = *(buffer + decoded) & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_drx_parameter_xml(drxparameter, iei);
#endif
  return decoded;
}

int encode_drx_parameter(DrxParameter *drxparameter, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, DRX_PARAMETER_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_drx_parameter_xml(drxparameter, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = drxparameter->splitpgcyclecode;
  encoded++;
  *(buffer + encoded) = 0x00 | ((drxparameter->cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode & 0xf) << 4) |
                        ((drxparameter->splitonccch & 0x1) << 3) |
                        (drxparameter->nondrxtimer & 0x7);
  encoded++;
  return encoded;
}

void dump_drx_parameter_xml(DrxParameter *drxparameter, uint8_t iei)
{
  printf("<Drx Parameter>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <SPLIT PG CYCLE CODE>%u</SPLIT PG CYCLE CODE>\n", drxparameter->splitpgcyclecode);
  printf("    <CN specific DRX cycle length coefficient and DRX value for S1 mode>%u</CN specific DRX cycle length coefficient and DRX value for S1 mode>\n",
         drxparameter->cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode);
  printf("    <SPLIT on CCCH>%u</SPLIT on CCCH>\n", drxparameter->splitonccch);
  printf("    <non DRX timer>%u</non DRX timer>\n", drxparameter->nondrxtimer);
  printf("</Drx Parameter>\n");
}

