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
#include <pthread.h>
#include <stdint.h>

#include "mme_default_values.h"

#ifndef MME_CONFIG_H_
#define MME_CONFIG_H_

#define MME_CONFIG_STRING_MME_CONFIG                     "MME"
#define MME_CONFIG_STRING_REALM                          "REALM"
#define MME_CONFIG_STRING_MAXENB                         "MAXENB"
#define MME_CONFIG_STRING_MAXUE                          "MAXUE"
#define MME_CONFIG_STRING_RELATIVE_CAPACITY              "RELATIVE_CAPACITY"
#define MME_CONFIG_STRING_STATISTIC_TIMER                "MME_STATISTIC_TIMER"
#define MME_CONFIG_STRING_EMERGENCY_ATTACH_SUPPORTED     "EMERGENCY_ATTACH_SUPPORTED"
#define MME_CONFIG_STRING_UNAUTHENTICATED_IMSI_SUPPORTED "UNAUTHENTICATED_IMSI_SUPPORTED"

#define MME_CONFIG_STRING_INTERTASK_INTERFACE_CONFIG     "INTERTASK_INTERFACE"
#define MME_CONFIG_STRING_INTERTASK_INTERFACE_QUEUE_SIZE "ITTI_QUEUE_SIZE"

#define MME_CONFIG_STRING_S6A_CONFIG                     "S6A"
#define MME_CONFIG_STRING_S6A_CONF_FILE_PATH             "S6A_CONF"
#define MME_CONFIG_STRING_S6A_HSS_HOSTNAME               "HSS_HOSTNAME"

#define MME_CONFIG_STRING_SCTP_CONFIG                    "SCTP"
#define MME_CONFIG_STRING_SCTP_INSTREAMS                 "SCTP_INSTREAMS"
#define MME_CONFIG_STRING_SCTP_OUTSTREAMS                "SCTP_OUTSTREAMS"


#define MME_CONFIG_STRING_S1AP_CONFIG                    "S1AP"
#define MME_CONFIG_STRING_S1AP_OUTCOME_TIMER             "S1AP_OUTCOME_TIMER"
#define MME_CONFIG_STRING_S1AP_PORT                      "S1AP_PORT"

#define MME_CONFIG_STRING_GUMMEI_CONFIG                  "GUMMEI"
#define MME_CONFIG_STRING_MME_CODE                       "MME_CODE"
#define MME_CONFIG_STRING_MME_GID                        "MME_GID"
#define MME_CONFIG_STRING_TAI_LIST                       "TAI_LIST"
#define MME_CONFIG_STRING_MCC                            "MCC"
#define MME_CONFIG_STRING_MNC                            "MNC"
#define MME_CONFIG_STRING_TAC                            "TAC"

#define MME_CONFIG_STRING_NETWORK_INTERFACES_CONFIG      "NETWORK_INTERFACES"
#define MME_CONFIG_STRING_INTERFACE_NAME_FOR_S1_MME      "MME_INTERFACE_NAME_FOR_S1_MME"
#define MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S1_MME        "MME_IPV4_ADDRESS_FOR_S1_MME"
#define MME_CONFIG_STRING_INTERFACE_NAME_FOR_S11_MME     "MME_INTERFACE_NAME_FOR_S11_MME"
#define MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S11_MME       "MME_IPV4_ADDRESS_FOR_S11_MME"

#define MME_CONFIG_STRING_ASN1_VERBOSITY                      "VERBOSITY"
#define MME_CONFIG_STRING_ASN1_VERBOSITY_NONE                 "none"
#define MME_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING             "annoying"
#define MME_CONFIG_STRING_ASN1_VERBOSITY_INFO                 "info"

