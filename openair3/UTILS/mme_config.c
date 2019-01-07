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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <libconfig.h>

#include <arpa/inet.h> /* To provide inet_addr */

#include "intertask_interface.h"
#include "assertions.h"
#include "msc.h"
#include "mme_config.h"
#include "spgw_config.h"
#include "intertask_interface_conf.h"

mme_config_t mme_config;

int mme_config_find_mnc_length(const char mcc_digit1P,
                               const char mcc_digit2P,
                               const char mcc_digit3P,
                               const char mnc_digit1P,
                               const char mnc_digit2P,
                               const char mnc_digit3P)
{

  uint16_t mcc = 100*mcc_digit1P + 10*mcc_digit2P + mcc_digit3P;
  uint16_t mnc3= 100*mnc_digit1P + 10*mnc_digit2P + mnc_digit3P;
  uint16_t mnc2=                   10*mnc_digit1P + mnc_digit2P;
  int  plmn_index = 0;

  AssertFatal((mcc_digit1P >= 0) && (mcc_digit1P <= 9)
              && (mcc_digit2P >= 0) && (mcc_digit2P <= 9)
              && (mcc_digit3P >= 0) && (mcc_digit3P <= 9) ,
              "BAD MCC PARAMETER (%d%d%d)!\n",
              mcc_digit1P, mcc_digit2P, mcc_digit3P);

  AssertFatal((mnc_digit2P >= 0) && (mnc_digit2P <= 9)
              && (mnc_digit1P >= 0) && (mnc_digit1P <= 9) ,
              "BAD MNC PARAMETER (%d.%d.%d)!\n",
              mnc_digit1P, mnc_digit2P, mnc_digit3P);


  while (plmn_index < mme_config.gummei.nb_plmns) {
    if (mme_config.gummei.plmn_mcc[plmn_index] == mcc) {
      if ((mme_config.gummei.plmn_mnc[plmn_index] == mnc2) &&
          (mme_config.gummei.plmn_mnc_len[plmn_index] == 2)) {
        return 2;
      } else if ((mme_config.gummei.plmn_mnc[plmn_index] == mnc3) &&
                 (mme_config.gummei.plmn_mnc_len[plmn_index] == 3)) {
        return 3;
      }
    }

    plmn_index += 1;
  }

  return 0;
}


