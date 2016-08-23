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

typedef struct {
    int size;
    int *items;
} plmns_list;

typedef struct {
    plmns_list users_controlled;
    plmns_list operators;
    plmns_list operators_controlled;
    plmns_list forbiddens;
    plmns_list equivalents_home;
} user_plmns_t;

typedef struct {
    plmn_conf_param_t conf;
    network_record_t record;
    plmn_t plmn;
} network_t;

typedef struct {
    int size;
    network_t *items;
} networks_t;

int get_config_from_file(const char *filename, config_t *config);
int parse_config_file(const char *output_dir, const char *filename);

void _display_usage(void);
void gen_network_record_from_conf(const plmn_conf_param_t *conf, network_record_t *record);

int parse_plmn_param(config_setting_t *plmn_setting, plmn_conf_param_t *conf);
int parse_plmns(config_setting_t *all_plmn_setting, networks_t *plmns);
int get_plmn_index(const char * mccmnc, const networks_t networks);
int parse_user_plmns_conf(config_setting_t *ue_setting, int user_id,
                          user_plmns_t *user_plmns, const char **h,
                          const networks_t networks);
int parse_Xplmn(config_setting_t *ue_setting, const char *section,
               int user_id, plmns_list *plmns, const networks_t networks);


#endif // _CONF2UEDATA_H
