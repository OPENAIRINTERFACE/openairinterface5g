#ifndef _CONF_EMM_H
#define _CONF_EMM_H

#include "emmData.h"
#include "conf_network.h"
#include "conf_user_plmn.h"

void gen_emm_data(
    emm_nvdata_t     *emm_data,
    const char       *hplmn,
    const char       *msin,
    const plmns_list *eplmn,
    const networks_t networks);

bool write_emm_data(const char *directory, int user_id, emm_nvdata_t *emm_data);
int get_msin_parity(const char * msin, const char *mcc, const char *mnc);

#endif
