#include <stdlib.h>
#include <string.h>

#include "userDef.h"
#include "usim_api.h"
#include "emmData.h"
#include "display.h"
#include "memory.h"
#include "fs.h"

#define PRINT_PLMN_DIGIT(d) if ((d) != 0xf) printf("%u", (d))

#define PRINT_PLMN(plmn)    \
    PRINT_PLMN_DIGIT((plmn).MCCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit3); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit3)

#define PRINT_PLMN_DIGIT(d) if ((d) != 0xf) printf("%u", (d))

#define PRINT_PLMN(plmn)    \
    PRINT_PLMN_DIGIT((plmn).MCCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit3); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit3)

// return number of files displayed
int display_data_from_directory(const char *directory, int flags) {
	int user_id = 0;
	char *filename;
	bool found = true;
  int displayed_count = 0;

	while ( found ) {
		found = false;

		if ( flags & DISPLAY_UEDATA ) {
			filename = get_ue_filename(directory, user_id);
			if ( file_exist_and_is_readable(filename) ) {
				display_ue_data(filename);
				displayed_count += 1;
				found = true;
				printf("UE identity data file: %s\n", filename);
			}
			free(filename);
		}

		if ( flags & DISPLAY_EMM ) {
			filename = get_emm_filename(directory, user_id);
			if ( file_exist_and_is_readable(filename) ) {
				display_emm_data(filename);
				displayed_count += 1;
				found = true;
				printf("EPS Mobility Management data file: %s\n", filename);
			}
			free(filename);
		}

		if ( flags & DISPLAY_USIM ) {
			filename = get_usim_filename(directory, user_id);
			if ( file_exist_and_is_readable(filename) ) {
				display_usim_data(filename);
				displayed_count += 1;
				found = true;
				printf("USIM data file: %s\n", filename);
			}
			free(filename);
		}

		user_id += 1;
	}
	return displayed_count;
}

void display_ue_data(const char *filename) {
	user_nvdata_t data;
	int rc;
	/*
	 * Read UE's non-volatile data
	 */
	memset(&data, 0, sizeof(user_nvdata_t));
	rc = memory_read(filename, &data, sizeof(user_nvdata_t));

	if (rc != RETURNok) {
		perror("ERROR\t: memory_read() failed");
		exit(false);
	}

	/*
	 * Display UE's non-volatile data
	 */
	printf("\nUE's non-volatile data:\n\n");
	printf("IMEI\t\t= %s\n", data.IMEI);
	printf("manufacturer\t= %s\n", data.manufacturer);
	printf("model\t\t= %s\n", data.model);
	printf("PIN\t\t= %s\n", data.PIN);
}

/*
 * Displays UE's non-volatile EMM data
 */
