#ifndef _CONF2UEDATA_H
#define _CONF2UEDATA_H

#include <libconfig.h>

#include "usim_api.h"
#include "conf_network.h"

#define UE "UE"

bool get_config_from_file(const char *filename, config_t *config);
bool parse_config_file(const char *output_dir, const char *filename);

void _display_usage(void);

#endif // _CONF2UEDATA_H
