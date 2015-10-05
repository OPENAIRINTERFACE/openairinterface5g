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
#include "GprsTimer.h"

static const long _gprs_timer_unit[] = {2, 60, 360, 60, 60, 60, 60, 0};

int decode_gprs_timer(GprsTimer *gprstimer, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  gprstimer->unit = (*(buffer + decoded) >> 5) & 0x7;
  gprstimer->timervalue = *(buffer + decoded) & 0x1f;
  decoded++;
#if defined (NAS_DEBUG)
  dump_gprs_timer_xml(gprstimer, iei);
#endif
  return decoded;
}

int encode_gprs_timer(GprsTimer *gprstimer, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, GPRS_TIMER_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_gprs_timer_xml(gprstimer, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = 0x00 | ((gprstimer->unit & 0x7) << 5) |
                        (gprstimer->timervalue & 0x1f);
  encoded++;
  return encoded;
}

void dump_gprs_timer_xml(GprsTimer *gprstimer, uint8_t iei)
{
  printf("<Gprs Timer>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Unit>%u</Unit>\n", gprstimer->unit);
  printf("    <Timer value>%u</Timer value>\n", gprstimer->timervalue);
  printf("</Gprs Timer>\n");
}

long gprs_timer_value(GprsTimer *gprstimer)
{
  return (gprstimer->timervalue * _gprs_timer_unit[gprstimer->unit]);
}

