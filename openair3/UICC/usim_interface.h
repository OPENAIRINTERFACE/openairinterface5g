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
* Author and copyright: Laurent Thomas, open-cells.comopenair3/UICC/usim_interface.h
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
#ifndef USIM_INTERFACE_H
#define USIM_INTERFACE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <common/utils/assertions.h>
#include <common/utils/LOG/log.h>
#include <common/utils/load_module_shlib.h>
#include <common/config/config_userapi.h>
#include "common_lib.h"

/* 3GPP glossary
RES	RESponse
XRES	eXpected RESponse
HRES	Hash RESponse
HXRES	Hash eXpected RESponse
So, RES can be either milenage res, or received response, so hash of milenage res
*/

typedef struct {
  char *imsiStr;
  char *imeisvStr;
  char *keyStr;
  char *opcStr;
  char *amfStr;
  char *sqnStr;
  char *dnnStr;
  int  nssai_sst;
  int  nssai_sd;
  uint8_t key[16];
  uint8_t opc[16];
  uint8_t amf[2];
  uint8_t sqn[6];
  int nmc_size;
  uint8_t rand[16];
  uint8_t autn[16];
  uint8_t ak[6]; 
  uint8_t akstar[6]; 
  uint8_t ck[16]; 
  uint8_t ik[16]; 
  uint8_t milenage_res[8];
} uicc_t;

/*
 * Read the configuration file, section name variable to be able to manage several UICC
 */
uicc_t *checkUicc(int Mod_id);
uicc_t *init_uicc(char *sectionName);
void uicc_milenage_generate(uint8_t * autn, uicc_t *uicc);
uint8_t getImeisvDigit(const uicc_t *uicc, uint8_t i);
#endif
