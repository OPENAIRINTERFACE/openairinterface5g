#include <stdio.h>  // perror, printf, fprintf, snprintf
#include <stdlib.h> // exit, free
#include <string.h> // memset, strncpy
#include <getopt.h>

#include "conf2uedata.h"
#include "memory.h"
#include "userDef.h"
#include "usim_api.h"
#include "utils.h"
#include "display.h"
#include "fs.h"

const char *output_dir = NULL;

const char *msin = NULL;
const char *usim_api_k = NULL;
const char *msisdn = NULL;
const char *opc = NULL;
const char *hplmn = NULL;

int *ucplmn = NULL;
int *oplmn = NULL;
int *ocplmn = NULL;
int *fplmn = NULL;
int *ehplmn = NULL;

int hplmn_index = 0;
int plmn_nb = 0;
int ucplmn_nb = 0;
int oplmn_nb = 0;
int ocplmn_nb = 0;
int fplmn_nb = 0;
int ehplmn_nb = 0;

plmn_conf_param_t* user_plmn_list=NULL;
network_record_t* user_network_record_list = NULL;

int main(int argc, char**argv) {
	int option;
    const char* conf_file = NULL;
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

    if ( parse_config_file(conf_file) == EXIT_FAILURE ) {
        exit(EXIT_FAILURE);
    }

    display_data_from_directory(output_dir);

	exit(EXIT_SUCCESS);
}

