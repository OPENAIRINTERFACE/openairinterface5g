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
#include "PlmnList.h"

int decode_plmn_list(PlmnList *plmnlist, uint8_t iei, uint8_t *buffer, uint32_t len)
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
  plmnlist->mccdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  plmnlist->mccdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  plmnlist->mncdigit3 = (*(buffer + decoded) >> 4) & 0xf;
  plmnlist->mccdigit3 = *(buffer + decoded) & 0xf;
  decoded++;
  plmnlist->mncdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  plmnlist->mncdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
#if defined (NAS_DEBUG)
  dump_plmn_list_xml(plmnlist, iei);
#endif
  return decoded;
}
int encode_plmn_list(PlmnList *plmnlist, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, PLMN_LIST_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_plmn_list_xml(plmnlist, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;
  *(buffer + encoded) = 0x00 | ((plmnlist->mccdigit2 & 0xf) << 4) |
                        (plmnlist->mccdigit1 & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((plmnlist->mncdigit3 & 0xf) << 4) |
                        (plmnlist->mccdigit3 & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((plmnlist->mncdigit2 & 0xf) << 4) |
                        (plmnlist->mncdigit1 & 0xf);
  encoded++;
  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void dump_plmn_list_xml(PlmnList *plmnlist, uint8_t iei)
{
  printf("<Plmn List>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <MCC digit 2>%u</MCC digit 2>\n", plmnlist->mccdigit2);
  printf("    <MCC digit 1>%u</MCC digit 1>\n", plmnlist->mccdigit1);
  printf("    <MNC digit 3>%u</MNC digit 3>\n", plmnlist->mncdigit3);
  printf("    <MCC digit 3>%u</MCC digit 3>\n", plmnlist->mccdigit3);
  printf("    <MNC digit 2>%u</MNC digit 2>\n", plmnlist->mncdigit2);
  printf("    <MNC digit 1>%u</MNC digit 1>\n", plmnlist->mncdigit1);
  printf("</Plmn List>\n");
}