static
void mme_config_init(mme_config_t *mme_config_p)
{
  memset(mme_config_p, 0, sizeof(mme_config_t));

  pthread_rwlock_init(&mme_config_p->rw_lock, NULL);

  mme_config_p->verbosity_level            = 0;
  mme_config_p->config_file                = NULL;
  mme_config_p->max_eNBs                   = MAX_NUMBER_OF_ENB;
  mme_config_p->max_ues                    = MAX_NUMBER_OF_UE;

  mme_config_p->emergency_attach_supported     = 0;
  mme_config_p->unauthenticated_imsi_supported = 0;

  /* Timer configuration */
  mme_config_p->gtpv1u_config.port_number = GTPV1_U_PORT_NUMBER;
  mme_config_p->s1ap_config.port_number   = S1AP_PORT_NUMBER;
  /* IP configuration */
  mme_config_p->ipv4.sgw_ip_address_for_S1u_S12_S4_up     = inet_addr(DEFAULT_SGW_IP_ADDRESS_FOR_S1U_S12_S4_UP);

  mme_config_p->ipv4.mme_interface_name_for_S1_MME        = DEFAULT_MME_INTERFACE_NAME_FOR_S1_MME;
  mme_config_p->ipv4.mme_ip_address_for_S1_MME            = inet_addr(DEFAULT_MME_IP_ADDRESS_FOR_S1_MME);

  mme_config_p->ipv4.mme_interface_name_for_S11           = DEFAULT_MME_INTERFACE_NAME_FOR_S11;
  mme_config_p->ipv4.mme_ip_address_for_S11               = inet_addr(DEFAULT_MME_IP_ADDRESS_FOR_S11);

  mme_config_p->ipv4.sgw_ip_address_for_S11               = inet_addr(DEFAULT_SGW_IP_ADDRESS_FOR_S11);

  mme_config_p->s6a_config.conf_file    = S6A_CONF_FILE;

  mme_config_p->itti_config.queue_size  = ITTI_QUEUE_MAX_ELEMENTS;
  mme_config_p->itti_config.log_file    = NULL;

  mme_config_p->sctp_config.in_streams  = SCTP_IN_STREAMS;
  mme_config_p->sctp_config.out_streams = SCTP_OUT_STREAMS;

  mme_config_p->relative_capacity = RELATIVE_CAPACITY;

  mme_config_p->mme_statistic_timer = MME_STATISTIC_TIMER_S;

  mme_config_p->gummei.nb_mme_gid = 1;
  mme_config_p->gummei.mme_gid    = calloc(1, sizeof(*mme_config_p->gummei.mme_gid));
  mme_config_p->gummei.mme_gid[0] = MMEGID;
  mme_config_p->gummei.nb_mmec    = 1;
  mme_config_p->gummei.mmec       = calloc(1, sizeof(*mme_config_p->gummei.mmec));
  mme_config_p->gummei.mmec[0]    = MMEC;

  /* Set the TAI */
  mme_config_p->gummei.nb_plmns   = 1;
  mme_config_p->gummei.plmn_mcc   = calloc(1, sizeof(*mme_config_p->gummei.plmn_mcc));
  mme_config_p->gummei.plmn_mnc   = calloc(1, sizeof(*mme_config_p->gummei.plmn_mnc));
  mme_config_p->gummei.plmn_mnc_len   = calloc(1, sizeof(*mme_config_p->gummei.plmn_mnc_len));
  mme_config_p->gummei.plmn_tac   = calloc(1, sizeof(*mme_config_p->gummei.plmn_tac));


  mme_config_p->gummei.plmn_mcc[0]     = PLMN_MCC;
  mme_config_p->gummei.plmn_mnc[0]     = PLMN_MNC;
  mme_config_p->gummei.plmn_mnc_len[0] = PLMN_MNC_LEN;
  mme_config_p->gummei.plmn_tac[0]     = PLMN_TAC;

  mme_config_p->s1ap_config.outcome_drop_timer_sec = S1AP_OUTCOME_TIMER_DEFAULT;
}

int mme_system(char *command_pP, int abort_on_errorP)
{
  int ret = -1;

  if (command_pP) {
    fprintf(stdout, "system command: %s\n",command_pP);
    ret = system(command_pP);

    if (ret < 0) {
      fprintf(stderr, "ERROR in system command %s: %d\n",
              command_pP,ret);

      if (abort_on_errorP) {
        exit(-1); // may be not exit
      }
    }
  }

  return ret;
}


