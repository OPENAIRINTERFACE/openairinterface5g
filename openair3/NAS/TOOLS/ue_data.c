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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************
Source    ue_data.c

Version   0.1

Date    2012/11/02

Product   UE data generator

Subsystem UE data generator main process

Author    Frederic Maurel

Description Implements the utility used to generate data stored in the
    UE's non-volatile memory device

*****************************************************************************/

#include "userDef.h"
#include "memory.h"

#include "emmData.h"
#include "network.h"

#include <stdio.h>  // perror, printf, fprintf, snprintf
#include <stdlib.h> // exit, free
#include <string.h> // memset, strncpy

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

#define USER_IMEI   "35611302209414"
#define USER_MANUFACTURER "EURECOM"
#define USER_MODEL    "LTE Android PC"
//#define USER_MANUFACTURER "SAGEM"
//#define USER_MODEL    "my225x"
#define USER_PIN    "0000"

#define PRINT_PLMN_DIGIT(d) if ((d) != 0xf) printf("%u", (d))

#define PRINT_PLMN(plmn)    \
    PRINT_PLMN_DIGIT((plmn).MCCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit3); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit3)

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static void _display_usage(const char* command);

static void _gen_user_data(user_nvdata_t* data);
static void _gen_emm_data(emm_nvdata_t* data);

