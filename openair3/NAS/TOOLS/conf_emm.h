#ifndef _CONF_EMM_H
#define _CONF_EMM_H

#include "conf2uedata.h"
#include "emmData.h"

void gen_emm_data(emm_nvdata_t *emm_data, const char *hplmn, const char *msin, int ehplmn_count, const networks_t networks);
int write_emm_data(const char *directory, int user_id, emm_nvdata_t *emm_data);
int get_msin_parity(const char * msin, const char *mcc, const char *mnc);

#endif