#define MME_CONFIG_STRING_NAS_CONFIG                    "NAS"
#define MME_CONFIG_STRING_NAS_SUPPORTED_INTEGRITY_ALGORITHM_LIST  "ORDERED_SUPPORTED_INTEGRITY_ALGORITHM_LIST"
#define MME_CONFIG_STRING_NAS_SUPPORTED_CIPHERING_ALGORITHM_LIST  "ORDERED_SUPPORTED_CIPHERING_ALGORITHM_LIST"
#define NAS_CONFIG_SECURITY_ALGORITHMS_EEA0    0b000
#define NAS_CONFIG_SECURITY_ALGORITHMS_EEA1    0b001
#define NAS_CONFIG_SECURITY_ALGORITHMS_EEA2    0b010
#define NAS_CONFIG_SECURITY_ALGORITHMS_EEA3    0b011
#define NAS_CONFIG_SECURITY_ALGORITHMS_EEA4    0b100
#define NAS_CONFIG_SECURITY_ALGORITHMS_EEA5    0b101
#define NAS_CONFIG_SECURITY_ALGORITHMS_EEA6    0b110
#define NAS_CONFIG_SECURITY_ALGORITHMS_EEA7    0b111
#define NAS_CONFIG_SECURITY_ALGORITHMS_EIA0    0b000
#define NAS_CONFIG_SECURITY_ALGORITHMS_EIA1    0b001
#define NAS_CONFIG_SECURITY_ALGORITHMS_EIA2    0b010
#define NAS_CONFIG_SECURITY_ALGORITHMS_EIA3    0b011
#define NAS_CONFIG_SECURITY_ALGORITHMS_EIA4    0b100
#define NAS_CONFIG_SECURITY_ALGORITHMS_EIA5    0b101
#define NAS_CONFIG_SECURITY_ALGORITHMS_EIA6    0b110
#define NAS_CONFIG_SECURITY_ALGORITHMS_EIA7    0b111



typedef struct mme_config_s {
  /* Reader/writer lock for this configuration */
  pthread_rwlock_t rw_lock;

  uint8_t verbosity_level;

  char *config_file;
  char *realm;
  int   realm_length;

  uint32_t max_eNBs;
  uint32_t max_ues;

  uint8_t relative_capacity;

  uint32_t mme_statistic_timer;

  uint8_t emergency_attach_supported;
  uint8_t unauthenticated_imsi_supported;

  struct {
    uint16_t  nb_mme_gid;
    uint16_t *mme_gid;

    uint16_t  nb_mmec;
    uint8_t  *mmec;

    uint8_t   nb_plmns;
    uint16_t *plmn_mcc;
    uint16_t *plmn_mnc;
    uint16_t *plmn_mnc_len;
    uint16_t *plmn_tac;
  } gummei;

  struct {
    uint16_t in_streams;
    uint16_t out_streams;
  } sctp_config;
  struct {
    uint16_t port_number;
  } gtpv1u_config;
  struct {
    uint16_t port_number;
    uint8_t  outcome_drop_timer_sec;
  } s1ap_config;
  struct {
    uint32_t  sgw_ip_address_for_S1u_S12_S4_up;

    char     *mme_interface_name_for_S1_MME;
    uint32_t  mme_ip_address_for_S1_MME;

    char     *mme_interface_name_for_S11;
    uint32_t  mme_ip_address_for_S11;

    uint32_t  sgw_ip_address_for_S11;
  } ipv4;
  struct {
    char *conf_file;
    char *hss_host_name;
  } s6a_config;
  struct {
    uint32_t  queue_size;
    char     *log_file;
  } itti_config;

  struct {
    uint8_t prefered_integrity_algorithm[8];
    uint8_t prefered_ciphering_algorithm[8];
  } nas_config;



} mme_config_t;

extern mme_config_t mme_config;

int mme_config_find_mnc_length(const char mcc_digit1P,
                               const char mcc_digit2P,
                               const char mcc_digit3P,
                               const char mnc_digit1P,
                               const char mnc_digit2P,
                               const char mnc_digit3P);
int config_parse_opt_line(int argc, char *argv[], mme_config_t *mme_config);

#define config_read_lock(mMEcONFIG)  pthread_rwlock_rdlock(&(mMEcONFIG)->rw_lock)
#define config_write_lock(mMEcONFIG) pthread_rwlock_wrlock(&(mMEcONFIG)->rw_lock)
#define config_unlock(mMEcONFIG)     pthread_rwlock_unlock(&(mMEcONFIG)->rw_lock)

//int yyparse(struct mme_config_s *mme_config_p);

#endif /* MME_CONFIG_H_ */