static int _luhn(const char* cc);
static void _display_ue_data(const user_nvdata_t* data);
static void _display_emm_data(const emm_nvdata_t* data);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int main (int argc, const char* argv[])
{
  int rc;
  char* path;
  user_nvdata_t user_data;
  emm_nvdata_t emm_data;

  unsigned char gen_data;

  /*
   * ----------------------------
   * Read command line parameters
   * ----------------------------
   */
  if (argc != 2) {
    fprintf(stderr, "Invalid parameter\n");
    _display_usage(argv[0]);
    exit(EXIT_FAILURE);
  } else if ( (strcmp(argv[1], "--gen") == 0) ||
              (strcmp(argv[1], "-g") == 0) ) {
    /* Generate UE data files */
    gen_data = TRUE;
  } else if ( (strcmp(argv[1], "--print") == 0) ||
              (strcmp(argv[1], "-p") == 0) ) {
    /* Display content of UE data files */
    gen_data = FALSE;
  } else {
    /* Display usage */
    _display_usage(argv[0]);
    exit(EXIT_SUCCESS);
  }
  /*
   * ----------------------
   * UE's non-volatile data
   * ----------------------
   */
  path = memory_get_path(USER_NVRAM_DIRNAME, USER_NVRAM_FILENAME);

  if (path == NULL) {
    perror("ERROR\t: memory_get_path() failed");
    exit(EXIT_FAILURE);
  }

  if (gen_data) {
    /*
     * Initialize UE's non-volatile data
     */
    memset(&user_data, 0, sizeof(user_nvdata_t));
    _gen_user_data(&user_data);
    /*
     * Write UE's non-volatile data
     */
    rc = memory_write(path, &user_data, sizeof(user_nvdata_t));

    if (rc != RETURNok) {
      perror("ERROR\t: memory_write() failed");
      free(path);
      exit(EXIT_FAILURE);
    }
  }

  /*
   * Read UE's non-volatile data
   */
  memset(&user_data, 0, sizeof(user_nvdata_t));
  rc = memory_read(path, &user_data, sizeof(user_nvdata_t));

  if (rc != RETURNok) {
    perror("ERROR\t: memory_read() failed");
    free(path);
    exit(EXIT_FAILURE);
  }

  free(path);
  /*
   * Display UE's non-volatile data
   */
  printf("\nUE's non-volatile data:\n\n");
  _display_ue_data(&user_data);

  /*
   * ---------------------
   * EMM non-volatile data
   * ---------------------
   */
  path = memory_get_path(EMM_NVRAM_DIRNAME, EMM_NVRAM_FILENAME);

  if (path == NULL) {
    perror("ERROR\t: memory_get_path() failed");
    exit(EXIT_FAILURE);
  }

  if (gen_data) {
    /*
     * Initialize EMM non-volatile data
     */
    memset(&emm_data, 0, sizeof(emm_nvdata_t));
    _gen_emm_data(&emm_data);
    /*
     * Write EMM non-volatile data
     */
    rc = memory_write(path, &emm_data, sizeof(emm_nvdata_t));

    if (rc != RETURNok) {
      perror("ERROR\t: memory_write() failed");
      free(path);
      exit(EXIT_FAILURE);
    }
  }

  /*
   * Read EMM non-volatile data
   */
  memset(&emm_data, 0, sizeof(emm_nvdata_t));
  rc = memory_read(path, &emm_data, sizeof(emm_nvdata_t));

  if (rc != RETURNok) {
    perror("ERROR\t: memory_read() failed ");
    free(path);
    exit(EXIT_FAILURE);
  }

  free(path);
  /*
   * Display EMM non-volatile data
   */
  printf("\nEMM non-volatile data:\n\n");
  _display_emm_data(&emm_data);

  /*
   *---------------
   * Files location
   *---------------
   */
  path = memory_get_path(USER_NVRAM_DIRNAME, USER_NVRAM_FILENAME);
  printf("\nUE identity data file: %s\n", path);
  free(path);
  path = memory_get_path(EMM_NVRAM_DIRNAME, EMM_NVRAM_FILENAME);
  printf("EPS Mobility Management data file: %s\n", path);
  free(path);

  exit(EXIT_SUCCESS);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * Displays command line usage
 */
static void _display_usage(const char* command)
{
  fprintf(stderr, "usage: %s [OPTION]\n", command);
  fprintf(stderr, "\t[--gen|-g]\tGenerate the UE data files\n");
  fprintf(stderr, "\t[--print|-p]\tDisplay the content of the UE data files\n");
  fprintf(stderr, "\t[--help|-h]\tDisplay this usage\n");
  const char* path = getenv("NVRAM_DIR");

  if (path != NULL) {
    fprintf(stderr, "NVRAM_DIR = %s\n", path);
  } else {
    fprintf(stderr, "NVRAM_DIR environment variable is not defined\n");
  }
}

/*
 * Generates UE's non-volatile data
 */
static void _gen_user_data(user_nvdata_t* data)
{
  /*
   * Product Serial Number Identification
   * IMEI = AA-BBBBBB-CCCCCC-D
   * AA-BBBBBB: Type Allocation Code (TAC)
   * CCCCCC: Serial Number
   * D: Luhn check digit
   */
  snprintf(data->IMEI, USER_IMEI_SIZE+1, "%s%d",
           USER_IMEI, _luhn(USER_IMEI));
  /*
   * Manufacturer identifier
   */
  strncpy(data->manufacturer, USER_MANUFACTURER, USER_MANUFACTURER_SIZE);
  /*
   * Model identifier
   */
  strncpy(data->model, USER_MODEL, USER_MODEL_SIZE);
  /*
   * SIM Personal Identification Number
   */
  strncpy(data->PIN, USER_PIN, USER_PIN_SIZE);
}

/*
 * Generates UE's non-volatile EMM data
 */
static void _gen_emm_data(emm_nvdata_t* data)
{
#if (SELECTED_PLMN == FCT1)
  /*
   * International Mobile Subscriber Identity
   * IMSI = MCC + MNC + MSIN = 310 (USA) + 028 (UNKNOWN) + 90832150
   */
#warning "IMSI 310.028.90832150"
  data->imsi.length = 8;
  data->imsi.u.num.parity = 0x0;  // Type of identity = IMSI, even
  data->imsi.u.num.digit1 = 3;    // MCC digit 1
  data->imsi.u.num.digit2 = 1;    // MCC digit 2
  data->imsi.u.num.digit3 = 0;    // MCC digit 3
  data->imsi.u.num.digit4 = 0;    // MNC digit 1
  data->imsi.u.num.digit5 = 2;    // MNC digit 2
  data->imsi.u.num.digit6 = 8;    // MNC digit 3
  data->imsi.u.num.digit7 = 9;
  data->imsi.u.num.digit8 = 0;
  data->imsi.u.num.digit9 = 8;
  data->imsi.u.num.digit10 = 3;
  data->imsi.u.num.digit11 = 2;
  data->imsi.u.num.digit12 = 1;
  data->imsi.u.num.digit13 = 5;
  data->imsi.u.num.digit14 = 0;
  data->imsi.u.num.digit15 = 0xF;
  /*
   * Last registered home PLMN
   */
  data->rplmn.MCCdigit1 = 3;
  data->rplmn.MCCdigit2 = 1;
  data->rplmn.MCCdigit3 = 0;
  data->rplmn.MNCdigit1 = 0;
  data->rplmn.MNCdigit2 = 2;
  data->rplmn.MNCdigit3 = 8;
#endif
#if (SELECTED_PLMN == SFR1)
  /*
   * International Mobile Subscriber Identity
   * IMSI = MCC + MNC + MSIN = 208 (France) + 10 (SFR) + 00001234
   */
     data->imsi.length = 8;
     data->imsi.u.num.parity = 0x0;  // Type of identity = IMSI, even
     data->imsi.u.num.digit1 = 2;  // MCC digit 1
     data->imsi.u.num.digit2 = 0;  // MCC digit 2
     data->imsi.u.num.digit3 = 8;  // MCC digit 3
     data->imsi.u.num.digit4 = 1;  // MNC digit 1
     data->imsi.u.num.digit5 = 0;  // MNC digit 2
     data->imsi.u.num.digit6 = 0;//0xF;  // MNC digit 3
     data->imsi.u.num.digit7 = 0;
     data->imsi.u.num.digit8 = 0;
     data->imsi.u.num.digit9 = 0;
     data->imsi.u.num.digit10 = 0;
     data->imsi.u.num.digit11 = 1;
     data->imsi.u.num.digit12 = 2;
     data->imsi.u.num.digit13 = 3;
     data->imsi.u.num.digit14 = 4;
     data->imsi.u.num.digit15 = 0xF;

     data->rplmn.MCCdigit1 = 2;
     data->rplmn.MCCdigit2 = 0;
     data->rplmn.MCCdigit3 = 8;
     data->rplmn.MNCdigit1 = 1;
     data->rplmn.MNCdigit2 = 0;
     data->rplmn.MNCdigit3 = 0xf;
#endif
#if (SELECTED_PLMN == OAI_LTEBOX)
  /*
   * International Mobile Subscriber Identity
   * IMSI = MCC + MNC + MSIN = 208 (France) + 93 (OAI) + 00001110
   */
     data->imsi.length = 8;
     data->imsi.u.num.parity = ODD_PARITY;  // Type of identity = IMSI, even
     data->imsi.u.num.digit1 = 2;  // MCC digit 1
     data->imsi.u.num.digit2 = 0;  // MCC digit 2
     data->imsi.u.num.digit3 = 8;  // MCC digit 3
     data->imsi.u.num.digit4 = 9;  // MNC digit 1
     data->imsi.u.num.digit5 = 3;  // MNC digit 2
     data->imsi.u.num.digit6 = 0;  // MNC digit 3
     data->imsi.u.num.digit7 = 1;
     data->imsi.u.num.digit8 = 0;
     data->imsi.u.num.digit9 = 0;
     data->imsi.u.num.digit10 = 0;
     data->imsi.u.num.digit11 = 0;
     data->imsi.u.num.digit12 = 1;
     data->imsi.u.num.digit13 = 1;
     data->imsi.u.num.digit14 = 1;
     data->imsi.u.num.digit15 = 1;

     data->rplmn.MCCdigit1 = 2;
     data->rplmn.MCCdigit2 = 0;
     data->rplmn.MCCdigit3 = 8;
     data->rplmn.MNCdigit1 = 9;
     data->rplmn.MNCdigit2 = 3;
     data->rplmn.MNCdigit3 = 0xf;
#endif
#if (SELECTED_PLMN == TEST1)
  /*
   * International Mobile Subscriber Identity
   * IMSI = MCC + MNC + MSIN = 001  + 01  + 00001234
   */
  data->imsi.length = 8;
  data->imsi.u.num.parity = 0x0;  // Type of identity = IMSI, even
  data->imsi.u.num.digit1 = 0;    // MCC digit 1
  data->imsi.u.num.digit2 = 0;    // MCC digit 2
  data->imsi.u.num.digit3 = 1;    // MCC digit 3
  data->imsi.u.num.digit4 = 0;    // MNC digit 1
  data->imsi.u.num.digit5 = 1;    // MNC digit 2
  data->imsi.u.num.digit6 = 0;
  data->imsi.u.num.digit7 = 0;
  data->imsi.u.num.digit8 = 0;
  data->imsi.u.num.digit9 = 0;
  data->imsi.u.num.digit10 = 0;
  data->imsi.u.num.digit11 = 1;
  data->imsi.u.num.digit12 = 2;
  data->imsi.u.num.digit13 = 3;
  data->imsi.u.num.digit14 = 4;
  data->imsi.u.num.digit15 = 0xF;

  /*
   * Last registered home PLMN
   */
  data->rplmn.MCCdigit1 = 0;
  data->rplmn.MCCdigit2 = 0;
  data->rplmn.MCCdigit3 = 1;
  data->rplmn.MNCdigit1 = 0;
  data->rplmn.MNCdigit2 = 1;
  data->rplmn.MNCdigit3 = 0xf;
#endif
  /*
   * List of Equivalent PLMNs
   */
  data->eplmn.n_plmns = 0;
}

/*
 * Computes the check digit using Luhn algorithm
 */
static int _luhn(const char* cc)
{
  const int m[] = {0,2,4,6,8,1,3,5,7,9};
  int odd = 1, sum = 0;

  for (int i = strlen(cc); i--; odd = !odd) {
    int digit = cc[i] - '0';
    sum += odd ? m[digit] : digit;
  }

  return 10 - (sum % 10);
}

/*
 * Displays UE's non-volatile data
 */
static void _display_ue_data(const user_nvdata_t* data)
{
  printf("IMEI\t\t= %s\n", data->IMEI);
  printf("manufacturer\t= %s\n", data->manufacturer);
  printf("model\t\t= %s\n", data->model);
  printf("PIN\t\t= %s\n", data->PIN);
}

/*
 * Displays UE's non-volatile EMM data
 */
static void _display_emm_data(const emm_nvdata_t* data)
{
  printf("IMSI\t\t= ");

  if (data->imsi.u.num.digit6 == 0b1111) {
    if (data->imsi.u.num.digit15 == 0b1111) {
      printf("%u%u%u.%u%u.%u%u%u%u%u%u%u%u\n",
             data->imsi.u.num.digit1,
             data->imsi.u.num.digit2,
             data->imsi.u.num.digit3,
             data->imsi.u.num.digit4,
             data->imsi.u.num.digit5,

             data->imsi.u.num.digit7,
             data->imsi.u.num.digit8,
             data->imsi.u.num.digit9,
             data->imsi.u.num.digit10,
             data->imsi.u.num.digit11,
             data->imsi.u.num.digit12,
             data->imsi.u.num.digit13,
             data->imsi.u.num.digit14);
    } else {
      printf("%u%u%u.%u%u.%u%u%u%u%u%u%u%u%u\n",
             data->imsi.u.num.digit1,
             data->imsi.u.num.digit2,
             data->imsi.u.num.digit3,
             data->imsi.u.num.digit4,
             data->imsi.u.num.digit5,

             data->imsi.u.num.digit7,
             data->imsi.u.num.digit8,
             data->imsi.u.num.digit9,
             data->imsi.u.num.digit10,
             data->imsi.u.num.digit11,
             data->imsi.u.num.digit12,
             data->imsi.u.num.digit13,
             data->imsi.u.num.digit14,
             data->imsi.u.num.digit15);
    }
  } else {
    if (data->imsi.u.num.digit15 == 0b1111) {
      printf("%u%u%u.%u%u%u.%u%u%u%u%u%u%u%u\n",
             data->imsi.u.num.digit1,
             data->imsi.u.num.digit2,
             data->imsi.u.num.digit3,
             data->imsi.u.num.digit4,
             data->imsi.u.num.digit5,
             data->imsi.u.num.digit6,

             data->imsi.u.num.digit7,
             data->imsi.u.num.digit8,
             data->imsi.u.num.digit9,
             data->imsi.u.num.digit10,
             data->imsi.u.num.digit11,
             data->imsi.u.num.digit12,
             data->imsi.u.num.digit13,
             data->imsi.u.num.digit14);
    } else {
      printf("%u%u%u.%u%u%u.%u%u%u%u%u%u%u%u%u\n",
             data->imsi.u.num.digit1,
             data->imsi.u.num.digit2,
             data->imsi.u.num.digit3,
             data->imsi.u.num.digit4,
             data->imsi.u.num.digit5,
             data->imsi.u.num.digit6,

             data->imsi.u.num.digit7,
             data->imsi.u.num.digit8,
             data->imsi.u.num.digit9,
             data->imsi.u.num.digit10,
             data->imsi.u.num.digit11,
             data->imsi.u.num.digit12,
             data->imsi.u.num.digit13,
             data->imsi.u.num.digit14,
             data->imsi.u.num.digit15);
    }
  }

  printf("RPLMN\t\t= ");
  PRINT_PLMN(data->rplmn);
  printf("\n");

  for (int i = 0; i < data->eplmn.n_plmns; i++) {
    printf("EPLMN[%d]\t= ", i);
    PRINT_PLMN(data->eplmn.plmn[i]);
    printf("\n");
  }
}
