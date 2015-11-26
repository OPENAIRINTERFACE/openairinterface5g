/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
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

