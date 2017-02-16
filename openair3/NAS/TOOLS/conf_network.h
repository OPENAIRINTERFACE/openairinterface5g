#ifndef _CONF_NETWORK_H
#define _CONF_NETWORK_H

#include <stdbool.h>
#include <libconfig.h>
#include "usim_api.h"

#define PLMN "PLMN"

#define FULLNAME "FULLNAME"
#define SHORTNAME "SHORTNAME"
#define MNC "MNC"
#define MCC "MCC"

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
    plmn_conf_param_t conf;
    network_record_t record;
    plmn_t plmn;
} network_t;

typedef struct {
    int size;
    network_t *items;
} networks_t;

bool parse_plmn_param(config_setting_t *plmn_setting, plmn_conf_param_t *conf);
bool parse_plmns(config_setting_t *all_plmn_setting, networks_t *plmns);

void gen_network_record_from_conf(const plmn_conf_param_t *conf, network_record_t *record);
int get_plmn_index(const char * mccmnc, const networks_t networks);

#endif
