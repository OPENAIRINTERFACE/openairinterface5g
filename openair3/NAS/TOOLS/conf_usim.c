#include <string.h>
#include <stdlib.h>

#include "userDef.h"
#include "utils.h"
#include "conf_emm.h"
#include "fs.h"
#include "conf_usim.h"

bool parse_ue_sim_param(config_setting_t *ue_setting, int user_id, usim_data_conf_t *u) {
	int rc = true;
	config_setting_t *ue_param_setting = NULL;
	ue_param_setting = config_setting_get_member(ue_setting, SIM);
	if (ue_param_setting == NULL) {
		printf("Check SIM section for UE%d. EXITING...\n", user_id);
		return false;
	}
	rc = config_setting_lookup_string(ue_param_setting, MSIN, &u->msin);
	if (rc != 1 || strlen(u->msin) > 10 || strlen(u->msin) < 9) {
		printf("Check SIM MSIN section for UE%d. Exiting\n", user_id);
		return false;
	}
	rc = config_setting_lookup_string(ue_param_setting, USIM_API_K,
			&u->usim_api_k);
	if (rc != 1) {
		printf("Check SIM USIM_API_K  section for UE%d. Exiting\n", user_id);
		return false;
	}
	rc = config_setting_lookup_string(ue_param_setting, OPC, &u->opc);
	if (rc != 1) {
		printf("Check SIM OPC section for UE%d. Exiting\n", user_id);
		return false;
	}
	rc = config_setting_lookup_string(ue_param_setting, MSISDN, &u->msisdn);
	if (rc != 1) {
		printf("Check SIM MSISDN section for UE%d. Exiting\n", user_id);
		return false;
	}
	return true;
}

