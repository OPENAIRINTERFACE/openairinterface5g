#include <string.h>

#include "memory.h"
#include "conf_emm.h"
#include "fs.h"

void gen_emm_data(
    emm_nvdata_t     *emm_data,
    const char       *hplmn,
    const char       *msin,
    const plmns_list *eplmn,
    const networks_t networks)
{
  memset(emm_data, 0, sizeof(emm_nvdata_t));
  int hplmn_index = get_plmn_index(hplmn, networks);
  plmn_conf_param_t *conf = &networks.items[hplmn_index].conf;
  int i;

  emm_data->imsi.length = 8;
  emm_data->imsi.u.num.parity = get_msin_parity(msin, conf->mcc, conf->mnc);
  emm_data->imsi.u.num.digit1 = conf->mcc[0];
  emm_data->imsi.u.num.digit2 = conf->mcc[1];
  emm_data->imsi.u.num.digit3 = conf->mcc[2];

  emm_data->imsi.u.num.digit4 = conf->mnc[0];
  emm_data->imsi.u.num.digit5 = conf->mnc[1];

  if (strlen(conf->mnc) == 3) {
    emm_data->rplmn.MNCdigit3 = conf->mnc[2];

    emm_data->imsi.u.num.digit6 = conf->mnc[2];
    emm_data->imsi.u.num.digit7 = msin[0];
    emm_data->imsi.u.num.digit8 = msin[1];
    emm_data->imsi.u.num.digit9 = msin[2];
    emm_data->imsi.u.num.digit10 = msin[3];
    emm_data->imsi.u.num.digit11 = msin[4];
    emm_data->imsi.u.num.digit12 = msin[5];
    emm_data->imsi.u.num.digit13 = msin[6];
    emm_data->imsi.u.num.digit14 = msin[7];
    emm_data->imsi.u.num.digit15 = msin[8];
  } else {
    emm_data->rplmn.MNCdigit3 = 0xf;

    emm_data->imsi.u.num.digit6 = msin[0];
    emm_data->imsi.u.num.digit7 = msin[1];
    emm_data->imsi.u.num.digit8 = msin[2];
    emm_data->imsi.u.num.digit9 = msin[3];
    emm_data->imsi.u.num.digit10 = msin[4];
    emm_data->imsi.u.num.digit11 = msin[5];
    emm_data->imsi.u.num.digit12 = msin[6];
    emm_data->imsi.u.num.digit13 = msin[7];
    emm_data->imsi.u.num.digit14 = msin[8];
    emm_data->imsi.u.num.digit15 = msin[9];
  }

  emm_data->rplmn.MCCdigit1 = conf->mcc[0];
  emm_data->rplmn.MCCdigit2 = conf->mcc[1];
  emm_data->rplmn.MCCdigit3 = conf->mcc[2];
  emm_data->rplmn.MNCdigit1 = conf->mnc[0];
  emm_data->rplmn.MNCdigit2 = conf->mnc[1];

  emm_data->eplmn.n_plmns = eplmn->size;
  for (i = 0; i < eplmn->size; i++) {
    emm_data->eplmn.plmn[i] = networks.items[eplmn->items[i]].plmn;
  }
}

bool write_emm_data(const char *directory, int user_id, emm_nvdata_t *emm_data) {
    int rc;
	char* filename = make_filename(directory, EMM_NVRAM_FILENAME, user_id);
	rc = memory_write(filename, emm_data, sizeof(emm_nvdata_t));
	free(filename);
	if (rc != RETURNok) {
		perror("ERROR\t: memory_write() failed");
		exit(false);
	}
    return(true);
}

int get_msin_parity(const char * msin, const char *mcc, const char *mnc) {
	int imsi_size = strlen(msin) + strlen(mcc)
			+ strlen(mnc);
	int result = (imsi_size % 2 == 0) ? 0 : 1;
	return result;

}


