#include <string.h>

#include "conf2uedata.h"
#include "memory.h"
#include "conf_emm.h"
#include "fs.h"

void gen_emm_data(emm_nvdata_t *emm_data) {
	hplmn_index = get_plmn_index(hplmn);
	memset(emm_data, 0, sizeof(emm_nvdata_t));
	int hplmn_index = get_plmn_index(hplmn);
	emm_data->imsi.length = 8;
	emm_data->imsi.u.num.parity = get_msin_parity(msin);
	emm_data->imsi.u.num.digit1 = user_plmn_list[hplmn_index].mcc[0];
	emm_data->imsi.u.num.digit2 = user_plmn_list[hplmn_index].mcc[1];
	emm_data->imsi.u.num.digit3 = user_plmn_list[hplmn_index].mcc[2];

	emm_data->imsi.u.num.digit4 = user_plmn_list[hplmn_index].mnc[0];
	emm_data->imsi.u.num.digit5 = user_plmn_list[hplmn_index].mnc[1];

	if (strlen(user_plmn_list[hplmn_index].mnc) == 3) {
		emm_data->rplmn.MNCdigit3 = user_plmn_list[hplmn_index].mnc[2];

		emm_data->imsi.u.num.digit6 = user_plmn_list[hplmn_index].mnc[2];
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

	emm_data->rplmn.MCCdigit1 = user_plmn_list[hplmn_index].mcc[0];
	emm_data->rplmn.MCCdigit2 = user_plmn_list[hplmn_index].mcc[1];
	emm_data->rplmn.MCCdigit3 = user_plmn_list[hplmn_index].mcc[2];
	emm_data->rplmn.MNCdigit1 = user_plmn_list[hplmn_index].mnc[0];
	emm_data->rplmn.MNCdigit2 = user_plmn_list[hplmn_index].mnc[1];

	emm_data->eplmn.n_plmns = ehplmn_nb;
}

int write_emm_data(const char *directory, int user_id, emm_nvdata_t *emm_data) {
    int rc;
	char* filename = make_filename(directory, EMM_NVRAM_FILENAME, user_id);
	rc = memory_write(filename, emm_data, sizeof(emm_nvdata_t));
	free(filename);
	if (rc != RETURNok) {
		perror("ERROR\t: memory_write() failed");
		exit(EXIT_FAILURE);
	}
    return(EXIT_SUCCESS);
}

int get_msin_parity(const char * msin) {
	int imsi_size = strlen(msin) + strlen(user_plmn_list[hplmn_index].mcc)
			+ strlen(user_plmn_list[hplmn_index].mnc);
	int result = (imsi_size % 2 == 0) ? 0 : 1;
	return result;

}


