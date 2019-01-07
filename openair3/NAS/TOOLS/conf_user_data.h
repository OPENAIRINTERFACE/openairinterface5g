#ifndef _CONF_USER_DATA_H
#define _CONF_USER_DATA_H

#include <stdbool.h>
#include <libconfig.h>
#include "userDef.h"

#define USER "USER"
#define MANUFACTURER "MANUFACTURER"
#define MODEL "MODEL"
#define UE_IMEI "IMEI"
#define PINCODE "PIN"

typedef struct {
	const char* imei;
	const char* manufacturer;
	const char* model;
	const char* pin;
} user_data_conf_t;

void gen_user_data(user_data_conf_t *u, user_nvdata_t *user_data);
bool write_user_data(const char *directory, int user_id, user_nvdata_t *data);
int parse_ue_user_data(config_setting_t *ue_setting, int user_id, user_data_conf_t *u);

int _luhn(const char* cc);

#endif
