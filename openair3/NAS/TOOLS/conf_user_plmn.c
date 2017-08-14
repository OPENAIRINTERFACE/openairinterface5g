#include <stdlib.h>
#include <string.h>
#include "conf_user_plmn.h"

bool parse_user_plmns_conf(config_setting_t *ue_setting, int user_id,
                          user_plmns_t *user_plmns, const char **h,
                          const networks_t networks) {
	int nb_errors = 0;
	const char *hplmn;

	if ( config_setting_lookup_string(ue_setting, HPLMN, h) != 1 ) {
		printf("Check HPLMN section for UE%d. Exiting\n", user_id);
		return false;
	}
	hplmn = *h;
	if (get_plmn_index(hplmn, networks) == -1) {
		printf("HPLMN for UE%d is not defined in PLMN section. Exiting\n",
				user_id);
		return false;
	}

	if ( parse_Xplmn(ue_setting, UCPLMN, user_id, &user_plmns->users_controlled, networks) == false )
		nb_errors++;
	if ( parse_Xplmn(ue_setting, OPLMN, user_id, &user_plmns->operators, networks) == false )
		nb_errors++;
	if ( parse_Xplmn(ue_setting, OCPLMN, user_id, &user_plmns->operators_controlled, networks) == false )
		nb_errors++;
	if ( parse_Xplmn(ue_setting, FPLMN, user_id, &user_plmns->forbiddens, networks) == false )
		nb_errors++;
	if ( parse_Xplmn(ue_setting, EHPLMN, user_id, &user_plmns->equivalents_home, networks) == false )
		nb_errors++;

	if ( nb_errors > 0 )
		return false;
	return true;
}

bool parse_Xplmn(config_setting_t *ue_setting, const char *section,
               int user_id, plmns_list *plmns, const networks_t networks) {
	int rc;
	int item_count;
	config_setting_t *setting;

	setting = config_setting_get_member(ue_setting, section);
	if (setting == NULL) {
		printf("Check %s section for UE%d. Exiting\n", section, user_id);
		return false;
	}

	item_count = config_setting_length(setting);
	int *datas = malloc(item_count * sizeof(int));
	for (int i = 0; i < item_count; i++) {
		const char *mccmnc = config_setting_get_string_elem(setting, i);
		if (mccmnc == NULL) {
			printf("Check %s section for UE%d. Exiting\n", section, user_id);
			free(datas);
			return false;
		}
		rc = get_plmn_index(mccmnc, networks);
		if (rc == -1) {
			printf("The PLMN %s is not defined in PLMN section. Exiting...\n",
					mccmnc);
			free(datas);
			return false;
		}
		datas[i] = rc;
	}

	plmns->size = item_count;
	plmns->items = datas;
	return true;
}

void user_plmns_free(user_plmns_t *user_plmns) {
	free(user_plmns->users_controlled.items);
	free(user_plmns->operators.items);
	free(user_plmns->operators_controlled.items);
	free(user_plmns->forbiddens.items);
	free(user_plmns->equivalents_home.items);
	memset(user_plmns, 0, sizeof(user_plmns_t));
}