int parse_config_file(const char *filename) {
	int rc = EXIT_SUCCESS;
    int ret;
    int ue_nb = 0;
    config_setting_t *root_setting = NULL;
    config_setting_t *ue_setting = NULL;
    config_setting_t *all_plmn_setting = NULL;
    char user[10];
    config_t cfg;

    ret = get_config_from_file(filename, &cfg);
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

    rc = parse_plmns(all_plmn_setting);
    if (rc == EXIT_FAILURE) {
        return rc;
    }
    fill_network_record_list();

    for (int i = 0; i < ue_nb; i++) {
        sprintf(user, "%s%d", UE, i);

        ue_setting = config_setting_get_member(root_setting, user);
        if (ue_setting == NULL) {
            printf("Check UE%d settings\n", i);
            return EXIT_FAILURE;
        }

        rc = parse_ue_user_param(ue_setting, i);
        if (rc != EXIT_SUCCESS) {
            printf("Problem in USER section for UE%d. EXITING...\n", i);
            return EXIT_FAILURE;
        }

        rc = parse_ue_sim_param(ue_setting, i);
        if (rc != EXIT_SUCCESS) {
            printf("Problem in SIM section for UE%d. EXITING...\n", i);
            return EXIT_FAILURE;
        }

        rc = parse_ue_plmn_param(ue_setting, i);
        if (rc != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
        gen_emm_data(i);
        gen_usim_data(i);
    }
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

void gen_usim_data(int user_id) {
	usim_data_t usim_data = { };
	memset(&usim_data, 0, sizeof(usim_data_t));
	usim_data.imsi.length = 8;
	usim_data.imsi.u.num.parity = get_msin_parity(msin);

	usim_data.imsi.u.num.digit1 = user_plmn_list[hplmn_index].mcc[0];
	usim_data.imsi.u.num.digit2 = user_plmn_list[hplmn_index].mcc[1];
	usim_data.imsi.u.num.digit3 = user_plmn_list[hplmn_index].mcc[2];

	usim_data.imsi.u.num.digit4 = user_plmn_list[hplmn_index].mnc[0];
	usim_data.imsi.u.num.digit5 = user_plmn_list[hplmn_index].mnc[1];

	if (strlen(user_plmn_list[hplmn_index].mnc) == 2) {
		usim_data.imsi.u.num.digit6 = msin[0];
		usim_data.imsi.u.num.digit7 = msin[1];
		usim_data.imsi.u.num.digit8 = msin[2];
		usim_data.imsi.u.num.digit9 = msin[3];
		usim_data.imsi.u.num.digit10 = msin[4];
		usim_data.imsi.u.num.digit11 = msin[5];
		usim_data.imsi.u.num.digit12 = msin[6];
		usim_data.imsi.u.num.digit13 = msin[7];
		usim_data.imsi.u.num.digit14 = msin[8];
		usim_data.imsi.u.num.digit15 = msin[9];
	} else {
		usim_data.imsi.u.num.digit6 = user_plmn_list[hplmn_index].mnc[2];
		usim_data.imsi.u.num.digit7 = msin[0];
		usim_data.imsi.u.num.digit8 = msin[1];
		usim_data.imsi.u.num.digit9 = msin[2];
		usim_data.imsi.u.num.digit10 = msin[3];
		usim_data.imsi.u.num.digit11 = msin[4];
		usim_data.imsi.u.num.digit12 = msin[5];
		usim_data.imsi.u.num.digit13 = msin[6];
		usim_data.imsi.u.num.digit14 = msin[7];
		usim_data.imsi.u.num.digit15 = msin[8];
	}

	/*
	 * Ciphering and Integrity Keys
	 */
	usim_data.keys.ksi = KSI;
	memset(&usim_data.keys.ck, 0, USIM_CK_SIZE);
	memset(&usim_data.keys.ik, 0, USIM_IK_SIZE);
	/*
	 * Higher Priority PLMN search period
	 */
	usim_data.hpplmn = 0x00; /* Disable timer */

	/*
	 * List of Forbidden PLMNs
	 */
	for (int i = 0; i < USIM_FPLMN_MAX; i++) {
		memset(&usim_data.fplmn[i], 0xff, sizeof(plmn_t));
	}
	if (fplmn_nb > 0) {
		for (int i = 0; i < fplmn_nb; i++) {
			usim_data.fplmn[i] = user_network_record_list[fplmn[i]].plmn;
		}
	}

	/*
	 * Location Information
	 */
	usim_data.loci.tmsi = DEFAULT_TMSI;
	usim_data.loci.lai.plmn = user_network_record_list[hplmn_index].plmn;
	usim_data.loci.lai.lac = DEFAULT_LAC;
	usim_data.loci.status = USIM_LOCI_NOT_UPDATED;
	/*
	 * Packet Switched Location Information
	 */
	usim_data.psloci.p_tmsi = DEFAULT_P_TMSI;
	usim_data.psloci.signature[0] = 0x01;
	usim_data.psloci.signature[1] = 0x02;
	usim_data.psloci.signature[2] = 0x03;
	usim_data.psloci.rai.plmn = user_network_record_list[hplmn_index].plmn;
	usim_data.psloci.rai.lac = DEFAULT_LAC;
	usim_data.psloci.rai.rac = DEFAULT_RAC;
	usim_data.psloci.status = USIM_PSLOCI_NOT_UPDATED;
	/*
	 * Administrative Data
	 */
	usim_data.ad.UE_Operation_Mode = USIM_NORMAL_MODE;
	usim_data.ad.Additional_Info = 0xffff;
	usim_data.ad.MNC_Length = strlen(user_plmn_list[hplmn_index].mnc);
	/*
	 * EPS NAS security context
	 */
	usim_data.securityctx.length = 52;
	usim_data.securityctx.KSIasme.type = USIM_KSI_ASME_TAG;
	usim_data.securityctx.KSIasme.length = 1;
	usim_data.securityctx.KSIasme.value[0] = KSI_ASME;
	usim_data.securityctx.Kasme.type = USIM_K_ASME_TAG;
	usim_data.securityctx.Kasme.length = USIM_K_ASME_SIZE;
	memset(usim_data.securityctx.Kasme.value, 0,
			usim_data.securityctx.Kasme.length);
	usim_data.securityctx.ulNAScount.type = USIM_UL_NAS_COUNT_TAG;
	usim_data.securityctx.ulNAScount.length = USIM_UL_NAS_COUNT_SIZE;
	memset(usim_data.securityctx.ulNAScount.value, 0,
			usim_data.securityctx.ulNAScount.length);
	usim_data.securityctx.dlNAScount.type = USIM_DL_NAS_COUNT_TAG;
	usim_data.securityctx.dlNAScount.length = USIM_DL_NAS_COUNT_SIZE;
	memset(usim_data.securityctx.dlNAScount.value, 0,
			usim_data.securityctx.dlNAScount.length);
	usim_data.securityctx.algorithmID.type = USIM_INT_ENC_ALGORITHMS_TAG;
	usim_data.securityctx.algorithmID.length = 1;
	usim_data.securityctx.algorithmID.value[0] = SECURITY_ALGORITHMS;
	/*
	 * Subcriber's Number
	 */
	usim_data.msisdn.length = 7;
	usim_data.msisdn.number.ext = 1;
	usim_data.msisdn.number.ton = MSISDN_TON_UNKNOWKN;
	usim_data.msisdn.number.npi = MSISDN_NPI_ISDN_TELEPHONY;
	usim_data.msisdn.conf1_record_id = 0xff; /* Not used */
	usim_data.msisdn.ext1_record_id = 0xff; /* Not used */
	int j = 0;
	for (int i = 0; i < strlen(msisdn); i += 2) {
		usim_data.msisdn.number.digit[j].msb = msisdn[i];
		j++;
	}
	j = 0;
	for (int i = 1; i < strlen(msisdn); i += 2) {
		usim_data.msisdn.number.digit[j].lsb = msisdn[i];
		j++;

	}
	if (strlen(msisdn) % 2 == 0) {
		for (int i = strlen(msisdn) / 2; i < 10; i++) {
			usim_data.msisdn.number.digit[i].msb = 0xf;
			usim_data.msisdn.number.digit[i].lsb = 0xf;
		}
	} else {
		usim_data.msisdn.number.digit[strlen(msisdn) / 2].lsb = 0xf;
		for (int i = (strlen(msisdn) / 2) + 1; i < 10; i++) {
			usim_data.msisdn.number.digit[i].msb = 0xf;
			usim_data.msisdn.number.digit[i].lsb = 0xf;
		}
	}
	/*
	 * PLMN Network Name and Operator PLMN List
	 */
	for (int i = 0; i < oplmn_nb; i++) {
		network_record_t record = user_network_record_list[oplmn[i]];
		usim_data.pnn[i].fullname.type = USIM_PNN_FULLNAME_TAG;
		usim_data.pnn[i].fullname.length = strlen(record.fullname);
		strncpy((char*) usim_data.pnn[i].fullname.value, record.fullname,
				usim_data.pnn[i].fullname.length);
		usim_data.pnn[i].shortname.type = USIM_PNN_SHORTNAME_TAG;
		usim_data.pnn[i].shortname.length = strlen(record.shortname);
		strncpy((char*) usim_data.pnn[i].shortname.value, record.shortname,
				usim_data.pnn[i].shortname.length);
		usim_data.opl[i].plmn = record.plmn;
		usim_data.opl[i].start = record.tac_start;
		usim_data.opl[i].end = record.tac_end;
		usim_data.opl[i].record_id = i;
	}
	if (oplmn_nb < USIM_OPL_MAX) {
		for (int i = oplmn_nb; i < USIM_OPL_MAX; i++) {
			memset(&usim_data.opl[i].plmn, 0xff, sizeof(plmn_t));
		}
	}

	/*
	 * List of Equivalent HPLMNs
	 */
	for (int i = 0; i < ehplmn_nb; i++) {
		usim_data.ehplmn[i] = user_network_record_list[ehplmn[i]].plmn;
	}
	if (ehplmn_nb < USIM_EHPLMN_MAX) {
		for (int i = ehplmn_nb; i < USIM_EHPLMN_MAX; i++) {
			memset(&usim_data.ehplmn[i], 0xff, sizeof(plmn_t));
		}
	}
	/*
	 * Home PLMN Selector with Access Technology
	 */
	usim_data.hplmn.plmn = user_network_record_list[hplmn_index].plmn;
	usim_data.hplmn.AcT = (USIM_ACT_GSM | USIM_ACT_UTRAN | USIM_ACT_EUTRAN);

	/*
	 * List of user controlled PLMN selector with Access Technology
	 */
	for (int i = 0; i < USIM_PLMN_MAX; i++) {
		memset(&usim_data.plmn[i], 0xff, sizeof(plmn_t));
	}
	if (ucplmn_nb > 0) {
		for (int i = 0; i < ucplmn_nb; i++) {
			usim_data.plmn[i].plmn = user_network_record_list[ucplmn[i]].plmn;
		}
	}

	// List of operator controlled PLMN selector with Access Technology

	for (int i = 0; i < USIM_OPLMN_MAX; i++) {
		memset(&usim_data.oplmn[i], 0xff, sizeof(plmn_t));
	}
	if (ocplmn_nb > 0) {
		for (int i = 0; i < ocplmn_nb; i++) {
			usim_data.oplmn[i].plmn = user_network_record_list[ocplmn[i]].plmn;
			usim_data.oplmn[i].AcT = (USIM_ACT_GSM | USIM_ACT_UTRAN
					| USIM_ACT_EUTRAN);
		}
	}
	/*
	 * EPS Location Information
	 */
	usim_data.epsloci.guti.gummei.plmn =
			user_network_record_list[hplmn_index].plmn;
	usim_data.epsloci.guti.gummei.MMEgid = DEFAULT_MME_ID;
	usim_data.epsloci.guti.gummei.MMEcode = DEFAULT_MME_CODE;
	usim_data.epsloci.guti.m_tmsi = DEFAULT_M_TMSI;
	usim_data.epsloci.tai.plmn = usim_data.epsloci.guti.gummei.plmn;
	usim_data.epsloci.tai.tac = DEFAULT_TAC;
	usim_data.epsloci.status = USIM_EPSLOCI_UPDATED;
	/*
	 * Non-Access Stratum configuration
	 */
	usim_data.nasconfig.NAS_SignallingPriority.type =
	USIM_NAS_SIGNALLING_PRIORITY_TAG;
	usim_data.nasconfig.NAS_SignallingPriority.length = 1;
	usim_data.nasconfig.NAS_SignallingPriority.value[0] = 0x00;
	usim_data.nasconfig.NMO_I_Behaviour.type = USIM_NMO_I_BEHAVIOUR_TAG;
	usim_data.nasconfig.NMO_I_Behaviour.length = 1;
	usim_data.nasconfig.NMO_I_Behaviour.value[0] = 0x00;
	usim_data.nasconfig.AttachWithImsi.type = USIM_ATTACH_WITH_IMSI_TAG;
	usim_data.nasconfig.AttachWithImsi.length = 1;
#if defined(START_WITH_GUTI)
	usim_data.nasconfig.AttachWithImsi.value[0] = 0x00;
#else
	usim_data.nasconfig.AttachWithImsi.value[0] = 0x01;
#endif
	usim_data.nasconfig.MinimumPeriodicSearchTimer.type =
	USIM_MINIMUM_PERIODIC_SEARCH_TIMER_TAG;
	usim_data.nasconfig.MinimumPeriodicSearchTimer.length = 1;
	usim_data.nasconfig.MinimumPeriodicSearchTimer.value[0] = 0x00;
	usim_data.nasconfig.ExtendedAccessBarring.type =
	USIM_EXTENDED_ACCESS_BARRING_TAG;
	usim_data.nasconfig.ExtendedAccessBarring.length = 1;
	usim_data.nasconfig.ExtendedAccessBarring.value[0] = 0x00;
	usim_data.nasconfig.Timer_T3245_Behaviour.type =
	USIM_TIMER_T3245_BEHAVIOUR_TAG;
	usim_data.nasconfig.Timer_T3245_Behaviour.length = 1;
	usim_data.nasconfig.Timer_T3245_Behaviour.value[0] = 0x00;

	/* initialize the subscriber authentication security key */
	hex_string_to_hex_value(usim_data.keys.usim_api_k,
			usim_api_k, USIM_API_K_SIZE);
	hex_string_to_hex_value(usim_data.keys.opc, opc,
	OPC_SIZE);

    char *path = make_filename(output_dir, USIM_API_NVRAM_FILENAME, user_id);
    usim_api_write(path, &usim_data);
    free(path);
}
void gen_emm_data(int user_id) {
	hplmn_index = get_plmn_index(hplmn);
	emm_nvdata_t emm_data;
	int rc;
	memset(&emm_data, 0, sizeof(emm_nvdata_t));
	int hplmn_index = get_plmn_index(hplmn);
	emm_data.imsi.length = 8;
	emm_data.imsi.u.num.parity = get_msin_parity(msin);
	emm_data.imsi.u.num.digit1 = user_plmn_list[hplmn_index].mcc[0];
	emm_data.imsi.u.num.digit2 = user_plmn_list[hplmn_index].mcc[1];
	emm_data.imsi.u.num.digit3 = user_plmn_list[hplmn_index].mcc[2];

	emm_data.imsi.u.num.digit4 = user_plmn_list[hplmn_index].mnc[0];
	emm_data.imsi.u.num.digit5 = user_plmn_list[hplmn_index].mnc[1];

	if (strlen(user_plmn_list[hplmn_index].mnc) == 3) {
		emm_data.rplmn.MNCdigit3 = user_plmn_list[hplmn_index].mnc[2];

		emm_data.imsi.u.num.digit6 = user_plmn_list[hplmn_index].mnc[2];
		emm_data.imsi.u.num.digit7 = msin[0];
		emm_data.imsi.u.num.digit8 = msin[1];
		emm_data.imsi.u.num.digit9 = msin[2];
		emm_data.imsi.u.num.digit10 = msin[3];
		emm_data.imsi.u.num.digit11 = msin[4];
		emm_data.imsi.u.num.digit12 = msin[5];
		emm_data.imsi.u.num.digit13 = msin[6];
		emm_data.imsi.u.num.digit14 = msin[7];
		emm_data.imsi.u.num.digit15 = msin[8];

	} else {
		emm_data.rplmn.MNCdigit3 = 0xf;

		emm_data.imsi.u.num.digit6 = msin[0];
		emm_data.imsi.u.num.digit7 = msin[1];
		emm_data.imsi.u.num.digit8 = msin[2];
		emm_data.imsi.u.num.digit9 = msin[3];
		emm_data.imsi.u.num.digit10 = msin[4];
		emm_data.imsi.u.num.digit11 = msin[5];
		emm_data.imsi.u.num.digit12 = msin[6];
		emm_data.imsi.u.num.digit13 = msin[7];
		emm_data.imsi.u.num.digit14 = msin[8];
		emm_data.imsi.u.num.digit15 = msin[9];

	}

	emm_data.rplmn.MCCdigit1 = user_plmn_list[hplmn_index].mcc[0];
	emm_data.rplmn.MCCdigit2 = user_plmn_list[hplmn_index].mcc[1];
	emm_data.rplmn.MCCdigit3 = user_plmn_list[hplmn_index].mcc[2];
	emm_data.rplmn.MNCdigit1 = user_plmn_list[hplmn_index].mnc[0];
	emm_data.rplmn.MNCdigit2 = user_plmn_list[hplmn_index].mnc[1];

	emm_data.eplmn.n_plmns = ehplmn_nb;

	char* path = make_filename(output_dir, EMM_NVRAM_FILENAME, user_id);
	rc = memory_write(path, &emm_data, sizeof(emm_nvdata_t));
	free(path);
	if (rc != RETURNok) {
		perror("ERROR\t: memory_write() failed");
		exit(EXIT_FAILURE);
	}

}

int parse_plmn_param(config_setting_t *plmn_setting, int index) {
	int rc = 0;
	rc = config_setting_lookup_string(plmn_setting,
	FULLNAME, &user_plmn_list[index].fullname);
	if (rc != 1) {
		printf("Check PLMN%d FULLNAME. Exiting\n", index);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(plmn_setting,
	SHORTNAME, &user_plmn_list[index].shortname);
	if (rc != 1) {
		printf("Check PLMN%d SHORTNAME. Exiting\n", index);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(plmn_setting,
	MNC, &user_plmn_list[index].mnc);
	if (rc != 1 || strlen(user_plmn_list[index].mnc) > 3
			|| strlen(user_plmn_list[index].mnc) < 2) {
		printf("Check PLMN%d MNC. Exiting\n", index);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(plmn_setting,
	MCC, &user_plmn_list[index].mcc);
	if (rc != 1 || strlen(user_plmn_list[index].mcc) != 3) {
		printf("Check PLMN%d MCC. Exiting\n", index);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int parse_plmns(config_setting_t *all_plmn_setting) {
	config_setting_t *plmn_setting = NULL;
	char plmn[10];
	int rc = EXIT_SUCCESS;
	plmn_nb = config_setting_length(all_plmn_setting);
	user_plmn_list = malloc(sizeof(plmn_conf_param_t) * plmn_nb);
	user_network_record_list = malloc(sizeof(network_record_t) * plmn_nb);
	for (int i = 0; i < plmn_nb; i++) {
		memset(&user_network_record_list[i], 0xff, sizeof(network_record_t));
		memset(&user_plmn_list[i], 0xff, sizeof(plmn_conf_param_t));
	}
	for (int i = 0; i < plmn_nb; i++) {
		sprintf(plmn, "%s%d", PLMN, i);
		plmn_setting = config_setting_get_member(all_plmn_setting, plmn);
		if (plmn_setting != NULL) {
			rc = parse_plmn_param(plmn_setting, i);
			if (rc == EXIT_FAILURE) {
				return rc;
			}
		} else {
			printf("Problem in PLMN%d. Exiting...\n", i);
			return EXIT_FAILURE;
		}
	}
	return rc;
}

int parse_ue_plmn_param(config_setting_t *ue_setting, int user_id) {
	int rc = EXIT_SUCCESS;
	config_setting_t *setting = NULL;
	rc = config_setting_lookup_string(ue_setting, HPLMN, &hplmn);
	if (rc != 1) {
		printf("Check HPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	} else if (get_plmn_index(hplmn) == -1) {
		printf("HPLMN for UE%d is not defined in PLMN section. Exiting\n",
				user_id);
		return EXIT_FAILURE;
	}
	setting = config_setting_get_member(ue_setting, UCPLMN);
	if (setting == NULL) {
		printf("Check UCPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = fill_ucplmn(setting, user_id);
	if (rc != EXIT_SUCCESS) {
		printf("Check UCPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	setting = config_setting_get_member(ue_setting, OPLMN);
	if (setting == NULL) {
		printf("Check OPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = fill_oplmn(setting, user_id);
	if (rc != EXIT_SUCCESS) {
		printf("Check OPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	setting = config_setting_get_member(ue_setting, OCPLMN);
	if (setting == NULL) {
		printf("Check OCPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = fill_ocplmn(setting, user_id);
	if (rc != EXIT_SUCCESS) {
		printf("Check OCPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	setting = config_setting_get_member(ue_setting, FPLMN);
	if (setting == NULL) {
		printf("Check FPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = fill_fplmn(setting, user_id);
	if (rc != EXIT_SUCCESS) {
		printf("Check FPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	setting = config_setting_get_member(ue_setting, EHPLMN);
	if (setting == NULL) {
		printf("Check EHPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = fill_ehplmn(setting, user_id);
	if (rc != EXIT_SUCCESS) {
		printf("Check EHPLMN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int parse_ue_sim_param(config_setting_t *ue_setting, int user_id) {
	int rc = EXIT_SUCCESS;
	config_setting_t *ue_param_setting = NULL;
	ue_param_setting = config_setting_get_member(ue_setting, SIM);
	if (ue_param_setting == NULL) {
		printf("Check SIM section for UE%d. EXITING...\n", user_id);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(ue_param_setting, MSIN, &msin);
	if (rc != 1 || strlen(msin) > 10 || strlen(msin) < 9) {
		printf("Check SIM MSIN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(ue_param_setting, USIM_API_K,
			&usim_api_k);
	if (rc != 1) {
		printf("Check SIM USIM_API_K  section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(ue_param_setting, OPC, &opc);
	if (rc != 1) {
		printf("Check SIM OPC section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(ue_param_setting, MSISDN, &msisdn);
	if (rc != 1) {
		printf("Check SIM MSISDN section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int parse_ue_user_param(config_setting_t *ue_setting, int user_id) {
	config_setting_t *ue_param_setting = NULL;
	user_nvdata_t user_data;
	const char* imei = NULL;
	const char* manufacturer = NULL;
	const char* model = NULL;
	const char* pin = NULL;

	int rc = EXIT_SUCCESS;
	ue_param_setting = config_setting_get_member(ue_setting, USER);
	if (ue_param_setting == NULL) {
		printf("Check USER section of UE%d. EXITING...\n", user_id);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(ue_param_setting, UE_IMEI, &imei);
	if (rc != 1) {
		printf("Check USER IMEI section for UE%d. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(ue_param_setting, MANUFACTURER,
			&manufacturer);
	if (rc != 1) {
		printf("Check USER MANUFACTURER for UE%d FULLNAME. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(ue_param_setting, MODEL, &model);
	if (rc != 1) {
		printf("Check USER MODEL for UE%d FULLNAME. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	rc = config_setting_lookup_string(ue_param_setting, PINCODE, &pin);
	if (rc != 1) {
		printf("Check USER PIN for UE%d FULLNAME. Exiting\n", user_id);
		return EXIT_FAILURE;
	}
	memset(&user_data, 0, sizeof(user_nvdata_t));
	snprintf(user_data.IMEI, USER_IMEI_SIZE + 1, "%s%d", imei, _luhn(imei));
	/*
	 * Manufacturer identifier
	 */
	strncpy(user_data.manufacturer, manufacturer, USER_MANUFACTURER_SIZE);
	/*
	 * Model identifier
	 */
	strncpy(user_data.model, model, USER_MODEL_SIZE);
	/*
	 * SIM Personal Identification Number
	 */
	strncpy(user_data.PIN, pin, USER_PIN_SIZE);

	char* path = make_filename(output_dir, USER_NVRAM_FILENAME, user_id);
	rc = memory_write(path, &user_data, sizeof(user_nvdata_t));
    free(path);
	if (rc != RETURNok) {
		perror("ERROR\t: memory_write() failed");
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}

int fill_ucplmn(config_setting_t* setting, int user_id) {
	int rc;
	ucplmn_nb = config_setting_length(setting);
	ucplmn = malloc(ucplmn_nb * sizeof(int));
	for (int i = 0; i < ucplmn_nb; i++) {
		const char *mccmnc = config_setting_get_string_elem(setting, i);
		if (mccmnc == NULL) {
			printf("Check UCPLMN section for UE%d. Exiting\n", user_id);
			return EXIT_FAILURE;
		}
		rc = get_plmn_index(mccmnc);
		if (rc == -1) {
			printf("The PLMN %s is not defined in PLMN section. Exiting...\n",
					mccmnc);
			return EXIT_FAILURE;
		}
		ucplmn[i] = rc;
	}
	return EXIT_SUCCESS;
}
int fill_oplmn(config_setting_t* setting, int user_id) {
	int rc;
	oplmn_nb = config_setting_length(setting);
	oplmn = malloc(oplmn_nb * sizeof(int));
	for (int i = 0; i < oplmn_nb; i++) {
		const char *mccmnc = config_setting_get_string_elem(setting, i);
		if (mccmnc == NULL) {
			printf("Check OPLMN section for UE%d. Exiting\n", user_id);
			return EXIT_FAILURE;
		}
		rc = get_plmn_index(mccmnc);
		if (rc == -1) {
			printf("The PLMN %s is not defined in PLMN section. Exiting...\n",
					mccmnc);
			return EXIT_FAILURE;
		}
		oplmn[i] = rc;
	}
	return EXIT_SUCCESS;
}
int fill_ocplmn(config_setting_t* setting, int user_id) {
	int rc;
	ocplmn_nb = config_setting_length(setting);
	ocplmn = malloc(ocplmn_nb * sizeof(int));
	for (int i = 0; i < ocplmn_nb; i++) {
		const char *mccmnc = config_setting_get_string_elem(setting, i);
		if (mccmnc == NULL) {
			printf("Check OCPLMN section for UE%d. Exiting\n", user_id);
			return EXIT_FAILURE;
		}
		rc = get_plmn_index(mccmnc);
		if (rc == -1) {
			printf("The PLMN %s is not defined in PLMN section. Exiting...\n",
					mccmnc);
			return EXIT_FAILURE;
		}
		ocplmn[i] = rc;
	}
	return EXIT_SUCCESS;
}
int fill_fplmn(config_setting_t* setting, int user_id) {
	int rc;
	fplmn_nb = config_setting_length(setting);
	fplmn = malloc(fplmn_nb * sizeof(int));
	for (int i = 0; i < fplmn_nb; i++) {
		const char *mccmnc = config_setting_get_string_elem(setting, i);
		if (mccmnc == NULL) {
			printf("Check FPLMN section for UE%d. Exiting\n", user_id);
			return EXIT_FAILURE;
		}
		rc = get_plmn_index(mccmnc);
		if (rc == -1) {
			printf("The PLMN %s is not defined in PLMN section. Exiting...\n",
					mccmnc);
			return EXIT_FAILURE;
		}
		fplmn[i] = rc;
	}
	return EXIT_SUCCESS;
}
int fill_ehplmn(config_setting_t* setting, int user_id) {
	int rc;
	ehplmn_nb = config_setting_length(setting);
	ehplmn = malloc(ehplmn_nb * sizeof(int));
	for (int i = 0; i < ehplmn_nb; i++) {
		const char *mccmnc = config_setting_get_string_elem(setting, i);
		if (mccmnc == NULL) {
			printf("Check EHPLMN section for UE%d. Exiting\n", user_id);
			return EXIT_FAILURE;
		}
		rc = get_plmn_index(mccmnc);
		if (rc == -1) {
			printf("The PLMN %s is not defined in PLMN section. Exiting...\n",
					mccmnc);
			return EXIT_FAILURE;
		}
		ehplmn[i] = rc;
	}
	return EXIT_SUCCESS;
}

int get_plmn_index(const char * mccmnc) {
	char mcc[4];
	char mnc[strlen(mccmnc) - 2];
	strncpy(mcc, mccmnc, 3);
	mcc[3] = '\0';
	strncpy(mnc, mccmnc + 3, 3);
	mnc[strlen(mccmnc) - 3] = '\0';
	for (int i = 0; i < plmn_nb; i++) {
		if (strcmp(user_plmn_list[i].mcc, mcc) == 0) {
			if (strcmp(user_plmn_list[i].mnc, mnc) == 0) {
				return i;
			}
		}
	}
	return -1;
}

int get_msin_parity(const char * msin) {
	int imsi_size = strlen(msin) + strlen(user_plmn_list[hplmn_index].mcc)
			+ strlen(user_plmn_list[hplmn_index].mnc);
	int result = (imsi_size % 2 == 0) ? 0 : 1;
	return result;

}

void fill_network_record_list() {
	for (int i = 0; i < plmn_nb; i++) {
		strcpy(user_network_record_list[i].fullname,
				user_plmn_list[i].fullname);
		strcpy(user_network_record_list[i].shortname,
				user_plmn_list[i].shortname);
		char num[6];
		sprintf(num, "%s%s", user_plmn_list[i].mcc, user_plmn_list[i].mnc);
		user_network_record_list[i].num = atoi(num);
		user_network_record_list[i].plmn.MCCdigit2 = user_plmn_list[i].mcc[1];
		user_network_record_list[i].plmn.MCCdigit1 = user_plmn_list[i].mcc[0];
		user_network_record_list[i].plmn.MCCdigit3 = user_plmn_list[i].mcc[2];
		user_network_record_list[i].plmn.MNCdigit2 = user_plmn_list[i].mnc[1];
		user_network_record_list[i].plmn.MNCdigit1 = user_plmn_list[i].mnc[0];
		user_network_record_list[i].tac_end = 0xfffd;
		user_network_record_list[i].tac_start = 0x0001;
		if (strlen(user_plmn_list[i].mnc) > 2) {
			user_network_record_list[i].plmn.MNCdigit3 =
					user_plmn_list[i].mnc[2];
		}

	}
}

/*
 * Computes the check digit using Luhn algorithm
 */
int _luhn(const char* cc) {
	const int m[] = { 0, 2, 4, 6, 8, 1, 3, 5, 7, 9 };
	int odd = 1, sum = 0;

	for (int i = strlen(cc); i--; odd = !odd) {
		int digit = cc[i] - '0';
		sum += odd ? m[digit] : digit;
	}

	return 10 - (sum % 10);
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
