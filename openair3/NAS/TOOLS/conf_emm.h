#ifndef _CONF_EMM_H
#define _CONF_EMM_H

#include "emmData.h"

void gen_emm_data(emm_nvdata_t *emm_data);
int write_emm_data(const char *directory, int user_id, emm_nvdata_t *emm_data);
int get_msin_parity(const char * msin);

#endif
