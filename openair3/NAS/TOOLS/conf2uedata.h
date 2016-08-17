#ifndef _CONF2UEDATA_H
#define _CONF2UEDATA_H

#include <libconfig.h>

#include "usim_api.h"

#define UE "UE"

#define PLMN "PLMN"

#define FULLNAME "FULLNAME"
#define SHORTNAME "SHORTNAME"
#define MNC "MNC"
#define MCC "MCC"

#define HPLMN "HPLMN"
#define UCPLMN "UCPLMN_LIST"
#define OPLMN "OPLMN_LIST"
#define OCPLMN "OCPLMN_LIST"
#define FPLMN "FPLMN_LIST"
#define EHPLMN "EHPLMN_LIST"

#define MIN_TAC     0x0000
#define MAX_TAC     0xFFFE

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

int get_config_from_file(const char *filename, config_t *config);
int parse_config_file(const char *output_dir, const char *filename);

void _display_usage(void);
void fill_network_record_list(void);

int parse_plmn_param(config_setting_t *plmn_setting, int index);
int parse_plmns(config_setting_t *all_plmn_setting);
int get_plmn_index(const char * mccmnc);
int parse_ue_plmn_param(config_setting_t *ue_setting, int user_id, const char **hplmn);
int fill_ucplmn(config_setting_t* setting, int use_id);
int fill_oplmn(config_setting_t* setting, int use_id);
int fill_ocplmn(config_setting_t* setting, int use_id);
int fill_fplmn(config_setting_t* setting, int use_id);
int fill_ehplmn(config_setting_t* setting, int use_id);

#endif // _CONF2UEDATA_H
