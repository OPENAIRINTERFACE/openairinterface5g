/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*****************************************************************************

Source    usim_api.h

Version   0.1

Date    2012/10/09

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Implements the API used by the NAS layer to read/write
    data to/from the USIM application

*****************************************************************************/


#include "usim_api.h"
#include "nas_log.h"
#include "memory.h"
#include <stdio.h>
#include "aka_functions.h"
#include <string.h> // memcpy, memset
#include <stdlib.h> // malloc, free

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * The name of the file where are stored data of the USIM application
 */
#define USIM_API_NVRAM_FILENAME ".usim.nvram"

/*
 * The name of the environment variable which defines the directory
 * where the USIM application file is located
 */
#define USIM_API_NVRAM_DIRNAME  "USIM_DIR"

/*
 * Subscriber authentication security key
 */
#define USIM_API_K_SIZE         16
//#define USIM_API_K_VALUE        "fec86ba6eb707ed08905757b1bb44b8f"
#define USIM_API_K_VALUE        "8BAF473F2F8FD09487CCCBD7097C6862"
#define TEST_USIM_API_K_VALUE   "000102030405060708090a0b0c0d0e0f" // CMW500 K key

static uint8_t _usim_api_k[USIM_API_K_SIZE];


/*
 * List of last used Sequence Numbers SQN
 */
#define USIM_API_AK_SIZE 6
#define USIM_API_SQN_SIZE USIM_API_AK_SIZE
#define USIM_API_SQNMS_SIZE USIM_API_SQN_SIZE

static struct _usim_api_data_s {
  /* Highest sequence number the USIM has ever accepted */
  uint8_t sqn_ms[USIM_API_SQNMS_SIZE];
  /* List of the last used sequence numbers   */
#define USIM_API_SQN_LIST_SIZE  32
  uint8_t n_sqns;
  uint32_t sqn[USIM_API_SQN_LIST_SIZE];
} _usim_api_data;

static uint8_t _usim_api_hex_char_to_hex_value (char c);
static void _usim_api_hex_string_to_hex_value (uint8_t *hex_value, const char *hex_string, int size);
static int _usim_api_check_sqn(uint32_t seq, uint8_t ind);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:        usim_api_read()                                           **
 **                                                                        **
 ** Description: Reads data from the USIM application                      **
 **                                                                        **
 ** Inputs:      None                                                      **
 **              Others:        File where are stored USIM data            **
 **                                                                        **
 ** Outputs:     data:          Pointer to the USIM application data       **
 **              Return:        RETURNerror, RETURNok                      **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
