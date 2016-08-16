#ifndef _CONF2UEDATA_H
#define _CONF2UEDATA_H

#include <libconfig.h>
#include "emmData.h"
#include "usim_api.h"
#include "userDef.h"
#include "memory.h"
#include "getopt.h"

#define USER "USER"
#define UE "UE"
#define SIM "SIM"
#define PLMN "PLMN"

#define FULLNAME "FULLNAME"
#define SHORTNAME "SHORTNAME"
#define MNC "MNC"
#define MCC "MCC"

#define MSIN "MSIN"
#define USIM_API_K "USIM_API_K"
#define OPC "OPC"
#define MSISDN "MSISDN"

#define UE_IMEI "IMEI"
#define MANUFACTURER "MANUFACTURER"
#define MODEL "MODEL"
#define PINCODE "PIN"

#define HPLMN "HPLMN"
#define UCPLMN "UCPLMN_LIST"
#define OPLMN "OPLMN_LIST"
#define OCPLMN "OCPLMN_LIST"
#define FPLMN "FPLMN_LIST"
#define EHPLMN "EHPLMN_LIST"

#define KSI     USIM_KSI_NOT_AVAILABLE
#define PRINT_PLMN_DIGIT(d) if ((d) != 0xf) printf("%u", (d))

#define PRINT_PLMN(plmn)    \
    PRINT_PLMN_DIGIT((plmn).MCCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit3); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit3)


#define KSI_ASME    USIM_KSI_NOT_AVAILABLE
#define INT_ALGO    USIM_INT_EIA1
#define ENC_ALGO    USIM_ENC_EEA0
#define SECURITY_ALGORITHMS (ENC_ALGO | INT_ALGO)
#define OPC_SIZE	16
#define MIN_TAC     0x0000
#define MAX_TAC     0xFFFE

#define DEFAULT_TMSI    0x0000000D
#define DEFAULT_P_TMSI    0x0000000D
#define DEFAULT_M_TMSI    0x0000000D
#define DEFAULT_LAC   0xFFFE
#define DEFAULT_RAC   0x01
#define DEFAULT_TAC   0x0001

#define DEFAULT_MME_ID    0x0102
#define DEFAULT_MME_CODE  0x0F

#define PRINT_PLMN_DIGIT(d) if ((d) != 0xf) printf("%u", (d))

#define PRINT_PLMN(plmn)    \
    PRINT_PLMN_DIGIT((plmn).MCCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit3); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit3)

/*
 * PLMN network operator record
 */
typedef struct {
  unsigned int num;
  plmn_t plmn;
  char fullname[NET_FORMAT_LONG_SIZE + 1];
  char shortname[NET_FORMAT_SHORT_SIZE + 1];
  tac_t tac_start;
  tac_t tac_end;
} network_record_t;

typedef struct {
	const char *fullname;
	const char *shortname;
	const char *mnc;
	const char *mcc;
} plmn_conf_param_t;

extern const char* output_dir;

extern const char *msin;
extern const char *usim_api_k;
extern const char *msisdn;
extern const char *opc;
extern const char *hplmn;

extern int *ucplmn;
extern int *oplmn;
extern int *ocplmn;
extern int *fplmn;
extern int *ehplmn;

extern int hplmn_index;
extern int plmn_nb;
extern int ucplmn_nb;
extern int oplmn_nb;
extern int ocplmn_nb;
extern int fplmn_nb;
extern int ehplmn_nb;

extern plmn_conf_param_t* user_plmn_list;
extern network_record_t* user_network_record_list;

char *make_filename(const char *output_dir, const char *filename, int ueid);
int get_config_from_file(const char *filename, config_t *config);
int parse_config_file(const char *filename);

void _display_usim_data(int user_id);

void _display_usage(void);
void gen_emm_data(int user_id) ;
void gen_usim_data(int user_id);
void fill_network_record_list(void);

int _luhn(const char* cc);

void _display_ue_data(int user_id);

void _display_emm_data(int user_id);
int parse_ue_user_param(config_setting_t *ue_setting, int user_id);
int parse_ue_sim_param(config_setting_t *ue_setting, int user_id);
int parse_plmn_param(config_setting_t *plmn_setting, int index);
int parse_plmns(config_setting_t *all_plmn_setting);
int get_msin_parity(const char * msin);
int get_plmn_index(const char * mccmnc);
int parse_ue_plmn_param(config_setting_t *ue_setting, int user_id);
int fill_ucplmn(config_setting_t* setting, int use_id);
int fill_oplmn(config_setting_t* setting, int use_id);
int fill_ocplmn(config_setting_t* setting, int use_id);
int fill_fplmn(config_setting_t* setting, int use_id);
int fill_ehplmn(config_setting_t* setting, int use_id);

#endif // _CONF2UEDATA_H
