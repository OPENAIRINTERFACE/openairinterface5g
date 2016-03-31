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
#include "MsNetworkFeatureSupport.h"

int decode_ms_network_feature_support(MsNetworkFeatureSupport *msnetworkfeaturesupport, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
 
  
  if (iei > 0) {
    CHECK_IEI_DECODER(iei, (*buffer & 0xc0));
  }
  msnetworkfeaturesupport->spare_bits= (*(buffer + decoded) >> 3) & 0x7;
  msnetworkfeaturesupport->extended_periodic_timers= *(buffer + decoded) & 0x1;
  decoded++;
  
#if defined (NAS_DEBUG)
  dump_ms_network_feature_support_xml(msnetworkfeaturesupport, iei);
#endif
  return decoded;
}
int encode_ms_network_feature_support(MsNetworkFeatureSupport *msnetworkfeaturesupport, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  //uint8_t *lenPtr;
  uint32_t encoded = 0;
  //int encode_result;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, MS_NETWORK_FEATURE_SUPPORT_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_ms_network_feature_support_xml(msnetworkfeaturesupport, iei);
#endif

  
  *(buffer + encoded) = 0x00 | ((msnetworkfeaturesupport->spare_bits & 0x7) << 3)  
                             | (msnetworkfeaturesupport->extended_periodic_timers & 0x1);
  encoded++;
  return encoded;
}

void dump_ms_network_feature_support_xml(MsNetworkFeatureSupport *msnetworkfeaturesupport, uint8_t iei)
{
  printf("<Ms Network Feature Support>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);
  
  printf("    <spare_bits>%u<spare_bits>\n",msnetworkfeaturesupport->spare_bits);
  printf("    <extended_periodic_timer>%u<extended_periodic_timer>\n",msnetworkfeaturesupport->extended_periodic_timers);
  printf("</Ms Network Feature Support>\n");
}

