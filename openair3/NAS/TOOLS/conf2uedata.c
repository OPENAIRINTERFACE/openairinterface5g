#include <stdio.h>  // perror, printf, fprintf, snprintf
#include <stdlib.h> // exit, free
#include <string.h> // memset, strncpy
#include <getopt.h>

#include "conf2uedata.h"
#include "memory.h"
#include "utils.h"
#include "display.h"
#include "fs.h"
#include "conf_emm.h"
#include "conf_user_data.h"
#include "conf_usim.h"

int main(int argc, char**argv) {
	int option;
    const char* conf_file = NULL;
    const char* output_dir = NULL;
    const char options[]="c:o:h";

    while ((option = getopt(argc, argv, options)) != -1) {
		switch (option) {
		case 'c':
			conf_file = optarg;
			break;
		case 'o':
			output_dir = optarg;
			break;
		case 'h':
			_display_usage();
			return EXIT_SUCCESS;
			break;
		default:
			break;
		}
	}

	if (output_dir == NULL ) {
		printf("No output option found\n");
		_display_usage();
		return EXIT_FAILURE;
	}

    if ( conf_file == NULL ) {
		printf("No Configuration file is given\n");
		_display_usage();
		return EXIT_FAILURE;
	}

    if ( parse_config_file(output_dir, conf_file) == EXIT_FAILURE ) {
        exit(EXIT_FAILURE);
    }

    display_data_from_directory(output_dir);

	exit(EXIT_SUCCESS);
}

