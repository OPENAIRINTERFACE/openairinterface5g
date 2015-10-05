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
#include "EmergencyNumberList.h"

int decode_emergency_number_list(EmergencyNumberList *emergencynumberlist, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  uint8_t ielen = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER(len - decoded, ielen);
  emergencynumberlist->lengthofemergency = *(buffer + decoded);
  decoded++;
  emergencynumberlist->emergencyservicecategoryvalue = *(buffer + decoded) & 0x1f;
  decoded++;
#if defined (NAS_DEBUG)
  dump_emergency_number_list_xml(emergencynumberlist, iei);
#endif
  return decoded;
}
int encode_emergency_number_list(EmergencyNumberList *emergencynumberlist, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, EMERGENCY_NUMBER_LIST_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_emergency_number_list_xml(emergencynumberlist, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;
  *(buffer + encoded) = emergencynumberlist->lengthofemergency;
  encoded++;
  *(buffer + encoded) = 0x00 |
                        (emergencynumberlist->emergencyservicecategoryvalue & 0x1f);
  encoded++;
  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void dump_emergency_number_list_xml(EmergencyNumberList *emergencynumberlist, uint8_t iei)
{
  printf("<Emergency Number List>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Length of emergency>%u</Length of emergency>\n", emergencynumberlist->lengthofemergency);
  printf("    <Emergency service category value>%u</Emergency service category value>\n", emergencynumberlist->emergencyservicecategoryvalue);
  printf("</Emergency Number List>\n");
}

