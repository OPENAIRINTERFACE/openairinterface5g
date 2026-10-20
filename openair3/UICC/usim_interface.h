/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
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
#include "pdu_session.h"

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
  pdu_session_config_t pdu_sessions[256];
  int n_pdu_sessions;
  char *routing_indicatorStr;
  int protection_scheme;
  char *home_network_public_keyStr;
  int home_network_public_key_id;
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
  uint8_t home_network_public_key[32];
} uicc_t;

/*
 * Read the configuration file, section name variable to be able to manage several UICC
 */
uicc_t *checkUicc(int Mod_id);
uicc_t *init_uicc(char *sectionName);
void uicc_milenage_generate(uint8_t * autn, uicc_t *uicc);
uint8_t getImeisvDigit(const uicc_t *uicc, uint8_t i);
#endif
