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
#include "TrackingAreaIdentity.h"

int decode_tracking_area_identity(TrackingAreaIdentity *trackingareaidentity, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  trackingareaidentity->mccdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  trackingareaidentity->mccdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  trackingareaidentity->mncdigit3 = (*(buffer + decoded) >> 4) & 0xf;
  trackingareaidentity->mccdigit3 = *(buffer + decoded) & 0xf;
  decoded++;
  trackingareaidentity->mncdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  trackingareaidentity->mncdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  //IES_DECODE_U16(trackingareaidentity->tac, *(buffer + decoded));
  IES_DECODE_U16(buffer, decoded, trackingareaidentity->tac);
#if defined (NAS_DEBUG)
  dump_tracking_area_identity_xml(trackingareaidentity, iei);
#endif
  return decoded;
}

int encode_tracking_area_identity(TrackingAreaIdentity *trackingareaidentity, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, TRACKING_AREA_IDENTITY_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_tracking_area_identity_xml(trackingareaidentity, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = 0x00 | ((trackingareaidentity->mccdigit2 & 0xf) << 4) |
                        (trackingareaidentity->mccdigit1 & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((trackingareaidentity->mncdigit3 & 0xf) << 4) |
                        (trackingareaidentity->mccdigit3 & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((trackingareaidentity->mncdigit2 & 0xf) << 4) |
                        (trackingareaidentity->mncdigit1 & 0xf);
  encoded++;
  IES_ENCODE_U16(buffer, encoded, trackingareaidentity->tac);
  return encoded;
}

void dump_tracking_area_identity_xml(TrackingAreaIdentity *trackingareaidentity, uint8_t iei)
{
  printf("<Tracking Area Identity>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <MCC digit 2>%u</MCC digit 2>\n", trackingareaidentity->mccdigit2);
  printf("    <MCC digit 1>%u</MCC digit 1>\n", trackingareaidentity->mccdigit1);
  printf("    <MNC digit 3>%u</MNC digit 3>\n", trackingareaidentity->mncdigit3);
  printf("    <MCC digit 3>%u</MCC digit 3>\n", trackingareaidentity->mccdigit3);
  printf("    <MNC digit 2>%u</MNC digit 2>\n", trackingareaidentity->mncdigit2);
  printf("    <MNC digit 1>%u</MNC digit 1>\n", trackingareaidentity->mncdigit1);
  printf("    <TAC>0x%.4x</TAC>\n", trackingareaidentity->tac);
  printf("</Tracking Area Identity>\n");
}

