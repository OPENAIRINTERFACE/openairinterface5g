#ifndef _CONF_USIM_H
#define _CONF_USIM_H

#include <libconfig.h>
#include "usim_api.h"
#include "conf_user_plmn.h"

#define SIM "SIM"
#define MSIN "MSIN"
#define USIM_API_K "USIM_API_K"
#define OPC "OPC"
#define MSISDN "MSISDN"

#define KSI               USIM_KSI_NOT_AVAILABLE
#define KSI_ASME          USIM_KSI_NOT_AVAILABLE

#define OPC_SIZE          16

#define DEFAULT_TMSI      0x0000000D
#define DEFAULT_P_TMSI    0x0000000D
#define DEFAULT_M_TMSI    0x0000000D

#define DEFAULT_RAC       0x01
#define DEFAULT_TAC       0x0001
#define DEFAULT_LAC       0xFFFE

#define DEFAULT_MME_ID    0x0102
#define DEFAULT_MME_CODE  0x0F

// TODO add this setting in configuration file
#define INT_ALGO          USIM_INT_EIA2
#define ENC_ALGO          USIM_ENC_EEA0
#define SECURITY_ALGORITHMS (ENC_ALGO | INT_ALGO)

typedef struct {
    const char *msin;
    const char *usim_api_k;
    const char *msisdn;
    const char *opc;
    const char *hplmn;
} usim_data_conf_t;

bool parse_ue_sim_param(config_setting_t *ue_setting, int user_id, usim_data_conf_t *u);
bool write_usim_data(const char *directory, int user_id, usim_data_t *usim_data);
void gen_usim_data(usim_data_conf_t *u, usim_data_t *usim_data,
                   const user_plmns_t *user_plmns, const networks_t networks);

#endif