static int config_parse_file(mme_config_t *mme_config_p)
{
  config_t          cfg;
  config_setting_t *setting_mme                      = NULL;
  config_setting_t *setting                          = NULL;
  config_setting_t *subsetting                       = NULL;
  config_setting_t *sub2setting                      = NULL;

  long int         alongint;
  int              i, num;
  char             *astring                          = NULL;
  char             *address                          = NULL;
  char             *cidr                             = NULL;

  const char*       tac                              = NULL;
  const char*       mcc                              = NULL;
  const char*       mnc                              = NULL;

  char             *sgw_ip_address_for_S1u_S12_S4_up = NULL;
  char             *mme_interface_name_for_S1_MME    = NULL;
  char             *mme_ip_address_for_S1_MME        = NULL;
  char             *mme_interface_name_for_S11       = NULL;
  char             *mme_ip_address_for_S11           = NULL;
  char             *sgw_ip_address_for_S11           = NULL;
  char                system_cmd[256];

  config_init(&cfg);

  if(mme_config_p->config_file != NULL) {
    /* Read the file. If there is an error, report it and exit. */
    if(! config_read_file(&cfg, mme_config_p->config_file)) {
      fprintf(stdout, "ERROR: %s:%d - %s\n", mme_config_p->config_file, config_error_line(&cfg), config_error_text(&cfg));
      config_destroy(&cfg);
      AssertFatal (1 == 0, "Failed to parse MME configuration file %s!\n", mme_config_p->config_file);
    }
  } else {
    fprintf(stdout, "ERROR No MME configuration file provided!\n");
    config_destroy(&cfg);
    AssertFatal (0, "No MME configuration file provided!\n");
  }

  setting_mme = config_lookup(&cfg, MME_CONFIG_STRING_MME_CONFIG);

  if(setting_mme != NULL) {
    // GENERAL MME SETTINGS
    if(  (config_setting_lookup_string( setting_mme, MME_CONFIG_STRING_REALM, (const char **)&astring) )) {
      mme_config_p->realm = strdup(astring);
      mme_config_p->realm_length = strlen(mme_config_p->realm);
    }

    if(  (config_setting_lookup_int( setting_mme, MME_CONFIG_STRING_MAXENB, &alongint) )) {
      mme_config_p->max_eNBs = (uint32_t)alongint;
    }

    if(  (config_setting_lookup_int( setting_mme, MME_CONFIG_STRING_MAXUE, &alongint) )) {
      mme_config_p->max_ues = (uint32_t)alongint;
    }

    if(  (config_setting_lookup_int( setting_mme, MME_CONFIG_STRING_RELATIVE_CAPACITY, &alongint) )) {
      mme_config_p->relative_capacity = (uint8_t)alongint;
    }

    if(  (config_setting_lookup_int( setting_mme, MME_CONFIG_STRING_STATISTIC_TIMER, &alongint) )) {
      mme_config_p->mme_statistic_timer = (uint32_t)alongint;
    }

    if(  (config_setting_lookup_string( setting_mme, MME_CONFIG_STRING_EMERGENCY_ATTACH_SUPPORTED, (const char **)&astring) )) {
      if (strcasecmp(astring , "yes") == 0)
        mme_config_p->emergency_attach_supported = 1;
      else
        mme_config_p->emergency_attach_supported = 0;
    }

    if(  (config_setting_lookup_string( setting_mme, MME_CONFIG_STRING_UNAUTHENTICATED_IMSI_SUPPORTED, (const char **)&astring) )) {
      if (strcasecmp(astring , "yes") == 0)
        mme_config_p->unauthenticated_imsi_supported = 1;
      else
        mme_config_p->unauthenticated_imsi_supported = 0;
    }

    if(  (config_setting_lookup_string( setting_mme, MME_CONFIG_STRING_ASN1_VERBOSITY, (const char **)&astring) )) {
      if (strcasecmp(astring , MME_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0)
        mme_config_p->verbosity_level = 0;
      else if (strcasecmp(astring , MME_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0)
        mme_config_p->verbosity_level = 2;
      else if (strcasecmp(astring , MME_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0)
        mme_config_p->verbosity_level = 1;
      else
        mme_config_p->verbosity_level = 0;
    }

    // ITTI SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_INTERTASK_INTERFACE_CONFIG);

    if (setting != NULL) {
      if(  (config_setting_lookup_int( setting, MME_CONFIG_STRING_INTERTASK_INTERFACE_QUEUE_SIZE, &alongint) )) {
        mme_config_p->itti_config.queue_size = (uint32_t)alongint;
      }
    }

    // S6A SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_S6A_CONFIG);

    if (setting != NULL) {
      if(  (config_setting_lookup_string( setting, MME_CONFIG_STRING_S6A_CONF_FILE_PATH, (const char **)&astring) )) {
        if (astring != NULL)
          mme_config_p->s6a_config.conf_file = strdup(astring);
      }

      if(  (config_setting_lookup_string( setting, MME_CONFIG_STRING_S6A_HSS_HOSTNAME, (const char **)&astring) )) {
        if (astring != NULL)
          mme_config_p->s6a_config.hss_host_name  = strdup(astring);
        else
          AssertFatal (1 == 0,
                       "You have to provide a valid HSS hostname %s=...\n",
                       MME_CONFIG_STRING_S6A_HSS_HOSTNAME);
      }
    }

    // SCTP SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_SCTP_CONFIG);

    if (setting != NULL) {
      if(  (config_setting_lookup_int( setting, MME_CONFIG_STRING_SCTP_INSTREAMS, &alongint) )) {
        mme_config_p->sctp_config.in_streams = (uint16_t)alongint;
      }

      if(  (config_setting_lookup_int( setting, MME_CONFIG_STRING_SCTP_OUTSTREAMS, &alongint) )) {
        mme_config_p->sctp_config.out_streams = (uint16_t)alongint;
      }
    }

    // S1AP SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_S1AP_CONFIG);

    if (setting != NULL) {
      if(  (config_setting_lookup_int( setting, MME_CONFIG_STRING_S1AP_OUTCOME_TIMER, &alongint) )) {
        mme_config_p->s1ap_config.outcome_drop_timer_sec = (uint8_t)alongint;
      }

      if(  (config_setting_lookup_int( setting, MME_CONFIG_STRING_S1AP_PORT, &alongint) )) {
        mme_config_p->s1ap_config.port_number = (uint16_t)alongint;
      }
    }

    // GUMMEI SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_GUMMEI_CONFIG);

    if (setting != NULL) {
      subsetting = config_setting_get_member (setting, MME_CONFIG_STRING_MME_CODE);

      if (subsetting != NULL) {
        num     = config_setting_length(subsetting);

        if (mme_config_p->gummei.nb_mmec != num) {
          if (mme_config_p->gummei.mmec != NULL) {
            free(mme_config_p->gummei.mmec);
          }

          mme_config_p->gummei.mmec = calloc(num, sizeof(*mme_config_p->gummei.mmec));
        }

        mme_config_p->gummei.nb_mmec = num;

        for (i = 0; i < num; i++) {
          mme_config_p->gummei.mmec[i] = config_setting_get_int_elem(subsetting, i);
        }
      }

      subsetting = config_setting_get_member (setting, MME_CONFIG_STRING_MME_GID);

      if (subsetting != NULL) {
        num     = config_setting_length(subsetting);

        if (mme_config_p->gummei.nb_mme_gid != num) {
          if (mme_config_p->gummei.mme_gid != NULL) {
            free(mme_config_p->gummei.mme_gid);
          }

          mme_config_p->gummei.mme_gid = calloc(num, sizeof(*mme_config_p->gummei.mme_gid));
        }

        mme_config_p->gummei.nb_mme_gid = num;

        for (i = 0; i < num; i++) {
          mme_config_p->gummei.mme_gid[i] = config_setting_get_int_elem(subsetting, i);
        }
      }

      subsetting = config_setting_get_member (setting, MME_CONFIG_STRING_TAI_LIST);

      if (subsetting != NULL) {
        num     = config_setting_length(subsetting);

        if (mme_config_p->gummei.nb_plmns != num) {
          if (mme_config_p->gummei.plmn_mcc != NULL)     free(mme_config_p->gummei.plmn_mcc);

          if (mme_config_p->gummei.plmn_mnc != NULL)     free(mme_config_p->gummei.plmn_mnc);

          if (mme_config_p->gummei.plmn_mnc_len != NULL) free(mme_config_p->gummei.plmn_mnc_len);

          if (mme_config_p->gummei.plmn_tac != NULL)     free(mme_config_p->gummei.plmn_tac);

          mme_config_p->gummei.plmn_mcc     = calloc(num, sizeof(*mme_config_p->gummei.plmn_mcc));
          mme_config_p->gummei.plmn_mnc     = calloc(num, sizeof(*mme_config_p->gummei.plmn_mnc));
          mme_config_p->gummei.plmn_mnc_len = calloc(num, sizeof(*mme_config_p->gummei.plmn_mnc_len));
          mme_config_p->gummei.plmn_tac     = calloc(num, sizeof(*mme_config_p->gummei.plmn_tac));
        }

        mme_config_p->gummei.nb_plmns = num;

        for (i = 0; i < num; i++) {
          sub2setting =  config_setting_get_elem(subsetting, i);

          if (sub2setting != NULL) {
            if(  (config_setting_lookup_string( sub2setting, MME_CONFIG_STRING_MCC, &mcc) )) {
              mme_config_p->gummei.plmn_mcc[i] = (uint16_t)atoi(mcc);
            }

            if(  (config_setting_lookup_string( sub2setting, MME_CONFIG_STRING_MNC, &mnc) )) {
              mme_config_p->gummei.plmn_mnc[i] = (uint16_t)atoi(mnc);
              mme_config_p->gummei.plmn_mnc_len[i] = strlen(mnc);
              AssertFatal((mme_config_p->gummei.plmn_mnc_len[i] == 2) || (mme_config_p->gummei.plmn_mnc_len[i] == 3),
                          "Bad MNC length %u, must be 2 or 3", mme_config_p->gummei.plmn_mnc_len[i]);
            }

            if(  (config_setting_lookup_string( sub2setting, MME_CONFIG_STRING_TAC, &tac) )) {
              mme_config_p->gummei.plmn_tac[i] = (uint16_t)atoi(tac);
              AssertFatal(mme_config_p->gummei.plmn_tac[i] != 0,
                          "TAC must not be 0");
            }
          }
        }
      }
    }

    // NETWORK INTERFACE SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

    if(setting != NULL) {
      if(  (
             config_setting_lookup_string( setting, MME_CONFIG_STRING_INTERFACE_NAME_FOR_S1_MME,
                                           (const char **)&mme_interface_name_for_S1_MME)
             && config_setting_lookup_string( setting, MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S1_MME,
                                              (const char **)&mme_ip_address_for_S1_MME)
             && config_setting_lookup_string( setting, MME_CONFIG_STRING_INTERFACE_NAME_FOR_S11_MME,
                                              (const char **)&mme_interface_name_for_S11)
             && config_setting_lookup_string( setting, MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S11_MME,
                                              (const char **)&mme_ip_address_for_S11)
           )
        ) {
        mme_config_p->ipv4.mme_interface_name_for_S1_MME = strdup(mme_interface_name_for_S1_MME);
        cidr = strdup(mme_ip_address_for_S1_MME);
        address = strtok(cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO ( address, mme_config_p->ipv4.mme_ip_address_for_S1_MME, "BAD IP ADDRESS FORMAT FOR MME S1_MME !\n" )
        free(cidr);

        mme_config_p->ipv4.mme_interface_name_for_S11 = strdup(mme_interface_name_for_S11);
        cidr = strdup(mme_ip_address_for_S11);
        address = strtok(cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO ( address, mme_config_p->ipv4.mme_ip_address_for_S11, "BAD IP ADDRESS FORMAT FOR MME S11 !\n" )
        free(cidr);

        if (strncasecmp("tun",mme_config_p->ipv4.mme_interface_name_for_S1_MME, strlen("tun")) == 0) {
          if (snprintf(system_cmd, 256,
                       "ip link set %s down ;openvpn --rmtun --dev %s",
                       mme_config_p->ipv4.mme_interface_name_for_S1_MME,
                       mme_config_p->ipv4.mme_interface_name_for_S1_MME
                      ) > 0) {
            mme_system(system_cmd, 1);
          } else {
            fprintf(stderr, "Del %s\n", mme_config_p->ipv4.mme_interface_name_for_S1_MME);
          }

          if (snprintf(system_cmd, 256,
                       "openvpn --mktun --dev %s;sync;ifconfig  %s up;sync",
                       mme_config_p->ipv4.mme_interface_name_for_S1_MME,
                       mme_config_p->ipv4.mme_interface_name_for_S1_MME) > 0) {
            mme_system(system_cmd, 1);
          } else {
            fprintf(stderr, "Create %s\n", mme_config_p->ipv4.mme_interface_name_for_S1_MME);
          }

          if (snprintf(system_cmd, 256,
                       "ip -4 addr add %s  dev %s",
                       mme_ip_address_for_S1_MME,
                       mme_config_p->ipv4.mme_interface_name_for_S1_MME) > 0) {
            mme_system(system_cmd, 1);
          } else {
            fprintf(stderr, "Set IPv4 address on %s\n", mme_config_p->ipv4.mme_interface_name_for_S1_MME);
          }
        }

      }
    }

    // NAS SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_NAS_CONFIG);

    if (setting != NULL) {
      subsetting = config_setting_get_member (setting, MME_CONFIG_STRING_NAS_SUPPORTED_INTEGRITY_ALGORITHM_LIST);

      if (subsetting != NULL) {
        num     = config_setting_length(subsetting);

        if (num <= 8) {
          for (i = 0; i < num; i++) {
            astring = config_setting_get_string_elem(subsetting, i);

            if (strcmp("EIA0", astring) == 0) mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
            else if (strcmp("EIA1", astring) == 0) mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA1;
            else if (strcmp("EIA2", astring) == 0) mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA2;
            else if (strcmp("EIA3", astring) == 0) mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
            else if (strcmp("EIA4", astring) == 0) mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
            else if (strcmp("EIA5", astring) == 0) mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
            else if (strcmp("EIA6", astring) == 0) mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
            else if (strcmp("EIA7", astring) == 0) mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
          }

          for (i = num; i < 8; i++) {
            mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
          }
        }
      }

      subsetting = config_setting_get_member (setting, MME_CONFIG_STRING_NAS_SUPPORTED_CIPHERING_ALGORITHM_LIST);

      if (subsetting != NULL) {
        num     = config_setting_length(subsetting);

        if (num <= 8) {
          for (i = 0; i < num; i++) {
            astring = config_setting_get_string_elem(subsetting, i);

            if (strcmp("EEA0", astring) == 0) mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
            else if (strcmp("EEA1", astring) == 0) mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA1;
            else if (strcmp("EEA2", astring) == 0) mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA2;
            else if (strcmp("EEA3", astring) == 0) mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
            else if (strcmp("EEA4", astring) == 0) mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
            else if (strcmp("EEA5", astring) == 0) mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
            else if (strcmp("EEA6", astring) == 0) mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
            else if (strcmp("EEA7", astring) == 0) mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
          }

          for (i = num; i < 8; i++) {
            mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
          }
        }
      }

    }
  }

  setting = config_lookup(&cfg, SGW_CONFIG_STRING_SGW_CONFIG);

  if(setting != NULL) {
    subsetting = config_setting_get_member (setting, SGW_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

    if(subsetting != NULL) {
      if(  (
             config_setting_lookup_string( subsetting, SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP,
                                           (const char **)&sgw_ip_address_for_S1u_S12_S4_up)
             && config_setting_lookup_string( subsetting, SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S11,
                                              (const char **)&sgw_ip_address_for_S11)
             && config_setting_lookup_int( subsetting, SGW_CONFIG_STRING_SGW_PORT_FOR_S1U_S12_S4_UP, &alongint)
           )
        ) {
        cidr = strdup(sgw_ip_address_for_S1u_S12_S4_up);
        address = strtok(cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO ( address, mme_config_p->ipv4.sgw_ip_address_for_S1u_S12_S4_up, "BAD IP ADDRESS FORMAT FOR SGW S1u_S12_S4 !\n" )
        free(cidr);

        cidr = strdup(sgw_ip_address_for_S11);
        address = strtok(cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO ( address, mme_config_p->ipv4.sgw_ip_address_for_S11, "BAD IP ADDRESS FORMAT FOR SGW S11 !\n" )
        free(cidr);

        mme_config_p->gtpv1u_config.port_number = (uint16_t) alongint;
      }
    }
  }

  return 0;
}

#define DISPLAY_ARRAY(size, format, args...)                        \
do {                                                                \
    int i;                                                          \
    for (i = 0; i < size; i++) {                                    \
        fprintf(stdout, format, args);                              \
        if ((i != (size - 1)) && ((i + 1) % 10 == 0))               \
        {                                                           \
            fprintf(stdout, "\n        ");                          \
        }                                                           \
    }                                                               \
    if (i > 0)                                                      \
        fprintf(stdout, "\n");                                      \
} while(0)

static void config_display(mme_config_t *mme_config_p)
{
  int j;

  fprintf(stdout, "==== EURECOM %s v%s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  fprintf(stdout, "Configuration:\n");
  fprintf(stdout, "- File ...............: %s\n", mme_config_p->config_file);
  fprintf(stdout, "- Verbosity level ....: %d\n", mme_config_p->verbosity_level);
  fprintf(stdout, "- Realm ..............: %s\n", mme_config_p->realm);
  fprintf(stdout, "- Max eNBs ...........: %u\n", mme_config_p->max_eNBs);
  fprintf(stdout, "- Max UEs ............: %u\n", mme_config_p->max_ues);
  fprintf(stdout, "- Emergency support ..: %s\n", mme_config_p->emergency_attach_supported == 0 ? "FALSE" : "TRUE");
  fprintf(stdout, "- Unauth IMSI support : %s\n", mme_config_p->unauthenticated_imsi_supported == 0 ? "FALSE" : "TRUE");
  fprintf(stdout, "- Relative capa ......: %u\n\n", mme_config_p->relative_capacity);
  fprintf(stdout, "- Statistics timer ...: %u (seconds)\n\n", mme_config_p->mme_statistic_timer);
  fprintf(stdout, "- S1-U:\n");
  fprintf(stdout, "    port number ......: %d\n", mme_config_p->gtpv1u_config.port_number);
  fprintf(stdout, "- S1-MME:\n");
  fprintf(stdout, "    port number ......: %d\n", mme_config_p->s1ap_config.port_number);
  fprintf(stdout, "- IP:\n");
  //fprintf(stdout, "    s1-u iface .......: %s\n", mme_config_p->ipv4.sgw_interface_name_for_S1u_S12_S4_up);
  fprintf(stdout, "    s1-u ip ..........: %s\n",
          inet_ntoa(*((struct in_addr *)&mme_config_p->ipv4.sgw_ip_address_for_S1u_S12_S4_up)));
  //fprintf(stdout, "    sgi iface ........: %s\n", mme_config_p->ipv4.pgw_interface_name_for_SGI);
  //fprintf(stdout, "    sgi ip ...........: %s/%d\n",
  //        inet_ntoa(*((struct in_addr *)&mme_config_p->ipv4.pgw_ip_addr_for_SGI)),
  //        mme_config_p->ipv4.pgw_ip_netmask_for_SGI);
  fprintf(stdout, "    s1-MME iface .....: %s\n", mme_config_p->ipv4.mme_interface_name_for_S1_MME);
  fprintf(stdout, "    s1-MME ip ........: %s\n",
          inet_ntoa(*((struct in_addr *)&mme_config_p->ipv4.mme_ip_address_for_S1_MME)));
  //fprintf(stdout, "    s11 S-GW iface ...: %s\n", mme_config_p->ipv4.sgw_interface_name_for_S11);
  //fprintf(stdout, "    s11 S-GW ip ......: %s/%d\n",
  //        inet_ntoa(*((struct in_addr *)&mme_config_p->ipv4.sgw_ip_address_for_S11)),
  //        mme_config_p->ipv4.sgw_ip_netmask_for_S11);
  fprintf(stdout, "    s11 MME iface ....: %s\n", mme_config_p->ipv4.mme_interface_name_for_S11);
  fprintf(stdout, "    s11 S-GW ip ......: %s\n",
          inet_ntoa(*((struct in_addr *)&mme_config_p->ipv4.mme_ip_address_for_S11)));
  fprintf(stdout, "- ITTI:\n");
  fprintf(stdout, "    queue size .......: %u (bytes)\n", mme_config_p->itti_config.queue_size);
  fprintf(stdout, "    log file .........: %s\n", mme_config_p->itti_config.log_file);
  fprintf(stdout, "- SCTP:\n");
  fprintf(stdout, "    in streams .......: %u\n", mme_config_p->sctp_config.in_streams);
  fprintf(stdout, "    out streams ......: %u\n", mme_config_p->sctp_config.out_streams);
  fprintf(stdout, "- GUMMEI:\n");
  fprintf(stdout, "    mme group ids ....:\n        ");
  DISPLAY_ARRAY(mme_config_p->gummei.nb_mme_gid, "| %u ", mme_config_p->gummei.mme_gid[i]);
  fprintf(stdout, "    mme codes ........:\n        ");
  DISPLAY_ARRAY(mme_config_p->gummei.nb_mmec, "| %u ", mme_config_p->gummei.mmec[i]);
  fprintf(stdout, "    plmns ............: (mcc.mnc:tac)\n");

  for (j= 0; j < mme_config_p->gummei.nb_plmns; j++) {
    if (mme_config_p->gummei.plmn_mnc_len[j] ==2 ) {
      fprintf(stdout, "            %3u.%3u:%u\n",
              mme_config_p->gummei.plmn_mcc[j],
              mme_config_p->gummei.plmn_mnc[j],
              mme_config_p->gummei.plmn_tac[j]);
    } else {
      fprintf(stdout, "            %3u.%03u:%u\n",
              mme_config_p->gummei.plmn_mcc[j],
              mme_config_p->gummei.plmn_mnc[j],
              mme_config_p->gummei.plmn_tac[j]);
    }
  }

  fprintf(stdout, "- S6A:\n");
  fprintf(stdout, "    conf file ........: %s\n", mme_config_p->s6a_config.conf_file);
}

static void usage(void)
{
  fprintf(stdout, "==== EURECOM %s v%s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  fprintf(stdout, "Please report any bug to: %s\n\n", PACKAGE_BUGREPORT);
  fprintf(stdout, "Usage: oaisim_mme [options]\n\n");
  fprintf(stdout, "Available options:\n");
  fprintf(stdout, "-h      Print this help and return\n");
  fprintf(stdout, "-c<path>\n");
  fprintf(stdout, "        Set the configuration file for mme\n");
  fprintf(stdout, "        See template in UTILS/CONF\n");
  fprintf(stdout, "-K<file>\n");
  fprintf(stdout, "        Output intertask messages to provided file\n");
  fprintf(stdout, "-V      Print %s version and return\n", PACKAGE_NAME);
  fprintf(stdout, "-v[1-2] Debug level:\n");
  fprintf(stdout, "            1 -> ASN1 XER printf on and ASN1 debug off\n");
  fprintf(stdout, "            2 -> ASN1 XER printf on and ASN1 debug on\n");
}

extern void
nwGtpv1uDisplayBanner(void);

int config_parse_opt_line(int argc, char *argv[], mme_config_t *mme_config_p)
{
  int c;
  mme_config_init(mme_config_p);

  /* Parsing command line */
  while ((c = getopt (argc, argv, "O:c:hi:K:v:V")) != -1) {
    switch (c) {
    case 'O':
    case 'c': {
      /* Store the given configuration file. If no file is given,
       * then the default values will be used.
       */
      int config_file_len = 0;
      config_file_len = strlen(optarg);
      mme_config_p->config_file = malloc(sizeof(char) * (config_file_len + 1));
      memcpy(mme_config_p->config_file, optarg, config_file_len);
      mme_config_p->config_file[config_file_len] = '\0';
    }
    break;

    case 'v': {
      mme_config_p->verbosity_level = atoi(optarg);
    }
    break;

    case 'V': {
      fprintf(stdout, "==== EURECOM %s v%s ====\n"
              "Please report any bug to: %s\n", PACKAGE_NAME, PACKAGE_VERSION,
              PACKAGE_BUGREPORT);
      exit(0);
      nwGtpv1uDisplayBanner();
    }
    break;

    case 'K':
      mme_config_p->itti_config.log_file = strdup(optarg);
      break;

    case 'h': /* Fall through */
    default:
      usage();
      exit(0);
    }
  }

  /* Parse the configuration file using libconfig */
  if (config_parse_file(mme_config_p) != 0) {
    return -1;
  }

  /* Display the configuration */
  config_display(mme_config_p);
  return 0;
}
