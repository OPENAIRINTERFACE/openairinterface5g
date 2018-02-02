#ifndef _CONF_USER_H
#define _CONF_USER_H

#include <stdbool.h>
#include <libconfig.h>
#include "conf_network.h"

#define HPLMN "HPLMN"
#define UCPLMN "UCPLMN_LIST"
#define OPLMN "OPLMN_LIST"
#define OCPLMN "OCPLMN_LIST"
#define FPLMN "FPLMN_LIST"
#define EHPLMN "EHPLMN_LIST"

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

bool parse_user_plmns_conf(config_setting_t *ue_setting, int user_id,
                          user_plmns_t *user_plmns, const char **h,
                          const networks_t networks);

bool parse_Xplmn(config_setting_t *ue_setting, const char *section,
               int user_id, plmns_list *plmns, const networks_t networks);

void user_plmns_free(user_plmns_t *user_plmns);

#endif
