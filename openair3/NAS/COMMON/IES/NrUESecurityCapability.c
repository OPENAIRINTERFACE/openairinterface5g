/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file NrUESecurityCapability.c
 * \brief 5GS UE Security Capability for registration request procedures
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 * \date 2020
 * \version 0.1
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "NrUESecurityCapability.h"



int encode_nrue_security_capability(NrUESecurityCapability *nruesecuritycapability, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  if(iei){
    *buffer = nruesecuritycapability->iei;
     encoded++;
     *(buffer+encoded) = nruesecuritycapability->length;
     encoded++;
     *(buffer+encoded) = nruesecuritycapability->fg_EA;
     encoded++;
     *(buffer+encoded) = nruesecuritycapability->fg_IA;
     encoded++;
     *(buffer+encoded) = nruesecuritycapability->EEA;
     encoded++;
     *(buffer+encoded) = nruesecuritycapability->EIA;
     encoded++;
     if((nruesecuritycapability->length - 4) > 0)
       encoded = encoded + nruesecuritycapability->length - 4;
  }
  return encoded;
}