void display_emm_data(const char *filename) {

	int rc;
	emm_nvdata_t data;

	/*
	 * Read EMM non-volatile data
	 */
	memset(&data, 0, sizeof(emm_nvdata_t));
	rc = memory_read(filename, &data, sizeof(emm_nvdata_t));
	if (rc != RETURNok) {
		perror("ERROR\t: memory_read() failed ");
		exit(false);
	}

	/*
	 * Display EMM non-volatile data
	 */
	printf("\nEMM non-volatile data:\n\n");

	printf("IMSI\t\t= ");

	if (data.imsi.u.num.digit6 == 0b1111) {
		if (data.imsi.u.num.digit15 == 0b1111) {
			printf("%u%u%u.%u%u.%u%u%u%u%u%u%u%u\n", data.imsi.u.num.digit1,
					data.imsi.u.num.digit2, data.imsi.u.num.digit3,
					data.imsi.u.num.digit4, data.imsi.u.num.digit5,

					data.imsi.u.num.digit7, data.imsi.u.num.digit8,
					data.imsi.u.num.digit9, data.imsi.u.num.digit10,
					data.imsi.u.num.digit11, data.imsi.u.num.digit12,
					data.imsi.u.num.digit13, data.imsi.u.num.digit14);
		} else {
			printf("%u%u%u.%u%u.%u%u%u%u%u%u%u%u%u\n", data.imsi.u.num.digit1,
					data.imsi.u.num.digit2, data.imsi.u.num.digit3,
					data.imsi.u.num.digit4, data.imsi.u.num.digit5,

					data.imsi.u.num.digit7, data.imsi.u.num.digit8,
					data.imsi.u.num.digit9, data.imsi.u.num.digit10,
					data.imsi.u.num.digit11, data.imsi.u.num.digit12,
					data.imsi.u.num.digit13, data.imsi.u.num.digit14,
					data.imsi.u.num.digit15);
		}
	} else {
		if (data.imsi.u.num.digit15 == 0b1111) {
			printf("%u%u%u.%u%u%u.%u%u%u%u%u%u%u%u\n", data.imsi.u.num.digit1,
					data.imsi.u.num.digit2, data.imsi.u.num.digit3,
					data.imsi.u.num.digit4, data.imsi.u.num.digit5,
					data.imsi.u.num.digit6,

					data.imsi.u.num.digit7, data.imsi.u.num.digit8,
					data.imsi.u.num.digit9, data.imsi.u.num.digit10,
					data.imsi.u.num.digit11, data.imsi.u.num.digit12,
					data.imsi.u.num.digit13, data.imsi.u.num.digit14);
		} else {
			printf("%u%u%u.%u%u%u.%u%u%u%u%u%u%u%u%u\n", data.imsi.u.num.digit1,
					data.imsi.u.num.digit2, data.imsi.u.num.digit3,
					data.imsi.u.num.digit4, data.imsi.u.num.digit5,
					data.imsi.u.num.digit6,

					data.imsi.u.num.digit7, data.imsi.u.num.digit8,
					data.imsi.u.num.digit9, data.imsi.u.num.digit10,
					data.imsi.u.num.digit11, data.imsi.u.num.digit12,
					data.imsi.u.num.digit13, data.imsi.u.num.digit14,
					data.imsi.u.num.digit15);
		}
	}

	printf("RPLMN\t\t= ");
	PRINT_PLMN(data.rplmn);
	printf("\n");

	for (int i = 0; i < data.eplmn.n_plmns; i++) {
		printf("EPLMN[%d]\t= ", i);
		PRINT_PLMN(data.eplmn.plmn[i]);
		printf("\n");
	}
}

