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

    if ( parse_plmns(all_plmn_setting, &networks) == false ) {
        return EXIT_FAILURE;
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

        if ( parse_user_plmns_conf(ue_setting, i, &user_plmns, &usim_data_conf.hplmn, networks) == false ) {
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

		user_plmns_free(&user_plmns);

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

/*
 * Displays command line usage
 */
void _display_usage(void) {
	fprintf(stderr, "usage: conf2uedata [OPTION] [directory] ...\n");
	fprintf(stderr, "\t[-c]\tConfig file to use\n");
	fprintf(stderr, "\t[-o]\toutput file directory\n");
	fprintf(stderr, "\t[-h]\tDisplay this usage\n");
}
