#ifndef _CONF_PARSER_H
#define _CONF_PARSER_H

#include <stdbool.h>
#include <libconfig.h>

#define UE "UE"

bool get_config_from_file(const char *filename, config_t *config);
bool parse_config_file(const char *output_dir, const char *filename);

#endif
