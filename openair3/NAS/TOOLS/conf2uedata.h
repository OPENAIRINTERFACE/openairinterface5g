#ifndef _CONF2UEDATA_H
#define _CONF2UEDATA_H

#include <libconfig.h>

#include "usim_api.h"
#include "conf_network.h"

#define UE "UE"

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

int get_config_from_file(const char *filename, config_t *config);
int parse_config_file(const char *output_dir, const char *filename);

void _display_usage(void);

int parse_user_plmns_conf(config_setting_t *ue_setting, int user_id,
                          user_plmns_t *user_plmns, const char **h,
                          const networks_t networks);
int parse_Xplmn(config_setting_t *ue_setting, const char *section,
               int user_id, plmns_list *plmns, const networks_t networks);


#endif // _CONF2UEDATA_H
