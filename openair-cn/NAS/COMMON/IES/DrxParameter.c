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