void display_usim_data(const char *filename) {

	int rc;
	usim_data_t data = { };

    rc = usim_api_read(filename, &data);

	if (rc != RETURNok) {
		perror("ERROR\t: usim_api_read() failed");
		exit(2);
	}

	/*
	 * Display USIM application data
	 */
	printf("\nUSIM data:\n\n");
	int digits;

	printf("Administrative Data:\n");
	printf("\tUE_Operation_Mode\t= 0x%.2x\n", data.ad.UE_Operation_Mode);
	printf("\tAdditional_Info\t\t= 0x%.4x\n", data.ad.Additional_Info);
	printf("\tMNC_Length\t\t= %d\n\n", data.ad.MNC_Length);

	printf("IMSI:\n");
	printf("\tlength\t= %d\n", data.imsi.length);
	printf("\tparity\t= %s\n",
			data.imsi.u.num.parity == EVEN_PARITY ? "Even" : "Odd");
	digits = (data.imsi.length * 2) - 1
			- (data.imsi.u.num.parity == EVEN_PARITY ? 1 : 0);
	printf("\tdigits\t= %d\n", digits);
	printf("\tdigits\t= %u%u%u%u%u%x%u%u%u%u",
			data.imsi.u.num.digit1, // MCC digit 1
			data.imsi.u.num.digit2, // MCC digit 2
			data.imsi.u.num.digit3, // MCC digit 3
			data.imsi.u.num.digit4, // MNC digit 1
			data.imsi.u.num.digit5, // MNC digit 2
			data.imsi.u.num.digit6, // MNC digit 3
			data.imsi.u.num.digit7, data.imsi.u.num.digit8,
			data.imsi.u.num.digit9, data.imsi.u.num.digit10);

	if (digits >= 11)
		printf("%x", data.imsi.u.num.digit11);

	if (digits >= 12)
		printf("%x", data.imsi.u.num.digit12);

	if (digits >= 13)
		printf("%x", data.imsi.u.num.digit13);

	if (digits >= 14)
		printf("%x", data.imsi.u.num.digit14);

	if (digits >= 15)
		printf("%x", data.imsi.u.num.digit15);

	printf("\n\n");

	printf("Ciphering and Integrity Keys:\n");
	printf("\tKSI\t: 0x%.2x\n", data.keys.ksi);
	char key[USIM_CK_SIZE + 1];
	key[USIM_CK_SIZE] = '\0';
	memcpy(key, data.keys.ck, USIM_CK_SIZE);
	printf("\tCK\t: \"%s\"\n", key);
	memcpy(key, data.keys.ik, USIM_IK_SIZE);
	printf("\tIK\t: \"%s\"\n", key);

        printf("\n\tusim_api_k:");
        for (int i = 0; i < 16; i++)
          printf(" %2.2x", data.keys.usim_api_k[i]);
        printf("\n\topc       :");
        for (int i = 0; i < 16; i++)
          printf(" %2.2x", data.keys.opc[i]);
        printf("\n\n");

	printf("EPS NAS security context:\n");
	printf("\tKSIasme\t: 0x%.2x\n", data.securityctx.KSIasme.value[0]);
	char kasme[USIM_K_ASME_SIZE + 1];
	kasme[USIM_K_ASME_SIZE] = '\0';
	memcpy(kasme, data.securityctx.Kasme.value, USIM_K_ASME_SIZE);
	printf("\tKasme\t: \"%s\"\n", kasme);
	/*printf("\tulNAScount\t: 0x%.8x\n",
			*(uint32_t*) data.securityctx.ulNAScount.value);
	printf("\tdlNAScount\t: 0x%.8x\n",
			*(uint32_t*) data.securityctx.dlNAScount.value);*/
	printf("\talgorithmID\t: 0x%.2x\n\n",
			data.securityctx.algorithmID.value[0]);

	printf("MSISDN\t= %u%u%u %u%u%u%u %u%u%u%u\n\n",
			data.msisdn.number.digit[0].msb, data.msisdn.number.digit[0].lsb,
			data.msisdn.number.digit[1].msb, data.msisdn.number.digit[1].lsb,
			data.msisdn.number.digit[2].msb, data.msisdn.number.digit[2].lsb,
			data.msisdn.number.digit[3].msb, data.msisdn.number.digit[3].lsb,
			data.msisdn.number.digit[4].msb, data.msisdn.number.digit[4].lsb,
			data.msisdn.number.digit[5].msb);

	for (int i = 0; i < USIM_PNN_MAX; i++) {
		printf("PNN[%d]\t= {%s, %s}\n", i, data.pnn[i].fullname.value,
				data.pnn[i].shortname.value);
	}

	printf("\n");

	for (int i = 0; i < USIM_OPL_MAX; i++) {
		printf("OPL[%d]\t= ", i);
		PRINT_PLMN(data.opl[i].plmn);
		printf(", TAC = [%.4x - %.4x], record_id = %d\n", data.opl[i].start,
				data.opl[i].end, data.opl[i].record_id);
	}

	printf("\n");

	printf("HPLMN\t\t= ");
	PRINT_PLMN(data.hplmn.plmn);
	printf(", AcT = 0x%x\n\n", data.hplmn.AcT);

	for (int i = 0; i < USIM_FPLMN_MAX; i++) {
		printf("FPLMN[%d]\t= ", i);
		PRINT_PLMN(data.fplmn[i]);
		printf("\n");
	}

	printf("\n");

	for (int i = 0; i < USIM_EHPLMN_MAX; i++) {
		printf("EHPLMN[%d]\t= ", i);
		PRINT_PLMN(data.ehplmn[i]);
		printf("\n");
	}

	printf("\n");

	for (int i = 0; i < USIM_PLMN_MAX; i++) {
		printf("PLMN[%d]\t\t= ", i);
		PRINT_PLMN(data.plmn[i].plmn);
		printf(", AcTPLMN = 0x%x", data.plmn[i].AcT);
		printf("\n");
	}

	printf("\n");

	for (int i = 0; i < USIM_OPLMN_MAX; i++) {
		printf("OPLMN[%d]\t= ", i);
		PRINT_PLMN(data.oplmn[i].plmn);
		printf(", AcTPLMN = 0x%x", data.oplmn[i].AcT);
		printf("\n");
	}

	printf("\n");

	printf("HPPLMN\t\t= 0x%.2x (%d minutes)\n\n", data.hpplmn, data.hpplmn);

	printf("LOCI:\n");
	printf("\tTMSI = 0x%.4x\n", data.loci.tmsi);
	printf("\tLAI\t: PLMN = ");
	PRINT_PLMN(data.loci.lai.plmn);
	printf(", LAC = 0x%.2x\n", data.loci.lai.lac);
	printf("\tstatus\t= %d\n\n", data.loci.status);

	printf("PSLOCI:\n");
	printf("\tP-TMSI = 0x%.4x\n", data.psloci.p_tmsi);
	printf("\tsignature = 0x%x 0x%x 0x%x\n", data.psloci.signature[0],
			data.psloci.signature[1], data.psloci.signature[2]);
	printf("\tRAI\t: PLMN = ");
	PRINT_PLMN(data.psloci.rai.plmn);
	printf(", LAC = 0x%.2x, RAC = 0x%.1x\n", data.psloci.rai.lac,
			data.psloci.rai.rac);
	printf("\tstatus\t= %d\n\n", data.psloci.status);

	printf("EPSLOCI:\n");
	printf("\tGUTI\t: GUMMEI\t: (PLMN = ");
	PRINT_PLMN(data.epsloci.guti.gummei.plmn);
	printf(", MMEgid = 0x%.2x, MMEcode = 0x%.1x)",
			data.epsloci.guti.gummei.MMEgid, data.epsloci.guti.gummei.MMEcode);
	printf(", M-TMSI = 0x%.4x\n", data.epsloci.guti.m_tmsi);
	printf("\tTAI\t: PLMN = ");
	PRINT_PLMN(data.epsloci.tai.plmn);
	printf(", TAC = 0x%.2x\n", data.epsloci.tai.tac);
	printf("\tstatus\t= %d\n\n", data.epsloci.status);

	printf("NASCONFIG:\n");
	printf("\tNAS_SignallingPriority\t\t: 0x%.2x\n",
			data.nasconfig.NAS_SignallingPriority.value[0]);
	printf("\tNMO_I_Behaviour\t\t\t: 0x%.2x\n",
			data.nasconfig.NMO_I_Behaviour.value[0]);
	printf("\tAttachWithImsi\t\t\t: 0x%.2x\n",
			data.nasconfig.AttachWithImsi.value[0]);
	printf("\tMinimumPeriodicSearchTimer\t: 0x%.2x\n",
			data.nasconfig.MinimumPeriodicSearchTimer.value[0]);
	printf("\tExtendedAccessBarring\t\t: 0x%.2x\n",
			data.nasconfig.ExtendedAccessBarring.value[0]);
	printf("\tTimer_T3245_Behaviour\t\t: 0x%.2x\n",
			data.nasconfig.Timer_T3245_Behaviour.value[0]);

}


