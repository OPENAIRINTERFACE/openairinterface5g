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
Source    usim_data.c

Version   0.1

Date    2012/10/31

Product   USIM data generator

Subsystem USIM data generator main process

Author    Frederic Maurel

Description Implements the utility used to generate data stored in the
    USIM application

 *****************************************************************************/

#include "network.h"

#include "usim_api.h"
#include "memory.h"
#include "network.h"

#include <stdio.h>  // perror, printf
#include <stdlib.h> // exit
#include <string.h> // memset, memcpy, strncpy


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

#define KSI     USIM_KSI_NOT_AVAILABLE
#define KSI_ASME    USIM_KSI_NOT_AVAILABLE
#define INT_ALGO    USIM_INT_EIA2
#define ENC_ALGO    USIM_ENC_EEA0
#define SECURITY_ALGORITHMS (ENC_ALGO | INT_ALGO)

#define MIN_TAC     0x0000
#define MAX_TAC     0xFFFE

#define DEFAULT_TMSI    0x0000000D
#define DEFAULT_P_TMSI    0x0000000D
#define DEFAULT_M_TMSI    0x0000000D
#define DEFAULT_LAC   0xFFFE
#define DEFAULT_RAC   0x01
#define DEFAULT_TAC   0x0001

#define DEFAULT_MME_ID    0x0102
#define DEFAULT_MME_CODE  0x0F

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

