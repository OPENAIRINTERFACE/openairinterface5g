#include <stdlib.h>
#include <string.h>

#include "conf_network.h"

int get_plmn_index(const char * mccmnc, const networks_t networks) {
	char mcc[4];
	char mnc[strlen(mccmnc) - 2];
	strncpy(mcc, mccmnc, 3);
	mcc[3] = '\0';
	strncpy(mnc, mccmnc + 3, 3);
	mnc[strlen(mccmnc) - 3] = '\0';
	for (int i = 0; i < networks.size; i++) {
		if (strcmp(networks.items[i].conf.mcc, mcc) == 0) {
			if (strcmp(networks.items[i].conf.mnc, mnc) == 0) {
				return i;
			}
		}
	}
	return -1;
}

plmn_t make_plmn_from_conf(const plmn_conf_param_t *plmn_conf) {
	plmn_t plmn;
	char num[6];

	memset(&plmn, 0xff, sizeof(plmn));

	snprintf(num, 6, "%s%s", plmn_conf->mcc, plmn_conf->mnc);

	plmn.MCCdigit2 = plmn_conf->mcc[1];
	plmn.MCCdigit1 = plmn_conf->mcc[0];
	plmn.MCCdigit3 = plmn_conf->mcc[2];
	plmn.MNCdigit2 = plmn_conf->mnc[1];
	plmn.MNCdigit1 = plmn_conf->mnc[0];
	if (strlen(plmn_conf->mnc) > 2) {
		plmn.MNCdigit3 = plmn_conf->mnc[2];
	}
	return plmn;
}

void gen_network_record_from_conf(const plmn_conf_param_t *conf, network_record_t *record) {
		strcpy(record->fullname, conf->fullname);
		strcpy(record->shortname, conf->shortname);

		char num[60];
		sprintf(num, "%s%s", conf->mcc, conf->mnc);
		record->num = atoi(num);

		record->plmn = make_plmn_from_conf(conf);
		record->tac_end = 0xfffd;
		record->tac_start = 0x0001;
}

bool parse_plmn_param(config_setting_t *plmn_setting, plmn_conf_param_t *conf) {
	int rc = 0;
	rc = config_setting_lookup_string(plmn_setting, FULLNAME, &conf->fullname);
	if (rc != 1) {
		printf("Error on FULLNAME\n");
		return false;
	}
	rc = config_setting_lookup_string(plmn_setting, SHORTNAME, &conf->shortname);
	if (rc != 1) {
		printf("Error on SHORTNAME\n");
		return false;
	}
	rc = config_setting_lookup_string(plmn_setting, MNC, &conf->mnc);
	if (rc != 1 || strlen(conf->mnc) > 3
			|| strlen(conf->mnc) < 2) {
		printf("Error ond MNC. Exiting\n");
		return false;
	}
	rc = config_setting_lookup_string(plmn_setting, MCC, &conf->mcc);
	if (rc != 1 || strlen(conf->mcc) != 3) {
		printf("Error on MCC\n");
		return false;
	}
	return true;
}

bool parse_plmns(config_setting_t *all_plmn_setting, networks_t *networks) {
	config_setting_t *plmn_setting = NULL;
	char plmn[100];
	int size = 0;

	size = config_setting_length(all_plmn_setting);

	networks->size = size;
	networks->items = malloc(sizeof(network_t) * size);
	for (int i = 0; i < size; i++) {
		memset(&networks->items[i].record, 0xff, sizeof(network_record_t));
	}

	for (int i = 0; i < networks->size; i++) {
		network_t *network = &networks->items[i];
		sprintf(plmn, "%s%d", PLMN, i);
		plmn_setting = config_setting_get_member(all_plmn_setting, plmn);
		if (plmn_setting == NULL) {
			printf("PLMN%d not fouund\n", i);
			return false;
		}

		if ( parse_plmn_param(plmn_setting, &network->conf) == false ) {
			return false;
		}
		gen_network_record_from_conf(&network->conf, &network->record);
		network->plmn = network->record.plmn;
	}
	return true;
}