int parse_config_file(const char *output_dir, const char *conf_filename) {
	int rc = EXIT_SUCCESS;
    int ret;
    int ue_nb = 0;
    config_setting_t *root_setting = NULL;
    config_setting_t *ue_setting = NULL;
    config_setting_t *all_plmn_setting = NULL;
    char user[10];
    config_t cfg;

	networks_t networks;;

    ret = get_config_from_file(conf_filename, &cfg);
    if (ret == EXIT_FAILURE) {
        exit(1);
    }

    root_setting = config_root_setting(&cfg);
    ue_nb = config_setting_length(root_setting) - 1;

    all_plmn_setting = config_setting_get_member(root_setting, PLMN);
    if (all_plmn_setting == NULL) {
        printf("NO PLMN SECTION...EXITING...\n");
        return (EXIT_FAILURE);
    }

    rc = parse_plmns(all_plmn_setting, &networks);
    if (rc == EXIT_FAILURE) {
        return rc;
    }

    for (int i = 0; i < ue_nb; i++) {
	    emm_nvdata_t emm_data;

	    user_nvdata_t user_data;
	    user_data_conf_t user_data_conf;

	    usim_data_t usim_data;
	    usim_data_conf_t usim_data_conf;

		user_plmns_t user_plmns;

        sprintf(user, "%s%d", UE, i);

        ue_setting = config_setting_get_member(root_setting, user);
        if (ue_setting == NULL) {
            printf("Check UE%d settings\n", i);
            return EXIT_FAILURE;
        }

        rc = parse_user_plmns_conf(ue_setting, i, &user_plmns, &usim_data_conf.hplmn, networks);
        if (rc != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        rc = parse_ue_user_data(ue_setting, i, &user_data_conf);
        if (rc != EXIT_SUCCESS) {
            printf("Problem in USER section for UE%d. EXITING...\n", i);
            return EXIT_FAILURE;
        }
        gen_user_data(&user_data_conf, &user_data);
        write_user_data(output_dir, i, &user_data);

        rc = parse_ue_sim_param(ue_setting, i, &usim_data_conf);
        if (rc != EXIT_SUCCESS) {
            printf("Problem in SIM section for UE%d. EXITING...\n", i);
            return EXIT_FAILURE;
        }
        gen_usim_data(&usim_data_conf, &usim_data, &user_plmns, networks);
        write_usim_data(output_dir, i, &usim_data);

        gen_emm_data(&emm_data, usim_data_conf.hplmn, usim_data_conf.msin,
                     user_plmns.equivalents_home.size, networks);
        write_emm_data(output_dir, i, &emm_data);

     }
    free(networks.items);
	networks.size=0;
    config_destroy(&cfg);
	return(EXIT_SUCCESS);
}

int get_config_from_file(const char *filename, config_t *config) {
    config_init(config);
    if (filename == NULL) {
        // XXX write error message ?
        exit(EXIT_FAILURE);
    }

    /* Read the file. If there is an error, report it and exit. */
    if (!config_read_file(config, filename)) {
        fprintf(stderr, "Cant read config file '%s': %s\n", filename,
                config_error_text(config));
        if ( config_error_type(config) == CONFIG_ERR_PARSE ) {
            fprintf(stderr, "This is line %d\n", config_error_line(config));
        }
        config_destroy(config);
        return (EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}


int parse_plmn_param(config_setting_t *plmn_setting, plmn_conf_param_t *conf) {
	int rc = 0;
	rc = config_setting_lookup_string(plmn_setting, FULLNAME, &conf->fullname);
	if (rc != 1) {
		printf("Error on FULLNAME\n");
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(plmn_setting, SHORTNAME, &conf->shortname);
	if (rc != 1) {
		printf("Error on SHORTNAME\n");
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(plmn_setting, MNC, &conf->mnc);
	if (rc != 1 || strlen(conf->mnc) > 3
			|| strlen(conf->mnc) < 2) {
		printf("Error ond MNC. Exiting\n");
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(plmn_setting, MCC, &conf->mcc);
	if (rc != 1 || strlen(conf->mcc) != 3) {
		printf("Error on MCC\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int parse_plmns(config_setting_t *all_plmn_setting, networks_t *networks) {
	config_setting_t *plmn_setting = NULL;
	char plmn[10];
	int rc = EXIT_SUCCESS;
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
		if (plmn_setting != NULL) {
			rc = parse_plmn_param(plmn_setting, &network->conf);
			if (rc == EXIT_FAILURE) {
				return rc;
			}
		} else {
			printf("Problem in PLMN%d. Exiting...\n", i);
			return EXIT_FAILURE;
		}
		gen_network_record_from_conf(&network->conf, &network->record);
		network->plmn = network->record.plmn;
	}
	return rc;
}

int parse_user_plmns_conf(config_setting_t *ue_setting, int user_id,
                          user_plmns_t *user_plmns, const char **h,
                          const networks_t networks) {
	int nb_errors = 0;
	const char *hplmn;

	if ( config_setting_lookup_string(ue_setting, HPLMN, h) != 1 ) {
		printf("Check HPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	hplmn = *h;
	if (get_plmn_index(hplmn, networks) == -1) {
		printf("HPLMN for UE%d is not defined in PLMN section. Exiting\n",
				user_id);
		return EXIT_FAILURE;
	}

	if ( parse_Xplmn(ue_setting, UCPLMN, user_id, &user_plmns->users_controlled, networks) == EXIT_FAILURE )
		nb_errors++;
	if ( parse_Xplmn(ue_setting, OPLMN, user_id, &user_plmns->operators, networks) == EXIT_FAILURE )
		nb_errors++;
	if ( parse_Xplmn(ue_setting, OCPLMN, user_id, &user_plmns->operators_controlled, networks) == EXIT_FAILURE )
		nb_errors++;
	if ( parse_Xplmn(ue_setting, FPLMN, user_id, &user_plmns->forbiddens, networks) == EXIT_FAILURE )
		nb_errors++;
	if ( parse_Xplmn(ue_setting, EHPLMN, user_id, &user_plmns->equivalents_home, networks) == EXIT_FAILURE )
		nb_errors++;

	if ( nb_errors > 0 )
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

int parse_Xplmn(config_setting_t *ue_setting, const char *section,
               int user_id, plmns_list *plmns, const networks_t networks) {
	int rc;
	int item_count;
	config_setting_t *setting;

	setting = config_setting_get_member(ue_setting, section);
	if (setting == NULL) {
		printf("Check %s section for UE%d. Exiting\n", section, user_id);
		return EXIT_FAILURE;
	}

	item_count = config_setting_length(setting);
	int *datas = malloc(item_count * sizeof(int));
	for (int i = 0; i < item_count; i++) {
		const char *mccmnc = config_setting_get_string_elem(setting, i);
		if (mccmnc == NULL) {
			printf("Check %s section for UE%d. Exiting\n", section, user_id);
			return EXIT_FAILURE;
		}
		rc = get_plmn_index(mccmnc, networks);
		if (rc == -1) {
			printf("The PLMN %s is not defined in PLMN section. Exiting...\n",
					mccmnc);
			return EXIT_FAILURE;
		}
		datas[i] = rc;
	}

	plmns->size = item_count;
	plmns->items = datas;
	return EXIT_SUCCESS;
}

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

		char num[6];
		sprintf(num, "%s%s", conf->mcc, conf->mnc);
		record->num = atoi(num);

		record->plmn = make_plmn_from_conf(conf);
		record->tac_end = 0xfffd;
		record->tac_start = 0x0001;
}

/*
 * Displays command line usage
 */
void _display_usage(void) {
	fprintf(stderr, "usage: conf2uedata [OPTION] [directory] ...\n");
	fprintf(stderr, "\t[-c]\tConfig file to use\n");
	fprintf(stderr, "\t[-o]\toutput file directory\n");
	fprintf(stderr, "\t[-h]\tDisplay this usage\n");
}