static void _display_usim_data(const usim_data_t* data);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int main (int argc, const char* argv[])
{
  int rc;
  usim_data_t usim_data;

  unsigned char gen_data;

  /*
   * Read command line parameters
   */
  if (argc != 2) {
    fprintf(stderr, "Invalid parameter\n");
    _display_usage(argv[0]);
    exit(EXIT_FAILURE);
  } else if ( (strcmp(argv[1], "--gen") == 0) ||
              (strcmp(argv[1], "-g") == 0) ) {
    /* Generate USIM data files */
    gen_data = TRUE;
  } else if ( (strcmp(argv[1], "--print") == 0) ||
              (strcmp(argv[1], "-p") == 0) ) {
    /* Display content of USIM data files */
    gen_data = FALSE;
  } else {
    /* Display usage */
    _display_usage(argv[0]);
    exit(EXIT_SUCCESS);
  }

  if (gen_data) {
    /*
     * Initialize USIM data
     */
    memset(&usim_data, 0, sizeof(usim_data_t));

#if (SELECTED_PLMN == FCT1)
    /*
     * International Mobile Subscriber Identity
     * IMSI = MCC + MNC + MSIN = 310 (USA) + 028 (UNKNOWN) + 90832150
     */
#warning "IMSI 310.028.90832150"
    usim_data.imsi.length = 8;
    usim_data.imsi.u.num.parity = EVEN_PARITY;      // Parity: even
    usim_data.imsi.u.num.digit1 = 3;                // MCC digit 1
    usim_data.imsi.u.num.digit2 = 1;                // MCC digit 2
    usim_data.imsi.u.num.digit3 = 0;                // MCC digit 3
    usim_data.imsi.u.num.digit4 = 0;                // MNC digit 1
    usim_data.imsi.u.num.digit5 = 2;                // MNC digit 2
    usim_data.imsi.u.num.digit6 = 8;                // MNC digit 3
    usim_data.imsi.u.num.digit7 = 9;
    usim_data.imsi.u.num.digit8 = 0;
    usim_data.imsi.u.num.digit9 = 8;
    usim_data.imsi.u.num.digit10 = 3;
    usim_data.imsi.u.num.digit11 = 2;
    usim_data.imsi.u.num.digit12 = 1;
    usim_data.imsi.u.num.digit13 = 5;
    usim_data.imsi.u.num.digit14 = 0;
    usim_data.imsi.u.num.digit15 = 0b1111;
#endif
#if (SELECTED_PLMN == SFR1)

    /*
     * International Mobile Subscriber Identity
     * IMSI = MCC + MNC + MSIN = 208 (France) + 10 (SFR) + 00001234
     */
#warning "IMSI 208.10.00001234"

    usim_data.imsi.length = 8;
    usim_data.imsi.u.num.parity = EVEN_PARITY;      // Parity: even
    usim_data.imsi.u.num.digit1 = 2;                // MCC digit 1
    usim_data.imsi.u.num.digit2 = 0;                // MCC digit 2
    usim_data.imsi.u.num.digit3 = 8;                // MCC digit 3
    usim_data.imsi.u.num.digit4 = 1;                // MNC digit 1
    usim_data.imsi.u.num.digit5 = 0;                // MNC digit 2
    usim_data.imsi.u.num.digit6 = 0;//0b1111;     // MNC digit 3
    usim_data.imsi.u.num.digit7 = 0;
    usim_data.imsi.u.num.digit8 = 0;
    usim_data.imsi.u.num.digit9 = 0;
    usim_data.imsi.u.num.digit10 = 0;
    usim_data.imsi.u.num.digit11 = 1;
    usim_data.imsi.u.num.digit12 = 2;
    usim_data.imsi.u.num.digit13 = 3;
    usim_data.imsi.u.num.digit14 = 4;
    usim_data.imsi.u.num.digit15 = 0b1111;
#endif
#if (SELECTED_PLMN == OAI_LTEBOX)
    /*
     * International Mobile Subscriber Identity
     * IMSI = MCC + MNC + MSIN = 208 (France) + 10 (SFR) + 00001234
     */
#warning "IMSI 208.93.0100001111"
    usim_data.imsi.length = 8;
    usim_data.imsi.u.num.parity = ODD_PARITY;      // Parity: even
    usim_data.imsi.u.num.digit1 = 2;                // MCC digit 1
    usim_data.imsi.u.num.digit2 = 0;                // MCC digit 2
    usim_data.imsi.u.num.digit3 = 8;                // MCC digit 3
    usim_data.imsi.u.num.digit4 = 9;                // MNC digit 1
    usim_data.imsi.u.num.digit5 = 3;                // MNC digit 2
    usim_data.imsi.u.num.digit6 = 0;     // MNC digit 3
    usim_data.imsi.u.num.digit7 = 1;
    usim_data.imsi.u.num.digit8 = 0;
    usim_data.imsi.u.num.digit9 = 0;
    usim_data.imsi.u.num.digit10 = 0;
    usim_data.imsi.u.num.digit11 = 0;
    usim_data.imsi.u.num.digit12 = 1;
    usim_data.imsi.u.num.digit13 = 1;
    usim_data.imsi.u.num.digit14 = 1;
    usim_data.imsi.u.num.digit15 = 1;
#endif
#if (SELECTED_PLMN == TEST1)
#warning "IMSI 001.01.000001234"
    usim_data.imsi.length = 8;
    usim_data.imsi.u.num.parity = 0x0;  // Type of identity = IMSI, even
    usim_data.imsi.u.num.digit1 = 0;    // MCC digit 1
    usim_data.imsi.u.num.digit2 = 0;    // MCC digit 2
    usim_data.imsi.u.num.digit3 = 1;    // MCC digit 3
    usim_data.imsi.u.num.digit4 = 0;    // MNC digit 1
    usim_data.imsi.u.num.digit5 = 1;    // MNC digit 2
    usim_data.imsi.u.num.digit6 = 0;
    usim_data.imsi.u.num.digit7 = 0;
    usim_data.imsi.u.num.digit8 = 0;
    usim_data.imsi.u.num.digit9 = 0;
    usim_data.imsi.u.num.digit10 = 0;
    usim_data.imsi.u.num.digit11 = 1;
    usim_data.imsi.u.num.digit12 = 2;
    usim_data.imsi.u.num.digit13 = 3;
    usim_data.imsi.u.num.digit14 = 4;
    usim_data.imsi.u.num.digit15 = 0xF;
    usim_data.usimtestmode = 1; // set usim in test mode in order to get the CMW500 K key
#endif
    /*
     * Ciphering and Integrity Keys
     */
    usim_data.keys.ksi = KSI;
    memset(&usim_data.keys.ck, 0, USIM_CK_SIZE);
    memset(&usim_data.keys.ik, 0, USIM_IK_SIZE);

    /*
     * Higher Priority PLMN search period
     */
    usim_data.hpplmn = 0x00;  /* Disable timer */

    /*
     * List of Forbidden PLMNs
     */
    for (int i = 0; i < USIM_FPLMN_MAX; i++) {
      memset(&usim_data.fplmn[i], 0xff, sizeof(plmn_t));
    }

    /*
     * Location Information
     */
    usim_data.loci.tmsi = DEFAULT_TMSI;
    usim_data.loci.lai.plmn = network_records[SELECTED_PLMN].plmn;
    usim_data.loci.lai.lac = DEFAULT_LAC;
    usim_data.loci.status = USIM_LOCI_NOT_UPDATED;
    /*
     * Packet Switched Location Information
     */
    usim_data.psloci.p_tmsi = DEFAULT_P_TMSI;
    usim_data.psloci.signature[0] = 0x01;
    usim_data.psloci.signature[1] = 0x02;
    usim_data.psloci.signature[2] = 0x03;
    usim_data.psloci.rai.plmn = network_records[SELECTED_PLMN].plmn;
    usim_data.psloci.rai.lac = DEFAULT_LAC;
    usim_data.psloci.rai.rac = DEFAULT_RAC;
    usim_data.psloci.status = USIM_PSLOCI_NOT_UPDATED;
    /*
     * Administrative Data
     */
    usim_data.ad.UE_Operation_Mode = USIM_NORMAL_MODE;
    usim_data.ad.Additional_Info = 0xffff;
    usim_data.ad.MNC_Length = 2;
    /*
     * EPS NAS security context
     */
    usim_data.securityctx.length = 52;
    usim_data.securityctx.KSIasme.type = USIM_KSI_ASME_TAG;
    usim_data.securityctx.KSIasme.length = 1;
    usim_data.securityctx.KSIasme.value[0] = KSI_ASME;
    usim_data.securityctx.Kasme.type = USIM_K_ASME_TAG;
    usim_data.securityctx.Kasme.length = USIM_K_ASME_SIZE;
    memset(usim_data.securityctx.Kasme.value, 0,
           usim_data.securityctx.Kasme.length);
    usim_data.securityctx.ulNAScount.type = USIM_UL_NAS_COUNT_TAG;
    usim_data.securityctx.ulNAScount.length = USIM_UL_NAS_COUNT_SIZE;
    memset(usim_data.securityctx.ulNAScount.value, 0,
           usim_data.securityctx.ulNAScount.length);
    usim_data.securityctx.dlNAScount.type = USIM_DL_NAS_COUNT_TAG;
    usim_data.securityctx.dlNAScount.length = USIM_DL_NAS_COUNT_SIZE;
    memset(usim_data.securityctx.dlNAScount.value, 0,
           usim_data.securityctx.dlNAScount.length);
    usim_data.securityctx.algorithmID.type = USIM_INT_ENC_ALGORITHMS_TAG;
    usim_data.securityctx.algorithmID.length = 1;
    usim_data.securityctx.algorithmID.value[0] = SECURITY_ALGORITHMS;
    /*
     * Subcriber's Number
     */
    usim_data.msisdn.length = 7;
    usim_data.msisdn.number.ext = 1;
    usim_data.msisdn.number.ton = MSISDN_TON_UNKNOWKN;
    usim_data.msisdn.number.npi = MSISDN_NPI_ISDN_TELEPHONY;
    usim_data.msisdn.number.digit[0].msb = 3;
    usim_data.msisdn.number.digit[0].lsb = 3;
    usim_data.msisdn.number.digit[1].msb = 6;
    usim_data.msisdn.number.digit[1].lsb = 1;
    usim_data.msisdn.number.digit[2].msb = 1;
    usim_data.msisdn.number.digit[2].lsb = 1;
    usim_data.msisdn.number.digit[3].msb = 2;
    usim_data.msisdn.number.digit[3].lsb = 3;
    usim_data.msisdn.number.digit[4].msb = 4;
    usim_data.msisdn.number.digit[4].lsb = 5;
    usim_data.msisdn.number.digit[5].msb = 6;
    usim_data.msisdn.number.digit[5].lsb = 0xf;
    usim_data.msisdn.number.digit[6].msb = 0xf;
    usim_data.msisdn.number.digit[6].lsb = 0xf;
    usim_data.msisdn.number.digit[7].msb = 0xf;
    usim_data.msisdn.number.digit[7].lsb = 0xf;
    usim_data.msisdn.number.digit[8].msb = 0xf;
    usim_data.msisdn.number.digit[8].lsb = 0xf;
    usim_data.msisdn.number.digit[9].msb = 0xf;
    usim_data.msisdn.number.digit[9].lsb = 0xf;
    usim_data.msisdn.conf1_record_id = 0xff;  /* Not used */
    usim_data.msisdn.ext1_record_id = 0xff;   /* Not used */

    /*
     * PLMN Network Name and Operator PLMN List
     */
    for (int i = TEST1; i < VDF1; i++) {
      network_record_t record = network_records[i];
      usim_data.pnn[i].fullname.type = USIM_PNN_FULLNAME_TAG;
      usim_data.pnn[i].fullname.length = strlen(record.fullname);
      strncpy((char*)usim_data.pnn[i].fullname.value, record.fullname,
              usim_data.pnn[i].fullname.length);
      usim_data.pnn[i].shortname.type = USIM_PNN_SHORTNAME_TAG;
      usim_data.pnn[i].shortname.length = strlen(record.shortname);
      strncpy((char*)usim_data.pnn[i].shortname.value, record.shortname,
              usim_data.pnn[i].shortname.length);
      usim_data.opl[i].plmn = record.plmn;
      usim_data.opl[i].start = record.tac_start;
      usim_data.opl[i].end = record.tac_end;
      usim_data.opl[i].record_id = i;
    }

    for (int i = VDF2; i < USIM_OPL_MAX; i++) {
      memset(&usim_data.opl[i].plmn, 0xff, sizeof(plmn_t));
    }

    /*
     * List of Equivalent HPLMNs
     */
    usim_data.ehplmn[0] = network_records[SFR2].plmn;
    usim_data.ehplmn[1] = network_records[SFR3].plmn;
    /*
     * Home PLMN Selector with Access Technology
     */
    usim_data.hplmn.plmn = network_records[SELECTED_PLMN].plmn;
    usim_data.hplmn.AcT = (USIM_ACT_GSM | USIM_ACT_UTRAN | USIM_ACT_EUTRAN);

    /*
     * List of user controlled PLMN selector with Access Technology
     */
    for (int i = 0; i < USIM_PLMN_MAX; i++) {
      memset(&usim_data.plmn[i], 0xff, sizeof(plmn_t));
    }

    /*
     * List of operator controlled PLMN selector with Access Technology
     */
    usim_data.oplmn[0].plmn = network_records[VDF1].plmn;
    usim_data.oplmn[0].AcT = (USIM_ACT_GSM | USIM_ACT_UTRAN | USIM_ACT_EUTRAN);
    usim_data.oplmn[1].plmn = network_records[VDF2].plmn;
    usim_data.oplmn[1].AcT = (USIM_ACT_GSM | USIM_ACT_UTRAN | USIM_ACT_EUTRAN);
    usim_data.oplmn[2].plmn = network_records[VDF3].plmn;
    usim_data.oplmn[2].AcT = (USIM_ACT_GSM | USIM_ACT_UTRAN | USIM_ACT_EUTRAN);
    usim_data.oplmn[3].plmn = network_records[VDF4].plmn;
    usim_data.oplmn[3].AcT = (USIM_ACT_GSM | USIM_ACT_UTRAN | USIM_ACT_EUTRAN);
    usim_data.oplmn[4].plmn = network_records[VDF5].plmn;
    usim_data.oplmn[4].AcT = (USIM_ACT_GSM | USIM_ACT_UTRAN | USIM_ACT_EUTRAN);

    for (int i = 5; i < USIM_OPLMN_MAX; i++) {
      memset(&usim_data.oplmn[i], 0xff, sizeof(plmn_t));
    }

    /*
     * EPS Location Information
     */
    usim_data.epsloci.guti.gummei.plmn = network_records[SELECTED_PLMN].plmn;
    usim_data.epsloci.guti.gummei.MMEgid = DEFAULT_MME_ID;
    usim_data.epsloci.guti.gummei.MMEcode = DEFAULT_MME_CODE;
    usim_data.epsloci.guti.m_tmsi = DEFAULT_M_TMSI;
    usim_data.epsloci.tai.plmn = usim_data.epsloci.guti.gummei.plmn;
    usim_data.epsloci.tai.tac = DEFAULT_TAC;
    usim_data.epsloci.status = USIM_EPSLOCI_UPDATED;
    /*
     * Non-Access Stratum configuration
     */
    usim_data.nasconfig.NAS_SignallingPriority.type = USIM_NAS_SIGNALLING_PRIORITY_TAG;
    usim_data.nasconfig.NAS_SignallingPriority.length = 1;
    usim_data.nasconfig.NAS_SignallingPriority.value[0] = 0x00;
    usim_data.nasconfig.NMO_I_Behaviour.type = USIM_NMO_I_BEHAVIOUR_TAG;
    usim_data.nasconfig.NMO_I_Behaviour.length = 1;
    usim_data.nasconfig.NMO_I_Behaviour.value[0] = 0x00;
    usim_data.nasconfig.AttachWithImsi.type = USIM_ATTACH_WITH_IMSI_TAG;
    usim_data.nasconfig.AttachWithImsi.length = 1;
#if defined(START_WITH_GUTI)
    usim_data.nasconfig.AttachWithImsi.value[0] = 0x00;
#else
    usim_data.nasconfig.AttachWithImsi.value[0] = 0x01;
#endif
    usim_data.nasconfig.MinimumPeriodicSearchTimer.type = USIM_MINIMUM_PERIODIC_SEARCH_TIMER_TAG;
    usim_data.nasconfig.MinimumPeriodicSearchTimer.length = 1;
    usim_data.nasconfig.MinimumPeriodicSearchTimer.value[0] = 0x00;
    usim_data.nasconfig.ExtendedAccessBarring.type = USIM_EXTENDED_ACCESS_BARRING_TAG;
    usim_data.nasconfig.ExtendedAccessBarring.length = 1;
    usim_data.nasconfig.ExtendedAccessBarring.value[0] = 0x00;
    usim_data.nasconfig.Timer_T3245_Behaviour.type = USIM_TIMER_T3245_BEHAVIOUR_TAG;
    usim_data.nasconfig.Timer_T3245_Behaviour.length = 1;
    usim_data.nasconfig.Timer_T3245_Behaviour.value[0] = 0x00;

    /*
     * Write USIM application data
     */
    rc = usim_api_write(&usim_data);

    if (rc != RETURNok) {
      perror("ERROR\t: usim_api_write() failed");
      exit(EXIT_FAILURE);
    }
  }

  /*
   * Read USIM application data
   */
  memset(&usim_data, 0, sizeof(usim_data_t));
  rc = usim_api_read(&usim_data);

  if (rc != RETURNok) {
    perror("ERROR\t: usim_api_read() failed");
    exit(EXIT_FAILURE);
  }

  /*
   * Display USIM application data
   */
  printf("\nUSIM data:\n\n");
  _display_usim_data(&usim_data);

  /*
   * Display USIM file location
   */
  char* path = memory_get_path("USIM_DIR", ".usim.nvram");
  printf("\nUSIM data file: %s\n", path);
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
  fprintf(stderr, "\t[--gen|-g]\tGenerate the USIM data file\n");
  fprintf(stderr, "\t[--print|-p]\tDisplay the content of the USIM data file\n");
  fprintf(stderr, "\t[--help|-h]\tDisplay this usage\n");
  const char* path = getenv("USIM_DIR");

  if (path != NULL) {
    fprintf(stderr, "USIM_DIR = %s\n", path);
  } else {
    fprintf(stderr, "USIM_DIR environment variable is not defined\n");
  }
}

