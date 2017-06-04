#ifndef _CONF_PARSER_H
#define _CONF_PARSER_H

#include <stdbool.h>
#include <libconfig.h>

#define UE "UE"

#define OUTPUT_EMM     1
#define OUTPUT_USIM    2
#define OUTPUT_UEDATA  4
#define OUTPUT_ALL     7

bool get_config_from_file(const char *filename, config_t *config);
bool parse_config_file(const char *output_dir, const char *filename, int output_flags);

#endif