int usim_api_read(usim_data_t* data)
{
  LOG_FUNC_IN;

  /* Get USIM application pathname */
  char* path = memory_get_path(USIM_API_NVRAM_DIRNAME,
                               USIM_API_NVRAM_FILENAME);

  if (path == NULL) {
    LOG_TRACE(ERROR, "USIM-API  - Failed to get USIM pathname");
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Read USIM application data */
  if (memory_read(path, data, sizeof(usim_data_t)) != RETURNok) {
    LOG_TRACE(ERROR, "USIM-API  - %s file is either not valid "
              "or not present", path);
    free(path);
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* initialize the subscriber authentication security key */
  if(data->usimtestmode == 0)
  {
    _usim_api_hex_string_to_hex_value(_usim_api_k, USIM_API_K_VALUE, USIM_API_K_SIZE);
  }
  else
  {
    _usim_api_hex_string_to_hex_value(_usim_api_k, TEST_USIM_API_K_VALUE, USIM_API_K_SIZE);
  }

  free(path);
  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:        usim_api_write()                                          **
 **                                                                        **
 ** Description: Writes data to the USIM application                       **
 **                                                                        **
 ** Inputs:      data:          Pointer to the USIM application data       **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **              Return:        RETURNerror, RETURNok                      **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
int usim_api_write(const usim_data_t* data)
{
  LOG_FUNC_IN;

  /* Get USIM application pathname */
  char* path = memory_get_path(USIM_API_NVRAM_DIRNAME,
                               USIM_API_NVRAM_FILENAME);

  if (path == NULL) {
    LOG_TRACE(ERROR, "USIM-API  - Failed to get USIM pathname");
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Write USIM application data */
  if (memory_write(path, data, sizeof(usim_data_t)) != RETURNok) {

    LOG_TRACE(ERROR, "USIM-API  - Unable to write USIM file %s", path);
    free(path);
    LOG_FUNC_RETURN (RETURNerror);
  }

  free(path);
  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:        usim_api_authenticate_test()                              **
 **                                                                        **
 ** Description: Performs mutual authentication of the USIM to the network,**
 **              checking whether authentication token AUTN can be accep-  **
 **              ted. If so, returns an authentication response RES and    **
 **              the ciphering and integrity keys.                         **
 **              In case of synch failure, returns a re-synchronization    **
 **              token AUTS.                                               **
 **                                                                        **
 **              Key Generation for Test USIM based on 34.108              **
 **                                                                        **
 **              Authentication and key generating function algorithms are **
 **              specified in 3GPP TS 35.206.                              **
 **                                                                        **
 ** Inputs:      rand_pP:          Random challenge number                    **
 **              autn_pP:          Authentication token                       **
 **                             AUTN = (SQN xor AK) || AMF || MAC          **
 **                                         48          16     64 bits     **
 **              Others:        Security key                               **
 **                                                                        **
 ** Outputs:     auts_pP:          Re-synchronization token                   **
 **              res_pP:           Authentication response                    **
 **              ck_pP:            Ciphering key                              **
 **              ik_pP             Integrity key                              **
 **                                                                        **
 **              Return:        RETURNerror, RETURNok                      **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
int usim_api_authenticate_test(const OctetString* rand_pP, const OctetString* autn_pP,
                               OctetString* auts_pP, OctetString* res_pP,
                               OctetString* ck_pP, OctetString* ik_pP)
{
  LOG_FUNC_IN;

  int rc;
  int i;

  LOG_TRACE(INFO, "USIM-API  - rand :%s",dump_octet_string(rand_pP));
  LOG_TRACE(INFO, "USIM-API  - autn :%s",dump_octet_string(autn_pP));

  //step1: XDOUT = RAND xor K
  //       RES = XDOUT
  for (i=0; i<USIM_API_K_SIZE; i++)
  {
      res_pP->value[i] = rand_pP->value[i] ^ _usim_api_k[i];
  }

  //step2: res = f2(xdout,n)
  //       ck  = f3(xdout)
  //       ik  = f4(xdout)
  //       ak  = f5(xdout)
  u8 ak[USIM_API_AK_SIZE];
  for (i=0; i<15; i++)
  {
      ck_pP->value[i] = res_pP->value[i+1];
  }
  ck_pP->value[15] = res_pP->value[0];

  for (i=0; i<14; i++)
  {
      ik_pP->value[i] = res_pP->value[i+2];
  }
  ik_pP->value[14] = res_pP->value[0];
  ik_pP->value[15] = res_pP->value[1];

  for (i=0; i<USIM_API_AK_SIZE; i++)
  {
      ak[i] = res_pP->value[i+3];
  }
  LOG_TRACE(INFO, "USIM-API  - res(f2)  :%s",dump_octet_string(res_pP));
  LOG_TRACE(INFO, "USIM-API  - ck(f3)   :%s",dump_octet_string(ck_pP));
  LOG_TRACE(INFO, "USIM-API  - ik(f4)   :%s",dump_octet_string(ik_pP));
  LOG_TRACE(INFO, "USIM-API  - ak(f5)   : %02X%02X%02X%02X%02X%02X",
            ak[0],ak[1],ak[2],ak[3],ak[4],ak[5]);

  //step3: concatenate SQN with AMP SQN||AMF
  //       SQN = AUTN xor ak
  u8 sqn[USIM_API_SQN_SIZE];
  for (i = 0; i < USIM_API_SQN_SIZE; i++) {
    sqn[i] = autn_pP->value[i] ^ ak[i];
  }
  LOG_TRACE(INFO, "USIM-API  - Retrieved SQN %02X%02X%02X%02X%02X%02X",
            sqn[0],sqn[1],sqn[2],sqn[3],sqn[4],sqn[5]);
  LOG_TRACE(INFO, "USIM-API  - Retrieved AMF %02X%02X",
            autn_pP->value[USIM_API_SQN_SIZE],autn_pP->value[USIM_API_SQN_SIZE+1]);

#define USIM_API_XMAC_SIZE 8
  u8 cdout[USIM_API_XMAC_SIZE];
  for (i = 0; i < USIM_API_XMAC_SIZE; i++)
  {
    if(i < USIM_API_SQN_SIZE)
    {
        cdout[i] = sqn[i];
    }
    else
    {
        cdout[i] = autn_pP->value[i];
    }
  }
  LOG_TRACE(INFO, "USIM-API  - Retrieved CDOUT %02X%02X%02X%02X%02X%02X%02X%02X",
          cdout[0],cdout[1],cdout[2],cdout[3],cdout[4],cdout[5],cdout[6],cdout[7]);

  //step4:calculate XMAC from cdout and xdout
  u8 xmac[USIM_API_XMAC_SIZE];
  for(i = 0; i < USIM_API_XMAC_SIZE; i++)
  {
      xmac[i] = res_pP->value[i] ^ cdout[i];
  }
  LOG_TRACE(INFO,
            "USIM-API  - Computed XMAC %02X%02X%02X%02X%02X%02X%02X%02X",
            xmac[0],xmac[1],xmac[2],xmac[3],
            xmac[4],xmac[5],xmac[6],xmac[7]);

  /* Compare the XMAC with the MAC included in AUTN */
#define USIM_API_AMF_SIZE 2

  if ( memcmp(xmac, &autn_pP->value[USIM_API_SQN_SIZE + USIM_API_AMF_SIZE],
              USIM_API_XMAC_SIZE) != 0 ) {
    LOG_TRACE(INFO,
              "USIM-API  - Comparing the XMAC with the MAC included in AUTN Failed");
    rc = RETURNerror;
    //LOG_FUNC_RETURN (RETURNerror);
  } else {
    LOG_TRACE(INFO,
              "USIM-API  - Comparing the XMAC with the MAC included in AUTN Succeeded");
    /* Verify that the received sequence number SQN is in the correct range */
    rc = _usim_api_check_sqn(*(uint32_t*)(sqn), sqn[USIM_API_SQN_SIZE - 1]);
  }


  if (rc != RETURNok) {
    /* Synchronisation failure; compute the AUTS parameter */

    /* Concealed value of the counter SQNms in the USIM:
     * Conc(SQNMS) = SQNMS ⊕ f5*K(RAND) */
    f5star(_usim_api_k, rand_pP->value, ak);


    u8 sqn_ms[USIM_API_SQNMS_SIZE];
    memset(sqn_ms, 0, USIM_API_SQNMS_SIZE);

    //#define USIM_API_SQN_MS_SIZE  3
    printf("_usim_api_data.sqn_ms %p\n",_usim_api_data.sqn_ms);
    for (i = 0; i < USIM_API_SQNMS_SIZE; i++) {
      //#warning "LG:BUG HERE TODO"
      printf("i %d:  ((uint8_t*)(_usim_api_data.sqn_ms))[USIM_API_SQNMS_SIZE - i] %d\n",i, ((uint8_t*)(_usim_api_data.sqn_ms))[USIM_API_SQNMS_SIZE - i]);
      sqn_ms[USIM_API_SQNMS_SIZE - i] =
        ((uint8_t*)(_usim_api_data.sqn_ms))[USIM_API_SQNMS_SIZE - i];
    }

    u8 sqnms[USIM_API_SQNMS_SIZE];

    for (i = 0; i < USIM_API_SQNMS_SIZE; i++) {
      sqnms[i] = sqn_ms[i] ^ ak[i];
    }

    LOG_TRACE(DEBUG, "USIM-API  - SQNms %02X%02X%02X%02X%02X%02X",
              sqnms[0],sqnms[1],sqnms[2],sqnms[3],sqnms[4],sqnms[5]);

    /* Synchronisation message authentication code:
     * MACS = f1*K(SQNMS || RAND || AMF) */
#define USIM_API_MACS_SIZE USIM_API_XMAC_SIZE
    u8 macs[USIM_API_MACS_SIZE];
    f1star(_usim_api_k, rand_pP->value, sqn_ms,
           &rand_pP->value[USIM_API_SQN_SIZE], macs);
    LOG_TRACE(DEBUG, "USIM-API  - MACS %02X%02X%02X%02X%02X%02X%02X%02X",
              macs[0],macs[1],macs[2],macs[3],
              macs[4],macs[5],macs[6],macs[7]);

    /* Synchronisation authentication token:
     * AUTS = Conc(SQNMS) || MACS */
    memcpy(&auts_pP->value[0], sqnms, USIM_API_SQNMS_SIZE);
    memcpy(&auts_pP->value[USIM_API_SQNMS_SIZE], macs, USIM_API_MACS_SIZE);
    auts_pP->length = USIM_API_SQNMS_SIZE + USIM_API_MACS_SIZE;
    LOG_FUNC_RETURN (RETURNerror);
  }

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:        usim_api_authenticate()                                   **
 **                                                                        **
 ** Description: Performs mutual authentication of the USIM to the network,**
 **              checking whether authentication token AUTN can be accep-  **
 **              ted. If so, returns an authentication response RES and    **
 **              the ciphering and integrity keys.                         **
 **              In case of synch failure, returns a re-synchronization    **
 **              token AUTS.                                               **
 **                                                                        **
 **              3GPP TS 31.102, section 7.1.1.1                           **
 **                                                                        **
 **              Authentication and key generating function algorithms are **
 **              specified in 3GPP TS 35.206.                              **
 **                                                                        **
 ** Inputs:      rand_pP:          Random challenge number                    **
 **              autn_pP:          Authentication token                       **
 **                             AUTN = (SQN xor AK) || AMF || MAC          **
 **                                         48          16     64 bits     **
 **              Others:        Security key                               **
 **                                                                        **
 ** Outputs:     auts_pP:          Re-synchronization token                   **
 **              res_pP:           Authentication response                    **
 **              ck_pP:            Ciphering key                              **
 **              ik_pP             Integrity key                              **
 **                                                                        **
 **              Return:        RETURNerror, RETURNok                      **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
int usim_api_authenticate(const OctetString* rand_pP, const OctetString* autn_pP,
                          OctetString* auts_pP, OctetString* res_pP,
                          OctetString* ck_pP, OctetString* ik_pP)
{
  LOG_FUNC_IN;

  int rc;
  int i;

  LOG_TRACE(DEBUG, "USIM-API  - rand :%s",dump_octet_string(rand_pP));
  LOG_TRACE(DEBUG, "USIM-API  - autn :%s",dump_octet_string(autn_pP));

  /* Compute the authentication response RES = f2K (RAND) */
  /* Compute the cipher key CK = f3K (RAND) */
  /* Compute the integrity key IK = f4K (RAND) */
  /* Compute the anonymity key AK = f5K (RAND) */

  u8 ak[USIM_API_AK_SIZE];
  f2345(_usim_api_k, rand_pP->value,
        res_pP->value, ck_pP->value, ik_pP->value, ak);
  LOG_TRACE(INFO, "USIM-API  - res(f2)  :%s",dump_octet_string(res_pP));
  LOG_TRACE(INFO, "USIM-API  - ck(f3)   :%s",dump_octet_string(ck_pP));
  LOG_TRACE(INFO, "USIM-API  - ik(f4)   :%s",dump_octet_string(ik_pP));
  LOG_TRACE(INFO, "USIM-API  - ak(f5)   : %02X%02X%02X%02X%02X%02X",
            ak[0],ak[1],ak[2],ak[3],ak[4],ak[5]);

  /* Retrieve the sequence number SQN = (SQN ⊕ AK) ⊕ AK */

  u8 sqn[USIM_API_SQN_SIZE];

  for (i = 0; i < USIM_API_SQN_SIZE; i++) {
    sqn[i] = autn_pP->value[i] ^ ak[i];
  }

  LOG_TRACE(INFO, "USIM-API  - Retrieved SQN %02X%02X%02X%02X%02X%02X",
            sqn[0],sqn[1],sqn[2],sqn[3],sqn[4],sqn[5]);

  /* Compute XMAC = f1K (SQN || RAND || AMF) */
#define USIM_API_XMAC_SIZE 8
  u8 xmac[USIM_API_XMAC_SIZE];
  f1(_usim_api_k, rand_pP->value, sqn, &autn_pP->value[USIM_API_SQN_SIZE], xmac);
  LOG_TRACE(DEBUG,
            "USIM-API  - Computed XMAC %02X%02X%02X%02X%02X%02X%02X%02X",
            xmac[0],xmac[1],xmac[2],xmac[3],
            xmac[4],xmac[5],xmac[6],xmac[7]);

  /* Compare the XMAC with the MAC included in AUTN */
#define USIM_API_AMF_SIZE 2

  if ( memcmp(xmac, &autn_pP->value[USIM_API_SQN_SIZE + USIM_API_AMF_SIZE],
              USIM_API_XMAC_SIZE) != 0 ) {
    LOG_TRACE(INFO,
              "USIM-API  - Comparing the XMAC with the MAC included in AUTN Failed");
    rc = RETURNerror;
    //LOG_FUNC_RETURN (RETURNerror);
  } else {
    LOG_TRACE(INFO,
              "USIM-API  - Comparing the XMAC with the MAC included in AUTN Succeeded");
    /* Verify that the received sequence number SQN is in the correct range */
    rc = _usim_api_check_sqn(*(uint32_t*)(sqn), sqn[USIM_API_SQN_SIZE - 1]);
  }


  if (rc != RETURNok) {
    /* Synchronisation failure; compute the AUTS parameter */

    /* Concealed value of the counter SQNms in the USIM:
     * Conc(SQNMS) = SQNMS ⊕ f5*K(RAND) */
    f5star(_usim_api_k, rand_pP->value, ak);


    u8 sqn_ms[USIM_API_SQNMS_SIZE];
    memset(sqn_ms, 0, USIM_API_SQNMS_SIZE);

    //#define USIM_API_SQN_MS_SIZE  3
    printf("_usim_api_data.sqn_ms %p\n",_usim_api_data.sqn_ms);
    for (i = 0; i < USIM_API_SQNMS_SIZE; i++) {
      //#warning "LG:BUG HERE TODO"
      printf("i %d:  ((uint8_t*)(_usim_api_data.sqn_ms))[USIM_API_SQNMS_SIZE - i] %d\n",i, ((uint8_t*)(_usim_api_data.sqn_ms))[USIM_API_SQNMS_SIZE - i]);
      sqn_ms[USIM_API_SQNMS_SIZE - i] =
        ((uint8_t*)(_usim_api_data.sqn_ms))[USIM_API_SQNMS_SIZE - i];
    }

    u8 sqnms[USIM_API_SQNMS_SIZE];

    for (i = 0; i < USIM_API_SQNMS_SIZE; i++) {
      sqnms[i] = sqn_ms[i] ^ ak[i];
    }

    LOG_TRACE(DEBUG, "USIM-API  - SQNms %02X%02X%02X%02X%02X%02X",
              sqnms[0],sqnms[1],sqnms[2],sqnms[3],sqnms[4],sqnms[5]);

    /* Synchronisation message authentication code:
     * MACS = f1*K(SQNMS || RAND || AMF) */
#define USIM_API_MACS_SIZE USIM_API_XMAC_SIZE
    u8 macs[USIM_API_MACS_SIZE];
    f1star(_usim_api_k, rand_pP->value, sqn_ms,
           &rand_pP->value[USIM_API_SQN_SIZE], macs);
    LOG_TRACE(DEBUG, "USIM-API  - MACS %02X%02X%02X%02X%02X%02X%02X%02X",
              macs[0],macs[1],macs[2],macs[3],
              macs[4],macs[5],macs[6],macs[7]);

    /* Synchronisation authentication token:
     * AUTS = Conc(SQNMS) || MACS */
    memcpy(&auts_pP->value[0], sqnms, USIM_API_SQNMS_SIZE);
    memcpy(&auts_pP->value[USIM_API_SQNMS_SIZE], macs, USIM_API_MACS_SIZE);
    auts_pP->length = USIM_API_SQNMS_SIZE + USIM_API_MACS_SIZE;
    LOG_FUNC_RETURN (RETURNerror);
  }

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:        _usim_api_hex_char_to_hex_value()                         **
 **                                                                        **
 ** Description: Converts an hexadecimal ASCII coded digit into its value. **
 **                                                                        **
 ** Inputs:      c:             A char holding the ASCII coded value       **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **              Return:        Converted value                            **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
static uint8_t _usim_api_hex_char_to_hex_value (char c)
{
  if (c >= 'A') {
    /* Remove case bit */
    c &= ~('a' ^ 'A');

    return (c - 'A' + 10);
  } else {
    return (c - '0');
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:        _usim_api_hex_string_to_hex_value()                       **
 **                                                                        **
 ** Description: Converts an hexadecimal ASCII coded string into its value.**
 **                                                                        **
 ** Inputs:      hex_value:     A pointer to the location to store the     **
 **                             conversion result                          **
 **              size:          The size of hex_value in bytes             **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     hex_value:     Converted value                            **
 **              Return:        None                                       **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
static void _usim_api_hex_string_to_hex_value (uint8_t *hex_value, const char *hex_string, int size)
{
  int i;

  for (i=0; i < size; i++) {
    hex_value[i] = (_usim_api_hex_char_to_hex_value(hex_string[2 * i]) << 4) | _usim_api_hex_char_to_hex_value(hex_string[2 * i + 1]);
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:        _usim_api_check_sqn()                                     **
 **                                                                        **
 ** Description: Verifies the freshness of sequence numbers to determine   **
 **              whether the specified sequence number is in the correct   **
 **              range and acceptabled by the USIM.                        **
 **                                                                        **
 **              3GPP TS 33.102, Annex C.2                                 **
 **                                                                        **
 ** Inputs:      seq:           Sequence number value                      **
 **              ind:           Index value                                **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **              Return:        RETURNerror, RETURNok                      **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
static int _usim_api_check_sqn(uint32_t seq, uint8_t ind)
{
  /* TODO */
  return (RETURNok);
}