/*
 * Displays USIM application data
 */
static void _display_usim_data(const usim_data_t* data)
{
  int digits;

  printf("Administrative Data:\n");
  printf("\tUE_Operation_Mode\t= 0x%.2x\n", data->ad.UE_Operation_Mode);
  printf("\tAdditional_Info\t\t= 0x%.4x\n", data->ad.Additional_Info);
  printf("\tMNC_Length\t\t= %d\n\n", data->ad.MNC_Length);

  printf("IMSI:\n");
  printf("\tlength\t= %d\n", data->imsi.length);
  printf("\tparity\t= %s\n", data->imsi.u.num.parity == EVEN_PARITY ? "Even" : "Odd");
  digits = (data->imsi.length * 2) - 1 - (data->imsi.u.num.parity == EVEN_PARITY ? 1 : 0);
  printf("\tdigits\t= %d\n", digits);
  
  printf("\tdigits\t= %u%u%u%u%u%x%u%u%u%u",
         data->imsi.u.num.digit1, // MCC digit 1
         data->imsi.u.num.digit2, // MCC digit 2
         data->imsi.u.num.digit3, // MCC digit 3
         data->imsi.u.num.digit4, // MNC digit 1
         data->imsi.u.num.digit5, // MNC digit 2
         data->imsi.u.num.digit6==0xf?0:data->imsi.u.num.digit6, // MNC digit 3
         data->imsi.u.num.digit7,
         data->imsi.u.num.digit8,
         data->imsi.u.num.digit9,
         data->imsi.u.num.digit10);

  if (digits >= 11)
    printf("%x", data->imsi.u.num.digit11);

  if (digits >= 12)
    printf("%x", data->imsi.u.num.digit12);

  if (digits >= 13)
    printf("%x", data->imsi.u.num.digit13);

  if (digits >= 14)
    printf("%x", data->imsi.u.num.digit14);

  if (digits >= 15)
    printf("%x", data->imsi.u.num.digit15);

  printf("\n\n");

  printf("Ciphering and Integrity Keys:\n");
  printf("\tKSI\t: 0x%.2x\n", data->keys.ksi);
  char key[USIM_CK_SIZE + 1];
  key[USIM_CK_SIZE] = '\0';
  memcpy(key, data->keys.ck, USIM_CK_SIZE);
  printf("\tCK\t: \"%s\"\n", key);
  memcpy(key, data->keys.ik, USIM_IK_SIZE);
  printf("\tIK\t: \"%s\"\n", key);

  printf("EPS NAS security context:\n");
  printf("\tKSIasme\t: 0x%.2x\n", data->securityctx.KSIasme.value[0]);
  char kasme[USIM_K_ASME_SIZE + 1];
  kasme[USIM_K_ASME_SIZE] = '\0';
  memcpy(kasme, data->securityctx.Kasme.value, USIM_K_ASME_SIZE);
  printf("\tKasme\t: \"%s\"\n", kasme);
  printf("\tulNAScount\t: 0x%.8x\n",
         *(uint32_t*)data->securityctx.ulNAScount.value);
  printf("\tdlNAScount\t: 0x%.8x\n",
         *(uint32_t*)data->securityctx.dlNAScount.value);
  printf("\talgorithmID\t: 0x%.2x\n\n",
         data->securityctx.algorithmID.value[0]);

  printf("MSISDN\t= %u%u%u %u%u%u%u %u%u%u%u\n\n",
         data->msisdn.number.digit[0].msb,
         data->msisdn.number.digit[0].lsb,
         data->msisdn.number.digit[1].msb,
         data->msisdn.number.digit[1].lsb,
         data->msisdn.number.digit[2].msb,
         data->msisdn.number.digit[2].lsb,
         data->msisdn.number.digit[3].msb,
         data->msisdn.number.digit[3].lsb,
         data->msisdn.number.digit[4].msb,
         data->msisdn.number.digit[4].lsb,
         data->msisdn.number.digit[5].msb);

  for (int i = 0; i < USIM_PNN_MAX; i++) {
    printf("PNN[%d]\t= {%s, %s}\n", i,
           data->pnn[i].fullname.value, data->pnn[i].shortname.value);
  }

  printf("\n");

  for (int i = 0; i < USIM_OPL_MAX; i++) {
    printf("OPL[%d]\t= ", i);
    PRINT_PLMN(data->opl[i].plmn);
    printf(", TAC = [%.4x - %.4x], record_id = %d\n",
           data->opl[i].start, data->opl[i].end, data->opl[i].record_id);
  }

  printf("\n");

  printf("HPLMN\t\t= ");
  PRINT_PLMN(data->hplmn.plmn);
  printf(", AcT = 0x%x\n\n", data->hplmn.AcT);

  for (int i = 0; i < USIM_FPLMN_MAX; i++) {
    printf("FPLMN[%d]\t= ", i);
    PRINT_PLMN(data->fplmn[i]);
    printf("\n");
  }

  printf("\n");

  for (int i = 0; i < USIM_EHPLMN_MAX; i++) {
    printf("EHPLMN[%d]\t= ", i);
    PRINT_PLMN(data->ehplmn[i]);
    printf("\n");
  }

  printf("\n");

  for (int i = 0; i < USIM_PLMN_MAX; i++) {
    printf("PLMN[%d]\t\t= ", i);
    PRINT_PLMN(data->plmn[i].plmn);
    printf(", AcTPLMN = 0x%x", data->plmn[i].AcT);
    printf("\n");
  }

  printf("\n");

  for (int i = 0; i < USIM_OPLMN_MAX; i++) {
    printf("OPLMN[%d]\t= ", i);
    PRINT_PLMN(data->oplmn[i].plmn);
    printf(", AcTPLMN = 0x%x", data->oplmn[i].AcT);
    printf("\n");
  }

  printf("\n");

  printf("HPPLMN\t\t= 0x%.2x (%d minutes)\n\n", data->hpplmn, data->hpplmn);

  printf("LOCI:\n");
  printf("\tTMSI = 0x%.4x\n", data->loci.tmsi);
  printf("\tLAI\t: PLMN = ");
  PRINT_PLMN(data->loci.lai.plmn);
  printf(", LAC = 0x%.2x\n", data->loci.lai.lac);
  printf("\tstatus\t= %d\n\n", data->loci.status);

  printf("PSLOCI:\n");
  printf("\tP-TMSI = 0x%.4x\n", data->psloci.p_tmsi);
  printf("\tsignature = 0x%x 0x%x 0x%x\n",
         data->psloci.signature[0],
         data->psloci.signature[1],
         data->psloci.signature[2]);
  printf("\tRAI\t: PLMN = ");
  PRINT_PLMN(data->psloci.rai.plmn);
  printf(", LAC = 0x%.2x, RAC = 0x%.1x\n",
         data->psloci.rai.lac, data->psloci.rai.rac);
  printf("\tstatus\t= %d\n\n", data->psloci.status);

  printf("EPSLOCI:\n");
  printf("\tGUTI\t: GUMMEI\t: (PLMN = ");
  PRINT_PLMN(data->epsloci.guti.gummei.plmn);
  printf(", MMEgid = 0x%.2x, MMEcode = 0x%.1x)",
         data->epsloci.guti.gummei.MMEgid,
         data->epsloci.guti.gummei.MMEcode);
  printf(", M-TMSI = 0x%.4x\n", data->epsloci.guti.m_tmsi);
  printf("\tTAI\t: PLMN = ");
  PRINT_PLMN(data->epsloci.tai.plmn);
  printf(", TAC = 0x%.2x\n",
         data->epsloci.tai.tac);
  printf("\tstatus\t= %d\n\n", data->epsloci.status);

  printf("NASCONFIG:\n");
  printf("\tNAS_SignallingPriority\t\t: 0x%.2x\n",
         data->nasconfig.NAS_SignallingPriority.value[0]);
  printf("\tNMO_I_Behaviour\t\t\t: 0x%.2x\n",
         data->nasconfig.NMO_I_Behaviour.value[0]);
  printf("\tAttachWithImsi\t\t\t: 0x%.2x\n",
         data->nasconfig.AttachWithImsi.value[0]);
  printf("\tMinimumPeriodicSearchTimer\t: 0x%.2x\n",
         data->nasconfig.MinimumPeriodicSearchTimer.value[0]);
  printf("\tExtendedAccessBarring\t\t: 0x%.2x\n",
         data->nasconfig.ExtendedAccessBarring.value[0]);
  printf("\tTimer_T3245_Behaviour\t\t: 0x%.2x\n",
         data->nasconfig.Timer_T3245_Behaviour.value[0]);
}