void gen_usim_data(usim_data_conf_t *u, usim_data_t *usim_data,
                   const user_plmns_t *user_plmns, const networks_t networks) {
    int hplmn_index = get_plmn_index(u->hplmn, networks);
	const plmn_conf_param_t *conf = &networks.items[hplmn_index].conf;
	memset(usim_data, 0, sizeof(usim_data_t));
	usim_data->imsi.length = 8;
	usim_data->imsi.u.num.parity = get_msin_parity(u->msin,
		conf->mcc,
		conf->mnc);

	usim_data->imsi.u.num.digit1 = conf->mcc[0];
	usim_data->imsi.u.num.digit2 = conf->mcc[1];
	usim_data->imsi.u.num.digit3 = conf->mcc[2];

	usim_data->imsi.u.num.digit4 = conf->mnc[0];
	usim_data->imsi.u.num.digit5 = conf->mnc[1];

	if (strlen(conf->mnc) == 2) {
		usim_data->imsi.u.num.digit6 = u->msin[0];
		usim_data->imsi.u.num.digit7 = u->msin[1];
		usim_data->imsi.u.num.digit8 = u->msin[2];
		usim_data->imsi.u.num.digit9 = u->msin[3];
		usim_data->imsi.u.num.digit10 = u->msin[4];
		usim_data->imsi.u.num.digit11 = u->msin[5];
		usim_data->imsi.u.num.digit12 = u->msin[6];
		usim_data->imsi.u.num.digit13 = u->msin[7];
		usim_data->imsi.u.num.digit14 = u->msin[8];
		usim_data->imsi.u.num.digit15 = u->msin[9];
	} else {
		usim_data->imsi.u.num.digit6 = conf->mnc[2];
		usim_data->imsi.u.num.digit7 = u->msin[0];
		usim_data->imsi.u.num.digit8 = u->msin[1];
		usim_data->imsi.u.num.digit9 = u->msin[2];
		usim_data->imsi.u.num.digit10 = u->msin[3];
		usim_data->imsi.u.num.digit11 = u->msin[4];
		usim_data->imsi.u.num.digit12 = u->msin[5];
		usim_data->imsi.u.num.digit13 = u->msin[6];
		usim_data->imsi.u.num.digit14 = u->msin[7];
		usim_data->imsi.u.num.digit15 = u->msin[8];
	}

	/*
	 * Ciphering and Integrity Keys
	 */
	usim_data->keys.ksi = KSI;
	memset(&usim_data->keys.ck, 0, USIM_CK_SIZE);
	memset(&usim_data->keys.ik, 0, USIM_IK_SIZE);
	/*
	 * Higher Priority PLMN search period
	 */
	usim_data->hpplmn = 0x00; /* Disable timer */

	/*
	 * List of Forbidden PLMNs
	 */
	for (int i = 0; i < USIM_FPLMN_MAX; i++) {
		memset(&usim_data->fplmn[i], 0xff, sizeof(plmn_t));
	}
	if (user_plmns->forbiddens.size > 0) {
		for (int i = 0; i < user_plmns->forbiddens.size; i++) {
			usim_data->fplmn[i] = networks.items[user_plmns->forbiddens.items[i]].plmn;
		}
	}

	/*
	 * Location Information
	 */
	usim_data->loci.tmsi = DEFAULT_TMSI;
	usim_data->loci.lai.plmn = networks.items[hplmn_index].plmn;
	usim_data->loci.lai.lac = DEFAULT_LAC;
	usim_data->loci.status = USIM_LOCI_NOT_UPDATED;
	/*
	 * Packet Switched Location Information
	 */
	usim_data->psloci.p_tmsi = DEFAULT_P_TMSI;
	usim_data->psloci.signature[0] = 0x01;
	usim_data->psloci.signature[1] = 0x02;
	usim_data->psloci.signature[2] = 0x03;
	usim_data->psloci.rai.plmn = networks.items[hplmn_index].plmn;
	usim_data->psloci.rai.lac = DEFAULT_LAC;
	usim_data->psloci.rai.rac = DEFAULT_RAC;
	usim_data->psloci.status = USIM_PSLOCI_NOT_UPDATED;
	/*
	 * Administrative Data
	 */
	usim_data->ad.UE_Operation_Mode = USIM_NORMAL_MODE;
	usim_data->ad.Additional_Info = 0xffff;
	usim_data->ad.MNC_Length = strlen(conf->mnc);
	/*
	 * EPS NAS security context
	 */
	usim_data->securityctx.length = 52;
	usim_data->securityctx.KSIasme.type = USIM_KSI_ASME_TAG;
	usim_data->securityctx.KSIasme.length = 1;
	usim_data->securityctx.KSIasme.value[0] = KSI_ASME;
	usim_data->securityctx.Kasme.type = USIM_K_ASME_TAG;
	usim_data->securityctx.Kasme.length = USIM_K_ASME_SIZE;
	memset(usim_data->securityctx.Kasme.value, 0,
			usim_data->securityctx.Kasme.length);
	usim_data->securityctx.ulNAScount.type = USIM_UL_NAS_COUNT_TAG;
	usim_data->securityctx.ulNAScount.length = USIM_UL_NAS_COUNT_SIZE;
	memset(usim_data->securityctx.ulNAScount.value, 0,
			usim_data->securityctx.ulNAScount.length);
	usim_data->securityctx.dlNAScount.type = USIM_DL_NAS_COUNT_TAG;
	usim_data->securityctx.dlNAScount.length = USIM_DL_NAS_COUNT_SIZE;
	memset(usim_data->securityctx.dlNAScount.value, 0,
			usim_data->securityctx.dlNAScount.length);
	usim_data->securityctx.algorithmID.type = USIM_INT_ENC_ALGORITHMS_TAG;
	usim_data->securityctx.algorithmID.length = 1;
	usim_data->securityctx.algorithmID.value[0] = SECURITY_ALGORITHMS;
	/*
	 * Subcriber's Number
	 */
	usim_data->msisdn.length = 7;
	usim_data->msisdn.number.ext = 1;
	usim_data->msisdn.number.ton = MSISDN_TON_UNKNOWKN;
	usim_data->msisdn.number.npi = MSISDN_NPI_ISDN_TELEPHONY;
	usim_data->msisdn.conf1_record_id = 0xff; /* Not used */
	usim_data->msisdn.ext1_record_id = 0xff; /* Not used */
	int j = 0;
	for (int i = 0; i < strlen(u->msisdn); i += 2) {
		usim_data->msisdn.number.digit[j].msb = u->msisdn[i];
		j++;
	}
	j = 0;
	for (int i = 1; i < strlen(u->msisdn); i += 2) {
		usim_data->msisdn.number.digit[j].lsb = u->msisdn[i];
		j++;

	}
	if (strlen(u->msisdn) % 2 == 0) {
		for (int i = strlen(u->msisdn) / 2; i < 10; i++) {
			usim_data->msisdn.number.digit[i].msb = 0xf;
			usim_data->msisdn.number.digit[i].lsb = 0xf;
		}
	} else {
		usim_data->msisdn.number.digit[strlen(u->msisdn) / 2].lsb = 0xf;
		for (int i = (strlen(u->msisdn) / 2) + 1; i < 10; i++) {
			usim_data->msisdn.number.digit[i].msb = 0xf;
			usim_data->msisdn.number.digit[i].lsb = 0xf;
		}
	}
	/*
	 * PLMN Network Name and Operator PLMN List
	 */
	for (int i = 0; i < user_plmns->operators.size; i++) {
		network_record_t record = networks.items[user_plmns->operators.items[i]].record;
		usim_data->pnn[i].fullname.type = USIM_PNN_FULLNAME_TAG;
		usim_data->pnn[i].fullname.length = strlen(record.fullname);
		strncpy((char*) usim_data->pnn[i].fullname.value, record.fullname,
				usim_data->pnn[i].fullname.length);
		usim_data->pnn[i].shortname.type = USIM_PNN_SHORTNAME_TAG;
		usim_data->pnn[i].shortname.length = strlen(record.shortname);
		strncpy((char*) usim_data->pnn[i].shortname.value, record.shortname,
				usim_data->pnn[i].shortname.length);
		usim_data->opl[i].plmn = record.plmn;
		usim_data->opl[i].start = record.tac_start;
		usim_data->opl[i].end = record.tac_end;
		usim_data->opl[i].record_id = i;
	}
	if (user_plmns->operators.size < USIM_OPL_MAX) {
		for (int i = user_plmns->operators.size; i < USIM_OPL_MAX; i++) {
			memset(&usim_data->opl[i].plmn, 0xff, sizeof(plmn_t));
		}
	}

	/*
	 * List of Equivalent HPLMNs
	 */
	for (int i = 0; i < user_plmns->equivalents_home.size; i++) {
		usim_data->ehplmn[i] = networks.items[user_plmns->equivalents_home.items[i]].plmn;
	}
	if (user_plmns->equivalents_home.size < USIM_EHPLMN_MAX) {
		for (int i = user_plmns->equivalents_home.size; i < USIM_EHPLMN_MAX; i++) {
			memset(&usim_data->ehplmn[i], 0xff, sizeof(plmn_t));
		}
	}
	/*
	 * Home PLMN Selector with Access Technology
	 */
	usim_data->hplmn.plmn = networks.items[hplmn_index].plmn;
	usim_data->hplmn.AcT = (USIM_ACT_GSM | USIM_ACT_UTRAN | USIM_ACT_EUTRAN);

	/*
	 * List of user controlled PLMN selector with Access Technology
	 */
	for (int i = 0; i < USIM_PLMN_MAX; i++) {
		memset(&usim_data->plmn[i], 0xff, sizeof(plmn_t));
	}
	if (user_plmns->users_controlled.size > 0) {
		for (int i = 0; i < user_plmns->users_controlled.size; i++) {
			usim_data->plmn[i].plmn = networks.items[user_plmns->users_controlled.items[i]].plmn;
		}
	}

	// List of operator controlled PLMN selector with Access Technology

	for (int i = 0; i < USIM_OPLMN_MAX; i++) {
		memset(&usim_data->oplmn[i], 0xff, sizeof(plmn_t));
	}
	if (user_plmns->operators_controlled.size > 0) {
		for (int i = 0; i < user_plmns->operators_controlled.size; i++) {
			usim_data->oplmn[i].plmn = networks.items[user_plmns->operators_controlled.items[i]].plmn;
			usim_data->oplmn[i].AcT = (USIM_ACT_GSM | USIM_ACT_UTRAN
					| USIM_ACT_EUTRAN);
		}
	}
	/*
	 * EPS Location Information
	 */
	usim_data->epsloci.guti.gummei.plmn =
			networks.items[hplmn_index].plmn;
	usim_data->epsloci.guti.gummei.MMEgid = DEFAULT_MME_ID;
	usim_data->epsloci.guti.gummei.MMEcode = DEFAULT_MME_CODE;
	usim_data->epsloci.guti.m_tmsi = DEFAULT_M_TMSI;
	usim_data->epsloci.tai.plmn = usim_data->epsloci.guti.gummei.plmn;
	usim_data->epsloci.tai.tac = DEFAULT_TAC;
	usim_data->epsloci.status = USIM_EPSLOCI_UPDATED;
	/*
	 * Non-Access Stratum configuration
	 */
	usim_data->nasconfig.NAS_SignallingPriority.type =
	USIM_NAS_SIGNALLING_PRIORITY_TAG;
	usim_data->nasconfig.NAS_SignallingPriority.length = 1;
	usim_data->nasconfig.NAS_SignallingPriority.value[0] = 0x00;
	usim_data->nasconfig.NMO_I_Behaviour.type = USIM_NMO_I_BEHAVIOUR_TAG;
	usim_data->nasconfig.NMO_I_Behaviour.length = 1;
	usim_data->nasconfig.NMO_I_Behaviour.value[0] = 0x00;
	usim_data->nasconfig.AttachWithImsi.type = USIM_ATTACH_WITH_IMSI_TAG;
	usim_data->nasconfig.AttachWithImsi.length = 1;
#if defined(START_WITH_GUTI)
	usim_data->nasconfig.AttachWithImsi.value[0] = 0x00;
#else
	usim_data->nasconfig.AttachWithImsi.value[0] = 0x01;
#endif
	usim_data->nasconfig.MinimumPeriodicSearchTimer.type =
	USIM_MINIMUM_PERIODIC_SEARCH_TIMER_TAG;
	usim_data->nasconfig.MinimumPeriodicSearchTimer.length = 1;
	usim_data->nasconfig.MinimumPeriodicSearchTimer.value[0] = 0x00;
	usim_data->nasconfig.ExtendedAccessBarring.type =
	USIM_EXTENDED_ACCESS_BARRING_TAG;
	usim_data->nasconfig.ExtendedAccessBarring.length = 1;
	usim_data->nasconfig.ExtendedAccessBarring.value[0] = 0x00;
	usim_data->nasconfig.Timer_T3245_Behaviour.type =
	USIM_TIMER_T3245_BEHAVIOUR_TAG;
	usim_data->nasconfig.Timer_T3245_Behaviour.length = 1;
	usim_data->nasconfig.Timer_T3245_Behaviour.value[0] = 0x00;

        /* initialize the subscriber authentication security key */
        if (hex_string_to_hex_value(usim_data->keys.usim_api_k,
                                    u->usim_api_k, USIM_API_K_SIZE) == -1 ||
            hex_string_to_hex_value(usim_data->keys.opc,
                                    u->opc, OPC_SIZE) == -1) {
          fprintf(stderr, "fix your configuration file\n");
          exit(1);
        }
}

bool write_usim_data(const char *directory, int user_id, usim_data_t *usim_data){
    int rc;
    char *filename = make_filename(directory, USIM_API_NVRAM_FILENAME, user_id);
    rc = usim_api_write(filename, usim_data);
    free(filename);
    return rc;
}


